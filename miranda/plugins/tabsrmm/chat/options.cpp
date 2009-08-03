/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

Chat module plugin for Miranda IM

Copyright (C) 2003 J�rgen Persson

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

$Id: options.c 10402 2009-07-24 00:35:21Z silvercircle $

*/

#include "../src/commonheaders.h"
#undef Translate

#define TranslateA(s)   ((char*)CallService(MS_LANGPACK_TRANSLATESTRING,0,(LPARAM)(s)))

#if !defined(_UNICODE)
	#undef TranslateT
	#undef TranslateTS
	#define TranslateT(s)	((char*)CallService(MS_LANGPACK_TRANSLATESTRING,0,(LPARAM)(s)))
	#define TranslateTS(s)	((char*)CallService(MS_LANGPACK_TRANSLATESTRING,0,(LPARAM)(s)))
#endif

#include <shlobj.h>
#include <shlwapi.h>

extern HBRUSH 			hListBkgBrush;
extern HICON			hIcons[30];
extern FONTINFO			aFonts[OPTIONS_FONTCOUNT];
extern SESSION_INFO		g_TabSession;
extern int              g_chat_integration_enabled;
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

extern LOGFONT lfDefault;

//remember to put these in the Translate( ) template file too
static struct FontOptionsList CHAT_fontOptionsList[] = {
	{LPGENT("Timestamp"), RGB(50, 50, 240), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Others nicknames"), RGB(0, 0, 192), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT("Your nickname"), RGB(0, 0, 192), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT("User has joined"), RGB(90, 160, 90), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("User has left"), RGB(160, 160, 90), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("User has disconnected"), RGB(160, 90, 90), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("User kicked ..."), RGB(100, 100, 100), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("User is now known as ..."), RGB(90, 90, 160), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Notice from user"), RGB(160, 130, 60), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Incoming message"), RGB(90, 90, 90), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Outgoing message"), RGB(90, 90, 90), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("The topic is ..."), RGB(70, 70, 160), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Information messages"), RGB(130, 130, 195), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("User enables status for ..."), RGB(70, 150, 70), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("User disables status for ..."), RGB(150, 70, 70), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Action message"), RGB(160, 90, 160), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Highlighted message"), RGB(180, 150, 80), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Chat log symbols (Webdings)"), RGB(170, 170, 170), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("User list members (Online)"), RGB(0, 0, 0), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("User list members (away)"), RGB(170, 170, 170), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
};

const int msgDlgFontCount = SIZEOF(CHAT_fontOptionsList);

