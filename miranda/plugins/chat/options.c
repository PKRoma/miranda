/*
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
*/

#include "chat.h"
#include <shlobj.h>
#include <shlwapi.h>

extern HANDLE	g_hInst;
extern HBRUSH 	hEditBkgBrush;
extern HICON	hIcons[OPTIONS_ICONCOUNT];	
extern FONTINFO	aFonts[OPTIONS_FONTCOUNT];
extern BOOL		PopUpInstalled;


HANDLE			g_hOptions = NULL;

#define FONTF_BOLD   1
#define FONTF_ITALIC 2
struct FontOptionsList
{
    char *szDescr;
    COLORREF defColour;
    char *szDefFace;
    BYTE defCharset, defStyle;
    char defSize;
    COLORREF colour;
    char szFace[LF_FACESIZE];
    BYTE charset, style;
    char size;
}

//remeber to put these in the Translate( ) template file too
static fontOptionsList[] = {
	{"Timestamp", RGB(50, 50, 240), "Terminal", DEFAULT_CHARSET, 0, -8},
	{"Others nicknames", RGB(0, 0, 0), "Courier New", DEFAULT_CHARSET, FONTF_BOLD, -13},
	{"Your nickname", RGB(0, 0, 0), "Courier New", DEFAULT_CHARSET, FONTF_BOLD, -13},
	{"User has joined", RGB(90, 160, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User has left", RGB(160, 160, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User has disconnected", RGB(160, 90, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User kicked ...", RGB(100, 100, 100), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User is now known as ...", RGB(90, 90, 160), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Notice from user", RGB(160, 130, 60), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Incoming message", RGB(90, 90, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Outgoing message", RGB(90, 90, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"The topic is ...", RGB(70, 70, 160), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Information messages", RGB(130, 130, 195), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User enables status for ...", RGB(70, 150, 70), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User disables status for ...", RGB(150, 70, 70), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Action message", RGB(160, 90, 160), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Highlighted message", RGB(180, 150, 80), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Message typing area", RGB(0, 0, 40), "MS Shell Dlg", DEFAULT_CHARSET, 0, -13},
	{"User list members", RGB(40,40, 90), "MS Shell Dlg", DEFAULT_CHARSET, 0, -11},
	{"User list statuses", RGB(0, 0, 0), "MS Shell Dlg", DEFAULT_CHARSET, FONTF_BOLD, -11},
};
const int msgDlgFontCount = sizeof(fontOptionsList) / sizeof(fontOptionsList[0]);

struct branch_t
{
    char *szDescr;
	char *szDBName;
	int iMode;
	BYTE bDefault;
	HTREEITEM hItem;
};
static struct branch_t branch1[] = {
	{"Send message by pressing the Enter key", "SendOnEnter", 0, 1, NULL},
	{"Send message by pressing the Enter key twice", "SendOnDblEnter", 0,0, NULL},
	{"Flash window when someone speaks", "FlashWindow", 0,0, NULL},
	{"Flash window when a word is highlighted", "FlashWindowHighlight", 0,0, NULL},
	{"Show list of users in the chat room", "ShowNicklist", 0,1, NULL},
	{"Show button for sending messages", "ShowSend", 0, 0, NULL},
	{"Show name of the chat room in the top left of the window", "ShowName", 0,1, NULL},
	{"Show buttons for controlling the chat room", "ShowTopButtons", 0,1, NULL},
	{"Show buttons for formatting the text you are typing", "ShowFormatButtons", 0,1, NULL},
	{"Show button menus when right clicking the buttons", "RightClickFilter", 0,0, NULL},
	{"Show lines in the userlist", "ShowLines", 0,1, NULL},
	{"Show new windows cascaded", "CascadeWindows", 0,1, NULL},
	{"Save the size and position of chat rooms individually", "SavePosition", 0,0, NULL},
	{"Do not play sounds when the chat room is focused", "SoundsFocus", 0, 0, NULL},
	{"Do not pop up the window when joining a chat room", "PopupOnJoin", 0,0, NULL},
	{"Toggle the visible state when double clicking in the contact list", "ToggleVisibility", 0,0, NULL}
};
static struct branch_t branch2[] = {
	{"Prefix all events with a timestamp", "ShowTimeStamp", 0,1, NULL},
	{"Only prefix with timestamp if it has changed", "ShowTimeStampIfChanged", 0,0, NULL},
	{"Indent the second line of a message", "LogIndentEnabled", 0,0, NULL},
	{"Add \':\' to auto-completed user names", "AddColonToAutoComplete", 0, 1, NULL},
	{"Strip colors from messages in the log", "StripFormatting", 0, 0, NULL},
	{"Enable the \'event filter\' for new rooms", "FilterEnabled", 0,0, NULL}
};
static struct branch_t branch3[] = {
	{"Show topic changes", "FilterFlags", GC_EVENT_TOPIC, 1, NULL},
	{"Show users joining", "FilterFlags", GC_EVENT_JOIN, 1, NULL},
	{"Show users disconnecting", "FilterFlags", GC_EVENT_QUIT, 1, NULL},
	{"Show messages", "FilterFlags", GC_EVENT_MESSAGE, 1, NULL},
	{"Show actions", "FilterFlags", GC_EVENT_ACTION, 1, NULL},
	{"Show users leaving", "FilterFlags", GC_EVENT_PART, 1, NULL},
	{"Show users being kicked", "FilterFlags", GC_EVENT_KICK, 1, NULL},
	{"Show notices", "FilterFlags", GC_EVENT_NOTICE, 1, NULL},
	{"Show users changing name", "FilterFlags", GC_EVENT_NICK, 1, NULL},
	{"Show information messages", "FilterFlags", GC_EVENT_INFORMATION, 1, NULL},
	{"Show status changes of users", "FilterFlags", GC_EVENT_ADDSTATUS, 1, NULL},
};

static struct branch_t branch4[] = {
	{"Show icon for topic changes", "IconFlags", GC_EVENT_TOPIC, 1, NULL},
	{"Show icon for users joining", "IconFlags", GC_EVENT_JOIN, 1, NULL},
	{"Show icon for users disconnecting", "IconFlags", GC_EVENT_QUIT, 1, NULL},
	{"Show icon for messages", "IconFlags", GC_EVENT_MESSAGE, 1, NULL},
	{"Show icon for actions", "IconFlags", GC_EVENT_ACTION, 1, NULL},
	{"Show icon for highlights", "IconFlags", GC_EVENT_HIGHLIGHT, 1, NULL},
	{"Show icon for users leaving", "IconFlags", GC_EVENT_PART, 1, NULL},
	{"Show icon for users kicking other user", "IconFlags", GC_EVENT_KICK, 1, NULL},
	{"Show icon for notices ", "IconFlags", GC_EVENT_NOTICE, 1, NULL},
	{"Show icon for name changes", "IconFlags", GC_EVENT_NICK, 1, NULL},
	{"Show icon for information messages", "IconFlags", GC_EVENT_INFORMATION, 1, NULL},
	{"Show icon for status changes", "IconFlags", GC_EVENT_ADDSTATUS, 1, NULL},
};
static struct branch_t branch5[] = {
	{"Show icons in tray only when the chat room is not active", "TrayIconInactiveOnly", 0, 1, NULL},
	{"Show icon in tray for topic changes", "TrayIconFlags", GC_EVENT_TOPIC, 0, NULL},
	{"Show icon in tray for users joining", "TrayIconFlags", GC_EVENT_JOIN, 0, NULL},
	{"Show icon in tray for users disconnecting", "TrayIconFlags", GC_EVENT_QUIT, 0, NULL},
	{"Show icon in tray for messages", "TrayIconFlags", GC_EVENT_MESSAGE, 0, NULL},
	{"Show icon in tray for actions", "TrayIconFlags", GC_EVENT_ACTION, 0, NULL},
	{"Show icon in tray for highlights", "TrayIconFlags", GC_EVENT_HIGHLIGHT, 1, NULL},
	{"Show icon in tray for users leaving", "TrayIconFlags", GC_EVENT_PART, 0, NULL},
	{"Show icon in tray for users kicking other user", "TrayIconFlags", GC_EVENT_KICK, 0, NULL},
	{"Show icon in tray for notices ", "TrayIconFlags", GC_EVENT_NOTICE, 0, NULL},
	{"Show icon in tray for name changes", "TrayIconFlags", GC_EVENT_NICK, 0, NULL},
	{"Show icon in tray for information messages", "TrayIconFlags", GC_EVENT_INFORMATION, 0, NULL},
	{"Show icon in tray for status changes", "TrayIconFlags", GC_EVENT_ADDSTATUS, 0, NULL},
};

static struct branch_t branch6[] = {
	{"Show pop-ups only when the chat room is not active", "PopUpInactiveOnly", 0, 1, NULL},
	{"Show pop-up for topic changes", "PopupFlags", GC_EVENT_TOPIC, 0, NULL},
	{"Show pop-up for users joining", "PopupFlags", GC_EVENT_JOIN, 0, NULL},
	{"Show pop-up for users disconnecting", "PopupFlags", GC_EVENT_QUIT, 0, NULL},
	{"Show pop-up for messages", "PopupFlags", GC_EVENT_MESSAGE, 0, NULL},
	{"Show pop-up for actions", "PopupFlags", GC_EVENT_ACTION, 0, NULL},
	{"Show pop-up for highlights", "PopupFlags", GC_EVENT_HIGHLIGHT, 0, NULL},
	{"Show pop-up for users leaving", "PopupFlags", GC_EVENT_PART, 0, NULL},
	{"Show pop-up for users kicking other user", "PopupFlags", GC_EVENT_KICK, 0, NULL},
	{"Show pop-up for notices ", "PopupFlags", GC_EVENT_NOTICE, 0, NULL},
	{"Show pop-up for name changes", "PopupFlags", GC_EVENT_NICK, 0, NULL},
	{"Show pop-up for information messages", "PopupFlags", GC_EVENT_INFORMATION, 0, NULL},
	{"Show pop-up for status changes", "PopupFlags", GC_EVENT_ADDSTATUS, 0, NULL},
};


static HTREEITEM InsertBranch(HWND hwndTree, char * pszDescr, BOOL bExpanded)
{
	TVINSERTSTRUCT tvis;

	tvis.hParent=NULL;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_TEXT|TVIF_STATE;
	tvis.item.pszText=Translate(pszDescr);
	tvis.item.stateMask=bExpanded?TVIS_STATEIMAGEMASK|TVIS_EXPANDED:TVIS_STATEIMAGEMASK;
	tvis.item.state=bExpanded?INDEXTOSTATEIMAGEMASK(1)|TVIS_EXPANDED:INDEXTOSTATEIMAGEMASK(1);
	return TreeView_InsertItem(hwndTree,&tvis);
}

static void FillBranch(HWND hwndTree, HTREEITEM hParent, struct branch_t *branch, int nValues, DWORD defaultval)
{
	TVINSERTSTRUCT tvis;
	int i;
	int iState;

	if(hParent == 0)
		return;

	tvis.hParent=hParent;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_TEXT|TVIF_STATE;
	for(i=0;i<nValues;i++) {
		tvis.item.pszText=Translate(branch[i].szDescr);
		tvis.item.stateMask=TVIS_STATEIMAGEMASK;
		if (branch[i].iMode)
			iState = ((DBGetContactSettingDword(NULL, "Chat", branch[i].szDBName, defaultval)&branch[i].iMode)&branch[i].iMode)!=0?2:1;
		else
			iState = DBGetContactSettingByte(NULL, "Chat", branch[i].szDBName, branch[i].bDefault)!=0?2:1;
		tvis.item.state=INDEXTOSTATEIMAGEMASK(iState);
		branch[i].hItem = TreeView_InsertItem(hwndTree,&tvis);
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
		else
		{
			DBWriteContactSettingByte(NULL, "Chat", branch[i].szDBName, bChecked);
		}
	}
}
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
			if (SHGetPathFromIDList((LPITEMIDLIST) lp ,szDir))
					SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
			break;
	}
	return 0;
}

static void LoadLogFonts(void)
{
	int i;
	LOGFONT lf;
    for(i = 0; i<OPTIONS_FONTCOUNT; i++)
	{
		LoadMsgDlgFont(i, &aFonts[i].lf, &aFonts[i].color);
	}
	lf = aFonts[2].lf ;
	lf = aFonts[10].lf ;
}
void LoadMsgDlgFont(int i, LOGFONTA * lf, COLORREF * colour)
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
        if (DBGetContactSetting(NULL, "ChatFonts", str, &dbv))
            lstrcpyA(lf->lfFaceName, fontOptionsList[i].szDefFace);
        else {
            lstrcpynA(lf->lfFaceName, dbv.pszVal, sizeof(lf->lfFaceName));
            DBFreeVariant(&dbv);
        }
    }
}
static void InitSetting(char ** ppPointer, char * pszSetting, char*pszDefault)
{
	DBVARIANT dbv;
	if (!DBGetContactSetting(NULL, "Chat", pszSetting, &dbv) && dbv.type == DBVT_ASCIIZ)
	{
		*ppPointer = realloc(*ppPointer, lstrlen(dbv.pszVal)+1);
		lstrcpy(*ppPointer, dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else
	{
		*ppPointer = realloc(*ppPointer, lstrlen(pszDefault)+1);
		lstrcpy(*ppPointer, pszDefault);
	}
	return;

}
#define OPT_FIXHEADINGS (WM_USER+1)
static BOOL CALLBACK DlgProcOptions1(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static HTREEITEM hListHeading1 = 0;
	static HTREEITEM hListHeading2= 0;
	static HTREEITEM hListHeading3= 0;
	static HTREEITEM hListHeading4= 0;
	static HTREEITEM hListHeading5= 0;
	static HTREEITEM hListHeading6= 0;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
		char * pszGroup = NULL;

		TranslateDialogDefault(hwndDlg);
		SetWindowLong(GetDlgItem(hwndDlg,IDC_CHECKBOXES),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_CHECKBOXES),GWL_STYLE)|TVS_NOHSCROLL|TVS_CHECKBOXES);
		SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_SETRANGE,0,MAKELONG(255,16));
		SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"Chat","NicklistIndent",18),0));
		SendDlgItemMessage(hwndDlg,IDC_SPIN2,UDM_SETRANGE,0,MAKELONG(255,8));
		SendDlgItemMessage(hwndDlg,IDC_SPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"Chat","NicklistRowDist",16),0));
		hListHeading1 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), Translate("Appearance and functionality of chat room windows"), DBGetContactSettingByte(NULL, "Chat", "Branch1Exp", 0)?TRUE:FALSE);
		hListHeading2 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), Translate("Appearance of the message log"), DBGetContactSettingByte(NULL, "Chat", "Branch2Exp", 0)?TRUE:FALSE);
		hListHeading3 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), Translate("Default events to show in new chat rooms if the \'event filter\' is enabled"), DBGetContactSettingByte(NULL, "Chat", "Branch3Exp", 0)?TRUE:FALSE);
		hListHeading4 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), Translate("Icons to display in the message log"), DBGetContactSettingByte(NULL, "Chat", "Branch4Exp", 0)?TRUE:FALSE);
		hListHeading5 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), Translate("Icons to display in the tray"), DBGetContactSettingByte(NULL, "Chat", "Branch5Exp", 0)?TRUE:FALSE);
		if(PopUpInstalled)
			hListHeading6 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), Translate("Pop-ups to display"), DBGetContactSettingByte(NULL, "Chat", "Branch6Exp", 0)?TRUE:FALSE);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading1, branch1, sizeof(branch1) / sizeof(branch1[0]), 0);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2, branch2, sizeof(branch2) / sizeof(branch2[0]), 0);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3, branch3, sizeof(branch3) / sizeof(branch3[0]), 0);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4, branch4, sizeof(branch4) / sizeof(branch4[0]), 0xFFFF);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5, branch5, sizeof(branch5) / sizeof(branch5[0]), 0x1000);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6, branch6, sizeof(branch6) / sizeof(branch6[0]), 0x0000);
		SendMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);
		InitSetting(&pszGroup, "AddToGroup", "Chat rooms"); 
		SendDlgItemMessage(hwndDlg, IDC_GROUP, WM_SETTEXT, 0, (LPARAM)pszGroup);
		free(pszGroup);
		}break;

	case OPT_FIXHEADINGS:
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading1);
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2);
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3);
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4);
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5);
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6);
		break;
	case WM_COMMAND:
		if(	(LOWORD(wParam)		  == IDC_NICKINDENT
				|| LOWORD(wParam) == IDC_NICKROW
				|| LOWORD(wParam) == IDC_GROUP)
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return 0;

		if(lParam != (LPARAM)NULL)
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;
	case WM_NOTIFY:
	{
		switch(((LPNMHDR)lParam)->idFrom) 
		{
		case IDC_CHECKBOXES:
			if(((LPNMHDR)lParam)->code==NM_CLICK) {
				TVHITTESTINFO hti;
				hti.pt.x=(short)LOWORD(GetMessagePos());
				hti.pt.y=(short)HIWORD(GetMessagePos());
				ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
				if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
					if(hti.flags&TVHT_ONITEMSTATEICON) 
					{
						TVITEM tvi = {0};
						tvi.mask=TVIF_HANDLE|TVIF_STATE;
						tvi.hItem=hti.hItem;
						TreeView_GetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
						if(tvi.hItem == branch1[0].hItem && INDEXTOSTATEIMAGEMASK(1)==tvi.state)
							TreeView_SetItemState(((LPNMHDR)lParam)->hwndFrom, branch1[1].hItem, INDEXTOSTATEIMAGEMASK(1),  TVIS_STATEIMAGEMASK);
						if(tvi.hItem == branch1[1].hItem && INDEXTOSTATEIMAGEMASK(1)==tvi.state)
							TreeView_SetItemState(((LPNMHDR)lParam)->hwndFrom, branch1[0].hItem, INDEXTOSTATEIMAGEMASK(1),  TVIS_STATEIMAGEMASK);
						
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
						else
							PostMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
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
		
						iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_GROUP));
						if(iLen > 0)
						{
							pszText = realloc(pszText, iLen+1);
							GetDlgItemText(hwndDlg, IDC_GROUP, pszText,iLen+1);
							DBWriteContactSettingString(NULL, "Chat", "AddToGroup", pszText);
						}
						else
							DBWriteContactSettingString(NULL, "Chat", "AddToGroup", "");
						if(pszText)
							free(pszText);

						iLen = SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_GETPOS,0,0);
						if(iLen > 0)
							DBWriteContactSettingByte(NULL, "Chat", "NicklistIndent", (BYTE)iLen);
						else
							DBDeleteContactSetting(NULL, "Chat", "NicklistIndent");
						iLen = SendDlgItemMessage(hwndDlg,IDC_SPIN2,UDM_GETPOS,0,0);
						if(iLen > 0)
							DBWriteContactSettingByte(NULL, "Chat", "NicklistRowDist", (BYTE)iLen);
						else
							DBDeleteContactSetting(NULL, "Chat", "NicklistRowDist");
						SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch1, sizeof(branch1) / sizeof(branch1[0]));
						SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch2, sizeof(branch2) / sizeof(branch2[0]));
						SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch3, sizeof(branch3) / sizeof(branch3[0]));
						SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch4, sizeof(branch4) / sizeof(branch4[0]));
						SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch5, sizeof(branch5) / sizeof(branch5[0]));
						if(PopUpInstalled)
							SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch6, sizeof(branch6) / sizeof(branch6[0]));
						g_LogOptions.dwIconFlags = DBGetContactSettingDword(NULL, "Chat", "IconFlags", 0xFFFF);
						g_LogOptions.dwTrayIconFlags = DBGetContactSettingDword(NULL, "Chat", "TrayIconFlags", 0x1000);
						g_LogOptions.dwPopupFlags = DBGetContactSettingDword(NULL, "Chat", "PopupFlags", 0x0000);
						g_LogOptions.StripFormat = (BOOL)DBGetContactSettingByte(NULL, "Chat", "TrimFormatting", 0);
						g_LogOptions.TrayIconInactiveOnly = (BOOL)DBGetContactSettingByte(NULL, "Chat", "TrayIconInactiveOnly", 1);
						g_LogOptions.PopUpInactiveOnly = (BOOL)DBGetContactSettingByte(NULL, "Chat", "PopUpInactiveOnly", 1);
