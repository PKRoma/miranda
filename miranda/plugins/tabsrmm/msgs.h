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


#define _STREAMTHREADING 1

#ifdef __GNUWIN32__
#define COLOR_HOTLIGHT 26
#define SB_SETICON (WM_USER+15)
#define GT_SELECTION 2
#define ST_SELECTION 2
#define TCS_BOTTOM 0x0002
#define ST_DEFAULT 0
#define CFM_WEIGHT 0x0040000

typedef struct __gettextex
{
	DWORD	cb;				// Count of bytes in the string				
	DWORD	flags;			// Flags (see the GT_XXX defines			
	UINT	codepage;		// Code page for translation (CP_ACP for sys default,
						    //  1200 for Unicode, -1 for control default)	
	LPCSTR	lpDefaultChar;	// Replacement for unmappable chars			
	LPBOOL	lpUsedDefChar;	// Pointer to flag set when def char used	
} _GETTEXTEX;

typedef struct tagNMMOUSE {
    NMHDR   hdr;
    DWORD_PTR dwItemSpec;
    DWORD_PTR dwItemData;
    POINT   pt;
    LPARAM  dwHitInfo; // any specifics about where on the item or control the mouse is
} NMMOUSE, *LPNMMOUSE;

typedef struct _settextex {
    DWORD	flags;
    UINT	codepage;
} SETTEXTEX;

#endif

#include <richedit.h>
#include <richole.h>
#include "m_tabsrmm.h"

#define MSGERROR_CANCEL	0
#define MSGERROR_RETRY	    1
#define MSGERROR_SENDLATER  2

int _DebugPopup(HANDLE hContact, const char *fmt, ...);

#define HISTORY_INITIAL_ALLOCSIZE 100

struct NewMessageWindowLParam {
	HANDLE hContact;
	int isSend;
	const char *szInitialText;
	int iTabID;				// XXX mod: tab support
	int iTabImage;			// XXX mod tabs...
    int iActivate;
    TCITEM item;
	struct ContainerWindowData *pContainer;		// parent container description
};

#define MAX_QUEUED_EVENTS 100

// flags for the container dwFlags 
#define CNT_MOUSEDOWN 1
#define CNT_NOTITLE 2
#define CNT_HIDETABS 4
#define CNT_SIZINGLOOP 8
#define CNT_NOFLASH 16
#define CNT_STICKY 32
#define CNT_DONTREPORT 64
#define CNT_FLASHALWAYS 128
#define CNT_TRANSPARENCY 256
#define CNT_TITLE_SHOWNAME 512
#define CNT_TITLE_SHOWSTATUS 1024
#define CNT_TITLE_PREFIX 2048
#define CNT_TITLE_SUFFIX 4096
#define CNT_NOSOUND 8192
#define CNT_SYNCSOUNDS 16384
#define CNT_DEFERREDCONFIGURE 32768
#define CNT_CREATE_MINIMIZED 65536
#define CNT_NEED_UPDATETITLE 131072
#define CNT_DEFERREDSIZEREQUEST 262144
#define CNT_DONTREPORTUNFOCUSED 524288
#define CNT_ALWAYSREPORTINACTIVE 1048576
#define CNT_SINGLEROWTABCONTROL 0x200000
#define CNT_DEFERREDTABSELECT 0x400000
#define CNT_CREATE_CLONED 0x800000
#define CNT_NOSTATUSBAR 0x1000000
#define CNT_NOMENUBAR 0x2000000
#define CNT_TABSBOTTOM 0x4000000

#define CNT_FLAGS_DEFAULT (CNT_HIDETABS | CNT_TITLE_SHOWNAME | CNT_TITLE_PREFIX)
#define CNT_TRANS_DEFAULT 0x00ff00ff

#define CNT_CREATEFLAG_CLONED 1
#define CNT_CREATEFLAG_MINIMIZED 2

#define MWF_LOG_ALL (MWF_LOG_SHOWNICK | MWF_LOG_SHOWTIME | MWF_LOG_SHOWSECONDS | \
        MWF_LOG_SHOWDATES | MWF_LOG_NEWLINE | MWF_LOG_INDENT | \
        MWF_LOG_UNDERLINE | MWF_LOG_SWAPNICK | MWF_LOG_SHOWICONS | MWF_LOG_GRID | MWF_LOG_INDIVIDUALBKG | MWF_LOG_LIMITAVATARHEIGHT | MWF_LOG_GROUPMODE | MWF_LOG_USERELATIVEDATES | MWF_LOG_LONGDATES)
        
