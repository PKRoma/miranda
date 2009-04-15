/*
Chat module plugin for Miranda IM

Copyright (C) 2003 J�rgen Persson

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

#ifndef _CHAT_H_
#define _CHAT_H_

#pragma warning( disable : 4786 ) // limitation in MSVC's debugger.
#pragma warning( disable : 4996 ) // limitation in MSVC's debugger.

#define WIN32_LEAN_AND_MEAN

#define _WIN32_WINNT 0x0501

#include "m_stdhdr.h"

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <process.h>
#include <ole2.h>
#include <richole.h>
#include <tom.h>
#include <commdlg.h>
#include <time.h>
#include <stdio.h>
#include <shellapi.h>
#include "../../../include/win2k.h"
#include "../../../include/newpluginapi.h"
#include "../../../include/m_system.h"
#include "../../../include/m_options.h"
#include "../../../include/m_database.h"
#include "../../../include/m_utils.h"
#include "../../../include/m_langpack.h"
#include "../../../include/m_skin.h"
#include "../../../include/m_button.h"
#include "../../../include/m_protomod.h"
#include "../../../include/m_protosvc.h"
#include "../../../include/m_addcontact.h"
#include "../../../include/m_clist.h"
#include "../../../include/m_clui.h"
//#include "../../../include/m_popup.h"
#include "chat_resource.h"
#include "m_chat.h"
#include "../API/m_ieview.h"
#include "../API/m_smileyadd.h"

#ifdef _MSC_VER
#ifndef NDEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif

//defines
#define OPTIONS_FONTCOUNT 20
#define GC_UPDATETITLE			(WM_USER+100)
#define GC_CLOSEWINDOW			(WM_USER+103)
#define GC_GETITEMDATA			(WM_USER+104)
#define GC_SETITEMDATA			(WM_USER+105)
#define GC_UPDATESTATUSBAR		(WM_USER+106)
#define GC_SETVISIBILITY		(WM_USER+107)
#define GC_SETWNDPROPS			(WM_USER+108)
#define GC_REDRAWLOG			(WM_USER+109)
#define GC_FIREHOOK				(WM_USER+110)
#define GC_FILTERFIX			(WM_USER+111)
#define GC_CHANGEFILTERFLAG		(WM_USER+112)
#define GC_SHOWFILTERMENU		(WM_USER+113)
//#define	GC_NICKLISTCLEAR		(WM_USER+117)
#define GC_REDRAWWINDOW			(WM_USER+118)
#define GC_SHOWCOLORCHOOSER		(WM_USER+119)
#define GC_ADDLOG				(WM_USER+120)
#define GC_ACKMESSAGE			(WM_USER+121)
//#define GC_ADDUSER				(WM_USER+122)
//#define GC_REMOVEUSER			(WM_USER+123)
//#define GC_NICKCHANGE			(WM_USER+124)
#define GC_UPDATENICKLIST		(WM_USER+125)
//#define GC_MODECHANGE			(WM_USER+126)
#define GC_SCROLLTOBOTTOM		(WM_USER+129)
#define GC_SESSIONNAMECHANGE	(WM_USER+131)
#define GC_SETMESSAGEHIGHLIGHT	(WM_USER+139)
#define GC_REDRAWLOG2			(WM_USER+140)
#define GC_REDRAWLOG3			(WM_USER+141)

#define EM_ACTIVATE				(WM_USER+202)

#define GC_EVENT_HIGHLIGHT		0x1000
#define STATE_TALK				0x0001

#define ICON_ACTION				0
#define ICON_ADDSTATUS			1
#define ICON_HIGHLIGHT			2
#define ICON_INFO				3
#define ICON_JOIN				4
#define ICON_KICK				5
#define ICON_MESSAGE			6
#define ICON_MESSAGEOUT			7
#define ICON_NICK				8
#define ICON_NOTICE				9
#define ICON_PART				10
#define ICON_QUIT				11
#define ICON_REMSTATUS			12
#define ICON_TOPIC				13

#define ICON_STATUS1			14
#define ICON_STATUS2			15
#define ICON_STATUS3			16
#define ICON_STATUS4			17
#define ICON_STATUS0			18
#define ICON_STATUS5			19

// special service for tweaking performance
#define MS_GC_GETEVENTPTR  "GChat/GetNewEventPtr"
typedef INT_PTR (*GETEVENTFUNC)(WPARAM wParam, LPARAM lParam);
typedef struct  {
	GETEVENTFUNC pfnAddEvent;
}GCPTRS;

//structs

typedef struct  MODULE_INFO_TYPE
{
	char*		pszModule;
	char*		pszModDispName;
	char*		pszHeader;
	BOOL		bBold;
	BOOL		bUnderline;
	BOOL		bItalics;
	BOOL		bColor;
	BOOL		bBkgColor;
	BOOL		bChanMgr;
	BOOL		bAckMsg;
	int			nColorCount;
	COLORREF*	crColors;
	HICON		hOnlineIcon;
	HICON		hOfflineIcon;
	HICON		hOnlineTalkIcon;
	HICON		hOfflineTalkIcon;
	int			OnlineIconIndex;
	int			OfflineIconIndex;
	int			iMaxText;
	struct MODULE_INFO_TYPE *next;
}
	MODULEINFO;

typedef struct COMMAND_INFO_TYPE
{
	char*  lpCommand;
	struct COMMAND_INFO_TYPE *last, *next;
}
	COMMAND_INFO;

typedef struct
{
	LOGFONT  lf;
	COLORREF color;
}
	FONTINFO;

typedef struct LOG_INFO_TYPE
{
	TCHAR*  ptszText;
	TCHAR*  ptszNick;
	TCHAR*  ptszUID;
	TCHAR*  ptszStatus;
	TCHAR*  ptszUserInfo;
	BOOL    bIsMe;
	BOOL    bIsHighlighted;
	time_t  time;
	int     iType;
	DWORD   dwFlags;
	struct  LOG_INFO_TYPE *next;
	struct  LOG_INFO_TYPE *prev;
}
	LOGINFO;

typedef struct STATUSINFO_TYPE
{
	TCHAR*  pszGroup;
	HICON   hIcon;
	WORD    Status;
	struct  STATUSINFO_TYPE *next;
}
	STATUSINFO;

typedef struct  USERINFO_TYPE
{
	TCHAR* pszNick;
	TCHAR* pszUID;
	WORD   Status;
	int    iStatusEx;
	WORD   ContactStatus;
	struct USERINFO_TYPE *next;
}
	USERINFO;

typedef struct  TABLIST_TYPE
{
	TCHAR* pszID;
	char*  pszModule;
	struct TABLIST_TYPE *next;
}
	TABLIST;

typedef struct SESSION_INFO_TYPE
{
	HWND        hWnd;

	BOOL        bFGSet;
	BOOL        bBGSet;
	BOOL        bFilterEnabled;
	BOOL        bNicklistEnabled;
	BOOL        bInitDone;

	char*       pszModule;
	TCHAR*      ptszID;
	TCHAR*      ptszName;
	TCHAR*      ptszStatusbarText;
	TCHAR*      ptszTopic;

	#if defined( _UNICODE )
		char*    pszID;		// ugly fix for returning static ANSI strings in GC_INFO
		char*    pszName;   // just to fix a bug quickly, should die after porting IRC to Unicode
	#endif

	int         iType;
	int         iFG;
	int         iBG;
	int         iSplitterY;
	int         iSplitterX;
	int         iLogFilterFlags;
    int         iLogPopupFlags;
    int         iLogTrayFlags;
	int         iDiskLogFlags;
	int         nUsersInNicklist;
	int         iEventCount;
	int         iX;
	int         iY;
	int         iWidth;
	int         iHeight;
	int         iStatusCount;

	WORD        wStatus;
	WORD        wState;
	WORD        wCommandsNum;
	DWORD       dwItemData;
	DWORD       dwFlags;
	HANDLE      hContact;
	HWND        hwndStatus;
	time_t      LastTime;
    TCHAR          szSearch[255];
    int            iSearchItem;
	COMMAND_INFO*  lpCommands;
	COMMAND_INFO*  lpCurrentCommand;
	LOGINFO*       pLog;
	LOGINFO*       pLogEnd;
	USERINFO*      pUsers;
	USERINFO*      pMe;
	STATUSINFO*    pStatuses;
	struct         ContainerWindowData *pContainer;
   int            wasTrimmed;
	struct SESSION_INFO_TYPE *next;
}	SESSION_INFO;

typedef struct
{
	char*         buffer;
	int           bufferOffset, bufferLen;
	HWND          hwnd;
	LOGINFO*      lin;
	BOOL          bStripFormat;
	BOOL          bRedraw;
	SESSION_INFO* si;
	int           crCount;
	struct MessageWindowData *dat;
}
	LOGSTREAMDATA;

struct GlobalLogSettings_t {
	HICON       hIconOverlay;
	BOOL        ShowTime;
	BOOL        ShowTimeIfChanged;
	BOOL        LoggingEnabled;
	BOOL        FlashWindow;
	BOOL        FlashWindowHightlight;
	BOOL        OpenInDefault;
	BOOL        HighlightEnabled;
	BOOL        LogIndentEnabled;
	BOOL        StripFormat;
	BOOL        SoundsFocus;
	BOOL        SkipWhenNoWindow;
	BOOL        BBCodeInPopups;
	BOOL        TrayIconInactiveOnly;
	BOOL        AddColonToAutoComplete;
	BOOL        LogLimitNames;
	BOOL        TimeStampEventColour;
	DWORD       dwIconFlags;
	int         LogTextIndent;
	long        LoggingLimit;
	int         iEventLimit;
	int         iPopupStyle;
	int         iPopupTimeout;
	int         iSplitterX;
	int         iSplitterY;
	int         iX;
	int         iY;
	int         iWidth;
	int         iHeight;
	TCHAR*      pszTimeStamp;
	TCHAR*      pszTimeStampLog;
	TCHAR*      pszIncomingNick;
	TCHAR*      pszOutgoingNick;
	TCHAR*      pszHighlightWords;
	char*       pszLogDir;
	HFONT       UserListFont;
	HFONT       UserListHeadingsFont;
	HFONT       NameFont;
	COLORREF    crLogBackground;
	COLORREF    crUserListColor;
	COLORREF    crUserListBGColor;
	COLORREF    crUserListHeadingsColor;
	COLORREF    crPUTextColour;
	COLORREF    crPUBkgColour;
	BYTE        ClassicIndicators;
	//MAD
	BYTE		LogClassicIndicators;
	BYTE		AlternativeSorting;
	BYTE		AnnoyingHighlight;
	BYTE		CreateWindowOnHighlight;
	//MAD_
	BYTE        LogSymbols;
	BYTE        ClickableNicks;
	BYTE        ColorizeNicks;
	BYTE        ColorizeNicksInLog;
	BYTE        ScaleIcons;
	BYTE        UseDividers;
	BYTE        DividersUsePopupConfig;
    BYTE        MathMod;
	COLORREF    nickColors[8];
	HBRUSH      SelectionBGBrush;
	BOOL		DoubleClick4Privat;
	BOOL		ShowContactStatus;
	BOOL		ContactStatusFirst;

};
extern struct GlobalLogSettings_t g_Settings;

typedef struct{
	MODULEINFO*   pModule;
	int           xPosition;
	int           yPosition;
	HWND          hWndTarget;
	BOOL          bForeground;
	SESSION_INFO* si;
} COLORCHOOSER;

#pragma comment(lib,"comctl32.lib")

//////////////////////////////////////////////////////////////////////////////////

#include "chatprototypes.h"
#include "../msgs.h"
#include "../nen.h"
#include "../functions.h"
#include "../msgdlgutils.h"

#if defined( _UNICODE )
	#define mir_tstrdup mir_wstrdup
#else
	#define mir_tstrdup mir_strdup
#endif

TCHAR* a2t( const char* str );
char*  t2a( const TCHAR* str, DWORD codepage );
TCHAR* a2tf( const TCHAR* str, int flags, DWORD cp );
TCHAR* replaceStr( TCHAR** dest, const TCHAR* src );
char*  replaceStrA( char** dest, const char* src );

extern HINSTANCE g_hInst;
extern char *szChatIconString;

#endif