//						WM_BroadcastMessage(NULL, GC_CHANGEFILTERFLAG, 0, (LPARAM)DBGetContactSettingDword(NULL, "Chat", "FilterFlags", 0xFFFF), FALSE);
						WM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, TRUE);


					}
					return TRUE;
				}
		}
	}break;
	case WM_DESTROY:
		{
		BYTE b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading1, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
		DBWriteContactSettingByte(NULL, "Chat", "Branch1Exp", b);
		b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
		DBWriteContactSettingByte(NULL, "Chat", "Branch2Exp", b);
		b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
		DBWriteContactSettingByte(NULL, "Chat", "Branch3Exp", b);
		b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
		DBWriteContactSettingByte(NULL, "Chat", "Branch4Exp", b);
		b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
		DBWriteContactSettingByte(NULL, "Chat", "Branch5Exp", b);
		if(PopUpInstalled)
		{
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
			DBWriteContactSettingByte(NULL, "Chat", "Branch6Exp", b);
		}
		}break;

	default:break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcOptions2(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    static HBRUSH hBkgColourBrush;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{

		TranslateDialogDefault(hwndDlg);
		SendDlgItemMessage(hwndDlg,IDC_SPIN2,UDM_SETRANGE,0,MAKELONG(5000,0));
		SendDlgItemMessage(hwndDlg,IDC_SPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"Chat","LogLimit",100),0));
