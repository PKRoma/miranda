/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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
#if defined(UNICODE)
#define _UNICODE
#endif

#ifndef _CHAT_H_
#define _CHAT_H_

#pragma warning( disable : 4786 ) // limitation in MSVC's debugger.

#define WIN32_LEAN_AND_MEAN	
#define _WIN32_WINNT 0x0501

#include "AggressiveOptimize.h"
#include <windows.h>
#include <commctrl.h>
#include <RichEdit.h>
#include <ole2.h>
#include <richole.h>
#include <malloc.h>
#include <commdlg.h>
#include <time.h>
#include <stdio.h>
#include <shellapi.h>
#include "../../include/newpluginapi.h"
#include "../../include/m_system.h"
#include "../../include/m_options.h"
#include "../../include/m_database.h"
#include "../../include/m_utils.h"
#include "../../include/m_langpack.h"
#include "../../include/m_skin.h"
#include "../../include/m_button.h"
#include "../../include/m_protomod.h"
#include "../../include/m_protosvc.h"
#include "../../include/m_addcontact.h"
#include "../../include/m_clist.h"
#include "../../include/m_clui.h"
#include "../../include/m_popup.h"
#include "resource.h"
#include "m_chat.h"
#include "m_smileyadd.h"
#include "m_uninstaller.h"

#ifndef NDEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

//defines
#define OPTIONS_FONTCOUNT 17
#define OPTIONS_ICONCOUNT 25
#define GC_UPDATEWINDOW			(WM_USER+100)
#define GC_SPLITTERMOVED		(WM_USER+101)
#define GC_CLOSEWINDOW			(WM_USER+103)
#define GC_GETITEMDATA			(WM_USER+104)
#define GC_SETITEMDATA			(WM_USER+105)
#define GC_SETSTATUSBARTEXT		(WM_USER+106)
#define GC_SETVISIBILITY		(WM_USER+107)
#define GC_SETWNDPROPS			(WM_USER+108)
#define GC_REDRAWLOG			(WM_USER+109)
#define GC_FIREHOOK				(WM_USER+110)
#define GC_FILTERFIX			(WM_USER+111)
#define GC_CHANGEFILTERFLAG		(WM_USER+112)
#define GC_SHOWFILTERMENU		(WM_USER+113)
#define GC_CASCADENEWWINDOW		(WM_USER+114)
#define GC_SAVEWNDPOS			(WM_USER+115)
#define GC_HIGHLIGHT			(WM_USER+116)
#define	GC_NICKLISTREINIT		(WM_USER+117)
#define GC_UPDATENICKLIST		(WM_USER+118)
#define GC_SHOWCOLORCHOOSER		(WM_USER+119)
#define EM_SUBCLASSED			(WM_USER+200)
#define EM_UNSUBCLASSED			(WM_USER+201)
#define EM_ACTIVATE				(WM_USER+202)

#define GC_EVENT_HIGHLIGHT		0x1000

// special service for tweaking performance
#define MS_GC_GETEVENTPTR  "GChat/GetNewEventPtr"
typedef int (*GETEVENTFUNC)(WPARAM wParam, LPARAM lParam);
typedef struct  {
	GETEVENTFUNC pfnAddEvent;
}GCPTRS;

//structs


typedef struct  {
	char *		pszModule;						
	char *		pszName;
	char *		pszID;
	char *		pszStatusbarText;
	int			iType;
	DWORD		dwItemData;
}NEWCHATWINDOWLPARAM;

typedef struct  {
	BOOL		bIsMe;
	BOOL		bAddLog;
	BOOL		bBroadcasted;
	char *		pszName;
	char *		pszUID;
	char *		pszStatus;	
	char *		pszText;
	char *		pszUserInfo;
	DWORD		dwItemData;
	time_t	time;


}NEWEVENTLPARAM;

typedef struct  {
	char*		pszModule;
	char*		pszModDispName;
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
	int			iMaxText;
}MODULE;

typedef struct  MODULE_INFO_TYPE{
	MODULE	data;
	struct MODULE_INFO_TYPE *next;
}MODULEINFO;

typedef struct{
	LOGFONT	lf;
	COLORREF color;
}FONTINFO;

typedef struct  LOG_INFO_TYPE{
	char*		pszText;
	char*		pszNick;
//	char*		pszUID;		do we need?
	char*		pszStatus;
	char *		pszUserInfo;
	BOOL		bIsMe;
	BOOL		bIsHighlighted;
	time_t		time;
	int			iType;
	struct LOG_INFO_TYPE *next;
	struct LOG_INFO_TYPE *prev;
}LOGINFO;

