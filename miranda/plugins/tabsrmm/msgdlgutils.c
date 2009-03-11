/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

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

/*
 * utility functions for the message dialog window
 */

#include "commonheaders.h"
#pragma hdrstop

//#include "m_MathModule.h"

extern      MYGLOBALS myGlobals;
extern      NEN_OPTIONS nen_options;
extern      LOGFONTA logfonts[MSGDLGFONTCOUNT + 2];
extern      COLORREF fontcolors[MSGDLGFONTCOUNT + 2];
extern      TemplateSet LTR_Active, RTL_Active;
extern      PAB MyAlphaBlend;
extern      HMODULE g_hInst;
extern      HANDLE hMessageWindowList;
extern      StatusItems_t StatusItems[];
extern      TCHAR *xStatusDescr[];

void ShowMultipleControls(HWND hwndDlg, const UINT * controls, int cControls, int state);


static struct RTFColorTable _rtf_ctable[] = {
	_T("red"), RGB(255, 0, 0), 0, ID_FONT_RED,
	_T("blue"), RGB(0, 0, 255), 0, ID_FONT_BLUE,
	_T("green"), RGB(0, 255, 0), 0, ID_FONT_GREEN,
	_T("magenta"), RGB(255, 0, 255), 0, ID_FONT_MAGENTA,
	_T("yellow"), RGB(255, 255, 0), 0, ID_FONT_YELLOW,
	_T("cyan"), RGB(0, 255, 255), 0, ID_FONT_CYAN,
	_T("black"), 0, 0, ID_FONT_BLACK,
	_T("white"), RGB(255, 255, 255), 0, ID_FONT_WHITE,
	_T(""), 0, 0, 0
};

struct RTFColorTable *rtf_ctable = 0;
unsigned int g_ctable_size;

#ifndef SHVIEW_THUMBNAIL
#define SHVIEW_THUMBNAIL 0x702D
#endif

#define EVENTTYPE_NICKNAME_CHANGE       9001
#define EVENTTYPE_STATUSMESSAGE_CHANGE  9002
#define EVENTTYPE_AVATAR_CHANGE         9003
#define EVENTTYPE_CONTACTLEFTCHANNEL    9004
#define EVENTTYPE_WAT_ANSWER            9602
#define EVENTTYPE_JABBER_CHATSTATES     2000
#define EVENTTYPE_JABBER_PRESENCE       2001

static int g_status_events[] = {
	EVENTTYPE_STATUSCHANGE,
	EVENTTYPE_CONTACTLEFTCHANNEL,
	EVENTTYPE_WAT_ANSWER,
	EVENTTYPE_JABBER_CHATSTATES,
	EVENTTYPE_JABBER_PRESENCE

};

static int g_status_events_size = 0;
#define MAX_REGS(_A_) ( sizeof(_A_) / sizeof(_A_[0]) )

BOOL IsStatusEvent(int eventType)
{
	int i;

	if (g_status_events_size == 0)
		g_status_events_size = MAX_REGS(g_status_events);

	for (i = 0; i < g_status_events_size; i++) {
		if (eventType == g_status_events[i])
			return TRUE;
	}
	return FALSE;
}

/*
 * init default color table. the table may grow when using custom colors via bbcodes
 */

void RTF_CTableInit()
{
	int iSize = sizeof(struct RTFColorTable) * RTF_CTABLE_DEFSIZE;

	rtf_ctable = (struct RTFColorTable *) malloc(iSize);
	ZeroMemory(rtf_ctable, iSize);
	CopyMemory(rtf_ctable, _rtf_ctable, iSize);
	myGlobals.rtf_ctablesize = g_ctable_size = RTF_CTABLE_DEFSIZE;
}

/*
 * add a color to the global rtf color table
 */

void RTF_ColorAdd(const TCHAR *tszColname, size_t length)
{
	TCHAR *stopped;
	COLORREF clr;

	myGlobals.rtf_ctablesize++;
	g_ctable_size++;
	rtf_ctable = (struct RTFColorTable *)realloc(rtf_ctable, sizeof(struct RTFColorTable) * g_ctable_size);
	clr = _tcstol(tszColname, &stopped, 16);
	mir_sntprintf(rtf_ctable[g_ctable_size - 1].szName, length + 1, _T("%06x"), clr);
	rtf_ctable[g_ctable_size - 1].menuid = rtf_ctable[g_ctable_size - 1].index = 0;

	clr = _tcstol(tszColname, &stopped, 16);
	rtf_ctable[g_ctable_size - 1].clr = (RGB(GetBValue(clr), GetGValue(clr), GetRValue(clr)));
}

/*
 * reorder tabs within a container. fSavePos indicates whether the new position should be saved to the
 * contacts db record (if so, the container will try to open the tab at the saved position later)
 */

void RearrangeTab(HWND hwndDlg, struct MessageWindowData *dat, int iMode, BOOL fSavePos)
{
	TCITEM item = {0};
	HWND hwndTab = GetParent(hwndDlg);
	int newIndex;
	TCHAR oldText[512];
	item.mask = TCIF_IMAGE | TCIF_TEXT | TCIF_PARAM;
	item.pszText = oldText;
	item.cchTextMax = 500;

	if (dat == NULL || !IsWindow(hwndDlg))
		return;

	TabCtrl_GetItem(hwndTab, dat->iTabID, &item);
	newIndex = LOWORD(iMode);

	if (newIndex >= 0 && newIndex <= TabCtrl_GetItemCount(hwndTab)) {
		TabCtrl_DeleteItem(hwndTab, dat->iTabID);
		item.lParam = (LPARAM)hwndDlg;
		TabCtrl_InsertItem(hwndTab, newIndex, &item);
		BroadCastContainer(dat->pContainer, DM_REFRESHTABINDEX, 0, 0);
		ActivateTabFromHWND(hwndTab, hwndDlg);
		if (fSavePos)
			DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "tabindex", newIndex * 100);
	}
}

/*
 * subclassing for the save as file dialog (needed to set it to thumbnail view on Windows 2000
 * or later
 */

static BOOL CALLBACK OpenFileSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG: {
			SetWindowLong(hwnd, GWL_USERDATA, (LONG)lParam);
			break;
		}
		case WM_NOTIFY: {
			OPENFILENAMEA *ofn = (OPENFILENAMEA *)GetWindowLong(hwnd, GWL_USERDATA);
			HWND hwndParent = GetParent(hwnd);
			HWND hwndLv = FindWindowEx(hwndParent, NULL, _T("SHELLDLL_DefView"), NULL) ;

			if (hwndLv != NULL && *((DWORD *)(ofn->lCustData))) {
				SendMessage(hwndLv, WM_COMMAND, SHVIEW_THUMBNAIL, 0);
				*((DWORD *)(ofn->lCustData)) = 0;
			}
			break;
		}
	}
	return FALSE;
}

/*
 * saves a contact picture to disk
 * takes hbm (bitmap handle) and bool isOwnPic (1 == save the picture as your own avatar)
 * requires AVS and ADVAIMG services (Miranda 0.7+)
 */

static void SaveAvatarToFile(struct MessageWindowData *dat, HBITMAP hbm, int isOwnPic)
{
	char szFinalPath[MAX_PATH];
	char szFinalFilename[MAX_PATH];
	char szBaseName[MAX_PATH];
	char szTimestamp[100];
	OPENFILENAMEA ofn = {0};
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	static char *forbiddenCharacters = "%/\\'";
	unsigned int i, j;
	DWORD setView = 1;

	mir_snprintf(szTimestamp, 100, "%04u %02u %02u_%02u%02u", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min);

	if (!myGlobals.szSkinsPath&&myGlobals.szDataPath)
		mir_snprintf(szFinalPath, MAX_PATH,"%sSaved Contact Pictures\\%s", myGlobals.szDataPath, dat->bIsMeta ? dat->szMetaProto : dat->szProto);
	else
		mir_snprintf(szFinalPath, MAX_PATH,"%s%s",myGlobals.szAvatarsPath,dat->bIsMeta ? dat->szMetaProto : dat->szProto);

	if (CreateDirectoryA(szFinalPath, 0) == 0) {
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			MessageBox(0, TranslateT("Error creating destination directory"), TranslateT("Save contact picture"), MB_OK | MB_ICONSTOP);
			return;
		}
	}
	if (isOwnPic)
		mir_snprintf(szBaseName, MAX_PATH,"My Avatar_%s", szTimestamp);
	else
		mir_snprintf(szBaseName, MAX_PATH, "%s_%s", dat->uin, szTimestamp);

	mir_snprintf(szFinalFilename, MAX_PATH, "%s.png", szBaseName);
	ofn.lpstrDefExt = "png";
	if (CallService(MS_AV_CANSAVEBITMAP, 0, PA_FORMAT_PNG) == 0) {
		ofn.lpstrDefExt = "bmp";
		mir_snprintf(szFinalFilename, MAX_PATH,"%s.bmp", szBaseName);
	}

	/*
	 * do not allow / or \ or % in the filename
	 */
	for (i = 0; i < strlen(forbiddenCharacters); i++) {
		for(j = 0; j < strlen(szFinalFilename); j++) {
			if(szFinalFilename[j] == forbiddenCharacters[i])
				szFinalFilename[j] = '_';
		}
	}

	ofn.lpstrFilter = "Image files\0*.bmp;*.png;*.jpg;*.gif\0\0";
	if (IsWinVer2000Plus()) {
		ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ENABLESIZING | OFN_ENABLEHOOK;
		ofn.lpfnHook = (LPOFNHOOKPROC)OpenFileSubclass;
		ofn.lStructSize = sizeof(ofn);
	} else {
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
		ofn.Flags = OFN_HIDEREADONLY;
	}
	ofn.hwndOwner = 0;
	ofn.lpstrFile = szFinalFilename;
	ofn.lpstrInitialDir = szFinalPath;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lCustData = (LPARAM) & setView;
	if (GetSaveFileNameA(&ofn)) {
		if (PathFileExistsA(szFinalFilename)) {
			if (MessageBox(0, TranslateT("The file exists. Do you want to overwrite it?"), TranslateT("Save contact picture"), MB_YESNO | MB_ICONQUESTION) == IDNO)
				return;
		}
		CallService(MS_AV_SAVEBITMAP, (WPARAM)hbm, (LPARAM)szFinalFilename);
	}
}

/*
 * flash a tab icon if mode = true, otherwise restore default icon
 * store flashing state into bState
 */

void FlashTab(struct MessageWindowData *dat, HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, HICON origImage)
{
	TCITEM item;

	ZeroMemory((void *)&item, sizeof(item));
	item.mask = TCIF_IMAGE;

	if (mode)
		*bState = !(*bState);
	else
		dat->hTabIcon = origImage;
	item.iImage = 0;
	TabCtrl_SetItem(hwndTab, iTabindex, &item);
}

/*
 * draw transparent avatar image. Get around crappy image rescaling quality of the
 * AlphaBlend() API.
 *
 * hdcMem contains the bitmap to draw (must be premultiplied for proper per-pixel alpha
 * rendering in AlphaBlend().
 */

static void MY_AlphaBlend(HDC hdcDraw, DWORD left, DWORD top,  int width, int height, int bmWidth, int bmHeight, HDC hdcMem)
{
	HDC hdcTemp = CreateCompatibleDC(hdcDraw);
	HBITMAP hbmTemp = CreateCompatibleBitmap(hdcMem, bmWidth, bmHeight);
	HBITMAP hbmOld = SelectObject(hdcTemp, hbmTemp);
	BLENDFUNCTION bf = {0};

	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendOp = AC_SRC_OVER;

	SetStretchBltMode(hdcTemp, HALFTONE);
	StretchBlt(hdcTemp, 0, 0, bmWidth, bmHeight, hdcDraw, left, top, width, height, SRCCOPY);
	if (MyAlphaBlend)
		MyAlphaBlend(hdcTemp, 0, 0, bmWidth, bmHeight, hdcMem, 0, 0, bmWidth, bmHeight, bf);
	else {
		SetStretchBltMode(hdcTemp, HALFTONE);
		StretchBlt(hdcTemp, 0, 0, bmWidth, bmHeight, hdcMem, 0, 0, bmWidth, bmHeight, SRCCOPY);
	}
	StretchBlt(hdcDraw, left, top, width, height, hdcTemp, 0, 0, bmWidth, bmHeight, SRCCOPY);
	SelectObject(hdcTemp, hbmOld);
	DeleteObject(hbmTemp);
	DeleteDC(hdcTemp);
}

/*
 * calculates avatar layouting, based on splitter position to find the optimal size
 * for the avatar w/o disturbing the toolbar too much.
 */

void CalcDynamicAvatarSize(HWND hwndDlg, struct MessageWindowData *dat, BITMAP *bminfo)
{
	RECT rc;
	double aspect = 0, newWidth = 0, picAspect = 0;
	double picProjectedWidth = 0;
	BOOL bBottomToolBar=dat->pContainer->dwFlags & CNT_BOTTOMTOOLBAR;
	BOOL bToolBar=dat->pContainer->dwFlags & CNT_HIDETOOLBAR?0:1;

	if (myGlobals.g_FlashAvatarAvail) {
		FLASHAVATAR fa = {0};
		fa.cProto = dat->szProto;
		fa.id = 25367;
		fa.hContact = (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) ? NULL : dat->hContact;
		CallService(MS_FAVATAR_GETINFO, (WPARAM)&fa, 0);
		if (fa.hWindow) {
			bminfo->bmHeight = FAVATAR_HEIGHT;
			bminfo->bmWidth = FAVATAR_WIDTH;
		}
	}
	GetClientRect(hwndDlg, &rc);

	if (dat->dwFlags & MWF_WASBACKGROUNDCREATE || dat->pContainer->dwFlags & CNT_DEFERREDCONFIGURE || dat->pContainer->dwFlags & CNT_CREATE_MINIMIZED || IsIconic(dat->pContainer->hwnd))
		return;                 // at this stage, the layout is not yet ready...

	if (bminfo->bmWidth == 0 || bminfo->bmHeight == 0)
		picAspect = 1.0;
	else
		picAspect = (double)(bminfo->bmWidth / (double)bminfo->bmHeight);
	picProjectedWidth = (double)((dat->dynaSplitter-((bBottomToolBar && bToolBar)? DPISCALEX(24):0) + ((dat->showUIElements != 0) ? DPISCALEX(28) : DPISCALEX(2)))) * picAspect;

	//MaD: by changing dat->iButtonBarReallyNeeds to dat->iButtonBarNeeds
	//	   we can change resize priority to buttons, if needed...
	if (((rc.right) - (int)picProjectedWidth) > (dat->iButtonBarReallyNeeds) && !myGlobals.m_AlwaysFullToolbarWidth)
		dat->iRealAvatarHeight = dat->dynaSplitter + 4 + (dat->showUIElements ? DPISCALEY(28):DPISCALEY(2));
	else
		dat->iRealAvatarHeight = dat->dynaSplitter +DPISCALEY(4);
	//mad
	dat->iRealAvatarHeight-=((bBottomToolBar&&bToolBar)? DPISCALEY(24):0);

	if (myGlobals.m_LimitStaticAvatarHeight > 0)
		dat->iRealAvatarHeight = min(dat->iRealAvatarHeight, myGlobals.m_LimitStaticAvatarHeight);

	if (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "dontscaleavatars", DBGetContactSettingByte(NULL, SRMSGMOD_T, "dontscaleavatars", 0)))
		dat->iRealAvatarHeight = min(bminfo->bmHeight, dat->iRealAvatarHeight);

	if (bminfo->bmHeight != 0)
		aspect = (double)dat->iRealAvatarHeight / (double)bminfo->bmHeight;
	else
		aspect = 1;

	newWidth = (double)bminfo->bmWidth * aspect;
	if (newWidth > (double)(rc.right) * 0.8)
		newWidth = (double)(rc.right) * 0.8;
	dat->pic.cy = dat->iRealAvatarHeight + 2;
	dat->pic.cx = (int)newWidth + 2;
}

/*
 + returns TRUE if the contact is handled by the MetaContacts protocol
 */

int IsMetaContact(HWND hwndDlg, struct MessageWindowData *dat)
{
	if (dat->hContact == 0 || dat->szProto == NULL)
		return 0;

	return (myGlobals.g_MetaContactsAvail && !strcmp(dat->szProto, myGlobals.szMetaName));
}

/*
 * retrieve the current (active) protocol of a given metacontact. Usually, this is the
 * "most online" protocol, unless a specific protocol is forced
 */

char *GetCurrentMetaContactProto(HWND hwndDlg, struct MessageWindowData *dat)
{
	dat->hSubContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)dat->hContact, 0);
	if (dat->hSubContact) {
		dat->szMetaProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)dat->hSubContact, 0);
		if (dat->szMetaProto)
			dat->wMetaStatus = DBGetContactSettingWord(dat->hSubContact, dat->szMetaProto, "Status", ID_STATUS_OFFLINE);
		else
			dat->wMetaStatus = ID_STATUS_OFFLINE;
		return dat->szMetaProto;
	} else
		return dat->szProto;
}

