/*
  	Name: NewEventNotify - Plugin for Miranda ICQ
  	File: neweventnotify.h - Main Header File
  	Version: 0.0.4
  	Description: Notifies you when you receive a message
  	Author: icebreaker, <icebreaker@newmail.net>
  	Date: 18.07.02 13:59 / Update: 16.09.02 17:45
  	Copyright: (C) 2002 Starzinger Michael

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	$Id$

	Event popups for tabSRMM - most of the code taken from NewEventNotify (see copyright above)

  	Code modified and adapted for tabSRMM by Nightwish (silvercircle@gmail.com)
  	Additional code (popup merging, options) by Prezes
*/

#include "commonheaders.h"
#pragma hdrstop
#include <malloc.h>

#include "../../include/m_icq.h"

extern      HINSTANCE g_hInst;
extern      NEN_OPTIONS nen_options;
extern      HANDLE hMessageWindowList;
extern      MYGLOBALS myGlobals;
extern      struct ContainerWindowData *pFirstContainer;
extern      BOOL CALLBACK DlgProcSetupStatusModes(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      HIMAGELIST CreateStateImageList();
extern      HANDLE hTrayAnimThread, g_hEvent;

PLUGIN_DATA *PopUpList[20];
static int PopupCount = 0;

BOOL        bWmNotify = TRUE;
extern int  safe_wcslen(wchar_t *msg, int chars);

static void CheckForRemoveMask()
{
	if(!DBGetContactSettingByte(NULL, MODULE, "firsttime", 0) && (nen_options.maskActL & MASK_REMOVE || nen_options.maskActR & MASK_REMOVE || nen_options.maskActTE & MASK_REMOVE)) {
		MessageBoxA(0, Translate("One of your popup actions is set to DISMISS EVENT.\nNote that this options may have unwanted side effects as it REMOVES the event from the unread queue.\nThis may lead to events not showing up as \"new\". If you don't want this behaviour, please review the Event Notifications settings page."), "tabSRMM Warning Message", MB_OK | MB_ICONSTOP);
		DBWriteContactSettingByte(NULL, MODULE, "firsttime", 1);
	}
}


int NEN_ReadOptions(NEN_OPTIONS *options)
{
	options->bPreview = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_PREVIEW, TRUE);
	options->bDefaultColorMsg = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_MESSAGE, FALSE);
	options->bDefaultColorUrl = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_URL, FALSE);
	options->bDefaultColorFile = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_FILE, FALSE);
	options->bDefaultColorOthers = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_OTHERS, FALSE);
	options->colBackMsg = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLBACK_MESSAGE, DEFAULT_COLBACK);
	options->colTextMsg = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLTEXT_MESSAGE, DEFAULT_COLTEXT);
	options->colBackUrl = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLBACK_URL, DEFAULT_COLBACK);
	options->colTextUrl = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLTEXT_URL, DEFAULT_COLTEXT);
	options->colBackFile = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLBACK_FILE, DEFAULT_COLBACK);
	options->colTextFile = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLTEXT_FILE, DEFAULT_COLTEXT);
	options->colBackOthers = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLBACK_OTHERS, DEFAULT_COLBACK);
	options->colTextOthers = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLTEXT_OTHERS, DEFAULT_COLTEXT);
	options->maskNotify = (UINT)DBGetContactSettingByte(NULL, MODULE, OPT_MASKNOTIFY, DEFAULT_MASKNOTIFY);
	options->maskActL = (UINT)DBGetContactSettingByte(NULL, MODULE, OPT_MASKACTL, DEFAULT_MASKACTL);
	options->maskActR = (UINT)DBGetContactSettingByte(NULL, MODULE, OPT_MASKACTR, DEFAULT_MASKACTR);
	options->maskActTE = (UINT)DBGetContactSettingByte(NULL, MODULE, OPT_MASKACTTE, DEFAULT_MASKACTR) & (MASK_OPEN | MASK_DISMISS);
	options->bMergePopup = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_MERGEPOPUP, FALSE);
	options->iDelayMsg = (int)DBGetContactSettingDword(NULL, MODULE, OPT_DELAY_MESSAGE, (DWORD)DEFAULT_DELAY);
	options->iDelayUrl = (int)DBGetContactSettingDword(NULL, MODULE, OPT_DELAY_URL, (DWORD)DEFAULT_DELAY);
	options->iDelayFile = (int)DBGetContactSettingDword(NULL, MODULE, OPT_DELAY_FILE, (DWORD)DEFAULT_DELAY);
	options->iDelayOthers = (int)DBGetContactSettingDword(NULL, MODULE, OPT_DELAY_OTHERS, (DWORD)DEFAULT_DELAY);
	options->iDelayDefault = (int)DBGetContactSettingRangedWord(NULL, "PopUp", "Seconds", SETTING_LIFETIME_DEFAULT, SETTING_LIFETIME_MIN, SETTING_LIFETIME_MAX);
	options->bShowDate = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_SHOW_DATE, FALSE);
	options->bShowTime = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_SHOW_TIME, FALSE);
	options->bShowHeaders = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_SHOW_HEADERS, FALSE);
	options->iNumberMsg = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_NUMBER_MSG, TRUE);
	options->bShowON = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_SHOW_ON, TRUE);
	options->bNoRSS = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_NORSS, FALSE);
	options->iDisable = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_DISABLE, 0);
	options->dwStatusMask = (DWORD)DBGetContactSettingDword(NULL, MODULE, "statusmask", (DWORD)-1);
	options->bTraySupport = (BOOL)DBGetContactSettingByte(NULL, MODULE, "traysupport", 0);
	options->bMinimizeToTray = (BOOL)DBGetContactSettingByte(NULL, MODULE, "mintotray", 0);
	options->iAutoRestore = 0;
	options->bWindowCheck = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_WINDOWCHECK, 0);
	options->bNoRSS = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_NORSS, 0);
	options->iLimitPreview = (int)DBGetContactSettingDword(NULL, MODULE, OPT_LIMITPREVIEW, 0);
	options->bAnimated = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_MINIMIZEANIMATED, 1);
	options->wMaxFavorites = 15;
	options->wMaxRecent = 15;
	options->iAnnounceMethod = (int)DBGetContactSettingByte(NULL, MODULE, OPT_ANNOUNCEMETHOD, 0);
	options->floaterMode = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_FLOATER, 0);
	options->bFloaterInWin = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_FLOATERINWIN, 1);
	options->bFloaterOnlyMin = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_FLOATERONLYMIN, 0);
	options->dwRemoveMask = DBGetContactSettingDword(NULL, MODULE, OPT_REMOVEMASK, 0);
	options->bSimpleMode = DBGetContactSettingByte(NULL, MODULE, OPT_SIMPLEOPT, 0);
	CheckForRemoveMask();
	return 0;
}

int NEN_WriteOptions(NEN_OPTIONS *options)
{
	DBWriteContactSettingByte(NULL, MODULE, OPT_PREVIEW, (BYTE)options->bPreview);
	DBWriteContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_MESSAGE, (BYTE)options->bDefaultColorMsg);
	DBWriteContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_URL, (BYTE)options->bDefaultColorUrl);
	DBWriteContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_FILE, (BYTE)options->bDefaultColorFile);
	DBWriteContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_OTHERS, (BYTE)options->bDefaultColorOthers);
	DBWriteContactSettingDword(NULL, MODULE, OPT_COLBACK_MESSAGE, (DWORD)options->colBackMsg);
	DBWriteContactSettingDword(NULL, MODULE, OPT_COLTEXT_MESSAGE, (DWORD)options->colTextMsg);
	DBWriteContactSettingDword(NULL, MODULE, OPT_COLBACK_URL, (DWORD)options->colBackUrl);
	DBWriteContactSettingDword(NULL, MODULE, OPT_COLTEXT_URL, (DWORD)options->colTextUrl);
	DBWriteContactSettingDword(NULL, MODULE, OPT_COLBACK_FILE, (DWORD)options->colBackFile);
	DBWriteContactSettingDword(NULL, MODULE, OPT_COLTEXT_FILE, (DWORD)options->colTextFile);
	DBWriteContactSettingDword(NULL, MODULE, OPT_COLBACK_OTHERS, (DWORD)options->colBackOthers);
	DBWriteContactSettingDword(NULL, MODULE, OPT_COLTEXT_OTHERS, (DWORD)options->colTextOthers);
	DBWriteContactSettingByte(NULL, MODULE, OPT_MASKNOTIFY, (BYTE)options->maskNotify);
	DBWriteContactSettingByte(NULL, MODULE, OPT_MASKACTL, (BYTE)options->maskActL);
	DBWriteContactSettingByte(NULL, MODULE, OPT_MASKACTR, (BYTE)options->maskActR);
	DBWriteContactSettingByte(NULL, MODULE, OPT_MASKACTTE, (BYTE)options->maskActTE);
	DBWriteContactSettingByte(NULL, MODULE, OPT_MERGEPOPUP, (BYTE)options->bMergePopup);
	DBWriteContactSettingDword(NULL, MODULE, OPT_DELAY_MESSAGE, (DWORD)options->iDelayMsg);
	DBWriteContactSettingDword(NULL, MODULE, OPT_DELAY_URL, (DWORD)options->iDelayUrl);
	DBWriteContactSettingDword(NULL, MODULE, OPT_DELAY_FILE, (DWORD)options->iDelayFile);
	DBWriteContactSettingDword(NULL, MODULE, OPT_DELAY_OTHERS, (DWORD)options->iDelayOthers);
	DBWriteContactSettingByte(NULL, MODULE, OPT_SHOW_DATE, (BYTE)options->bShowDate);
	DBWriteContactSettingByte(NULL, MODULE, OPT_SHOW_TIME, (BYTE)options->bShowTime);
	DBWriteContactSettingByte(NULL, MODULE, OPT_SHOW_HEADERS, (BYTE)options->bShowHeaders);
	DBWriteContactSettingByte(NULL, MODULE, OPT_NUMBER_MSG, (BYTE)options->iNumberMsg);
	DBWriteContactSettingByte(NULL, MODULE, OPT_SHOW_ON, (BYTE)options->bShowON);
	DBWriteContactSettingByte(NULL, MODULE, OPT_DISABLE, (BYTE)options->iDisable);
	DBWriteContactSettingByte(NULL, MODULE, "traysupport", (BYTE)options->bTraySupport);
	DBWriteContactSettingByte(NULL, MODULE, "mintotray", (BYTE)options->bMinimizeToTray);
	DBWriteContactSettingByte(NULL, MODULE, OPT_WINDOWCHECK, (BYTE)options->bWindowCheck);
	DBWriteContactSettingByte(NULL, MODULE, OPT_NORSS, (BYTE)options->bNoRSS);
	DBWriteContactSettingDword(NULL, MODULE, OPT_LIMITPREVIEW, options->iLimitPreview);
	DBWriteContactSettingByte(NULL, MODULE, OPT_MINIMIZEANIMATED, (BYTE)options->bAnimated);
	DBWriteContactSettingByte(NULL, MODULE, OPT_ANNOUNCEMETHOD, (BYTE)options->iAnnounceMethod);
	DBWriteContactSettingByte(NULL, MODULE, OPT_FLOATER, (BYTE)options->floaterMode);
	DBWriteContactSettingByte(NULL, MODULE, OPT_FLOATERINWIN, (BYTE)options->bFloaterInWin);
	DBWriteContactSettingByte(NULL, MODULE, OPT_FLOATERONLYMIN, (BYTE)options->bFloaterOnlyMin);
	DBWriteContactSettingDword(NULL, MODULE, OPT_REMOVEMASK, options->dwRemoveMask);
	DBWriteContactSettingByte(NULL, MODULE, OPT_SIMPLEOPT, (BYTE)options->bSimpleMode);
	return 0;
}

