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

#ifndef _MSGS_H
#define _MSGS_H

#define __MATHMOD_SUPPORT 1

#ifdef __GNUWIN32__
#define COLOR_HOTLIGHT 26
#if !defined(SB_SETICON)

#define SB_SETICON (WM_USER+15)
#define SB_SETTIPTEXTA (WM_USER+16)
#define TCS_BOTTOM 0x0002

#define TVS_NOHSCROLL 0x8000
#define TVS_CHECKBOXES          0x0100
#endif

#define CFM_ALL (CFM_EFFECTS | CFM_SIZE | CFM_FACE | CFM_OFFSET | CFM_CHARSET)
#define SES_EXTENDBACKCOLOR 4           // missing from the mingw32 headers
#define MIM_BACKGROUND              0x00000002
#define GT_SELECTION 2
#define ST_SELECTION 2
#define ST_DEFAULT 0
#define ST_KEEPUNDO 1
#define CFM_WEIGHT 0x0040000
#define SBT_TOOLTIPS 0x0800
#define DFCS_HOT 0x1000

#if defined(_UNICODE)
    #define TTM_SETTITLE (WM_USER+33)
#else
    #define TTM_SETTITLE (WM_USER+32)
#endif

#define FLASHW_STOP 0
#define FLASHW_TRAY 0x00000002
#define FLASHW_CAPTION 0x00000001
#define FLASHW_ALL (FLASHW_TRAY | FLASHW_CAPTION)
#define FLASHW_TIMERNOFG 0x0000000C
#define FLASHW_TIMER 0x00000004
#define IMF_AUTOKEYBOARD 0x0001
#define ODS_INACTIVE        0x0080
#define NIN_BALLOONUSERCLICK (WM_USER + 5)
#define NIN_BALLOONHIDE (WM_USER + 3)
#define NIN_BALLOONTIMEOUT (WM_USER + 4)
#define IMF_AUTOFONTSIZEADJUST	0x0010

#define WM_THEMECHANGED 0x031A

typedef struct __gettextex
{
	DWORD	cb;				// Count of bytes in the string				
	DWORD	flags;			// Flags (see the GT_XXX defines			
	UINT	codepage;		// Code page for translation (CP_ACP for sys default,
						    //  1200 for Unicode, -1 for control default)	
	LPCSTR	lpDefaultChar;	// Replacement for unmappable chars			
	LPBOOL	lpUsedDefChar;	// Pointer to flag set when def char used	
} _GETTEXTEX;

#define WM_APPCOMMAND                   0x0319
#define APPCOMMAND_BROWSER_BACKWARD       1
#define APPCOMMAND_BROWSER_FORWARD        2
#define FAPPCOMMAND_MASK  0xF000
#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))

#ifndef _WIN32_IE
typedef struct tagNMMOUSE {
    NMHDR       hdr;
    DWORD_PTR   dwItemSpec;
    DWORD_PTR   dwItemData;
    POINT       pt;
    LPARAM      dwHitInfo; // any specifics about where on the item or control the mouse is
} NMMOUSE, *LPNMMOUSE;
#endif

#define __forceinline __inline

#define GRADIENT_FILL_RECT_V 1
#define GRADIENT_FILL_RECT_H 0
#define CS_DROPSHADOW       0x00020000
#define CFE_LINK		0x0020

#define __try
#define __except(x) if (0) /* don't execute handler */
#define __finally

#define _try __try
#define _except __except
#define _finally __finally

/*
typedef struct _settextex {
    DWORD	flags;
    UINT	codepage;
} SETTEXTEX;
*/
#endif

#include <richedit.h>
#include <richole.h>
#include "m_avatars.h"
#include "m_message.h"

#define MSGERROR_CANCEL	0
#define MSGERROR_RETRY	    1
#define MSGERROR_SENDLATER  2

#define HISTORY_INITIAL_ALLOCSIZE 300

#define CONTAINER_NAMELEN 25
#define TITLE_FORMATLEN 50

#define NR_SENDJOBS 30

#define MWF_SAVEBTN_SAV 2

#define MWF_DEFERREDSCROLL 4
#define MWF_NEEDHISTORYSAVE 8
#define MWF_WASBACKGROUNDCREATE 16
//#define MWF_MOUSEDOWN 32
#define MWF_ERRORSTATE 128
#define MWF_DEFERREDREMAKELOG 256

#define MWF_LOG_NORMALTEMPLATES 512
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
#define MWF_EX_DELAYEDSPLITTER 32768
#define MWF_EX_AVATARCHANGED 65536
#define MWF_EX_WARNCLOSE     0x20000

#define SMODE_DEFAULT 0
#define SMODE_MULTIPLE 1
#define SMODE_CONTAINER 2
#define SMODE_FORCEANSI 4
#define SMODE_SENDLATER 8
#define SMODE_NOACK 16

#define SENDFORMAT_BBCODE 1
#define SENDFORMAT_NONE 0

#define AVATARMODE_DYNAMIC 0
//#define AVATARMODE_STATIC 1

#define MSGDLGFONTCOUNT 22
#define IPFONTCOUNT 6
#define CHATFONTCOUNT 19

#define TMPL_MSGIN 0
#define TMPL_MSGOUT 1
#define TMPL_GRPSTARTIN 2
#define TMPL_GRPSTARTOUT 3
#define TMPL_GRPINNERIN 4
#define TMPL_GRPINNEROUT 5
#define TMPL_STATUSCHG 6
#define TMPL_ERRMSG 7