static struct FontOptionsList IM_fontOptionsList[] = {
	{LPGENT(">> Outgoing messages"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT(">> Outgoing misc events"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("<< Incoming messages"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("<< Incoming misc events"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT(">> Outgoing name"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT(">> Outgoing timestamp"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT("<< Incoming name"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT("<< Incoming timestamp"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT(">> Outgoing messages (old)"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT(">> Outgoing misc events (old)"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("<< Incoming messages (old)"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("<< Incoming misc events (old)"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT(">> Outgoing name (old)"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT(">> Outgoing timestamp (old)"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT("<< Incoming name (old)"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT("<< Incoming timestamp (old)"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, FONTF_BOLD, -12},
	{LPGENT("* Message Input Area"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("* Status changes"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("* Dividers"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("* Error and warning Messages"), RGB(50, 50, 50), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("* Symbols (incoming)"), RGB(50, 50, 50), _T("Webdings"), SYMBOL_CHARSET, 0, -12},
	{LPGENT("* Symbols (outgoing)"), RGB(50, 50, 50), _T("Webdings"), SYMBOL_CHARSET, 0, -12},
};

static struct FontOptionsList IP_fontOptionsList[] = {
	{LPGENT("Nickname"), RGB(0, 0, 0), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("UIN"), RGB(0, 0, 0), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Status"), RGB(0, 0, 0), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Protocol"), RGB(0, 0, 0), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Contacts local time"), RGB(0, 0, 0), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
	{LPGENT("Window caption (skinned mode)"), RGB(255, 255, 255), lfDefault.lfFaceName, DEFAULT_CHARSET, 0, -12},
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
	{LPGENT("Open new chat rooms in the default container"), "DefaultContainer", 0, 1, NULL},
	{LPGENT("Flash window when someone speaks"), "FlashWindow", 0, 0, NULL},
	{LPGENT("Flash window when a word is highlighted"), "FlashWindowHighlight", 0, 1, NULL},
//MAD: highlight mod
	{LPGENT("Create container/tab on highlight, if it's not available"), "CreateWindowOnHighlight", 0,0, NULL},
	{LPGENT("Activate chat window on highlight"), "AnnoyingHighlight", 0,0, NULL},
//
	{LPGENT("Show list of users in the chat room"), "ShowNicklist", 0, 1, NULL},
	{LPGENT("Colorize nicknames in member list"), "ColorizeNicks", 0, 1, NULL},
	{LPGENT("Show button menus when right clicking the buttons"), "RightClickFilter", 0, 0, NULL},
	{LPGENT("Show the topic of the room on your contact list (if supported)"), "TopicOnClist", 0, 1, NULL},
	{LPGENT("Do not play sounds when the chat room is focused"), "SoundsFocus", 0, 0, NULL},
	{LPGENT("Do not pop up the window when joining a chat room"), "PopupOnJoin", 0, 0, NULL},
	{LPGENT("Toggle the visible state when double clicking in the contact list"), "ToggleVisibility", 0, 0, NULL},
	{LPGENT("Sync splitter position with standard IM sessions"), "SyncSplitter", 0, 0, NULL},
	{LPGENT("Show contact statuses if protocol supports them"), "ShowContactStatus", 0, 1, NULL},
	{LPGENT("Display contact status icon before user role icon"), "ContactStatusFirst", 0, 0, NULL},
	//MAD: simple customization improvement
	{LPGENT("Use IRC style status indicators in the member list (@, %, + etc.)"), "ClassicIndicators", 0, 0, NULL},
	{LPGENT("Use alternative sorting method in member list"), "AlternativeSorting", 0, 1, NULL},

};
static struct branch_t branch2[] = {
	{LPGENT("Prefix all events with a timestamp"), "ShowTimeStamp", 0, 1, NULL},
	{LPGENT("Only prefix with timestamp if it has changed"), "ShowTimeStampIfChanged", 0, 0, NULL},
	{LPGENT("Timestamp has same colour as the event"), "TimeStampEventColour", 0, 0, NULL},
	{LPGENT("Indent the second line of a message"), "LogIndentEnabled", 0, 1, NULL},
	{LPGENT("Limit user names in the message log to 20 characters"), "LogLimitNames", 0, 1, NULL},
	{LPGENT("Add \':\' to auto-completed user names"), "AddColonToAutoComplete", 0, 1, NULL},
	{LPGENT("Start private conversation on doubleclick in nick list (insert nick if unchecked)"), "DoubleClick4Privat", 0, 0, NULL},
	{LPGENT("Strip colors from messages in the log"), "StripFormatting", 0, 0, NULL},
	{LPGENT("Enable the \'event filter\' for new rooms"), "FilterEnabled", 0, 0, NULL},
//MAD: simple customization improvement
	{LPGENT("Use IRC style status indicators in the log"), "LogClassicIndicators", 0,0, NULL},
//
	{LPGENT("Use text symbols instead of icons in the chat log (faster)"), "LogSymbols", 0, 1, NULL},
	{LPGENT("Make nicknames clickable hyperlinks"), "ClickableNicks", 0, 1, NULL},
	{LPGENT("Colorize nicknames in message log"), "ColorizeNicksInLog", 0, 1, NULL},
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
	{LPGENT("Use bold form for nickname (requires BBCode support)"), "BBCodeInPopups", 0, 0, NULL},
};

static struct branch_t branch7[] = {
        {LPGENT("Log topic changes"), "DiskLogFlags", GC_EVENT_TOPIC, 0, NULL},
        {LPGENT("Log users joining"), "DiskLogFlags", GC_EVENT_JOIN, 0, NULL},
        {LPGENT("Log users disconnecting"), "DiskLogFlags", GC_EVENT_QUIT, 0, NULL},
        {LPGENT("Log messages"), "DiskLogFlags", GC_EVENT_MESSAGE, 0, NULL},
        {LPGENT("Log actions"), "DiskLogFlags", GC_EVENT_ACTION, 0, NULL},
        {LPGENT("Log highlights"), "DiskLogFlags", GC_EVENT_HIGHLIGHT, 0, NULL},
        {LPGENT("Log users leaving"), "DiskLogFlags", GC_EVENT_PART, 0, NULL},
        {LPGENT("Log users kicking other user"), "DiskLogFlags", GC_EVENT_KICK, 0, NULL},
        {LPGENT("Log notices "), "DiskLogFlags", GC_EVENT_NOTICE, 0, NULL},
        {LPGENT("Log name changes"), "DiskLogFlags", GC_EVENT_NICK, 0, NULL},
        {LPGENT("Log information messages"), "DiskLogFlags", GC_EVENT_INFORMATION, 0, NULL},
        {LPGENT("Log status changes"), "DiskLogFlags", GC_EVENT_ADDSTATUS, 0, NULL},
};

static HWND hPathTip = 0;

void LoadMsgDlgFont(int section, int i, LOGFONT *lf, COLORREF* colour, char *szMod)
{
	char str[32];
	int style;
	DBVARIANT dbv;
	int j = (i >= 100 ? i - 100 : i);

	struct FontOptionsList *fol = fontOptionsList;
	switch (section)
	{
		case FONTSECTION_CHAT:	fol = CHAT_fontOptionsList;	break;
		case FONTSECTION_IM:	fol = IM_fontOptionsList;	break;
		case FONTSECTION_IP:	fol = IP_fontOptionsList;	break;
	}

	if (colour) {
		wsprintfA(str, "Font%dCol", i);
		*colour = M->GetDword(szMod, str, fol[j].defColour);
	}
	if (lf) {
		wsprintfA(str, "Font%dSize", i);
		lf->lfHeight = (char) M->GetByte(szMod, str, fol[j].defSize);
		lf->lfWidth = 0;
		lf->lfEscapement = 0;
		lf->lfOrientation = 0;
		wsprintfA(str, "Font%dSty", i);
		style = M->GetByte(szMod, str, fol[j].defStyle);
		lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
		lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
		lf->lfUnderline = style & FONTF_UNDERLINE ? 1 : 0;
		lf->lfStrikeOut =  style & FONTF_STRIKEOUT ? 1 : 0;
		wsprintfA(str, "Font%dSet", i);
		lf->lfCharSet = M->GetByte(szMod, str, fol[j].defCharset);
		lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf->lfQuality = DEFAULT_QUALITY;
		lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		wsprintfA(str, "Font%d", i);
		if (i == 17 && !strcmp(szMod, CHAT_FONTMODULE)) {
			lf->lfCharSet = SYMBOL_CHARSET;
			lstrcpyn(lf->lfFaceName, _T("Webdings"), SIZEOF(lf->lfFaceName));
		} else {
			if (M->GetTString(NULL, szMod, str, &dbv)) {
				lstrcpy(lf->lfFaceName, fol[j].szDefFace);
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
			iState = ((M->GetDword("Chat", branch[i].szDBName, defaultval) & branch[i].iMode) & branch[i].iMode) != 0 ? 3 : 2;
		else
			iState = M->GetByte("Chat", branch[i].szDBName, branch[i].bDefault) != 0 ? 3 : 2;
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
		bChecked = ((tvi.state & TVIS_STATEIMAGEMASK) >> 12 == 2) ? 0 : 1;
		if (branch[i].iMode) {
			if (bChecked)
				iState |= branch[i].iMode;
			if (iState & GC_EVENT_ADDSTATUS)
				iState |= GC_EVENT_REMOVESTATUS;
			M->WriteDword("Chat", branch[i].szDBName, (DWORD)iState);
		} else M->WriteByte("Chat", branch[i].szDBName, bChecked);
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
			if (((tvi.state&TVIS_STATEIMAGEMASK) >> 12 == 2))
				bChecked = FALSE;
		}
		tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
	}
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.state = INDEXTOSTATEIMAGEMASK(bChecked ? 3 : 2);
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
	if (((tvi.state&TVIS_STATEIMAGEMASK) >> 12 == 3)||((tvi.state & TVIS_STATEIMAGEMASK) >> 12 == 1))
		bChecked = FALSE;

	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.state = INDEXTOSTATEIMAGEMASK(bChecked ? 2 : 1);
	TreeView_SetItem(hwndTree, &tvi);
	tvi.hItem = TreeView_GetNextItem(hwndTree, hHeading, TVGN_CHILD);
	while (tvi.hItem) {
		tvi.state = INDEXTOSTATEIMAGEMASK(bChecked ? 3:2);
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
		LoadMsgDlgFont(FONTSECTION_CHAT, i, &aFonts[i].lf, &aFonts[i].color, CHAT_FONTMODULE);
}

static struct _tagicons { char *szDesc; char *szName; int id; UINT size;} _icons[] = {
	LPGEN("Window Icon"), "chat_window", IDI_CHANMGR, 16,
// 	LPGEN("Background colour"), "chat_bkgcol", IDI_BKGCOLOR, 16,
// 	LPGEN("Room settings"), "chat_settings", IDI_TOPICBUT, 16,
// 	LPGEN("Event filter disabled"), "chat_filter", IDI_FILTER, 16,
// 	LPGEN("Event filter enabled"), "chat_filter2", IDI_FILTER2, 16,
	LPGEN("Icon overlay"), "chat_overlay", IDI_OVERLAY, 16,
// 	LPGEN("Show nicklist"), "chat_shownicklist", IDI_SHOWNICKLIST, 16,
// 	LPGEN("Hide nicklist"), "chat_hidenicklist", IDI_HIDENICKLIST, 16,

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
		sid.pszSection = TranslateA("TabSRMM/Group chat windows");
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
		sid.pszSection = TranslateA("TabSRMM/Group chat log");
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
	if (!M->GetTString(NULL, "Chat", pszSetting, &dbv)) {
		replaceStr(ppPointer, dbv.ptszVal);
		DBFreeVariant(&dbv);
	} else
		replaceStr(ppPointer, pszDefault);
}

#define OPT_FIXHEADINGS (WM_USER+1)

static UINT _o1controls[] = {IDC_CHECKBOXES, IDC_GROUP, IDC_STATIC_ADD, 0};

HWND CreateToolTip(HWND hwndParent, LPTSTR ptszText, LPTSTR ptszTitle)
{
	TOOLINFO ti = { 0 };
	HWND hwndTT;
	hwndTT = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hwndParent, NULL, g_hInst, NULL);

	SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS | TTF_CENTERTIP;
	ti.hwnd = hwndParent;
	ti.hinst = g_hInst;
	ti.lpszText = ptszText;
	GetClientRect (hwndParent, &ti.rect);
	ti.rect.left =- 85;

	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
	SendMessage(hwndTT, TTM_SETTITLE, 1, (LPARAM)ptszTitle);
	return hwndTT;
}

INT_PTR CALLBACK DlgProcOptions1(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HTREEITEM hListHeading1 = 0;
	static HTREEITEM hListHeading2 = 0;
	static HTREEITEM hListHeading3 = 0;
	static HTREEITEM hListHeading4 = 0;
	static HTREEITEM hListHeading5 = 0;
	static HTREEITEM hListHeading6 = 0;
	static HTREEITEM hListHeading7 = 0;

	switch (uMsg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			if (g_chat_integration_enabled) {
				HIMAGELIST himlOptions;

				SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHECKBOXES), GWL_STYLE, GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHECKBOXES), GWL_STYLE) | TVS_NOHSCROLL | TVS_CHECKBOXES);

				himlOptions = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_CHECKBOXES, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());
				if (himlOptions)
					ImageList_Destroy(himlOptions);

				hListHeading1 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Appearance and functionality of chat room windows"), M->GetByte("Chat", "Branch1Exp", 0) ? TRUE : FALSE);
				hListHeading2 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Appearance of the message log"), M->GetByte("Chat", "Branch2Exp", 0) ? TRUE : FALSE);
				hListHeading3 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Default events to show in new chat rooms if the \'event filter\' is enabled"), M->GetByte("Chat", "Branch3Exp", 0) ? TRUE : FALSE);
				hListHeading4 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Icons to display in the message log"), M->GetByte("Chat", "Branch4Exp", 0) ? TRUE : FALSE);
				hListHeading5 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Icons to display in the tray and the message window tabs / title"), M->GetByte("Chat", "Branch5Exp", 0) ? TRUE : FALSE);
				hListHeading7 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Log these events to the log file (when file logging is enabled)"), M->GetByte("Chat", "Branch7Exp", 0) ? TRUE : FALSE);

				if (PluginConfig.g_PopupAvail)
					hListHeading6 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Pop-ups to display"), M->GetByte("Chat", "Branch6Exp", 0) ? TRUE : FALSE);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading1, branch1, SIZEOF(branch1), 0x0000);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2, branch2, SIZEOF(branch2), 0x0000);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3, branch3, SIZEOF(branch3), 0x03E0);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4, branch4, SIZEOF(branch4), 0x0000);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5, branch5, SIZEOF(branch5), 0x1000);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6, branch6, SIZEOF(branch6), 0x0000);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading7, branch7, SIZEOF(branch7), 0xFFFFF);

				SendMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);
				{
					TCHAR* pszGroup = NULL;
					InitSetting(&pszGroup, "AddToGroup", _T("Chat rooms"));
					SetWindowText(GetDlgItem(hwndDlg, IDC_GROUP), pszGroup);
					mir_free(pszGroup);
					ShowWindow(GetDlgItem(hwndDlg, IDC_STATIC_MESSAGE), SW_HIDE);
				}
			} else {
				int i = 0;

				while (_o1controls[i])
					ShowWindow(GetDlgItem(hwndDlg, _o1controls[i++]), SW_HIDE);
			}
			CheckDlgButton(hwndDlg, IDC_CHAT_ENABLE, M->GetByte("enable_chat", 1));
			break;

		case OPT_FIXHEADINGS:
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading1);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading7);
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_CHAT_ENABLE) {
				M->WriteByte("PluginDisable", "chat.dll", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_CHAT_ENABLE) ? 1 : 0));
				MessageBox(0, TranslateT("You should now immediatly restart Miranda to make this change take effect."), TranslateT("tabSRMM Message"), MB_OK);
			}
			if ((LOWORD(wParam) == IDC_GROUP)
					&& (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))	return 0;

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
								else if (tvi.hItem == hListHeading7)
									CheckBranches(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading7);
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

								iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_GROUP));
								if (iLen > 0) {
									pszText = (TCHAR *)realloc(pszText, (iLen + 2) * sizeof(TCHAR));
									GetDlgItemText(hwndDlg, IDC_GROUP, pszText, iLen + 1);
									M->WriteTString(NULL, "Chat", "AddToGroup", pszText);
								} else
									M->WriteTString(NULL, "Chat", "AddToGroup", _T(""));

								if (pszText)
									free(pszText);

								b = M->GetByte("Chat", "Tabs", 1);
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch1, sizeof(branch1) / sizeof(branch1[0]));
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch2, sizeof(branch2) / sizeof(branch2[0]));
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch3, sizeof(branch3) / sizeof(branch3[0]));
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch4, sizeof(branch4) / sizeof(branch4[0]));
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch5, sizeof(branch5) / sizeof(branch5[0]));
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch7, SIZEOF(branch7));

								if (PluginConfig.g_PopupAvail)
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
			M->WriteByte("Chat", "Branch1Exp", b);
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
			M->WriteByte("Chat", "Branch2Exp", b);
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
			M->WriteByte("Chat", "Branch3Exp", b);
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
			M->WriteByte("Chat", "Branch4Exp", b);
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
			M->WriteByte("Chat", "Branch5Exp", b);
			if (PluginConfig.g_PopupAvail) {
				b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
				M->WriteByte("Chat", "Branch6Exp", b);
			}
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading7, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
            M->WriteByte("Chat", "Branch7Exp", b);

		}
		break;