#define MWF_LOG_DEFAULT (MWF_LOG_SHOWTIME | MWF_LOG_SHOWNICK | MWF_LOG_SHOWDATES)

struct ErrorDialogData {
	char title[128];
	char *pszError;
	HWND hwnd;
    BOOL bMustFreeString;
};

struct ProtocolData {
    char szName[30];
    int  iFirstIconID;
};
// XXX end mod

#define HM_EVENTSENT         (WM_USER+10)
#define DM_REMAKELOG         (WM_USER+11)
#define HM_DBEVENTADDED      (WM_USER+12)
#define DM_CASCADENEWWINDOW  (WM_USER+13)
#define DM_OPTIONSAPPLIED    (WM_USER+14)
#define DM_SPLITTERMOVED     (WM_USER+15)
#define DM_UPDATETITLE       (WM_USER+16)
#define DM_APPENDTOLOG       (WM_USER+17)
#define DM_ERRORDECIDED      (WM_USER+18)
#define DM_SCROLLLOGTOBOTTOM (WM_USER+19)
#define DM_TYPING            (WM_USER+20)
#define DM_UPDATEWINICON     (WM_USER+21)
#define DM_UPDATELASTMESSAGE (WM_USER+22)

// special for tabs...
#define DM_SELECTTAB		 (WM_USER+23)
#define DM_CLOSETABATMOUSE   (WM_USER+24)
#define DM_SAVELOCALE        (WM_USER+25)
#define DM_SETLOCALE         (WM_USER+26)
#define DM_REALLYSCROLL      (WM_USER+27)
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
#define DM_SAVEINPUTHISTORY  (WM_USER+46)
#define DM_QUERYRECENT       (WM_USER+47)
#define DM_REGISTERHOTKEYS   (WM_USER+48)
#define DM_FORCEUNREGISTERHOTKEYS (WM_USER+49)
#define DM_ADDDIVIDER        (WM_USER+50)
#define DM_LOADSPLITTERPOS   (WM_USER+51)
#define DM_CONTACTSETTINGCHANGED (WM_USER+52)
#define DM_PICTURECHANGED    (WM_USER+53)
#define DM_PICTUREACK        (WM_USER+54)
#define DM_RETRIEVEAVATAR    (WM_USER+55)
#define DM_ALIGNSPLITTERMAXLOG (WM_USER+56)
#define DM_ALIGNSPLITTERFULL (WM_USER+57)
#define DM_PICTHREADCOMPLETE (WM_USER+58)
#define DM_UINTOCLIPBOARD   (WM_USER+59)
#define DM_INSERTICON        (WM_USER+60)
#define DM_RECALCPICTURESIZE (WM_USER+61)
#define DM_FORCEDREMAKELOG   (WM_USER+62)
#define DM_QUERYFLAGS        (WM_USER+63)
#define DM_STATUSBARCHANGED  (WM_USER+64)
#define DM_SAVEMESSAGELOG    (WM_USER+65)
#define DM_CHECKAUTOCLOSE    (WM_USER+66)

#define DM_SC_BUILDLIST      (WM_USER+100)
#define DM_SC_INITDIALOG     (WM_USER+101)

#define MINSPLITTERY         50
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