static struct LISTOPTIONSGROUP lGroups[] = {
    0, _T("Announce events of type..."),
    0, _T("General options"),
    0, _T("System tray and floater options"),
    0, _T("Left click actions (popups only)"),
    0, _T("Right click actions (popups only)"),
    0, _T("Timeout actions (popups only)"),
    0, _T("Popup merging (per user) options"),
    0, _T("Remove popups under following conditions"),
    0, NULL
};

static struct LISTOPTIONSITEM defaultItems[] = {
    0, _T("Show a preview of the event"), IDC_CHKPREVIEW, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bPreview, 1,
    0, _T("Don't announce event when message dialog is open"), IDC_CHKWINDOWCHECK, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bWindowCheck, 1,
    0, _T("Don't announce events from RSS protocols"), IDC_NORSS, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bNoRSS, 1,
    0, _T("Enable the system tray icon"), IDC_ENABLETRAYSUPPORT, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bTraySupport, 2,
    0, _T("Show the floater"), IDC_ENABLETRAYSUPPORT, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.floaterMode, 2,
    0, _T("When floater is enabled, only show it while the contact list is minimized"), IDC_ENABLETRAYSUPPORT, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bFloaterOnlyMin, 2,
    0, _T("Show session list menu on the message windows status bar"), IDC_MINIMIZETOTRAY, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bFloaterInWin, 2,
    0, _T("Minimize containers to system tray or floater"), IDC_MINIMIZETOTRAY, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bMinimizeToTray, 2,
    0, _T("Minimize and restore animated"), IDC_ANIMATED, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bAnimated, 2,
    0, _T("Merge popups \"per user\" (experimental, unstable)"), IDC_CHKMERGEPOPUP, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bMergePopup, 6,
    0, _T("Show date for merged popups"), IDC_CHKSHOWDATE, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bShowDate, 6,
    0, _T("Show time for merged popups"), IDC_CHKSHOWTIME, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bShowTime, 6,
    0, _T("Show headers"), IDC_CHKSHOWHEADERS, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.bShowHeaders, 6,
    0, _T("Dismiss popup"), MASK_DISMISS, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskActL, 3,
    0, _T("Open event"), MASK_OPEN, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskActL, 3,
    0, _T("Dismiss event"), MASK_REMOVE, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskActL, 3,

    0, _T("Dismiss popup"), MASK_DISMISS, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskActR, 4,
    0, _T("Open event"), MASK_OPEN, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskActR, 4,
    0, _T("Dismiss event"), MASK_REMOVE, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskActR, 4,

    0, _T("Dismiss popup"), MASK_DISMISS, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskActTE, 5,
    0, _T("Open event"), MASK_OPEN, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskActTE, 5,
//    0, "Dismiss event", MASK_REMOVE, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskActTE, 5,

    0, _T("Disable ALL event notifications (check, if you're using an external plugin for event notifications)"), IDC_CHKWINDOWCHECK, LOI_TYPE_SETTING, (UINT_PTR)&nen_options.iDisable, 0,
    0, _T("Message events"), MASK_MESSAGE, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskNotify, 0,
    0, _T("File events"), MASK_FILE, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskNotify, 0,
    0, _T("URL events"), MASK_URL, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskNotify, 0,
    0, _T("Other events"), MASK_OTHER, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.maskNotify, 0,

    0, _T("Remove event popups for a contact when its message window becomes focused"), PU_REMOVE_ON_FOCUS, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.dwRemoveMask, 7,
    0, _T("Remove event popups for a contact when you start typing a reply"), PU_REMOVE_ON_TYPE, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.dwRemoveMask, 7,
    0, _T("Remove event popups for a contact when you send a reply"), PU_REMOVE_ON_SEND, LOI_TYPE_FLAG, (UINT_PTR)&nen_options.dwRemoveMask, 7,
    
    0, NULL, 0, 0, 0, 0
};