		default:
			break;
	}
	return FALSE;
}

static TCHAR* chatcolorsnames[] ={
//	LPGENT("Your nickname"),
	LPGENT("Channel operators"),
	LPGENT("Half operators"),
	LPGENT("Voiced"),
//	LPGENT("Others nicknames"),
	LPGENT("Extended mode 1"),
	LPGENT("Extended mode 2"),
	LPGENT("Selection background"),
	LPGENT("Selected text"),
	LPGENT("Incremental search highlight")
	};

void RegisterFontServiceFonts() {
	int i;
	char szTemp[100];
	LOGFONT lf;
	FontIDT fid = {0};
	ColourIDT cid = {0};

	fid.cbSize = sizeof(FontIDT);
	cid.cbSize = sizeof(ColourIDT);

	strncpy(fid.dbSettingsGroup, FONTMODULE, SIZEOF(fid.dbSettingsGroup));
	fid.flags = FIDF_DEFAULTVALID|FIDF_ALLOWEFFECTS;

	for (i = 0; i < SIZEOF(IM_fontOptionsList); i++) {
		LoadMsgDlgFont(FONTSECTION_IM, i , &lf, &fontOptionsList[i].colour, FONTMODULE);
		mir_snprintf(szTemp, SIZEOF(szTemp), "Font%d", i);
		strncpy(fid.prefix, szTemp, SIZEOF(fid.prefix));
		fid.order = i;
		_tcsncpy(fid.name, fontOptionsList[i].szDescr, SIZEOF(fid.name));
		fid.deffontsettings.colour = fontOptionsList[i].colour;
		fid.deffontsettings.size = (char) lf.lfHeight;
		fid.deffontsettings.style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
		fid.deffontsettings.charset = lf.lfCharSet;
		fid.flags = fid.flags & ~FIDF_CLASSMASK | (fid.deffontsettings.style&FONTF_BOLD ? FIDF_CLASSHEADER : FIDF_CLASSGENERAL);
		_tcsncpy(fid.deffontsettings.szFace, lf.lfFaceName, LF_FACESIZE);
		_tcsncpy(fid.backgroundGroup, _T("TabSRMM/Single Messaging"), SIZEOF(fid.backgroundGroup));
		_tcsncpy(fid.group, _T("TabSRMM/Single Messaging"), SIZEOF(fid.group));
		switch (i) {
			case MSGFONTID_MYMSG:
			case 1:
			case MSGFONTID_MYNAME:
			case MSGFONTID_MYTIME:
			case 21:
				_tcsncpy(fid.backgroundName, _T("Outgoing background"), SIZEOF(fid.backgroundName));
				break;
			case 8:
			case 9:
			case 12:
			case 13:
				_tcsncpy(fid.backgroundName, _T("Outgoing background(old)"), SIZEOF(fid.backgroundName));
				break;
			case 10:
			case 11:
			case 14:
			case 15:
				_tcsncpy(fid.backgroundName, _T("Incoming background(old)"), SIZEOF(fid.backgroundName));
				break;
			case MSGFONTID_MESSAGEAREA:
				_tcsncpy(fid.group, _T("TabSRMM"), SIZEOF(fid.group));
				_tcsncpy(fid.backgroundGroup, _T("TabSRMM"), SIZEOF(fid.backgroundGroup));
				_tcsncpy(fid.backgroundName, _T("Input area background"), SIZEOF(fid.backgroundName));
				break;
			case 17:
				_tcsncpy(fid.backgroundName, _T("Status background"), SIZEOF(fid.backgroundName));
				break;
			case 18:
				_tcsncpy(fid.backgroundGroup, _T("TabSRMM"), SIZEOF(fid.backgroundGroup));
				_tcsncpy(fid.backgroundName, _T("Log Background"), SIZEOF(fid.backgroundName));
				break;
 			case 19:
 				_tcsncpy(fid.backgroundName, _T(""), SIZEOF(fid.backgroundName));
 				break;
			default:
				_tcsncpy(fid.backgroundName, _T("Incoming background"), SIZEOF(fid.backgroundName));
				break;
			}
		CallService(MS_FONT_REGISTERT, (WPARAM)&fid, 0);
		}

	strncpy(cid.dbSettingsGroup, FONTMODULE, SIZEOF(fid.dbSettingsGroup));

	cid.order = 0;
 	_tcsncpy(cid.group, _T("TabSRMM"), SIZEOF(fid.group));
 	_tcsncpy(cid.name, _T("Log Background"), SIZEOF(cid.name));
 	strncpy(cid.setting, (SRMSGSET_BKGCOLOUR), SIZEOF(cid.setting));
 	cid.defcolour = SRMSGDEFSET_BKGCOLOUR;
 	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

 	cid.order = 1;
 	_tcsncpy(cid.name, _T("Input area background"), SIZEOF(cid.name));
 	strncpy(cid.setting, "inputbg", SIZEOF(cid.setting));
 	cid.defcolour = SRMSGDEFSET_BKGCOLOUR;
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

	cid.order = 0;
	_tcsncpy(cid.group, _T("TabSRMM/Single Messaging"), SIZEOF(fid.group));
	_tcsncpy(cid.name, _T("Outgoing background"), SIZEOF(cid.name));
	strncpy(cid.setting, "outbg", SIZEOF(cid.setting));
	cid.defcolour = SRMSGDEFSET_BKGOUTCOLOUR;
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

 	cid.order = 1;
 	_tcsncpy(cid.name, _T("Incoming background"), SIZEOF(cid.name));
 	strncpy(cid.setting, "inbg", SIZEOF(cid.setting));
 	cid.defcolour = SRMSGDEFSET_BKGINCOLOUR;
 	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

 	cid.order = 2;
 	_tcsncpy(cid.name, _T("Status background"), SIZEOF(cid.name));
 	strncpy(cid.setting, "statbg", SIZEOF(cid.setting));
 	cid.defcolour = SRMSGDEFSET_BKGCOLOUR;
 	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

	cid.order = 3;
	_tcsncpy(cid.name, _T("Incoming background(old)"), SIZEOF(cid.name));
	strncpy(cid.setting, "oldinbg", SIZEOF(cid.setting));
	cid.defcolour = SRMSGDEFSET_BKGINCOLOUR;
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

	cid.order = 4;
	_tcsncpy(cid.name, _T("Outgoing background(old)"), SIZEOF(cid.name));
	strncpy(cid.setting, "oldoutbg", SIZEOF(cid.setting));
	cid.defcolour = SRMSGDEFSET_BKGOUTCOLOUR;
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

	cid.order = 5;
	_tcsncpy(cid.name, _T("Horizontal Grid Lines"), SIZEOF(cid.name));
	strncpy(cid.setting, "hgrid", SIZEOF(cid.setting));
	cid.defcolour = RGB(224, 224, 224);
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);


	fontOptionsList=IP_fontOptionsList;
	fid.flags|=FIDF_SAVEPOINTSIZE;
	_tcsncpy(fid.group, _T("TabSRMM/Info Panel"), SIZEOF(fid.group));
	_tcsncpy(fid.backgroundGroup, _T("TabSRMM/Info Panel"), SIZEOF(fid.backgroundGroup));
	_tcsncpy(fid.backgroundName, _T("Fields background"), SIZEOF(fid.backgroundName));
	for (i =0; i < IPFONTCOUNT; i++) {
		LoadMsgDlgFont(FONTSECTION_IP, i + 100 , &lf, &fontOptionsList[i].colour, FONTMODULE);
		mir_snprintf(szTemp, SIZEOF(szTemp), "Font%d", i + 100);
		strncpy(fid.prefix, szTemp, SIZEOF(fid.prefix));
		fid.order = i + 100 ;
		_tcsncpy(fid.name, fontOptionsList[i].szDescr, SIZEOF(fid.name));
		fid.deffontsettings.colour = fontOptionsList[i].colour;
		fid.deffontsettings.size = (char) lf.lfHeight;
		fid.deffontsettings.style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
		fid.flags = fid.flags & ~FIDF_CLASSMASK | (fid.deffontsettings.style&FONTF_BOLD ? FIDF_CLASSHEADER : FIDF_CLASSGENERAL);
		fid.deffontsettings.charset = lf.lfCharSet;
		_tcsncpy(fid.deffontsettings.szFace, lf.lfFaceName, LF_FACESIZE);
		if(i==IPFONTCOUNT-1){
			_tcsncpy(fid.backgroundGroup, _T(""), SIZEOF(fid.backgroundGroup));
			_tcsncpy(fid.backgroundName, _T(""), SIZEOF(fid.backgroundName));
			_tcsncpy(fid.group, _T("TabSRMM"), SIZEOF(fid.group));
			}
		CallService(MS_FONT_REGISTERT, (WPARAM)&fid, 0);
		}
	cid.order=0;
	_tcsncpy(cid.group, _T("TabSRMM/Info Panel"), SIZEOF(fid.group));
	_tcsncpy(cid.name, _T("Fields background"), SIZEOF(cid.name));
	strncpy(cid.setting, "ipfieldsbg", SIZEOF(cid.setting));
	cid.defcolour = GetSysColor(COLOR_3DFACE);
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

	fontOptionsList=CHAT_fontOptionsList;
	fid.flags&=~FIDF_SAVEPOINTSIZE;
	_tcsncpy(fid.group, _T("TabSRMM/Group Chats"), SIZEOF(fid.group));
	_tcsncpy(fid.backgroundGroup, _T("TabSRMM"), SIZEOF(fid.backgroundGroup));
	_tcsncpy(fid.backgroundName, _T("Log Background"), SIZEOF(fid.backgroundName));
	strncpy(fid.dbSettingsGroup, CHAT_FONTMODULE, SIZEOF(fid.dbSettingsGroup));
	for (i =0; i < msgDlgFontCount; i++) {
		LoadMsgDlgFont(FONTSECTION_CHAT, i , &lf, &fontOptionsList[i].colour, CHAT_FONTMODULE);
		mir_snprintf(szTemp, SIZEOF(szTemp), "Font%d", i);
		strncpy(fid.prefix, szTemp, SIZEOF(fid.prefix));
		fid.order = i;
		_tcsncpy(fid.name, fontOptionsList[i].szDescr, SIZEOF(fid.name));
		fid.deffontsettings.colour = fontOptionsList[i].colour;
		fid.deffontsettings.size = (char) lf.lfHeight;
		fid.deffontsettings.style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
		fid.flags = fid.flags & ~FIDF_CLASSMASK | (fid.deffontsettings.style&FONTF_BOLD ? FIDF_CLASSHEADER : FIDF_CLASSGENERAL);
		fid.deffontsettings.charset = lf.lfCharSet;
		_tcsncpy(fid.deffontsettings.szFace, lf.lfFaceName, LF_FACESIZE);
		CallService(MS_FONT_REGISTERT, (WPARAM)&fid, 0);
		}

	_tcsncpy(cid.group, _T("TabSRMM/Group Chats"), SIZEOF(cid.group));
	strncpy(cid.dbSettingsGroup, "Chat", SIZEOF(cid.dbSettingsGroup));
	for (i = 0; i <= 7; i++) {
		mir_snprintf(szTemp, SIZEOF(szTemp), "NickColor%d", i);
		_tcsncpy(cid.name, chatcolorsnames[i], SIZEOF(cid.name));
		cid.order=i+1;
		strncpy(cid.setting, szTemp, SIZEOF(cid.setting));
		switch (i) {
			case 5:
				cid.defcolour = GetSysColor(COLOR_HIGHLIGHT);
				break;
			case 6:
				cid.defcolour = GetSysColor(COLOR_HIGHLIGHTTEXT);
				break;
			default:
				cid.defcolour =RGB(0, 0, 0);
				break;
			}
		CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);
		}
	cid.order=8;
	_tcsncpy(cid.name, _T("Userlist background"), SIZEOF(cid.name));
	strncpy(cid.setting, "ColorNicklistBG", SIZEOF(cid.setting));
	cid.defcolour = SRMSGDEFSET_BKGCOLOUR;
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);
}

