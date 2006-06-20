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
*/

#ifndef _M_TABSRMM_H
#define _M_TABSRMM_H

//brings up the send message dialog for a contact
//wParam=(WPARAM)(HANDLE)hContact
//lParam=(LPARAM)(char*)szText
//returns 0 on success or nonzero on failure
//returns immediately, just after the dialog is shown
//szText is the text to put in the edit box of the window (but not send)
//szText=NULL will not use any text
//szText!=NULL is only supported on v0.1.2.0+
//NB: Current versions of the convers plugin use the name
//"SRMsg/LaunchMessageWindow" instead. For compatibility you should call
//both names and the correct one will work.
#define MS_MSG_SENDMESSAGE  "SRMsg/SendCommand"

//brings up the send message dialog with the 'multiple' option open and no
//contact selected.                                                  v0.1.2.1+
//wParam=0
//lParam=(LPARAM)(char*)szText
//returns 0 on success or nonzero on failure
#define MS_MSG_FORWARDMESSAGE  "SRMsg/ForwardMessage"

#define ME_MSG_WINDOWEVENT "MessageAPI/WindowEvent"
//wParam=(WPARAM)(MessageWindowEventData*)hWindowEvent;
//lParam=0
//Event types
#define MSG_WINDOW_EVT_OPENING 1 //window is about to be opened (uType is not used)
#define MSG_WINDOW_EVT_OPEN    2 //window has been opened (uType is not used)
#define MSG_WINDOW_EVT_CLOSING 3 //window is about to be closed (uType is not used)
#define MSG_WINDOW_EVT_CLOSE   4 //window has been closed (uType is not used)
#define MSG_WINDOW_EVT_CUSTOM  5 //custom event for message plugins to use (uType may be used)

#define MSG_WINDOW_UFLAG_MSG_FROM 0x00000001
#define MSG_WINDOW_UFLAG_MSG_TO   0x00000002
#define MSG_WINDOW_UFLAG_MSG_BOTH 0x00000004
    
typedef struct {
   int cbSize;
   HANDLE hContact;
   HWND hwndWindow; // top level window for the contact
   const char* szModule; // used to get plugin type (which means you could use local if needed)
   unsigned int uType; // see event types above
   unsigned int uFlags; // might be needed for some event types
   void *local; // used to store pointer to custom data
} MessageWindowEventData;

#define CONTAINER_NAMELEN 25
#define TITLE_FORMATLEN 50

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
//#define MWF_LOG_NEWLINE 8192
#define MWF_LOG_INDENT 16384
#define MWF_LOG_RTL 32768
//#define MWF_LOG_UNDERLINE 65536
//#define MWF_LOG_SWAPNICK 131072
#define MWF_LOG_SHOWICONS 262144
//#define MWF_LOG_DYNAMICAVATAR 524288

//#define MWF_LOG_INDENTWITHTABS 1048576
#define MWF_LOG_SYMBOLS 0x200000
#define MWF_INITMODE  0x400000
#define MWF_NEEDCHECKSIZE 0x800000
#define MWF_DIVIDERSET 0x1000000
#define MWF_LOG_TEXTFORMAT 0x2000000
#define MWF_LOG_GRID 0x4000000
#define MWF_LOG_INDIVIDUALBKG 0x8000000
#define MWF_LOG_INOUTICONS 0x10000000
#define MWF_SMBUTTONSELECTED 0x20000000
#define MWF_DIVIDERWANTED 0x40000000
#define MWF_LOG_GROUPMODE 0x80000000

#define MWF_SHOW_URLEVENTS 1
#define MWF_SHOW_FILEEVENTS 2
#define MWF_SHOW_PRIVATETHEME 4
//#define MWF_SHOW_EMPTYLINEFIX 8
//#define MWF_SHOW_MICROLF 16
//#define MWF_SHOW_MARKFOLLOWUPTS 32
#define MWF_SHOW_FLASHCLIST 64
#define MWF_SHOW_SPLITTEROVERRIDE 128
#define MWF_SHOW_SCROLLINGDISABLED 256
#define MWF_SHOW_BBCODE 512
#define MWF_SHOW_INFOPANEL 1024
#define MWF_SHOW_INFONOTES 2048
#define MWF_SHOW_ISIDLE 4096
#define MWF_SHOW_AWAYMSGTIMER 8192
#define MWF_SHOW_USELOCALTIME 16384
#define MWF_SHOW_RESIZEIPONLY 0x10000000

#define SMODE_DEFAULT 0
#define SMODE_MULTIPLE 1
#define SMODE_CONTAINER 2
#define SMODE_FORCEANSI 4
#define SMODE_SENDLATER 8
#define SMODE_NOACK 16