//		SendDlgItemMessage(hwndDlg,IDC_CHOOSEFONT,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[1]);
		SendDlgItemMessage(hwndDlg,IDC_SPIN3,UDM_SETRANGE,0,MAKELONG(10000,0));
		SendDlgItemMessage(hwndDlg,IDC_SPIN3,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"Chat","LoggingLimit",100),0));
		SetDlgItemText(hwndDlg, IDC_LOGDIRECTORY, g_LogOptions.pszLogDir);
		SetDlgItemText(hwndDlg, IDC_HIGHLIGHTWORDS, g_LogOptions.pszHighlightWords);
		SetDlgItemText(hwndDlg, IDC_LOGTIMESTAMP, g_LogOptions.pszTimeStampLog);
		SetDlgItemText(hwndDlg, IDC_TIMESTAMP, g_LogOptions.pszTimeStamp);
		SetDlgItemText(hwndDlg, IDC_OUTSTAMP, g_LogOptions.pszOutgoingNick);
		SetDlgItemText(hwndDlg, IDC_INSTAMP, g_LogOptions.pszIncomingNick);
		SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_SETCOLOUR,0,g_LogOptions.crLogBackground);
		SendDlgItemMessage(hwndDlg, IDC_MESSAGEBKG, CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL, "Chat", "ColorMessageBG", GetSysColor(COLOR_WINDOW)));
		SendDlgItemMessage(hwndDlg, IDC_NICKLISTBKG, CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));
		SendDlgItemMessage(hwndDlg, IDC_NICKLISTLINES, CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL, "Chat", "ColorNicklistLines", GetSysColor(COLOR_INACTIVEBORDER)));
		SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_SETDEFAULTCOLOUR, 0, g_LogOptions.crLogBackground);
		SendDlgItemMessage(hwndDlg, IDC_MESSAGEBKG, CPM_SETDEFAULTCOLOUR, 0, DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));
		SendDlgItemMessage(hwndDlg, IDC_NICKLISTBKG, CPM_SETDEFAULTCOLOUR, 0, DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));
		SendDlgItemMessage(hwndDlg, IDC_NICKLISTLINES, CPM_SETDEFAULTCOLOUR, 0, DBGetContactSettingDword(NULL, "Chat", "ColorNicklistLines", GetSysColor(COLOR_INACTIVEBORDER)));
		hBkgColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_GETCOLOUR, 0, 0));
		CheckDlgButton(hwndDlg, IDC_HIGHLIGHT, g_LogOptions.HighlightEnabled);
		EnableWindow(GetDlgItem(hwndDlg, IDC_HIGHLIGHTWORDS), g_LogOptions.HighlightEnabled?TRUE:FALSE);
		CheckDlgButton(hwndDlg, IDC_LOGGING, g_LogOptions.LoggingEnabled);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), g_LogOptions.LoggingEnabled?TRUE:FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_FONTCHOOSE), g_LogOptions.LoggingEnabled?TRUE:FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), g_LogOptions.LoggingEnabled?TRUE:FALSE);

		{
			int i;
			LOGFONTA lf;
			for (i = 0; i < sizeof(fontOptionsList) / sizeof(fontOptionsList[0]); i++) {
				LoadMsgDlgFont(i, &lf, &fontOptionsList[i].colour);
				lstrcpyA(fontOptionsList[i].szFace, lf.lfFaceName);
				fontOptionsList[i].size = (char) lf.lfHeight;
				fontOptionsList[i].style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
				fontOptionsList[i].charset = lf.lfCharSet;
				//I *think* some OSs will fail LB_ADDSTRING if lParam==0
				SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_ADDSTRING, 0, i + 1);
			}
			SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETSEL, TRUE, 0);
			SendDlgItemMessage(hwndDlg, IDC_FONTCOLOR, CPM_SETCOLOUR, 0, fontOptionsList[0].colour);
			SendDlgItemMessage(hwndDlg, IDC_FONTCOLOR, CPM_SETDEFAULTCOLOUR, 0, fontOptionsList[0].defColour);
		}
		
		}break;

    
	case WM_MEASUREITEM:
	{
		MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *) lParam;
		HFONT hFont, hoFont;
		HDC hdc;
		SIZE fontSize;
		int iItem = mis->itemData - 1;
		hFont = CreateFontA(fontOptionsList[iItem].size, 0, 0, 0,
						fontOptionsList[iItem].style & FONTF_BOLD ? FW_BOLD : FW_NORMAL,
						fontOptionsList[iItem].style & FONTF_ITALIC ? 1 : 0, 0, 0, fontOptionsList[iItem].charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontOptionsList[iItem].szFace);
		hdc = GetDC(GetDlgItem(hwndDlg, mis->CtlID));
		hoFont = (HFONT) SelectObject(hdc, hFont);
		GetTextExtentPoint32A(hdc, fontOptionsList[iItem].szDescr, lstrlenA(fontOptionsList[iItem].szDescr), &fontSize);
		SelectObject(hdc, hoFont);
		ReleaseDC(GetDlgItem(hwndDlg, mis->CtlID), hdc);
		DeleteObject(hFont);
		mis->itemWidth = fontSize.cx;
		mis->itemHeight = fontSize.cy;
		return TRUE;
	}
	case WM_DRAWITEM:
	{
		DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *) lParam;
		HFONT hFont, hoFont;
		char *pszText;
		int iItem = dis->itemData - 1;
		hFont = CreateFontA(fontOptionsList[iItem].size, 0, 0, 0,
		fontOptionsList[iItem].style & FONTF_BOLD ? FW_BOLD : FW_NORMAL,
		fontOptionsList[iItem].style & FONTF_ITALIC ? 1 : 0, 0, 0, fontOptionsList[iItem].charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontOptionsList[iItem].szFace);
		hoFont = (HFONT) SelectObject(dis->hDC, hFont);
		SetBkMode(dis->hDC, TRANSPARENT);
		FillRect(dis->hDC, &dis->rcItem, hBkgColourBrush);
		if (dis->itemState & ODS_SELECTED)
			FrameRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
		SetTextColor(dis->hDC, fontOptionsList[iItem].colour);
		pszText = Translate(fontOptionsList[iItem].szDescr);
		TextOutA(dis->hDC, dis->rcItem.left, dis->rcItem.top, pszText, lstrlenA(pszText));
		SelectObject(dis->hDC, hoFont);
		DeleteObject(hFont);
		return TRUE;
	}
	
	case WM_CTLCOLORLISTBOX:
        SetBkColor((HDC) wParam, SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_GETCOLOUR, 0, 0));
        return (BOOL) hBkgColourBrush;

	case WM_COMMAND:
		if(	(LOWORD(wParam)		  == IDC_INSTAMP
				|| LOWORD(wParam) == IDC_OUTSTAMP
				|| LOWORD(wParam) == IDC_TIMESTAMP
				|| LOWORD(wParam) == IDC_LOGLIMIT
				|| LOWORD(wParam) == IDC_HIGHLIGHTWORDS
				|| LOWORD(wParam) == IDC_LOGDIRECTORY
				|| LOWORD(wParam) == IDC_LOGTIMESTAMP
				|| LOWORD(wParam) == IDC_LIMIT)
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return 0;

		switch (LOWORD(wParam)) {
		case IDC_LOGBKG:
			DeleteObject(hBkgColourBrush);
			hBkgColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_GETCOLOUR, 0, 0));
			InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, TRUE);
			break;
		case IDC_HIGHLIGHT:
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIGHLIGHTWORDS), IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHT) == BST_CHECKED?TRUE:FALSE);

			break;
		case IDC_LOGGING:
			EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED?TRUE:FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_FONTCHOOSE), IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED?TRUE:FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED?TRUE:FALSE);
			break;
		case IDC_FONTLIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				if (SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELCOUNT, 0, 0) > 1) {
					SendDlgItemMessage(hwndDlg, IDC_FONTCOLOR, CPM_SETCOLOUR, 0, GetSysColor(COLOR_3DFACE));
					SendDlgItemMessage(hwndDlg, IDC_FONTCOLOR, CPM_SETDEFAULTCOLOUR, 0, GetSysColor(COLOR_WINDOWTEXT));
				}
			else {
				int i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA,
									   SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETCURSEL, 0, 0), 0) - 1;
				SendDlgItemMessage(hwndDlg, IDC_FONTCOLOR, CPM_SETCOLOUR, 0, fontOptionsList[i].colour);
				SendDlgItemMessage(hwndDlg, IDC_FONTCOLOR, CPM_SETDEFAULTCOLOUR, 0, fontOptionsList[i].defColour);
				}
			}
			if (HIWORD(wParam) != LBN_DBLCLK)
				return TRUE;
			//fall through
		case IDC_CHOOSEFONT:
		{
			CHOOSEFONTA cf = { 0 };
			LOGFONTA lf = { 0 };
			int i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETCURSEL, 0, 0),0) - 1;
			lf.lfHeight = fontOptionsList[i].size;
			lf.lfWeight = fontOptionsList[i].style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
			lf.lfItalic = fontOptionsList[i].style & FONTF_ITALIC ? 1 : 0;
			lf.lfCharSet = fontOptionsList[i].charset;
			lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
			lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			lf.lfQuality = DEFAULT_QUALITY;
			lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
			lstrcpyA(lf.lfFaceName, fontOptionsList[i].szFace);
			cf.lStructSize = sizeof(cf);
			cf.hwndOwner = hwndDlg;
			cf.lpLogFont = &lf;
			cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
			if (ChooseFontA(&cf)) {
				int selItems[sizeof(fontOptionsList) / sizeof(fontOptionsList[0])];
				int sel, selCount;

				selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, sizeof(fontOptionsList) / sizeof(fontOptionsList[0]), (LPARAM) selItems);
				for (sel = 0; sel < selCount; sel++) {
					i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[sel], 0) - 1;
					fontOptionsList[i].size = (char) lf.lfHeight;
					fontOptionsList[i].style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
					fontOptionsList[i].charset = lf.lfCharSet;
					lstrcpyA(fontOptionsList[i].szFace, lf.lfFaceName);
					{
						MEASUREITEMSTRUCT mis = { 0 };
						mis.CtlID = IDC_FONTLIST;
						mis.itemData = i + 1;
						SendMessage(hwndDlg, WM_MEASUREITEM, 0, (LPARAM) & mis);
						SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETITEMHEIGHT, selItems[sel], mis.itemHeight);
					}
				}
				InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, TRUE);
				break;
			}
			return TRUE;
		}
		case  IDC_FONTCHOOSE:
		{
			char szDirectory[MAX_PATH];
			LPITEMIDLIST idList;
			LPMALLOC psMalloc; 
			BROWSEINFO bi = {0};

			if(SUCCEEDED(CoGetMalloc(1,&psMalloc))) 
			{
				bi.hwndOwner=hwndDlg;
				bi.pszDisplayName=szDirectory;
				bi.lpszTitle=Translate("Select Folder");
				bi.ulFlags=BIF_NEWDIALOGSTYLE|BIF_EDITBOX|BIF_RETURNONLYFSDIRS;			
				bi.lpfn=BrowseCallbackProc;
				bi.lParam=(LPARAM)szDirectory;

				idList=SHBrowseForFolder(&bi);
				if(idList) {
					SHGetPathFromIDList(idList,szDirectory);
					lstrcat(szDirectory,"\\");
				SetWindowText(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), szDirectory);
				}
				psMalloc->lpVtbl->Free(psMalloc,idList);
				psMalloc->lpVtbl->Release(psMalloc);
			}
		}break;	

		case IDC_FONTCOLOR:
		{
			int selItems[sizeof(fontOptionsList) / sizeof(fontOptionsList[0])];
			int sel, selCount, i;

			selCount = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, sizeof(fontOptionsList) / sizeof(fontOptionsList[0]), (LPARAM) selItems);
			for (sel = 0; sel < selCount; sel++) {
				i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[sel], 0) - 1;
				fontOptionsList[i].colour = SendDlgItemMessage(hwndDlg, IDC_FONTCOLOR, CPM_GETCOLOUR, 0, 0);
			}
			InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, FALSE);
			break;
		}
		default:break;
		}
		if(lParam != (LPARAM)NULL)
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;
	case WM_NOTIFY:
	{
		switch(((LPNMHDR)lParam)->idFrom) 
		{
			case 0:
				switch (((LPNMHDR)lParam)->code) 
				{

				case PSN_APPLY:
					{
						int iLen;
						char * pszText = NULL;
						char * p2 = NULL;
		
						iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_HIGHLIGHTWORDS));
						if(iLen > 0)
						{
							pszText = realloc(pszText, iLen+1);
							GetDlgItemText(hwndDlg, IDC_HIGHLIGHTWORDS, pszText,iLen+1);
							p2 = strchr(pszText, ',');
							while (p2)
							{
								*p2 = ' ';
								p2 = strchr(pszText, ',');
							}

							DBWriteContactSettingString(NULL, "Chat", "HighlightWords", pszText);
						}
						else
							DBDeleteContactSetting(NULL, "Chat", "HighlightWords");

						iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY));
						if(iLen > 0)
						{
							pszText = realloc(pszText, iLen+1);
							GetDlgItemText(hwndDlg, IDC_LOGDIRECTORY, pszText,iLen+1);
							DBWriteContactSettingString(NULL, "Chat", "LogDirectory", pszText);
						}
						else
							DBDeleteContactSetting(NULL, "Chat", "LogDirectory");

						iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOGTIMESTAMP));
						if(iLen > 0)
						{
							pszText = realloc(pszText, iLen+1);
							GetDlgItemText(hwndDlg, IDC_LOGTIMESTAMP, pszText,iLen+1);
							DBWriteContactSettingString(NULL, "Chat", "LogTimestamp", pszText);
						}
						else
							DBDeleteContactSetting(NULL, "Chat", "LogTimestamp");

						iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_TIMESTAMP));
						if(iLen > 0)
						{
							pszText = realloc(pszText, iLen+1);
							GetDlgItemText(hwndDlg, IDC_TIMESTAMP, pszText,iLen+1);
							DBWriteContactSettingString(NULL, "Chat", "HeaderTime", pszText);
						}
						else
							DBDeleteContactSetting(NULL, "Chat", "HeaderTime");

						iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_INSTAMP));
						if(iLen > 0)
						{
							pszText = realloc(pszText, iLen+1);
							GetDlgItemText(hwndDlg, IDC_INSTAMP, pszText,iLen+1);
							DBWriteContactSettingString(NULL, "Chat", "HeaderIncoming", pszText);
						}
						else
							DBDeleteContactSetting(NULL, "Chat", "HeaderIncoming");

						iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_OUTSTAMP));
						if(iLen > 0)
						{
							pszText = realloc(pszText, iLen+1);
							GetDlgItemText(hwndDlg, IDC_OUTSTAMP, pszText,iLen+1);
							DBWriteContactSettingString(NULL, "Chat", "HeaderOutgoing", pszText);
						}
						else
							DBDeleteContactSetting(NULL, "Chat", "HeaderOutgoing");

						g_LogOptions.HighlightEnabled = IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHT) == BST_CHECKED?TRUE:FALSE;
						DBWriteContactSettingByte(NULL, "Chat", "HighlightEnabled", (BYTE)g_LogOptions.HighlightEnabled);

						g_LogOptions.LoggingEnabled = IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED?TRUE:FALSE;
						DBWriteContactSettingByte(NULL, "Chat", "LoggingEnabled", (BYTE)g_LogOptions.LoggingEnabled);
						if(g_LogOptions.LoggingEnabled && g_LogOptions.pszLogDir)
						{
							if(!PathIsDirectory(g_LogOptions.pszLogDir))
								CreateDirectory(g_LogOptions.pszLogDir, NULL);
						}

						iLen = SendDlgItemMessage(hwndDlg,IDC_SPIN2,UDM_GETPOS,0,0);
						DBWriteContactSettingWord(NULL, "Chat", "LogLimit", (WORD)iLen);
						iLen = SendDlgItemMessage(hwndDlg,IDC_SPIN3,UDM_GETPOS,0,0);
						DBWriteContactSettingWord(NULL, "Chat", "LoggingLimit", (WORD)iLen);

						DBWriteContactSettingDword(NULL, "Chat", "ColorLogBG", (DWORD)SendDlgItemMessage(hwndDlg,IDC_LOGBKG,CPM_GETCOLOUR,0,0));
						DBWriteContactSettingDword(NULL, "Chat", "ColorMessageBG", (DWORD)SendDlgItemMessage(hwndDlg,IDC_MESSAGEBKG,CPM_GETCOLOUR,0,0));
						DBWriteContactSettingDword(NULL, "Chat", "ColorNicklistBG", (DWORD)SendDlgItemMessage(hwndDlg,IDC_NICKLISTBKG,CPM_GETCOLOUR,0,0));
						DBWriteContactSettingDword(NULL, "Chat", "ColorNicklistLines", (DWORD)SendDlgItemMessage(hwndDlg,IDC_NICKLISTLINES,CPM_GETCOLOUR,0,0));
						if(pszText != NULL)
							free(pszText);
						if(hEditBkgBrush)
							DeleteObject(hEditBkgBrush);
						hEditBkgBrush = CreateSolidBrush(DBGetContactSettingDword(NULL, "Chat", "ColorMessageBG", GetSysColor(COLOR_WINDOW)));

                        {
                            int i;
                            char str[32];
                            for (i = 0; i < sizeof(fontOptionsList) / sizeof(fontOptionsList[0]); i++) {
                                wsprintfA(str, "Font%d", i);
                                DBWriteContactSettingString(NULL, "ChatFonts", str, fontOptionsList[i].szFace);
                                wsprintfA(str, "Font%dSize", i);
                                DBWriteContactSettingByte(NULL, "ChatFonts", str, fontOptionsList[i].size);
                                wsprintfA(str, "Font%dSty", i);
                                DBWriteContactSettingByte(NULL, "ChatFonts", str, fontOptionsList[i].style);
                                wsprintfA(str, "Font%dSet", i);
                                DBWriteContactSettingByte(NULL, "ChatFonts", str, fontOptionsList[i].charset);
                                wsprintfA(str, "Font%dCol", i);
                                DBWriteContactSettingDword(NULL, "ChatFonts", str, fontOptionsList[i].colour);
                            }
                        }
						LoadLogFonts();
						FreeMsgLogBitmaps();
						LoadMsgLogBitmaps();
						MM_FixColors();
						WM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, TRUE);
					}
					return TRUE;
				}
		}
	}break;
	case WM_DESTROY:
		DeleteObject(hBkgColourBrush);
		break;


	default:break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcOptionsPopup(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
		TranslateDialogDefault(hwndDlg);

		SendDlgItemMessage(hwndDlg, IDC_BKG, CPM_SETCOLOUR,0,g_LogOptions.crPUBkgColour);
		SendDlgItemMessage(hwndDlg, IDC_TEXT, CPM_SETCOLOUR,0,g_LogOptions.crPUTextColour);

		if(g_LogOptions.iPopupStyle ==2)
			CheckDlgButton(hwndDlg, IDC_RADIO2, BST_CHECKED);
		else if(g_LogOptions.iPopupStyle ==3)
			CheckDlgButton(hwndDlg, IDC_RADIO3, BST_CHECKED);
		else
			CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);

		EnableWindow(GetDlgItem(hwndDlg, IDC_BKG), IsDlgButtonChecked(hwndDlg, IDC_RADIO3) ==BST_CHECKED?TRUE:FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT), IsDlgButtonChecked(hwndDlg, IDC_RADIO3) ==BST_CHECKED?TRUE:FALSE);

		SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_SETRANGE,0,MAKELONG(100,-1));
		SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_SETPOS,0,MAKELONG(g_LogOptions.iPopupTimeout,0));
		}break;

	case WM_COMMAND:
		if(	(LOWORD(wParam)		  == IDC_TIMEOUT)
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return 0;

		if(lParam != (LPARAM)NULL)
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		switch (LOWORD(wParam)) {

		case IDC_RADIO1:
		case IDC_RADIO2:
		case IDC_RADIO3:
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG), IsDlgButtonChecked(hwndDlg, IDC_RADIO3) ==BST_CHECKED?TRUE:FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT), IsDlgButtonChecked(hwndDlg, IDC_RADIO3) ==BST_CHECKED?TRUE:FALSE);
			break;

		default:break;
		}

		break;
	case WM_NOTIFY:
	{
		switch(((LPNMHDR)lParam)->idFrom) 
		{
			case 0:
				switch (((LPNMHDR)lParam)->code) 
				{
				case PSN_APPLY:
					{
						int iLen;

						if(IsDlgButtonChecked(hwndDlg, IDC_RADIO2) == BST_CHECKED)
							iLen = 2;
						else if(IsDlgButtonChecked(hwndDlg, IDC_RADIO3) == BST_CHECKED)
							iLen = 3;
						else 
							iLen = 1;

						g_LogOptions.iPopupStyle = iLen;
						DBWriteContactSettingByte(NULL, "Chat", "PopupStyle", (BYTE)iLen);

						iLen = SendDlgItemMessage(hwndDlg,IDC_SPIN1,UDM_GETPOS,0,0);
						g_LogOptions.iPopupTimeout = iLen;
						DBWriteContactSettingWord(NULL, "Chat", "PopupTimeout", (WORD)iLen);

						g_LogOptions.crPUBkgColour = SendDlgItemMessage(hwndDlg,IDC_BKG,CPM_GETCOLOUR,0,0);
						DBWriteContactSettingDword(NULL, "Chat", "PopupColorBG", (DWORD)SendDlgItemMessage(hwndDlg,IDC_BKG,CPM_GETCOLOUR,0,0));
						g_LogOptions.crPUTextColour = SendDlgItemMessage(hwndDlg,IDC_TEXT,CPM_GETCOLOUR,0,0);
						DBWriteContactSettingDword(NULL, "Chat", "PopupColorText", (DWORD)SendDlgItemMessage(hwndDlg,IDC_TEXT,CPM_GETCOLOUR,0,0));

					}
					return TRUE;
				}
		}
	}break;

	default:break;
	}
	return FALSE;
}

