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

#define __MATHMOD_SUPPORT 1
// #define _RELEASE_BUILD 1

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

#define GT_SELECTION 2
#define ST_SELECTION 2
#define ST_DEFAULT 0
#define ST_KEEPUNDO 1
#define CFM_WEIGHT 0x0040000
#define SBT_TOOLTIPS 0x0800
#define DFCS_HOT 0x1000

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

#ifndef _WIN32_IE
typedef struct tagNMMOUSE {
    NMHDR   hdr;
    DWORD_PTR dwItemSpec;
    DWORD_PTR dwItemData;
    POINT   pt;
    LPARAM  dwHitInfo; // any specifics about where on the item or control the mouse is
} NMMOUSE, *LPNMMOUSE;
#endif

typedef struct _settextex {
    DWORD	flags;
    UINT	codepage;
} SETTEXTEX;

#endif

#define NR_LOGICONS 8
#define NR_BUTTONBARICONS 29
#define NR_SIDEBARICONS 10

#include <richedit.h>
#include <richole.h>
#include "m_tabsrmm.h"
#include "templates.h"

#define MSGERROR_CANCEL	0
#define MSGERROR_RETRY	    1
#define MSGERROR_SENDLATER  2

#define HISTORY_INITIAL_ALLOCSIZE 300

struct NewMessageWindowLParam {
	HANDLE hContact;
	int isSend;
	const char *szInitialText;
	int iTabID;				// XXX mod: tab support
	int iTabImage;			// XXX mod tabs...
    int iActivate;
    TCITEM item;
	struct ContainerWindowData *pContainer;		// parent container description
    BOOL bWantPopup;
    HANDLE hdbEvent;
};

// flags for the container dwFlags 
#define CNT_MOUSEDOWN 1
#define CNT_NOTITLE 2
#define CNT_HIDETABS 4
#define CNT_SIDEBAR 8
#define CNT_NOFLASH 16
#define CNT_STICKY 32
#define CNT_DONTREPORT 64
#define CNT_FLASHALWAYS 128
#define CNT_TRANSPARENCY 256
#define CNT_TITLE_PRIVATE 512
#define CNT_GLOBALSETTINGS 1024
#define CNT_GLOBALSIZE 2048
#define CNT_INFOPANEL 4096
#define CNT_NOSOUND 8192
#define CNT_SYNCSOUNDS 16384
#define CNT_DEFERREDCONFIGURE 32768
#define CNT_CREATE_MINIMIZED 65536
#define CNT_NEED_UPDATETITLE 131072
#define CNT_DEFERREDSIZEREQUEST 262144
#define CNT_DONTREPORTUNFOCUSED 524288
#define CNT_ALWAYSREPORTINACTIVE 1048576
// #define CNT_SINGLEROWTABCONTROL 0x200000     ** FREE **
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

#define CNT_FLAGS_DEFAULT (CNT_HIDETABS)
#define CNT_TRANS_DEFAULT 0x00ff00ff

#define CNT_CREATEFLAG_CLONED 1
#define CNT_CREATEFLAG_MINIMIZED 2

#define MWF_LOG_ALL (MWF_LOG_SHOWNICK | MWF_LOG_SHOWTIME | MWF_LOG_SHOWSECONDS | \
        MWF_LOG_SHOWDATES | MWF_LOG_INDENT | MWF_LOG_TEXTFORMAT | MWF_LOG_SYMBOLS | MWF_LOG_INOUTICONS | \
        MWF_LOG_SHOWICONS | MWF_LOG_GRID | MWF_LOG_INDIVIDUALBKG | MWF_LOG_GROUPMODE)
        
#define MWF_LOG_DEFAULT (MWF_LOG_SHOWTIME | MWF_LOG_SHOWNICK | MWF_LOG_SHOWDATES | MWF_LOG_SYMBOLS)

struct ProtocolData {
    char szName[30];
    int  iFirstIconID;
};

#define EM_SUBCLASSED             (WM_USER+0x101)
#define EM_SEARCHSCROLLER         (WM_USER+0x103)
#define EM_VALIDATEBOTTOM         (WM_USER+0x104)
#define EM_THEMECHANGED           (WM_USER+0x105)

