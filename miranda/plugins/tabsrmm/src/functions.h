/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * Global function prototypes
 *
 */

#ifndef _TABSRMM_FUNCTIONS_H
#define _TABSRMM_FUNCTIONS_H

/*
 * global prototypes
 */

TCHAR *FilterEventMarkers(TCHAR *wszText);
#if defined(_UNICODE)
	char  *FilterEventMarkers(char *wszText);
#endif
const TCHAR *DoubleAmpersands(TCHAR *pszText);

/*
 * nen / event popup stuff
 */

int         NEN_ReadOptions(NEN_OPTIONS *options);
int         NEN_WriteOptions(NEN_OPTIONS *options);
int 		UpdateTrayMenu(const _MessageWindowData *dat, WORD wStatus, const char *szProto, const TCHAR *szStatus, HANDLE hContact, DWORD fromEvent);
static int  PopupPreview(NEN_OPTIONS *pluginOptions);
void        DeletePopupsForContact(HANDLE hContact, DWORD dwMask);
void        RemoveBalloonTip();

/*
 * tray stuff
 */

void        CreateSystrayIcon(int create);
void        FlashTrayIcon(HICON hIcon);
void        UpdateTrayMenuState(struct _MessageWindowData *dat, BOOL bForced);
void        LoadFavoritesAndRecent();
void        AddContactToFavorites(HANDLE hContact, TCHAR *szNickname, char *szProto, TCHAR *szStatus, WORD wStatus, HICON hIcon, BOOL mode, HMENU hMenu, UINT codePage);
void        CreateTrayMenus(int mode);
void        HandleMenuEntryFromhContact(int iSelection);

/*
 * gneric msgwindow functions (creation, container management etc.)
 */

BOOL 		IsUtfSendAvailable(HANDLE hContact);
void 		DM_NotifyTyping(_MessageWindowData *dat, int mode);
int         ActivateExistingTab(struct ContainerWindowData *pContainer, HWND hwndChild);
HWND        CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTAb, BOOL bPopupContainer, BOOL bWantPopup, HANDLE hdbEvent);
int         GetProtoIconFromList(const char *szProto, int iStatus);
static void CreateImageList(BOOL bInitial);
int         ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
void        FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iNum);
void        LoadMsgAreaBackground();
int         MY_GetContactDisplayNameW(HANDLE hContact, wchar_t *szwBuf, unsigned int size, const char *szProto, UINT codePage);
struct      ContainerWindowData *FindMatchingContainer(const TCHAR *szName, HANDLE hContact);
struct      ContainerWindowData *CreateContainer(const TCHAR *name, int iTemp, HANDLE hContactFrom);
int         CutContactName(TCHAR *oldname, TCHAR *newname, unsigned int size);
struct      ContainerWindowData *FindContainerByName(const TCHAR *name);
void        BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);
int         GetTabIndexFromHWND(HWND hwndTab, HWND hwnd);
int			GetTabItemFromMouse(HWND hwndTab, POINT *pt);
int         ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
int         GetProtoIconFromList(const char *szProto, int iStatus);
void        AdjustTabClientRect(struct ContainerWindowData *pContainer, RECT *rc);
void        FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iCount);
void        ReflashContainer(struct ContainerWindowData *pContainer);
HMENU       BuildMCProtocolMenu(HWND hwndDlg);
static struct ContainerWindowData *AppendToContainerList(struct ContainerWindowData *pContainer);
static struct ContainerWindowData *RemoveContainerFromList(struct ContainerWindowData *pContainer);
void        DeleteContainer(int iIndex);
void        RenameContainer(int iIndex, const TCHAR *newName);
int         EnumContainers(HANDLE hContact, DWORD dwAction, const TCHAR *szTarget, const TCHAR *szNew, DWORD dwExtinfo, DWORD dwExtinfoEx);
void        GetLocaleID(struct _MessageWindowData *dat, char *szKLName);
int         GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen);
HMENU       BuildContainerMenu();
void        BuildCodePageList();
void        PreTranslateDates();
void        ApplyContainerSetting(struct ContainerWindowData *pContainer, DWORD flags, UINT mode, bool fForceResize);
void        BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);
void        GetDefaultContainerTitleFormat();
extern const WCHAR *EncodeWithNickname(const char *string, const WCHAR *szNick, UINT codePage);
INT_PTR     MessageWindowOpened(WPARAM wParam, LPARAM lParam);
void 		SetAeroMargins(ContainerWindowData *pContainer);
int         TABSRMM_FireEvent(HANDLE hContact, HWND hwnd, unsigned int type, unsigned int subType);
LRESULT CALLBACK IEViewKFSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK IEViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK HPPKFSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
 * skinning engine
 */