void WriteStatsOnClose(HWND hwndDlg, struct MessageWindowData *dat)
{
	DBEVENTINFO dbei;
	char buffer[450];
	HANDLE hNewEvent;
	time_t now = time(NULL);
	now = now - dat->stats.started;

	return;

	if (dat->hContact != 0 &&(myGlobals.m_LogStatusChanges != 0) && dat->dwFlags & MWF_LOG_STATUSCHANGES) {
		mir_snprintf(buffer, sizeof(buffer), "Session close - active for: %d:%02d:%02d, Sent: %d (%d), Rcvd: %d (%d)", now / 3600, now / 60, now % 60, dat->stats.iSent, dat->stats.iSentBytes, dat->stats.iReceived, dat->stats.iReceivedBytes);
		dbei.cbSize = sizeof(dbei);
		dbei.pBlob = (PBYTE) buffer;
		dbei.cbBlob = strlen(buffer) + 1;
		dbei.eventType = EVENTTYPE_STATUSCHANGE;
		dbei.flags = DBEF_READ;
		dbei.timestamp = time(NULL);
		dbei.szModule = dat->szProto;
		hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) dat->hContact, (LPARAM) & dbei);
		if (dat->hDbEventFirst == NULL) {
			dat->hDbEventFirst = hNewEvent;
			SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
		}
	}
}

int MsgWindowUpdateMenu(HWND hwndDlg, struct MessageWindowData *dat, HMENU submenu, int menuID)
{
	if (menuID == MENU_TABCONTEXT) {
		SESSION_INFO *si = dat->si;
		int iTabs = TabCtrl_GetItemCount(GetParent(hwndDlg));

		EnableMenuItem(submenu, ID_TABMENU_ATTACHTOCONTAINER, DBGetContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", 0) || DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0) ? MF_GRAYED : MF_ENABLED);
		EnableMenuItem(submenu, ID_TABMENU_CLEARSAVEDTABPOSITION, (DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "tabindex", -1) != -1) ? MF_ENABLED : MF_GRAYED);
	} else if (menuID == MENU_PICMENU) {
		MENUITEMINFO mii = {0};
		TCHAR *szText = NULL;
		char  avOverride = (char)DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "hideavatar", -1);
		HMENU visMenu = GetSubMenu(submenu, 0);
		BOOL picValid = dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? (dat->hOwnPic != 0) : (dat->ace && dat->ace->hbmPic && dat->ace->hbmPic != myGlobals.g_hbmUnknown);
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING;

		EnableMenuItem(submenu, ID_PICMENU_SAVETHISPICTUREAS, MF_BYCOMMAND | (picValid ? MF_ENABLED : MF_GRAYED));

		CheckMenuItem(visMenu, ID_VISIBILITY_DEFAULT, MF_BYCOMMAND | (avOverride == -1 ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(visMenu, ID_VISIBILITY_HIDDENFORTHISCONTACT, MF_BYCOMMAND | (avOverride == 0 ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(visMenu, ID_VISIBILITY_VISIBLEFORTHISCONTACT, MF_BYCOMMAND | (avOverride == 1 ? MF_CHECKED : MF_UNCHECKED));

		CheckMenuItem(submenu, ID_PICMENU_ALWAYSKEEPTHEBUTTONBARATFULLWIDTH, MF_BYCOMMAND | (myGlobals.m_AlwaysFullToolbarWidth ? MF_CHECKED : MF_UNCHECKED));
		if (!(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)) {
			EnableMenuItem(submenu, ID_PICMENU_SETTINGS, MF_BYCOMMAND | (ServiceExists(MS_AV_GETAVATARBITMAP) ? MF_ENABLED : MF_GRAYED));
			szText = TranslateT("Contact picture settings...");
			EnableMenuItem(submenu, 0, MF_BYPOSITION | MF_ENABLED);
		} else {
			EnableMenuItem(submenu, 0, MF_BYPOSITION | MF_GRAYED);
			EnableMenuItem(submenu, ID_PICMENU_SETTINGS, MF_BYCOMMAND | ((ServiceExists(MS_AV_SETMYAVATAR) && CallService(MS_AV_CANSETMYAVATAR, (WPARAM)(dat->bIsMeta ? dat->szMetaProto : dat->szProto), 0)) ? MF_ENABLED : MF_GRAYED));
			szText = TranslateT("Set your avatar...");
		}
		mii.dwTypeData = szText;
		mii.cch = lstrlen(szText) + 1;
		SetMenuItemInfo(submenu, ID_PICMENU_SETTINGS, FALSE, &mii);
	} else if (menuID == MENU_PANELPICMENU) {
		HMENU visMenu = GetSubMenu(submenu, 0);
		char  avOverride = (char)DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "hideavatar", -1);

		CheckMenuItem(visMenu, ID_VISIBILITY_DEFAULT, MF_BYCOMMAND | (avOverride == -1 ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(visMenu, ID_VISIBILITY_HIDDENFORTHISCONTACT, MF_BYCOMMAND | (avOverride == 0 ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem(visMenu, ID_VISIBILITY_VISIBLEFORTHISCONTACT, MF_BYCOMMAND | (avOverride == 1 ? MF_CHECKED : MF_UNCHECKED));

		EnableMenuItem(submenu, ID_PICMENU_SETTINGS, MF_BYCOMMAND | (ServiceExists(MS_AV_GETAVATARBITMAP) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(submenu, ID_PANELPICMENU_SAVETHISPICTUREAS, MF_BYCOMMAND | ((ServiceExists(MS_AV_SAVEBITMAP) && dat->ace && dat->ace->hbmPic && dat->ace->hbmPic != myGlobals.g_hbmUnknown) ? MF_ENABLED : MF_GRAYED));
	}
	return 0;
}

int MsgWindowMenuHandler(HWND hwndDlg, struct MessageWindowData *dat, int selection, int menuId)
{
	if (menuId == MENU_PICMENU || menuId == MENU_PANELPICMENU || menuId == MENU_TABCONTEXT) {
		switch (selection) {
			case ID_TABMENU_ATTACHTOCONTAINER:
				CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SELECTCONTAINER), hwndDlg, SelectContainerDlgProc, (LPARAM) hwndDlg);
				return 1;
			case ID_TABMENU_CONTAINEROPTIONS:
				if (dat->pContainer->hWndOptions == 0)
					CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndDlg, DlgProcContainerOptions, (LPARAM) dat->pContainer);
				return 1;
			case ID_TABMENU_CLOSECONTAINER:
				SendMessage(dat->pContainer->hwnd, WM_CLOSE, 0, 0);
				return 1;
			case ID_TABMENU_CLOSETAB:
				SendMessage(hwndDlg, WM_CLOSE, 1, 0);
				return 1;
			case ID_TABMENU_SAVETABPOSITION:
				DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "tabindex", dat->iTabID * 100);
				break;
			case ID_TABMENU_CLEARSAVEDTABPOSITION:
				DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "tabindex");
				break;
			case ID_TABMENU_LEAVECHATROOM: {
				if (dat && dat->bType == SESSIONTYPE_CHAT) {
					SESSION_INFO *si = (SESSION_INFO *)dat->si;
					if ( (si != NULL) && (dat->hContact != NULL) ) {
						char* szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) dat->hContact, 0);
						if ( szProto )
							CallProtoService( szProto, PS_LEAVECHAT, (WPARAM)dat->hContact, 0 );
					}
				}
				return 1;
			}
			case ID_VISIBILITY_DEFAULT:
			case ID_VISIBILITY_HIDDENFORTHISCONTACT:
			case ID_VISIBILITY_VISIBLEFORTHISCONTACT: {
				BYTE avOverrideMode;

				if (selection == ID_VISIBILITY_DEFAULT)
					avOverrideMode = -1;
				else if (selection == ID_VISIBILITY_HIDDENFORTHISCONTACT)
					avOverrideMode = 0;
				else if (selection == ID_VISIBILITY_VISIBLEFORTHISCONTACT)
					avOverrideMode = 1;
				DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "hideavatar", avOverrideMode);
				dat->panelWidth = -1;
				ShowPicture(hwndDlg, dat, FALSE);
				SendMessage(hwndDlg, WM_SIZE, 0, 0);
				DM_ScrollToBottom(hwndDlg, dat, 0, 1);
				return 1;
			}
			case ID_PICMENU_ALWAYSKEEPTHEBUTTONBARATFULLWIDTH:
				myGlobals.m_AlwaysFullToolbarWidth = !myGlobals.m_AlwaysFullToolbarWidth;
				DBWriteContactSettingByte(NULL, SRMSGMOD_T, "alwaysfulltoolbar", (BYTE)myGlobals.m_AlwaysFullToolbarWidth);
				WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 1);
				break;
			case ID_PICMENU_SAVETHISPICTUREAS:
				if (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
					if (dat)
						SaveAvatarToFile(dat, dat->hOwnPic, 1);
				} else {
					if (dat && dat->ace)
						SaveAvatarToFile(dat, dat->ace->hbmPic, 0);
				}
				break;
			case ID_PANELPICMENU_SAVETHISPICTUREAS:
				if (dat && dat->ace)
					SaveAvatarToFile(dat, dat->ace->hbmPic, 0);
				break;
			case ID_PICMENU_CHOOSEBACKGROUNDCOLOR:
			case ID_PANELPICMENU_CHOOSEBACKGROUNDCOLOR: {
				CHOOSECOLOR cc = {0};
				COLORREF custColors[20];
				int i;

				for (i = 0; i < 20; i++)
					custColors[i] = GetSysColor(COLOR_3DFACE);

				cc.lStructSize = sizeof(cc);
				cc.hwndOwner = dat->pContainer->hwnd;
				cc.lpCustColors = custColors;
				if (ChooseColor(&cc)) {
					dat->avatarbg = cc.rgbResult;
					DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "avbg", dat->avatarbg);
				}
				break;
			}
			case ID_PICMENU_SETTINGS: {
				if (menuId == MENU_PANELPICMENU)
					CallService(MS_AV_CONTACTOPTIONS, (WPARAM)dat->hContact, 0);
				else if (menuId == MENU_PICMENU) {
					if (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
						if (ServiceExists(MS_AV_SETMYAVATAR) && CallService(MS_AV_CANSETMYAVATAR, (WPARAM)(dat->bIsMeta ? dat->szMetaProto : dat->szProto), 0))
							CallService(MS_AV_SETMYAVATAR, (WPARAM)(dat->bIsMeta ? dat->szMetaProto : dat->szProto), 0);
					} else
						CallService(MS_AV_CONTACTOPTIONS, (WPARAM)dat->hContact, 0);
				}
			}
			return 1;
		}
	} else if (menuId == MENU_LOGMENU) {
		int iLocalTime = 0;
		int iRtl = (myGlobals.m_RTLDefault == 0 ? DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0) : DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 1));
		int iLogStatus = (myGlobals.m_LogStatusChanges != 0) && dat->dwFlags & MWF_LOG_STATUSCHANGES;

		DWORD dwOldFlags = dat->dwFlags;

		switch (selection) {
			case ID_MESSAGELOGSETTINGS_GLOBAL: {
				OPENOPTIONSDIALOG	ood = {0};

				ood.cbSize = sizeof(OPENOPTIONSDIALOG);
				ood.pszGroup = NULL;
				ood.pszPage = "Message Sessions";
				ood.pszTab = NULL;
				DBWriteContactSettingByte(0, SRMSGMOD_T, "opage", 3);			// force 3th tab to appear
				CallService (MS_OPT_OPENOPTIONS, 0, (LPARAM)&ood);
				return 1;
			}
			case ID_MESSAGELOGSETTINGS_FORTHISCONTACT:
				CallService(MS_TABMSG_SETUSERPREFS, (WPARAM)dat->hContact, (LPARAM)0);
				return 1;

			case ID_MESSAGELOG_EXPORTMESSAGELOGSETTINGS: {
				char *szFilename = GetThemeFileName(1);
				if (szFilename != NULL)
					WriteThemeToINI(szFilename, dat);
				return 1;
			}
			case ID_MESSAGELOG_IMPORTMESSAGELOGSETTINGS: {
				char *szFilename = GetThemeFileName(0);
				DWORD dwFlags = THEME_READ_FONTS;
				int   result;

				if (szFilename != NULL) {
					result = MessageBox(0, TranslateT("Do you want to also read message templates from the theme?\nCaution: This will overwrite the stored template set which may affect the look of your message window significantly.\nSelect cancel to not load anything at all."),
										TranslateT("Load theme"), MB_YESNOCANCEL);
					if (result == IDCANCEL)
						return 1;
					else if (result == IDYES)
						dwFlags |= THEME_READ_TEMPLATES;
					ReadThemeFromINI(szFilename, 0, 0, dwFlags);
					CacheLogFonts();
					CacheMsgLogIcons();
					WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 1, 0);
					WindowList_Broadcast(hMessageWindowList, DM_FORCEDREMAKELOG, (WPARAM)hwndDlg, (LPARAM)(dat->dwFlags & MWF_LOG_ALL));
				}
				return 1;
			}
		}
	}
	return 0;
}

/*
 * update the status bar field which displays the number of characters in the input area
 * and various indicators (caps lock, num lock, insert mode).
 */

void UpdateReadChars(HWND hwndDlg, struct MessageWindowData *dat)
{

	if (dat->pContainer->hwndStatus && SendMessage(dat->pContainer->hwndStatus, SB_GETPARTS, 0, 0) >= 3) {
		char buf[128];
		int len;
		char szIndicators[20];
		BOOL fCaps, fNum;

		szIndicators[0] = 0;
		if (dat->bType == SESSIONTYPE_CHAT)
			len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
		else {
#if defined(_UNICODE)
			/*
			 * retrieve text length in UTF8 bytes, because this is the relevant length for most protocols
			 */
			GETTEXTLENGTHEX gtxl = {0};
			gtxl.codepage = CP_UTF8;
			gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMBYTES;

			len = SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETTEXTLENGTHEX, (WPARAM) & gtxl, 0);
#else
			len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
#endif
		}

		fCaps = (GetKeyState(VK_CAPITAL) & 1);
		fNum = (GetKeyState(VK_NUMLOCK) & 1);

		if (dat->fInsertMode)
			lstrcatA(szIndicators, "O");
		if (fCaps)
			lstrcatA(szIndicators, "C");
		if (fNum)
			lstrcatA(szIndicators, "N");
		if (dat->fInsertMode || fCaps || fNum)
			lstrcatA(szIndicators, " | ");

		_snprintf(buf, sizeof(buf), "%s%s %d/%d", szIndicators, dat->lcID, dat->iOpenJobs, len);
		SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 1, (LPARAM) buf);
	}
}

/*
 * update all status bar fields and force a redraw of the status bar.
 */

void UpdateStatusBar(HWND hwndDlg, struct MessageWindowData *dat)
{
	if (dat && dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
		if (dat->bType == SESSIONTYPE_IM)
			DM_UpdateLastMessage(hwndDlg, dat);
		UpdateReadChars(hwndDlg, dat);
		InvalidateRect(dat->pContainer->hwndStatus, NULL, TRUE);
		SendMessage(dat->pContainer->hwndStatus, WM_USER + 101, 0, (LPARAM)dat);
	}
}

/*
 * provide user feedback via icons on tabs. Used to indicate "send in progress" or
 * any error state.
 *
 * NOT used for typing notification feedback as this is handled directly from the
 * MTN handler.
 */

void HandleIconFeedback(HWND hwndDlg, struct MessageWindowData *dat, HICON iIcon)
{
	TCITEM item = {0};
	HICON iOldIcon = dat->hTabIcon;

	if (iIcon == (HICON) - 1) { // restore status image
		if (dat->dwFlags & MWF_ERRORSTATE) {
			dat->hTabIcon = myGlobals.g_iconErr;
		} else {
			dat->hTabIcon = dat->hTabStatusIcon;
		}
	} else
		dat->hTabIcon = iIcon;
	item.iImage = 0;
	item.mask = TCIF_IMAGE;
	TabCtrl_SetItem(GetDlgItem(dat->pContainer->hwnd, IDC_MSGTABS), dat->iTabID, &item);
}

/*
 * retrieve the visiblity of the avatar window, depending on the global setting
 * and local mode
 */