int FontServiceFontsChanged(WPARAM wParam, LPARAM lParam)
{
	if (g_chat_integration_enabled) {
		LOGFONT lf;
		HFONT hFont;
		int iText;

		LoadLogFonts();
		FreeMsgLogBitmaps();
		LoadMsgLogBitmaps();

		LoadMsgDlgFont(FONTSECTION_CHAT, 0, &lf, NULL, CHAT_FONTMODULE);
		hFont = CreateFontIndirect(&lf);
		iText = GetTextPixelSize(MakeTimeStamp(g_Settings.pszTimeStamp, time(NULL)), hFont, TRUE);
		DeleteObject(hFont);
		g_Settings.LogTextIndent = iText;
		g_Settings.LogTextIndent = g_Settings.LogTextIndent * 12 / 10;
		g_Settings.LogIndentEnabled = (M->GetByte("Chat", "LogIndentEnabled", 1) != 0) ? TRUE : FALSE;

		LoadGlobalSettings();
		MM_FontsChanged();
		MM_FixColors();
		SM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, TRUE);
	}

	ReloadGlobals();
	CacheMsgLogIcons();
	CacheLogFonts();
	M->BroadcastMessage(DM_OPTIONSAPPLIED, 1, 0);
	return 0;
}


static UINT _o2chatcontrols[] = { IDC_CHAT_SPIN2, IDC_LIMIT, IDC_CHAT_SPIN4, IDC_LOGTIMESTAMP, IDC_TIMESTAMP,
								  IDC_OUTSTAMP, IDC_FONTCHOOSE, IDC_LOGGING, IDC_LOGDIRECTORY, IDC_INSTAMP, IDC_HIGHLIGHT, IDC_HIGHLIGHTWORDS, IDC_CHAT_SPIN2, IDC_CHAT_SPIN3, IDC_NICKROW2, IDC_LOGLIMIT,
								  IDC_STATIC110, IDC_STATIC112, 0};