#define TEMPLATE_LENGTH 150
#define CUSTOM_COLORS 5

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
	struct  ContainerWindowData *pNextContainer;
	TCHAR   szName[CONTAINER_NAMELEN + 4];		// container name
	HWND    hwndActive;		// active message window
	HWND    hwnd;				// the container handle
	int     iTabIndex;			// next tab id
	int	    iChilds;
    int     iContainerIndex;
	HMENU   hMenuContext;
	HWND    hwndTip;			// tab - tooltips...
    BOOL    bDontSmartClose;      // if set, do not search and select the next possible tab after closing one.
    DWORD   dwFlags, dwPrivateFlags;
    UINT    uChildMinHeight;
    SIZE    oldSize, preSIZE;
	DWORD   dwTransparency;
    int     tBorder;
	int	    tBorder_outer_left, tBorder_outer_right, tBorder_outer_top, tBorder_outer_bottom;
    HANDLE  hContactFrom;
    BOOL    isCloned;
    HMENU   hMenu;
    HWND    hwndStatus, hwndSlist;
    int     statusBarHeight;
    DWORD   dwLastActivity;
    int     hIcon;                // current window icon stick indicator
    DWORD   dwFlashingStarted;
    int     bInTray;              // 1 = in tray normal, 2 = in tray (was maximized)
    RECT    restoreRect;
    HWND    hWndOptions;
    BOOL    bSizingLoop;
    int     sb_NrTopButtons, sb_NrBottomButtons, sb_FirstButton;
    int     sb_TopHeight, sb_BottomHeight;
    TCHAR   szTitleFormat[TITLE_FORMATLEN + 2];
    char    szThemeFile[MAX_PATH];
    TemplateSet *ltr_templates, *rtl_templates;
    LOGFONTA *logFonts;
    COLORREF *fontColors;
    char    *rtfFonts;
	HDC     cachedDC;
	HBITMAP cachedHBM, oldHBM;
	SIZE    oldDCSize;
	BOOL    bSkinned;
	RECT    rcClose, rcMin, rcMax;
	struct  TitleBtn buttons[3];
	struct  TitleBtn oldbuttons[3];
	int     ncActive;
    HWND    hwndSaved;
    ButtonItem *buttonItems;
    RECT    rcSaved;
    DWORD   exFlags;
};

#define STICK_ICON_MSG 10
#define SENDJOBS_MAX_SENDS 20

struct SendJob {
    HANDLE  hContact[SENDJOBS_MAX_SENDS];
    HANDLE  hSendId[SENDJOBS_MAX_SENDS];
    char    *sendBuffer;
    int     dwLen;        // actual buffer langth (checked for reallocs()
    int     sendCount;
    HANDLE  hOwner;
    HWND    hwndOwner;
    unsigned int iStatus;
    char    szErrorMsg[128];
    DWORD   dwFlags;
    int     iAcksNeeded;
    HANDLE  hEventSplit;
    int     chunkSize;
    DWORD   dwTime;
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
	BYTE    bType;
    BYTE    bWasDeleted;
	HANDLE  hContact, hSubContact;
	HWND    hwndIEView, hwndFlash, hwndIWebBrowserControl, hwndHPP;
    HWND    hwnd;
	HANDLE  hDbEventFirst,hDbEventLast, hDbEventLastFeed;
	int     sendMode;
	HBRUSH  hBkgBrush, hInputBkgBrush;
	int     splitterY, originalSplitterY, dynaSplitter, savedSplitter;
	int     multiSplitterX;
	char    *sendBuffer;
    int     iSendBufferSize;
    SIZE    minEditBoxSize;
	int     showUIElements;
	int     nTypeSecs;
	int     nTypeMode;
	DWORD   nLastTyping;
	int     showTyping;
	int     showTypingWin;
	DWORD   lastMessage;
	struct  ContainerWindowData *pContainer;		// parent container description structure
	int     iTabID;
	BOOL    bTabFlash;
    HICON   hTabIcon, hTabStatusIcon, hXStatusIcon, hClientIcon;
	BOOL    mayFlashTab;
    HKL     hkl;                                    // keyboard layout identifier
    DWORD   dwTickLastEvent;
	HBITMAP hOwnPic;
	SIZE    pic;
	int     showPic, showInfoPic;
    BOOL    fMustOffset;
    UINT    uMinHeight;
	BOOL    isHistory;
    DWORD   dwFlags;
    HICON   iFlashIcon;
    //POINT   ptLast;
    WORD    wOldStatus;
    int     iOldHash;
    struct  InputHistory *history;
    int     iHistoryCurrent, iHistoryTop, iHistorySize;
    int     doSmileys;
    UINT    codePage;
    HICON   hSmileyIcon;
    char    *szProto;
    char    *szMetaProto;
    TCHAR   szNickname[130], szMyNickname[130];
    char    szStatus[50];
    WORD    wStatus, wMetaStatus;
    int     iLastEventType;
    time_t  lastEventTime;
    DWORD   dwFlagsEx;
    int     iRealAvatarHeight;
    int     iButtonBarNeeds, iButtonBarReallyNeeds, controlsHidden;
    DWORD   dwLastActivity;
    struct  MessageSessionStats stats;
    int     iOpenJobs;
    int     iCurrentQueueError;
    HANDLE  hMultiSendThread;
    BOOL    bIsMeta;
    HANDLE  hFlashingEvent;
    char    uin[80], myUin[80];
    BOOL    bNotOnList;
    int     SendFormat;
    DWORD   dwIsFavoritOrRecent;
    DWORD   dwLastUpdate;
    TemplateSet *ltr_templates, *rtl_templates;
    HANDLE  *hQueuedEvents;
    int     iNextQueuedEvent;
#define EVENT_QUEUE_SIZE 10
    int     iEventQueueSize;
    TCHAR   newtitle[130];        // tab title...
    LCID    lcid;
    char    lcID[4];
    int     panelHeight, panelWidth;
    WORD    wApparentMode;
    DWORD   idle;
    HWND    hwndTip;
    TOOLINFO ti;
    DWORD   lastRetrievedStatusMsg;
    TCHAR   statusMsg[1025];
    HANDLE  hProcessAwayMsg;
    DWORD   timezone, timediff;
    DWORD   panelStatusCX;
    BYTE    xStatus;
    COLORREF inputbg;
    SIZE    szLabel;
    struct  MessageWindowTheme theme;
    struct  avatarCacheEntry *ace, *ownAce;
    COLORREF avatarbg;
	HANDLE  *hHistoryEvents;
	int     maxHistory, curHistory;
	HANDLE  hTheme;
	BYTE    bFlatMsgLog;
    BYTE    isIRC;
    PVOID   si;
    char    szMicroLf[128];
    DWORD   isAutoRTL;
    int     nMax;            // max message size
    int     textLen;         // current text len
    LONG    ipFieldHeight;
    WNDPROC oldIEViewProc;
    BOOL    clr_added;
    BOOL    fIsReattach;
    WPARAM  wParam;          // used for "delayed" actions like moved splitters in minimized windows
    LPARAM  lParam;
    int     iHaveRTLLang;
    BOOL    fInsertMode;
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
    BOOL    m_skinning;
    BOOL    m_moderntabs;
    HWND    hwnd;
    DWORD   dwStyle;
    DWORD   cx, cy;
    HANDLE  hTheme, hThemeButton;
    BYTE    m_xpad;
    struct  ContainerWindowData *pContainer;
    BOOL    bDragging;
    int     iBeginIndex;
    HWND    hwndDrag;
    struct  MessageWindowData *dragDat;
    HIMAGELIST himlDrag;
    BOOL    bRefreshWithoutClip;
    BOOL    fSavePos;
    BOOL    fTipActive;
};