BOOL CALLBACK DlgProcPopupOpts(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	NEN_OPTIONS *options = &nen_options;
	LRESULT iIndex;

	switch (msg) {
	case WM_INITDIALOG: {
		TVINSERTSTRUCT tvi = {0};
		int i = 0;

		SetWindowLong(GetDlgItem(hWnd, IDC_EVENTOPTIONS), GWL_STYLE, GetWindowLong(GetDlgItem(hWnd, IDC_EVENTOPTIONS), GWL_STYLE) | (TVS_NOHSCROLL | TVS_CHECKBOXES));
		SendDlgItemMessage(hWnd, IDC_EVENTOPTIONS, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());
		TranslateDialogDefault(hWnd);

		/*
		* fill the tree view
		*/

		while(lGroups[i].szName != NULL) {
			tvi.hParent = 0;
			tvi.hInsertAfter = TVI_LAST;
			tvi.item.mask = TVIF_TEXT | TVIF_STATE;
			tvi.item.pszText = TranslateTS(lGroups[i].szName);
			tvi.item.stateMask = TVIS_STATEIMAGEMASK | TVIS_EXPANDED | TVIS_BOLD;
			tvi.item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_EXPANDED | TVIS_BOLD;
			lGroups[i++].handle = (LRESULT)TreeView_InsertItem( GetDlgItem(hWnd, IDC_EVENTOPTIONS), &tvi);
		}
		i = 0;

		while(defaultItems[i].szName != 0) {
			tvi.hParent = (HTREEITEM)lGroups[defaultItems[i].uGroup].handle;
			tvi.hInsertAfter = TVI_LAST;
			tvi.item.pszText = TranslateTS(defaultItems[i].szName);
			tvi.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
			tvi.item.lParam = i;
			tvi.item.stateMask = TVIS_STATEIMAGEMASK;
			if(defaultItems[i].uType == LOI_TYPE_SETTING)
				tvi.item.state = INDEXTOSTATEIMAGEMASK(*((BOOL *)defaultItems[i].lParam) ? 3 : 2);
			else if(defaultItems[i].uType == LOI_TYPE_FLAG) {
				UINT uVal = *((UINT *)defaultItems[i].lParam);
				tvi.item.state = INDEXTOSTATEIMAGEMASK(uVal & defaultItems[i].id ? 3 : 2);
			}
			defaultItems[i].handle = (LRESULT)TreeView_InsertItem( GetDlgItem(hWnd, IDC_EVENTOPTIONS), &tvi);
			i++;
		}
		SendDlgItemMessage(hWnd, IDC_COLBACK_MESSAGE, CPM_SETCOLOUR, 0, options->colBackMsg);
		SendDlgItemMessage(hWnd, IDC_COLTEXT_MESSAGE, CPM_SETCOLOUR, 0, options->colTextMsg);
		SendDlgItemMessage(hWnd, IDC_COLBACK_URL, CPM_SETCOLOUR, 0, options->colBackUrl);
		SendDlgItemMessage(hWnd, IDC_COLTEXT_URL, CPM_SETCOLOUR, 0, options->colTextUrl);
		SendDlgItemMessage(hWnd, IDC_COLBACK_FILE, CPM_SETCOLOUR, 0, options->colBackFile);
		SendDlgItemMessage(hWnd, IDC_COLTEXT_FILE, CPM_SETCOLOUR, 0, options->colTextFile);
		SendDlgItemMessage(hWnd, IDC_COLBACK_OTHERS, CPM_SETCOLOUR, 0, options->colBackOthers);
		SendDlgItemMessage(hWnd, IDC_COLTEXT_OTHERS, CPM_SETCOLOUR, 0, options->colTextOthers);
		CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_MESSAGE, options->bDefaultColorMsg?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_URL, options->bDefaultColorUrl?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_FILE, options->bDefaultColorFile?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_OTHERS, options->bDefaultColorOthers?BST_CHECKED:BST_UNCHECKED);
		SetDlgItemInt(hWnd, IDC_DELAY_MESSAGE, options->iDelayMsg != -1?options->iDelayMsg:0, TRUE);
		SetDlgItemInt(hWnd, IDC_DELAY_URL, options->iDelayUrl != -1?options->iDelayUrl:0, TRUE);
		SetDlgItemInt(hWnd, IDC_DELAY_FILE, options->iDelayFile != -1?options->iDelayFile:0, TRUE);
		SetDlgItemInt(hWnd, IDC_DELAY_OTHERS, options->iDelayOthers != -1?options->iDelayOthers:0, TRUE);

		EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_MESSAGE), !options->bDefaultColorMsg);
		EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_MESSAGE), !options->bDefaultColorMsg);
		EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_URL), !options->bDefaultColorUrl);
		EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_URL), !options->bDefaultColorUrl);
		EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_FILE), !options->bDefaultColorFile);
		EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_FILE), !options->bDefaultColorFile);
		EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_OTHERS), !options->bDefaultColorOthers);
		EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_OTHERS), !options->bDefaultColorOthers);
		
		//disable merge messages options when is not using

		EnableWindow(GetDlgItem(hWnd, IDC_DELAY_MESSAGE), options->iDelayMsg != -1);
		EnableWindow(GetDlgItem(hWnd, IDC_DELAY_URL), options->iDelayUrl != -1);
		EnableWindow(GetDlgItem(hWnd, IDC_DELAY_FILE), options->iDelayFile != -1);
		EnableWindow(GetDlgItem(hWnd, IDC_DELAY_OTHERS), options->iDelayOthers != -1);

		SetDlgItemInt(hWnd, IDC_MESSAGEPREVIEWLIMIT, options->iLimitPreview, FALSE);
		CheckDlgButton(hWnd, IDC_LIMITPREVIEW, (options->iLimitPreview > 0) ? 1 : 0);
		SendDlgItemMessage(hWnd, IDC_MESSAGEPREVIEWLIMITSPIN, UDM_SETRANGE, 0, MAKELONG(2048, options->iLimitPreview > 0 ? 50 : 0));
		SendDlgItemMessage(hWnd, IDC_MESSAGEPREVIEWLIMITSPIN, UDM_SETPOS, 0, (LPARAM)options->iLimitPreview);
		EnableWindow(GetDlgItem(hWnd, IDC_MESSAGEPREVIEWLIMIT), IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW));
		EnableWindow(GetDlgItem(hWnd, IDC_MESSAGEPREVIEWLIMITSPIN), IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW));

		SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Don't use simple mode"));
		SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Notify always"));
		SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Notify for unfocused sessions"));
		SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Notify only when no window is open"));
		EnableWindow(GetDlgItem(hWnd, IDC_EVENTOPTIONS), options->bSimpleMode == 0);
		SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_SETCURSEL, options->bSimpleMode, 0);

		iIndex = SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_ADDSTRING, -1, (LPARAM)TranslateT("<None>"));
		SendDlgItemMessageA(hWnd, IDC_ANNOUNCEMETHOD, CB_SETCURSEL, iIndex, 0);

		if(ServiceExists(MS_POPUP_ADDPOPUPEX)) {
			iIndex = SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_ADDSTRING, -1, (LPARAM)TranslateT("Popups"));
			SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETITEMDATA, (WPARAM)iIndex, 1);
			if(options->iAnnounceMethod == 1)
				SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETCURSEL, iIndex, 0);
		}
		iIndex = SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_ADDSTRING, -1, (LPARAM)TranslateT("Balloon tooltips"));
		SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETITEMDATA, (WPARAM)iIndex, 2);
		if(options->iAnnounceMethod == 2)
			SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETCURSEL, iIndex, 0);
		if(ServiceExists("OSD/Announce")) {
			iIndex = SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_ADDSTRING, -1, (LPARAM)TranslateT("On screen display"));
			SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETITEMDATA, (WPARAM)iIndex, 3);
			if(options->iAnnounceMethod == 3)
				SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_SETCURSEL, iIndex, 0);
		}
		bWmNotify = FALSE;
		return TRUE;
	}
	case DM_STATUSMASKSET:
		DBWriteContactSettingDword(0, MODULE, "statusmask", (DWORD)lParam);
		options->dwStatusMask = (int)lParam;
		break;
	case WM_COMMAND:
		if (!bWmNotify) {
			switch (LOWORD(wParam)) {
				case IDC_PREVIEW:
					PopupPreview(options);
					break;
				case IDC_SIMPLEMODE:
					options->bSimpleMode = (BYTE)SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_GETCURSEL, 0, 0);
					EnableWindow(GetDlgItem(hWnd, IDC_EVENTOPTIONS), options->bSimpleMode == 0);
					break;
				case IDC_POPUPSTATUSMODES:
				{   
					HWND hwndNew = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CHOOSESTATUSMODES), hWnd, DlgProcSetupStatusModes, DBGetContactSettingDword(0, MODULE, "statusmask", (DWORD)-1));
					SendMessage(hwndNew, DM_SETPARENTDIALOG, 0, (LPARAM)hWnd);
					break;
				}	
				default:
				{
					options->bDefaultColorMsg = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_MESSAGE);
					options->bDefaultColorUrl = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_URL);
					options->bDefaultColorFile = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_FILE);
					options->bDefaultColorOthers = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_OTHERS);
					options->iDelayMsg = IsDlgButtonChecked(hWnd, IDC_CHKINFINITE_MESSAGE)?(DWORD)-1:(DWORD)GetDlgItemInt(hWnd, IDC_DELAY_MESSAGE, NULL, FALSE);
					options->iDelayUrl = IsDlgButtonChecked(hWnd, IDC_CHKINFINITE_URL)?(DWORD)-1:(DWORD)GetDlgItemInt(hWnd, IDC_DELAY_URL, NULL, FALSE);
					options->iDelayFile = IsDlgButtonChecked(hWnd, IDC_CHKINFINITE_FILE)?(DWORD)-1:(DWORD)GetDlgItemInt(hWnd, IDC_DELAY_FILE, NULL, FALSE);
					options->iDelayOthers = IsDlgButtonChecked(hWnd, IDC_CHKINFINITE_OTHERS)?(DWORD)-1:(DWORD)GetDlgItemInt(hWnd, IDC_DELAY_OTHERS, NULL, FALSE);
					options->iAnnounceMethod = SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_GETITEMDATA, (WPARAM)SendDlgItemMessage(hWnd, IDC_ANNOUNCEMETHOD, CB_GETCURSEL, 0, 0), 0);
					options->bSimpleMode = (BYTE)SendDlgItemMessage(hWnd, IDC_SIMPLEMODE, CB_GETCURSEL, 0, 0);
		
					if(IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW))
						options->iLimitPreview = GetDlgItemInt(hWnd, IDC_MESSAGEPREVIEWLIMIT, NULL, FALSE);
					else
						options->iLimitPreview = 0;
					EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_MESSAGE), !options->bDefaultColorMsg);
					EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_MESSAGE), !options->bDefaultColorMsg);
					EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_URL), !options->bDefaultColorUrl);
					EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_URL), !options->bDefaultColorUrl);
					EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_FILE), !options->bDefaultColorFile);
					EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_FILE), !options->bDefaultColorFile);
					EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_OTHERS), !options->bDefaultColorOthers);
					EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_OTHERS), !options->bDefaultColorOthers);
		
					EnableWindow(GetDlgItem(hWnd, IDC_MESSAGEPREVIEWLIMIT), IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW));
					EnableWindow(GetDlgItem(hWnd, IDC_MESSAGEPREVIEWLIMITSPIN), IsDlgButtonChecked(hWnd, IDC_LIMITPREVIEW));
					//disable delay textbox when infinite is checked
					EnableWindow(GetDlgItem(hWnd, IDC_DELAY_MESSAGE), options->iDelayMsg != -1);
					EnableWindow(GetDlgItem(hWnd, IDC_DELAY_URL), options->iDelayUrl != -1);
					EnableWindow(GetDlgItem(hWnd, IDC_DELAY_FILE), options->iDelayFile != -1);
					EnableWindow(GetDlgItem(hWnd, IDC_DELAY_OTHERS), options->iDelayOthers != -1);
					EnableWindow(GetDlgItem(hWnd, IDC_ANIMATED), options->bMinimizeToTray);
					if (HIWORD(wParam) == CPN_COLOURCHANGED) {
						options->colBackMsg = SendDlgItemMessage(hWnd, IDC_COLBACK_MESSAGE, CPM_GETCOLOUR, 0, 0);
						options->colTextMsg = SendDlgItemMessage(hWnd, IDC_COLTEXT_MESSAGE, CPM_GETCOLOUR, 0, 0);
						options->colBackUrl = SendDlgItemMessage(hWnd, IDC_COLBACK_URL, CPM_GETCOLOUR, 0, 0);
						options->colTextUrl = SendDlgItemMessage(hWnd, IDC_COLTEXT_URL, CPM_GETCOLOUR, 0, 0);
						options->colBackFile = SendDlgItemMessage(hWnd, IDC_COLBACK_FILE, CPM_GETCOLOUR, 0, 0);
						options->colTextFile = SendDlgItemMessage(hWnd, IDC_COLTEXT_FILE, CPM_GETCOLOUR, 0, 0);
						options->colBackOthers = SendDlgItemMessage(hWnd, IDC_COLBACK_OTHERS, CPM_GETCOLOUR, 0, 0);
						options->colTextOthers = SendDlgItemMessage(hWnd, IDC_COLTEXT_OTHERS, CPM_GETCOLOUR, 0, 0);
					}
					EnableWindow(GetDlgItem(hWnd, IDC_USESHELLNOTIFY), options->bTraySupport);
					SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
					break;
				}
			}
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->idFrom) {
		case IDC_EVENTOPTIONS:
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
					SendDlgItemMessageA(hWnd, IDC_EVENTOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
					if(item.state & TVIS_BOLD && hti.flags & TVHT_ONITEMSTATEICON) {
						item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_BOLD;
						SendDlgItemMessageA(hWnd, IDC_EVENTOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
					}
					else if(hti.flags&TVHT_ONITEMSTATEICON) {
						if(((item.state & TVIS_STATEIMAGEMASK) >> 12) == 3) {
							item.state = INDEXTOSTATEIMAGEMASK(1);
							SendDlgItemMessageA(hWnd, IDC_EVENTOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
						}
						SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
					}
				}
			}
			break;
			}
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY: {
				int i = 0;
				TVITEM item = {0};
				struct ContainerWindowData *pContainer = pFirstContainer;

				while(defaultItems[i].szName != NULL) {
					item.mask = TVIF_HANDLE | TVIF_STATE;
					item.hItem = (HTREEITEM)defaultItems[i].handle;
					item.stateMask = TVIS_STATEIMAGEMASK;

					SendDlgItemMessageA(hWnd, IDC_EVENTOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
					if(defaultItems[i].uType == LOI_TYPE_SETTING) {
						BOOL *ptr = (BOOL *)defaultItems[i].lParam;
						*ptr = (item.state >> 12) == 3 ? TRUE : FALSE;
					}
					else if(defaultItems[i].uType == LOI_TYPE_FLAG) {
						UINT *uVal = (UINT *)defaultItems[i].lParam;
						*uVal = ((item.state >> 12) == 3) ? *uVal | defaultItems[i].id : *uVal & ~defaultItems[i].id;
					}
					i++;
				}
				NEN_WriteOptions(&nen_options);
				CheckForRemoveMask();
				CreateSystrayIcon(nen_options.bTraySupport);
                SetEvent(g_hEvent);                                 // wake up the thread which cares about the floater and tray
				/*
				* check if there are containers minimized to the tray, get them back, otherwise the're trapped forever :)
				* need to temporarily re-enable tray support, because the container checks for it.
				*/
				if(!nen_options.bTraySupport || !nen_options.bMinimizeToTray) {
					BOOL oldTray = nen_options.bTraySupport;
					BOOL oldMin = nen_options.bMinimizeToTray;

					nen_options.bTraySupport = TRUE;
					nen_options.bMinimizeToTray = TRUE;
					while(pContainer) {
						if(pContainer->bInTray)
							SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
						pContainer = pContainer->pNextContainer;
					}
					nen_options.bTraySupport = oldTray;
					nen_options.bMinimizeToTray = oldMin;
				}
				ShowWindow(myGlobals.g_hwndHotkeyHandler, nen_options.floaterMode ? SW_SHOW : SW_HIDE);
				break;
			}
		case PSN_RESET:
			NEN_ReadOptions(&nen_options);
			break;
		}
		break;
	case WM_DESTROY:
		{
			SendDlgItemMessage(hWnd, IDC_EVENTOPTIONS, TVM_GETIMAGELIST, TVSIL_STATE, 0);
			bWmNotify = TRUE;
			break;
		}
	default:
		break;
	}
	return FALSE;
}

