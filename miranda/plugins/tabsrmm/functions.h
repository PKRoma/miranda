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

/*
 * global prototypes
 */

/*
 * nen / event popup stuff
 */
 
int         NEN_ReadOptions(NEN_OPTIONS *options);
int         NEN_WriteOptions(NEN_OPTIONS *options);
int         UpdateTrayMenu(struct MessageWindowData *dat, WORD wStatus, char *szProto, TCHAR *szStatus, HANDLE hContact, DWORD fromEvent);
static int  PopupPreview(NEN_OPTIONS *pluginOptions);
void        DeletePopupsForContact(HANDLE hContact, DWORD dwMask);
void        RemoveBalloonTip();

/*
 * tray stuff
 */

void        CreateSystrayIcon(int create);
void        MinimiseToTray(HWND hWnd, BOOL bForceAnimation);
void        MaximiseFromTray(HWND hWnd, BOOL bForceAnimation, RECT *rc);
void        FlashTrayIcon(HICON hIcon);
void        UpdateTrayMenuState(struct MessageWindowData *dat, BOOL bForced);
void        LoadFavoritesAndRecent();
void        AddContactToFavorites(HANDLE hContact, TCHAR *szNickname, char *szProto, TCHAR *szStatus, WORD wStatus, HICON hIcon, BOOL mode, HMENU hMenu, UINT codePage);
void        CreateTrayMenus(int mode);
void        HandleMenuEntryFromhContact(int iSelection);

/*
 * gneric msgwindow functions (creation, container management etc.)
 */

int         ActivateExistingTab(struct ContainerWindowData *pContainer, HWND hwndChild);
HWND        CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTAb, BOOL bPopupContainer, BOOL bWantPopup, HANDLE hdbEvent);
int         GetProtoIconFromList(const char *szProto, int iStatus);
static void CreateImageList(BOOL bInitial);
int         ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
void        FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iNum);
void        LoadMsgAreaBackground();
int         CacheIconToBMP(struct MsgLogIcon *theIcon, HICON hIcon, COLORREF backgroundColor, int sizeX, int sizeY);
void        DeleteCachedIcon(struct MsgLogIcon *theIcon);
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
void        GetLocaleID(struct MessageWindowData *dat, char *szKLName);
int         GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen);
UINT        DrawRichEditFrame(HWND hwnd, struct MessageWindowData *mwdat, UINT skinID, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC OldWndProc);
UINT        NcCalcRichEditFrame(HWND hwnd, struct MessageWindowData *mwdat, UINT skinID, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC OldWndProc);
void        ViewReleaseNotes();
HMENU       BuildContainerMenu();
void        BuildCodePageList();
void        PreTranslateDates();
void        DrawDimmedIcon(HDC hdc, LONG left, LONG top, LONG dx, LONG dy, HICON hIcon, BYTE alpha);
void        ApplyContainerSetting(struct ContainerWindowData *pContainer, DWORD flags, int mode);
void        BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);
void        GetDefaultContainerTitleFormat();
extern const WCHAR *EncodeWithNickname(const char *string, const WCHAR *szNick, UINT codePage);
void        UpdateContainerMenu(HWND hwndDlg, struct MessageWindowData *dat);
int         MessageWindowOpened(WPARAM wParam, LPARAM lParam);
int         TABSRMM_FireEvent(HANDLE hContact, HWND hwnd, unsigned int type, unsigned int subType);

/*
 * skinning engine
 */
HICON       *BTN_GetIcon(char *szIconName);
void        IMG_RefreshItems();
void __fastcall IMG_RenderImageItem(HDC hdc, ImageItem *item, RECT *rc);
static      void LoadSkinItems(char *file, int onStartup);
static      void IMG_CreateItem(ImageItem *item, const char *fileName, HDC hdc);
static      void IMG_LoadItems(char *szFileName);
void        IMG_DeleteItems();
void        DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, int alpha, DWORD basecolor2, BYTE transparent, BYTE FLG_GRADIENT, BYTE FLG_CORNER, DWORD BORDERSTYLE, ImageItem *imageItem);
void        SkinDrawBG(HWND hwndClient, HWND hwnd, struct ContainerWindowData *pContainer, RECT *rcClient, HDC hdcTarget);
void        ReloadContainerSkin(int doLoad, int onStartup);

// the cached message log icons

void        CacheMsgLogIcons();
void        UncacheMsgLogIcons();
void        CacheLogFonts();
static void InitAPI();
void        ReloadGlobals();
static void LoadIconTheme();
static int  LoadFromIconLib();
static  int SetupIconLibConfig();
void        RTF_CTableInit();

BOOL CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int         InitOptions(void);
static      BOOL CALLBACK DlgProcContainer(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ErrorDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int         DbEventIsShown(struct MessageWindowData *dat, DBEVENTINFO *dbei);
void        StreamInEvents(HWND hwndDlg,HANDLE hDbEventFirst,int count,int fAppend, DBEVENTINFO *dbei_s);
void        LoadLogfont(int i,LOGFONTA *lf,COLORREF *colour, char *szModule);


// custom tab control

int         InitVSApi(), FreeVSApi();
void        ReloadTabConfig(), FreeTabConfig();
int         RegisterTabCtrlClass(void);
void        FreeTabConfig();
void        ReloadTabConfig();

void        ReloadGlobals(), LoadIconTheme(), UnloadIconTheme();
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
int         _DebugMessage(HWND hwndDlg, struct MessageWindowData *dat, const char *fmt, ...);

// themes

char        *GetThemeFileName(int iMode);
static void LoadLogfontFromINI(int i, char *szKey, LOGFONTA *lf, COLORREF *colour, const char *szIniFilename);
int         CheckThemeVersion(const char *szIniFilename);
void        WriteThemeToINI(const char *szIniFilename, struct MessageWindowData *dat);
void        ReadThemeFromINI(const char *szIniFilename, struct MessageWindowData *dat, int noAdvanced, DWORD dwFlags);

// compatibility

typedef     BOOL ( *pfnSetMenuInfo )( HMENU hmenu, LPCMENUINFO lpcmi );
extern      pfnSetMenuInfo fnSetMenuInfo;