#define SENDFORMAT_BBCODE 2
#define SENDFORMAT_SIMPLE 1
#define SENDFORMAT_NONE 0

#define AVATARMODE_DYNAMIC 0
#define AVATARMODE_STATIC 1

#define MSGDLGFONTCOUNT 22
#define IPFONTCOUNT 6
#define CHATFONTCOUNT 19

/*
 * info panel field edges
 */

#define IPFIELD_SUNKEN 0
#define IPFIELD_RAISEDINNER 1
#define IPFIELD_RAISEDOUTER 2
#define IPFIELD_EDGE 3
#define IPFIELD_FLAT 4

#define TEMPLATE_LENGTH 150
#define CUSTOM_COLORS 5

#define TEMPLATES_MODULE "tabSRMM_Templates"
#define RTLTEMPLATES_MODULE "tabSRMM_RTLTemplates"

#define TMPL_MSGIN 0
#define TMPL_MSGOUT 1
#define TMPL_GRPSTARTIN 2
#define TMPL_GRPSTARTOUT 3
#define TMPL_GRPINNERIN 4
#define TMPL_GRPINNEROUT 5
#define TMPL_STATUSCHG 6
#define TMPL_ERRMSG 7

typedef struct _tagTemplateSet {
    BOOL valid;             // all templates populated (may still contain crap.. so it's only half-assed safety :)
    TCHAR szTemplates[TMPL_ERRMSG + 1][TEMPLATE_LENGTH];      // the template strings
    char szSetName[20];     // everything in this world needs a name. so does this poor template set.
} TemplateSet;

struct TitleBtn {
	BOOL isHot;
	BOOL isPressed;
};

#define BTN_MIN 0
#define BTN_MAX 1
#define BTN_CLOSE 2

#define NR_LOGICONS 8
#define NR_BUTTONBARICONS 29
#define NR_SIDEBARICONS 10

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
    DWORD dwFlags, dwPrivateFlags;
    UINT  uChildMinHeight;
    SIZE  oldSize;
    POINT ptLast;
	DWORD dwTransparency;
    int   iContainerIndex;
    int   tBorder;
	int	  tBorder_outer_left, tBorder_outer_right, tBorder_outer_top, tBorder_outer_bottom;
    HANDLE hContactFrom;
    BOOL  isCloned;
    HMENU hMenu;
    HWND hwndStatus, hwndSlist;
    int statusBarHeight;
    DWORD dwLastActivity;
    int hIcon;                // current window icon stick indicator
    DWORD dwFlashingStarted;
    int bInTray;              // 1 = in tray normal, 2 = in tray (was maximized)
    RECT restoreRect;
    HWND hWndOptions;
    BOOL bSizingLoop;
    int sb_NrTopButtons, sb_NrBottomButtons, sb_FirstButton;
    TCHAR szTitleFormat[TITLE_FORMATLEN + 2];
    char szThemeFile[MAX_PATH];
    TemplateSet *ltr_templates, *rtl_templates;
    LOGFONTA *logFonts;
    COLORREF *fontColors;
    char *rtfFonts;
	HDC cachedDC;
	HBITMAP cachedHBM, oldHBM;
	SIZE oldDCSize;
	BOOL bSkinned;
	RECT rcClose, rcMin, rcMax;
	struct TitleBtn buttons[3];
	struct TitleBtn oldbuttons[3];
	int ncActive;
    HICON hTitleIcon;
    BOOL  repaintMenu;
};

#define STICK_ICON_MSG 10

#define SENDJOBS_MAX_SENDS 20

struct SendJob {
    HANDLE hContact[SENDJOBS_MAX_SENDS];
    HANDLE hSendId[SENDJOBS_MAX_SENDS];
    char *sendBuffer;
    int  dwLen;        // actual buffer langth (checked for reallocs()
    int sendCount;
    HANDLE hOwner;
    HWND hwndOwner;
    unsigned int iStatus;
    char szErrorMsg[128];
    DWORD dwFlags;
    int iAcksNeeded;
};

struct MessageSessionStats {
    time_t started;
    unsigned int iSent, iReceived, iSentBytes, iReceivedBytes;
    unsigned int iFailures;
    unsigned int lastReceivedChars;
    BOOL bWritten;
};

struct MessageWindowTheme {
    COLORREF inbg, outbg, bg, inputbg;
    COLORREF hgrid;
    COLORREF custom_colors[5];
    DWORD dwFlags;
    DWORD left_indent, right_indent;
    LOGFONTA *logFonts;
    COLORREF *fontColors;
    char *rtfFonts;
};