/*
 * configuration data for custom tab ctrl
 */

struct myTabCtrl {
    HPEN    m_hPenShadow, m_hPenItemShadow, m_hPenLight, m_hPenStyledLight, m_hPenStyledDark;
    HFONT   m_hMenuFont;
    COLORREF colors[10];
    HBRUSH  m_hBrushDefault, m_hBrushActive, m_hBrushUnread, m_hBrushHottrack;
    DWORD   m_fixedwidth;
    int     m_bottomAdjust;
};

/*
 * configuration data for the info panel (fonts, colors)
 */

struct infopanelconfig {
    HFONT       hFonts[IPFONTCOUNT];
    COLORREF    clrs[IPFONTCOUNT];
    COLORREF    clrClockSymbol, clrBackground;
    BOOL        isValid;                   // valid data exist (font service required, otherwise, defaults are used)
    BYTE        borderStyle;
    HBRUSH      bkgBrush;
    UINT        edgeType, edgeFlags;
    
};

typedef struct _globals {
    HWND        g_hwndHotkeyHandler;
    HICON       g_iconIn, g_iconOut, g_iconErr, g_iconContainer, g_iconStatus;
    HCURSOR     hCurSplitNS, hCurSplitWE, hCurHyperlinkHand;
    HBITMAP     g_hbmUnknown;
    int         g_MetaContactsAvail, g_SmileyAddAvail, g_WantIEView, g_PopupAvail, g_PopupWAvail, g_WantHPP;
    int         g_FlashAvatarAvail;
    HIMAGELIST  g_hImageList;
    HICON       g_IconMsgEvent, g_IconTypingEvent, g_IconFileEvent, g_IconUrlEvent, g_IconSend;
    HICON       g_IconFolder, g_IconChecked, g_IconUnchecked;
	HICON       g_closeGlyph, g_maxGlyph, g_minGlyph, g_pulldownGlyph;
    int         g_nrProtos;
    HMENU       g_hMenuContext, g_hMenuContainer, g_hMenuEncoding, g_hMenuTrayUnread;
    HMENU       g_hMenuFavorites, g_hMenuRecent, g_hMenuTrayContext;
    //int         g_wantSnapping;
    HICON       g_buttonBarIcons[NR_BUTTONBARICONS];
    HICON       g_sideBarIcons[NR_SIDEBARICONS];
    int         iSendJobCurrent;
    // dynamic options, need reload when options change
    int         m_SendOnShiftEnter;
    int         m_SendOnEnter;
    int         m_SendOnDblEnter;
    int         m_AutoLocaleSupport;
    int         m_AutoSwitchTabs;
    int         m_CutContactNameOnTabs;
    int         m_CutContactNameTo;
    int         m_StatusOnTabs;
    int         m_LogStatusChanges;
    int         m_UseDividers;
    int         m_DividersUsePopupConfig;
    int         m_MsgTimeout;
    int         m_EscapeCloses;
    int         m_WarnOnClose;
    int         m_AvatarMode, m_OwnAvatarMode;
    int         m_FlashOnClist;
    int         m_TabAutoClose;
    int         m_AlwaysFullToolbarWidth;
    int         m_LimitStaticAvatarHeight;
    int         m_SendFormat;
    int         m_FormatWholeWordsOnly;
    int         m_AllowSendButtonHidden;
    int         m_ToolbarHideMode;
    int         m_FixFutureTimestamps;
    int         m_RTLDefault;
    int         m_SplitterSaveOnClose;
    int         m_MathModAvail;
    TCHAR       m_MathModStartDelimiter[40];
    int         m_UnreadInTray;
    int         m_TrayFlashes;
    int         m_TrayFlashState;
    BOOL        m_SuperQuiet;
    HANDLE      m_TipOwner;
    HANDLE      m_UserMenuItem;
    BYTE        m_WinVerMajor;
    BYTE        m_WinVerMinor;
    BYTE        m_bIsXP;
    BYTE        m_SideBarEnabled;
    HWND        m_hwndClist;
    int         m_TabAppearance;
    int         m_VSApiEnabled;
    struct      myTabCtrl tabConfig;
    //BYTE        m_ExtraRedraws;
    char        szDataPath[MAX_PATH + 1];
    int         m_panelHeight;
    TCHAR       szDefaultTitleFormat[256];
    DWORD       m_GlobalContainerFlags;
    DWORD       m_GlobalContainerTrans;
    WINDOWPLACEMENT m_GlobalContainerWpos;
    HANDLE      hLastOpenedContact;
    int         m_Send7bitStrictAnsi;
    int         m_IdleDetect;
    int         m_DoStatusMsg;
    int         m_smcxicon, m_smcyicon;
    DWORD       local_gmt_diff;
    int         m_PasteAndSend;
    TCHAR       *m_szNoStatus;
    HFONT       m_hFontWebdings;
    struct      infopanelconfig ipConfig;
    COLORREF    crDefault, crIncoming, crOutgoing;
    BOOL        bUnicodeBuild;
    BYTE        bClipBorder;
    DWORD       bRoundedCorner;
	BYTE        bAvatarBoderType;
	HFONT       hFontCaption;
	COLORREF    skinDefaultFontColor;
    DWORD       m_LangPackCP;
    BYTE        m_SmileyButtonOverride;
	char        g_SkinnedFrame_left, g_SkinnedFrame_right, g_SkinnedFrame_bottom, g_SkinnedFrame_caption;
	char        g_realSkinnedFrame_left, g_realSkinnedFrame_right, g_realSkinnedFrame_bottom, g_realSkinnedFrame_caption;
    HPEN        g_SkinLightShadowPen, g_SkinDarkShadowPen;
    NONCLIENTMETRICS ncm;
    HICON       m_AnimTrayIcons[4];
    BOOL        g_DisableScrollbars;
    BOOL        m_visualMessageSizeIndicator;
    BOOL        m_autoSplit;
    int         rtf_ctablesize;
    DWORD       dwThreadID;
    char        szMetaName[256];
} MYGLOBALS;