INT_PTR CALLBACK DlgProcOptions2(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {

			TranslateDialogDefault(hwndDlg);

			g_Settings.crLogBackground = (BOOL)M->GetDword("Chat", "ColorLogBG", SRMSGDEFSET_BKGCOLOUR);
			if (g_chat_integration_enabled) {
				TCHAR	tszTemp[MAX_PATH];

				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN2, UDM_SETRANGE, 0, MAKELONG(5000, 0));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN2, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingWord(NULL, "Chat", "LogLimit", 100), 0));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN3, UDM_SETRANGE, 0, MAKELONG(255, 10));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN3, UDM_SETPOS, 0, MAKELONG(M->GetByte("Chat", "NicklistRowDist", 12), 0));
				SetDlgItemText(hwndDlg, IDC_HIGHLIGHTWORDS, g_Settings.pszHighlightWords);
				SetDlgItemText(hwndDlg, IDC_LOGTIMESTAMP, g_Settings.pszTimeStampLog);
				SetDlgItemText(hwndDlg, IDC_TIMESTAMP, g_Settings.pszTimeStamp);
				SetDlgItemText(hwndDlg, IDC_OUTSTAMP, g_Settings.pszOutgoingNick);
				SetDlgItemText(hwndDlg, IDC_INSTAMP, g_Settings.pszIncomingNick);
				CheckDlgButton(hwndDlg, IDC_HIGHLIGHT, g_Settings.HighlightEnabled);
				EnableWindow(GetDlgItem(hwndDlg, IDC_HIGHLIGHTWORDS), g_Settings.HighlightEnabled ? TRUE : FALSE);
				CheckDlgButton(hwndDlg, IDC_LOGGING, g_Settings.LoggingEnabled);
				CallService(MS_UTILS_PATHTORELATIVET, (WPARAM)g_Settings.pszLogDir, (LPARAM)tszTemp);
				SetDlgItemText(hwndDlg, IDC_LOGDIRECTORY, tszTemp);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), g_Settings.LoggingEnabled ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FONTCHOOSE), g_Settings.LoggingEnabled ? TRUE : FALSE);
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN4, UDM_SETRANGE, 0, MAKELONG(10000, 0));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN4, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingWord(NULL, "Chat", "LoggingLimit", 100), 0));
				EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), g_Settings.LoggingEnabled ? TRUE : FALSE);

				if (ServiceExists(MS_UTILS_REPLACEVARS)) {
					TCHAR tszTooltipText[2048];

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
						_T("%yy%"),			TranslateT("year without century, 01-99"),
						_T("%yyyy%"),		TranslateT("year with century, 1901-9999"),
						_T("%wday%"),		TranslateT("abbreviated weekday name"),
						_T("%weekday%"),	TranslateT("full weekday name") );
					hPathTip = CreateToolTip(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), tszTooltipText, TranslateT("Variables"));
				}

			} else {
				int i = 0;

				while (_o2chatcontrols[i])
					EnableWindow(GetDlgItem(hwndDlg, _o2chatcontrols[i++]), FALSE);
			}
			if (hPathTip)
				SetTimer(hwndDlg, 0, 3000, NULL);
			break;
		}
		case WM_COMMAND:
			if ((LOWORD(wParam)		  == IDC_INSTAMP
					|| LOWORD(wParam) == IDC_OUTSTAMP
					|| LOWORD(wParam) == IDC_TIMESTAMP
					|| LOWORD(wParam) == IDC_LOGLIMIT
					|| LOWORD(wParam) == IDC_NICKROW2
					|| LOWORD(wParam) == IDC_HIGHLIGHTWORDS
					|| LOWORD(wParam) == IDC_LOGDIRECTORY
					|| LOWORD(wParam) == IDC_LIMIT
					|| LOWORD(wParam) == IDC_LOGTIMESTAMP)
					&& (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))	return 0;

			switch (LOWORD(wParam)) {
				case  IDC_FONTCHOOSE: {
					TCHAR tszDirectory[MAX_PATH];
					LPITEMIDLIST idList;
					LPMALLOC psMalloc;
					BROWSEINFO bi = {0};

					if (SUCCEEDED(CoGetMalloc(1, &psMalloc))) {
						TCHAR tszTemp[MAX_PATH];
						bi.hwndOwner = hwndDlg;
						bi.pszDisplayName = tszDirectory;
						bi.lpszTitle = TranslateT("Select Folder");
						bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_EDITBOX | BIF_RETURNONLYFSDIRS;
						bi.lpfn = BrowseCallbackProc;
						bi.lParam = (LPARAM)tszDirectory;

						idList = SHBrowseForFolder(&bi);
						if (idList) {
							SHGetPathFromIDList(idList, tszDirectory);
							lstrcat(tszDirectory, _T("\\"));
							CallService(MS_UTILS_PATHTORELATIVET, (WPARAM)tszDirectory, (LPARAM)tszTemp);
							SetWindowText(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), lstrlen(tszTemp) > 1 ? tszTemp : DEFLOGFILENAME);
						}
						psMalloc->Free(idList);
						psMalloc->Release();
					}
					break;
				}

				case IDC_HIGHLIGHT:
					EnableWindow(GetDlgItem(hwndDlg, IDC_HIGHLIGHTWORDS), IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHT) == BST_CHECKED ? TRUE : FALSE);
					break;
				case IDC_LOGGING:
					if (g_chat_integration_enabled) {
						EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED ? TRUE : FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_FONTCHOOSE), IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED ? TRUE : FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED ? TRUE : FALSE);
					}
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
				TCHAR *ptszPath = NULL;

				if (g_chat_integration_enabled) {

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY));
					if (iLen > 0) {
						TCHAR *pszText1 = (TCHAR *)malloc(iLen*sizeof(TCHAR) + 2);
						GetDlgItemText(hwndDlg, IDC_LOGDIRECTORY, pszText1, iLen + 1);
						M->WriteTString(NULL, "Chat", "LogDirectory", pszText1);
						free(pszText1);
						CallService(MS_UTILS_PATHTOABSOLUTET, (WPARAM)pszText, (LPARAM)g_Settings.pszLogDir);
					} else
						DBDeleteContactSetting(NULL, "Chat", "LogDirectory");

					g_Settings.LoggingEnabled = IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED ? TRUE : FALSE;
					M->WriteByte("Chat", "LoggingEnabled", (BYTE)g_Settings.LoggingEnabled);

					iLen = SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN4, UDM_GETPOS, 0, 0);
					DBWriteContactSettingWord(NULL, "Chat", "LoggingLimit", (WORD)iLen);

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_HIGHLIGHTWORDS));
					if (iLen > 0) {
						TCHAR *ptszText = (TCHAR *)malloc((iLen + 2) * sizeof(TCHAR));
						if (ptszText) {
							GetDlgItemText(hwndDlg, IDC_HIGHLIGHTWORDS, ptszText, iLen + 1);
							p2 = _tcschr(ptszText, (TCHAR)',');
							while (p2) {
								*p2 = (TCHAR)' ';
								p2 = _tcschr(ptszText, ',');
							}
							M->WriteTString(NULL, "Chat", "HighlightWords", ptszText);
							free(ptszText);
						}
					} else
						DBDeleteContactSetting(NULL, "Chat", "HighlightWords");

					iLen = SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN3, UDM_GETPOS, 0, 0);
					if (iLen > 0)
						M->WriteByte("Chat", "NicklistRowDist", (BYTE)iLen);
					else
						DBDeleteContactSetting(NULL, "Chat", "NicklistRowDist");

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOGTIMESTAMP));
					if (iLen > 0) {
						pszText = (char *)realloc(pszText, iLen + 1);
						GetDlgItemTextA(hwndDlg, IDC_LOGTIMESTAMP, pszText, iLen + 1);
						DBWriteContactSettingString(NULL, "Chat", "LogTimestamp", pszText);
					} else DBDeleteContactSetting(NULL, "Chat", "LogTimestamp");

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_TIMESTAMP));
					if (iLen > 0) {
						pszText = (char *)realloc(pszText, iLen + 1);
						GetDlgItemTextA(hwndDlg, IDC_TIMESTAMP, pszText, iLen + 1);
						DBWriteContactSettingString(NULL, "Chat", "HeaderTime", pszText);
					} else DBDeleteContactSetting(NULL, "Chat", "HeaderTime");

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_INSTAMP));
					if (iLen > 0) {
						pszText = (char *)realloc(pszText, iLen + 1);
						GetDlgItemTextA(hwndDlg, IDC_INSTAMP, pszText, iLen + 1);
						DBWriteContactSettingString(NULL, "Chat", "HeaderIncoming", pszText);
					} else DBDeleteContactSetting(NULL, "Chat", "HeaderIncoming");

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_OUTSTAMP));
					if (iLen > 0) {
						pszText = (char *)realloc(pszText, iLen + 1);
						GetDlgItemTextA(hwndDlg, IDC_OUTSTAMP, pszText, iLen + 1);
						DBWriteContactSettingString(NULL, "Chat", "HeaderOutgoing", pszText);
					} else DBDeleteContactSetting(NULL, "Chat", "HeaderOutgoing");

					g_Settings.HighlightEnabled = IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHT) == BST_CHECKED ? TRUE : FALSE;
					M->WriteByte("Chat", "HighlightEnabled", (BYTE)g_Settings.HighlightEnabled);

					iLen = SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN2, UDM_GETPOS, 0, 0);
					DBWriteContactSettingWord(NULL, "Chat", "LogLimit", (WORD)iLen);
				}


				if (pszText != NULL)
					free(pszText);
				if (hListBkgBrush)
					DeleteObject(hListBkgBrush);
				hListBkgBrush = CreateSolidBrush(M->GetDword("Chat", "ColorNicklistBG", SRMSGDEFSET_BKGCOLOUR));


				if (g_chat_integration_enabled) {
					LOGFONT lf;
					HFONT hFont;
					int iText;

					LoadLogFonts();
					FreeMsgLogBitmaps();
					LoadMsgLogBitmaps();

					LoadMsgDlgFont(FONTSECTION_CHAT, 0, &lf, NULL, CHAT_FONTMODULE);
					hFont = CreateFontIndirect(&lf);
					iText = GetTextPixelSize(MakeTimeStamp(g_Settings.pszTimeStamp, time(NULL)), hFont, TRUE);
					DeleteObject(hFont);
					g_Settings.LogTextIndent = iText;
					g_Settings.LogTextIndent = g_Settings.LogTextIndent * 12 / 10;
					g_Settings.LogIndentEnabled = (M->GetByte("Chat", "LogIndentEnabled", 1) != 0) ? TRUE : FALSE;

					LoadGlobalSettings();
					MM_FontsChanged();
					MM_FixColors();
					SM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, TRUE);
				}

				ReloadGlobals();
				CacheMsgLogIcons();
				CacheLogFonts();
				M->BroadcastMessage(DM_OPTIONSAPPLIED, 1, 0);
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
			break;
	}
	return FALSE;
}

