#define CONTAINER_NAMELEN 25
#define NR_SENDJOBS 30

#define MWF_SAVEBTN_SAV 2

#define MWF_DEFERREDSCROLL 4
#define MWF_NEEDHISTORYSAVE 8
#define MWF_WASBACKGROUNDCREATE 16
#define MWF_MOUSEDOWN 32
#define MWF_ERRORSTATE 128
#define MWF_DEFERREDREMAKELOG 256

#define MWF_LOG_SHOWNICK 512
#define MWF_LOG_SHOWTIME 1024
#define MWF_LOG_SHOWSECONDS 2048
#define MWF_LOG_SHOWDATES 4096
#define MWF_LOG_NEWLINE 8192
#define MWF_LOG_INDENT 16384
#define MWF_LOG_RTL 32768
#define MWF_LOG_UNDERLINE 65536
#define MWF_LOG_SWAPNICK 131072
#define MWF_LOG_SHOWICONS 262144
#define MWF_LOG_DYNAMICAVATAR 524288

#define MWF_LOG_INDENTWITHTABS 1048576
#define MWF_CLISTMODE 0x200000
#define MWF_INITMODE  0x400000
#define MWF_NEEDCHECKSIZE 0x800000
#define MWF_DIVIDERSET 0x1000000
#define MWF_LOCALIZED 0x2000000                     // F R E E (currently unused)
#define MWF_LOG_GRID 0x4000000
#define MWF_LOG_INDIVIDUALBKG 0x8000000
#define MWF_LOG_LIMITAVATARHEIGHT 0x10000000
#define MWF_SMBUTTONSELECTED 0x20000000
#define MWF_DIVIDERWANTED 0x40000000
#define MWF_LOG_GROUPMODE 0x80000000
#define MWF_LOG_LONGDATES 64
#define MWF_LOG_USERELATIVEDATES 1

#define MWF_UI_SHOWBUTTON 1
#define MWF_UI_SHOWINFO 2
#define MWF_UI_SHOWSEND 4

#define MWF_SHOW_URLEVENTS 1
#define MWF_SHOW_FILEEVENTS 2
#define MWF_SHOW_INOUTICONS 4
#define MWF_SHOW_EMPTYLINEFIX 8
#define MWF_SHOW_MICROLF 16
#define MWF_SHOW_MARKFOLLOWUPTS 32
#define MWF_SHOW_FLASHCLIST 64

struct ContainerWindowData {
	struct ContainerWindowData *pNextContainer;
	TCHAR szName[CONTAINER_NAMELEN + 4];		// container name
	HWND hwndActive;		// active message window
	HWND hwnd;				// the container handle
	int  iTabIndex;			// next tab id
	int	 iChilds;
	HMENU hMenuContext;
	HWND  hwndTip;			// tab - tooltips...
    BOOL  bDontSmartClose;      // if set, do not search and select the next possible tab after closing one.
    UINT  iLastClick;
    POINT pLastPos;
    DWORD dwFlags;
    UINT  uChildMinHeight;
    SIZE  oldSize;
    POINT ptLast;
	DWORD dwTransparency;
    int   iContainerIndex;
    int   tBorder;
    HANDLE hContactFrom;
    BOOL  isCloned;
    HMENU hMenu;
    HWND hwndStatus;
    int statusBarHeight;
#if defined(_STREAMTHREADING)
    int volatile pendingStream;
#endif
    DWORD dwLastActivity;
    int hIcon;                // current window icon stick indicator
    DWORD dwFlashingStarted;
};

#define STICK_ICON_MSG 10

/*
struct MessageSendInfo {
	HANDLE hContact;
	HANDLE hSendId;
};*/

#define SENDJOBS_MAX_SENDS 20

struct SendJob {
    HANDLE hContact[SENDJOBS_MAX_SENDS];
    HANDLE hSendId[SENDJOBS_MAX_SENDS];
    char *sendBuffer;
    DWORD dwLen;        // actual buffer langth (checked for reallocs()
    int sendCount;
    HANDLE hOwner;
    HWND hwndOwner;
    unsigned int iStatus;
    char szErrorMsg[128];
    int iAcksNeeded;
};

struct MessageSessionStats {
    time_t started;
    unsigned int iSent, iReceived, iSentBytes, iReceivedBytes;
    unsigned int iFailures;
    unsigned int lastReceivedChars;
    BOOL bWritten;
};