static int NumberPopupData(HANDLE hContact)
{
    int n;

    for (n=0;n<20;n++) {
        if (!PopUpList[n] && !hContact)
            return n;

        if (PopUpList[n] && PopUpList[n]->hContact == hContact)
            return n;
    }
    return -1;
}

static char* GetPreview(UINT eventType, char* pBlob)
{
    char* comment1 = NULL;
    char* comment2 = NULL;
    char* commentFix = NULL;
    static char szPreviewHelp[256];

    //now get text
    switch (eventType) {
        case EVENTTYPE_MESSAGE:
            if (pBlob) comment1 = pBlob;
            commentFix = Translate(POPUP_COMMENT_MESSAGE);
            break;
        case EVENTTYPE_AUTHREQUEST:
            if(pBlob) {
                mir_snprintf(szPreviewHelp, 256, Translate("%s requested authorization"), pBlob + 8);
                return szPreviewHelp;
            }
            commentFix = Translate(POPUP_COMMENT_AUTH);
            break;
        case EVENTTYPE_ADDED:
            if(pBlob) {
                mir_snprintf(szPreviewHelp, 256, Translate("%s added you to the contact list"), pBlob + 8);
                return szPreviewHelp;
            }
            commentFix = Translate(POPUP_COMMENT_ADDED);
            break;
        case EVENTTYPE_URL:
            if (pBlob) comment2 = pBlob;
            if (pBlob) comment1 = pBlob + strlen(comment2) + 1;
            commentFix = Translate(POPUP_COMMENT_URL);
            break;

        case EVENTTYPE_FILE:
            if (pBlob) comment2 = pBlob + 4;
            if (pBlob) comment1 = pBlob + strlen(comment2) + 5;
            commentFix = Translate(POPUP_COMMENT_FILE);
            break;

        case EVENTTYPE_CONTACTS:
            commentFix = Translate(POPUP_COMMENT_CONTACTS);
            break;
        case ICQEVENTTYPE_WEBPAGER:
            if (pBlob) comment1 = pBlob;
            commentFix = Translate(POPUP_COMMENT_WEBPAGER);
            break;
        case ICQEVENTTYPE_EMAILEXPRESS:
            if (pBlob) comment1 = pBlob;
            commentFix = Translate(POPUP_COMMENT_EMAILEXP);
            break;

        default:
            commentFix = Translate(POPUP_COMMENT_OTHER);
            break;
    }

    if (comment1)
        if (strlen(comment1) > 0)
            return comment1;
    if (comment2)
        if (strlen(comment2) > 0)
            return comment2;

    return commentFix;
}

static int PopupUpdate(HANDLE hContact, HANDLE hEvent)
{
    PLUGIN_DATA *pdata;
    DBEVENTINFO dbe;
    char lpzText[MAX_SECONDLINE] = "";
    char timestamp[MAX_DATASIZE] = "";
    char formatTime[MAX_DATASIZE] = "";
    int iEvent = 0;
    char *p = lpzText;
    int  available = 0, i;
    
    pdata = (PLUGIN_DATA*)PopUpList[NumberPopupData(hContact)];
    
    ZeroMemory((void *)&dbe, sizeof(dbe));
    
    if (hEvent) {
        if(pdata->pluginOptions->bShowHeaders) 
            mir_snprintf(pdata->szHeader, sizeof(pdata->szHeader), "[b]%s %d[/b]\n", Translate("New messages: "), pdata->nrMerged + 1);

		ZeroMemory(&dbe, sizeof(dbe));
		dbe.cbSize = sizeof(dbe);
        if(pdata->pluginOptions->bPreview && hContact) {
            dbe.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hEvent, 0);
            dbe.pBlob = (PBYTE)malloc(dbe.cbBlob);
		}
        CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbe);
        if (pdata->pluginOptions->bShowDate || pdata->pluginOptions->bShowTime) {
            strncpy(formatTime,"",sizeof(formatTime));
            if (pdata->pluginOptions->bShowDate)
                strncpy(formatTime, "%Y.%m.%d ", sizeof(formatTime));
            if (pdata->pluginOptions->bShowTime)
                strncat(formatTime, "%H:%M", sizeof(formatTime));
            strftime(timestamp,sizeof(timestamp), formatTime, localtime((time_t *)&dbe.timestamp));
            mir_snprintf(pdata->eventData[pdata->nrMerged].szText, MAX_SECONDLINE, "\n[b][i]%s[/i][/b]\n", timestamp);
        }
        strncat(pdata->eventData[pdata->nrMerged].szText, GetPreview(dbe.eventType, (char *)dbe.pBlob), MAX_SECONDLINE);
        pdata->eventData[pdata->nrMerged].szText[MAX_SECONDLINE - 1] = 0;

        /*
         * now, reassemble the popup text, make sure the *last* event is shown, and then show the most recent events
         * for which there is enough space in the popup text
         */

        available = MAX_SECONDLINE - 1;
        if(pdata->pluginOptions->bShowHeaders) {
            strncpy(lpzText, pdata->szHeader, MAX_SECONDLINE);
            available -= lstrlenA(pdata->szHeader);
        }
        for(i = pdata->nrMerged; i >= 0; i--) {
            available -= lstrlenA(pdata->eventData[i].szText);
            if(available <= 0)
                break;
        }
        i = (available > 0) ? i + 1: i + 2;
        for(; i <= pdata->nrMerged; i++) {
            strncat(lpzText, pdata->eventData[i].szText, MAX_SECONDLINE);
        }
        pdata->eventData[pdata->nrMerged].hEvent = hEvent;
        pdata->eventData[pdata->nrMerged].timestamp = dbe.timestamp;
        pdata->nrMerged++;
        if(pdata->nrMerged >= pdata->nrEventsAlloced) {
            pdata->nrEventsAlloced += 5;
            pdata->eventData = (EVENT_DATA *)realloc(pdata->eventData, pdata->nrEventsAlloced * sizeof(EVENT_DATA));
        }
        if(dbe.pBlob)
            free(dbe.pBlob);
        
        SendMessage(pdata->hWnd, WM_SETREDRAW, FALSE, 0);
        CallService(MS_POPUP_CHANGETEXT, (WPARAM)pdata->hWnd, (LPARAM)lpzText);
        SendMessage(pdata->hWnd, WM_SETREDRAW, TRUE, 0);
    }
    return 0;
}

static int PopupAct(HWND hWnd, UINT mask, PLUGIN_DATA* pdata)
{
    pdata->iActionTaken = TRUE;
    if (mask & MASK_OPEN) {
        int i;

        for(i = 0; i < pdata->nrMerged; i++) {
            if(pdata->eventData[i].hEvent != 0) {
                PostMessage(myGlobals.g_hwndHotkeyHandler, DM_HANDLECLISTEVENT, (WPARAM)pdata->hContact, (LPARAM)pdata->eventData[i].hEvent);
                pdata->eventData[i].hEvent = 0;
            }
        }
    }
    if (mask & MASK_REMOVE) {
        int i;
        
        for(i = 0; i < pdata->nrMerged; i++) {
            if(pdata->eventData[i].hEvent != 0) {
                PostMessage(myGlobals.g_hwndHotkeyHandler, DM_REMOVECLISTEVENT, (WPARAM)pdata->hContact, (LPARAM)pdata->eventData[i].hEvent);
                pdata->eventData[i].hEvent = 0;
            }
        }
        PopUpList[NumberPopupData(pdata->hContact)] = NULL;
    }
    if (mask & MASK_DISMISS) {
        PopUpList[NumberPopupData(pdata->hContact)] = NULL;
        PUDeletePopUp(hWnd);
    }
    return 0;
}