#define HM_EVENTSENT         (WM_USER+10)
#define DM_REMAKELOG         (WM_USER+11)
#define HM_DBEVENTADDED      (WM_USER+12)
#define DM_SETINFOPANEL      (WM_USER+13)
#define DM_OPTIONSAPPLIED    (WM_USER+14)
#define DM_SPLITTERMOVED     (WM_USER+15)
#define DM_UPDATETITLE       (WM_USER+16)
#define DM_APPENDTOLOG       (WM_USER+17)
#define DM_ERRORDECIDED      (WM_USER+18)
#define DM_SCROLLLOGTOBOTTOM (WM_USER+19)
#define DM_TYPING            (WM_USER+20)
#define DM_UPDATEWINICON     (WM_USER+21)
#define DM_UPDATELASTMESSAGE (WM_USER+22)

#define DM_SELECTTAB		 (WM_USER+23)
#define DM_CLOSETABATMOUSE   (WM_USER+24)
#define DM_SAVELOCALE        (WM_USER+25)
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
#define DM_PICTURECHANGED    (WM_USER+53)
#define DM_PROTOACK          (WM_USER+54)
#define DM_RETRIEVEAVATAR    (WM_USER+55)
#define DM_CONFIGURETOOLBAR  (WM_USER+56)
#define DM_LOADBUTTONBARICONS (WM_USER+57)
#define DM_ACTIVATETOOLTIP   (WM_USER+58)
#define DM_UINTOCLIPBOARD   (WM_USER+59)
#define DM_SPLITTEREMERGENCY (WM_USER+60)
#define DM_RECALCPICTURESIZE (WM_USER+61)
#define DM_FORCEDREMAKELOG   (WM_USER+62)
#define DM_QUERYFLAGS        (WM_USER+63)
#define DM_STATUSBARCHANGED  (WM_USER+64)
#define DM_SAVEMESSAGELOG    (WM_USER+65)
#define DM_CHECKAUTOCLOSE    (WM_USER+66)
#define DM_UPDATEMETACONTACTINFO (WM_USER+67)
#define DM_SETICON           (WM_USER+68)
#define DM_MULTISENDTHREADCOMPLETE (WM_USER+69)
#define DM_SECURE_CHANGED    (WM_USER+70)
#define DM_QUERYSTATUS       (WM_USER+71)
#define DM_SETPARENTDIALOG   (WM_USER+72)
#define DM_HANDLECLISTEVENT  (WM_USER+73)
#define DM_TRAYICONNOTIFY    (WM_USER+74)
#define DM_REMOVECLISTEVENT  (WM_USER+75)
#define DM_GETWINDOWSTATE    (WM_USER+76)
#define DM_DOCREATETAB       (WM_USER+77)
#define DM_LOADLOCALE        (WM_USER+78)
#define DM_REPLAYQUEUE       (WM_USER+79)
#define DM_HKDETACH          (WM_USER+80)
#define DM_HKSAVESIZE        (WM_USER+81)
#define DM_SETSIDEBARBUTTONS (WM_USER+82)
#define DM_REFRESHTABINDEX   (WM_USER+83)
#define DM_CHANGELOCALAVATAR (WM_USER+84)

#define DM_SC_BUILDLIST      (WM_USER+100)
#define DM_SC_INITDIALOG     (WM_USER+101)
#define MINSPLITTERY         52
#define MINLOGHEIGHT         30

// wParam values for DM_SELECTTAB

#define DM_SELECT_NEXT		 1
#define DM_SELECT_PREV		 2

#define DM_SELECT_BY_HWND	 3		// lParam specifies hwnd
#define DM_SELECT_BY_INDEX   4		// lParam specifies tab index

#define DM_QUERY_NEXT 1
#define DM_QUERY_MOSTRECENT 2

struct CREOleCallback {
	IRichEditOleCallbackVtbl *lpVtbl;
	unsigned refCount;
	IStorage *pictStg;
	int nextStgId;
};

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

extern const int msgDlgFontCount;

#define LOADHISTORY_UNREAD    0
#define LOADHISTORY_COUNT     1
#define LOADHISTORY_TIME      2

#define SRMSGSET_AUTOPOPUP         "AutoPopup"
#define SRMSGDEFSET_AUTOPOPUP      0
#define SRMSGSET_AUTOMIN           "AutoMin"
#define SRMSGDEFSET_AUTOMIN        0
#define SRMSGSET_SENDONENTER       "SendOnEnter"
#define SRMSGDEFSET_SENDONENTER    1
#define SRMSGSET_MSGTIMEOUT        "MessageTimeout"
#define SRMSGDEFSET_MSGTIMEOUT     10000
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

