
/*
 * nen / event popup stuff
 */
 
int NEN_ReadOptions(NEN_OPTIONS *options);
int NEN_WriteOptions(NEN_OPTIONS *options);
int UpdateTrayMenu(struct MessageWindowData *dat, WORD wStatus, char *szProto, char *szStatus, HANDLE hContact, BOOL fromEvent);
int PopupPreview(NEN_OPTIONS *pluginOptions);
int safe_wcslen(wchar_t *msg, int chars);
void DeletePopupsForContact(HANDLE hContact, DWORD dwMask);

/*
 * tray stuff
 */

void CreateSystrayIcon(int create);
BOOL CALLBACK FindTrayWnd(HWND hwnd, LPARAM lParam);
void GetTrayWindowRect(LPRECT lprect);
BOOL RemoveTaskbarIcon(HWND hWnd);
void MinimiseToTray(HWND hWnd, BOOL bForceAnimation);
void MaximiseFromTray(HWND hWnd, BOOL bForceAnimation, RECT *rc);
void RemoveBalloonTip();
void FlashTrayIcon(int mode);
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
void CreateImageList(BOOL bInitial);
int ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
void FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iNum);
void LoadMsgAreaBackground();
int CacheIconToBMP(struct MsgLogIcon *theIcon, HICON hIcon, COLORREF backgroundColor, int sizeX, int sizeY);
void DeleteCachedIcon(struct MsgLogIcon *theIcon);
int MY_GetContactDisplayNameW(HANDLE hContact, wchar_t *szwBuf, unsigned int size, const char *szProto, UINT codePage);
int GetTabIndexFromHWND(HWND, HWND);

// the cached message log icons
void CacheMsgLogIcons();
void UncacheMsgLogIcons();
void CacheLogFonts();
void ConvertAllToUTF8();
void InitAPI();
void ReloadGlobals();
void LoadIconTheme();
int LoadFromIconLib();
int SetupIconLibConfig();
void CreateSystrayIcon();
void PreTranslateDates();

BOOL CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int InitOptions(void);
BOOL CALLBACK DlgProcContainer(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ErrorDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int DbEventIsShown(struct MessageWindowData *dat, DBEVENTINFO *dbei);
void StreamInEvents(HWND hwndDlg,HANDLE hDbEventFirst,int count,int fAppend, DBEVENTINFO *dbei_s);
void LoadMsgLogIcons(void);
void FreeMsgLogIcons(void);

void LoadLogfont(int i,LOGFONTA *lf,COLORREF *colour);

int _DebugPopup(HANDLE hContact, const char *fmt, ...);
int _DebugMessage(HWND hwndDlg, struct MessageWindowData *dat, const char *fmt, ...);

int InitVSApi(), FreeVSApi();
void ReloadTabConfig(), FreeTabConfig();
int RegisterTabCtrlClass(void);

void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);
void UpdateContainerMenu(HWND hwndDlg, struct MessageWindowData *dat);

// buttons

int LoadTSButtonModule(void);
int UnloadTSButtonModule(WPARAM wParam, LPARAM lParam);

void ApplyContainerSetting(struct ContainerWindowData *pContainer, DWORD flags, int mode);
void ReloadGlobalContainerSettings();
void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);

/*
 * font service support
 */

void FS_RegisterFonts();
void MoveFonts();

extern const WCHAR *EncodeWithNickname(const char *string, const WCHAR *szNick, UINT codePage);
void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam);
void GetDefaultContainerTitleFormat();

/*
 * debugging support
 */

int _DebugTraceW(const wchar_t *fmt, ...);
int _DebugTraceA(const char *fmt, ...);
