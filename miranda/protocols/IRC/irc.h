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

#pragma warning( disable : 4786 ) // limitation in MSVC's debugger.

#define WIN32_LEAN_AND_MEAN	
#include "AggressiveOptimize.h"
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
#include "../../include/newpluginapi.h"
#include "../../include/m_system.h"
#include "../../include/m_protocols.h"
#include "../../include/m_protomod.h"
#include "../../include/m_protosvc.h"
#include "../../include/m_clist.h"
#include "../../include/m_options.h"
#include "../../include/m_database.h"
#include "../../include/m_utils.h"
#include "../../include/m_skin.h"
#include "../../include/m_netlib.h"
#include "../../include/m_clui.h"
#include "../../include/m_langpack.h"
#include "../../include/m_message.h"
#include "../../include/m_userinfo.h"
#include "../../include/m_addcontact.h"
#include "../../include/m_button.h"
#include "../../include/m_file.h"
#include "../../plugins/chat/m_chat.h"
#include "resource.h"
#include "irclib.h"
#include "commandmonitor.h"
#include "m_uninstaller.h"

#ifndef NDEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define	IRC_UPDATELIST				(WM_USER+1)
#define	IRC_QUESTIONAPPLY			(WM_USER+2)
#define	IRC_QUESTIONCLOSE			(WM_USER+3)
#define	IRC_ACTIVATE				(WM_USER+4)
#define	IRC_INITMANAGER				(WM_USER+5)

#define STR_QUITMESSAGE				"\002Miranda IM!\002 Smaller, Faster, Easier. http://miranda-im.org"
#define STR_USERINFO				"I'm a happy Miranda IM user! Get it here: http://miranda-im.org"
#define STR_AWAYMESSAGE				"I'm away from the computer." // Default away
#define DCCSTRING					" (DCC)"

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
	char MySpecifiedHost[50];
	char MyLocalHost[50];
	char * Alias;
	int ServerComboSelection;
	int QuickComboSelection;
	char OnlineNotificationTime[10];
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
	BYTE AutoOnlineNotifTempAlso;
	BYTE SendKeepAlive;
	BYTE JoinOnInvite;
	BYTE Perform;
	BYTE ForceVisible;
	BYTE Ignore;
	BYTE UseServer;
	BYTE DCCFileEnabled;
	BYTE DCCChatEnabled;
	BYTE DCCChatAccept;
	BYTE DCCChatIgnore;
	BYTE DCCPassive;
	BYTE ManualHost;
	POINT ListSize;
	COLORREF colors[16];

} PREFERENCES;

//main.cpp

//services.cpp
void						HookEvents(void);
void						UnhookEvents(void);
void						CreateServiceFunctions(void);
void						ConnectToServer(void);
void						DisconnectFromServer(void);

//options.cpp
void						InitPrefs(void);
void						UnInitOptions(void);
int							InitOptionsPages(WPARAM wParam,LPARAM lParam);

//tools.cpp
int							WCCmp(char* wild, char *string);
char *						IrcLoadFile(char * szPath);
void						AddToUHTemp(String sCommand);
void						AddToJTemp(String sCommand);
void __cdecl				forkthread_r(void *param);
unsigned long				forkthread (	void (__cdecl *threadcode)(void*),unsigned long stacksize,void *arg);
String						GetWord(const char * text, int index);
String						ReplaceString (String text, char * replaceme, char * newword);
bool						IsChannel(String sName); 
char *						GetWordAddress(const char * text, int index);
String						RemoveLinebreaks(String Message);
char*						my_strstri(char *s1, char *s2) ;
char *						DoColorCodes (const char * text, bool bStrip, bool bReplacePercent);
int							DoEvent(int iEvent, const char* pszWindow, const char * pszNick, const char* pszText, const char* pszStatus, const char* pszUserInfo, DWORD dwItemData, bool bAddToLog, bool bIsMe);
String						ModeToStatus(char sMode) ;
String						PrefixToStatus(char cPrefix) ;
void						SetChatTimer(UINT_PTR &nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc);
void						KillChatTimer(UINT_PTR &nIDEvent);
int							SetChannelSBText(String sWindow, CHANNELINFO * wi);
String						MakeWndID(String sWindow);
bool						FreeWindowItemData(String window, CHANNELINFO* wis);
bool						AddWindowItemData(String window, const char * pszLimit, const char * pszMode, const char * pszPassword, const char * pszTopic);
void						FindLocalIP(HANDLE con);
//clist.cpp
HANDLE						CList_AddContact(struct CONTACT_TYPE * user, bool InList, bool SetOnline);
bool						CList_SetAllOffline(BYTE ChatsToo);
HANDLE						CList_SetOffline(struct CONTACT_TYPE * user);

bool						CList_AddEvent(struct CONTACT_TYPE * user, HICON Icon, HANDLE event, const char * tooltip, int type ) ;
HANDLE						CList_FindContact (struct CONTACT_TYPE * user);
int							WCCmp(char* wild, char *string);
BOOL						CList_AddDCCChat(String name, String hostmask, unsigned long adr, int port) ;

//input.cpp
bool						PostIrcMessageWnd(char * pszWindow, HANDLE hContact,const char * szBuf);
bool						PostIrcMessage( const char * fmt, ...);

//output
BOOL						ShowMessage (const CIrcMessage* pmsg);

//windows.cpp
BOOL CALLBACK				MessageboxWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK				InfoWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK				NickWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK				JoinWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK				InitWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK				ListWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK				QuickWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK				UserDetailsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK				ManagerWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK				QuestionWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
int CALLBACK				ListViewSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
LRESULT CALLBACK			MgrEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) ;

//commandmonitor.cpp
VOID CALLBACK				KeepAliveTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);
VOID CALLBACK				OnlineNotifTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);

#ifndef NDEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma comment(lib,"comctl32.lib")

#endif