int GetAvatarVisibility(HWND hwndDlg, struct MessageWindowData *dat)
{
	BYTE bAvatarMode = myGlobals.m_AvatarMode;
	BYTE bOwnAvatarMode = myGlobals.m_OwnAvatarMode;
	char hideOverride = (char)DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "hideavatar", -1);
	// infopanel visible, consider own avatar display

	dat->showPic = 0;

	if (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
		if (bOwnAvatarMode)
			dat->showPic = FALSE;
		else {
			FLASHAVATAR fa = {0};
			if (myGlobals.g_FlashAvatarAvail) {
				fa.cProto = dat->szProto;
				fa.id = 25367;
				fa.hParentWindow = GetDlgItem(hwndDlg, IDC_CONTACTPIC);

				CallService(MS_FAVATAR_MAKE, (WPARAM)&fa, 0);
				if (fa.hWindow != NULL&&dat->hwndContactPic) {
					DestroyWindow(dat->hwndContactPic);
					dat->hwndContactPic = NULL;
				}
				dat->showPic = ((dat->hOwnPic && dat->hOwnPic != myGlobals.g_hbmUnknown) || (fa.hWindow != NULL)) ? 1 : 0;
			} else
				dat->showPic = (dat->hOwnPic && dat->hOwnPic != myGlobals.g_hbmUnknown) ? 1 : 0;

			if (!myGlobals.g_bDisableAniAvatars&&fa.hWindow == NULL&&!dat->hwndContactPic)
				dat->hwndContactPic =CreateWindowEx(WS_EX_TOPMOST, AVATAR_CONTROL_CLASS, _T(""), WS_VISIBLE | WS_CHILD, 1, 1, 1, 1, GetDlgItem(hwndDlg, IDC_CONTACTPIC), (HMENU)0, NULL, NULL);

		}
		switch (bAvatarMode) {
			case 0:
			case 1:
				dat->showInfoPic = 1;
				break;
			case 4:
				dat->showInfoPic = 0;
				break;
			case 3: {
				FLASHAVATAR fa = {0};
				HBITMAP hbm = ((dat->ace && !(dat->ace->dwFlags & AVS_HIDEONCLIST)) ? dat->ace->hbmPic : 0);

				if (myGlobals.g_FlashAvatarAvail) {
					fa.cProto = dat->szProto;
					fa.id = 25367;
					fa.hContact = dat->hContact;
					fa.hParentWindow = GetDlgItem(hwndDlg, IDC_PANELPIC);

					CallService(MS_FAVATAR_MAKE, (WPARAM)&fa, 0);
					if (fa.hWindow != NULL&&dat->hwndPanelPic) {
						DestroyWindow(dat->hwndPanelPic);
						dat->hwndPanelPic = NULL;
					}
				}
				if (!myGlobals.g_bDisableAniAvatars&&fa.hWindow == NULL&&!dat->hwndPanelPic) {
					dat->hwndPanelPic =CreateWindowEx(WS_EX_TOPMOST, AVATAR_CONTROL_CLASS, _T(""), WS_VISIBLE | WS_CHILD, 1, 1, 1, 1, GetDlgItem(hwndDlg, IDC_PANELPIC), (HMENU)0, NULL, NULL);

				}
				if ((hbm && hbm != myGlobals.g_hbmUnknown) || (fa.hWindow != NULL))
					dat->showInfoPic = 1;
				else
					dat->showInfoPic = 0;
				break;
			}
		}
		if (dat->showInfoPic)
			dat->showInfoPic = hideOverride == 0 ? 0 : dat->showInfoPic;
		else
			dat->showInfoPic = hideOverride == 1 ? 1 : dat->showInfoPic;
		//Bolshevik: reloads avatars
		if (dat->showInfoPic)
// 			if(dat->hwndPanelPic)
		{
			/** panel and contact is shown, reloads contact's avatar -> panel
			*  user avatar -> bottom picture
			*/
			SendMessage(dat->hwndPanelPic, AVATAR_SETCONTACT, (WPARAM)0, (LPARAM)dat->hContact);
			if (dat->hwndContactPic)
				SendMessage(dat->hwndContactPic, AVATAR_SETPROTOCOL, (WPARAM)0, (LPARAM)dat->szProto);
		}
// 			else
// 			{
// 				//show only contact picture at bottom
// 				if(dat->hwndContactPic)
// 					SendMessage(dat->hwndContactPic, AVATAR_SETCONTACT, (WPARAM)0, (LPARAM)dat->hContact);
// 			}
		else
			//show only user picture(contact is unaccessible)
			if (dat->hwndContactPic)
				SendMessage(dat->hwndContactPic, AVATAR_SETPROTOCOL, (WPARAM)0, (LPARAM)dat->szProto);
//Bolshevik_
	} else {
		dat->showInfoPic = 0;

		switch (bAvatarMode) {
			case 0:             // globally on
				dat->showPic = 1;
				break;
			case 4:             // globally OFF
				dat->showPic = 0;
				break;
			case 3:             // on, if present
			case 2:
			case 1: {
				FLASHAVATAR fa = {0};
				HBITMAP hbm = (dat->ace && !(dat->ace->dwFlags & AVS_HIDEONCLIST)) ? dat->ace->hbmPic : 0;

				if (myGlobals.g_FlashAvatarAvail) {
					fa.cProto = dat->szProto;
					fa.id = 25367;
					fa.hContact = dat->hContact;
					fa.hParentWindow = GetDlgItem(hwndDlg, IDC_CONTACTPIC);

					CallService(MS_FAVATAR_MAKE, (WPARAM)&fa, 0);

					if (fa.hWindow != NULL&&dat->hwndContactPic) {
						DestroyWindow(dat->hwndContactPic);
						dat->hwndContactPic = NULL;
					}
				}
				if (!myGlobals.g_bDisableAniAvatars&&fa.hWindow == NULL&&!dat->hwndContactPic)
					dat->hwndContactPic =CreateWindowEx(WS_EX_TOPMOST, AVATAR_CONTROL_CLASS, _T(""), WS_VISIBLE | WS_CHILD, 1, 1, 1, 1, GetDlgItem(hwndDlg, IDC_CONTACTPIC), (HMENU)0, NULL, NULL);

				if ((hbm && hbm != myGlobals.g_hbmUnknown) || (fa.hWindow != NULL))
					dat->showPic = 1;
				else
					dat->showPic = 0;
				break;
			}
		}
		if (dat->showPic)
			dat->showPic = hideOverride == 0 ? 0 : dat->showPic;
		else
			dat->showPic = hideOverride == 1 ? 1 : dat->showPic;
		//Bolshevik: reloads avatars
		if (dat->showPic)
			//shows contact or user picture, depending on panel visibility
			if (dat->hwndPanelPic)
				SendMessage(dat->hwndContactPic, AVATAR_SETPROTOCOL, (WPARAM)0, (LPARAM)dat->szProto);
		if (dat->hwndContactPic)
			SendMessage(dat->hwndContactPic, AVATAR_SETCONTACT, (WPARAM)0, (LPARAM)dat->hContact);

	}
	return dat->showPic;
}

/*
 * checks, if there is a valid smileypack installed for the given protocol
 */

int CheckValidSmileyPack(char *szProto, HANDLE hContact, HICON *hButtonIcon)
{
	SMADD_INFO2 smainfo = {0};

	if (myGlobals.g_SmileyAddAvail) {
		smainfo.cbSize = sizeof(smainfo);
		smainfo.Protocolname = szProto;
		smainfo.hContact = hContact;
		CallService(MS_SMILEYADD_GETINFO2, 0, (LPARAM)&smainfo);
		if (hButtonIcon)
			*hButtonIcon = smainfo.ButtonIcon;
		return smainfo.NumberOfVisibleSmileys;
	} else
		return 0;
}

/*
 * return value MUST be free()'d by caller.
 */

TCHAR *QuoteText(TCHAR *text, int charsPerLine, int removeExistingQuotes)
{
	int inChar, outChar, lineChar;
	int justDoneLineBreak, bufSize;
	TCHAR *strout;

#ifdef _UNICODE
	bufSize = lstrlenW(text) + 23;
#else
	bufSize = strlen(text) + 23;
#endif
	strout = (TCHAR*)malloc(bufSize * sizeof(TCHAR));
	inChar = 0;
	justDoneLineBreak = 1;
	for (outChar = 0, lineChar = 0;text[inChar];) {
		if (outChar >= bufSize - 8) {
			bufSize += 20;
			strout = (TCHAR *)realloc(strout, bufSize * sizeof(TCHAR));
		}
		if (justDoneLineBreak && text[inChar] != '\r' && text[inChar] != '\n') {
			if (removeExistingQuotes)
				if (text[inChar] == '>') {
					while (text[++inChar] != '\n');
					inChar++;
					continue;
				}
			strout[outChar++] = '>';
			strout[outChar++] = ' ';
			lineChar = 2;
		}
		if (lineChar == charsPerLine && text[inChar] != '\r' && text[inChar] != '\n') {
			int decreasedBy;
			for (decreasedBy = 0;lineChar > 10;lineChar--, inChar--, outChar--, decreasedBy++)
				if (strout[outChar] == ' ' || strout[outChar] == '\t' || strout[outChar] == '-') break;
			if (lineChar <= 10) {
				lineChar += decreasedBy;
				inChar += decreasedBy;
				outChar += decreasedBy;
			} else inChar++;
			strout[outChar++] = '\r';
			strout[outChar++] = '\n';
			justDoneLineBreak = 1;
			continue;
		}
		strout[outChar++] = text[inChar];
		lineChar++;
		if (text[inChar] == '\n' || text[inChar] == '\r') {
			if (text[inChar] == '\r' && text[inChar+1] != '\n')
				strout[outChar++] = '\n';
			justDoneLineBreak = 1;
			lineChar = 0;
		} else justDoneLineBreak = 0;
		inChar++;
	}
	strout[outChar++] = '\r';
	strout[outChar++] = '\n';
	strout[outChar] = 0;
	return strout;
}


void AdjustBottomAvatarDisplay(HWND hwndDlg, struct MessageWindowData *dat)
{
	HBITMAP hbm = dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown);

	if (myGlobals.g_FlashAvatarAvail) {
		FLASHAVATAR fa = {0};

		fa.hContact = dat->hContact;
		fa.hWindow = 0;
		fa.id = 25367;
		fa.cProto = dat->szProto;
		CallService(MS_FAVATAR_GETINFO, (WPARAM)&fa, 0);
		if (fa.hWindow) {
			dat->hwndFlash = fa.hWindow;
			SetParent(dat->hwndFlash, GetDlgItem(hwndDlg, (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) ? IDC_PANELPIC : IDC_CONTACTPIC));
		}
		fa.hContact = 0;
		fa.hWindow = 0;
		if (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
			fa.hParentWindow = GetDlgItem(hwndDlg, IDC_CONTACTPIC);
			CallService(MS_FAVATAR_MAKE, (WPARAM)&fa, 0);
			if (fa.hWindow) {
				SetParent(fa.hWindow, GetDlgItem(hwndDlg, IDC_CONTACTPIC));
				ShowWindow(fa.hWindow, SW_SHOW);
			}
		} else {
			CallService(MS_FAVATAR_GETINFO, (WPARAM)&fa, 0);
			if (fa.hWindow)
				ShowWindow(fa.hWindow, SW_HIDE);
		}
	}

	if (hbm) {
		dat->showPic = GetAvatarVisibility(hwndDlg, dat);
		if (dat->dynaSplitter == 0 || dat->splitterY == 0)
			LoadSplitter(hwndDlg, dat);
		dat->dynaSplitter = dat->splitterY - DPISCALEY(34);
		DM_RecalcPictureSize(hwndDlg, dat);
		ShowWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), dat->showPic ? SW_SHOW : SW_HIDE);
		InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
	} else {
		dat->showPic = GetAvatarVisibility(hwndDlg, dat);
		ShowWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), dat->showPic ? SW_SHOW : SW_HIDE);
		dat->pic.cy = dat->pic.cx = DPISCALEY(60);
		InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
	}
}

void ShowPicture(HWND hwndDlg, struct MessageWindowData *dat, BOOL showNewPic)
{
	DBVARIANT dbv = {0};
	RECT rc;

	if (!(dat->dwFlagsEx & MWF_SHOW_INFOPANEL))
		dat->pic.cy = dat->pic.cx = DPISCALEY(60);

	if (showNewPic) {
		if (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
			if (!dat->hwndPanelPic)
				InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELPIC), NULL, FALSE);
			return;
		}
		AdjustBottomAvatarDisplay(hwndDlg, dat);
	} else {
		dat->showPic = dat->showPic ? 0 : 1;
		DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic", (BYTE)dat->showPic);
	}

	GetWindowRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), &rc);
	if (dat->minEditBoxSize.cy + DPISCALEY(3)> dat->splitterY)
		SendMessage(hwndDlg, DM_SPLITTERMOVED, (WPARAM)rc.bottom - dat->minEditBoxSize.cy, (LPARAM)GetDlgItem(hwndDlg, IDC_SPLITTER));
	if (!showNewPic)
		SetDialogToType(hwndDlg);
	else
		SendMessage(hwndDlg, WM_SIZE, 0, 0);

	SendMessage(hwndDlg, DM_CALCMINHEIGHT, 0, 0);
	SendMessage(dat->pContainer->hwnd, DM_REPORTMINHEIGHT, (WPARAM) hwndDlg, (LPARAM) dat->uMinHeight);
}

void FlashOnClist(HWND hwndDlg, struct MessageWindowData *dat, HANDLE hEvent, DBEVENTINFO *dbei)
{
	CLISTEVENT cle;

	dat->dwTickLastEvent = GetTickCount();
	if ((GetForegroundWindow() != dat->pContainer->hwnd || dat->pContainer->hwndActive != hwndDlg) && !(dbei->flags & DBEF_SENT) && dbei->eventType == EVENTTYPE_MESSAGE) {
		UpdateTrayMenu(dat, (WORD)(dat->bIsMeta ? dat->wMetaStatus : dat->wStatus), dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->szStatus, dat->hContact, 0L);
		if (nen_options.bTraySupport == TRUE && myGlobals.m_WinVerMajor >= 5)
			return;
	}

	if (hEvent == 0)
		return;

	if (!myGlobals.m_FlashOnClist)
		return;
	if ((GetForegroundWindow() != dat->pContainer->hwnd || dat->pContainer->hwndActive != hwndDlg) && !(dbei->flags & DBEF_SENT) && dbei->eventType == EVENTTYPE_MESSAGE && !(dat->dwFlagsEx & MWF_SHOW_FLASHCLIST)) {
		ZeroMemory(&cle, sizeof(cle));
		cle.cbSize = sizeof(cle);
		cle.hContact = (HANDLE) dat->hContact;
		cle.hDbEvent = hEvent;
		cle.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
		cle.pszService = "SRMsg/ReadMessage";
		CallService(MS_CLIST_ADDEVENT, 0, (LPARAM) & cle);
		dat->dwFlagsEx |= MWF_SHOW_FLASHCLIST;
		dat->hFlashingEvent = hEvent;
	}
}

/*
 * callback function for text streaming
 */

static DWORD CALLBACK Message_StreamCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
	static DWORD dwRead;
	char ** ppText = (char **) dwCookie;

	if (*ppText == NULL) {
		*ppText = malloc(cb + 2);
		CopyMemory(*ppText, pbBuff, cb);
		*pcb = cb;
		dwRead = cb;
		*(*ppText + cb) = '\0';
	} else {
		char  *p = realloc(*ppText, dwRead + cb + 2);
		CopyMemory(p + dwRead, pbBuff, cb);
		*ppText = p;
		*pcb = cb;
		dwRead += cb;
		*(*ppText + dwRead) = '\0';
	}
	return 0;
}

/*
 * retrieve contents of the richedit control by streaming. Used to get the
 * typed message before sending it.
 * caller must free the returned pointer.
 * UNICODE version returns UTF-8 encoded string.
 */

char * Message_GetFromStream(HWND hwndRtf, struct MessageWindowData* dat, DWORD dwPassedFlags)
{
	EDITSTREAM stream;
	char *pszText = NULL;
	DWORD dwFlags = 0;

	if (hwndRtf == 0 || dat == 0)
		return NULL;

	ZeroMemory(&stream, sizeof(stream));
	stream.pfnCallback = Message_StreamCallback;
	stream.dwCookie = (DWORD) & pszText; // pass pointer to pointer
#if defined(_UNICODE)
	if (dwPassedFlags == 0)
		dwFlags = (CP_UTF8 << 16) | (SF_RTFNOOBJS | SFF_PLAINRTF | SF_USECODEPAGE);
	else
		dwFlags = (CP_UTF8 << 16) | dwPassedFlags;
#else
	if (dwPassedFlags == 0)
		dwFlags = SF_RTF | SF_RTFNOOBJS | SFF_PLAINRTF;
	else
		dwFlags = dwPassedFlags;
#endif
	SendMessage(hwndRtf, EM_STREAMOUT, (WPARAM)dwFlags, (LPARAM) & stream);

	return pszText; // pszText contains the text
}

static void CreateColorMap(TCHAR *Text)
{
	TCHAR * pszText = Text;
	TCHAR * p1;
	TCHAR * p2;
	TCHAR * pEnd;
	int iIndex = 1, i = 0;
	COLORREF default_color;

	static const TCHAR *lpszFmt = _T("\\red%[^ \x5b\\]\\green%[^ \x5b\\]\\blue%[^ \x5b;];");
	TCHAR szRed[10], szGreen[10], szBlue[10];

	p1 = _tcsstr(pszText, _T("\\colortbl"));
	if (!p1)
		return;

	pEnd = _tcschr(p1, '}');

	p2 = _tcsstr(p1, _T("\\red"));

	for (i = 0; i < RTF_CTABLE_DEFSIZE; i++)
		rtf_ctable[i].index = 0;

	default_color = (COLORREF)DBGetContactSettingDword(NULL, SRMSGMOD_T, "Font16Col", 0);

	while (p2 && p2 < pEnd) {
		if (_stscanf(p2, lpszFmt, &szRed, &szGreen, &szBlue) > 0) {
			int i;
			for (i = 0; i < RTF_CTABLE_DEFSIZE; i++) {
				if (rtf_ctable[i].clr == RGB(_ttoi(szRed), _ttoi(szGreen), _ttoi(szBlue)))
					rtf_ctable[i].index = iIndex;
			}
		}
		iIndex++;
		p1 = p2;
		p1 ++;

		p2 = _tcsstr(p1, _T("\\red"));
	}
	return ;
}

int RTFColorToIndex(int iCol)
{
	int i = 0;
	for (i = 0; i < RTF_CTABLE_DEFSIZE; i++) {
		if (rtf_ctable[i].index == iCol)
			return i + 1;
	}
	return 0;
}

/*
 * convert rich edit code to bbcode (if wanted). Otherwise, strip all RTF formatting
 * tags and return plain text
 */