struct MessageWindowData {
	BYTE bType;
    BYTE bWasDeleted;
	HANDLE hContact, hSubContact;
	HWND hwndLog, hwndFlash;
    HWND hwnd;
	HANDLE hDbEventFirst,hDbEventLast, hDbEventLastFeed;
	int sendMode;
	HBRUSH hBkgBrush, hInputBkgBrush;
	int splitterY, originalSplitterY, dynaSplitter, savedSplitter;
	int multiSplitterX;
	char *sendBuffer;
    int  iSendBufferSize;
    SIZE minEditBoxSize;
	int showUIElements;
	int nTypeSecs;
	int nTypeMode;
	DWORD nLastTyping;
	int showTyping;
	int showTypingWin;
	DWORD lastMessage;
	struct ContainerWindowData *pContainer;		// parent container description structure
	int iTabID;			// XXX mod (tab support)
	BOOL bTabFlash;		// XXX tab flashing state...
    HICON hTabIcon, hTabStatusIcon, hXStatusIcon, hClientIcon;
	BOOL mayFlashTab;	// XXX tabs...
    HKL  hkl;           // keyboard layout identifier
    DWORD dwTickLastEvent;
	HBITMAP hOwnPic;
	SIZE pic;
	int showPic, showInfoPic;
    int bottomOffset;
    UINT uMinHeight;
	BOOL isHistory;
    DWORD dwFlags;
    HICON  iFlashIcon;
    POINT ptLast;
    WORD  wOldStatus;
    int   iOldHash;
    struct InputHistory *history;
    int   iHistoryCurrent, iHistoryTop, iHistorySize;
    int doSmileys;
    UINT codePage;
    HICON hSmileyIcon;
    char *szProto;
    char *szMetaProto;
    TCHAR szNickname[130];
    char szStatus[50];
    WORD wStatus, wMetaStatus;
    int iLastEventType;
    time_t lastEventTime;
    DWORD dwEventIsShown;
    int iRealAvatarHeight;
    int iButtonBarNeeds, iButtonBarReallyNeeds, controlsHidden;
    DWORD dwLastActivity;
    struct MessageSessionStats stats;
    int iOpenJobs;
    int iCurrentQueueError;
    HANDLE hMultiSendThread;
    BOOL bIsMeta;
    HANDLE hFlashingEvent;
    char uin[80], myUin[80];
    BOOL bNotOnList;
    int  iAvatarDisplayMode;
    int  SendFormat;
    DWORD dwIsFavoritOrRecent;
    DWORD dwLastUpdate;
    TemplateSet *ltr_templates, *rtl_templates;
    HANDLE *hQueuedEvents;
    int    iNextQueuedEvent;
#define EVENT_QUEUE_SIZE 10
    int    iEventQueueSize;
    HBITMAP hbmMsgArea;
    TCHAR newtitle[130];        // tab title...
    LCID lcid;
    char  lcID[4];
    int   panelHeight, panelWidth;
    WORD wApparentMode;
    DWORD idle;
    HWND hwndTip;
    TOOLINFO ti;
    DWORD lastRetrievedStatusMsg;
    TCHAR statusMsg[1025];
    HANDLE hProcessAwayMsg;
    DWORD timezone, timediff;
    DWORD panelStatusCX;
    BYTE xStatus;
    COLORREF inputbg;
    SIZE szLabel;
    struct MessageWindowTheme theme;
    struct avatarCacheEntry *ace;
    COLORREF avatarbg;
	HANDLE *hHistoryEvents;
	int maxHistory, curHistory;
	BYTE needIEViewScroll;
	HANDLE hTheme;
	BYTE bFlatMsgLog;
    BYTE isIRC;
    PVOID si;
    char  szSep1[152], szMicroLf[128], szExtraLf[50];
    char  szSep1_RTL[152], szMicroLf_RTL[128];
    DWORD isAutoRTL;
};

typedef struct _recentinfo {
    DWORD dwFirst, dwMostRecent;        // timestamps
    int   iFirstIndex, iMostRecent;     // tab indices
    HWND  hwndFirst, hwndMostRecent;    // client window handles
} RECENTINFO;

/*
 * window data for the tab control window class
 */

struct TabControlData {
    BOOL m_skinning;
    HWND hwnd;
    DWORD dwStyle;
    DWORD cx, cy;
    HANDLE hTheme, hThemeButton;
    BYTE m_xpad;
    struct ContainerWindowData *pContainer;
    BOOL   bDragging;
    int    iBeginIndex;
    HWND   hwndDrag;
    struct MessageWindowData *dragDat;
    HIMAGELIST himlDrag;
};