static BOOL CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PLUGIN_DATA* pdata = NULL;

    pdata = (PLUGIN_DATA *)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd, (LPARAM)pdata);
    if (!pdata) return FALSE;

    switch (message) {
        case WM_COMMAND:
            PopupAct(hWnd, pdata->pluginOptions->maskActL, pdata);
            break;
        case WM_CONTEXTMENU:
            PopupAct(hWnd, pdata->pluginOptions->maskActR, pdata);
            break;
        case UM_FREEPLUGINDATA:
            PopUpList[NumberPopupData(pdata->hContact)] = NULL;
            PopupCount--;
            if(pdata->eventData)
                free(pdata->eventData);
            free(pdata);
            return TRUE;
        case UM_INITPOPUP:
            pdata->hWnd = hWnd;
            if (pdata->iSeconds > 0)
                SetTimer(hWnd, TIMER_TO_ACTION, pdata->iSeconds * 1000, NULL);
            break;
        case WM_MOUSEWHEEL:
            break;
        case WM_SETCURSOR:
            break;
        case WM_TIMER:
            if (wParam != TIMER_TO_ACTION)
                break;
            if (pdata->iSeconds != -1)
                KillTimer(hWnd, TIMER_TO_ACTION);
            PopupAct(hWnd, pdata->pluginOptions->maskActTE, pdata);
            break;
        default:
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static int PopupShow(NEN_OPTIONS *pluginOptions, HANDLE hContact, HANDLE hEvent, UINT eventType)
{
    POPUPDATAEX pud;
    PLUGIN_DATA* pdata;
    DBEVENTINFO dbe;
    char* sampleEvent;
    long iSeconds = 0;
    int iPreviewLimit = nen_options.iLimitPreview;
    
    //there has to be a maximum number of popups shown at the same time
    
    if (PopupCount >= MAX_POPUPS)
        return 2;

    if (!myGlobals.g_PopupAvail)
        return 0;
    
    //check if we should report this kind of event
    //get the prefered icon as well
    //CHANGE: iSeconds is -1 because I use my timer to hide popup
    
    switch (eventType) {
        case EVENTTYPE_MESSAGE:
            if (!(pluginOptions->maskNotify&MASK_MESSAGE)) return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
            pud.colorBack = pluginOptions->bDefaultColorMsg ? 0 : pluginOptions->colBackMsg;
            pud.colorText = pluginOptions->bDefaultColorMsg ? 0 : pluginOptions->colTextMsg;
            iSeconds = pluginOptions->iDelayMsg;
            sampleEvent = Translate("This is a sample message event :-)");
            break;
        case EVENTTYPE_URL:
            if (!(pluginOptions->maskNotify&MASK_URL)) return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_URL);
            pud.colorBack = pluginOptions->bDefaultColorUrl ? 0 : pluginOptions->colBackUrl;
            pud.colorText = pluginOptions->bDefaultColorUrl ? 0 : pluginOptions->colTextUrl;
            iSeconds = pluginOptions->iDelayUrl;
            sampleEvent = Translate("This is a sample URL event ;-)");
            break;
        case EVENTTYPE_FILE:
            if (!(pluginOptions->maskNotify&MASK_FILE)) return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_FILE);
            pud.colorBack = pluginOptions->bDefaultColorFile ? 0 : pluginOptions->colBackFile;
            pud.colorText = pluginOptions->bDefaultColorFile ? 0 : pluginOptions->colTextFile;
            iSeconds = pluginOptions->iDelayFile;
            sampleEvent = Translate("This is a sample file event :-D");
            break;
        default:
            if (!(pluginOptions->maskNotify&MASK_OTHER)) return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
            pud.colorBack = pluginOptions->bDefaultColorOthers ? 0 : pluginOptions->colBackOthers;
            pud.colorText = pluginOptions->bDefaultColorOthers ? 0 : pluginOptions->colTextOthers;
            iSeconds = pluginOptions->iDelayOthers;
            sampleEvent = Translate("This is a sample other event ;-D");
            break;
    }

    dbe.pBlob = NULL;
    dbe.cbSize = sizeof(dbe);
    
    // fix for a crash
    
    if (hEvent && (pluginOptions->bPreview || hContact == 0)) {
        dbe.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hEvent, 0);
        dbe.pBlob = (PBYTE)malloc(dbe.cbBlob);
    }
    else
        dbe.cbBlob = 0;
    CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbe);

    pdata = (PLUGIN_DATA*)malloc(sizeof(PLUGIN_DATA));
    ZeroMemory((void *)pdata, sizeof(PLUGIN_DATA));
    
    pdata->eventType = eventType;
    pdata->hContact = hContact;
    pdata->pluginOptions = pluginOptions;
    pdata->pud = &pud;
    pdata->iSeconds = iSeconds; // ? iSeconds : pluginOptions->iDelayDefault;
    pud.iSeconds = pdata->iSeconds ? -1 : 0;

    //finally create the popup
    pud.lchContact = hContact;
    pud.PluginWindowProc = (WNDPROC)PopupDlgProc;
    pud.PluginData = pdata;

    if(hContact == 0 && (eventType == EVENTTYPE_MESSAGE || eventType == EVENTTYPE_FILE || eventType == EVENTTYPE_URL || eventType == -1)) {
        strncpy(pud.lpzContactName, "Plugin Test", MAX_CONTACTNAME);
        strncpy(pud.lpzText, sampleEvent, MAX_SECONDLINE);
    }
    else {
        if (hContact) 
            mir_snprintf(pud.lpzContactName, MAX_CONTACTNAME, "%s", (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
        else
            mir_snprintf(pud.lpzContactName, MAX_CONTACTNAME, "%s", dbe.szModule);

        mir_snprintf(pud.lpzText, MAX_SECONDLINE, "%s", GetPreview(eventType, (char *)dbe.pBlob));
    }
    if(iPreviewLimit > 4 && iPreviewLimit < lstrlenA(pud.lpzText)) {
        iPreviewLimit = iPreviewLimit <= MAX_SECONDLINE ? iPreviewLimit : MAX_SECONDLINE;
        strncpy(&pud.lpzText[iPreviewLimit - 4], "...", 3);
        pud.lpzText[iPreviewLimit -1] = 0;
    }
    pdata->eventData = (EVENT_DATA *)malloc(NR_MERGED * sizeof(EVENT_DATA));
    pdata->eventData[0].hEvent = hEvent;
    pdata->eventData[0].timestamp = dbe.timestamp;
    strncpy(pdata->eventData[0].szText, pud.lpzText, MAX_SECONDLINE);
    pdata->eventData[0].szText[MAX_SECONDLINE - 1] = 0;
    pdata->nrEventsAlloced = NR_MERGED;
    pdata->nrMerged = 1;
    PopupCount++;

    PopUpList[NumberPopupData(NULL)] = pdata;

    if (CallService(MS_POPUP_ADDPOPUPEX, (WPARAM)&pud, 0) < 0) {
    	  // failed to display, perform cleanup
        PopUpList[NumberPopupData(pdata->hContact)] = NULL;
        PopupCount--;
        if(pdata->eventData)
            free(pdata->eventData);
        free(pdata);
    }

	// nullbie: end of fix

    if (dbe.pBlob)
        free(dbe.pBlob);

    return 0;
}

#if defined(_UNICODE)

static char *GetPreviewW(UINT eventType, DBEVENTINFO* dbe, BOOL *isWstring)
{
    char* comment1 = NULL;
    char* comment2 = NULL;
    char* commentFix = NULL;
    static char szPreviewHelp[2048];
	 char* pBlob = dbe->pBlob;

    *isWstring = 0;
    
    //now get text
    switch (eventType) {
        case EVENTTYPE_MESSAGE:
			   if ( pBlob && ServiceExists( MS_DB_EVENT_GETTEXT )) {
					WCHAR* buf = DbGetEventTextW( dbe, CP_ACP );
					wcsncpy(( WCHAR* )szPreviewHelp, buf, sizeof(szPreviewHelp) / sizeof(WCHAR));
					mir_free( buf );
               *isWstring = 1;
					return (char *)szPreviewHelp;
				}

            if (pBlob) {
                int msglen = lstrlenA((char *) pBlob) + 1;
                wchar_t *msg;
                int wlen;
                
                if ((dbe->cbBlob >= (DWORD)(2 * msglen))) {
                    msg = (wchar_t *) &pBlob[msglen];
                    wlen = safe_wcslen(msg, (dbe->cbBlob - msglen) / 2);
                    if(wlen <= (msglen - 1) && wlen > 0){
                        *isWstring = 1;
                        return (char *)msg;
                    }
                    else
                        goto nounicode;
                }
                else {
nounicode:
                    *isWstring = 0;
                    return (char *)pBlob;
                }
            }
            commentFix = Translate(POPUP_COMMENT_MESSAGE);
            break;
        case EVENTTYPE_AUTHREQUEST:
            if(pBlob) {
                mir_snprintf(szPreviewHelp, 256, Translate("%s requested authorization"), pBlob + 8);
                return szPreviewHelp;
            }
            commentFix = Translate(POPUP_COMMENT_AUTH);
            break;
        case EVENTTYPE_ADDED:
            if(pBlob) {
                mir_snprintf(szPreviewHelp, 256, Translate("%s added you to the contact list"), pBlob + 8);
                return szPreviewHelp;
            }
            commentFix = Translate(POPUP_COMMENT_ADDED);
            break;
        case EVENTTYPE_URL:
            if (pBlob) comment2 = pBlob;
            if (pBlob) comment1 = pBlob + strlen(comment2) + 1;
            commentFix = Translate(POPUP_COMMENT_URL);
            break;

        case EVENTTYPE_FILE:
            if (pBlob) comment2 = pBlob + 4;
            if (pBlob) comment1 = pBlob + strlen(comment2) + 5;
            commentFix = Translate(POPUP_COMMENT_FILE);
            break;

        case EVENTTYPE_CONTACTS:
            commentFix = Translate(POPUP_COMMENT_CONTACTS);
            break;
        case ICQEVENTTYPE_WEBPAGER:
            if (pBlob) comment1 = pBlob;
            commentFix = Translate(POPUP_COMMENT_WEBPAGER);
            break;
        case ICQEVENTTYPE_EMAILEXPRESS:
            if (pBlob) comment1 = pBlob;
            commentFix = Translate(POPUP_COMMENT_EMAILEXP);
            break;

        default:
            commentFix = Translate(POPUP_COMMENT_OTHER);
            break;
    }

    if (comment1)
        if (strlen(comment1) > 0)
            return comment1;
    if (comment2)
        if (strlen(comment2) > 0)
            return comment2;

    return commentFix;
}

static int PopupUpdateW(HANDLE hContact, HANDLE hEvent)
{
    PLUGIN_DATAW *pdata;
    DBEVENTINFO dbe;
    wchar_t lpzText[MAX_SECONDLINE] = L"";
    wchar_t timestamp[MAX_DATASIZE] = L"";
    wchar_t formatTime[MAX_DATASIZE] = L"";
    int iEvent = 0;
    wchar_t *p = lpzText;
    int  available = 0, i;
    char szHeader[256], *szPreview = NULL;
    BOOL isUnicode = 0;
    DWORD codePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", myGlobals.m_LangPackCP);
    
    pdata = (PLUGIN_DATAW *)PopUpList[NumberPopupData(hContact)];
    
    ZeroMemory((void *)&dbe, sizeof(dbe));
    
    if (hEvent) {
        if(pdata->pluginOptions->bShowHeaders) {
            mir_snprintf(szHeader, sizeof(szHeader), "[b]%s %d[/b]\n", Translate("New messages: "), pdata->nrMerged + 1);
            MultiByteToWideChar(myGlobals.m_LangPackCP, 0, szHeader, -1, pdata->szHeader, 256);
            pdata->szHeader[255] = 0;
        }
		ZeroMemory(&dbe, sizeof(dbe));
		dbe.cbSize = sizeof(dbe);
        if(pdata->pluginOptions->bPreview && hContact) {
            dbe.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hEvent, 0);
            dbe.pBlob = (PBYTE)malloc(dbe.cbBlob);
		}
        CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbe);
        if (pdata->pluginOptions->bShowDate || pdata->pluginOptions->bShowTime) {
            formatTime[0] = 0;
            if (pdata->pluginOptions->bShowDate)
                wcsncpy(formatTime, L"%Y.%m.%d ", MAX_DATASIZE);
            if (pdata->pluginOptions->bShowTime)
                wcsncat(formatTime, L"%H:%M", MAX_DATASIZE);
            wcsftime(timestamp, MAX_DATASIZE, formatTime, localtime((time_t *)&dbe.timestamp));
            mir_snprintfW(pdata->eventData[pdata->nrMerged].szText, MAX_SECONDLINE, L"\n[b][i]%s[/i][/b]\n", timestamp);
        }
        szPreview = GetPreviewW(dbe.eventType, &dbe, &isUnicode);
        if(szPreview) {
            if(isUnicode)
                wcsncat(pdata->eventData[pdata->nrMerged].szText, (wchar_t *)szPreview, MAX_SECONDLINE);
            else {
                wchar_t temp[MAX_SECONDLINE];
                
                MultiByteToWideChar(codePage, 0, szPreview, -1, temp, MAX_SECONDLINE);
                temp[MAX_SECONDLINE - 1] = 0;
                wcsncat(pdata->eventData[pdata->nrMerged].szText, temp, MAX_SECONDLINE);
            }
        }
        else
            wcsncat(pdata->eventData[pdata->nrMerged].szText, L"No body", MAX_SECONDLINE);

        pdata->eventData[pdata->nrMerged].szText[MAX_SECONDLINE - 1] = 0;

        /*
         * now, reassemble the popup text, make sure the *last* event is shown, and then show the most recent events
         * for which there is enough space in the popup text
         */

        available = MAX_SECONDLINE - 1;
        if(pdata->pluginOptions->bShowHeaders) {
            wcsncpy(lpzText, pdata->szHeader, MAX_SECONDLINE);
            available -= lstrlenW(pdata->szHeader);
        }
        for(i = pdata->nrMerged; i >= 0; i--) {
            available -= lstrlenW(pdata->eventData[i].szText);
            if(available <= 0)
                break;
        }
        i = (available > 0) ? i + 1: i + 2;
        for(; i <= pdata->nrMerged; i++) {
            wcsncat(lpzText, pdata->eventData[i].szText, MAX_SECONDLINE);
        }
        pdata->eventData[pdata->nrMerged].hEvent = hEvent;
        pdata->eventData[pdata->nrMerged].timestamp = dbe.timestamp;
        pdata->nrMerged++;
        if(pdata->nrMerged >= pdata->nrEventsAlloced) {
            pdata->nrEventsAlloced += 5;
            pdata->eventData = (EVENT_DATAW *)realloc(pdata->eventData, pdata->nrEventsAlloced * sizeof(EVENT_DATAW));
        }
        if(dbe.pBlob)
            free(dbe.pBlob);
        
        SendMessage(pdata->hWnd, WM_SETREDRAW, FALSE, 0);
        CallService(MS_POPUP_CHANGETEXTW, (WPARAM)pdata->hWnd, (LPARAM)lpzText);
        SendMessage(pdata->hWnd, WM_SETREDRAW, TRUE, 0);
    }
    return 0;
}