BOOL DoRtfToTags(TCHAR * pszText, struct MessageWindowData *dat)
{
	TCHAR * p1;
	BOOL bJustRemovedRTF = TRUE;
	BOOL bTextHasStarted = FALSE;
	LOGFONTA lf;
	COLORREF color;
	static int inColor = 0;

	if (!pszText)
		return FALSE;

	/*
	 * used to filter out attributes which are already set for the default message input area
	 * font
	 */

	lf = dat->theme.logFonts[MSGFONTID_MESSAGEAREA];
	color = dat->theme.fontColors[MSGFONTID_MESSAGEAREA];

	// create an index of colors in the module and map them to
	// corresponding colors in the RTF color table

	CreateColorMap(pszText);
	// scan the file for rtf commands and remove or parse them
	inColor = 0;
	p1 = _tcsstr(pszText, _T("\\pard"));
	if (p1) {
		int iRemoveChars;
		TCHAR InsertThis[50];
		p1 += 5;

		MoveMemory(pszText, p1, (_tcslen(p1) + 1) * sizeof(TCHAR));
		p1 = pszText;
		// iterate through all characters, if rtf control character found then take action
		while (*p1 != (TCHAR)'\0') {
			_sntprintf(InsertThis, 50, _T(""));
			iRemoveChars = 0;

			switch (*p1) {
				case(TCHAR)'\\':
								if (p1 == _tcsstr(p1, _T("\\cf"))) { // foreground color
							TCHAR szTemp[20];
							int iCol = _ttoi(p1 + 3);
							int iInd = RTFColorToIndex(iCol);
							bJustRemovedRTF = TRUE;

							_sntprintf(szTemp, 20, _T("%d"), iCol);
							iRemoveChars = 3 + _tcslen(szTemp);
							if (bTextHasStarted || iCol)
								_sntprintf(InsertThis, sizeof(InsertThis) / sizeof(TCHAR), (iInd > 0) ? (inColor ? _T("[/color][color=%s]") : _T("[color=%s]")) : (inColor ? _T("[/color]") : _T("")), rtf_ctable[iInd  - 1].szName);
							inColor = iInd > 0 ? 1 : 0;
						} else if (p1 == _tcsstr(p1, _T("\\highlight"))) { //background color
							TCHAR szTemp[20];
							int iCol = _ttoi(p1 + 10);
							//int iInd = RTFColorToIndex(pIndex, iCol, dat);
							bJustRemovedRTF = TRUE;

							_sntprintf(szTemp, 20, _T("%d"), iCol);
							iRemoveChars = 10 + _tcslen(szTemp);
							//if(bTextHasStarted || iInd >= 0)
							//	_snprintf(InsertThis, sizeof(InsertThis), ( iInd >= 0 ) ? _T("%%f%02u") : _T("%%F"), iInd);
						} else if (p1 == _tcsstr(p1, _T("\\par"))) { // newline
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = 4;
							//_sntprintf(InsertThis, sizeof(InsertThis), _T("\n"));
						} else if (p1 == _tcsstr(p1, _T("\\line"))) { // soft line break;
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = 5;
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("\n"));
						} else if (p1 == _tcsstr(p1, _T("\\emdash"))) {
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = 7;
#if defined(_UNICODE)
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("%c"), 0x2014);
#else
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("-"));
#endif
						} else if (p1 == _tcsstr(p1, _T("\\bullet"))) {
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = 7;
#if defined(_UNICODE)
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("%c"), 0x2022);
#else
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("*"));
#endif
						} else if (p1 == _tcsstr(p1, _T("\\ldblquote"))) {
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = 10;
#if defined(_UNICODE)
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("%c"), 0x201C);
#else
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("\""));
#endif
						} else if (p1 == _tcsstr(p1, _T("\\rdblquote"))) {
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = 10;
#if defined(_UNICODE)
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("%c"), 0x201D);
#else
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("\""));
#endif
						} else if (p1 == _tcsstr(p1, _T("\\b"))) { //bold
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = (p1[2] != (TCHAR)'0') ? 2 : 3;
							if (!(lf.lfWeight == FW_BOLD)) {         // only allow bold if the font itself isn't a bold one, otherwise just strip it..
								if (dat->SendFormat)
									_sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("[b]") : _T("[/b]"));
							}

						} else if (p1 == _tcsstr(p1, _T("\\i"))) { // italics
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = (p1[2] != (TCHAR)'0') ? 2 : 3;
							if (!lf.lfItalic) {                      // same as for bold
								if (dat->SendFormat)
									_sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[2] != (TCHAR)'0') ? _T("[i]") : _T("[/i]"));
							}

						} else if (p1 == _tcsstr(p1, _T("\\strike"))) { // strike-out
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = (p1[7] != (TCHAR)'0') ? 7 : 8;
							if (!lf.lfStrikeOut) {                      // same as for bold
								if (dat->SendFormat)
									_sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[7] != (TCHAR)'0') ? _T("[s]") : _T("[/s]"));
							}

						} else if (p1 == _tcsstr(p1, _T("\\ul"))) { // underlined
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							if (p1[3] == (TCHAR)'n')
								iRemoveChars = 7;
							else if (p1[3] == (TCHAR)'0')
								iRemoveChars = 4;
							else
								iRemoveChars = 3;
							if (!lf.lfUnderline)  {                  // same as for bold
								if (dat->SendFormat)
									_sntprintf(InsertThis, safe_sizeof(InsertThis), (p1[3] != (TCHAR)'0' && p1[3] != (TCHAR)'n') ? _T("[u]") : _T("[/u]"));
							}

						} else if (p1 == _tcsstr(p1, _T("\\tab"))) { // tab
							bTextHasStarted = TRUE;
							bJustRemovedRTF = TRUE;
							iRemoveChars = 4;
#if defined(_UNICODE)
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("%c"), 0x09);
#else
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T(" "));
#endif
						} else if (p1[1] == (TCHAR)'\\' || p1[1] == (TCHAR)'{' || p1[1] == (TCHAR)'}') { // escaped characters
							bTextHasStarted = TRUE;
							//bJustRemovedRTF = TRUE;
							iRemoveChars = 2;
							_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("%c"), p1[1]);
						} else if (p1[1] == (TCHAR)'\'') { // special character
							bTextHasStarted = TRUE;
							bJustRemovedRTF = FALSE;
							if (p1[2] != (TCHAR)' ' && p1[2] != (TCHAR)'\\') {
								int iLame = 0;
								TCHAR * p3;
								TCHAR *stoppedHere;

								if (p1[3] != (TCHAR)' ' && p1[3] != (TCHAR)'\\') {
									_tcsncpy(InsertThis, p1 + 2, 3);
									iRemoveChars = 4;
									InsertThis[2] = 0;
								} else {
									_tcsncpy(InsertThis, p1 + 2, 2);
									iRemoveChars = 3;
									InsertThis[2] = 0;
								}
								// convert string containing char in hex format to int.
								p3 = InsertThis;
								iLame = _tcstol(p3, &stoppedHere, 16);
								_sntprintf(InsertThis, safe_sizeof(InsertThis), _T("%c"), (TCHAR)iLame);

							} else
								iRemoveChars = 2;
						} else { // remove unknown RTF command
							int j = 1;
							bJustRemovedRTF = TRUE;
							while (!_tcschr(_T(" !$%()#*\"'"), p1[j]) && p1[j] != (TCHAR)'§' && p1[j] != (TCHAR)'\\' && p1[j] != (TCHAR)'\0')
//                    while(!_tcschr(_T(" !§$%&()#*"), p1[j]) && p1[j] != (TCHAR)'\\' && p1[j] != (TCHAR)'\0')
								j++;
//					while(p1[j] != (TCHAR)' ' && p1[j] != (TCHAR)'\\' && p1[j] != (TCHAR)'\0')
//						j++;
							iRemoveChars = j;
						}
					break;

				case(TCHAR)'{':  // other RTF control characters
							case(TCHAR)'}':
									iRemoveChars = 1;
					break;

				case(TCHAR)' ':  // remove spaces following a RTF command
								if (bJustRemovedRTF)
									iRemoveChars = 1;
					bJustRemovedRTF = FALSE;
					bTextHasStarted = TRUE;
					break;

				default: // other text that should not be touched
					bTextHasStarted = TRUE;
					bJustRemovedRTF = FALSE;

					break;
			}

			// move the memory and paste in new commands instead of the old RTF
			if (_tcslen(InsertThis) || iRemoveChars) {
				MoveMemory(p1 + _tcslen(InsertThis) , p1 + iRemoveChars, (_tcslen(p1) - iRemoveChars + 1) * sizeof(TCHAR));
				CopyMemory(p1, InsertThis, _tcslen(InsertThis) * sizeof(TCHAR));
				p1 += _tcslen(InsertThis);
			} else
				p1++;
		}


	} else {
		return FALSE;
	}
	return TRUE;
}

/*
 * trims the output from DoRtfToTags(), removes trailing newlines and whitespaces...
 */

void DoTrimMessage(TCHAR *msg)
{
	int iLen = _tcslen(msg);
	int i = iLen;

	while (i && (msg[i-1] == '\r' || msg[i-1] == '\n') || msg[i-1] == ' ') {
		i--;
	}
	if (i < iLen)
		msg[i] = '\0';
}

/*
 * saves message to the input history.
 * its using streamout in UTF8 format - no unicode "issues" and all RTF formatting is saved aswell,
 * so restoring a message from the input history will also restore its formatting
 */

void SaveInputHistory(HWND hwndDlg, struct MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
	int iLength = 0, iStreamLength = 0;
	int oldTop = 0;
	char *szFromStream = NULL;

	if (wParam) {
		oldTop = dat->iHistoryTop;
		dat->iHistoryTop = (int)wParam;
	}

	szFromStream = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, (CP_UTF8 << 16) | (SF_RTFNOOBJS | SFF_PLAINRTF | SF_USECODEPAGE));

	iLength = iStreamLength = (lstrlenA(szFromStream) + 1);

	if (iLength > 0 && dat->history != NULL) {
		if ((dat->iHistoryTop == dat->iHistorySize) && oldTop == 0) {         // shift the stack down...
			struct InputHistory ihTemp = dat->history[0];
			dat->iHistoryTop--;
			MoveMemory((void *)&dat->history[0], (void *)&dat->history[1], (dat->iHistorySize - 1) * sizeof(struct InputHistory));
			dat->history[dat->iHistoryTop] = ihTemp;
		}
		if (iLength > dat->history[dat->iHistoryTop].lLen) {
			if (dat->history[dat->iHistoryTop].szText == NULL) {
				if (iLength < HISTORY_INITIAL_ALLOCSIZE)
					iLength = HISTORY_INITIAL_ALLOCSIZE;
				dat->history[dat->iHistoryTop].szText = (TCHAR *)malloc(iLength);
				dat->history[dat->iHistoryTop].lLen = iLength;
			} else {
				if (iLength > dat->history[dat->iHistoryTop].lLen) {
					dat->history[dat->iHistoryTop].szText = (TCHAR *)realloc(dat->history[dat->iHistoryTop].szText, iLength);
					dat->history[dat->iHistoryTop].lLen = iLength;
				}
			}
		}
		CopyMemory(dat->history[dat->iHistoryTop].szText, szFromStream, iStreamLength);
		if (!oldTop) {
			if (dat->iHistoryTop < dat->iHistorySize) {
				dat->iHistoryTop++;
				dat->iHistoryCurrent = dat->iHistoryTop;
			}
		}
	}
	if (szFromStream)
		free(szFromStream);

	if (oldTop)
		dat->iHistoryTop = oldTop;
}

/*
 * retrieve both buddys and my own UIN for a message session and store them in the message window *dat
 * respects metacontacts and uses the current protocol if the contact is a MC
 */

void GetContactUIN(HWND hwndDlg, struct MessageWindowData *dat)
{
	CONTACTINFO ci;

	ZeroMemory((void *)&ci, sizeof(ci));

	if (dat->bIsMeta) {
		ci.hContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)dat->hContact, 0);
		ci.szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)ci.hContact, 0);
	} else {
		ci.hContact = dat->hContact;
		ci.szProto = dat->szProto;
	}
	ci.cbSize = sizeof(ci);
	ci.dwFlag = CNF_DISPLAYUID;
	if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
		switch (ci.type) {
			case CNFT_ASCIIZ:
				mir_snprintf(dat->uin, sizeof(dat->uin), "%s", ci.pszVal);
				mir_free(ci.pszVal);
				break;
			case CNFT_DWORD:
				mir_snprintf(dat->uin, sizeof(dat->uin), "%u", ci.dVal);
				break;
		}
	} else
		dat->uin[0] = 0;

	// get own uin...
	ci.hContact = 0;

	if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
		switch (ci.type) {
			case CNFT_ASCIIZ:
				mir_snprintf(dat->myUin, sizeof(dat->myUin), "%s", ci.pszVal);
				mir_free(ci.pszVal);
				break;
			case CNFT_DWORD:
				mir_snprintf(dat->myUin, sizeof(dat->myUin), "%u", ci.dVal);
				break;
		}
	} else
		dat->myUin[0] = 0;
	if (dat->dwFlagsEx & MWF_SHOW_INFOPANEL)
		SetDlgItemTextA(hwndDlg, IDC_PANELUIN, dat->uin);
}

static int g_IEViewAvail = -1;
static int g_HPPAvail = -1;

#define WANT_IEVIEW_LOG 1
#define WANT_HPP_LOG 2

//MAD
HWND GetLastChild(HWND hwndParent)
{
	HWND hwnd = hwndParent;
	char classbuf[25]={'\0'};
	WCHAR classbufW[25]={_T('\0')};
	while (TRUE) {
		HWND hwndChild = GetWindow(hwnd, GW_CHILD);
		if (hwndChild==NULL)
			return hwnd;
		hwnd = hwndChild;
		if ((GetClassNameA(hwnd,classbuf,25)&&!strcmp(classbuf,"Internet Explorer_Server"))||(GetClassNameW(hwnd,classbufW,25)&&!wcscmp(classbufW,L"Internet Explorer_Server")))
			return hwnd;
	}
	return NULL;
}

//MAD_

unsigned int GetIEViewMode(HWND hwndDlg, HANDLE hContact)
{
	int iWantIEView = 0, iWantHPP = 0;

	if (g_IEViewAvail == -1)
		g_IEViewAvail = ServiceExists(MS_IEVIEW_WINDOW);

	if (g_HPPAvail == -1)
		g_HPPAvail = ServiceExists("History++/ExtGrid/NewWindow");

	myGlobals.g_WantIEView = g_IEViewAvail && DBGetContactSettingByte(NULL, SRMSGMOD_T, "default_ieview", 0);
	myGlobals.g_WantHPP = g_HPPAvail && DBGetContactSettingByte(NULL, SRMSGMOD_T, "default_hpp", 1);

	iWantIEView = (myGlobals.g_WantIEView) || (DBGetContactSettingByte(hContact, SRMSGMOD_T, "ieview", 0) == 1 && g_IEViewAvail);
	iWantIEView = (DBGetContactSettingByte(hContact, SRMSGMOD_T, "ieview", 0) == (BYTE) - 1) ? 0 : iWantIEView;

	iWantHPP = (myGlobals.g_WantHPP) || (DBGetContactSettingByte(hContact, SRMSGMOD_T, "hpplog", 0) == 1 && g_HPPAvail);
	iWantHPP = (DBGetContactSettingByte(hContact, SRMSGMOD_T, "hpplog", 0) == (BYTE) - 1) ? 0 : iWantHPP;

	return iWantHPP ? WANT_HPP_LOG : (iWantIEView ? WANT_IEVIEW_LOG : 0);
}

void GetRealIEViewWindow(HWND hwndDlg, struct MessageWindowData *dat)
{
	POINT pt;

	pt.x = 10;
	pt.y = dat->panelHeight + 10;
	if (dat->hwndIEView)
		dat->hwndIWebBrowserControl = ChildWindowFromPointEx(dat->hwndIEView, pt, CWP_SKIPDISABLED);
	else if (dat->hwndHPP)
		dat->hwndIWebBrowserControl = ChildWindowFromPointEx(dat->hwndHPP, pt, CWP_SKIPDISABLED);
}

static void CheckAndDestroyHPP(struct MessageWindowData *dat)
{
	if (dat->hwndHPP) {
		IEVIEWWINDOW ieWindow;
		ieWindow.cbSize = sizeof(IEVIEWWINDOW);
		ieWindow.iType = IEW_DESTROY;
		ieWindow.hwnd = dat->hwndHPP;
		//MAD: restore old wndProc
		if(dat->oldIEViewProc){
			SetWindowLong(dat->hwndHPP, GWL_WNDPROC, (LONG)dat->oldIEViewProc);
			dat->oldIEViewProc=0;
			}
		//MAD_
		CallService(MS_HPP_EG_WINDOW, 0, (LPARAM)&ieWindow);
		dat->hwndHPP = 0;
	}
}

static void CheckAndDestroyIEView(struct MessageWindowData *dat)
{
	if (dat->hwndIEView) {
		IEVIEWWINDOW ieWindow;
		ieWindow.cbSize = sizeof(IEVIEWWINDOW);
		ieWindow.iType = IEW_DESTROY;
		ieWindow.hwnd = dat->hwndIEView;
		if (dat->oldIEViewProc){
			SetWindowLong(dat->hwndIEView, GWL_WNDPROC, (LONG)dat->oldIEViewProc);
			dat->oldIEViewProc =0;
			}
		if (dat->oldIEViewLastChildProc) {
			//mad: get LAST child, because ieserver has many of them..
			SetWindowLong(GetLastChild(dat->hwndIEView), GWL_WNDPROC, (LONG)dat->oldIEViewLastChildProc);
			dat->oldIEViewLastChildProc = 0;
			}
		//mad_
		CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
		dat->oldIEViewProc = 0;
		dat->hwndIEView = 0;
	}
}