typedef struct STATUSINFO_TYPE{
	char*		pszGroup;
	WORD		Status;
	HTREEITEM	hItem;
	struct STATUSINFO_TYPE *next;
}STATUSINFO;


typedef struct  USERINFO_TYPE{
	char *		pszNick;
	char *		pszUID;
	WORD 		Status;	
	HTREEITEM	hItem;
	struct USERINFO_TYPE *next;

}USERINFO;



struct CREOleCallback {
	IRichEditOleCallbackVtbl *lpVtbl;
	unsigned refCount;
	IStorage *pictStg;
	int nextStgId;
};

typedef struct  {
	BOOL		bFilterEnabled;
	BOOL		windowWasCascaded;
	BOOL		bFGSet;
	BOOL		bBGSet;
	int			nUsersInNicklist;
	int			iLogFilterFlags;
	int			iType;
	char*		pszModule;
	char*		pszID;
	char*		pszName;
	char*		pszStatusbarText;
	char*		pszTopic;
	USERINFO*	pMe;
	int			iSplitterY;
	int			iSplitterX;
	int			iFG;
	int			iBG;
	time_t		LastTime;
	LPARAM		ItemData;	
	STATUSINFO* pStatusList;
	USERINFO*	pUserList;
	LOGINFO*	pEventListStart;
	LOGINFO*	pEventListEnd;
	UINT		iEventCount;
	HWND		hwndStatus;
	HANDLE		hContact;

}CHATWINDOWDATA;

typedef struct GlobalLogSettings_t {
	BOOL		ShowTime;
    BOOL		ShowTimeIfChanged;
	BOOL		LoggingEnabled;
	BOOL		FlashWindow;
	BOOL		HighlightEnabled;
	BOOL		LogIndentEnabled;
	BOOL		StripFormat;
	BOOL		SoundsFocus;
	BOOL		PopUpInactiveOnly;
	BOOL		TrayIconInactiveOnly;
	BOOL		AddColonToAutoComplete;
	DWORD		dwIconFlags;
	DWORD		dwTrayIconFlags;
	DWORD		dwPopupFlags;
	int			LogTextIndent;
	int			LoggingLimit;
	int			iEventLimit;
	int			iPopupStyle;
	int			iPopupTimeout;
	int			iSplitterX;
	int			iSplitterY;
	int			iX;
	int			iY;
	int			iWidth;
	int			iHeight;
	char *		pszTimeStamp;
	char *		pszTimeStampLog;
	char *		pszIncomingNick;
	char *		pszOutgoingNick;
	char *		pszHighlightWords;
	char *		pszLogDir;
	HFONT		UserListFont;
	HFONT		UserListHeadingsFont;
	HFONT		MessageBoxFont;
	HFONT		NameFont;
	BOOL		bShowName;
	COLORREF	crLogBackground;
	COLORREF	crUserListColor;
	COLORREF	crUserListBGColor;
	COLORREF	crUserListHeadingsColor;
	COLORREF	crPUTextColour;
	COLORREF	crPUBkgColour;
	HICON		TagIcon1;
	HICON		TagIcon2;
	HICON		GroupOpenIcon;
	HICON		GroupClosedIcon;
};
struct GlobalLogSettings_t g_LogOptions;

typedef struct{
  MODULE* pModule;
  int xPosition;
  int yPosition;
  HWND hWndTarget;
  BOOL bForeground;
  CHATWINDOWDATA * dat;
}COLORCHOOSER;

//main.c
void				LoadIcons(void);
void				FreeIcons(void);

