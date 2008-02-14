/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-08 George Hazan

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

#define MIRANDA_VER 0x0800

#include "m_stdhdr.h"

#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <math.h>
#include <winsock.h>
#include <time.h>

#include "newpluginapi.h"
#include "m_system.h"
#include "m_system_cpp.h"
#include "m_protocols.h"
#include "m_protomod.h"
#include "m_protosvc.h"
#include "m_protoint.h"
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
#include "ui_utils.h"

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
#define IRC_UM_SHOWCHANNEL    "/UMenuShowChannel"
#define IRC_UM_JOINLEAVE      "/UMenuJoinLeave"
#define IRC_UM_CHANSETTINGS	  "/UMenuChanSettings"
#define IRC_UM_WHOIS	      "/UMenuWhois"
#define IRC_UM_DISCONNECT     "/UMenuDisconnect"
#define IRC_UM_IGNORE		  "/UMenuIgnore"

#define STR_QUITMESSAGE  "\002Miranda IM!\002 Smaller, Faster, Easier. http://miranda-im.org"
#define STR_USERINFO     "I'm a happy Miranda IM user! Get it here: http://miranda-im.org"
#define STR_AWAYMESSAGE  "I'm away from the computer." // Default away
#define DCCSTRING        " (DCC)"
#define SERVERWINDOW	 _T("Network log")

#define WNDCLASS_IRCWINDOW "MirandaIrcVoidWindow"

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
	IPRESOLVE( CIrcProto* _pro, const char* _addr, int _type ) :
		sAddr( _addr ),
		iType( _type ),
		ppro( _pro )
	{}

	~IPRESOLVE()
	{}

	String     sAddr;
	int        iType;
	CIrcProto* ppro;
};

struct CHANNELINFO   // Contains info about the channels
{
	TCHAR* pszTopic;
	TCHAR* pszMode;
	TCHAR* pszPassword;
	TCHAR* pszLimit;
	BYTE   OwnMode;	/* own mode on the channel. Bitmask:
												0: voice
												1: halfop
												2: op
												3: admin
												4: owner		*/
	int    codepage;
};

struct SERVER_INFO  // Contains info about different servers
{
	~SERVER_INFO();

	char* Group;
	char* Address;
	char* Name;
	char* PortStart;
	char* PortEnd;
	int   iSSL;
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

/////////////////////////////////////////////////////////////////////////////////////////

struct CIrcProto;
typedef int  ( __cdecl CIrcProto::*IrcEventFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CIrcProto::*IrcServiceFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CIrcProto::*IrcServiceFuncParam )( WPARAM, LPARAM, LPARAM );
typedef void ( __cdecl CIrcProto::*IrcTimerFunc )( int eventId );

typedef std::map<HANDLE, CDccSession*> DccSessionMap;
typedef std::pair<HANDLE, CDccSession*> DccSessionPair;

struct CIrcProto : public PROTO_INTERFACE
{
				CIrcProto( const char*, const TCHAR* );
			   ~CIrcProto();

				__inline void* operator new( size_t size )
				{	return ::calloc( 1, size );
				}
				__inline void operator delete( void* p )
				{	::free( p );
				}

				// Protocol interface

	virtual	HANDLE __cdecl AddToList( int flags, PROTOSEARCHRESULT* psr );
	virtual	HANDLE __cdecl AddToListByEvent( int flags, int iContact, HANDLE hDbEvent );