void SetMessageLog(HWND hwndDlg, struct MessageWindowData *dat)
{
	unsigned int iLogMode = GetIEViewMode(hwndDlg, dat->hContact);

	if (iLogMode == WANT_IEVIEW_LOG && dat->hwndIEView == 0) {
		IEVIEWWINDOW ieWindow;

		CheckAndDestroyHPP(dat);
		ieWindow.cbSize = sizeof(IEVIEWWINDOW);
		ieWindow.iType = IEW_CREATE;
		ieWindow.dwFlags = 0;
		ieWindow.dwMode = IEWM_TABSRMM;
		ieWindow.parent = hwndDlg;
		ieWindow.x = 0;
		ieWindow.y = 0;
		ieWindow.cx = 10;
		ieWindow.cy = 10;
		CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
		dat->hwndIEView = ieWindow.hwnd;
		ShowWindow(GetDlgItem(hwndDlg, IDC_LOG), SW_HIDE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), FALSE);
	} else if (iLogMode == WANT_HPP_LOG && dat->hwndHPP == 0) {
		IEVIEWWINDOW ieWindow;

		CheckAndDestroyIEView(dat);
		ieWindow.cbSize = sizeof(IEVIEWWINDOW);
		ieWindow.iType = IEW_CREATE;
		ieWindow.dwFlags = 0;
		ieWindow.dwMode = IEWM_TABSRMM;
		ieWindow.parent = hwndDlg;
		ieWindow.x = 0;
		ieWindow.y = 0;
		ieWindow.cx = 10;
		ieWindow.cy = 10;
		CallService(MS_HPP_EG_WINDOW, 0, (LPARAM)&ieWindow);
		dat->hwndHPP = ieWindow.hwnd;
		ShowWindow(GetDlgItem(hwndDlg, IDC_LOG), SW_HIDE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), FALSE);
	} else {
		if (iLogMode != WANT_IEVIEW_LOG)
			CheckAndDestroyIEView(dat);
		if (iLogMode != WANT_HPP_LOG)
			CheckAndDestroyHPP(dat);
		ShowWindow(GetDlgItem(hwndDlg, IDC_LOG), SW_SHOW);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), TRUE);
		dat->hwndIEView = 0;
		dat->hwndIWebBrowserControl = 0;
		dat->hwndHPP = 0;
	}
}

void SwitchMessageLog(HWND hwndDlg, struct MessageWindowData *dat, int iMode)
{
	if (iMode) {           // switch from rtf to IEview or hpp
		SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), FALSE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_LOG), SW_HIDE);
		SetMessageLog(hwndDlg, dat);
	} else                    // switch from IEView or hpp to rtf
		SetMessageLog(hwndDlg, dat);

	SetDialogToType(hwndDlg);
	SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
	SendMessage(hwndDlg, WM_SIZE, 0, 0);
	UpdateContainerMenu(hwndDlg, dat);

	//MAD: simple subclassing after log changed
	if (dat->hwndIEView) {
		if (dat->oldIEViewLastChildProc == 0) {
			WNDPROC wndProc = (WNDPROC)SetWindowLong(GetLastChild(dat->hwndIEView), GWL_WNDPROC, (LONG)IEViewKFSubclassProc);
			dat->oldIEViewLastChildProc = wndProc;
		}
 		if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "subclassIEView", 0)&&dat->oldIEViewProc == 0) {
 			WNDPROC wndProc = (WNDPROC)SetWindowLong(dat->hwndIEView, GWL_WNDPROC, (LONG)IEViewSubclassProc);
 			dat->oldIEViewProc = wndProc;
 		}
	} else if (dat->hwndHPP) {
		if (dat->oldIEViewProc == 0) {
			WNDPROC wndProc = (WNDPROC)SetWindowLong(dat->hwndHPP, GWL_WNDPROC, (LONG)HPPKFSubclassProc);
			dat->oldIEViewProc = wndProc;
		}
	}
//MAD_

}

void FindFirstEvent(HWND hwndDlg, struct MessageWindowData *dat)
{
	int historyMode = DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_LOADHISTORY, -1);
	if (historyMode == -1)
		historyMode = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, SRMSGDEFSET_LOADHISTORY);

	dat->hDbEventFirst = (HANDLE) CallService(MS_DB_EVENT_FINDFIRSTUNREAD, (WPARAM) dat->hContact, 0);
	switch (historyMode) {
		case LOADHISTORY_COUNT: {
			int i;
			HANDLE hPrevEvent;
			DBEVENTINFO dbei = { 0};
			dbei.cbSize = sizeof(dbei);
			//MAD: ability to load only current session's history
			if (dat->bActualHistory)
				i=dat->messageCount;
			else i = DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADCOUNT, SRMSGDEFSET_LOADCOUNT);
			//
			for (; i > 0; i--) {
				if (dat->hDbEventFirst == NULL)
					hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->hContact, 0);
				else
					hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) dat->hDbEventFirst, 0);
				if (hPrevEvent == NULL)
					break;
				dbei.cbBlob = 0;
				dat->hDbEventFirst = hPrevEvent;
				CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
				if (!DbEventIsShown(dat, &dbei))
					i++;
			}
			break;
		}
		case LOADHISTORY_TIME: {
			HANDLE hPrevEvent;
			DBEVENTINFO dbei = { 0};
			DWORD firstTime;

			dbei.cbSize = sizeof(dbei);
			if (dat->hDbEventFirst == NULL)
				dbei.timestamp = time(NULL);
			else
				CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
			firstTime = dbei.timestamp - 60 * DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADTIME, SRMSGDEFSET_LOADTIME);
			for (;;) {
				if (dat->hDbEventFirst == NULL)
					hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->hContact, 0);
				else
					hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) dat->hDbEventFirst, 0);
				if (hPrevEvent == NULL)
					break;
				dbei.cbBlob = 0;
				CallService(MS_DB_EVENT_GET, (WPARAM) hPrevEvent, (LPARAM) & dbei);
				if (dbei.timestamp < firstTime)
					break;
				dat->hDbEventFirst = hPrevEvent;
			}
			break;
		}
		default: {
			break;
		}
	}
}

void SaveSplitter(HWND hwndDlg, struct MessageWindowData *dat)
{
	if (dat->panelHeight < 110 && dat->panelHeight > 50) {          // only save valid panel splitter positions
		if (dat->dwFlagsEx & MWF_SHOW_SPLITTEROVERRIDE)
			DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "panelheight", dat->panelHeight);
		else {
			myGlobals.m_panelHeight = dat->panelHeight;
			DBWriteContactSettingDword(NULL, SRMSGMOD_T, "panelheight", dat->panelHeight);
		}
	}

	if (dat->splitterY < DPISCALEY_S(MINSPLITTERY) || dat->splitterY < 0)
		dat->splitterY = DPISCALEY_S(MINSPLITTERY);

	if (dat->dwFlagsEx & MWF_SHOW_SPLITTEROVERRIDE)
		DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "splitsplity", dat->splitterY);
	else
		DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitsplity", dat->splitterY);
}

void LoadSplitter(HWND hwndDlg, struct MessageWindowData *dat)
{
	if (!(dat->dwFlagsEx & MWF_SHOW_SPLITTEROVERRIDE))
		dat->splitterY = (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "splitsplity", (DWORD) 60);
	else
		dat->splitterY = (int)DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "splitsplity", DBGetContactSettingDword(NULL, SRMSGMOD_T, "splitsplity", (DWORD) 60));

	if (dat->splitterY < DPISCALEY_S(MINSPLITTERY) || dat->splitterY < 0)
		dat->splitterY = 150;
}

void LoadPanelHeight(HWND hwndDlg, struct MessageWindowData *dat)
{
	if (!(dat->dwFlagsEx & MWF_SHOW_SPLITTEROVERRIDE))
		dat->panelHeight = myGlobals.m_panelHeight;
	else
		dat->panelHeight = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "panelheight", myGlobals.m_panelHeight);

	if (dat->panelHeight <= 0 || dat->panelHeight > 120)
		dat->panelHeight = 52;
}

void PlayIncomingSound(struct ContainerWindowData *pContainer, HWND hwnd)
{
	int iPlay = 0;
	DWORD dwFlags = pContainer->dwFlags;

	if (nen_options.iNoSounds)
		return;

	if (nen_options.bSimpleMode == 0) {
		if (dwFlags & CNT_NOSOUND)
			iPlay = FALSE;
		else if (dwFlags & CNT_SYNCSOUNDS) {
			iPlay = !MessageWindowOpened(0, (LPARAM)hwnd);
		} else
			iPlay = TRUE;
	} else
		iPlay = dwFlags & CNT_NOSOUND ? FALSE : TRUE;

	if (iPlay) {
		if (GetForegroundWindow() == pContainer->hwnd && pContainer->hwndActive == hwnd)
			SkinPlaySound("RecvMsgActive");
		else
			SkinPlaySound("RecvMsgInactive");
	}
}

/*
 * reads send format and configures the toolbar buttons
 * if mode == 0, int only configures the buttons and does not change send format
 */

void GetSendFormat(HWND hwndDlg, struct MessageWindowData *dat, int mode)
{
	UINT controls[5] = {IDC_FONTBOLD, IDC_FONTITALIC, IDC_FONTUNDERLINE,IDC_FONTSTRIKEOUT, IDC_FONTFACE};
	int i;

	if (mode) {
		dat->SendFormat = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "sendformat", myGlobals.m_SendFormat);
		if (dat->SendFormat == -1)          // per contact override to disable it..
			dat->SendFormat = 0;
		else if (dat->SendFormat == 0)
			dat->SendFormat = myGlobals.m_SendFormat ? 1 : 0;
	}
	for (i = 0; i < 5; i++) {
		EnableWindow(GetDlgItem(hwndDlg, controls[i]), dat->SendFormat != 0 ? TRUE : FALSE);
		ShowWindow(GetDlgItem(hwndDlg, controls[i]), (dat->SendFormat != 0 /*&& !(dat->pContainer->dwFlags & CNT_HIDETOOLBAR)*/ )? SW_SHOW : SW_HIDE);
	}
	return;
}

/*
 * get 2-digit locale id from current keyboard layout
 */

void GetLocaleID(struct MessageWindowData *dat, char *szKLName)
{
	char szLI[20], *stopped = NULL;
	USHORT langID;
	WORD   wCtype2[3];
	PARAFORMAT2 pf2 = {0};
	BOOL fLocaleNotSet;
	char szTest[4] = {(char)0xe4, (char)0xf6, (char)0xfc, 0 };

	langID = (USHORT)strtol(szKLName, &stopped, 16);
	dat->lcid = MAKELCID(langID, 0);
	GetLocaleInfoA(dat->lcid, LOCALE_SISO639LANGNAME , szLI, 10);
	fLocaleNotSet = (dat->lcID[0] == 0 && dat->lcID[1] == 0);
	dat->lcID[0] = toupper(szLI[0]);
	dat->lcID[1] = toupper(szLI[1]);
	dat->lcID[2] = 0;
	GetStringTypeA(dat->lcid, CT_CTYPE2, szTest, 3, wCtype2);
	pf2.cbSize = sizeof(pf2);
	pf2.dwMask = PFM_RTLPARA;
	SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_GETPARAFORMAT, 0, (LPARAM)&pf2);
	if (FindRTLLocale(dat) && fLocaleNotSet) {
		if (wCtype2[0] == C2_RIGHTTOLEFT || wCtype2[1] == C2_RIGHTTOLEFT || wCtype2[2] == C2_RIGHTTOLEFT) {
			ZeroMemory(&pf2, sizeof(pf2));
			pf2.dwMask = PFM_RTLPARA;
			pf2.cbSize = sizeof(pf2);
			pf2.wEffects = PFE_RTLPARA;
			SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
		} else {
			ZeroMemory(&pf2, sizeof(pf2));
			pf2.dwMask = PFM_RTLPARA;
			pf2.cbSize = sizeof(pf2);
			pf2.wEffects = 0;
			SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
		}
		SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOKEYBOARD);
	}
}

/*
 * Returns true if the unicode buffer only contains 7-bit characters.
 * The resulting message could be sent and stored as ANSI then
 */

/*
BOOL IsUnicodeAscii(const wchar_t* pBuffer, int nSize)
{
	BOOL bResult = TRUE;
	int nIndex;

	for (nIndex = 0; nIndex < nSize; nIndex++) {
		if (pBuffer[nIndex] > 0x7F) {
			bResult = FALSE;
			break;
		}
	}
	return bResult;
}
*/
BYTE GetInfoPanelSetting(HWND hwndDlg, struct MessageWindowData *dat)
{
	BYTE bDefault = dat->pContainer->dwFlags & CNT_INFOPANEL ? 1 : 0;
	BYTE bContact = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "infopanel", 0);
	if (dat->hContact == 0)     // no info panel, if no hcontact
		return 0;
	return bContact == 0 ? bDefault : (bContact == (BYTE) - 1 ? 0 : 1);
}

int FoldersPathChanged(WPARAM wParam,LPARAM lParam)
{
	if (ServiceExists(MS_FOLDERS_REGISTER_PATH)) {
		char szTemp[MAX_PATH]={'\0'};
		FoldersGetCustomPath(myGlobals.m_hDataPath, szTemp, MAX_PATH, PROFILE_PATH "\\tabSRMM\\");
		mir_snprintf(myGlobals.szDataPath,MAX_PATH,"%s",szTemp);
		FoldersGetCustomPath(myGlobals.m_hSkinsPath, szTemp, MAX_PATH, PROFILE_PATH "\\tabSRMM\\Skins\\");
		mir_snprintf(myGlobals.szSkinsPath,MAX_PATH,"%s",szTemp);
		FoldersGetCustomPath(myGlobals.m_hAvatarsPath,szTemp, MAX_PATH,PROFILE_PATH "\\tabSRMM\\Saved Contact Pictures\\");
		mir_snprintf(myGlobals.szAvatarsPath,MAX_PATH,"%s",szTemp);
	} else {
		char pszDBPath[MAX_PATH + 1];
		CallService(MS_DB_GETPROFILEPATH, MAX_PATH, (LPARAM)pszDBPath);
		pszDBPath[MAX_PATH] = 0;

		mir_snprintf(myGlobals.szDataPath, MAX_PATH, "%s\\tabSRMM\\", pszDBPath);
		myGlobals.szDataPath[MAX_PATH] = 0;

		mir_snprintf(myGlobals.szSkinsPath, MAX_PATH, "%sskins\\", myGlobals.szDataPath);
		mir_snprintf(myGlobals.szAvatarsPath, MAX_PATH, "%sSaved Contact Pictures\\", myGlobals.szDataPath);
	}

	CreateDirectoryA(myGlobals.szDataPath, NULL);
	CreateDirectoryA(myGlobals.szSkinsPath, NULL);
	CreateDirectoryA(myGlobals.szAvatarsPath, NULL);
	return 0;
}

void GetDataDir()
{
	if (ServiceExists(MS_FOLDERS_REGISTER_PATH)) {
		myGlobals.m_hDataPath=(HANDLE)FoldersRegisterCustomPath("TabSRMM", "TabSRMM data", PROFILE_PATH "\\tabSRMM\\");
		myGlobals.m_hSkinsPath=(HANDLE)FoldersRegisterCustomPath("TabSRMM", "TabSRMM Skins", PROFILE_PATH "\\tabSRMM\\Skins\\");
		myGlobals.m_hAvatarsPath=(HANDLE)FoldersRegisterCustomPath("TabSRMM", "TabSRMM Saved Avatars", PROFILE_PATH "\\tabSRMM\\Saved Contact Pictures\\");
		HookEvent(ME_FOLDERS_PATH_CHANGED,FoldersPathChanged);
	}
	FoldersPathChanged(0,0);
}

void LoadContactAvatar(HWND hwndDlg, struct MessageWindowData *dat)
{
	if (ServiceExists(MS_AV_GETAVATARBITMAP) && dat)
		dat->ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)dat->hContact, 0);
	else if (dat)
		dat->ace = NULL;

	if (dat && !(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)) {
		BITMAP bm;
		dat->iRealAvatarHeight = 0;

		if (dat->ace && dat->ace->hbmPic) {
			AdjustBottomAvatarDisplay(hwndDlg, dat);
			GetObject(dat->ace->hbmPic, sizeof(bm), &bm);
			CalcDynamicAvatarSize(hwndDlg, dat, &bm);
			PostMessage(hwndDlg, WM_SIZE, 0, 0);
		} else if (dat->ace == NULL) {
			AdjustBottomAvatarDisplay(hwndDlg, dat);
			GetObject(myGlobals.g_hbmUnknown, sizeof(bm), &bm);
			CalcDynamicAvatarSize(hwndDlg, dat, &bm);
			PostMessage(hwndDlg, WM_SIZE, 0, 0);
		}
	}
}

void LoadOwnAvatar(HWND hwndDlg, struct MessageWindowData *dat)
{
	AVATARCACHEENTRY *ace = NULL;

	if (ServiceExists(MS_AV_GETMYAVATAR))
		ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETMYAVATAR, 0, (LPARAM)(dat->bIsMeta ? dat->szMetaProto : dat->szProto));

	if (ace) {
		dat->hOwnPic = ace->hbmPic;
		dat->ownAce = ace;
	} else {
		dat->hOwnPic = myGlobals.g_hbmUnknown;
		dat->ownAce = NULL;
	}

	if (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
		BITMAP bm;

		dat->iRealAvatarHeight = 0;
		AdjustBottomAvatarDisplay(hwndDlg, dat);
		GetObject(dat->hOwnPic, sizeof(bm), &bm);
		CalcDynamicAvatarSize(hwndDlg, dat, &bm);
		SendMessage(hwndDlg, WM_SIZE, 0, 0);
	}
}