static int PopupActW(HWND hWnd, UINT mask, PLUGIN_DATAW* pdata)
{
    pdata->iActionTaken = TRUE;
    if (mask & MASK_OPEN) {
        int i;

        for(i = 0; i < pdata->nrMerged; i++)
            PostMessage(myGlobals.g_hwndHotkeyHandler, DM_HANDLECLISTEVENT, (WPARAM)pdata->hContact, (LPARAM)pdata->eventData[i].hEvent);
    }
    if (mask & MASK_REMOVE) {
        int i;
        
        for(i = 0; i < pdata->nrMerged; i++)
            PostMessage(myGlobals.g_hwndHotkeyHandler, DM_REMOVECLISTEVENT, (WPARAM)pdata->hContact, (LPARAM)pdata->eventData[i].hEvent);
        PopUpList[NumberPopupData(pdata->hContact)] = NULL;
    }
    if (mask & MASK_DISMISS) {
        PopUpList[NumberPopupData(pdata->hContact)] = NULL;
        PUDeletePopUp(hWnd);
    }
    return 0;
}

static BOOL CALLBACK PopupDlgProcW(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PLUGIN_DATAW* pdata = NULL;

    pdata = (PLUGIN_DATAW *)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd, (LPARAM)pdata);
    if (!pdata) return FALSE;

    switch (message) {
        case WM_COMMAND:
            PopupActW(hWnd, pdata->pluginOptions->maskActL, pdata);
            break;
        case WM_CONTEXTMENU:
            PopupActW(hWnd, pdata->pluginOptions->maskActR, pdata);
            break;
        case UM_FREEPLUGINDATA:
            PopUpList[NumberPopupData(pdata->hContact)] = NULL;
            PopupCount--;
            if(pdata->eventData)
                free(pdata->eventData);
            free(pdata);
            return TRUE;
        case UM_INITPOPUP:
            pdata->hWnd = hWnd;
            if (pdata->iSeconds > 0)
                SetTimer(hWnd, TIMER_TO_ACTION, pdata->iSeconds * 1000, NULL);
            break;
        case WM_MOUSEWHEEL:
            break;
        case WM_SETCURSOR:
            SetFocus(hWnd);
            break;
        case WM_TIMER:
            if (wParam != TIMER_TO_ACTION)
                break;
            if (pdata->iSeconds > 0)
                KillTimer(hWnd, TIMER_TO_ACTION);
            PopupActW(hWnd, pdata->pluginOptions->maskActTE, pdata);
            break;
        default:
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static int PopupShowW(NEN_OPTIONS *pluginOptions, HANDLE hContact, HANDLE hEvent, UINT eventType)
{
    POPUPDATAW pud;
    PLUGIN_DATAW *pdata;
    DBEVENTINFO dbe;
    long iSeconds = 0;
    int iPreviewLimit = nen_options.iLimitPreview;
    BOOL isUnicode = 0;
    char *szPreview = NULL;
    DWORD codePage;
    
    //there has to be a maximum number of popups shown at the same time
    if (PopupCount >= MAX_POPUPS)
        return 2;

    if (!myGlobals.g_PopupAvail)
        return 0;
    
    //check if we should report this kind of event
    //get the prefered icon as well
    //CHANGE: iSeconds is -1 because I use my timer to hide popup
    switch (eventType) {
        case EVENTTYPE_MESSAGE:
            if (!pluginOptions->bSimpleMode && !(pluginOptions->maskNotify&MASK_MESSAGE)) 
				return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
            pud.colorBack = pluginOptions->bDefaultColorMsg ? 0 : pluginOptions->colBackMsg;
            pud.colorText = pluginOptions->bDefaultColorMsg ? 0 : pluginOptions->colTextMsg;
            iSeconds = pluginOptions->iDelayMsg;
            break;
        case EVENTTYPE_URL:
            if (!pluginOptions->bSimpleMode && !(pluginOptions->maskNotify&MASK_URL)) 
				return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_URL);
            pud.colorBack = pluginOptions->bDefaultColorUrl ? 0 : pluginOptions->colBackUrl;
            pud.colorText = pluginOptions->bDefaultColorUrl ? 0 : pluginOptions->colTextUrl;
            iSeconds = pluginOptions->iDelayUrl;
            break;
        case EVENTTYPE_FILE:
            if (!pluginOptions->bSimpleMode && !(pluginOptions->maskNotify&MASK_FILE)) 
				return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_FILE);
            pud.colorBack = pluginOptions->bDefaultColorFile ? 0 : pluginOptions->colBackFile;
            pud.colorText = pluginOptions->bDefaultColorFile ? 0 : pluginOptions->colTextFile;
            iSeconds = pluginOptions->iDelayFile;
            break;
        default:
            if (!pluginOptions->bSimpleMode && !(pluginOptions->maskNotify&MASK_OTHER)) 
				return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
            pud.colorBack = pluginOptions->bDefaultColorOthers ? 0 : pluginOptions->colBackOthers;
            pud.colorText = pluginOptions->bDefaultColorOthers ? 0 : pluginOptions->colTextOthers;
            iSeconds = pluginOptions->iDelayOthers;
            break;
    }

    dbe.pBlob = NULL;
    dbe.cbSize = sizeof(dbe);

    // fix for a crash
    if (hEvent && (pluginOptions->bPreview || hContact == 0)) {
        dbe.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hEvent, 0);
        dbe.pBlob = (PBYTE)malloc(dbe.cbBlob);
    }
    else
        dbe.cbBlob = 0;
    CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbe);

    pdata = (PLUGIN_DATAW *)malloc(sizeof(PLUGIN_DATAW));
    ZeroMemory((void *)pdata, sizeof(PLUGIN_DATAW));
    
    pdata->eventType = eventType;
    pdata->hContact = hContact;
    pdata->pluginOptions = pluginOptions;
    pdata->pud = &pud;
    pdata->iSeconds = iSeconds; // ? iSeconds : pluginOptions->iDelayDefault;
    pud.iSeconds = pdata->iSeconds ? -1 : 0;

    //finally create the popup
    pud.lchContact = hContact;
    pud.PluginWindowProc = (WNDPROC)PopupDlgProcW;
    pud.PluginData = pdata;

    codePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", myGlobals.m_LangPackCP);
    
    if (hContact) {
        MY_GetContactDisplayNameW(hContact, pud.lpwzContactName, MAX_CONTACTNAME, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), 0);
        pud.lpwzContactName[MAX_CONTACTNAME - 1] = 0;
    }
    else {
        MultiByteToWideChar(myGlobals.m_LangPackCP, 0, dbe.szModule, -1, pud.lpwzContactName, MAX_CONTACTNAME);
        pud.lpwzContactName[MAX_CONTACTNAME - 1] = 0;
    }

    szPreview = GetPreviewW(eventType, &dbe, &isUnicode);
    if(szPreview) {
        if(isUnicode)
            mir_snprintfW(pud.lpwzText, MAX_SECONDLINE, L"%s", (wchar_t *)szPreview);
        else {
            MultiByteToWideChar(codePage, 0, szPreview, -1, pud.lpwzText, MAX_SECONDLINE);
            pud.lpwzText[MAX_SECONDLINE - 1] = 0;
        }
    }
    else
        wcsncpy(pud.lpwzText, L"No body", MAX_SECONDLINE);
        
    if(iPreviewLimit > 4 && iPreviewLimit < lstrlenW(pud.lpwzText)) {
        iPreviewLimit = iPreviewLimit <= MAX_SECONDLINE ? iPreviewLimit : MAX_SECONDLINE;
        wcsncpy(&pud.lpwzText[iPreviewLimit - 4], L"...", 3);
        pud.lpwzText[iPreviewLimit -1] = 0;
    }

    pdata->eventData = (EVENT_DATAW *)malloc(NR_MERGED * sizeof(EVENT_DATAW));
    pdata->eventData[0].hEvent = hEvent;
    pdata->eventData[0].timestamp = dbe.timestamp;
    wcsncpy(pdata->eventData[0].szText, pud.lpwzText, MAX_SECONDLINE);
    pdata->eventData[0].szText[MAX_SECONDLINE - 1] = 0;
    pdata->nrEventsAlloced = NR_MERGED;
    pdata->nrMerged = 1;
    
    PopupCount++;

    PopUpList[NumberPopupData(NULL)] = (PLUGIN_DATA *)pdata;

    // fix for broken popups -- process failures
    if (CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&pud, 0) < 0) {
        // failed to display, perform cleanup
        PopUpList[NumberPopupData(pdata->hContact)] = NULL;
        PopupCount--;
        if(pdata->eventData)
           free(pdata->eventData);
        free(pdata);
    }

    if (dbe.pBlob)
        free(dbe.pBlob);

    return 0;
}