static INT_PTR CALLBACK DlgProcOptionsPopup(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
							M->WriteByte("Chat", "PopupStyle", (BYTE)iLen);

							iLen = SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN1, UDM_GETPOS, 0, 0);
							g_Settings.iPopupTimeout = iLen;
							DBWriteContactSettingWord(NULL, "Chat", "PopupTimeout", (WORD)iLen);

							g_Settings.crPUBkgColour = SendDlgItemMessage(hwndDlg, IDC_BKG, CPM_GETCOLOUR, 0, 0);
							M->WriteDword("Chat", "PopupColorBG", (DWORD)SendDlgItemMessage(hwndDlg, IDC_BKG, CPM_GETCOLOUR, 0, 0));
							g_Settings.crPUTextColour = SendDlgItemMessage(hwndDlg, IDC_TEXT, CPM_GETCOLOUR, 0, 0);
							M->WriteDword("Chat", "PopupColorText", (DWORD)SendDlgItemMessage(hwndDlg, IDC_TEXT, CPM_GETCOLOUR, 0, 0));

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

	if (PluginConfig.g_PopupAvail) {
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

	g_Settings.LogLimitNames = M->GetByte("Chat", "LogLimitNames", 1);
	g_Settings.ShowTime = M->GetByte("Chat", "ShowTimeStamp", 1);
	g_Settings.SoundsFocus = M->GetByte("Chat", "SoundsFocus", 0);
	g_Settings.ShowTimeIfChanged = (BOOL)M->GetByte("Chat", "ShowTimeStampIfChanged", 0);
	g_Settings.TimeStampEventColour = (BOOL)M->GetByte("Chat", "TimeStampEventColour", 0);
	g_Settings.iEventLimit = DBGetContactSettingWord(NULL, "Chat", "LogLimit", 100);
	g_Settings.dwIconFlags = M->GetDword("Chat", "IconFlags", 0x0000);
	g_Settings.LoggingLimit = (size_t)DBGetContactSettingWord(NULL, "Chat", "LoggingLimit", 100);
	g_Settings.LoggingEnabled = (BOOL)M->GetByte("Chat", "LoggingEnabled", 0);
	g_Settings.OpenInDefault = (BOOL)M->GetByte("Chat", "DefaultContainer", 1);
	g_Settings.FlashWindow = (BOOL)M->GetByte("Chat", "FlashWindow", 0);
	g_Settings.FlashWindowHightlight = (BOOL)M->GetByte("Chat", "FlashWindowHighlight", 0);
	g_Settings.HighlightEnabled = (BOOL)M->GetByte("Chat", "HighlightEnabled", 1);
	g_Settings.crUserListColor = (BOOL)M->GetDword(CHAT_FONTMODULE, "Font18Col", RGB(0, 0, 0));
	g_Settings.crUserListBGColor = (BOOL)M->GetDword("Chat", "ColorNicklistBG", SRMSGDEFSET_BKGCOLOUR);
	g_Settings.crUserListHeadingsColor = (BOOL)M->GetDword(CHAT_FONTMODULE, "Font19Col", RGB(170, 170, 170));
	g_Settings.crLogBackground = (BOOL)M->GetDword("Chat", "ColorLogBG", SRMSGDEFSET_BKGCOLOUR);
	g_Settings.StripFormat = (BOOL)M->GetByte("Chat", "StripFormatting", 0);
	g_Settings.TrayIconInactiveOnly = (BOOL)M->GetByte("Chat", "TrayIconInactiveOnly", 1);
	g_Settings.SkipWhenNoWindow = (BOOL)M->GetByte("Chat", "SkipWhenNoWindow", 0);
	g_Settings.BBCodeInPopups = (BOOL)M->GetByte("Chat", "BBCodeInPopups", 0);
	g_Settings.AddColonToAutoComplete = (BOOL)M->GetByte("Chat", "AddColonToAutoComplete", 1);
	g_Settings.iPopupStyle = M->GetByte("Chat", "PopupStyle", 1);
	g_Settings.iPopupTimeout = DBGetContactSettingWord(NULL, "Chat", "PopupTimeout", 3);
	g_Settings.crPUBkgColour = M->GetDword("Chat", "PopupColorBG", GetSysColor(COLOR_WINDOW));
	g_Settings.crPUTextColour = M->GetDword("Chat", "PopupColorText", 0);
	g_Settings.ClassicIndicators = M->GetByte("Chat", "ClassicIndicators", 0);
	//MAD
	g_Settings.LogClassicIndicators = M->GetByte("Chat", "LogClassicIndicators", 0);
	g_Settings.AlternativeSorting   = M->GetByte("Chat", "AlternativeSorting", 1);
	g_Settings.AnnoyingHighlight	= M->GetByte("Chat", "AnnoyingHighlight", 0);
	g_Settings.CreateWindowOnHighlight = M->GetByte("Chat", "CreateWindowOnHighlight", 1);
	//MAD_
	g_Settings.LogSymbols = M->GetByte("Chat", "LogSymbols", 1);
	g_Settings.ClickableNicks = M->GetByte("Chat", "ClickableNicks", 1);
	g_Settings.ColorizeNicks = M->GetByte("Chat", "ColorizeNicks", 1);
	g_Settings.ColorizeNicksInLog = M->GetByte("Chat", "ColorizeNicksInLog", 1);
	g_Settings.ScaleIcons = M->GetByte("Chat", "ScaleIcons", 1);
	g_Settings.UseDividers = M->GetByte("Chat", "UseDividers", 1);
	g_Settings.DividersUsePopupConfig = M->GetByte("Chat", "DividersUsePopupConfig", 1);
	g_Settings.MathMod = ServiceExists(MATH_RTF_REPLACE_FORMULAE) && M->GetByte("Chat", "MathModSupport", 0);

	g_Settings.DoubleClick4Privat = (BOOL)M->GetByte("Chat", "DoubleClick4Privat", 0);
	g_Settings.ShowContactStatus = M->GetByte("Chat", "ShowContactStatus", 1);
	g_Settings.ContactStatusFirst = M->GetByte("Chat", "ContactStatusFirst", 0);


	if (hListBkgBrush)
		DeleteObject(hListBkgBrush);
	hListBkgBrush = CreateSolidBrush(M->GetDword("Chat", "ColorNicklistBG", SRMSGDEFSET_BKGCOLOUR));

	InitSetting(&g_Settings.pszTimeStamp, "HeaderTime", _T("[%H:%M]"));
	InitSetting(&g_Settings.pszTimeStampLog, "LogTimestamp", _T("[%d %b %y %H:%M]"));
	InitSetting(&g_Settings.pszIncomingNick, "HeaderIncoming", _T("%n:"));
	InitSetting(&g_Settings.pszOutgoingNick, "HeaderOutgoing", _T("%n:"));
	InitSetting(&g_Settings.pszHighlightWords, "HighlightWords", _T("%m"));

	{
		DBVARIANT dbv;
		g_Settings.pszLogDir = (TCHAR *)mir_realloc(g_Settings.pszLogDir, MAX_PATH*sizeof(TCHAR));
		if (!M->GetTString(NULL, "Chat", "LogDirectory", &dbv)) {
			lstrcpyn(g_Settings.pszLogDir, dbv.ptszVal, MAX_PATH);
			DBFreeVariant(&dbv);
		} else lstrcpyn(g_Settings.pszLogDir, DEFLOGFILENAME, MAX_PATH);
	}

	g_Settings.LogIndentEnabled = (M->GetByte("Chat", "LogIndentEnabled", 1) != 0) ? TRUE : FALSE;

	if (g_Settings.UserListFont)
		DeleteObject(g_Settings.UserListFont);
	LoadMsgDlgFont(FONTSECTION_CHAT, 18, &lf, NULL, CHAT_FONTMODULE);
	g_Settings.UserListFont = CreateFontIndirect(&lf);

	if (g_Settings.UserListHeadingsFont)
		DeleteObject(g_Settings.UserListHeadingsFont);
	LoadMsgDlgFont(FONTSECTION_CHAT, 19, &lf, NULL, CHAT_FONTMODULE);
	g_Settings.UserListHeadingsFont = CreateFontIndirect(&lf);

	for (i = 0; i < 7; i++) {
		mir_snprintf(szBuf, 20, "NickColor%d", i);
		g_Settings.nickColors[i] = M->GetDword("Chat", szBuf, g_Settings.crUserListColor);
	}
	g_Settings.nickColors[5] = M->GetDword("Chat", "NickColor5", GetSysColor(COLOR_HIGHLIGHT));
	g_Settings.nickColors[6] = M->GetDword("Chat", "NickColor6", GetSysColor(COLOR_HIGHLIGHTTEXT));
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
	if (g_Settings.SelectionBGBrush)
		DeleteObject(g_Settings.SelectionBGBrush);
}