BOOL CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int InitOptions(void);
// XXX container dlg proc
BOOL CALLBACK DlgProcContainer(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int InitOptions(void);
// end mod
BOOL CALLBACK ErrorDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int DbEventIsShown(struct MessageWindowData *dat, DBEVENTINFO *dbei);
void StreamInEvents(HWND hwndDlg,HANDLE hDbEventFirst,int count,int fAppend, DBEVENTINFO *dbei_s);
void LoadMsgLogIcons(void);
void FreeMsgLogIcons(void);

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

void LoadMsgDlgFont(int i,LOGFONTA *lf,COLORREF *colour);
extern const int msgDlgFontCount;

#define LOADHISTORY_UNREAD    0
#define LOADHISTORY_COUNT     1
#define LOADHISTORY_TIME      2

#define SRMSGSET_SPLIT             "Split"
#define SRMSGDEFSET_SPLIT          1
#define SRMSGSET_SHOWBUTTONLINE    "ShowButtonLine"
#define SRMSGDEFSET_SHOWBUTTONLINE 1
#define SRMSGSET_SHOWINFOLINE      "ShowInfoLine"
#define SRMSGDEFSET_SHOWINFOLINE   1
#define SRMSGSET_AUTOPOPUP         "AutoPopup"
#define SRMSGDEFSET_AUTOPOPUP      0
#define SRMSGSET_AUTOMIN           "AutoMin"
#define SRMSGDEFSET_AUTOMIN        0
#define SRMSGSET_AUTOCLOSE         "AutoClose"
#define SRMSGDEFSET_AUTOCLOSE      0
#define SRMSGSET_SAVEPERCONTACT    "SavePerContact"
#define SRMSGDEFSET_SAVEPERCONTACT 0
#define SRMSGSET_CASCADE           "Cascade"
#define SRMSGDEFSET_CASCADE        1
#define SRMSGSET_SENDONENTER       "SendOnEnter"
#define SRMSGDEFSET_SENDONENTER    1
#define SRMSGSET_CLOSEONREPLY      "CloseOnReply"
#define SRMSGDEFSET_CLOSEONREPLY   1
#define SRMSGSET_STATUSICON        "UseStatusWinIcon"
#define SRMSGDEFSET_STATUSICON     0
#define SRMSGSET_SENDBUTTON        "UseSendButton"
#define SRMSGDEFSET_SENDBUTTON     1
#define SRMSGSET_MSGTIMEOUT        "MessageTimeout"
#define SRMSGDEFSET_MSGTIMEOUT     10000
#define SRMSGSET_MSGTIMEOUT_MIN    4000 // minimum value (4 seconds)

#define SRMSGSET_LOADHISTORY       "LoadHistory"
#define SRMSGDEFSET_LOADHISTORY    LOADHISTORY_UNREAD
#define SRMSGSET_LOADCOUNT         "LoadCount"
#define SRMSGDEFSET_LOADCOUNT      10
#define SRMSGSET_LOADTIME          "LoadTime"
#define SRMSGDEFSET_LOADTIME       10

#define SRMSGSET_SHOWLOGICONS      "ShowLogIcons"
#define SRMSGDEFSET_SHOWLOGICONS   0
#define SRMSGSET_HIDENAMES         "HideNames"
#define SRMSGDEFSET_HIDENAMES      0
#define SRMSGSET_SHOWTIME          "ShowTime"
#define SRMSGDEFSET_SHOWTIME       0
#define SRMSGSET_SHOWDATE          "ShowDate"
#define SRMSGDEFSET_SHOWDATE       0
#define SRMSGSET_SHOWURLS          "ShowURLs"
#define SRMSGDEFSET_SHOWURLS       0
#define SRMSGSET_SHOWFILES         "ShowFiles"
#define SRMSGDEFSET_SHOWFILES      0
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
#define TIMEOUT_HEARTBEAT    10000

#define SRMSGMOD "SRMsg"
#define SRMSGMOD_T "Tab_SRMsg"

#define IDM_STAYONTOP (WM_USER + 1)
#define IDM_NOTITLE (WM_USER + 2)
#define IDM_NOREPORTMIN (WM_USER +3)
#define IDM_MOREOPTIONS (WM_USER +4)

typedef DWORD (WINAPI *PSLWA)(HWND, DWORD, BYTE, DWORD);

// constants for the container management functions

#define CNT_ENUM_DELETE 1           // delete the target container...
#define CNT_ENUM_RENAME 2
#define CNT_ENUM_WRITEFLAGS 4

//mathMod begin
#define SRMSGSET_MATH_BKGCOLOUR    "MathBkgColour"
#define SRMSGDEFSET_MATH_BKGCOLOUR GetSysColor(COLOR_WINDOW)
#define SRMSGSET_MATH_SUBSTITUTE_DELIMITER "MathSubsDelimiter"
#define SRMSGDEFSET_MATH_SUBSTITUTE_DELIMITER "\""
//service-function:
#define MATH_SETBKGCOLOR "Math/SetBackGroundColor"
//0x00BFF5F7
//mathMod end

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

#define NR_LOGICONS 8
#define NR_BUTTONBARICONS 14

#define IDI_HISTORY 1
#define IDI_TIMESTAMP 2
#define IDI_ADDCONTACT 3
#define IDI_MULTISEND 4
#define IDI_TYPING 5
#define IDI_QUOTE 6
#define IDI_SAVE 7
#define IDI_CHECK 8
#define IDI_CONTACTPIC 9
#define IDI_CLOSEMSGDLG 10
#define IDI_USERMENU 11
#define IDI_MSGERROR 12
#define IDI_ICONIN 13
#define IDI_ICONOUT 14
#define IDI_SMILEYICON 15
#define IDI_SELFTYPING_ON 16
#define IDI_SELFTYPING_OFF 17
#define IDI_RESERVED3 18
#define IDI_RESERVED4 19

#define IDB_UNKNOWNAVATAR 100

#define MSGDLGFONTCOUNT 19

WCHAR *Utf8Decode(const char *str);
char *Utf8Encode(const WCHAR *str);