typedef struct _tag_ICONDESC {
    char    *szName;
    char    *szDesc;
    HICON   *phIcon;      // where the handle is saved...
    int     uId;           // icon ID
    BOOL    bForceSmall;   // true: force 16x16
} ICONDESC;

struct InputHistory {
    TCHAR   *szText;
    int     lLen;
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

/*
 * info panel field edges
 */

#define IPFIELD_SUNKEN 0
#define IPFIELD_RAISEDINNER 1
#define IPFIELD_RAISEDOUTER 2
#define IPFIELD_EDGE 3
#define IPFIELD_FLAT 4

struct NewMessageWindowLParam {
	HANDLE  hContact;
	int     isWchar;
	const   char *szInitialText;
	int     iTabID;				// XXX mod: tab support
	int     iTabImage;			// XXX mod tabs...
    int     iActivate;
    TCITEM  item;
	struct  ContainerWindowData *pContainer;		// parent container description
    BOOL    bWantPopup;
    HANDLE  hdbEvent;
};

// flags for the container dwFlags 
#define CNT_MOUSEDOWN 1
#define CNT_NOTITLE 2
#define CNT_HIDETABS 4
#define CNT_SIDEBAR 8
#define CNT_NOFLASH 0x10
#define CNT_STICKY 0x20
#define CNT_DONTREPORT 0x40
#define CNT_FLASHALWAYS 0x80
#define CNT_TRANSPARENCY 0x100
#define CNT_TITLE_PRIVATE 0x200
#define CNT_GLOBALSETTINGS 0x400
#define CNT_GLOBALSIZE 0x800
#define CNT_INFOPANEL 0x1000
#define CNT_NOSOUND 0x2000
#define CNT_SYNCSOUNDS 0x4000
#define CNT_DEFERREDCONFIGURE 0x8000
#define CNT_CREATE_MINIMIZED 0x10000
#define CNT_NEED_UPDATETITLE 0x20000
#define CNT_DEFERREDSIZEREQUEST 0x40000
#define CNT_DONTREPORTUNFOCUSED 0x80000
#define CNT_ALWAYSREPORTINACTIVE 0x100000
#define CNT_NEWCONTAINERFLAGS 0x200000
#define CNT_DEFERREDTABSELECT 0x400000
#define CNT_CREATE_CLONED 0x800000
#define CNT_NOSTATUSBAR 0x1000000
#define CNT_NOMENUBAR 0x2000000
#define CNT_TABSBOTTOM 0x4000000
#define CNT_STATICICON 0x8000000
// #define CNT_TITLE_SHOWUIN 0x10000000  *free*
#define CNT_HIDETOOLBAR 0x20000000
#define CNT_UINSTATUSBAR 0x40000000
#define CNT_VERTICALMAX 0x80000000

#define CNT_FLAGS_DEFAULT (CNT_HIDETABS | CNT_GLOBALSETTINGS | CNT_NEWCONTAINERFLAGS)
#define CNT_TRANS_DEFAULT 0x00ff00ff

#define CNT_CREATEFLAG_CLONED 1
#define CNT_CREATEFLAG_MINIMIZED 2

#define CNT_EX_CLOSEWARN 1

#define MWF_LOG_ALL (MWF_LOG_NORMALTEMPLATES | MWF_LOG_SHOWTIME | MWF_LOG_SHOWSECONDS | \
        MWF_LOG_SHOWDATES | MWF_LOG_INDENT | MWF_LOG_TEXTFORMAT | MWF_LOG_SYMBOLS | MWF_LOG_INOUTICONS | \
        MWF_LOG_SHOWICONS | MWF_LOG_GRID | MWF_LOG_INDIVIDUALBKG | MWF_LOG_GROUPMODE)
        
