/*
IRC plugin for Miranda IM

Copyright (C) 2003-2005 Jurgen Persson
Copyright (C) 2007 George Hazan

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

#ifndef _IRCWIN_H_
#define _IRCWIN_H_

#define MIRANDA_VER 0x0700

#if defined( UNICODE ) && !defined( _UNICODE )
	#define _UNICODE 
#endif

// disable a lot of warnings. It should comppile on VS 6 also
#pragma warning( disable : 4076 ) 
#pragma warning( disable : 4786 ) 
#pragma warning( disable : 4996 ) 

#define WIN32_LEAN_AND_MEAN	
#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <shellapi.h>
#include <malloc.h>
#include <stdio.h>
#include <process.h>
#include <math.h>
#include <winsock.h>
#include <time.h>

#include "newpluginapi.h"
#include "m_system.h"
#include "m_protocols.h"
#include "m_protomod.h"
#include "m_protosvc.h"
#include "m_clist.h"
#include "m_options.h"
#include "m_database.h"
#include "m_utils.h"
#include "m_skin.h"
#include "m_netlib.h"
#include "m_clui.h"
#include "m_langpack.h"
#include "m_message.h"
#include "m_userinfo.h"
#include "m_addcontact.h"
#include "m_button.h"
#include "m_file.h"
#include "m_ignore.h"
#include "m_chat.h"
#include "m_icolib.h"
#include "m_ircscript.h"
#include "win2k.h"

#include "resource.h"
#include "irclib.h"
#include "commandmonitor.h"

#ifndef NDEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define IRC_UPDATELIST        (WM_USER+1)
#define IRC_QUESTIONAPPLY     (WM_USER+2)
#define IRC_QUESTIONCLOSE     (WM_USER+3)
#define IRC_ACTIVATE          (WM_USER+4)
#define IRC_INITMANAGER       (WM_USER+5)
#define IRC_REBUILDIGNORELIST (WM_USER+6)
#define IRC_UPDATEIGNORELIST  (WM_USER+7)
#define IRC_FIXIGNOREBUTTONS  (WM_USER+8)

#define IRC_QUICKCONNECT      "/QuickConnectMenu"
#define IRC_JOINCHANNEL       "/JoinChannelMenu"
#define IRC_CHANGENICK        "/ChangeNickMenu"
#define IRC_SHOWLIST          "/ShowListMenu"
#define IRC_SHOWSERVER        "/ShowServerMenu"
#define IRC_MENU1CHANNEL      "/Menu1ChannelMenu"
#define IRC_MENU2CHANNEL      "/Menu2ChannelMenu"
#define IRC_MENU3CHANNEL      "/Menu3ChannelMenu"

#if defined( _UNICODE )
	#define IRC_DEFAULT_CODEPAGE CP_UTF8
#else
	#define IRC_DEFAULT_CODEPAGE CP_ACP
#endif

#define STR_QUITMESSAGE  "\002Miranda IM!\002 Smaller, Faster, Easier. http://miranda-im.org"
#define STR_USERINFO     "I'm a happy Miranda IM user! Get it here: http://miranda-im.org"
#define STR_AWAYMESSAGE  "I'm away from the computer." // Default away
#define DCCSTRING        " (DCC)"

#define DCC_CHAT		1
#define DCC_SEND		2

#define FILERESUME_CANCEL	11

using namespace std;
using namespace irc;

struct _A2T
{
	_A2T( const char* s ) :
		buf( mir_a2t( s ))
		{}

	_A2T( const char* s, int cp ) :
		buf( mir_a2t_cp( s, cp ))
		{}

	_A2T( const String& s ) :
		buf( mir_a2t( s.c_str() ))
		{}

	~_A2T()
	{	mir_free(buf);
	}

	__inline operator TCHAR*() const
	{	return buf;
	}

	__inline operator TString() const
	{	return TString(buf);
	}

	TCHAR* buf;
};

struct _T2A
{
	_T2A( const TCHAR* s ) :
		buf( mir_t2a(s) )
		{}

	_T2A( const TCHAR* s, int cp ) :
		buf( mir_t2a_cp( s, cp ))
		{}

	_T2A( const TString& s ) :
		buf( mir_t2a(s.c_str()))
		{}

	~_T2A()
	{	mir_free(buf);
	}

	__inline operator char*() const
	{	return buf;
	}

	__inline operator String() const
	{	return String(buf);
	}

	char* buf;
};

// special service for tweaking performance, implemented in chat.dll
#define MS_GC_GETEVENTPTR  "GChat/GetNewEventPtr"
typedef int (*GETEVENTFUNC)(WPARAM wParam, LPARAM lParam);
typedef struct  {
	GETEVENTFUNC pfnAddEvent;
}
	GCPTRS;

#define IP_AUTO       1
#define IP_MANUAL     2

struct IPRESOLVE      // Contains info about the channels
{
	IPRESOLVE( const char* _addr, int _type ) :
		sAddr( _addr ),
		iType( _type )
	{}

	~IPRESOLVE()
	{}

	String sAddr;
	int    iType;
};

struct CHANNELINFO   // Contains info about the channels
{
	TCHAR* pszTopic;
	TCHAR* pszMode;
	TCHAR* pszPassword;
	TCHAR* pszLimit;
	int    codepage;
};

struct SERVER_INFO  // Contains info about different servers
{
	char* Group;
	char* Address;
	char* Name;
	char* PortStart;
	char* PortEnd;	
	int iSSL;
};

struct PERFORM_INFO  // Contains 'Perform buffer' for different networks
{
	PERFORM_INFO( const char* szSetting, const TCHAR* value ) :
		mSetting( szSetting ),
		mText( value )
	{}

	~PERFORM_INFO()
	{}

	String mSetting;
	TString mText;
};

struct CONTACT // Contains info about users
{
	TCHAR* name;
	TCHAR* user;
	TCHAR* host;
	bool ExactOnly;
	bool ExactWCOnly;
	bool ExactNick;
};

struct PREFERENCES  // Preferences structure
{
	char     ServerName[100];
	char     Password [500];
	TCHAR    IdentSystem[10];
	char     Network[30];
	char     PortStart[10];
	char     PortEnd[10];
	int      IdentTimer;
	int      iSSL;
	TCHAR    IdentPort[10];
	TCHAR    RetryWait[10];
	TCHAR    RetryCount[10];
			 
	TCHAR    Nick[30];
	TCHAR    AlternativeNick[30];
	TCHAR    Name[200];
	TCHAR    UserID[200];
	TCHAR    QuitMessage[400];
	TCHAR    UserInfo[500];
	char     MyHost[50];
	char     MySpecifiedHost[500];
	char     MySpecifiedHostIP[50];
	char     MyLocalHost[50];
	TCHAR*   Alias;
	int      ServerComboSelection;
	int      QuickComboSelection;
	int      OnlineNotificationTime;
	int      OnlineNotificationLimit;
	BYTE     ScriptingEnabled;
	BYTE     IPFromServer;
	BYTE     ShowAddresses;
	BYTE     DisconnectDCCChats;
	BYTE     DisableErrorPopups;
	BYTE     RejoinChannels;
	BYTE     RejoinIfKicked;
	BYTE     HideServerWindow;
	BYTE     Ident;
	BYTE     Retry;
	BYTE     DisableDefaultServer;
	BYTE     AutoOnlineNotification;
	BYTE     SendKeepAlive;
	BYTE     JoinOnInvite;
	BYTE     Perform;
	BYTE     ForceVisible;
	BYTE     Ignore;
	BYTE     IgnoreChannelDefault;
	BYTE     UseServer;
	BYTE     DCCFileEnabled;
	BYTE     DCCChatEnabled;
	BYTE     DCCChatAccept;
	BYTE     DCCChatIgnore;
	BYTE     DCCPassive;
	BYTE     ManualHost;
	BYTE     OldStyleModes;
	BYTE     ChannelAwayNotification;
	BYTE     SendNotice;
	BYTE     UtfAutodetect;
	int      Codepage;
	POINT    ListSize;
	COLORREF colors[16];
	HICON    hIcon[13];
};

//main.cpp
extern char* IRCPROTONAME; 
extern char* ALTIRCPROTONAME;
extern char* pszServerFile;
extern char  mirandapath[MAX_PATH];
extern DWORD mirVersion;

extern CIrcSession  g_ircSession;

extern CRITICAL_SECTION cs;
extern CRITICAL_SECTION m_gchook;

extern HMODULE      m_ssleay32;
extern HINSTANCE    g_hInstance;	
extern PREFERENCES* prefs;	
extern PLUGININFOEX pluginInfo;

void   UpgradeCheck(void);

//services.cpp
extern TString StatusMessage;
extern bool    bMbotInstalled;
extern int     iTempCheckTime;
extern HANDLE  hMenuQuick, hMenuServer, hMenuJoin, hMenuNick, hMenuList;
extern HANDLE  hNetlib, hNetlibDCC;

void   HookEvents(void);
void   UnhookEvents(void);
void   CreateServiceFunctions(void);
void   ConnectToServer(void);
void   DisconnectFromServer(void);
int    Service_GCEventHook(WPARAM wParam,LPARAM lParam);

//options.cpp
extern UINT_PTR	OnlineNotifTimer;
extern UINT_PTR	OnlineNotifTimer3;

void    InitPrefs(void);
void    UnInitOptions(void);
int     InitOptionsPages(WPARAM wParam,LPARAM lParam);
void    AddIcons(void);
HICON   LoadIconEx(int iIndex);
HANDLE  GetIconHandle(int iconId);
void    RewriteIgnoreSettings( void );
void    InitIgnore(void);

//tools.cpp
int     WCCmp(const TCHAR* wild, const TCHAR* string);
char*   IrcLoadFile(char * szPath);
void    AddToJTemp(TString sCommand);
TString GetWord(const TCHAR* text, int index);
TString ReplaceString (TString text, const TCHAR* replaceme, const TCHAR* newword);
bool    IsChannel(TString sName); 
TCHAR*  GetWordAddress(const TCHAR* text, int index);
TString RemoveLinebreaks(TString Message);
TCHAR*  my_strstri(const TCHAR *s1, const TCHAR *s2) ;
TCHAR*  DoColorCodes (const TCHAR* text, bool bStrip, bool bReplacePercent);
int     DoEvent(int iEvent, const TCHAR* pszWindow, const TCHAR* pszNick, const TCHAR* pszText, const TCHAR* pszStatus, const TCHAR* pszUserInfo, DWORD dwItemData, bool bAddToLog, bool bIsMe,time_t timestamp = 1);
int     CallChatEvent(WPARAM wParam, LPARAM lParam);
TString ModeToStatus(int sMode);
TString PrefixToStatus(int cPrefix);
void    SetChatTimer(UINT_PTR &nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc);
void    KillChatTimer(UINT_PTR &nIDEvent);
int     SetChannelSBText(TString sWindow, CHANNELINFO * wi);
TString MakeWndID(const TCHAR* sWindow);
bool    FreeWindowItemData(TString window, CHANNELINFO* wis);
bool    AddWindowItemData(TString window, const TCHAR* pszLimit, const TCHAR* pszMode, const TCHAR* pszPassword, const TCHAR* pszTopic);
void    FindLocalIP(HANDLE con);
void    DoUserhostWithReason(int type, TString reason, bool bSendCommand, TString userhostparams, ...);
TString GetNextUserhostReason(int type);
void    ClearUserhostReasons(int type);
TString PeekAtReasons(int type);
char*   rtrim( char *string );

#if defined( _UNICODE )
String  ReplaceString (String text, const char* replaceme, const char* newword);
bool    IsChannel(String sName); 
String  GetWord(const char* text, int index);
char*   GetWordAddress(const char* text, int index);
#endif

//clist.cpp
HANDLE CList_AddContact(struct CONTACT * user, bool InList, bool SetOnline);
bool   CList_SetAllOffline(BYTE ChatsToo);
HANDLE CList_SetOffline(struct CONTACT * user);

bool   CList_AddEvent(struct CONTACT * user, HICON Icon, HANDLE event, const char * tooltip, int type ) ;
HANDLE CList_FindContact (struct CONTACT * user);
BOOL   CList_AddDCCChat(TString name, TString hostmask, unsigned long adr, int port) ;

//input.cpp
extern bool  bTempDisableCheck, bTempForceCheck, bEcho;

bool   PostIrcMessageWnd(TCHAR* pszWindow, HANDLE hContact,const TCHAR* szBuf);
bool   PostIrcMessage( const TCHAR* fmt, ...);

//output
BOOL   ShowMessage (const CIrcMessage* pmsg);

//windows.cpp
BOOL    CALLBACK MessageboxWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL    CALLBACK InfoWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL    CALLBACK NickWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL    CALLBACK JoinWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL    CALLBACK InitWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL    CALLBACK ListWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL    CALLBACK QuickWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL    CALLBACK UserDetailsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL    CALLBACK ManagerWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL    CALLBACK QuestionWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
int     CALLBACK ListViewSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
LRESULT CALLBACK MgrEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) ;

//commandmonitor.cpp
extern	bool     bPerformDone;
extern	HWND     join_hWnd, list_hWnd, manager_hWnd, nick_hWnd, whois_hWnd, quickconn_hWnd;
extern   HWND     IgnoreWndHwnd;
extern	int      NoOfChannels, ManualWhoisCount;
extern   UINT_PTR KeepAliveTimer;	
extern	String   sChannelModes, sUserModes;
extern	TString  sChannelPrefixes, sUserModePrefixes, WhoisAwayReply;

VOID    CALLBACK KeepAliveTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime); 
VOID    CALLBACK OnlineNotifTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);
VOID    CALLBACK OnlineNotifTimerProc3(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);

//scripting.cpp
int  Scripting_InsertRawIn(WPARAM wParam,LPARAM lParam);
int  Scripting_InsertRawOut(WPARAM wParam,LPARAM lParam);
int  Scripting_InsertGuiIn(WPARAM wParam,LPARAM lParam);
int  Scripting_InsertGuiOut(WPARAM wParam,LPARAM lParam);
int  Scripting_GetIrcData(WPARAM wparam, LPARAM lparam);
BOOL Scripting_TriggerMSPRawIn(char ** pszRaw);
BOOL Scripting_TriggerMSPRawOut(char ** pszRaw);
BOOL Scripting_TriggerMSPGuiIn(WPARAM * wparam, GCEVENT * gce);
BOOL Scripting_TriggerMSPGuiOut(GCHOOK * gch);

#ifndef NDEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)

#pragma comment(lib,"comctl32.lib")

#endif

