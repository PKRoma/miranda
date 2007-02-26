/*
IRC plugin for Miranda IM

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

#ifndef _IRCWIN_H_
#define _IRCWIN_H_

#define MIRANDA_VER 0x0600

// disable a lot of warnings. It should comppile on VS 6 also
#pragma warning( disable : 4076 ) 
#pragma warning( disable : 4786 ) 
#pragma warning( disable : 4996 ) 

#define WIN32_LEAN_AND_MEAN	
//#include "AggressiveOptimize.h"
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <shellapi.h>
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

#define STR_QUITMESSAGE  "\002Miranda IM!\002 Smaller, Faster, Easier. http://miranda-im.org"
#define STR_USERINFO     "I'm a happy Miranda IM user! Get it here: http://miranda-im.org"
#define STR_AWAYMESSAGE  "I'm away from the computer." // Default away
#define DCCSTRING        " (DCC)"

#define DCC_CHAT		1
#define DCC_SEND		2

#define FILERESUME_CANCEL	11

using namespace std;
using namespace irc;

// special service for tweaking performance, implemented in chat.dll
#define MS_GC_GETEVENTPTR  "GChat/GetNewEventPtr"
typedef int (*GETEVENTFUNC)(WPARAM wParam, LPARAM lParam);
typedef struct  {
	GETEVENTFUNC pfnAddEvent;
}GCPTRS;

#define IP_AUTO			1
#define IP_MANUAL		2

typedef struct IPRESOLVE_INFO_TYPE			// Contains info about the channels
{
	int iType;
	char * pszAdr;
} IPRESOLVE;

typedef struct WINDOW_INFO_TYPE			// Contains info about the channels
{
	char * pszTopic;
	char * pszMode;
	char * pszPassword;
	char * pszLimit;
} CHANNELINFO;

typedef struct SERVER_INFO_TYPE			// Contains info about different servers
{
	char * Group;
	char * Address;
	char * Name;
	char * PortStart;
	char * PortEnd;	
	int iSSL;
} SERVER_INFO;



typedef struct PERFORM_INFO_TYPE		// Contains 'Perform buffer' for different networks
{
	char * Perform;

} PERFORM_INFO;

typedef struct CONTACT_TYPE			// Contains info about users
{
	char * name;
	char * user;
	char * host;
	bool ExactOnly;
	bool ExactWCOnly;
	bool ExactNick;
} CONTACT;

typedef struct PREFERENCES_TYPE			// Preferences structure
{
	char ServerName[100];
	char Password [500];
	char IdentSystem[10];
	char Network[30];
	char PortStart[10];
	char PortEnd[10];
	int IdentTimer;
	int iSSL;
	char IdentPort[10];
	char RetryWait[10];
	char RetryCount[10];

	char Nick[30];
	char AlternativeNick[30];
	char Name[200];
	char UserID[200];
	char QuitMessage[400];
	char UserInfo[500];
	char MyHost[50];
	char MySpecifiedHost[500];
	char MySpecifiedHostIP[50];
	char MyLocalHost[50];
	char * Alias;
	int ServerComboSelection;
	int QuickComboSelection;
	int OnlineNotificationTime;
	int OnlineNotificationLimit;
	BYTE ScriptingEnabled;
	BYTE IPFromServer;
	BYTE ShowAddresses;
	BYTE DisconnectDCCChats;
	BYTE DisableErrorPopups;
	BYTE RejoinChannels;
	BYTE RejoinIfKicked;
	BYTE HideServerWindow;
	BYTE Ident;
	BYTE Retry;
	BYTE DisableDefaultServer;
	BYTE AutoOnlineNotification;
	BYTE SendKeepAlive;
	BYTE JoinOnInvite;
	BYTE Perform;
	BYTE ForceVisible;
	BYTE Ignore;
	BYTE IgnoreChannelDefault;
	BYTE UseServer;
	BYTE DCCFileEnabled;
	BYTE DCCChatEnabled;
	BYTE DCCChatAccept;
	BYTE DCCChatIgnore;
	BYTE DCCPassive;
	BYTE ManualHost;
	BYTE OldStyleModes;
	BYTE ChannelAwayNotification;
	BYTE SendNotice;
	POINT ListSize;
	COLORREF colors[16];
	HICON hIcon[13];

} PREFERENCES;

//main.cpp
void   UpgradeCheck(void);

//services.cpp
void   HookEvents(void);
void   UnhookEvents(void);
void   CreateServiceFunctions(void);
void   ConnectToServer(void);
void   DisconnectFromServer(void);
int    Service_GCEventHook(WPARAM wParam,LPARAM lParam);

//options.cpp
void   InitPrefs(void);
void   UnInitOptions(void);
int    InitOptionsPages(WPARAM wParam,LPARAM lParam);
void   AddIcons(void);
HICON  LoadIconEx(int iIndex);
HANDLE GetIconHandle(int iconId);

//tools.cpp
int    WCCmp(char* wild, char *string);
char*  IrcLoadFile(char * szPath);
void   AddToJTemp(String sCommand);
String GetWord(const char * text, int index);
String ReplaceString (String text, char * replaceme, char * newword);
bool   IsChannel(String sName); 
char*  GetWordAddress(const char * text, int index);
String RemoveLinebreaks(String Message);
char*  my_strstri(char *s1, char *s2) ;
char*  DoColorCodes (const char * text, bool bStrip, bool bReplacePercent);
int    DoEvent(int iEvent, const char* pszWindow, const char * pszNick, const char* pszText, const char* pszStatus, const char* pszUserInfo, DWORD dwItemData, bool bAddToLog, bool bIsMe,time_t timestamp = 1);
int    CallChatEvent(WPARAM wParam, LPARAM lParam);
String ModeToStatus(char sMode) ;
String PrefixToStatus(char cPrefix) ;
void   SetChatTimer(UINT_PTR &nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc);
void   KillChatTimer(UINT_PTR &nIDEvent);
int    SetChannelSBText(String sWindow, CHANNELINFO * wi);
String MakeWndID(String sWindow);
bool   FreeWindowItemData(String window, CHANNELINFO* wis);
bool   AddWindowItemData(String window, const char * pszLimit, const char * pszMode, const char * pszPassword, const char * pszTopic);
void   FindLocalIP(HANDLE con);
void   DoUserhostWithReason(int type, String reason, bool bSendCommand, String userhostparams, ...);
String GetNextUserhostReason(int type);
void   ClearUserhostReasons(int type);
String PeekAtReasons(int type);

//clist.cpp
HANDLE CList_AddContact(struct CONTACT_TYPE * user, bool InList, bool SetOnline);
bool   CList_SetAllOffline(BYTE ChatsToo);
HANDLE CList_SetOffline(struct CONTACT_TYPE * user);

bool   CList_AddEvent(struct CONTACT_TYPE * user, HICON Icon, HANDLE event, const char * tooltip, int type ) ;
HANDLE CList_FindContact (struct CONTACT_TYPE * user);
int    WCCmp(char* wild, char *string);
BOOL   CList_AddDCCChat(String name, String hostmask, unsigned long adr, int port) ;

//input.cpp
bool   PostIrcMessageWnd(char * pszWindow, HANDLE hContact,const char * szBuf);
bool   PostIrcMessage( const char * fmt, ...);

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

#pragma comment(lib,"comctl32.lib")

#endif