static int OptionsInitialize(WPARAM wParam, LPARAM lParam)
{

	OPTIONSDIALOGPAGE odp = {0};

	odp.cbSize = sizeof(odp);
	odp.position = 910000000;
	odp.hInstance = g_hInst;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS1);
	odp.pszTitle = Translate("Chat");
	odp.pszGroup = Translate("Events");
	odp.pfnDlgProc = DlgProcOptions1;
	odp.flags = ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);

	odp.cbSize = sizeof(odp);
	odp.position = 910000001;
	odp.hInstance = g_hInst;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS2);
	odp.pszTitle = Translate("Chat Log");
	odp.pszGroup = Translate("Events");
	odp.pfnDlgProc = DlgProcOptions2;
	odp.flags = ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);

	if(PopUpInstalled)
	{
		odp.cbSize = sizeof(odp);
		odp.position = 910000002;
		odp.hInstance = g_hInst;
		odp.pszTemplate = MAKEINTRESOURCE(IDD_OPTIONSPOPUP);
		odp.pszTitle = Translate("Chat");
		odp.pszGroup = Translate("Popups");
		odp.pfnDlgProc = DlgProcOptionsPopup;
		odp.flags = ODPF_BOLDGROUPS;
		CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	}
	return 0;
}

void LoadGlobalSettings(void)
{
	char szTemp[MAX_PATH];
	char* p1;
	LOGFONT lf;

	GetModuleFileName(NULL,szTemp, MAX_PATH);
	p1 = strrchr(szTemp,'\\');
	if (p1)
		*p1 = '\0';
	lstrcat(szTemp, "\\Logs");

	g_LogOptions.ShowTime = DBGetContactSettingByte(NULL, "Chat", "ShowTimeStamp", 1);
	g_LogOptions.SoundsFocus = DBGetContactSettingByte(NULL, "Chat", "SoundsFocus", 0);
	g_LogOptions.ShowTimeIfChanged = (BOOL)DBGetContactSettingByte(NULL, "Chat", "ShowTimeStampIfChanged", 0);
	g_LogOptions.iEventLimit = DBGetContactSettingWord(NULL, "Chat", "LogLimit", 100);
	g_LogOptions.dwIconFlags = DBGetContactSettingDword(NULL, "Chat", "IconFlags", 0xFFFF);
	g_LogOptions.dwTrayIconFlags = DBGetContactSettingDword(NULL, "Chat", "TrayIconFlags", 0x1000);
	g_LogOptions.dwPopupFlags = DBGetContactSettingDword(NULL, "Chat", "PopupFlags", 0x0000);
	g_LogOptions.LoggingLimit = DBGetContactSettingWord(NULL, "Chat", "LoggingLimit", 100);
	g_LogOptions.LoggingEnabled = (BOOL)DBGetContactSettingByte(NULL, "Chat", "LoggingEnabled", 0);
	g_LogOptions.FlashWindow = (BOOL)DBGetContactSettingByte(NULL, "Chat", "FlashWindow", 0);
	g_LogOptions.HighlightEnabled = (BOOL)DBGetContactSettingByte(NULL, "Chat", "HighlightEnabled", 1);
	g_LogOptions.crUserListColor = (BOOL)DBGetContactSettingDword(NULL, "ChatFonts", "Font18Col", RGB(40,40,90));
	g_LogOptions.crUserListBGColor = (BOOL)DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW));
	g_LogOptions.crUserListHeadingsColor = (BOOL)DBGetContactSettingDword(NULL, "ChatFonts", "Font19Col", RGB(0,0,0));
	g_LogOptions.crLogBackground = (BOOL)DBGetContactSettingDword(NULL, "Chat", "ColorLogBG", GetSysColor(COLOR_WINDOW));
	g_LogOptions.bShowName = (BOOL)DBGetContactSettingByte(NULL, "Chat", "ShowName", 1);
	g_LogOptions.StripFormat = (BOOL)DBGetContactSettingByte(NULL, "Chat", "StripFormatting", 0);
	g_LogOptions.TrayIconInactiveOnly = (BOOL)DBGetContactSettingByte(NULL, "Chat", "TrayIconInactiveOnly", 1);
	g_LogOptions.PopUpInactiveOnly = (BOOL)DBGetContactSettingByte(NULL, "Chat", "PopUpInactiveOnly", 1);
	g_LogOptions.AddColonToAutoComplete = (BOOL)DBGetContactSettingByte(NULL, "Chat", "AddColonToAutoComplete", 1);
	g_LogOptions.iPopupStyle = DBGetContactSettingByte(NULL, "Chat", "PopupStyle", 1);
	g_LogOptions.iPopupTimeout = DBGetContactSettingWord(NULL, "Chat", "PopupTimeout", 3);
	g_LogOptions.crPUBkgColour = DBGetContactSettingDword(NULL, "Chat", "PopupColorBG", GetSysColor(COLOR_WINDOW));
	g_LogOptions.crPUTextColour = DBGetContactSettingDword(NULL, "Chat", "PopupColorText", 0);
	
	InitSetting(&g_LogOptions.pszTimeStamp, "HeaderTime", "[%H:%M]"); 
	InitSetting(&g_LogOptions.pszTimeStampLog, "LogTimestamp", "[%d %b %y %H:%M]"); 
	InitSetting(&g_LogOptions.pszIncomingNick, "HeaderIncoming", "%n:"); 
	InitSetting(&g_LogOptions.pszOutgoingNick, "HeaderOutgoing", "%n:"); 
	InitSetting(&g_LogOptions.pszHighlightWords, "HighlightWords", "%m"); 
	InitSetting(&g_LogOptions.pszLogDir, "LogDirectory", szTemp);
	
	{
		HFONT hFont;
		int iText;

		LoadMsgDlgFont(0, &lf, NULL);
		hFont = CreateFontIndirect(&lf);
		iText = GetTextPixelSize(MakeTimeStamp(g_LogOptions.pszTimeStamp, time(NULL)),hFont, TRUE);
		DeleteObject(hFont);
		g_LogOptions.LogTextIndent = iText;
		g_LogOptions.LogTextIndent = g_LogOptions.LogTextIndent*12/10;
		g_LogOptions.LogIndentEnabled = (DBGetContactSettingByte(NULL, "Chat", "LogIndentEnabled", 0) != 0)?TRUE:FALSE;
	}

	if(g_LogOptions.MessageBoxFont)
		DeleteObject(g_LogOptions.MessageBoxFont);
	LoadMsgDlgFont(17, &lf, NULL);
	g_LogOptions.MessageBoxFont = CreateFontIndirectA(&lf);

	if(g_LogOptions.UserListFont)
		DeleteObject(g_LogOptions.UserListFont);
	LoadMsgDlgFont(18, &lf, NULL);
	g_LogOptions.UserListFont = CreateFontIndirectA(&lf);
	
	if(g_LogOptions.UserListHeadingsFont)
		DeleteObject(g_LogOptions.UserListHeadingsFont);
	LoadMsgDlgFont(19, &lf, NULL);
	g_LogOptions.UserListHeadingsFont = CreateFontIndirectA(&lf);

	g_LogOptions.GroupOpenIcon = CopyIcon(LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN));
	g_LogOptions.GroupClosedIcon = CopyIcon(LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT));


}
static void FreeGlobalSettings(void)
{
	free(g_LogOptions.pszTimeStamp);
	free(g_LogOptions.pszTimeStampLog);
	free(g_LogOptions.pszIncomingNick);
	free(g_LogOptions.pszOutgoingNick);
	free(g_LogOptions.pszHighlightWords);
	free(g_LogOptions.pszLogDir);
	if(g_LogOptions.MessageBoxFont)
		DeleteObject(g_LogOptions.MessageBoxFont);
	if(g_LogOptions.UserListFont)
		DeleteObject(g_LogOptions.UserListFont);
	if(g_LogOptions.UserListHeadingsFont)
		DeleteObject(g_LogOptions.UserListHeadingsFont);
	if(g_LogOptions.GroupClosedIcon)
		DestroyIcon(g_LogOptions.GroupClosedIcon);
	if(g_LogOptions.GroupOpenIcon)
		DestroyIcon(g_LogOptions.GroupOpenIcon);
}

