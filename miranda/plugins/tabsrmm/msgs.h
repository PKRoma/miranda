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
    NMHDR   hdr;
    DWORD_PTR dwItemSpec;
    DWORD_PTR dwItemData;
    POINT   pt;
    LPARAM  dwHitInfo; // any specifics about where on the item or control the mouse is
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
#include "m_tabsrmm.h"
#include "templates.h"

#define MSGERROR_CANCEL	0
#define MSGERROR_RETRY	    1
#define MSGERROR_SENDLATER  2

#define HISTORY_INITIAL_ALLOCSIZE 300

struct NewMessageWindowLParam {
	HANDLE hContact;
	int isWchar;
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

#define MWF_LOG_ALL (MWF_LOG_SHOWNICK | MWF_LOG_SHOWTIME | MWF_LOG_SHOWSECONDS | \
        MWF_LOG_SHOWDATES | MWF_LOG_INDENT | MWF_LOG_TEXTFORMAT | MWF_LOG_SYMBOLS | MWF_LOG_INOUTICONS | \
        MWF_LOG_SHOWICONS | MWF_LOG_GRID | MWF_LOG_INDIVIDUALBKG | MWF_LOG_GROUPMODE)
        
#define MWF_LOG_DEFAULT (MWF_LOG_SHOWTIME | MWF_LOG_SHOWNICK | MWF_LOG_SHOWDATES | MWF_LOG_SYMBOLS | MWF_LOG_INDIVIDUALBKG | MWF_LOG_GRID | MWF_LOG_GROUPMODE)

struct ProtocolData {
    char szName[30];
    int  iFirstIconID;
};

#define EM_SUBCLASSED             (WM_USER+0x101)
#define EM_SEARCHSCROLLER         (WM_USER+0x103)
#define EM_VALIDATEBOTTOM         (WM_USER+0x104)
#define EM_THEMECHANGED           (WM_USER+0x105)
#define EM_UNSUBCLASSED			  (WM_USER+0x106)

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
//#define DM_PICTURECHANGED    (WM_USER+53) **free**
#define DM_PROTOACK          (WM_USER+54)
//#define DM_RETRIEVEAVATAR    (WM_USER+55) **free**
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
#define DM_PROTOAVATARCHANGED (WM_USER+84)
#define DM_SMILEYOPTIONSCHANGED (WM_USER+85)
#define DM_MYAVATARCHANGED	 (WM_USER+86)
#define DM_PRINTCLIENT		 (WM_USER+87)
#define DM_IEVIEWOPTIONSCHANGED (WM_USER+88)
#define DM_SPLITTERMOVEDGLOBAL (WM_USER+89)
#define DM_DOCREATETAB_CHAT    (WM_USER+90)
#define DM_CLIENTCHANGED       (WM_USER+91)
#define DM_PLAYINCOMINGSOUND   (WM_USER+92)

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
#define TIMERID_HOVER 10

#define SRMSGMOD "SRMsg"
#define SRMSGMOD_T "Tab_SRMsg"
#define FONTMODULE "TabSRMM_Fonts"

#define IDM_STAYONTOP (WM_USER + 1)
#define IDM_NOTITLE (WM_USER + 2)
#define IDM_MOREOPTIONS (WM_USER +4)

typedef DWORD (WINAPI *PSLWA)(HWND, DWORD, BYTE, DWORD);
typedef BOOL (WINAPI *PULW)(HWND, HDC, POINT *, SIZE *, HDC, POINT *, COLORREF, BLENDFUNCTION *, DWORD);
typedef BOOL (WINAPI *PFWEX)(FLASHWINFO *);
typedef BOOL (WINAPI *PAB)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
typedef BOOL (WINAPI *PGF)(HDC, PTRIVERTEX, ULONG, PVOID, ULONG, ULONG);

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

#include "icons/iconsxp/resource.h"         // icon pack values

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
    TCHAR  szTip[128];
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

/*
 * skinning stuff - most is taken from clist_nicer as I'am using basically the same skinning engine
 */

typedef struct _tagImageItem {
    char szName[40];
    HBITMAP hbm;
    BYTE bLeft, bRight, bTop, bBottom;      // sizing margins
    BYTE alpha;
    DWORD dwFlags;
    HDC hdc;
    HBITMAP hbmOld;
    LONG inner_height, inner_width;
    LONG width, height;
    BLENDFUNCTION bf;
    BYTE bStretch;
	LONG glyphMetrics[4];
    struct _tagImageItem *nextItem;
	HBRUSH fillBrush;
} ImageItem;

typedef struct {
    char szName[40];
    char szDBname[40];
    int statusID;

    BYTE GRADIENT;
    BYTE CORNER;

    DWORD COLOR;
    DWORD COLOR2;

    BYTE COLOR2_TRANSPARENT;

    DWORD TEXTCOLOR;

    int ALPHA;

    int MARGIN_LEFT;
    int MARGIN_TOP;
    int MARGIN_RIGHT;
    int MARGIN_BOTTOM;

    BYTE IGNORED;
    BYTE RADIUS;
    ImageItem *imageItem;
} StatusItems_t;

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
#define ID_EXTBK_LAST 20

#define CLCDEFAULT_GRADIENT 0
#define CLCDEFAULT_CORNER 0

#define CLCDEFAULT_COLOR 0xE0E0E0
#define CLCDEFAULT_COLOR2 0xE0E0E0

#define CLCDEFAULT_TEXTCOLOR 0x000000

#define CLCDEFAULT_COLOR2_TRANSPARENT 1

#define CLCDEFAULT_ALPHA 85
#define CLCDEFAULT_MRGN_LEFT 0
#define CLCDEFAULT_MRGN_TOP 0
#define CLCDEFAULT_MRGN_RIGHT 0
#define CLCDEFAULT_MRGN_BOTTOM 0
#define CLCDEFAULT_IGNORE 1

// FLAGS
#define CORNER_NONE 0
#define CORNER_ACTIVE 1
#define CORNER_TL 2
#define CORNER_TR 4
#define CORNER_BR 8
#define CORNER_BL 16

#define GRADIENT_NONE 0
#define GRADIENT_ACTIVE 1
#define GRADIENT_LR 2
#define GRADIENT_RL 4
#define GRADIENT_TB 8
#define GRADIENT_BT 16

#define IMAGE_PERPIXEL_ALPHA 1
#define IMAGE_FLAG_DIVIDED 2
#define IMAGE_FILLSOLID 4
#define IMAGE_GLYPH 8

#define IMAGE_STRETCH_V 1
#define IMAGE_STRETCH_H 2
#define IMAGE_STRETCH_B 4

#define SESSIONTYPE_IM 1
#define SESSIONTYPE_CHAT 2

#define SIDEBARWIDTH         30

static void __fastcall IMG_RenderImageItem(HDC hdc, ImageItem *item, RECT *rc);
void IMG_InitDecoder();
static void LoadSkinItems(char *file);
static void IMG_CreateItem(ImageItem *item, const char *fileName, HDC hdc);
static void IMG_LoadItems(char *szFileName);
void IMG_DeleteItems();
void DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, BYTE alpha, DWORD basecolor2, BOOL transparent, DWORD FLG_GRADIENT, DWORD FLG_CORNER, BYTE RADIUS, ImageItem *imageItem);
void SkinDrawBG(HWND hwndClient, HWND hwnd, struct ContainerWindowData *pContainer, RECT *rcClient, HDC hdcTarget);

void ReloadContainerSkin();

#endif