#define MWF_LOG_DEFAULT (MWF_LOG_SHOWTIME | MWF_LOG_NORMALTEMPLATES | MWF_LOG_SHOWDATES | MWF_LOG_SYMBOLS | MWF_LOG_INDIVIDUALBKG | MWF_LOG_GRID | MWF_LOG_GROUPMODE)

/*
struct ProtocolData {
    char szName[30];
    int  iFirstIconID;
};*/

#define EM_SUBCLASSED             (WM_USER+0x101)
#define EM_SEARCHSCROLLER         (WM_USER+0x103)
#define EM_VALIDATEBOTTOM         (WM_USER+0x104)
#define EM_THEMECHANGED           (WM_USER+0x105)
#define EM_UNSUBCLASSED			  (WM_USER+0x106)
#define EM_REFRESHWITHOUTCLIP     (WM_USER+0x107)

#define HM_EVENTSENT         (WM_USER+10)
#define DM_REMAKELOG         (WM_USER+11)
#define HM_DBEVENTADDED      (WM_USER+12)
#define DM_SETINFOPANEL      (WM_USER+13)
#define DM_OPTIONSAPPLIED    (WM_USER+14)
#define DM_SPLITTERMOVED     (WM_USER+15)
#define DM_UPDATETITLE       (WM_USER+16)
#define DM_APPENDTOLOG       (WM_USER+17)
#define DM_ERRORDECIDED      (WM_USER+18)
#define DM_SPLITSENDACK      (WM_USER+19)
#define DM_TYPING            (WM_USER+20)
#define DM_UPDATEWINICON     (WM_USER+21)
#define DM_UPDATELASTMESSAGE (WM_USER+22) 

#define DM_SELECTTAB		 (WM_USER+23)
#define DM_CLOSETABATMOUSE   (WM_USER+24)
#define DM_STATUSICONCHANGE  (WM_USER+25)
#define DM_SETLOCALE         (WM_USER+26)
#define DM_SESSIONLIST       (WM_USER+27)
#define DM_QUERYLASTUNREAD   (WM_USER+28)
#define DM_QUERYPENDING      (WM_USER+29)
#define DM_UPDATEPICLAYOUT   (WM_USER+30)
#define DM_QUERYCONTAINER    (WM_USER+31)
#define DM_QUERYCONTAINERHWND    (WM_USER+32)
#define DM_CALCMINHEIGHT     (WM_USER+33)       // msgdialog asked to recalculate its minimum height
#define DM_REPORTMINHEIGHT   (WM_USER+34)       // msg dialog reports its minimum height to the container
#define DM_QUERYMINHEIGHT    (WM_USER+35)       // container queries msg dialog about minimum height
#define DM_SAVESIZE          (WM_USER+36)
#define DM_CHECKSIZE         (WM_USER+37)
#define DM_SAVEPERCONTACT    (WM_USER+38)
#define DM_CONTAINERSELECTED (WM_USER+39)
#define DM_CONFIGURECONTAINER (WM_USER+40)
#define DM_QUERYHCONTACT    (WM_USER+41)
#define DM_DEFERREDREMAKELOG (WM_USER+42)
#define DM_RESTOREWINDOWPOS  (WM_USER+43)
#define DM_FORCESCROLL       (WM_USER+44)
#define DM_QUERYCLIENTAREA   (WM_USER+45)
#define DM_QUERYRECENT       (WM_USER+47)
#define DM_ACTIVATEME        (WM_USER+46)
#define DM_REGISTERHOTKEYS   (WM_USER+48)
#define DM_FORCEUNREGISTERHOTKEYS (WM_USER+49)
#define DM_ADDDIVIDER        (WM_USER+50)
#define DM_STATUSMASKSET     (WM_USER+51)       
#define DM_CONTACTSETTINGCHANGED (WM_USER+52)
#define DM_UPDATESTATUSMSG   (WM_USER+53)
#define DM_PROTOACK          (WM_USER+54)
#define DM_OWNNICKCHANGED    (WM_USER+55)
#define DM_CONFIGURETOOLBAR  (WM_USER+56)
#define DM_LOADBUTTONBARICONS (WM_USER+57)
#define DM_ACTIVATETOOLTIP   (WM_USER+58)
#define DM_UINTOCLIPBOARD   (WM_USER+59)
#define DM_SPLITTEREMERGENCY (WM_USER+60)
#define DM_SENDMESSAGECOMMAND (WM_USER+61)
#define DM_FORCEDREMAKELOG   (WM_USER+62)
#define DM_QUERYFLAGS        (WM_USER+63)
#define DM_STATUSBARCHANGED  (WM_USER+64)
#define DM_SAVEMESSAGELOG    (WM_USER+65)
#define DM_CHECKAUTOCLOSE    (WM_USER+66)
#define DM_UPDATEMETACONTACTINFO (WM_USER+67)
#define DM_SETICON           (WM_USER+68)
#define DM_MULTISENDTHREADCOMPLETE (WM_USER+69)
#define DM_CHECKQUEUEFORCLOSE (WM_USER+70)
#define DM_QUERYSTATUS       (WM_USER+71)
#define DM_SETPARENTDIALOG   (WM_USER+72)
#define DM_HANDLECLISTEVENT  (WM_USER+73)
#define DM_TRAYICONNOTIFY    (WM_USER+74)
#define DM_REMOVECLISTEVENT  (WM_USER+75)
#define DM_GETWINDOWSTATE    (WM_USER+76)
#define DM_DOCREATETAB       (WM_USER+77)
#define DM_DELAYEDSCROLL     (WM_USER+78)
#define DM_REPLAYQUEUE       (WM_USER+79)
#define DM_HKDETACH          (WM_USER+80)
#define DM_HKSAVESIZE        (WM_USER+81)
#define DM_SETSIDEBARBUTTONS (WM_USER+82)
#define DM_REFRESHTABINDEX   (WM_USER+83)
#define DM_PROTOAVATARCHANGED (WM_USER+84)
#define DM_SMILEYOPTIONSCHANGED (WM_USER+85)
#define DM_MYAVATARCHANGED	 (WM_USER+86)
#define DM_PRINTCLIENT		 (WM_USER+87)
#define DM_IEVIEWOPTIONSCHANGED (WM_USER+88)
#define DM_SPLITTERMOVEDGLOBAL (WM_USER+89)
#define DM_DOCREATETAB_CHAT    (WM_USER+90)
#define DM_CLIENTCHANGED       (WM_USER+91)
#define DM_PLAYINCOMINGSOUND   (WM_USER+92)
#define DM_SENDMESSAGECOMMANDW (WM_USER+93)
#define DM_REMOVEPOPUPS        (WM_USER+94)

