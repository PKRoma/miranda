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
#include <windows.h>
#include <stdio.h>

/* 
 * Yahoo Services
 */
#include "pthread.h"

#include "libyahoo2/config.h"
#include "libyahoo2/yahoo2.h"
#include "libyahoo2/yahoo2_callbacks.h"
#include "libyahoo2/yahoo_util.h"

#include <newpluginapi.h>
#include <m_database.h>
#include <m_protomod.h>
#include <m_netlib.h>
#include <m_clist.h>

//=======================================================
//	Definitions
//=======================================================
// Build is a cvs build
//
// If defined, the build will add cvs info to the plugin info
#define YAHOO_CVSBUILD

//#define modname			"myYahoo"
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
#define YAHOO_CUSTOM_STATUS                99

#define YAHOO_DEBUGLOG ext_yahoo_log

#define LOG(x) if(do_yahoo_debug) { YAHOO_DEBUGLOG("%s:%d: ", __FILE__, __LINE__); \
	YAHOO_DEBUGLOG x; \
	YAHOO_DEBUGLOG(" ");}

#define YAHOO_SET_CUST_STAT  "/SetCustomStatCommand" 
#define YAHOO_SHOW_PROFILE   "/YahooShowProfileCommand"
#define YAHOO_SHOW_MY_PROFILE "/YahooShowMyProfileCommand"
#define YAHOO_YAHOO_MAIL     "/YahooGotoMailboxCommand"
#define YAHOO_REFRESH     "/YahooRefreshCommand"

#define STYLE_DEFAULTBGCOLOUR     RGB(173,206,247)

#define MENU_ITEMS_COUNT 5
extern	HANDLE         YahooMenuItems[ MENU_ITEMS_COUNT ];

#define LocalEventUnhook(hook)	if(hook) UnhookEvent(hook)

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
//General
extern HANDLE			hNetlibUser;
extern HINSTANCE		hinstance;
extern int				yahooStatus;
extern char				yahooProtocolName[MAX_PATH];
extern BOOL             yahooLoggedIn;

extern HANDLE           YahooMenuItems[ MENU_ITEMS_COUNT ];
extern pthread_mutex_t connectionHandleMutex;

int ext_yahoo_log(char *fmt,...);

HANDLE __stdcall YAHOO_CreateProtoServiceFunction( 
	const char* szService,
	MIRANDASERVICE serviceProc );

int __stdcall YAHOO_CallService( const char* szSvcName, WPARAM wParam, LPARAM lParam );

#ifdef __MINGW32__
void __stdcall	YAHOO_DebugLog( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
#else
void __stdcall	YAHOO_DebugLog( const char *fmt, ... );
#endif

DWORD __stdcall YAHOO_GetByte( const char* valueName, int parDefltValue );
DWORD __stdcall YAHOO_SetByte( const char* valueName, int parValue );

DWORD __stdcall YAHOO_GetDword( const char* valueName, DWORD parDefltValue );
DWORD __stdcall YAHOO_SetDword( const char* valueName, DWORD parValue );

WORD __stdcall YAHOO_GetWord( HANDLE hContact, const char* valueName, int parDefltValue );
DWORD __stdcall YAHOO_SetWord( HANDLE hContact, const char* valueName, int parValue );

int __stdcall YAHOO_SendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam );

DWORD __stdcall YAHOO_SetString( HANDLE hContact, const char* valueName, const char* parValue );

int __stdcall	YAHOO_ShowPopup( const char* nickname, const char* msg, int flags );

#define YAHOO_hasnotification() ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)

int YAHOO_shownotification(const char *title, const char *info, DWORD flags);
int YAHOO_util_dbsettingchanged(WPARAM wParam, LPARAM lParam);
void YAHOO_utils_logversion();
void YAHOO_ShowError(const char *buff);

//Services.c
int GetCaps(WPARAM wParam,LPARAM lParam);
int GetName(WPARAM wParam,LPARAM lParam);
int BPLoadIcon(WPARAM wParam,LPARAM lParam); //BPLoadIcon because LoadIcon wont work..
int SetStatus(WPARAM wParam,LPARAM lParam);
int GetStatus(WPARAM wParam,LPARAM lParam);
void yahoo_util_broadcaststatus(int s);
void __cdecl yahoo_server_main(void *empty);
const char *find_buddy( const char *yahoo_id);
HANDLE getbuddyH(const char *yahoo_id);
void yahoo_send_msg(const char *id, const char *msg, int utf8);
void yahoo_logoff_buddies();
void yahoo_set_status(int myyahooStatus, char *msg, int away);
int miranda_to_yahoo(int myyahooStatus);
void yahoo_stealth(const char *buddy, int add);
YList * YAHOO_GetIgnoreList(void);
void YAHOO_IgnoreBuddy(const char *buddy, int ignore);

BOOL CALLBACK DlgProcYahooOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcYahooPopUpOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int YahooOptInit(WPARAM wParam,LPARAM lParam);
void register_callbacks();
char* YAHOO_GetContactName(HANDLE hContact);
void YAHOO_basicsearch(const char *nick);
void YAHOO_remove_buddy(const char *who);
void YAHOO_reject(const char *who, const char *msg);
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
} yahoo_local_account;


typedef struct {
	char *who;
	char *msg;
	char *filename;
	HANDLE hContact;
	int  cancel;
	char *url;
	char *savepath;
	unsigned long fsize;
	HANDLE hWaitEvent;
	DWORD action;
} y_filetransfer;

void YAHOO_SendFile(y_filetransfer *ft);
void YAHOO_RecvFile(y_filetransfer *ft);
void YAHOO_request_avatar(const char* who);
void GetAvatarFileName(HANDLE hContact, char* pszDest, int cbLen);
#define FILERESUME_CANCEL	11

char * yahoo_status_code(enum yahoo_status s);
void YAHOO_refresh();
int LoadYahooServices(void);
void yahoo_logout();
void yahoo_callback(struct _conn *c, yahoo_input_condition cond);
void ext_yahoo_login(int login_mode);
void __stdcall Utf8Decode( char* str, int maxSize, wchar_t** ucs2 );
char* __stdcall Utf8EncodeUcs2( const wchar_t* src );
void YAHOO_ping(void);
