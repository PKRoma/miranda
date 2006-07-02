
/*
 * nen / event popup stuff
 */
 
int NEN_ReadOptions(NEN_OPTIONS *options);
int NEN_WriteOptions(NEN_OPTIONS *options);
int UpdateTrayMenu(struct MessageWindowData *dat, WORD wStatus, char *szProto, char *szStatus, HANDLE hContact, DWORD fromEvent);
int PopupPreview(NEN_OPTIONS *pluginOptions);
int safe_wcslen(wchar_t *msg, int chars);
void DeletePopupsForContact(HANDLE hContact, DWORD dwMask);

/*
 * tray stuff
 */

void CreateSystrayIcon(int create);
void MinimiseToTray(HWND hWnd, BOOL bForceAnimation);
void MaximiseFromTray(HWND hWnd, BOOL bForceAnimation, RECT *rc);
void RemoveBalloonTip();
void FlashTrayIcon(HICON hIcon);
void UpdateTrayMenuState(struct MessageWindowData *dat, BOOL bForced);
void LoadFavoritesAndRecent();
void AddContactToFavorites(HANDLE hContact, TCHAR *szNickname, char *szProto, char *szStatus, WORD wStatus, HICON hIcon, BOOL mode, HMENU hMenu, UINT codePage);
void CreateTrayMenus(int mode);
void HandleMenuEntryFromhContact(int iSelection);

/*
 * msgs.c
 */

int ActivateExistingTab(struct ContainerWindowData *pContainer, HWND hwndChild);
HWND CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTAb, BOOL bPopupContainer, BOOL bWantPopup, HANDLE hdbEvent);
int GetProtoIconFromList(const char *szProto, int iStatus);
static void CreateImageList(BOOL bInitial);
int ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
void FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iNum);
void LoadMsgAreaBackground();
int CacheIconToBMP(struct MsgLogIcon *theIcon, HICON hIcon, COLORREF backgroundColor, int sizeX, int sizeY);
void DeleteCachedIcon(struct MsgLogIcon *theIcon);
int MY_GetContactDisplayNameW(HANDLE hContact, wchar_t *szwBuf, unsigned int size, const char *szProto, UINT codePage);
int GetTabIndexFromHWND(HWND, HWND);
struct ContainerWindowData *FindMatchingContainer(const TCHAR *szName, HANDLE hContact);
struct ContainerWindowData *CreateContainer(const TCHAR *name, int iTemp, HANDLE hContactFrom);
int CutContactName(TCHAR *oldname, TCHAR *newname, unsigned int size);
struct ContainerWindowData *FindContainerByName(const TCHAR *name);
void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);
int GetTabIndexFromHWND(HWND hwndTab, HWND hwnd);
static int GetTabItemFromMouse(HWND hwndTab, POINT *pt);
int ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
int GetProtoIconFromList(const char *szProto, int iStatus);
void AdjustTabClientRect(struct ContainerWindowData *pContainer, RECT *rc);
void FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iCount);
void ReflashContainer(struct ContainerWindowData *pContainer);
HMENU BuildMCProtocolMenu(HWND hwndDlg);
static struct ContainerWindowData *AppendToContainerList(struct ContainerWindowData *pContainer);
static struct ContainerWindowData *RemoveContainerFromList(struct ContainerWindowData *pContainer);
void DeleteContainer(int iIndex), RenameContainer(int iIndex, const TCHAR *newName);
int EnumContainers(HANDLE hContact, DWORD dwAction, const TCHAR *szTarget, const TCHAR *szNew, DWORD dwExtinfo, DWORD dwExtinfoEx);
void GetLocaleID(struct MessageWindowData *dat, char *szKLName);
int GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen);
UINT DrawRichEditFrame(HWND hwnd, struct MessageWindowData *mwdat, UINT skinID, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC OldWndProc);
UINT NcCalcRichEditFrame(HWND hwnd, struct MessageWindowData *mwdat, UINT skinID, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC OldWndProc);

void ViewReleaseNotes();

HICON *BTN_GetIcon(char *szIconName);
void IMG_RefreshItems();
int  RegisterContainer();
HMENU BuildContainerMenu();
void BuildCodePageList();
void RearrangeTab(HWND hwndDlg, struct MessageWindowData *dat, int iMode);

// the cached message log icons
void CacheMsgLogIcons();
void UncacheMsgLogIcons();
void CacheLogFonts();
static void InitAPI();
void ReloadGlobals();
static void LoadIconTheme();
static int LoadFromIconLib();
static int SetupIconLibConfig();
void PreTranslateDates();
void DrawDimmedIcon(HDC hdc, LONG left, LONG top, LONG dx, LONG dy, HICON hIcon, BYTE alpha);

BOOL CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int InitOptions(void);
BOOL CALLBACK DlgProcContainer(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ErrorDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int DbEventIsShown(struct MessageWindowData *dat, DBEVENTINFO *dbei);
void StreamInEvents(HWND hwndDlg,HANDLE hDbEventFirst,int count,int fAppend, DBEVENTINFO *dbei_s);
void LoadMsgLogIcons(void);
void FreeMsgLogIcons(void);

void LoadLogfont(int i,LOGFONTA *lf,COLORREF *colour, char *szModule);

int _DebugPopup(HANDLE hContact, const char *fmt, ...);
int _DebugMessage(HWND hwndDlg, struct MessageWindowData *dat, const char *fmt, ...);

int InitVSApi(), FreeVSApi();
void ReloadTabConfig(), FreeTabConfig();
int RegisterTabCtrlClass(void);
void FreeTabConfig();
void ReloadTabConfig();

void CacheMsgLogIcons(), CacheLogFonts(), ReloadGlobals(), LoadIconTheme(), UnloadIconTheme();
void CreateImageList(BOOL bInitial);
void GetDefaultContainerTitleFormat();

int Chat_OptionsInitialize(WPARAM wParam, LPARAM lParam);

void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);
void UpdateContainerMenu(HWND hwndDlg, struct MessageWindowData *dat);
int MessageWindowOpened(WPARAM wParam, LPARAM lParam);
void TABSRMM_FireEvent(HANDLE hContact, HWND hwnd, unsigned int type, unsigned int subType);

// buttons

int LoadTSButtonModule(void);
int UnloadTSButtonModule(WPARAM wParam, LPARAM lParam);

void ApplyContainerSetting(struct ContainerWindowData *pContainer, DWORD flags, int mode);
void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);

extern const WCHAR *EncodeWithNickname(const char *string, const WCHAR *szNick, UINT codePage);
void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);
void GetDefaultContainerTitleFormat();

/*
 * debugging support
 */

int _DebugTraceW(const wchar_t *fmt, ...);
int _DebugTraceA(const char *fmt, ...);

// themes

char *GetThemeFileName(int iMode);
static void LoadLogfontFromINI(int i, char *szKey, LOGFONTA *lf, COLORREF *colour, const char *szIniFilename);
int CheckThemeVersion(const char *szIniFilename);
void WriteThemeToINI(const char *szIniFilename, struct MessageWindowData *dat);
void ReadThemeFromINI(const char *szIniFilename, struct MessageWindowData *dat, int noAdvanced, DWORD dwFlags);