int OptionsInit(void)
{
	LOGFONT lf;

	LoadLogFonts();
	LoadMsgDlgFont(18, &lf, NULL);
	lstrcpy(lf.lfFaceName, "MS Shell Dlg");
	lf.lfUnderline = lf.lfItalic = lf.lfStrikeOut = 0;
	lf.lfHeight = -17;
	lf.lfWeight = FW_BOLD;
	g_LogOptions.NameFont = CreateFontIndirect(&lf);
	g_LogOptions.UserListFont = NULL;
	g_LogOptions.UserListHeadingsFont = NULL;
	g_LogOptions.MessageBoxFont = NULL;
	g_LogOptions.iSplitterX = DBGetContactSettingWord(NULL, "Chat", "SplitterX", 120);
	g_LogOptions.iSplitterY = DBGetContactSettingWord(NULL, "Chat", "SplitterY", 90);
	g_LogOptions.iX = DBGetContactSettingDword(NULL, "Chat", "roomx", -1);
	g_LogOptions.iY = DBGetContactSettingDword(NULL, "Chat", "roomy", -1);
	g_LogOptions.iWidth = DBGetContactSettingDword(NULL, "Chat", "roomwidth", -1);
	g_LogOptions.iHeight = DBGetContactSettingDword(NULL, "Chat", "roomheight", -1);

	LoadGlobalSettings();
	g_hOptions = HookEvent(ME_OPT_INITIALISE, OptionsInitialize);
	hEditBkgBrush = CreateSolidBrush(DBGetContactSettingDword(NULL, "Chat", "ColorMessageBG", GetSysColor(COLOR_WINDOW)));

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

	if(g_LogOptions.LoggingEnabled && g_LogOptions.pszLogDir)
	{
		if(!PathIsDirectory(g_LogOptions.pszLogDir))
			CreateDirectory(g_LogOptions.pszLogDir, NULL);
	}

	return 0;
}


int OptionsUnInit(void)
{
	FreeGlobalSettings();
	UnhookEvent(g_hOptions);
	DeleteObject(hEditBkgBrush);
	DeleteObject(g_LogOptions.NameFont);
	return 0;
}