HICON       *BTN_GetIcon(const TCHAR *szIconName);
void        DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, int alpha, DWORD basecolor2, BYTE transparent, BYTE FLG_GRADIENT, BYTE FLG_CORNER, DWORD BORDERSTYLE, CImageItem *imageItem);
// the cached message log icons

void        CacheMsgLogIcons();
void        UncacheMsgLogIcons();
void        CacheLogFonts();
static void InitAPI();
static void LoadIconTheme();
static int  LoadFromIconLib();
static  int SetupIconLibConfig();
void        RTF_CTableInit();

INT_PTR CALLBACK DlgProcTemplateHelp(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ErrorDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int         InitOptions(void);
static 		INT_PTR CALLBACK DlgProcContainer(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int         DbEventIsShown(struct _MessageWindowData *dat, DBEVENTINFO *dbei);
void        StreamInEvents(HWND hwndDlg,HANDLE hDbEventFirst,int count,int fAppend, DBEVENTINFO *dbei_s);
void        LoadLogfont(int i,LOGFONTA *lf,COLORREF *colour, char *szModule);


// custom tab control

void        ReloadTabConfig(), FreeTabConfig();
int         RegisterTabCtrlClass(void);

void        LoadIconTheme(), UnloadIconTheme();
int         Chat_OptionsInitialize(WPARAM wParam, LPARAM lParam);


// buttons

int         LoadTSButtonModule(void);
int         UnloadTSButtonModule(WPARAM wParam, LPARAM lParam);


/*
 * debugging support
 */

int         _DebugTraceW(const wchar_t *fmt, ...);
int         _DebugTraceA(const char *fmt, ...);
int         _DebugPopup(HANDLE hContact, const TCHAR *fmt, ...);
int         _DebugMessage(HWND hwndDlg, struct _MessageWindowData *dat, const char *fmt, ...);

// themes

const TCHAR *GetThemeFileName(int iMode);
static void LoadLogfontFromINI(int i, char *szKey, LOGFONTA *lf, COLORREF *colour, const char *szIniFilename);
int         CheckThemeVersion(const TCHAR *szIniFilename);
void        WriteThemeToINI(const TCHAR *szIniFilename, struct _MessageWindowData *dat);
void        ReadThemeFromINI(const TCHAR *szIniFilename, struct _MessageWindowData *dat, int noAdvanced, DWORD dwFlags);

// compatibility

// user prefs

int			LoadLocalFlags(HWND hwnd, struct _MessageWindowData *dat);

//TypingNotify
int			TN_ModuleInit();
int			TN_OptionsInitialize(WPARAM wParam, LPARAM lParam);
int			TN_ModuleDeInit();
int			TN_TypingMessage(WPARAM wParam, LPARAM lParam);

// mod plus

int			ChangeClientIconInStatusBar(WPARAM wparam, LPARAM lparam);

// hotkeys

LRESULT 	ProcessHotkeysByMsgFilter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR ctrlId);

// send laster

void 		SendLater_Add(const HANDLE hContact);
void		SendLater_Process(const HANDLE hContact);
void 		SendLater_Remove(const HANDLE hContact);
HANDLE	 	SendLater_ProcessAck(const ACKDATA *ack);
void 		SendLater_ClearAll();
int 		SendLater_SendIt(const char *szSetting, LPARAM lParam);
#endif /* _TABSRMM_FUNCTIONS_H */