#define DM_SC_BUILDLIST      (WM_USER+100)
#define DM_SC_INITDIALOG     (WM_USER+101)
#define DM_SCROLLIEVIEW		 (WM_USER+102)

#define MINSPLITTERY         52
#define MINLOGHEIGHT         30

// wParam values for DM_SELECTTAB

#define DM_SELECT_NEXT		 1
#define DM_SELECT_PREV		 2

#define DM_SELECT_BY_HWND	 3		// lParam specifies hwnd
#define DM_SELECT_BY_INDEX   4		// lParam specifies tab index

#define DM_QUERY_NEXT 1
#define DM_QUERY_MOSTRECENT 2

#ifndef CPLUSPLUS
struct CREOleCallback {
	IRichEditOleCallbackVtbl *lpVtbl;
	unsigned refCount;
	IStorage *pictStg;
	int nextStgId;
};
#endif

#define MSGFONTID_MYMSG		  0
#define MSGFONTID_MYMISC	  1
#define MSGFONTID_YOURMSG	  2
#define MSGFONTID_YOURMISC	  3
#define MSGFONTID_MYNAME	  4
#define MSGFONTID_MYTIME	  5
#define MSGFONTID_YOURNAME	  6
#define MSGFONTID_YOURTIME	  7
#define H_MSGFONTID_MYMSG		8
#define H_MSGFONTID_MYMISC		9
#define H_MSGFONTID_YOURMSG		10
#define H_MSGFONTID_YOURMISC	11
#define H_MSGFONTID_MYNAME		12
#define H_MSGFONTID_MYTIME		13
#define H_MSGFONTID_YOURNAME	14
#define H_MSGFONTID_YOURTIME	15
#define MSGFONTID_MESSAGEAREA 16
#define H_MSGFONTID_STATUSCHANGES 17
#define H_MSGFONTID_DIVIDERS 18
#define MSGFONTID_ERROR 19
#define MSGFONTID_SYMBOLS_IN 20
#define MSGFONTID_SYMBOLS_OUT 21

#define IPFONTID_NICK 0
#define IPFONTID_UIN 1
#define IPFONTID_STATUS 2
#define IPFONTID_PROTO 3
#define IPFONTID_TIME 4

extern const int msgDlgFontCount;

#define LOADHISTORY_UNREAD    0
#define LOADHISTORY_COUNT     1
#define LOADHISTORY_TIME      2

#define SRMSGSET_AUTOPOPUP         "AutoPopup"
#define SRMSGDEFSET_AUTOPOPUP      0
#define SRMSGSET_AUTOMIN           "AutoMin"
#define SRMSGDEFSET_AUTOMIN        0
#define SRMSGSET_SENDONENTER       "SendOnEnter"
#define SRMSGDEFSET_SENDONENTER    0
#define SRMSGSET_MSGTIMEOUT        "MessageTimeout"
#define SRMSGDEFSET_MSGTIMEOUT     30000
#define SRMSGSET_MSGTIMEOUT_MIN    4000 // minimum value (4 seconds)