	virtual	int    __cdecl Authorize( HANDLE hContact );
	virtual	int    __cdecl AuthDeny( HANDLE hContact, const char* szReason );
	virtual	int    __cdecl AuthRecv( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl AuthRequest( HANDLE hContact, const char* szMessage );

	virtual	HANDLE __cdecl ChangeInfo( int iInfoType, void* pInfoData );

	virtual	int    __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath );
	virtual	int    __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer );
	virtual	int    __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason );
	virtual	int    __cdecl FileResume( HANDLE hTransfer, int* action, const char** szFilename );

	virtual	DWORD  __cdecl GetCaps( int type );
	virtual	HICON  __cdecl GetIcon( int iconIndex );
	virtual	int    __cdecl GetInfo( HANDLE hContact, int infoType );

	virtual	HANDLE __cdecl SearchBasic( const char* id );
	virtual	HANDLE __cdecl SearchByEmail( const char* email );
	virtual	HANDLE __cdecl SearchByName( const char* nick, const char* firstName, const char* lastName );
	virtual	HWND   __cdecl SearchAdvanced( HWND owner );
	virtual	HWND   __cdecl CreateExtendedSearchUI( HWND owner );

	virtual	int    __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvFile( HANDLE hContact, PROTORECVFILE* );
	virtual	int    __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* );

	virtual	int    __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList );
	virtual	int    __cdecl SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles );
	virtual	int    __cdecl SendMsg( HANDLE hContact, int flags, const char* msg );
	virtual	int    __cdecl SendUrl( HANDLE hContact, int flags, const char* url );

	virtual	int    __cdecl SetApparentMode( HANDLE hContact, int mode );
	virtual	int    __cdecl SetStatus( int iNewStatus );

	virtual	int    __cdecl GetAwayMsg( HANDLE hContact );
	virtual	int    __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt );
	virtual	int    __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg );
	virtual	int    __cdecl SetAwayMsg( int m_iStatus, const char* msg );

	virtual	int    __cdecl UserIsTyping( HANDLE hContact, int type );

	virtual	int    __cdecl OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam );

	// Events
	int __cdecl OnChangeNickMenuCommand( WPARAM, LPARAM );
	int __cdecl OnDeletedContact( WPARAM, LPARAM );
	int __cdecl OnDoubleclicked( WPARAM, LPARAM );
	int __cdecl OnInitOptionsPages( WPARAM, LPARAM );
	int __cdecl OnInitUserInfo( WPARAM, LPARAM );
	int __cdecl OnJoinMenuCommand( WPARAM, LPARAM );
	int __cdecl OnMenuChanSettings( WPARAM, LPARAM );
	int __cdecl OnMenuDisconnect( WPARAM , LPARAM );
	int __cdecl OnMenuIgnore( WPARAM, LPARAM );
	int __cdecl OnMenuJoinLeave( WPARAM, LPARAM );
	int __cdecl OnMenuShowChannel( WPARAM, LPARAM );
	int __cdecl OnMenuWhois( WPARAM, LPARAM );
	int __cdecl OnModulesLoaded( WPARAM, LPARAM );
	int __cdecl OnMenuPreBuild( WPARAM, LPARAM );
	int __cdecl OnPreShutdown( WPARAM, LPARAM );
	int __cdecl OnQuickConnectMenuCommand(WPARAM, LPARAM );
	int __cdecl OnShowListMenuCommand( WPARAM, LPARAM );
	int __cdecl OnShowServerMenuCommand( WPARAM, LPARAM );

	int __cdecl GCEventHook( WPARAM, LPARAM );
	int __cdecl GCMenuHook( WPARAM, LPARAM );

	// Data

	char     ServerName[100];
	char     Password [500];
	TCHAR    IdentSystem[10];
	char     Network[30];
	char     PortStart[10];
	char     PortEnd[10];
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

	char* pszServerFile;

	std::vector<TString> vUserhostReasons;
	std::vector<TString> vWhoInProgress;

	CRITICAL_SECTION cs;
	CRITICAL_SECTION m_gchook;
	CRITICAL_SECTION m_resolve;

	TString StatusMessage;
	bool    bMbotInstalled;
	int     iTempCheckTime;

	CIrcSessionInfo si;

	int       iRetryCount;
	int       PortCount;
	DWORD     bConnectRequested;
	DWORD     bConnectThreadRunning;

	HANDLE  hMenuRoot, hMenuQuick, hMenuServer, hMenuJoin, hMenuNick, hMenuList;
	HANDLE  hNetlib, hNetlibDCC, hUMenuShowChannel, hUMenuJoinLeave, hUMenuChanSettings, hUMenuWhois;
	HANDLE  hUMenuDisconnect, hUMenuIgnore;

	bool  bTempDisableCheck, bTempForceCheck, bEcho;
	bool	nickflag;

	bool     bPerformDone;
	HWND     join_hWnd, list_hWnd, manager_hWnd, nick_hWnd, whois_hWnd, quickconn_hWnd;
	HWND     IgnoreWndHwnd;
	int      NoOfChannels, ManualWhoisCount;
	String   sChannelModes, sUserModes;
	TString  sChannelPrefixes, sUserModePrefixes, WhoisAwayReply;

	HWND    m_hwndTimer;

	CDlgBase::CreateParam OptCreateAccount, OptCreateConn, OptCreateIgnore, OptCreateOther;

	//clist.cpp
	HANDLE CList_AddContact(struct CONTACT * user, bool InList, bool SetOnline);
	bool   CList_SetAllOffline(BYTE ChatsToo);
	HANDLE CList_SetOffline(struct CONTACT * user);

	bool   CList_AddEvent(struct CONTACT * user, HICON Icon, HANDLE event, const char * tooltip, int type ) ;
	HANDLE CList_FindContact (struct CONTACT * user);
	BOOL   CList_AddDCCChat(TString name, TString hostmask, unsigned long adr, int port) ;

	//commandmonitor.cpp
	UINT_PTR IdentTimer;
	void __cdecl IdentTimerProc( int idEvent );

	UINT_PTR InitTimer;
	void __cdecl TimerProc( int idEvent );

	UINT_PTR KeepAliveTimer;
	void __cdecl KeepAliveTimerProc( int idEvent );

	UINT_PTR OnlineNotifTimer;	
	void __cdecl OnlineNotifTimerProc( int idEvent );

	UINT_PTR OnlineNotifTimer3;	
	void __cdecl OnlineNotifTimerProc3( int idEvent );

	int  AddOutgoingMessageToDB(HANDLE hContact, TCHAR* msg);
	bool DoOnConnect(const CIrcMessage *pmsg);
	int  DoPerform(const char* event);

	bool AddIgnore(const TCHAR* mask, const TCHAR* mode, const TCHAR* network) ;
	int  IsIgnored(TString nick, TString address, TString host, char type) ;
	int  IsIgnored(TString user, char type);
	bool RemoveIgnore(const TCHAR* mask) ;

	//input.cpp
	TString DoAlias( const TCHAR *text, TCHAR *window);
	BOOL    DoHardcodedCommand( TString text, TCHAR* window, HANDLE hContact );
	TString DoIdentifiers( TString text, const TCHAR* window );
	TString FormatMsg(TString text);
	bool    PostIrcMessageWnd(TCHAR* pszWindow, HANDLE hContact,const TCHAR* szBuf);
	bool    PostIrcMessage( const TCHAR* fmt, ...);

	// irclib.cpp
	UINT_PTR	DCCTimer;	
	void __cdecl DCCTimerProc( int idEvent );

	//options.cpp
	HWND connect_hWnd;
	HWND addserver_hWnd;

	std::vector<CIrcIgnoreItem> g_ignoreItems;

	int            ChannelNumber;
	TString        WhoReply;
	TString        sNamesList;
	TString        sTopic;
	TString        sTopicName;
	TString		   sTopicTime;
	TString        NamesToWho;
	TString        ChannelsToWho;
	TString        NamesToUserhost;

	bool     ServerlistModified, PerformlistModified;

	void    InitPrefs(void);
	int     GetPrefsString(const char *szSetting, char * prefstoset, int n, char * defaulttext);
	#if defined( _UNICODE )
		int CIrcProto::GetPrefsString(const char *szSetting, TCHAR* prefstoset, int n, TCHAR* defaulttext);
	#endif

	HANDLE* hIconLibItems;
	void    AddIcons(void);
	void    InitIgnore(void);
	HICON   LoadIconEx(int iIndex);
	HANDLE  GetIconHandle(int iconId);
	void    RewriteIgnoreSettings( void );

	//output
	BOOL   ShowMessage (const CIrcMessage* pmsg);

	//scripting.cpp
	int  __cdecl Scripting_InsertRawIn(WPARAM wParam,LPARAM lParam);
	int  __cdecl Scripting_InsertRawOut(WPARAM wParam,LPARAM lParam);
	int  __cdecl Scripting_InsertGuiIn(WPARAM wParam,LPARAM lParam);
	int  __cdecl Scripting_InsertGuiOut(WPARAM wParam,LPARAM lParam);
	int  __cdecl Scripting_GetIrcData(WPARAM wparam, LPARAM lparam);
	BOOL Scripting_TriggerMSPRawIn(char ** pszRaw);
	BOOL Scripting_TriggerMSPRawOut(char ** pszRaw);
	BOOL Scripting_TriggerMSPGuiIn(WPARAM * wparam, GCEVENT * gce);
	BOOL Scripting_TriggerMSPGuiOut(GCHOOK * gch);

	// services.cpp
	void   ConnectToServer(void);
	void   DisconnectFromServer(void);
	void   DoNetlibLog( const char* fmt, ... );
	void   IrcHookEvent( const char*, IrcEventFunc );
	void   InitMenus( void );

	UINT_PTR  RetryTimer;
	void __cdecl RetryTimerProc( int idEvent );

	int __cdecl GetName( WPARAM, LPARAM );
	int __cdecl GetStatus( WPARAM, LPARAM );

	//tools.cpp
	void     AddToJTemp(TString sCommand);
	bool     AddWindowItemData(TString window, const TCHAR* pszLimit, const TCHAR* pszMode, const TCHAR* pszPassword, const TCHAR* pszTopic);
	int      CallChatEvent(WPARAM wParam, LPARAM lParam);
	void     CreateProtoService( const char* serviceName, IrcServiceFunc pFunc );
	int      DoEvent(int iEvent, const TCHAR* pszWindow, const TCHAR* pszNick, const TCHAR* pszText, const TCHAR* pszStatus, const TCHAR* pszUserInfo, DWORD dwItemData, bool bAddToLog, bool bIsMe,time_t timestamp = 1);
	void     FindLocalIP(HANDLE con);
	bool     FreeWindowItemData(TString window, CHANNELINFO* wis);
	#if defined( _UNICODE )
		bool    IsChannel(String sName);
	#endif
	bool     IsChannel(TString sName);
	void     KillChatTimer(UINT_PTR &nIDEvent);
	TString  MakeWndID(const TCHAR* sWindow);
	TString  ModeToStatus(int sMode);
	TString  PrefixToStatus(int cPrefix);
	int      SetChannelSBText(TString sWindow, CHANNELINFO * wi);
	void     SetChatTimer(UINT_PTR &nIDEvent,UINT uElapse, IrcTimerFunc lpTimerFunc);

	void     ClearUserhostReasons(int type);
	void     DoUserhostWithReason(int type, TString reason, bool bSendCommand, TString userhostparams, ...);
	TString  GetNextUserhostReason(int type);
	TString  PeekAtReasons(int type);

	void     setByte( const char* name, BYTE value );
	void     setByte( HANDLE hContact, const char* name, BYTE value );
	void     setDword( const char* name, DWORD value );
	void     setDword( HANDLE hContact, const char* name, DWORD value );
	void     setString( const char* name, const char* value );
	void     setString( HANDLE hContact, const char* name, const char* value );
	void     setTString( const char* name, const TCHAR* value );
	void     setTString( HANDLE hContact, const char* name, const TCHAR* value );
	void     setWord( const char* name, int value );
	void     setWord( HANDLE hContact, const char* name, int value );

	////////////////////////////////////////////////////////////////////////////////////////
	// former CIrcProto class

	friend class CIrcDefaultMonitor;

	void AddDCCSession(HANDLE hContact, CDccSession* dcc);
	void AddDCCSession(DCCINFO*  pdci, CDccSession* dcc);
	void RemoveDCCSession(HANDLE hContact);
	void RemoveDCCSession(DCCINFO*  pdci);
	
	CDccSession* FindDCCSession(HANDLE hContact);
	CDccSession* FindDCCSession(DCCINFO* pdci);
	CDccSession* FindDCCSendByPort(int iPort);
	CDccSession* FindDCCRecvByPortAndName(int iPort, const TCHAR* szName);
	CDccSession* FindPassiveDCCSend(int iToken);
	CDccSession* FindPassiveDCCRecv(TString sName, TString sToken);
	
	void DisconnectAllDCCSessions(bool Shutdown);
	void CheckDCCTimeout(void);

	bool Connect(const CIrcSessionInfo& info);
	void Disconnect(void);
	void KillIdent(void);

	CIrcSessionInfo& GetInfo() const { return (CIrcSessionInfo&)m_info; }

	#if defined( _UNICODE )
		int NLSend(const TCHAR* fmt, ...);
	#endif
	int NLSend(const char* fmt, ...);
	int NLSend(const unsigned char* buf, int cbBuf);
	int NLSendNoScript( const unsigned char* buf, int cbBuf);
	int NLReceive(unsigned char* buf, int cbBuf);
	void InsertIncomingEvent(TCHAR* pszRaw);

	__inline bool IsConnected() const { return con != NULL; }

	// send-to-stream operators
	CIrcProto& operator << (const CIrcMessage& m);

	int getCodepage() const;
	__inline void setCodepage( int aPage ) { codepage = aPage; }