void UpdateApparentModeDisplay(HWND hwndDlg, struct MessageWindowData *dat)
{
	if (dat->wApparentMode == ID_STATUS_OFFLINE) {
		CheckDlgButton(hwndDlg, IDC_APPARENTMODE, BST_CHECKED);
		SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadSkinnedIcon(SKINICON_STATUS_OFFLINE));
	} else if (dat->wApparentMode == ID_STATUS_ONLINE || dat->wApparentMode == 0) {
		CheckDlgButton(hwndDlg, IDC_APPARENTMODE, BST_UNCHECKED);
		SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BM_SETIMAGE, IMAGE_ICON, (LPARAM)(dat->wApparentMode == ID_STATUS_ONLINE ? LoadSkinnedIcon(SKINICON_STATUS_INVISIBLE) : LoadSkinnedIcon(SKINICON_STATUS_ONLINE)));
		if (!dat->pContainer->bSkinned)
			SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BUTTONSETASFLATBTN, 0, (LPARAM)(dat->wApparentMode == ID_STATUS_ONLINE ? 1 : 0));
	}
}


int MY_DBFreeVariant(DBVARIANT *dbv)
{
	return DBFreeVariant(dbv);
}

int MY_DBGetContactSettingTString(HANDLE hContact, char *szModule, char *szSetting, DBVARIANT *dbv)
{
	return(DBGetContactSettingTString(hContact, szModule, szSetting, dbv));
}

// free() the return value

TCHAR *MY_DBGetContactSettingString(HANDLE hContact, char *szModule, char *szSetting)
{
	DBVARIANT dbv;
	TCHAR *szResult = NULL;
	if (!DBGetContactSetting(hContact, szModule, szSetting, &dbv)) {
		if (dbv.type == DBVT_ASCIIZ && lstrlenA(dbv.pszVal) > 0) {
#if defined(_UNICODE)
			szResult = Utf8_Decode(dbv.pszVal);
#else
			szResult = malloc(lstrlenA(dbv.pszVal) + 1);
			strcpy(szResult, dbv.pszVal);
#endif
		}
		DBFreeVariant(&dbv);
		return szResult;
	}
	return NULL;
}

void LoadTimeZone(HWND hwndDlg, struct MessageWindowData *dat)
{
	dat->timezone = (DWORD)(DBGetContactSettingByte(dat->hContact, "UserInfo", "Timezone", DBGetContactSettingByte(dat->hContact, dat->szProto, "Timezone", -1)));
	if (dat->timezone != -1) {
		DWORD contact_gmt_diff;
		contact_gmt_diff = dat->timezone > 128 ? 256 - dat->timezone : 0 - dat->timezone;
		dat->timediff = (int)myGlobals.local_gmt_diff - (int)contact_gmt_diff * 60 * 60 / 2;
	}
}

/*
 * paste contents of the clipboard into the message input area and send it immediately
 */

void HandlePasteAndSend(HWND hwndDlg, struct MessageWindowData *dat)
{
	if (!myGlobals.m_PasteAndSend) {
		SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, IDC_MESSAGE, (LPARAM)TranslateT("The 'paste and send' feature is disabled. You can enable it on the 'General' options page in the 'Sending Messages' section"));
		return;                                     // feature disabled
	}

	SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_PASTESPECIAL, CF_TEXT, 0);
	if (GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE)) > 0)
		SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
}

/*
 * draw various elements of the message window, like avatar(s), info panel fields
 * and the color formatting menu
 */

int MsgWindowDrawHandler(WPARAM wParam, LPARAM lParam, HWND hwndDlg, struct MessageWindowData *dat)
{
	LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;

	if (!dat)
		return 0;


	if (dis->CtlType == ODT_MENU && dis->hwndItem == (HWND)GetSubMenu(myGlobals.g_hMenuContext, 7)) {
		RECT rc = { 0 };
		HBRUSH old, col;
		COLORREF clr;
		switch (dis->itemID) {
			case ID_FONT_RED:
				clr = RGB(255, 0, 0);
				break;
			case ID_FONT_BLUE:
				clr = RGB(0, 0, 255);
				break;
			case ID_FONT_GREEN:
				clr = RGB(0, 255, 0);
				break;
			case ID_FONT_MAGENTA:
				clr = RGB(255, 0, 255);
				break;
			case ID_FONT_YELLOW:
				clr = RGB(255, 255, 0);
				break;
			case ID_FONT_WHITE:
				clr = RGB(255, 255, 255);
				break;
			case ID_FONT_DEFAULTCOLOR:
				clr = GetSysColor(COLOR_MENU);
				break;
			case ID_FONT_CYAN:
				clr = RGB(0, 255, 255);
				break;
			case ID_FONT_BLACK:
				clr = RGB(0, 0, 0);
				break;
			default:
				clr = 0;
		}
		col = CreateSolidBrush(clr);
		old = SelectObject(dis->hDC, col);
		rc.left = 2;
		rc.top = dis->rcItem.top - 5;
		rc.right = 15;
		rc.bottom = dis->rcItem.bottom + 4;
		Rectangle(dis->hDC, rc.left - 1, rc.top - 1, rc.right + 1, rc.bottom + 1);
		FillRect(dis->hDC, &rc, col);
		SelectObject(dis->hDC, old);
		DeleteObject(col);
		return TRUE;
	}

	else if ((dis->hwndItem == GetDlgItem(hwndDlg, IDC_CONTACTPIC) && (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown) && dat->showPic) || (dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELPIC) && dat->dwFlagsEx & MWF_SHOW_INFOPANEL && (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown))) {
		HBRUSH hOldBrush;
		BITMAP bminfo;
		double dAspect = 0, dNewWidth = 0, dNewHeight = 0;
		DWORD iMaxHeight = 0, top, cx, cy;
		RECT rc, rcClient, rcFrame;
		HDC hdcDraw;
		HBITMAP hbmDraw, hbmOld;
		BOOL bPanelPic = dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELPIC);
		DWORD aceFlags = 0;
		HBRUSH bgBrush = 0;
		BYTE borderType = myGlobals.bAvatarBoderType;
		HPEN hPenBorder = 0, hPenOld = 0;
		HRGN clipRgn = 0;
		int  iRad = myGlobals.m_WinVerMajor >= 5 ? 4 : 6;
		BOOL flashAvatar = FALSE;

		if (myGlobals.g_FlashAvatarAvail && (!bPanelPic || (bPanelPic && dat->showInfoPic == 1))) {
			FLASHAVATAR fa = {0};

			fa.id = 25367;
			fa.cProto = dat->szProto;
			if (!bPanelPic && (dat->dwFlagsEx & MWF_SHOW_INFOPANEL)) {
				fa.hParentWindow = GetDlgItem(hwndDlg, IDC_CONTACTPIC);
				CallService(MS_FAVATAR_MAKE, (WPARAM)&fa, 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), fa.hWindow != 0);
			} else {
				fa.hContact = dat->hContact;
				fa.hParentWindow = GetDlgItem(hwndDlg, (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) ? IDC_PANELPIC : IDC_CONTACTPIC);
				CallService(MS_FAVATAR_MAKE, (WPARAM)&fa, 0);
				if (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
					if (fa.hWindow != NULL&&dat->hwndPanelPic) {
						DestroyWindow(dat->hwndPanelPic);
						dat->hwndPanelPic = NULL;
					}
					if (!myGlobals.g_bDisableAniAvatars&&fa.hWindow == NULL&&!dat->hwndPanelPic) {
						dat->hwndPanelPic =CreateWindowEx(WS_EX_TOPMOST, AVATAR_CONTROL_CLASS, _T(""), WS_VISIBLE | WS_CHILD, 1, 1, 1, 1, GetDlgItem(hwndDlg, IDC_PANELPIC), (HMENU)0, NULL, NULL);
						SendMessage(dat->hwndPanelPic, AVATAR_SETCONTACT, (WPARAM)0, (LPARAM)dat->hContact);
					}
				} else {
					if (fa.hWindow != NULL&&dat->hwndContactPic) {
						DestroyWindow(dat->hwndContactPic);
						dat->hwndContactPic = NULL;
					}
					if (!myGlobals.g_bDisableAniAvatars&&fa.hWindow == NULL&&!dat->hwndContactPic) {
						dat->hwndContactPic =CreateWindowEx(WS_EX_TOPMOST, AVATAR_CONTROL_CLASS, _T(""), WS_VISIBLE | WS_CHILD, 1, 1, 1, 1, GetDlgItem(hwndDlg, IDC_CONTACTPIC), (HMENU)0, NULL, NULL);
						SendMessage(dat->hwndContactPic, AVATAR_SETCONTACT, (WPARAM)0, (LPARAM)dat->hContact);
					}
				}
				EnableWindow(GetDlgItem(hwndDlg, (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) ? IDC_PANELPIC : IDC_CONTACTPIC), fa.hWindow != 0);
				dat->hwndFlash = fa.hWindow;
			}
			if (fa.hWindow != 0) {
				bminfo.bmHeight = FAVATAR_HEIGHT;
				bminfo.bmWidth = FAVATAR_WIDTH;
				CallService(MS_FAVATAR_SETBKCOLOR, (WPARAM)&fa, (LPARAM)dat->avatarbg);
				flashAvatar = TRUE;
			}
		}

		if (bPanelPic) {
			if (!flashAvatar) {
				GetObject(dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown, sizeof(bminfo), &bminfo);
			}
			if ((dat->ace && dat->showInfoPic && !(dat->ace->dwFlags & AVS_HIDEONCLIST)) || dat->showInfoPic)
				aceFlags = dat->ace ? dat->ace->dwFlags : 0;
			else {
				if (dat->panelWidth) {
					dat->panelWidth = 0;
					SendMessage(hwndDlg, WM_SIZE, 0, 0);
				}
				return TRUE;
			}
		} else  {
			if (!(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)) {
				if (dat->ace)
					aceFlags = dat->ace->dwFlags;
			} else if (dat->ownAce)
				aceFlags = dat->ownAce->dwFlags;

			GetObject(dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown), sizeof(bminfo), &bminfo);
		}

		GetClientRect(hwndDlg, &rc);
		GetClientRect(dis->hwndItem, &rcClient);
		cx = rcClient.right;
		cy = rcClient.bottom;

		if (cx < 5 || cy < 5)
			return TRUE;

		if (bPanelPic) {
			if (bminfo.bmHeight > bminfo.bmWidth) {
				if (bminfo.bmHeight > 0)
					dAspect = (double)(cy - 2) / (double)bminfo.bmHeight;
				else
					dAspect = 1.0;
				dNewWidth = (double)bminfo.bmWidth * dAspect;
				dNewHeight = cy - 2;
			} else {
				if (bminfo.bmWidth > 0)
					dAspect = (double)(cy - 2) / (double)bminfo.bmWidth;
				else
					dAspect = 1.0;
				dNewHeight = (double)bminfo.bmHeight * dAspect;
				dNewWidth = cy - 2;
			}
			if (dat->panelWidth == -1) {
				dat->panelWidth = (int)dNewWidth + 2;
				SendMessage(hwndDlg, WM_SIZE, 0, 0);
			}
		} else {

			if (bminfo.bmHeight > 0)
				dAspect = (double)dat->iRealAvatarHeight / (double)bminfo.bmHeight;
			else
				dAspect = 1.0;
			dNewWidth = (double)bminfo.bmWidth * dAspect;
			if (dNewWidth > (double)(rc.right) * 0.8)
				dNewWidth = (double)(rc.right) * 0.8;
			iMaxHeight = dat->iRealAvatarHeight;
		}

		if (flashAvatar)
			return TRUE;

		hPenBorder = CreatePen(PS_SOLID, 1, (COLORREF)DBGetContactSettingDword(NULL, SRMSGMOD_T, "avborderclr", RGB(0, 0, 0)));

		hdcDraw = CreateCompatibleDC(dis->hDC);
		hbmDraw = CreateCompatibleBitmap(dis->hDC, cx, cy);
		hbmOld = SelectObject(hdcDraw, hbmDraw);

		hPenOld = SelectObject(hdcDraw, hPenBorder);

		bgBrush = CreateSolidBrush(dat->avatarbg);
		hOldBrush = SelectObject(hdcDraw, bgBrush);
		rcFrame = rcClient;


