
#define TIMERID_MSGSEND      100
#define TIMERID_TYPE         3
#define ANTIBOMB_COUNT       3
#define TIMEOUT_TYPEOFF      10000      // send type off after 10 seconds of inactivity
#define SB_CHAR_WIDTH        45

#define SQ_INPROGRESS 1
#define SQ_ERROR 2
#define SQ_UNDEFINED 0

/*
 * send flags
 */

#if defined(_UNICODE)
    #define SEND_FLAGS PREF_UNICODE
#else
    #define SEND_FLAGS 0
#endif

extern int g_IconEmpty, g_IconMsgEvent, g_IconTypingEvent, g_IconFileEvent, g_IconUrlEvent, g_IconError, g_IconSend;
extern HICON g_iconErr;
extern HBITMAP g_hbmUnknown;

void ClearSendJob(int iIndex);
int FindNextFailedMsg(HWND hwndDlg, struct MessageWindowData *dat);
int HandleQueueError(HWND hwndDlg, struct MessageWindowData *dat, int iEntry);
int AddToSendQueue(HWND hwndDlg, struct MessageWindowData *dat, int iLen);
int SendQueuedMessage(HWND hwndDlg, struct MessageWindowData *dat, int iEntry);
void CheckSendQueue(HWND hwndDlg, struct MessageWindowData *dat);
void LogErrorMessage(HWND hwndDlg, struct MessageWindowData *dat, int iSendJobIndex, char *szErrMsg);
void RecallFailedMessage(HWND hwndDlg, struct MessageWindowData *dat, int iEntry);
void UpdateSaveAndSendButton(HWND hwndDlg, struct MessageWindowData *dat);
void NotifyDeliveryFailure(HWND hwndDlg, struct MessageWindowData *dat);
void ShowErrorControls(HWND hwndDlg, struct MessageWindowData *dat, int showCmd);
void EnableSending(HWND hwndDlg, struct MessageWindowData *dat, int iMode);
void UpdateReadChars(HWND hwndDlg, struct MessageWindowData *dat);
void ShowMultipleControls(HWND hwndDlg, const UINT * controls, int cControls, int state);
void HandleIconFeedback(HWND hwndDlg, struct MessageWindowData *dat, int iIcon);
int GetProtoIconFromList(const char *szProto, int iStatus);
int ActivateExistingTab(struct ContainerWindowData *pContainer, HWND hwndChild);

static int CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

char *MsgServiceName(HANDLE hContact);