#define SRMSGMOD "SRMsg"
#define SRMSGMOD_T "Tab_SRMsg"
#define FONTMODULE "TabSRMM_Fonts"

#define IDM_STAYONTOP (WM_USER + 1)
#define IDM_NOTITLE (WM_USER + 2)
#define IDM_MOREOPTIONS (WM_USER +4)

typedef DWORD (WINAPI *PSLWA)(HWND, DWORD, BYTE, DWORD);

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

#define BUDDYPOUNCE_SERVICENAME "BuddyPounce/AddToPounce"

struct MsgLogIcon {
    HBITMAP hBmp, hoBmp;
    HDC hdc, hdcMem;
    HBRUSH hBkgBrush;
};

#define IDI_HISTORY 1           /* history button */
#define IDI_TIMESTAMP 2         /* message log options */
#define IDI_ADDCONTACT 3        /* add contact symbol (not a button anymore) */
#define IDI_MULTISEND 4         /* multisend (not a button anymore, symbol still needed) */
#define IDI_TYPING 5            /* typing notify icon in the statusbar */
#define IDI_QUOTE 6             /* quote button */
#define IDI_SAVE 7              /* close tab (with non-empty message area) */
#define IDI_CHECK 8             /* send button */
#define IDI_CONTACTPIC 9        /* avatar button */
#define IDI_CLOSEMSGDLG 10      /* close tab (when message area is empty) */
#define IDI_USERMENU 11         /* user menu pulldown */
#define IDI_MSGERROR 12         /* error (for the message log and tab image) */
#define IDI_ICONIN 13           /* incoming msg icon (log) */
#define IDI_ICONOUT 14          /* outgoing msg icon (log) */
#define IDI_SMILEYICON 15       /* smiley button fallback */
#define IDI_SELFTYPING_ON 16    /* sending typing notify is on */
#define IDI_SELFTYPING_OFF 17   /* sending typing notify is off */
#define IDI_CONTAINER 18        /* static container icon */
#define IDI_SECUREIM_ENABLED 20 /* connection is secured via secureim */
#define IDI_SECUREIM_DISABLED 19    /* connection is not secured */
#define IDI_STATUSCHANGE 21     /* status change events (message log) */
#define IDI_FONTBOLD 22         /* bold */
#define IDI_FONTITALIC 23       /* italic */
#define IDI_FONTUNDERLINE 24    /* underline */
#define IDI_FONTFACE 25         /* font face (currently not in use) */
#define IDI_FONTCOLOR 26        /* font color (not in use yet) */
#define IDI_SOUNDSON  27        /* msg window sounds are enabled */
#define IDI_SOUNDSOFF 28        /* msg window sounds are disabled */
#define IDI_EMPTY 29
#define IDI_RESERVED10 30

// v2 stuff

#define IDI_SESSIONLIST                 31
#define IDI_FAVLIST                     32
#define IDI_RECENTLIST                  33
#define IDI_CONFIGSIDEBAR               34
#define IDI_USERPREFS                   35

#define IDB_UNKNOWNAVATAR 100   /* fallback image for non-existing avatars (BITMAP) */
#define IDS_IDENTIFY 101        /* string resource to identify icon pack */

WCHAR *Utf8_Decode(const char *str);
char *Utf8_Encode(const WCHAR *str);

struct CPTABLE {
    UINT cpId;
    char *cpName;
};

#define LOI_TYPE_FLAG 1
#define LOI_TYPE_SETTING 2

struct LISTOPTIONSGROUP {
    LRESULT handle;
    char *szName;
};

struct LISTOPTIONSITEM {
    LRESULT handle;
    char *szName;
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

// fixed sidebaritem identifiers

#define IDC_SBAR_SLIST                  1111
#define IDC_SBAR_FAVORITES              1112
#define IDC_SBAR_RECENT                 1113
#define IDC_SBAR_SETUP                  1114
#define IDC_SBAR_USERPREFS              1115
#define IDC_SBAR_TOGGLEFORMAT           1117

struct SIDEBARITEM {
    UINT uId;
    DWORD dwFlags;
    HICON *hIcon;
    char szTip[128];
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