int OptionsInit(void)
{
	LOGFONT lf;
	HFONT hFont;
	int iText;

	LoadLogFonts();
	LoadMsgDlgFont(FONTSECTION_CHAT, 17, &lf, NULL, CHAT_FONTMODULE);
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
	g_Settings.iX = M->GetDword("Chat", "roomx", -1);
	g_Settings.iY = M->GetDword("Chat", "roomy", -1);
	g_Settings.iWidth = M->GetDword("Chat", "roomwidth", -1);
	g_Settings.iHeight = M->GetDword("Chat", "roomheight", -1);
	LoadGlobalSettings();
	SkinAddNewSoundEx("ChatMessage", "Chat", TranslateA("Incoming message"));
	SkinAddNewSoundEx("ChatHighlight", "Chat", TranslateA("Message is highlighted"));
	SkinAddNewSoundEx("ChatAction", "Chat", TranslateA("User has performed an action"));
	SkinAddNewSoundEx("ChatJoin", "Chat", TranslateA("User has joined"));
	SkinAddNewSoundEx("ChatPart", "Chat", TranslateA("User has left"));
	SkinAddNewSoundEx("ChatKick", "Chat", TranslateA("User has kicked some other user"));
	SkinAddNewSoundEx("ChatMode", "Chat", TranslateA("User's status was changed"));
	SkinAddNewSoundEx("ChatNick", "Chat", TranslateA("User has changed name"));
	SkinAddNewSoundEx("ChatNotice", "Chat", TranslateA("User has sent a notice"));
	SkinAddNewSoundEx("ChatQuit", "Chat", TranslateA("User has disconnected"));
	SkinAddNewSoundEx("ChatTopic", "Chat", TranslateA("The topic has been changed"));

	LoadMsgDlgFont(FONTSECTION_CHAT, 0, &lf, NULL, CHAT_FONTMODULE);
	hFont = CreateFontIndirect(&lf);
	iText = GetTextPixelSize(MakeTimeStamp(g_Settings.pszTimeStamp, time(NULL)), hFont, TRUE);
	DeleteObject(hFont);
	g_Settings.LogTextIndent = iText;
	g_Settings.LogTextIndent = g_Settings.LogTextIndent * 12 / 10;

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
