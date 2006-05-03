//main.c
void				LoadIcons(void);
void				LoadLogIcons(void);
void				FreeIcons(void);
void				UpgradeCheck(void);

//colorchooser.c
BOOL CALLBACK		DlgProcColorToolWindow(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

//log.c
void				Log_StreamInEvent(HWND hwndDlg, LOGINFO* lin, SESSION_INFO* si, BOOL bRedraw, BOOL bPhaseTwo);
void				LoadMsgLogBitmaps(void);
void				FreeMsgLogBitmaps(void);
void				ValidateFilename (char * filename);
char *				MakeTimeStamp(char * pszStamp, time_t time);
char *				Log_CreateRtfHeader(MODULEINFO * mi);

//window.c
BOOL CALLBACK		RoomWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
int					GetTextPixelSize(char * pszText, HFONT hFont, BOOL bWidth);

//options.c
int					OptionsInit(void);
int					OptionsUnInit(void);
void				LoadMsgDlgFont(int i, LOGFONT * lf, COLORREF * colour, char *szmod);
void				LoadGlobalSettings(void);
void				AddIcons(void);
HICON				LoadIconEx(int iIndex, char * pszIcoLibName, int iX, int iY);

//services.c
void				HookEvents(void);
void				UnhookEvents(void);
void				CreateServiceFunctions(void);
void           DestroyServiceFunctions(void);
void				CreateHookableEvents(void);
void				TabsInit(void);
int					ModulesLoaded(WPARAM wParam,LPARAM lParam);
int					SmileyOptionsChanged(WPARAM wParam,LPARAM lParam);
int					PreShutdown(WPARAM wParam,LPARAM lParam);
int					IconsChanged(WPARAM wParam,LPARAM lParam);
void				ShowRoom(SESSION_INFO * si, WPARAM wp, BOOL bSetForeground);
int					Service_Register(WPARAM wParam, LPARAM lParam);
int					Service_AddEvent(WPARAM wParam, LPARAM lParam);
int					Service_GetAddEventPtr(WPARAM wParam, LPARAM lParam);
int					Service_NewChat(WPARAM wParam, LPARAM lParam);
int					Service_ItemData(WPARAM wParam, LPARAM lParam);
int					Service_SetSBText(WPARAM wParam, LPARAM lParam);
int					Service_SetVisibility(WPARAM wParam, LPARAM lParam);
int					Service_GetCount(WPARAM wParam,LPARAM lParam);
int					Service_GetInfo(WPARAM wParam,LPARAM lParam);
HWND                CreateNewRoom(struct ContainerWindowData *pContainer, SESSION_INFO *si, BOOL bActivateTab, BOOL bPopupContainer, BOOL bWantPopup);

//manager.c
void				SetActiveSession(char * pszID, char * pszModule);
void				SetActiveSessionEx(SESSION_INFO * si);
SESSION_INFO *		GetActiveSession(void);
SESSION_INFO *		SM_AddSession(char * pszID, char * pszModule);
int					SM_RemoveSession(char * pszID, char * pszModule);
SESSION_INFO *		SM_FindSession(char *pszID, char * pszModule);
USERINFO *			SM_AddUser(char *pszID, char * pszModule, char * pszUID, char * pszNick, WORD wStatus);
BOOL				SM_ChangeUID(char *pszID, char * pszModule, char * pszUID, char * pszNewUID);
BOOL				SM_ChangeNick(char *pszID, char * pszModule, GCEVENT * gce);
BOOL				SM_RemoveUser(char *pszID, char * pszModule, char * pszUID);
BOOL				SM_SetOffline(char *pszID, char * pszModule);
BOOL				SM_SetTabbedWindowHwnd(SESSION_INFO * si, HWND hwnd);
HICON				SM_GetStatusIcon(SESSION_INFO * si, USERINFO * ui, char *szIndicator);
BOOL				SM_SetStatus(char *pszID, char * pszModule, int wStatus);
BOOL				SM_SetStatusEx(char *pszID, char * pszModule, char * pszText);
BOOL				SM_SendUserMessage(char *pszID, char * pszModule, char * pszText);
STATUSINFO *		SM_AddStatus(char *pszID, char * pszModule, char * pszStatus);
SESSION_INFO *		SM_GetNextWindow(SESSION_INFO * si);
SESSION_INFO *		SM_GetPrevWindow(SESSION_INFO * si);
BOOL				SM_AddEventToAllMatchingUID(GCEVENT * gce);
BOOL				SM_AddEvent(char *pszID, char * pszModule, GCEVENT * gce, BOOL bIsHighlighted);
LRESULT				SM_SendMessage(char *pszID, char * pszModule, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL				SM_PostMessage(char *pszID, char * pszModule, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL				SM_BroadcastMessage(char * pszModule, UINT msg, WPARAM wParam, LPARAM lParam, BOOL bAsync);
BOOL				SM_RemoveAll (void);
BOOL				SM_GiveStatus(char *pszID, char * pszModule, char * pszUID,  char * pszStatus);
BOOL				SM_TakeStatus(char *pszID, char * pszModule, char * pszUID,  char * pszStatus);
BOOL				SM_MoveUser(char *pszID, char * pszModule, char * pszUID);
void				SM_AddCommand(char *pszID, char * pszModule, const char *lpNewCommand);
char *				SM_GetPrevCommand(char *pszID, char * pszModule);
char *				SM_GetNextCommand(char *pszID, char * pszModule);
int					SM_GetCount(char * pszModule);
SESSION_INFO *		SM_FindSessionByIndex(char * pszModule, int iItem);
SESSION_INFO *		SM_FindSessionByHWND(HWND h);
SESSION_INFO *		SM_FindSessionByHCONTACT(HANDLE h);
char *				SM_GetUsers(SESSION_INFO * si);
USERINFO *			SM_GetUserFromIndex(char *pszID, char * pszModule, int index);
int                 SM_IsIRC(SESSION_INFO *si);
MODULEINFO *		MM_AddModule(char* pszModule);
MODULEINFO	*		MM_FindModule(char * pszModule);
void				MM_FixColors();
void				MM_FontsChanged(void);
void				MM_IconsChanged(void);
BOOL				MM_RemoveAll (void);
STATUSINFO *		TM_AddStatus(STATUSINFO** ppStatusList, char * pszStatus, int * iCount);
STATUSINFO *		TM_FindStatus(STATUSINFO* pStatusList, char* pszStatus);
WORD				TM_StringToWord(STATUSINFO* pStatusList, char* pszStatus);
char *				TM_WordToString(STATUSINFO* pStatusList, WORD Status);
BOOL				TM_RemoveAll (STATUSINFO** pStatusList);
BOOL				UM_SetStatusEx(USERINFO* pUserList,char* pszText);
USERINFO*			UM_AddUser(STATUSINFO* pStatusList, USERINFO** pUserList, char * pszUID, char * pszNick, WORD wStatus);
USERINFO *			UM_SortUser(USERINFO** ppUserList, char * pszUID);
USERINFO*			UM_FindUser(USERINFO* pUserList, char* pszUID);
USERINFO*			UM_FindUserFromIndex(USERINFO* pUserList, int index);
USERINFO*			UM_GiveStatus(USERINFO* pUserList, char* pszUID, WORD status);
USERINFO*			UM_TakeStatus(USERINFO* pUserList, char* pszUID, WORD status);
char*				UM_FindUserAutoComplete(USERINFO* pUserList, char * pszOriginal, char* pszCurrent);
BOOL				UM_RemoveUser(USERINFO** pUserList, char* pszUID);
BOOL				UM_RemoveAll (USERINFO** ppUserList);
LOGINFO *			LM_AddEvent(LOGINFO** ppLogListStart, LOGINFO** ppLogListEnd);
BOOL				LM_TrimLog(LOGINFO** ppLogListStart, LOGINFO** ppLogListEnd, int iCount);
BOOL				LM_RemoveAll (LOGINFO** ppLogListStart, LOGINFO** ppLogListEnd);

//clist.c
HANDLE				CList_AddRoom(char * pszModule, char * pszRoom, char * pszDisplayName, int  iType);
BOOL				CList_SetOffline(HANDLE hContact, BOOL bHide);
BOOL				CList_SetAllOffline(BOOL bHide);
int					CList_RoomDoubleclicked(WPARAM wParam,LPARAM lParam);
int					CList_EventDoubleclicked(WPARAM wParam,LPARAM lParam);
void				CList_CreateGroup(char *group);
BOOL				CList_AddEvent(HANDLE hContact, HICON Icon, HANDLE event, int type, char * fmt, ... ) ;
HANDLE				CList_FindRoom (char * pszModule, char * pszRoom) ;
int					WCCmp(char* wild, char *string);

//tools.c
char *				RemoveFormatting(char * pszText);
BOOL				DoSoundsFlashPopupTrayStuff(SESSION_INFO * si, GCEVENT * gce, BOOL bHighlight, int bManyFix);
int					Chat_GetColorIndex(char * pszModule, COLORREF cr);
void				CheckColorsInModule(char * pszModule);
char*				my_strstri(char *s1, char *s2) ;
int					GetRichTextLength(HWND hwnd);
BOOL				IsHighlighted(SESSION_INFO * si, char * pszText);
UINT				CreateGCMenu(HWND hwndDlg, HMENU *hMenu, int iIndex, POINT pt, SESSION_INFO * si, char * pszUID, char * pszWordText);
void				DestroyGCMenu(HMENU *hMenu, int iIndex);
BOOL				DoEventHookAsync(HWND hwnd, char * pszID, char * pszModule, int iType, char * pszUID, char * pszText, DWORD dwItem);
BOOL				DoEventHook(char * pszID, char * pszModule, int iType, char * pszUID, char * pszText, DWORD dwItem);
BOOL				LogToFile(SESSION_INFO * si, GCEVENT * gce);

// message.c
char *				Chat_Message_GetFromStream(HWND hwndDlg, SESSION_INFO* si);
BOOL				Chat_DoRtfToTags(char * pszText, SESSION_INFO * si);