/*
 * configuration data for custom tab ctrl
 */

struct myTabCtrl {
    HPEN m_hPenShadow, m_hPenItemShadow, m_hPenLight;
    HFONT m_hMenuFont;
    COLORREF colors[10];
    HBRUSH m_hBrushDefault, m_hBrushActive, m_hBrushUnread, m_hBrushHottrack;
    DWORD m_fixedwidth;
    int m_bottomAdjust;
};

/*
 * configuration data for the info panel (fonts, colors)
 */

struct infopanelconfig {
    HFONT hFonts[IPFONTCOUNT];
    COLORREF clrs[IPFONTCOUNT];
    COLORREF clrClockSymbol, clrBackground;
    BOOL isValid;                   // valid data exist (font service required, otherwise, defaults are used)
    BYTE borderStyle;
    HBRUSH bkgBrush;
    UINT edgeType, edgeFlags;
    
};

typedef struct _globals {
    HWND g_hwndHotkeyHandler;
    HICON g_iconIn, g_iconOut, g_iconErr, g_iconContainer, g_iconStatus;
    HCURSOR hCurSplitNS, hCurSplitWE, hCurHyperlinkHand;
    HBITMAP g_hbmUnknown;
    int g_MetaContactsAvail, g_SmileyAddAvail, g_SecureIMAvail, g_WantIEView, g_PopupAvail, g_PopupWAvail, g_FontServiceAvail;
    int g_FlashAvatarAvail;
    HICON g_IconMsgEvent, g_IconTypingEvent, g_IconEmpty, g_IconFileEvent, g_IconUrlEvent, g_IconSend;
    HICON g_IconFolder, g_IconChecked, g_IconUnchecked;
	HICON g_closeGlyph, g_maxGlyph, g_minGlyph, g_pulldownGlyph;
    HIMAGELIST g_hImageList;
    int g_nrProtos;
    HMENU g_hMenuContext, g_hMenuContainer, g_hMenuEncoding, g_hMenuTrayUnread;
    HMENU g_hMenuFavorites, g_hMenuRecent, g_hMenuTrayContext;
    int  g_wantSnapping;
    HICON g_buttonBarIcons[NR_BUTTONBARICONS];
    HICON g_sideBarIcons[NR_SIDEBARICONS];
    int iSendJobCurrent;
    // dynamic options, need reload when options change
    int m_SendOnShiftEnter;
    int m_SendOnEnter;
    int m_SendOnDblEnter;
    int m_AutoLocaleSupport;
    int m_AutoSwitchTabs;
    int m_CutContactNameOnTabs;
    int m_CutContactNameTo;
    int m_StatusOnTabs;
    int m_LogStatusChanges;
    int m_UseDividers;
    int m_DividersUsePopupConfig;
    int m_MsgTimeout;
    int m_EscapeCloses;
    int m_WarnOnClose;
    int m_ExtraMicroLF;
    int m_AvatarMode, m_OwnAvatarMode;
    int m_FlashOnClist;
    int m_TabAutoClose;
    int m_AlwaysFullToolbarWidth;
    int m_LimitStaticAvatarHeight;
    int m_SendFormat;
    int m_FormatWholeWordsOnly;
    int m_AllowSendButtonHidden;
    int m_ToolbarHideMode;
    int m_FixFutureTimestamps;
    int m_AvatarDisplayMode;
    int m_RTLDefault;
    int m_SplitterSaveOnClose;
    int m_MathModAvail;
    TCHAR m_MathModStartDelimiter[40];
    int m_UnreadInTray;
    int m_TrayFlashes;
    int m_TrayFlashState;
    BOOL m_SuperQuiet;
    HANDLE m_TipOwner;
    HANDLE m_UserMenuItem;
    HBITMAP m_hbmMsgArea;
    int m_WheelDefault;
    BYTE m_WinVerMajor;
    BYTE m_WinVerMinor;
    BYTE m_SideBarEnabled;
    HWND m_hwndClist;
    int  m_TabAppearance;
    int  m_VSApiEnabled;
    struct myTabCtrl tabConfig;
    BYTE m_ExtraRedraws;
    char szDataPath[MAX_PATH + 1];
    int  m_panelHeight;
    TCHAR szDefaultTitleFormat[256];
    DWORD m_GlobalContainerFlags;
    DWORD m_GlobalContainerTrans;
    WINDOWPLACEMENT m_GlobalContainerWpos;
    HANDLE hLastOpenedContact;
    int m_Send7bitStrictAnsi;
    int  m_IdleDetect;
    int  m_DoStatusMsg;
    int m_smcxicon, m_smcyicon;
    DWORD local_gmt_diff;
    int m_PasteAndSend;
    TCHAR *m_szNoStatus;
    HFONT m_hFontWebdings;
    struct infopanelconfig ipConfig;
    COLORREF crDefault, crIncoming, crOutgoing;
    BOOL bUnicodeBuild;
    BYTE bClipBorder;
    DWORD bRoundedCorner;
	BYTE bAvatarBoderType;
	HFONT hFontCaption;
	COLORREF skinDefaultFontColor;
	BYTE m_dropShadow;
    DWORD m_LangPackCP;
    BYTE  m_SmileyButtonOverride;
	char g_SkinnedFrame_left, g_SkinnedFrame_right, g_SkinnedFrame_bottom, g_SkinnedFrame_caption;
	char g_realSkinnedFrame_left, g_realSkinnedFrame_right, g_realSkinnedFrame_bottom, g_realSkinnedFrame_caption;
    HPEN g_SkinLightShadowPen, g_SkinDarkShadowPen;
    NONCLIENTMETRICS ncm;
    HICON m_AnimTrayIcons[4];
    BOOL  g_DisableScrollbars;
} MYGLOBALS;