//   		if (bPanelPic)
//   			rcFrame.left = rcClient.right - dat->panelWidth;
		if (dat->pContainer->bSkinned)
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &rcFrame, hdcDraw);
		else
			FillRect(hdcDraw, &rcFrame, bgBrush);
		if (borderType == 1)
			borderType = aceFlags & AVS_PREMULTIPLIED ? 2 : 3;

		if (!bPanelPic) {
			top = (cy - dat->pic.cy) / 2;
			if (borderType) {
				RECT rcEdge = {0, top, dat->pic.cx, top + dat->pic.cy};
				if (borderType == 2)
					DrawEdge(hdcDraw, &rcEdge, BDR_SUNKENINNER, BF_RECT);
				else if (borderType == 3)
					Rectangle(hdcDraw, rcEdge.left, rcEdge.top, rcEdge.right, rcEdge.bottom);
				else if (borderType == 4) {
					clipRgn = CreateRoundRectRgn(rcEdge.left, rcEdge.top, rcEdge.right + 1, rcEdge.bottom + 1, iRad, iRad);
					SelectClipRgn(hdcDraw, clipRgn);
				}
			}
		}
		if (((dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown)) && dat->showPic) || bPanelPic) {
			HDC hdcMem = CreateCompatibleDC(dis->hDC);
			HBITMAP hbmAvatar = bPanelPic ? (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown) : (dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown));
			HBITMAP hbmMem = (HBITMAP)SelectObject(hdcMem, hbmAvatar);
			if (bPanelPic) {
				LONG width_off = borderType ? 0 : 2;
				LONG height_off = 0;

				rcFrame = rcClient;
				rcFrame.left = rcFrame.right - ((LONG)dNewWidth + 2);
				rcFrame.bottom = rcFrame.top + (LONG)dNewHeight + 2;
				if (rcFrame.bottom < rcClient.bottom) {
					height_off = (rcClient.bottom - ((LONG)dNewHeight + 2)) / 2;
					rcFrame.top += height_off;
					rcFrame.bottom += height_off;
				}
				SetStretchBltMode(hdcDraw, HALFTONE);
				if (borderType == 2)
					DrawEdge(hdcDraw, &rcFrame, BDR_SUNKENINNER, BF_RECT);
				else if (borderType == 3)
					Rectangle(hdcDraw, rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom);
				else if (borderType == 4) {
					clipRgn = CreateRoundRectRgn(rcFrame.left, rcFrame.top, rcFrame.right + 1, rcFrame.bottom + 1, iRad, iRad);
					SelectClipRgn(hdcDraw, clipRgn);
				}
				//mad
//   				if(!dat->hwndPanelPic)
//    				{
				if (aceFlags & AVS_PREMULTIPLIED)
					MY_AlphaBlend(hdcDraw, rcFrame.left + (borderType ? 1 : 0), height_off + (borderType ? 1 : 0), (int)dNewWidth + width_off, (int)dNewHeight + width_off, bminfo.bmWidth, bminfo.bmHeight, hdcMem);
				else
					StretchBlt(hdcDraw, rcFrame.left + (borderType ? 1 : 0), height_off + (borderType ? 1 : 0), (int)dNewWidth + width_off, (int)dNewHeight + width_off, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
// 				}

			} else {
				LONG xy_off = borderType ? 1 : 0;
				LONG width_off = borderType ? 0 : 2;

				SetStretchBltMode(hdcDraw, HALFTONE);
// 				if(!dat->hwndContactPic)
//  				{
				if (aceFlags & AVS_PREMULTIPLIED)
					MY_AlphaBlend(hdcDraw, xy_off, top + xy_off, (int)dNewWidth + width_off, iMaxHeight + width_off, bminfo.bmWidth, bminfo.bmHeight, hdcMem);
				else
					StretchBlt(hdcDraw, xy_off, top + xy_off, (int)dNewWidth + width_off, iMaxHeight + width_off, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
// 				}
			}
			if (clipRgn) {
				HBRUSH hbr = CreateSolidBrush((COLORREF)DBGetContactSettingDword(NULL, SRMSGMOD_T, "avborderclr", RGB(0, 0, 0)));
				FrameRgn(hdcDraw, clipRgn, hbr, 1, 1);
				DeleteObject(hbr);
				DeleteObject(clipRgn);
			}
			//mad
			SelectObject(hdcMem, hbmMem);
			//
			DeleteObject(hbmMem);
			DeleteDC(hdcMem);
		}
		SelectObject(hdcDraw, hPenOld);
		SelectObject(hdcDraw, hOldBrush);
		DeleteObject(bgBrush);
		DeleteObject(hPenBorder);
		BitBlt(dis->hDC, 0, 0, cx, cy, hdcDraw, 0, 0, SRCCOPY);
		SelectObject(hdcDraw, hbmOld);
		DeleteObject(hbmDraw);
		DeleteDC(hdcDraw);
		return TRUE;
	}

	else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELSTATUS) && dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
		char	*szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
		SIZE	sProto = {0}, sStatus = {0};
		DWORD	oldPanelStatusCX = dat->panelStatusCX;
		RECT	rc;
		HFONT	hOldFont = 0;
		BOOL	config = myGlobals.ipConfig.isValid;
		StatusItems_t *item = &StatusItems[ID_EXTBKINFOPANEL];
		TCHAR   *szFinalProto = a2tf((TCHAR *)szProto, 0, 0);

		if (config)
			hOldFont = SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_STATUS]);

		if (dat->szStatus[0])
			GetTextExtentPoint32(dis->hDC, dat->szStatus, lstrlen(dat->szStatus), &sStatus);
		if (szFinalProto) {
			if (config)
				SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_PROTO]);
			GetTextExtentPoint32(dis->hDC, szFinalProto, lstrlen(szFinalProto), &sProto);
		}

		dat->panelStatusCX = sStatus.cx + sProto.cx + 14 + (dat->hClientIcon ? 20 : 0);

		if (dat->panelStatusCX != oldPanelStatusCX)
			SendMessage(hwndDlg, WM_SIZE, 0, 0);

		GetClientRect(dis->hwndItem, &rc);
		if (dat->pContainer->bSkinned) {
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &rc, dis->hDC);
			rc.left += item->MARGIN_LEFT;
			rc.right -= item->MARGIN_RIGHT;
			rc.top += item->MARGIN_TOP;
			rc.bottom -= item->MARGIN_BOTTOM;
			if (!item->IGNORED)
				DrawAlpha(dis->hDC, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
						  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
		} else
			FillRect(dis->hDC, &rc, myGlobals.ipConfig.bkgBrush);
		SetBkMode(dis->hDC, TRANSPARENT);
		if (myGlobals.ipConfig.borderStyle < IPFIELD_FLAT && (!dat->pContainer->bSkinned || item->IGNORED))
			DrawEdge(dis->hDC, &rc, myGlobals.ipConfig.edgeType, myGlobals.ipConfig.edgeFlags);

		rc.left += 3;
		if (dat->szStatus[0]) {
			if (config) {
				SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_STATUS]);
				SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_STATUS]);
			}
			DrawText(dis->hDC, dat->szStatus, lstrlen(dat->szStatus), &rc, DT_SINGLELINE | DT_VCENTER);
		}
		if (szFinalProto) {
			rc.left = rc.right - sProto.cx - 3 - (dat->hClientIcon ? 20 : 0);
			if (config) {
				SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_PROTO]);
				SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_PROTO]);
			} else
				SetTextColor(dis->hDC, GetSysColor(COLOR_HOTLIGHT));
			DrawText(dis->hDC, szFinalProto, lstrlen(szFinalProto), &rc, DT_SINGLELINE | DT_VCENTER);
		}

		if (dat->hClientIcon)
			DrawIconEx(dis->hDC, rc.right - 19, (rc.bottom + rc.top - 16) / 2, dat->hClientIcon, 16, 16, 0, 0, DI_NORMAL);

		if (config && hOldFont)
			SelectObject(dis->hDC, hOldFont);

		if (szFinalProto)
			mir_free(szFinalProto);

		return TRUE;
	} else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELNICK) && dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
		RECT rc = dis->rcItem;
		TCHAR *szStatusMsg = NULL;
		TCHAR *szLabel = TranslateT("Name:");
		int   iNameLen = lstrlen(szLabel);
		TCHAR *szLabelUIN = TranslateT("User Id:");
		int   iNameLenUIN = lstrlen(szLabelUIN);
		SIZE  szUIN;
		StatusItems_t *item = &StatusItems[ID_EXTBKINFOPANEL];

		szStatusMsg = dat->statusMsg;

		GetTextExtentPoint32(dis->hDC, szLabel, iNameLen, &dat->szLabel);
		GetTextExtentPoint32(dis->hDC, szLabelUIN, iNameLenUIN, &szUIN);

		if (szUIN.cx > dat->szLabel.cx)
			dat->szLabel = szUIN;

		rc.right = rc.left + dat->szLabel.cx + 3;
		SetBkMode(dis->hDC, TRANSPARENT);
		if (dat->pContainer->bSkinned) {
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &dis->rcItem, dis->hDC);
			SetTextColor(dis->hDC, myGlobals.skinDefaultFontColor);
		} else {
			FillRect(dis->hDC, &rc, GetSysColorBrush(COLOR_3DFACE));
			SetTextColor(dis->hDC, GetSysColor(COLOR_BTNTEXT));
		}
		DrawText(dis->hDC, szLabel, iNameLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
		dis->rcItem.left += (dat->szLabel.cx + 3);
		if (dat->pContainer->bSkinned) {
			RECT rc = dis->rcItem;
			rc.left += item->MARGIN_LEFT;
			rc.right -= item->MARGIN_RIGHT;
			rc.top += item->MARGIN_TOP;
			rc.bottom -= item->MARGIN_BOTTOM;
			if (!item->IGNORED)
				DrawAlpha(dis->hDC, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
						  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
		} else
			FillRect(dis->hDC, &dis->rcItem, myGlobals.ipConfig.bkgBrush);
		if (myGlobals.ipConfig.borderStyle < IPFIELD_FLAT && (!dat->pContainer->bSkinned || item->IGNORED))
			DrawEdge(dis->hDC, &dis->rcItem, myGlobals.ipConfig.edgeType, myGlobals.ipConfig.edgeFlags);
		dis->rcItem.left += 2;
		if (dat->szNickname[0]) {
			HFONT hOldFont = 0;
			HICON xIcon = 0;

			xIcon = GetXStatusIcon(dat);

			if (xIcon) {
				DrawIconEx(dis->hDC, dis->rcItem.left, (dis->rcItem.bottom + dis->rcItem.top - myGlobals.m_smcyicon) / 2, xIcon, myGlobals.m_smcxicon, myGlobals.m_smcyicon, 0, 0, DI_NORMAL | DI_COMPAT);
				DestroyIcon(xIcon);
				dis->rcItem.left += 21;
			}

			if (myGlobals.ipConfig.isValid) {
				hOldFont = SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_NICK]);
				SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_NICK]);
			}
			if (szStatusMsg && szStatusMsg[0]) {
				SIZE szNick, sStatusMsg, sMask;
				DWORD dtFlags, dtFlagsNick;

				GetTextExtentPoint32(dis->hDC, dat->szNickname, lstrlen(dat->szNickname), &szNick);
				GetTextExtentPoint32(dis->hDC, _T("A"), 1, &sMask);
				GetTextExtentPoint32(dis->hDC, szStatusMsg, lstrlen(szStatusMsg), &sStatusMsg);
				dtFlagsNick = DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX;
				if ((szNick.cx + sStatusMsg.cx + 6) < (dis->rcItem.right - dis->rcItem.left) || (dis->rcItem.bottom - dis->rcItem.top) < (2 * sMask.cy))
					dtFlagsNick |= DT_VCENTER;
				DrawText(dis->hDC, dat->szNickname, -1, &dis->rcItem, dtFlagsNick);
				if (myGlobals.ipConfig.isValid) {
					SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_STATUS]);
					SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_STATUS]);
				}
				dis->rcItem.left += (szNick.cx + 10);

				if (!(dtFlagsNick & DT_VCENTER))
					//if(dis->rcItem.bottom - dis->rcItem.top >= 2 * sStatusMsg.cy)
					dtFlags = DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX;
				else
					dtFlags = DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_VCENTER;


				dis->rcItem.right -= 3;
				if (dis->rcItem.left + 30 < dis->rcItem.right) {
					//RECT rc = dis->rcItem;
					//dis->rcItem.left += (rc.right - rc.left);
					DrawText(dis->hDC, szStatusMsg, -1, &dis->rcItem, dtFlags);
				}
			} else
				DrawText(dis->hDC, dat->szNickname, -1, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | DT_NOPREFIX);

			if (hOldFont)
				SelectObject(dis->hDC, hOldFont);
		}
		return TRUE;
	} else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELUIN) && dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
		char szBuf[256];
		BOOL config = myGlobals.ipConfig.isValid;
		HFONT hOldFont = 0;
		RECT rc = dis->rcItem;
		TCHAR *szLabel = TranslateT("User Id:");
		int   iNameLen = lstrlen(szLabel);
		StatusItems_t *item = &StatusItems[ID_EXTBKINFOPANEL];

		if (dat->pContainer->bSkinned) {
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &dis->rcItem, dis->hDC);
			SetTextColor(dis->hDC, myGlobals.skinDefaultFontColor);
		} else {
			FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
			SetTextColor(dis->hDC, GetSysColor(COLOR_BTNTEXT));
		}

		SetBkMode(dis->hDC, TRANSPARENT);
		DrawText(dis->hDC, szLabel, iNameLen, &dis->rcItem, DT_SINGLELINE | DT_VCENTER);

		rc.right = rc.left + dat->szLabel.cx + 3;
		dis->rcItem.left += (dat->szLabel.cx + 3);
		if (dat->pContainer->bSkinned) {
			RECT rc = dis->rcItem;
			rc.left += item->MARGIN_LEFT;
			rc.right -= item->MARGIN_RIGHT;
			rc.top += item->MARGIN_TOP;
			rc.bottom -= item->MARGIN_BOTTOM;
			if (!item->IGNORED)
				DrawAlpha(dis->hDC, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
						  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
		} else
			FillRect(dis->hDC, &dis->rcItem, myGlobals.ipConfig.bkgBrush);
		if (myGlobals.ipConfig.borderStyle < IPFIELD_FLAT && (!dat->pContainer->bSkinned || item->IGNORED))
			DrawEdge(dis->hDC, &dis->rcItem, myGlobals.ipConfig.edgeType, myGlobals.ipConfig.edgeFlags);
		dis->rcItem.left += 2;
		if (config) {
			hOldFont = SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_UIN]);
			SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_UIN]);
		}
		if (dat->uin[0]) {
			SIZE sUIN, sTime;
			if (dat->idle) {
				time_t diff = time(NULL) - dat->idle;
				int i_hrs = diff / 3600;
				int i_mins = (diff - i_hrs * 3600) / 60;
				mir_snprintf(szBuf, sizeof(szBuf), "%s    Idle: %dh,%02dm", dat->uin, i_hrs, i_mins);
				GetTextExtentPoint32A(dis->hDC, szBuf, lstrlenA(szBuf), &sUIN);
				DrawTextA(dis->hDC, szBuf, lstrlenA(szBuf), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
			} else {
				GetTextExtentPoint32A(dis->hDC, dat->uin, lstrlenA(dat->uin), &sUIN);
				DrawTextA(dis->hDC, dat->uin, lstrlenA(dat->uin), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
			}
			if (dat->timezone != -1) {
				DBTIMETOSTRING dbtts;
				char szResult[80];
				time_t final_time;
				time_t now = time(NULL);
				HFONT oldFont = 0;
				int base_hour;
				char symbolic_time[3];

				final_time = now - dat->timediff;
				dbtts.szDest = szResult;
				dbtts.cbDest = 70;
				dbtts.szFormat = "t";
				CallService(MS_DB_TIME_TIMESTAMPTOSTRING, final_time, (LPARAM) & dbtts);
				if (config) {
					SelectObject(dis->hDC, myGlobals.ipConfig.hFonts[IPFONTID_TIME]);
					SetTextColor(dis->hDC, myGlobals.ipConfig.clrs[IPFONTID_TIME]);
				}
				GetTextExtentPoint32A(dis->hDC, szResult, lstrlenA(szResult), &sTime);
				if (sUIN.cx + sTime.cx + 23 < dis->rcItem.right - dis->rcItem.left) {
					dis->rcItem.left = dis->rcItem.right - sTime.cx - 3 - 16;
					oldFont = SelectObject(dis->hDC, myGlobals.m_hFontWebdings);
					base_hour = atoi(szResult);
					base_hour = base_hour > 11 ? base_hour - 12 : base_hour;
					symbolic_time[0] = (char)(0xB7 + base_hour);
					symbolic_time[1] = 0;
					DrawTextA(dis->hDC, symbolic_time, 1, &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
					SelectObject(dis->hDC, oldFont);
					dis->rcItem.left += 16;
					DrawTextA(dis->hDC, szResult, lstrlenA(szResult), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
				}
			}
		}
		if (hOldFont)
			SelectObject(dis->hDC, hOldFont);
		return TRUE;
	} else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_STATICTEXT) || dis->hwndItem == GetDlgItem(hwndDlg, IDC_LOGFROZENTEXT)) {
		TCHAR szWindowText[256];
		if (dat->pContainer->bSkinned) {
			SetTextColor(dis->hDC, myGlobals.skinDefaultFontColor);
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &dis->rcItem, dis->hDC);
		} else {
			SetTextColor(dis->hDC, GetSysColor(COLOR_BTNTEXT));
			FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
		}
		GetWindowText(dis->hwndItem, szWindowText, 255);
		szWindowText[255] = 0;
		SetBkMode(dis->hDC, TRANSPARENT);
		DrawText(dis->hDC, szWindowText, -1, &dis->rcItem, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP | DT_END_ELLIPSIS);
		return TRUE;
	} else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_STATICERRORICON)) {
		if (dat->pContainer->bSkinned)
			SkinDrawBG(dis->hwndItem, dat->pContainer->hwnd, dat->pContainer, &dis->rcItem, dis->hDC);
		else
			FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
		DrawIconEx(dis->hDC, (dis->rcItem.right - dis->rcItem.left) / 2 - 8, (dis->rcItem.bottom - dis->rcItem.top) / 2 - 8,
				   myGlobals.g_iconErr, 16, 16, 0, 0, DI_NORMAL);
		return TRUE;
	}
	return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
}

void LoadThemeDefaults(HWND hwndDlg, struct MessageWindowData *dat)
{
	int i;
	char szTemp[40];
	COLORREF colour;
	dat->theme.statbg = myGlobals.crStatus;
	dat->theme.oldinbg = myGlobals.crOldIncoming;
	dat->theme.oldoutbg = myGlobals.crOldOutgoing;
	dat->theme.inbg = myGlobals.crIncoming;
	dat->theme.outbg = myGlobals.crOutgoing;
	dat->theme.bg = myGlobals.crDefault;
	dat->theme.hgrid = DBGetContactSettingDword(NULL, FONTMODULE, "hgrid", RGB(224, 224, 224));
	dat->theme.left_indent = DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 20) * 15;
	dat->theme.right_indent = DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 20) * 15;
	dat->theme.inputbg = DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", SRMSGDEFSET_BKGCOLOUR);

	for (i = 1; i <= 5; i++) {
		_snprintf(szTemp, 10, "cc%d", i);
		colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, szTemp, RGB(224, 224, 224));
		if (colour == 0)
			colour = RGB(1, 1, 1);
		dat->theme.custom_colors[i - 1] = colour;
	}
	dat->theme.logFonts = logfonts;
	dat->theme.fontColors = fontcolors;
	dat->theme.rtfFonts = NULL;
}

void LoadOverrideTheme(HWND hwndDlg, struct MessageWindowData *dat)
{
	BOOL bReadTemplates = ((dat->pContainer->ltr_templates == NULL) || (dat->pContainer->rtl_templates == NULL) ||
						   (dat->pContainer->logFonts == NULL) || (dat->pContainer->fontColors == NULL));

	if (lstrlenA(dat->pContainer->szThemeFile) > 1) {
		if (PathFileExistsA(dat->pContainer->szThemeFile)) {
			if (CheckThemeVersion(dat->pContainer->szThemeFile) == 0) {
				LoadThemeDefaults(hwndDlg, dat);
				return;
			}
			if (dat->pContainer->ltr_templates == NULL) {
				dat->pContainer->ltr_templates = (TemplateSet *)malloc(sizeof(TemplateSet));
				CopyMemory(dat->pContainer->ltr_templates, &LTR_Active, sizeof(TemplateSet));
			}
			if (dat->pContainer->rtl_templates == NULL) {
				dat->pContainer->rtl_templates = (TemplateSet *)malloc(sizeof(TemplateSet));
				CopyMemory(dat->pContainer->rtl_templates, &RTL_Active, sizeof(TemplateSet));
			}

			if (dat->pContainer->logFonts == NULL)
				dat->pContainer->logFonts = malloc(sizeof(LOGFONTA) * (MSGDLGFONTCOUNT + 2));
			if (dat->pContainer->fontColors == NULL)
				dat->pContainer->fontColors = malloc(sizeof(COLORREF) * (MSGDLGFONTCOUNT + 2));
			if (dat->pContainer->rtfFonts == NULL)
				dat->pContainer->rtfFonts = malloc((MSGDLGFONTCOUNT + 2) * RTFCACHELINESIZE);

			dat->ltr_templates = dat->pContainer->ltr_templates;
			dat->rtl_templates = dat->pContainer->rtl_templates;
			dat->theme.logFonts = dat->pContainer->logFonts;
			dat->theme.fontColors = dat->pContainer->fontColors;
			dat->theme.rtfFonts = dat->pContainer->rtfFonts;
			ReadThemeFromINI(dat->pContainer->szThemeFile, dat, bReadTemplates ? 0 : 1, THEME_READ_ALL);
			dat->dwFlags = dat->theme.dwFlags;
			dat->theme.left_indent *= 15;
			dat->theme.right_indent *= 15;
			dat->dwFlags |= MWF_SHOW_PRIVATETHEME;
			return;
		}
	}
	LoadThemeDefaults(hwndDlg, dat);
}

void SaveMessageLogFlags(HWND hwndDlg, struct MessageWindowData *dat)
{
	/*
	char szBuf[100];

	if (!(dat->dwFlags & MWF_SHOW_PRIVATETHEME))
		DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
	else {
		if (PathFileExistsA(dat->pContainer->szThemeFile))
			WritePrivateProfileStringA("Message Log", "DWFlags", _itoa(dat->dwFlags & MWF_LOG_ALL, szBuf, 10), dat->pContainer->szThemeFile);
	}
	*/
}

