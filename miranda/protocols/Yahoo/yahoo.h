/*
 * $Id$
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */
#ifndef _YAHOO_YAHOO_H_
#define _YAHOO_YAHOO_H_

#define _USE_32BIT_TIME_T

#include <windows.h>

/* 
 * Yahoo Services
 */
#define USE_STRUCT_CALLBACKS
#include "libyahoo2/yahoo2.h"
#include "libyahoo2/yahoo2_callbacks.h"
#include "libyahoo2/yahoo_util.h"

#include <newpluginapi.h>
#include <m_system.h>
#include <m_database.h>
#include <m_protomod.h>
#include <m_netlib.h>
#include <m_clist.h>
#include <m_langpack.h>

//=======================================================
//	Definitions
//=======================================================
// Build is a cvs build
//
// If defined, the build will add cvs info to the plugin info
#define YAHOO_CVSBUILD

#define YAHOO_LOGINSERVER                 "LoginServer"
#define YAHOO_LOGINPORT                   "LoginPort"
#define YAHOO_LOGINID                     "yahoo_id"
#define YAHOO_PASSWORD                    "Password"
#define YAHOO_CHECKMAIL                   "CheckMail"
#define YAHOO_TNOTIF                      "TypeNotif"
#define YAHOO_CUSTSTATDB                  "CustomStat"
#define	YAHOO_ALLOW_MSGBOX		           1
#define	YAHOO_ALLOW_ENTER		           2
#define	YAHOO_MAIL_POPUP		           4
#define	YAHOO_NOTIFY_POPUP		           8
#define YAHOO_DEFAULT_PORT              5050
#define YAHOO_DEFAULT_LOGIN_SERVER      "scs.msg.yahoo.com"	
#define YAHOO_DEFAULT_JAPAN_LOGIN_SERVER      "cs.yahoo.co.jp"	
#define YAHOO_CUSTOM_STATUS                99

#define YAHOO_DEBUGLOG YAHOO_DebugLog

extern int do_yahoo_debug;

#define LOG(x) if(do_yahoo_debug) { YAHOO_DEBUGLOG("%s:%d: ", __FILE__, __LINE__); \
	YAHOO_DEBUGLOG x; \
	YAHOO_DEBUGLOG(" ");}

#define YAHOO_SET_CUST_STAT			"/SetCustomStatCommand" 
#define YAHOO_SHOW_PROFILE			"/YahooShowProfileCommand"
#define YAHOO_SHOW_MY_PROFILE		"/YahooShowMyProfileCommand"
#define YAHOO_YAHOO_MAIL			"/YahooGotoMailboxCommand"
#define YAHOO_REFRESH				"/YahooRefreshCommand"
#define YAHOO_AB					"/YahooAddressBook"
#define YAHOO_CALENDAR				"/YahooCalendar"
#define YAHOO_SEND_NUDGE			"/SendNudge"
#define YAHOO_GETUNREAD_EMAILCOUNT	"/GetUnreadEmailCount"

#define STYLE_DEFAULTBGCOLOUR     RGB(173,206,247)

#define MENU_ITEMS_COUNT 7
extern	HANDLE         YahooMenuItems[ MENU_ITEMS_COUNT ];

#define LocalEventUnhook(hook)	if(hook) UnhookEvent(hook)
#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)

struct _conn {
	int tag;
	int id;
	int fd;
	yahoo_input_condition cond;
	void *data;
	int remove;
};

//=======================================================
//	Defines
//=======================================================
extern HANDLE			hNetlibUser;
extern HINSTANCE		hinstance;
extern int				yahooStatus, mUnreadMessages;
extern char				yahooProtocolName[MAX_PATH];
extern BOOL             yahooLoggedIn;
extern HANDLE           YahooMenuItems[ MENU_ITEMS_COUNT ];


#ifdef HTTP_GATEWAY
extern int 				iHTTPGateway;
#endif

HANDLE __stdcall YAHOO_CreateProtoServiceFunction( 
	const char* szService,
	MIRANDASERVICE serviceProc );

int __stdcall YAHOO_CallService( const char* szSvcName, WPARAM wParam, LPARAM lParam );

#ifdef __GNUC__
int YAHOO_DebugLog( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
#else
int YAHOO_DebugLog( const char *fmt, ... );
#endif

DWORD __stdcall YAHOO_GetByte( const char* valueName, int parDefltValue );
DWORD __stdcall YAHOO_SetByte( const char* valueName, int parValue );

DWORD __stdcall YAHOO_GetDword( const char* valueName, DWORD parDefltValue );
DWORD __stdcall YAHOO_SetDword( const char* valueName, DWORD parValue );

WORD __stdcall YAHOO_GetWord( HANDLE hContact, const char* valueName, int parDefltValue );
DWORD __stdcall YAHOO_SetWord( HANDLE hContact, const char* valueName, int parValue );

int __stdcall YAHOO_SendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam );

DWORD __stdcall YAHOO_SetString( HANDLE hContact, const char* valueName, const char* parValue );
DWORD __stdcall YAHOO_SetStringUtf( HANDLE hContact, const char* valueName, const char* parValue );

int __stdcall	YAHOO_ShowPopup( const char* nickname, const char* msg, const char *szURL );

#define YAHOO_hasnotification() ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)

int YAHOO_shownotification(const char *title, const char *info, DWORD flags);
int YAHOO_util_dbsettingchanged(WPARAM wParam, LPARAM lParam);
void YAHOO_utils_logversion();
void YAHOO_ShowError(const char *title, const char *buff);

//Services.c
int GetCaps(WPARAM wParam,LPARAM lParam);
int GetName(WPARAM wParam,LPARAM lParam);
int SetStatus(WPARAM wParam,LPARAM lParam);
int GetStatus(WPARAM wParam,LPARAM lParam);
void yahoo_util_broadcaststatus(int s);
void __cdecl yahoo_server_main(void *empty);
const char *find_buddy( const char *yahoo_id);
HANDLE getbuddyH(const char *yahoo_id);

void yahoo_logoff_buddies();
void yahoo_set_status(int myyahooStatus, char *msg, int away);
int miranda_to_yahoo(int myyahooStatus);
void yahoo_stealth(const char *buddy, int add);

void register_callbacks();
char* YAHOO_GetContactName(HANDLE hContact);

void YAHOO_remove_buddy(const char *who);
void YAHOO_reject(const char *who, const char *msg);
void YAHOO_accept(const char *who);
void YAHOO_add_buddy(const char *who, const char *group, const char *msg);
HANDLE add_buddy( const char *yahoo_id, const char *yahoo_name, DWORD flags );
void YAHOO_sendtyping(const char *who, int stat);

typedef struct {
	char yahoo_id[255];
	char name[255];
	int status;
	int away;
	char *msg;
	char group[255];
} yahoo_account;

typedef struct {
	char yahoo_id[255];
	char password[255];
	int id;
	int fd;
	int status;
	char *msg;
	int  rpkts;
} yahoo_local_account;

void SetButtonCheck(HWND hwndDlg, int CtrlID, BOOL bCheck);
void YahooOpenURL(const char *url, int autoLogin);

char * yahoo_status_code(enum yahoo_status s);
void YAHOO_refresh();
int LoadYahooServices(void);
void yahoo_logout();
void yahoo_callback(struct _conn *c, yahoo_input_condition cond);
void ext_yahoo_login(int login_mode);
int YahooGotoMailboxCommand( WPARAM wParam, LPARAM lParam );

void YahooMenuInit( void );
void YahooIconsInit( void );
HICON LoadIconEx( const char* name );
int YahooIdleEvent(WPARAM wParam, LPARAM lParam);
#endif