#endif

static int PopupPreview(NEN_OPTIONS *pluginOptions)
{
    PopupShow(pluginOptions, NULL, NULL, EVENTTYPE_MESSAGE);
    PopupShow(pluginOptions, NULL, NULL, EVENTTYPE_URL);
    PopupShow(pluginOptions, NULL, NULL, EVENTTYPE_FILE);
    PopupShow(pluginOptions, NULL, NULL, (DWORD)-1);

    return 0;
}

/*
 * announce via either balloon tooltip or OSD plugin service.
 * wParam = hContact
 * lParam = handle of the database event to announce
 * truncates the announce string at 255 characters (balloon tooltip limitation)
 */

static int tabSRMM_ShowBalloon(WPARAM wParam, LPARAM lParam, UINT eventType)
{
    DBEVENTINFO dbei = {0};
    char *szPreview;
    NOTIFYICONDATA nim;
    char szTitle[64], *nickName = NULL;
    int iPreviewLimit = nen_options.iLimitPreview;

    if(iPreviewLimit > 255 || iPreviewLimit == 0)
        iPreviewLimit = 255;
    
    ZeroMemory((void *)&nim, sizeof(nim));
    nim.cbSize = sizeof(nim);

    nim.hWnd = myGlobals.g_hwndHotkeyHandler;
    nim.uID = 100;
    nim.uFlags = NIF_ICON | NIF_INFO;
    nim.hIcon = myGlobals.g_iconContainer;
    nim.uCallbackMessage = DM_TRAYICONNOTIFY;
    nim.uTimeout = 10000;
    nim.dwInfoFlags = NIIF_INFO;
    
    dbei.cbSize = sizeof(dbei);
    dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM) lParam, 0);
    if (dbei.cbBlob == -1)
        return 0;
    dbei.pBlob = (PBYTE) malloc(dbei.cbBlob);
    CallService(MS_DB_EVENT_GET, (WPARAM) lParam, (LPARAM) & dbei);
    szPreview = GetPreview(eventType, (char *)dbei.pBlob);
    nickName = (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)wParam, 0);
    if(nickName) {
        if(lstrlenA(nickName) >= 30)
            mir_snprintf(szTitle, 64, "%27.27s...", nickName);
        else
            mir_snprintf(szTitle, 64, "%s", nickName);
    }
    else
        mir_snprintf(szTitle, 64, "No Nickname");

#if defined(_UNICODE)
    MultiByteToWideChar(myGlobals.m_LangPackCP, 0, szTitle, -1, nim.szInfoTitle, 64);
#else
    strcpy(nim.szInfoTitle, szTitle);
#endif
    
    if(eventType == EVENTTYPE_MESSAGE) {
#if defined(_UNICODE)
        int msglen = lstrlenA((char *)dbei.pBlob) + 1;
        wchar_t *msg;
        int wlen;
        
        if (dbei.cbBlob >= (DWORD)(2 * msglen))  {
            msg = (wchar_t *) &dbei.pBlob[msglen];
            wlen = safe_wcslen(msg, (dbei.cbBlob - msglen) / 2);
            if(wlen <= (msglen - 1) && wlen > 0) {
                if(lstrlenW(msg) >= iPreviewLimit) {
                    wcsncpy(&msg[iPreviewLimit - 3], L"...", 3);
                    msg[iPreviewLimit] = 0;
                }
            }
            else
                goto nounicode;
        }
        else {
nounicode:
            msg = (wchar_t *)alloca(2 * (msglen + 1));
            MultiByteToWideChar(myGlobals.m_LangPackCP, 0, (char *)dbei.pBlob, -1, msg, msglen);
            if(lstrlenW(msg) >= iPreviewLimit) {
                wcsncpy(&msg[iPreviewLimit - 3], L"...", 3);
                msg[iPreviewLimit] = 0;
            }
        }
        wcsncpy(nim.szInfo, msg, 256);
        nim.szInfo[255] = 0;
#else
        if(lstrlenA((char *)dbei.pBlob) >= iPreviewLimit) {
            strncpy((char *)&dbei.pBlob[iPreviewLimit - 3], "...", 3);
            dbei.pBlob[iPreviewLimit] = 0;
        }
        strncpy(nim.szInfo, (char *)dbei.pBlob, 256);
        nim.szInfo[255] = 0;
#endif        
    }
    else {
#if defined(_UNICODE)
        MultiByteToWideChar(myGlobals.m_LangPackCP, 0, (char *)szPreview, -1, nim.szInfo, 250);
#else
        strncpy(nim.szInfo, (char *)dbei.pBlob, 250);
#endif        
        nim.szInfo[250] = 0;
    }
    if(nen_options.iAnnounceMethod == 3) {                          // announce via OSD service
        int iLen = _tcslen(nim.szInfo) + _tcslen(nim.szInfoTitle) + 30;
        TCHAR *finalOSDString = malloc(iLen * sizeof(TCHAR));

        mir_sntprintf(finalOSDString, iLen, TranslateT("Message from %s: %s"), nim.szInfoTitle, nim.szInfo);
        CallService("OSD/Announce", (WPARAM)finalOSDString, 0);
        free(finalOSDString);
    }
    else {
        myGlobals.m_TipOwner = (HANDLE)wParam;
        Shell_NotifyIcon(NIM_MODIFY, (NOTIFYICONDATA *)&nim);
    }
    if(dbei.pBlob)
        free(dbei.pBlob);
    return 0;
}

/*
 * updates the menu entry...
 * bForced is used to only update the status, nickname etc. and does NOT update the unread count
 */
void UpdateTrayMenuState(struct MessageWindowData *dat, BOOL bForced)
{
    MENUITEMINFO mii = {0};
    char szMenuEntry[80];
#if defined(_UNICODE)
    const wchar_t *szMenuEntryW;
#endif    
    if(myGlobals.g_hMenuTrayUnread == 0)
        return;
    
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA | MIIM_BITMAP;
    
    if(dat->hContact != 0) {
        GetMenuItemInfo(myGlobals.g_hMenuTrayUnread, (UINT_PTR)dat->hContact, FALSE, &mii);
        if(!bForced)
            myGlobals.m_UnreadInTray -= (mii.dwItemData & 0x0000ffff);
        if(mii.dwItemData > 0 || bForced) {
            if(!bForced)
                mii.dwItemData = 0;
            mii.fMask |= MIIM_STRING;
#if defined(_UNICODE)
			mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s) [%d]", dat->bIsMeta ? dat->szMetaProto : dat->szProto, "%nick%", dat->szStatus[0] ? dat->szStatus : "(undef)", mii.dwItemData & 0x0000ffff);
            szMenuEntryW = EncodeWithNickname(szMenuEntry, dat->szNickname, myGlobals.m_LangPackCP);
            mii.dwTypeData = (LPWSTR)szMenuEntryW;
            mii.cch = lstrlenW(szMenuEntryW) + 1;
#else
			mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s) [%d]", dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->szNickname, dat->szStatus[0] ? dat->szStatus : "(undef)", mii.dwItemData & 0x0000ffff);
            mii.dwTypeData = szMenuEntry;
            mii.cch = lstrlenA(szMenuEntry) + 1;
#endif            
        }
        mii.hbmpItem = HBMMENU_CALLBACK;
        SetMenuItemInfo(myGlobals.g_hMenuTrayUnread, (UINT_PTR)dat->hContact, FALSE, &mii);
    }
}
/*
 * if we want tray support, add the contact to the list of unread sessions in the tray menu
 */