void ConfigureSmileyButton(HWND hwndDlg, struct MessageWindowData *dat)
{
	HICON hButtonIcon = 0;
	int nrSmileys = 0;
  	int showToolbar = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
	int iItemID = IDC_SMILEYBTN;

	if (myGlobals.g_SmileyAddAvail) {

		nrSmileys = CheckValidSmileyPack(dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->bIsMeta ? dat->hSubContact : dat->hContact, &hButtonIcon);

		dat->doSmileys = 1;

		if (hButtonIcon == 0 || myGlobals.m_SmileyButtonOverride) {
			dat->hSmileyIcon = 0;
			SendDlgItemMessage(hwndDlg, iItemID, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[11]);
			if (hButtonIcon)
				DestroyIcon(hButtonIcon);
		} else {
			SendDlgItemMessage(hwndDlg, iItemID, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hButtonIcon);
			dat->hSmileyIcon = hButtonIcon;
		}
	}

	if (nrSmileys == 0 || dat->hContact == 0)
		dat->doSmileys = 0;

	ShowWindow(GetDlgItem(hwndDlg, iItemID), (dat->doSmileys && showToolbar) ? SW_SHOW : SW_HIDE);
	EnableWindow(GetDlgItem(hwndDlg, iItemID), dat->doSmileys ? TRUE : FALSE);
}

HICON GetXStatusIcon(struct MessageWindowData *dat)
{
	char szServiceName[128];
	char *szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;

	mir_snprintf(szServiceName, 128, "%s/GetXStatusIcon", szProto);

	if (ServiceExists(szServiceName) && dat->xStatus > 0 && dat->xStatus <= 32)
		return (HICON)(CallProtoService(szProto, "/GetXStatusIcon", dat->xStatus, 0));
	return 0;
}

LRESULT GetSendButtonState(HWND hwnd)
{
	HWND hwndIDok=GetDlgItem(hwnd, IDOK);

	if(hwndIDok)
		return(SendMessage(hwndIDok, BUTTONSETASFLATBTN + 15, 0, 0));
	else
		return 0;
}

void EnableSendButton(HWND hwnd, int iMode)
{
	HWND hwndOK;
	SendMessage(GetDlgItem(hwnd, IDOK), BUTTONSETASFLATBTN + 14, iMode, 0);

	hwndOK = GetDlgItem(GetParent(GetParent(hwnd)), IDOK);

	if (IsWindow(hwndOK))
		SendMessage(hwndOK, BUTTONSETASFLATBTN + 14, iMode, 0);
}

void SendNudge(struct MessageWindowData *dat, HWND hwndDlg)
{
	char *szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
	char szServiceName[128];
	HANDLE hContact = dat->bIsMeta ? dat->hSubContact : dat->hContact;

	mir_snprintf(szServiceName, 128, "%s/SendNudge", szProto);
	if (ServiceExists(szServiceName) && ServiceExists(MS_NUDGE_SEND))
		CallService(MS_NUDGE_SEND, (WPARAM)hContact, 0);
	//CallProtoService(szProto, "/SendNudge", (WPARAM)hContact, 0);
}

void GetClientIcon(struct MessageWindowData *dat, HWND hwndDlg)
{
	DBVARIANT dbv = {0};

	if (dat->hClientIcon) {
		DestroyIcon(dat->hClientIcon);
		dat->hClientIcon = 0;
	}

	if (ServiceExists(MS_FP_GETCLIENTICON)) {
		if (!DBGetContactSettingString(dat->hContact, dat->szProto, "MirVer", &dbv)) {
			dat->hClientIcon = (HICON)CallService(MS_FP_GETCLIENTICON, (WPARAM)dbv.pszVal, 0);
			DBFreeVariant(&dbv);
		}
	}
}
/* buffer size in TCHARs
 * szwBuf must be large enough to hold at least size wchar_t's
 * proto may be NULL
 * per contact codepage only used with non-unicode cores (to get "faked" unicode nicknames...)
 */

/*
#if defined(_UNICODE)
int MY_GetContactDisplayNameW(HANDLE hContact, wchar_t *szwBuf, unsigned int size, const char *szProto, UINT codePage)
{
	CONTACTINFO ci;
	char *szBasenick = NULL;

	if (szProto == NULL)
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

	if (myGlobals.bUnicodeBuild) {
		ZeroMemory(&ci, sizeof(ci));
		ci.cbSize = sizeof(ci);
		ci.hContact = hContact;
		ci.szProto = (char *)szProto;
		ci.dwFlag = CNF_DISPLAY | CNF_UNICODE;
		if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
			if (ci.type == CNFT_ASCIIZ) {
				size_t len = lstrlenW(ci.pszVal);
				wcsncpy(szwBuf, ci.pszVal, size);
				szwBuf[size - 1] = 0;
				mir_free(ci.pszVal);
				return 0;
			}
			if (ci.type == CNFT_DWORD) {
				_ltow(ci.dVal, szwBuf, 10);
				szwBuf[size - 1] = 0;
				return 0;
			}
		}
	}
	szBasenick = (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0);
	MultiByteToWideChar(codePage, 0, szBasenick, -1, szwBuf, size);
	szwBuf[size - 1] = 0;
	return 0;
}
#endif
*/

int MY_CallService(const char *svc, WPARAM wParam, LPARAM lParam)
{
	return(CallService(svc, wParam, lParam));
}

int MY_ServiceExists(const char *svc)
{
	return(ServiceExists(svc));
}

void GetMaxMessageLength(HWND hwndDlg, struct MessageWindowData *dat)
{
	HANDLE hContact;
	char   *szProto;

	if (dat->bIsMeta)
		GetCurrentMetaContactProto(hwndDlg, dat);

	hContact = dat->bIsMeta ? dat->hSubContact : dat->hContact;
	szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;

	if (dat->szProto) {
		int nMax;

		nMax = CallProtoService(szProto, PS_GETCAPS, PFLAG_MAXLENOFMESSAGE, (LPARAM)hContact);
		if (nMax) {
			if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "autosplit", 0))
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_EXLIMITTEXT, 0, 20000);
			else
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_EXLIMITTEXT, 0, (LPARAM)nMax);
			dat->nMax = nMax;
		} else {
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_EXLIMITTEXT, 0, (LPARAM)20000);
			dat->nMax = 20000;
		}
	}
}

#define STATUSMSG_XSTATUSNAME 1
#define STATUSMSG_CLIST 2
#define STATUSMSG_YIM 3
#define STATUSMSG_GG 4
#define STATUSMSG_XSTATUS 5

/*
 * read status message from the database. Standard clist status message has priority,
 * but if not available, custom messages like xStatus can be used.
 */

void GetCachedStatusMsg(HWND hwndDlg, struct MessageWindowData *dat)
{
	DBVARIANT dbv = {0};
	HANDLE hContact;
	BYTE bStatusMsgValid = 0;
	char *szProto = dat ? dat->szProto : NULL;

	if (dat == NULL)
		return;

	dat->statusMsg[0] = 0;

	hContact = dat->hContact;

	if (!DBGetContactSettingTString(hContact, "CList", "StatusMsg", &dbv) && lstrlen(dbv.ptszVal) > 1)
		bStatusMsgValid = STATUSMSG_CLIST;
	else {
		if (szProto) {
			if (!DBGetContactSettingTString(hContact, szProto, "YMsg", &dbv) && lstrlen(dbv.ptszVal) > 1)
				bStatusMsgValid = STATUSMSG_YIM;
			else if (!DBGetContactSettingTString(hContact, szProto, "StatusDescr", &dbv) && lstrlen(dbv.ptszVal) > 1)
				bStatusMsgValid = STATUSMSG_GG;
			else if (!DBGetContactSettingTString(hContact, szProto, "XStatusMsg", &dbv) && lstrlen(dbv.ptszVal) > 1)
				bStatusMsgValid = STATUSMSG_XSTATUS;
		}
	}
	if (bStatusMsgValid == 0) {     // no status msg, consider xstatus name (if available)
		if (!DBGetContactSettingTString(hContact, szProto, "XStatusName", &dbv) && lstrlen(dbv.ptszVal) > 1) {
			bStatusMsgValid = STATUSMSG_XSTATUSNAME;
			mir_sntprintf(dat->statusMsg, safe_sizeof(dat->statusMsg), _T("%s"), dbv.ptszVal);
			mir_free(dbv.ptszVal);
		} else {
			BYTE bXStatus = DBGetContactSettingByte(hContact, szProto, "XStatusId", 0);
			if (bXStatus > 0 && bXStatus <= 31) {
				mir_sntprintf(dat->statusMsg, safe_sizeof(dat->statusMsg), _T("%s"), xStatusDescr[bXStatus]);
				bStatusMsgValid = STATUSMSG_XSTATUSNAME;
			}
		}
	}
	if (bStatusMsgValid > STATUSMSG_XSTATUSNAME) {
		int j = 0, i, iLen;
		iLen = lstrlen(dbv.ptszVal);
		for (i = 0; i < iLen && j < 1024; i++) {
			if (dbv.ptszVal[i] == (TCHAR)0x0d)
				continue;
			dat->statusMsg[j] = dbv.ptszVal[i] == (TCHAR)0x0a ? (TCHAR)' ' : dbv.ptszVal[i];
			j++;
		}
		dat->statusMsg[j] = (TCHAR)0;
		mir_free(dbv.ptszVal);
	}
}

/*
 * path utilities (make various paths relative to *PROFILE* directory, not miranda directory.
 * taken and modified from core services
 */

static char g_szProfilePath[MAX_PATH] = "\0";

static int MY_pathIsAbsolute(const char *path)
{
	if (!path || !(strlen(path) > 2))
		return 0;
	if ((path[1] == ':' && path[2] == '\\') || (path[0] == '\\' && path[1] == '\\')) return 1;
	return 0;
}

int MY_pathToRelative(const char *pSrc, char *pOut)
{
	if (g_szProfilePath[0] == 0) {
		CallService(MS_DB_GETPROFILEPATH, MAX_PATH, (LPARAM)g_szProfilePath);
		_strlwr(g_szProfilePath);
	}

	if (!pSrc || !strlen(pSrc) || strlen(pSrc) > MAX_PATH) return 0;
	if (!MY_pathIsAbsolute(pSrc)) {
		mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
		return strlen(pOut);
	} else {
		char szTmp[MAX_PATH];
		char szSTmp[MAX_PATH];
		mir_snprintf(szTmp, SIZEOF(szTmp), "%s", pSrc);
		_strlwr(szTmp);
		if (myGlobals.szSkinsPath) {
			mir_snprintf(szSTmp, SIZEOF(szSTmp), "%s", myGlobals.szSkinsPath);
			_strlwr(szSTmp);
		}
		if (strstr(szTmp, ".tsk")&&szSTmp&&strstr(szTmp, szSTmp)) {
			mir_snprintf(pOut, MAX_PATH, "%s", pSrc + strlen(myGlobals.szSkinsPath));
			return strlen(pOut);
		} else if (strstr(szTmp, g_szProfilePath)) {
			mir_snprintf(pOut, MAX_PATH, "%s", pSrc + strlen(g_szProfilePath) - 1);
			pOut[0]='.';
			return strlen(pOut);
		} else {
			mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
			return strlen(pOut);
		}
	}
}

int MY_pathToAbsolute(const char *pSrc, char *pOut)
{
	if (g_szProfilePath[0] == 0) {
		CallService(MS_DB_GETPROFILEPATH, MAX_PATH, (LPARAM)g_szProfilePath);
		_strlwr(g_szProfilePath);
	}
	if (!pSrc || !strlen(pSrc) || strlen(pSrc) > MAX_PATH) return 0;
	if (MY_pathIsAbsolute(pSrc)&&pSrc[0]!='.')
		mir_snprintf(pOut, MAX_PATH, "%s", pSrc);

	else if (myGlobals.szSkinsPath&&strstr(pSrc, ".tsk")&&pSrc[0]!='.')
		mir_snprintf(pOut, MAX_PATH, "%s%s", myGlobals.szSkinsPath, pSrc);

	else if (pSrc[0]=='.')
		mir_snprintf(pOut, MAX_PATH, "%s\\%s", g_szProfilePath, pSrc);

	return strlen(pOut);
}

void GetMyNick(HWND hwndDlg, struct MessageWindowData *dat)
{
	CONTACTINFO ci;

	ZeroMemory(&ci, sizeof(ci));
	ci.cbSize = sizeof(ci);
	ci.hContact = NULL;
	ci.szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
	ci.dwFlag = CNF_DISPLAY;
#if defined(_UNICODE)
	if (myGlobals.bUnicodeBuild)
		ci.dwFlag |= CNF_UNICODE;
	if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM)&ci)) {
		if (ci.type == CNFT_ASCIIZ) {
			if (lstrlen(ci.pszVal) < 1 || !_tcscmp(ci.pszVal, TranslateT("'(Unknown Contact)'"))) {
				MultiByteToWideChar(CP_ACP, 0, dat->myUin, -1, dat->szMyNickname, 130);
				dat->szMyNickname[129] = 0;
				if (ci.pszVal) {
					mir_free(ci.pszVal);
					ci.pszVal = NULL;
				}
			} else {
				_tcsncpy(dat->szMyNickname, ci.pszVal, 110);
				dat->szMyNickname[109] = 0;
				if (ci.pszVal) {
					mir_free(ci.pszVal);
					ci.pszVal = NULL;
				}
			}
		} else if (ci.type == CNFT_DWORD)
			_ltow(ci.dVal, dat->szMyNickname, 10);
		else
			_tcsncpy(dat->szMyNickname, _T("<undef>"), 110);                // that really should *never* happen
	} else
		_tcsncpy(dat->szMyNickname, _T("<undef>"), 110);                    // same here
#else
	if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM)&ci)) {
		if (ci.type == CNFT_ASCIIZ) {
			if (lstrlen(ci.pszVal) < 1 || !_tcscmp(ci.pszVal, TranslateT("'(Unknown Contact)'"))) {
				lstrcpynA(dat->szMyNickname, dat->myUin, 130);
				dat->szMyNickname[129] = 0;
				if (ci.pszVal) {
					mir_free(ci.pszVal);
					ci.pszVal = NULL;
				}
			} else {
				_tcsncpy(dat->szMyNickname, ci.pszVal, 130);
				dat->szMyNickname[129] = 0;
				if (ci.pszVal) {
					mir_free(ci.pszVal);
					ci.pszVal = NULL;
				}
			}
		} else if (ci.type == CNFT_DWORD)
			_ltoa(ci.dVal, dat->szMyNickname, 10);
		else
			_tcsncpy(dat->szMyNickname, "<undef>", 110);
	} else
		_tcsncpy(dat->szMyNickname, "<undef>", 110);
#endif
	if (ci.pszVal)
		mir_free(ci.pszVal);
}

/*
 * returns != 0 when one of the installed keyboard layouts belongs to an rtl language
 * used to find out whether we need to configure the message input box for bidirectional mode
 */

int FindRTLLocale(struct MessageWindowData *dat)
{
	HKL layouts[20];
	int i, result = 0;
	LCID lcid;
	WORD wCtype2[5];

	if (dat->iHaveRTLLang == 0) {
		ZeroMemory(layouts, 20 * sizeof(HKL));
		GetKeyboardLayoutList(20, layouts);
		for (i = 0; i < 20 && layouts[i]; i++) {
			lcid = MAKELCID(LOWORD(layouts[i]), 0);
			GetStringTypeA(lcid, CT_CTYPE2, "äöü", 3, wCtype2);
			if (wCtype2[0] == C2_RIGHTTOLEFT || wCtype2[1] == C2_RIGHTTOLEFT || wCtype2[2] == C2_RIGHTTOLEFT)
				result = 1;
		}
		dat->iHaveRTLLang = (result ? 1 : -1);
	} else
		result = dat->iHaveRTLLang == 1 ? 1 : 0;

	return result;
}

HICON MY_GetContactIcon(struct MessageWindowData *dat)
{
	if (dat->bIsMeta)
		return CopyIcon(LoadSkinnedProtoIcon(dat->szMetaProto, dat->wMetaStatus));

	if (ServiceExists(MS_CLIST_GETCONTACTICON) && ServiceExists(MS_CLIST_GETICONSIMAGELIST)) {
		HIMAGELIST iml = (HIMAGELIST)CallService(MS_CLIST_GETICONSIMAGELIST, 0, 0);
		int index = CallService(MS_CLIST_GETCONTACTICON, (WPARAM)dat->hContact, 0);
		if (iml && index >= 0)
			return ImageList_GetIcon(iml, index, ILD_NORMAL);
	}
	return CopyIcon(LoadSkinnedProtoIcon(dat->szProto, dat->wStatus));
}

void MTH_updatePreview(HWND hwndDlg, struct MessageWindowData *dat)
{
	TMathWindowInfo mathWndInfo;
	HWND hwndEdit = GetDlgItem(hwndDlg, dat->bType == SESSIONTYPE_IM ? IDC_MESSAGE : IDC_CHAT_MESSAGE);
	int len = GetWindowTextLengthA(hwndEdit);
	RECT windRect;
	char * thestr = malloc(len + 5);

	GetWindowTextA(hwndEdit, thestr, len + 1);
	GetWindowRect(dat->pContainer->hwnd, &windRect);
	mathWndInfo.top = windRect.top;
	mathWndInfo.left = windRect.left;
	mathWndInfo.right = windRect.right;
	mathWndInfo.bottom = windRect.bottom;

	CallService(MTH_SETFORMULA, 0, (LPARAM) thestr);
	CallService(MTH_RESIZE, 0, (LPARAM) &mathWndInfo);
	free(thestr);
}

void MTH_updateMathWindow(HWND hwndDlg, struct MessageWindowData *dat)
{
	WINDOWPLACEMENT cWinPlace;

	if (!myGlobals.m_MathModAvail)
		return;

	MTH_updatePreview(hwndDlg, dat);
	CallService(MTH_SHOW, 0, 0);
	cWinPlace.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(dat->pContainer->hwnd, &cWinPlace);
	return;
}