protected :
	int codepage;
	CIrcSessionInfo m_info;
	CSSLSession sslSession;
	HANDLE con;
	HANDLE hBindPort;
	void DoReceive();
	DccSessionMap m_dcc_chats;
	DccSessionMap m_dcc_xfers;

private :
	CRITICAL_SECTION    m_dcc;      // protect the dcc objects
	CIrcDefaultMonitor *m_monitor;  // Object that processes data from the IRC server

	void createMessageFromPchar( const char* p );
	void Notify(const CIrcMessage* pmsg);
	static void __cdecl ThreadProc(void *pparam);
};

/////////////////////////////////////////////////////////////////////////////////////////
// Functions

//main.cpp
extern HINSTANCE hInst;
extern HMODULE m_ssleay32;

extern char mirandapath[MAX_PATH];
extern int mirVersion;

void   UpgradeCheck(void);

// services.cpp

extern BOOL bChatInstalled, bMbotInstalled;

//tools.cpp
int      WCCmp(const TCHAR* wild, const TCHAR* string);
char*    IrcLoadFile(char * szPath);
TString  GetWord(const TCHAR* text, int index);
TString& ReplaceString (TString& text, const TCHAR* replaceme, const TCHAR* newword);
TCHAR*   GetWordAddress(const TCHAR* text, int index);
void     RemoveLinebreaks( TString& Message );
TCHAR*   my_strstri(const TCHAR *s1, const TCHAR *s2) ;
TCHAR*   DoColorCodes (const TCHAR* text, bool bStrip, bool bReplacePercent);
char*    rtrim( char *string );

#if defined( _UNICODE )
String& ReplaceString (String& text, const char* replaceme, const char* newword);
String  GetWord(const char* text, int index);
char*   GetWordAddress(const char* text, int index);
#endif

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)
#define NEWTSTR_ALLOCA(A) (A==NULL)?NULL:_tcscpy((TCHAR*)alloca(sizeof(TCHAR)*(_tcslen(A)+1)),A)

#pragma comment(lib,"comctl32.lib")

#endif