int UpdateTrayMenu(struct MessageWindowData *dat, WORD wStatus, char *szProto, char *szStatus, HANDLE hContact, DWORD fromEvent)
{
    if(myGlobals.g_hMenuTrayUnread != 0 && hContact != 0 && szProto != NULL) {
        char szMenuEntry[80];
#if defined(_UNICODE)
        wchar_t *szMenuEntryW = NULL, szWNick[102];
#endif        
        MENUITEMINFO mii = {0};
        WORD wMyStatus;
        char *szMyStatus;
        TCHAR *szNick = NULL;
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA | MIIM_ID | MIIM_BITMAP;

        if(szProto == NULL)
            return 0;                                     // should never happen...
            
        wMyStatus = (wStatus == 0) ? DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE) : wStatus;
        szMyStatus = (szStatus == NULL) ? (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)wMyStatus, 0) : szStatus;
        mii.wID = (UINT)hContact;
        mii.hbmpItem = HBMMENU_CALLBACK;
        
        if(dat != 0) {
            szNick = dat->szNickname;
            GetMenuItemInfo(myGlobals.g_hMenuTrayUnread, (UINT_PTR)hContact, FALSE, &mii);
            mii.dwItemData++;
            if(fromEvent == 2)                          // from chat...
                mii.dwItemData |= 0x10000000;
            DeleteMenu(myGlobals.g_hMenuTrayUnread, (UINT_PTR)hContact, MF_BYCOMMAND);
#if defined(_UNICODE)
            mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s) [%d]", szProto, "%nick%", szMyStatus, mii.dwItemData & 0x0000ffff);
            szMenuEntryW = (WCHAR *)EncodeWithNickname((const char *)szMenuEntry, (const wchar_t *)szNick, myGlobals.m_LangPackCP);
            AppendMenuW(myGlobals.g_hMenuTrayUnread, MF_BYCOMMAND | MF_STRING, (UINT_PTR)hContact, szMenuEntryW);
#else
            mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s) [%d]", szProto, szNick, szMyStatus, mii.dwItemData & 0x0000ffff);
            AppendMenuA(myGlobals.g_hMenuTrayUnread, MF_BYCOMMAND | MF_STRING, (UINT_PTR)hContact, szMenuEntry);
#endif            
            myGlobals.m_UnreadInTray++;
            if(myGlobals.m_UnreadInTray)
                SetEvent(g_hEvent);
            SetMenuItemInfo(myGlobals.g_hMenuTrayUnread, (UINT_PTR)hContact, FALSE, &mii);
        }
        else {
            UINT codePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", myGlobals.m_LangPackCP);
#if defined(_UNICODE)
            MY_GetContactDisplayNameW(hContact, szWNick, 100, (const char *)szProto, codePage);
            szNick = szWNick;
#else
            szNick = (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0);
#endif
            if(CheckMenuItem(myGlobals.g_hMenuTrayUnread, (UINT_PTR)hContact, MF_BYCOMMAND | MF_UNCHECKED) == -1) {
#if defined(_UNICODE)
                mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s) [%d]", szProto, "%nick%", szMyStatus, fromEvent ? 1 : 0);
                szMenuEntryW = (WCHAR *)EncodeWithNickname(szMenuEntry, szNick, codePage);
                AppendMenuW(myGlobals.g_hMenuTrayUnread, MF_BYCOMMAND | MF_STRING, (UINT_PTR)hContact, szMenuEntryW);
#else
                mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s) [%d]", szProto, szNick, szMyStatus, fromEvent ? 1 : 0);
                AppendMenuA(myGlobals.g_hMenuTrayUnread, MF_BYCOMMAND | MF_STRING, (UINT_PTR)hContact, szMenuEntry);
#endif            
                mii.dwItemData = fromEvent ? 1 : 0;
                myGlobals.m_UnreadInTray += (mii.dwItemData & 0x0000ffff);
                if(myGlobals.m_UnreadInTray)
                    SetEvent(g_hEvent);
                if(fromEvent == 2)
                    mii.dwItemData |= 0x10000000;
            }
            else {
                GetMenuItemInfo(myGlobals.g_hMenuTrayUnread, (UINT_PTR)hContact, FALSE, &mii);
                mii.dwItemData += (fromEvent ? 1 : 0);
                myGlobals.m_UnreadInTray += (fromEvent ? 1 : 0);
                if(myGlobals.m_UnreadInTray)
                    SetEvent(g_hEvent);
                mii.fMask |= MIIM_STRING;
                if(fromEvent == 2)
                    mii.dwItemData |= 0x10000000;
#if defined(_UNICODE)
                mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s) [%d]", szProto, "%nick%", szMyStatus, mii.dwItemData & 0x0000ffff);
                szMenuEntryW = (WCHAR *)EncodeWithNickname(szMenuEntry, szNick, codePage);
                mii.cch = lstrlenW(szMenuEntryW) + 1;
                mii.dwTypeData = (LPWSTR)szMenuEntryW;
#else
                mir_snprintf(szMenuEntry, sizeof(szMenuEntry), "%s: %s (%s) [%d]", szProto, szNick, szMyStatus, mii.dwItemData & 0x0000ffff);
                mii.cch = lstrlenA(szMenuEntry) + 1;
                mii.dwTypeData = szMenuEntry;
#endif                
            }
            SetMenuItemInfo(myGlobals.g_hMenuTrayUnread, (UINT_PTR)hContact, FALSE, &mii);
        }
    }
    return 0;
}


int tabSRMM_ShowPopup(WPARAM wParam, LPARAM lParam, WORD eventType, int windowOpen, struct ContainerWindowData *pContainer, HWND hwndChild, char *szProto, struct MessageWindowData *dat)
{
    if(nen_options.iDisable)                          // no popups at all. Period
        return 0;
    /*
     * check the status mode against the status mask
     */

	if(nen_options.bSimpleMode) {
	    if(nen_options.bNoRSS && szProto != NULL && !strncmp(szProto, "RSS", 3))
		    return 0;
		switch(nen_options.bSimpleMode) {
			case 1:
				goto passed;
			case 3:
				if(!windowOpen)
					goto passed;
				else
					return 0;
			case 2:
				if(!windowOpen)
					goto passed;
				if(pContainer != NULL) {
					if(IsIconic(pContainer->hwnd) || GetForegroundWindow() != pContainer->hwnd)
						goto passed;
				}
				return 0;
			default:
				return 0;
		}
	}

    if(nen_options.dwStatusMask != -1) {
        DWORD dwStatus = 0;
        if(szProto != NULL) {
            dwStatus = (DWORD)CallProtoService(szProto, PS_GETSTATUS, 0, 0);
            if(!(dwStatus == 0 || dwStatus <= ID_STATUS_OFFLINE || ((1<<(dwStatus - ID_STATUS_ONLINE)) & nen_options.dwStatusMask)))              // should never happen, but...
                return 0;
        }
    }
    if(nen_options.bNoRSS && szProto != NULL && !strncmp(szProto, "RSS", 3))
        return 0;                                        // filter out RSS popups
                                                       // 
	if(windowOpen && pContainer != 0) {                // message window is open, need to check the container config if we want to see a popup nonetheless
        if(nen_options.bWindowCheck)                   // no popups at all for open windows... no exceptions
            return 0;
        if (pContainer->dwFlags & CNT_DONTREPORT && (IsIconic(pContainer->hwnd) || pContainer->bInTray))        // in tray counts as "minimised"
                goto passed;
        if (pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED) {
            if (!IsIconic(pContainer->hwnd) && GetForegroundWindow() != pContainer->hwnd && GetActiveWindow() != pContainer->hwnd)
                goto passed;
        }
        if (pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE) {
            if(pContainer->hwndActive == hwndChild)
                return 0;
            else
                goto passed;
        }
        return 0;
    }
passed:
    if((nen_options.bTraySupport && nen_options.iAnnounceMethod == 2) || nen_options.iAnnounceMethod == 3) {
        tabSRMM_ShowBalloon(wParam, lParam, (UINT)eventType);
        return 0;
    }
    if (NumberPopupData((HANDLE)wParam) != -1 && nen_options.bMergePopup && eventType == EVENTTYPE_MESSAGE) {
#if defined(_UNICODE)
        if(myGlobals.g_PopupWAvail)
            PopupUpdateW((HANDLE)wParam, (HANDLE)lParam);
        else
            PopupUpdate((HANDLE)wParam, (HANDLE)lParam);
#else
        PopupUpdate((HANDLE)wParam, (HANDLE)lParam);
#endif        
    } else {
#if defined(_UNICODE)
        if(myGlobals.g_PopupWAvail)
            PopupShowW(&nen_options, (HANDLE)wParam, (HANDLE)lParam, (UINT)eventType);
        else
            PopupShow(&nen_options, (HANDLE)wParam, (HANDLE)lParam, (UINT)eventType);
#else
        PopupShow(&nen_options, (HANDLE)wParam, (HANDLE)lParam, (UINT)eventType);
#endif    
    }
    return 0;
}

/*
 * remove all popups for hContact, but only if the mask matches the current "mode"
 */

void DeletePopupsForContact(HANDLE hContact, DWORD dwMask)
{
    int i = 0;

    if(!(dwMask & nen_options.dwRemoveMask) || nen_options.iDisable || !myGlobals.g_PopupAvail)
        return;
        
    while((i = NumberPopupData(hContact)) != -1) {
        if(PopUpList[i]->hWnd != 0 && IsWindow(PopUpList[i]->hWnd))
            PUDeletePopUp(PopUpList[i]->hWnd);
        PopUpList[i] = NULL;
    }
}