struct MessageWindowData {
	HANDLE hContact;
    HWND hwnd;
	HANDLE hDbEventFirst,hDbEventLast;
	HANDLE hAckEvent;
	int multiple;
	HBRUSH hBkgBrush, hInputBkgBrush;
	int splitterY, originalSplitterY, dynaSplitter;
	int multiSplitterX;
	char *sendBuffer;
    SIZE minEditBoxSize;
	int showUIElements;
	int nLabelRight;
	int nTypeSecs;
	int nTypeMode;
	DWORD nLastTyping;
	int showTyping;
	int showTypingWin;
	DWORD lastMessage;
	struct ContainerWindowData *pContainer;		// parent container description structure
	int iTabID;			// XXX mod (tab support)
	BOOL bTabFlash;		// XXX tab flashing state...
	int iTabImage;		// XXX tabs...
	BOOL mayFlashTab;	// XXX tabs...
    HKL  hkl;           // keyboard layout identifier
    DWORD dwTickLastEvent;
	HBITMAP hContactPic;
	SIZE pic;
	int showPic;
    int bottomOffset;
    UINT uMinHeight;
// BEGIN MOD#32: Use different fonts for old history events
	BOOL isHistory;
	int isHistoryCount;
// END MOD#32
    DWORD dwFlags;
    int   iFlashIcon;
    POINT ptLast;
    WORD  wOldStatus;
    int   iOldHash;
    struct InputHistory *history;
    int   iHistoryCurrent, iHistoryTop, iHistorySize;
    HANDLE hThread;
    int doSmileys;
    UINT codePage;
#if defined(_STREAMTHREADING)
    int volatile pendingStream;
#endif    
    HBITMAP hSmileyIcon;
    char *szProto;
    WORD wStatus;
    int iLastEventType;
    time_t lastEventTime;
    DWORD dwEventIsShown;
    int iRealAvatarHeight;
    int iButtonBarNeeds;
    DWORD dwLastActivity;
    HICON hProtoIcon;
    struct MessageSessionStats stats;
    int iOpenJobs;
    int iCurrentQueueError;
    HANDLE hMultiSendThread;
    BOOL bIsMeta;
    HANDLE hFlashingEvent;
};

typedef struct _recentinfo {
    DWORD dwFirst, dwMostRecent;        // timestamps
    int   iFirstIndex, iMostRecent;     // tab indices
    HWND  hwndFirst, hwndMostRecent;    // client window handles
} RECENTINFO;

struct InputHistory {
    TCHAR *szText;
    int   lLen;
};

struct StreamJob {
    HWND hwndOwner;
    LONG startAt;
    struct MessageWindowData *dat;
    int  fAppend;
};

// menu IDS

#define MENU_LOGMENU 1
#define MENU_PICMENU 2
#define MENU_TABCONTEXT 3

#define TABSRMM_SMILEYADD_THREADING 25367
#define TABSRMM_SMILEYADD_BKGCOLORMODE 0x10000000

#define ADDEDEVENTSQUEUESIZE 100

//Checks if there is a message window opened
//wParam=(LPARAM)(HANDLE)hContact  - handle of the contact for which the window is searched. ignored if lParam
//is not zero.
//lParam=(LPARAM)(HWND)hwnd - a window handle - SET THIS TO 0 if you want to obtain the window handle
//from the hContact.
#define MS_MSG_MOD_MESSAGEDIALOGOPENED "SRMsg_MOD/MessageDialogOpened"

//obtain a pointer to the struct MessageWindowData of the given window.
//wParam = hContact - ignored if lParam is given.
//lParam = hwnd
//returns struct MessageWindowData *dat, 0 if no window is found
#define MS_MSG_MOD_GETWINDOWDATA "SRMsg_MOD/GetWindowData"

//obtain the message window flags
//wParam = hContact - ignored if lParam is given.
//lParam = hwnd
//returns struct MessageWindowData *dat, 0 if no window is found
#define MS_MSG_MOD_GETWINDOWFLAGS "SRMsg_MOD/GetWindowFlags"

// custom tabSRMM events

#define tabMSG_WINDOW_EVT_CUSTOM_BEFORESEND 1

struct TABSRMM_SessionInfo {
    unsigned int cbSize;
    unsigned int evtCode;
    HWND hwnd;              // handle of the message dialog (tab)
    HWND hwndContainer;     // handle of the parent container
    HWND hwndInput;         // handle of the input area (rich edit)
    struct MessageWindowData *dat;      // the session info
    struct ContainerWindowData *pContainer;
};

// fired, when a message is about to be sent, but BEFORE the contents of the 
                                                        // input area is examined. A plugin can therefore use this event to modify
                                                        // the contents of the input box before it is actually sent.