typedef struct _tag_ICONDESC {
    char *szName;
    char *szDesc;
    HICON *phIcon;      // where the handle is saved...
    int  uId;           // icon ID
    BOOL bForceSmall;   // true: force 16x16
} ICONDESC;

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
#define MENU_PANELPICMENU 4

#define TABSRMM_SMILEYADD_BKGCOLORMODE 0x10000000

#define ADDEDEVENTSQUEUESIZE 100

/*
 * tab config flags
 */

#define TCF_FLAT 1
#define TCF_NOSKINNING 4
#define TCF_FLASHICON 8
#define TCF_FLASHLABEL 16
#define TCF_SINGLEROWTABCONTROL 32
#define TCF_LABELUSEWINCOLORS 64
#define TCF_BKGUSEWINCOLORS 128

#define TCF_DEFAULT (TCF_FLASHICON | TCF_LABELUSEWINCOLORS | TCF_BKGUSEWINCOLORS)

//Checks if there is a message window opened
//wParam=(LPARAM)(HANDLE)hContact  - handle of the contact for which the window is searched. ignored if lParam
//is not zero.
//lParam=(LPARAM)(HWND)hwnd - a window handle - SET THIS TO 0 if you want to obtain the window handle
//from the hContact.
#define MS_MSG_MOD_MESSAGEDIALOGOPENED "SRMsg_MOD/MessageDialogOpened"

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

#define MS_MSG_GETWINDOWAPI "MessageAPI/WindowAPI"
//wparam=0
//lparam=0
//Returns a dword with the current message api version 
//Current version is 0,0,0,3

#define MS_MSG_GETWINDOWCLASS "MessageAPI/WindowClass"
//wparam=(char*)szBuf
//lparam=(int)cbSize size of buffer
//Sets the window class name in wParam (ex. "SRMM" for srmm.dll)

typedef struct {
	int cbSize;
	HANDLE hContact;
	int uFlags; // see uflags above
} MessageWindowInputData;

#define MSG_WINDOW_STATE_EXISTS  0x00000001 // Window exists should always be true if hwndWindow exists
#define MSG_WINDOW_STATE_VISIBLE 0x00000002
#define MSG_WINDOW_STATE_FOCUS   0x00000004
#define MSG_WINDOW_STATE_ICONIC  0x00000008

typedef struct {
	int cbSize;
	HANDLE hContact;
	int uFlags;  // should be same as input data unless 0, then it will be the actual type
	HWND hwndWindow; //top level window for the contact or NULL if no window exists
	int uState; // see window states
	void *local; // used to store pointer to custom data
} MessageWindowOutputData;

#define MS_MSG_GETWINDOWDATA "MessageAPI/GetWindowData"
//wparam=(MessageWindowInputData*)
//lparam=(MessageWindowData*)
//returns 0 on success and returns non-zero (1) on error or if no window data exists for that hcontact

// callback for the user menu entry

#define MS_TABMSG_SETUSERPREFS "SRMsg_MOD/SetUserPrefs"

// show one of the tray menus
// wParam = 0 -> session list
// wParam = 1 -> tray menu
// lParam must be 0
// 
#define MS_TABMSG_TRAYSUPPORT "SRMsg_MOD/Show_TrayMenu"

#endif
