
/*
 * nen / event popup stuff
 */
 
int NEN_ReadOptions(NEN_OPTIONS *options);
int NEN_WriteOptions(NEN_OPTIONS *options);
int UpdateTrayMenu(struct MessageWindowData *dat, char *szProto, HANDLE hContact);
int PopupPreview(NEN_OPTIONS *pluginOptions);
int safe_wcslen(wchar_t *msg, int chars);

/*
 * tray stuff
 */

void CreateSystrayIcon(int create);
BOOL CALLBACK FindTrayWnd(HWND hwnd, LPARAM lParam);
void GetTrayWindowRect(LPRECT lprect);
BOOL RemoveTaskbarIcon(HWND hWnd);
void MinimiseToTray(HWND hWnd, BOOL bForceAnimation);
void MaximiseFromTray(HWND hWnd, BOOL bForceAnimation, RECT *rc);

/*
 * msgs.c
 */

int ActivateExistingTab(struct ContainerWindowData *pContainer, HWND hwndChild);
HWND CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTAb, BOOL bPopupContainer, BOOL bWantPopup, HANDLE hdbEvent);
int GetProtoIconFromList(const char *szProto, int iStatus);
void CreateImageList(BOOL bInitial);
int ActivateTabFromHWND(HWND hwndTab, HWND hwnd);
void FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iNum);
void ShowPicture(HWND hwndDlg, struct MessageWindowData *dat, BOOL changePic, BOOL showNewPic, BOOL startThread);

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