#define SRMSGSET_LOADHISTORY       "LoadHistory"
#define SRMSGDEFSET_LOADHISTORY    LOADHISTORY_UNREAD
#define SRMSGSET_LOADCOUNT         "LoadCount"
#define SRMSGDEFSET_LOADCOUNT      10
#define SRMSGSET_LOADTIME          "LoadTime"
#define SRMSGDEFSET_LOADTIME       10

#define SRMSGSET_SHOWURLS          "ShowURLs"
#define SRMSGDEFSET_SHOWURLS       1
#define SRMSGSET_SHOWFILES         "ShowFiles"
#define SRMSGDEFSET_SHOWFILES      1
#define SRMSGSET_BKGCOLOUR         "BkgColour"
#define SRMSGDEFSET_BKGCOLOUR      GetSysColor(COLOR_WINDOW)

#define SRMSGSET_TYPING             "SupportTyping"
#define SRMSGSET_TYPINGNEW          "DefaultTyping"
#define SRMSGDEFSET_TYPINGNEW       1
#define SRMSGSET_TYPINGUNKNOWN      "UnknownTyping"
#define SRMSGDEFSET_TYPINGUNKNOWN   0
#define SRMSGSET_SHOWTYPING         "ShowTyping"
#define SRMSGDEFSET_SHOWTYPING      1
#define SRMSGSET_SHOWTYPINGWIN      "ShowTypingWin"
#define SRMSGDEFSET_SHOWTYPINGWIN   1
#define SRMSGSET_SHOWTYPINGNOWIN    "ShowTypingTray"
#define SRMSGDEFSET_SHOWTYPINGNOWIN 0
#define SRMSGSET_SHOWTYPINGCLIST    "ShowTypingClist"
#define SRMSGDEFSET_SHOWTYPINGCLIST 1

// xxx rtl support
#define SRMSGDEFSET_MOD_RTL                      0

#define TIMERID_FLASHWND     1
#define TIMEOUT_FLASHWND     900
#define TIMERID_HEARTBEAT    2
#define TIMEOUT_HEARTBEAT    20000
#define TIMERID_HOVER 10
#define TIMERID_HOVER_T 11

#define SRMSGMOD "SRMsg"
#define SRMSGMOD_T "Tab_SRMsg"
#define FONTMODULE "TabSRMM_Fonts"
#define CHAT_FONTMODULE "TabSRMM_chat_Fonts"

#define IDM_STAYONTOP (WM_USER + 1)
#define IDM_NOTITLE (WM_USER + 2)
#define IDM_MOREOPTIONS (WM_USER +4)

typedef DWORD   (WINAPI *PSLWA)(HWND, DWORD, BYTE, DWORD);
typedef BOOL    (WINAPI *PULW)(HWND, HDC, POINT *, SIZE *, HDC, POINT *, COLORREF, BLENDFUNCTION *, DWORD);
typedef BOOL    (WINAPI *PFWEX)(FLASHWINFO *);
typedef BOOL    (WINAPI *PAB)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
typedef BOOL    (WINAPI *PGF)(HDC, PTRIVERTEX, ULONG, PVOID, ULONG, ULONG);

typedef BOOL (WINAPI *PITA)();
typedef HANDLE (WINAPI *POTD)(HWND, LPCWSTR);
typedef UINT (WINAPI *PDTB)(HANDLE, HDC, int, int, RECT *, RECT *);
typedef UINT (WINAPI *PCTD)(HANDLE);
typedef UINT (WINAPI *PDTT)(HANDLE, HDC, int, int, LPCWSTR, int, DWORD, DWORD, RECT *);
typedef BOOL (WINAPI *PITBPT)(HANDLE, int, int);
typedef HRESULT (WINAPI *PDTPB)(HWND, HDC, RECT *);
typedef HRESULT (WINAPI *PGTBCR)(HANDLE, HDC, int, int, const RECT *, const RECT *);

// constants for the container management functions

#define CNT_ENUM_DELETE 1           // delete the target container...
#define CNT_ENUM_RENAME 2
#define CNT_ENUM_WRITEFLAGS 4

#define IDM_CONTAINERMENU 50500

#define EVENTTYPE_STATUSCHANGE 25368
#define EVENTTYPE_DIVIDER 25367
#define EVENTTYPE_ERRMSG 25366

// hotkey modifiers...

#define HOTKEY_MODIFIERS_CTRLSHIFT 0
#define HOTKEY_MODIFIERS_CTRLALT 1
#define HOTKEY_MODIFIERS_ALTSHIFT 2

struct MsgLogIcon {
    HBITMAP hBmp, hoBmp;
    HDC hdc, hdcMem;
    HBRUSH hBkgBrush;
};

#include "icons/iconsxp/resource.h"         // icon pack values

WCHAR *Utf8_Decode(const char *str);
char *Utf8_Encode(const WCHAR *str);

struct CPTABLE {
    UINT cpId;
    TCHAR *cpName;
};

#define LOI_TYPE_FLAG 1
#define LOI_TYPE_SETTING 2

struct LISTOPTIONSGROUP {
    LRESULT handle;
    TCHAR *szName;
};

struct LISTOPTIONSITEM {
    LRESULT handle;
    TCHAR *szName;
    UINT id;
    UINT uType;
    UINT_PTR lParam;
    UINT uGroup;
};

// sidebar button flags

#define SBI_TOP 1
#define SBI_BOTTOM 2
#define SBI_HIDDEN 4
#define SBI_DISABLED 8
#define SBI_TOGGLE   16
#define SBI_HANDLEBYCLIENT 32