//colorchooser.c
BOOL CALLBACK		DlgProcColorToolWindow(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

//log.c
void				Log_StreamInEvent(HWND hwndDlg, LOGINFO* lin, CHATWINDOWDATA* dat, BOOL bAppend);
void				LoadMsgLogBitmaps(void);
void				FreeMsgLogBitmaps(void);
void				ValidateFilename (char * filename);
char *				MakeTimeStamp(char * pszStamp, time_t time);

//window.c
BOOL CALLBACK		RoomWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
int					GetTextPixelSize(char * pszText, HFONT hFont, BOOL bWidth);

//options.c
int					OptionsInit(void);
int					OptionsUnInit(void);
void				LoadMsgDlgFont(int i, LOGFONTA * lf, COLORREF * colour);
void				LoadGlobalSettings(void);
//services.c
void				HookEvents(void);
void				UnhookEvents(void);
void				CreateServiceFunctions(void);
void				CreateHookableEvents(void);
int					ModulesLoaded(WPARAM wParam,LPARAM lParam);
int					PreShutdown(WPARAM wParam,LPARAM lParam);
int					IconsChanged(WPARAM wParam,LPARAM lParam);
int					Service_Register(WPARAM wParam, LPARAM lParam);
int					Service_AddEvent(WPARAM wParam, LPARAM lParam);
int					Service_GetAddEventPtr(WPARAM wParam, LPARAM lParam);
int					Service_NewChat(WPARAM wParam, LPARAM lParam);
int					Service_ItemData(WPARAM wParam, LPARAM lParam);
int					Service_SetSBText(WPARAM wParam, LPARAM lParam);
int					Service_SetVisibility(WPARAM wParam, LPARAM lParam);
int					Hook_IconsChanged(WPARAM wParam,LPARAM lParam);

//manager.c
void				SetActiveChatWindow(char * pszID, char * pszModule);
HWND				GetActiveChatWindow(void);
BOOL				WM_AddWindow(HWND hWnd, char * pszID, char * pszModule);
BOOL				WM_RemoveWindow(char *pszID, char * pszModule);
HWND				WM_FindWindow(char *pszID, char * pszModule);
LRESULT				WM_SendMessage(char *pszID, char * pszModule, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL				WM_PostMessage(char *pszID, char * pszModule, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL				WM_BroadcastMessage(char * pszModule, UINT msg, WPARAM wParam, LPARAM lParam, BOOL bAsync);
BOOL				WM_RemoveAll (void);
void				WM_AddCommand(char *pszID, char * pszModule, const char *lpNewCommand);
char *				WM_GetPrevCommand(char *pszID, char * pszModule);
char *				WM_GetNextCommand(char *pszID, char * pszModule);
BOOL				MM_AddModule(MODULE* info);
MODULE	*			MM_FindModule(char * pszModule);
void				MM_FixColors();
BOOL				MM_RemoveAll (void);
BOOL				SM_AddStatus(STATUSINFO** pStatusList, char * pszStatus, HTREEITEM hItem);
HTREEITEM			SM_FindTVGroup(STATUSINFO* pStatusList, char* pszStatus);
WORD				SM_StringToWord(STATUSINFO* pStatusList, char* pszStatus);
char *				SM_WordToString(STATUSINFO* pStatusList, WORD Status);
BOOL				SM_RemoveAll (STATUSINFO** pStatusList);
USERINFO*			UM_AddUser(STATUSINFO* pStatusList, USERINFO** pUserList, char * pszNick, char * pszUID, char* pszStatus, HTREEITEM hItem);
USERINFO*			UM_FindUser(USERINFO* pUserList, char* pszUID);
char*				UM_FindUserAutoComplete(USERINFO* pUserList, char * pszOriginal, char* pszCurrent);
BOOL				UM_RemoveUser(USERINFO** pUserList, char* pszUID);
BOOL				UM_RemoveAll (USERINFO** ppUserList);
BOOL				LM_AddEvent(LOGINFO** ppLogListStart, LOGINFO** ppLogListEnd, char * pszNick, int iType, char * pszText, char* pszStatus, char * pszUserInfo, BOOL bIsMe, BOOL bHighlight, time_t time, int* piCount, int iLimit);
BOOL				LM_TrimLog(LOGINFO** ppLogListStart, LOGINFO** ppLogListEnd, int iCount);
BOOL				LM_RemoveAll (LOGINFO** ppLogListStart, LOGINFO** ppLogListEnd);

//clist.c
HANDLE				CList_AddRoom(char * pszModule, char * pszRoom, char * pszDisplayName, int  iType);
BOOL				CList_SetOffline(HANDLE hContact, BOOL bHide);
BOOL				CList_SetAllOffline(BOOL bHide);
int					CList_RoomDoubleclicked(WPARAM wParam,LPARAM lParam);
int					CList_EventDoubleclicked(WPARAM wParam,LPARAM lParam);
void				CList_CreateGroup(char *group);
BOOL				CList_AddEvent(HWND hwnd, HANDLE hContact, HICON Icon, HANDLE event, int type, char * fmt, ... ) ;
HANDLE				CList_FindRoom (char * pszModule, char * pszRoom) ;
int					WCCmp(char* wild, char *string);

//tools.c
char *				RemoveFormatting(char * pszText);
BOOL				DoPopupAndTrayIcon(HWND hWnd, int iEvent, CHATWINDOWDATA * dat, NEWEVENTLPARAM * nlu);
int					GetColorIndex(char * pszModule, COLORREF cr);
void				CheckColorsInModule(char * pszModule);
char*				my_strstri(char *s1, char *s2) ;
int					GetRichTextLength(HWND hwnd);

// message.c
char *				Message_GetFromStream(HWND hwndDlg, CHATWINDOWDATA* dat);
BOOL				DoRtfToTags(char * pszText, CHATWINDOWDATA * dat);

#pragma comment(lib,"comctl32.lib")

#endif