// fixed stock button identifiers

#define IDC_SBAR_SLIST                  1111
#define IDC_SBAR_FAVORITES              1112
#define IDC_SBAR_RECENT                 1113
#define IDC_SBAR_SETUP                  1114
#define IDC_SBAR_USERPREFS              1115
#define IDC_SBAR_TOGGLEFORMAT           1117
#define IDC_SBAR_CANCEL                 1118

struct SIDEBARITEM {
    UINT    uId;
    DWORD   dwFlags;
    HICON   *hIcon, *hIconPressed, *hIconHover;
    char    *szName;
    void    (*pfnAction)(ButtonItem *item, HWND hwndDlg, struct MessageWindowData *dat, HWND hwndItem);
    void    (*pfnCallback)(ButtonItem *item, HWND hwndDlg, struct MessageWindowData *dat, HWND hwndItem);
    TCHAR   *tszTip;
};

#if defined(_UNICODE)
static __inline int mir_snprintfW(wchar_t *buffer, size_t count, const wchar_t* fmt, ...) {
	va_list va;
	int len;

	va_start(va, fmt);
	len = _vsnwprintf(buffer, count-1, fmt, va);
	va_end(va);
	buffer[count-1] = 0;
	return len;
}
#endif

#define FONTF_BOLD   1
#define FONTF_ITALIC 2
#define FONTF_UNDERLINE 4

#define RTFCACHELINESIZE 128

#define ID_EXTBKCONTAINER 0
#define ID_EXTBKBUTTONBAR 1
#define ID_EXTBKBUTTONSPRESSED 2
#define ID_EXTBKBUTTONSNPRESSED 3
#define ID_EXTBKBUTTONSMOUSEOVER 4
#define ID_EXTBKINFOPANEL 5
#define ID_EXTBKTITLEBUTTON 6
#define ID_EXTBKTITLEBUTTONMOUSEOVER 7
#define ID_EXTBKTITLEBUTTONPRESSED 8
#define ID_EXTBKTABPAGE 9
#define ID_EXTBKTABITEM 10
#define ID_EXTBKTABITEMACTIVE 11
#define ID_EXTBKTABITEMBOTTOM 12
#define ID_EXTBKTABITEMACTIVEBOTTOM 13
#define ID_EXTBKFRAME 14
#define ID_EXTBKHISTORY 15
#define ID_EXTBKINPUTAREA 16
#define ID_EXTBKFRAMEINACTIVE 17
#define ID_EXTBKTABITEMHOTTRACK 18
#define ID_EXTBKTABITEMHOTTRACKBOTTOM 19
#define ID_EXTBKSTATUSBARPANEL 20
#define ID_EXTBKSTATUSBAR      21
#define ID_EXTBKUSERLIST       22
#define ID_EXTBK_LAST 22

#define SESSIONTYPE_IM 1
#define SESSIONTYPE_CHAT 2

#define DEFAULT_SIDEBARWIDTH         30

#define THEME_READ_FONTS 1
#define THEME_READ_TEMPLATES 2
#define THEME_READ_ALL (THEME_READ_FONTS | THEME_READ_TEMPLATES)

#define BUDDYPOUNCE_SERVICENAME "BuddyPounce/AddToPounce"
#define IDC_TBFIRSTUID 10000            // first uId for custom buttons

#define RTF_CTABLE_DEFSIZE 8

#include "templates.h"

struct StatusIconListNode {
    struct StatusIconListNode *next;
	StatusIconData sid;
};

struct TABSRMM_SessionInfo {
    unsigned int cbSize;
    unsigned short evtCode;
    HWND hwnd;              // handle of the message dialog (tab)
    HWND hwndContainer;     // handle of the parent container
    HWND hwndInput;         // handle of the input area (rich edit)
    UINT extraFlags;
    UINT extraFlagsEX;
    void *local;
};

typedef struct {
	int cbSize;
	HANDLE hContact;
	int uFlags;  // should be same as input data unless 0, then it will be the actual type
	HWND hwndWindow; //top level window for the contact or NULL if no window exists
	int uState; // see window states
	void *local; // used to store pointer to custom data
} MessageWindowOutputData;


#endif

#define MS_MSG_FORWARDMESSAGE  "SRMsg/ForwardMessage"

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

#define MBF_DISABLED		0x01

#define TEMPLATES_MODULE "tabSRMM_Templates"
#define RTLTEMPLATES_MODULE "tabSRMM_RTLTemplates"

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


/* temporary HPP API for emulating message log */

#define MS_HPP_EG_WINDOW "History++/ExtGrid/NewWindow"
#define MS_HPP_EG_EVENT  "History++/ExtGrid/Event"
#define MS_HPP_EG_UTILS  "History++/ExtGrid/Utils"
#define MS_HPP_EG_OPTIONSCHANGED "History++/ExtGrid/OptionsChanged"
#define MS_HPP_EG_NOTIFICATION   "History++/ExtGrid/Notification"

/*
 * encryption status bar indicator
 */

extern HANDLE hHookIconPressedEvt;
extern int status_icon_list_size;

int SI_InitStatusIcons();
int SI_DeinitStatusIcons();

int  GetStatusIconsCount();
void DrawStatusIcons(struct MessageWindowData *dat, HDC hdc, RECT r, int gap);
void SI_CheckStatusIconClick(struct MessageWindowData *dat, HWND hwndFrom, POINT pt, RECT rc, int gap, int code);


