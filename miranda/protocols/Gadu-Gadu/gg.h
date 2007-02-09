////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#ifndef GG_H
#define GG_H

#if defined(__DEBUG__) || defined(_DEBUG)
#define DEBUGMODE				// Debug Mode
#endif

#if _WIN32_WINNT < 0x0500
#define _WIN32_WINNT 0x0500
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Windows headers
// Visual C++ .NET tries to include winsock.h
// which is very ver bad
#if (_MSC_VER >= 1300)
#include <winsock2.h>
#else
#include <windows.h>
#endif
#include <commctrl.h>
#include <commdlg.h>
#include <process.h>
#include <stdio.h>
#include <time.h>

// Miranda IM headers
#include <newpluginapi.h>
#include <m_system.h>
#include <m_database.h>
#include <m_netlib.h>
#include <m_protocols.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_langpack.h>
#include <m_plugins.h>
#include <m_skin.h>
#include <m_utils.h>
#include <m_ignore.h>
#include <m_clist.h>
#include <m_options.h>
#include <m_userinfo.h>
#include <m_clui.h>
#include <m_button.h>
#include <m_clc.h>
#include <m_message.h>

// Groupchat is now in miranda headers
#include <m_chat.h>

// Visual C++ extras
#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#define GGINLINE
#else
#define GGINLINE inline
#endif

// Plugin headers
#include "pthread.h"
#include "resource.h"

// libgadu headers
#include "libgadu/libgadu.h"
#include "dynstuff.h"

// Search
// Extended search result structure, used for all searches
typedef struct
{
	PROTOSEARCHRESULT hdr;
	uin_t uin;
} GGSEARCHRESULT;

typedef struct
{
	int mode;
	uin_t uin;
	char *pass;
	char *email;
	HFONT hBoldFont;
} GGUSERUTILDLGDATA;

typedef struct
{
	list_t watches, transfers, requests;
	pthread_t dccId;
	pthread_t id;
	struct gg_session *sess;
	struct gg_dcc *dcc;
	HANDLE event;
} GGTHREAD;

typedef struct
{
	uin_t *recipients;
	int recipients_count;
	char id[32];
	BOOL ignore;
} GGGC;

// Main strings
extern char ggProto[];
extern char *ggProtoName;
extern char *ggProtoError;

#define GG_PROTO		 ggProto		// Protocol ID
#define GG_PROTONAME	 ggProtoName	// Protocol Name
#define GG_PROTOERROR	 ggProtoError	// Protocol Error
#define GGDEF_PROTO 	 "GG"			// Default Proto
#define GGDEF_PROTONAME  "Gadu-Gadu"	// Default ProtoName

// Process handles / seqs
#define GG_SEQ_INFO				100
#define GG_SEQ_SEARCH			200
#define GG_SEQ_AWAYMSG			300
#define GG_SEQ_GETNICK			400
#define GG_SEQ_CHINFO			500

// Services
#define GGS_IMPORT_SERVER		"%s/ImportFromServer"
#define GGS_REMOVE_SERVER		"%s/RemoveFromServer"
#define GGS_IMPORT_TEXT 		"%s/ImportFromText"
#define GGS_EXPORT_SERVER		"%s/ExportFromServer"
#define GGS_EXPORT_TEXT 		"%s/ExportFromText"
#define GGS_CHPASS				"%s/ChangePassword"

#define GGS_SENDIMAGE			"%s/SendImage"
#define GGS_RECVIMAGE			"%s/RecvImage"

// Keys
#define GG_PLUGINVERSION		"Version"		// Plugin version.. user for cleanup from previous versions

#define GG_KEY_UIN				"UIN"			// Uin - unique number
#define GG_KEY_PASSWORD			"Password"		// Password
#define GG_KEY_EMAIL			"e-mail"		// E-mail
#define GG_KEY_STATUS			"Status"		// Status
#define GG_KEY_STARTUPSTATUS	"StartupStatus" // Status used when starting up
#define GG_KEY_NICK				"Nick"			// Nick
#define GG_KEY_STATUSDESCR		"StatusMsg" 	// Users status description, to be compatible with MWClist
												// should be stored in "CList" group
#define GG_KEY_KEEPALIVE		"KeepAlive" 	// Keep-alive support
#define GG_KEYDEF_KEEPALIVE		1

#define GG_KEY_SHOWCERRORS		"ShowCErrors"	// Show connection errors
#define GG_KEYDEF_SHOWCERRORS	1

#define GG_KEY_ARECONNECT		"AReconnect"	// Automatically reconnect
#define GG_KEYDEF_ARECONNECT	0

#define GG_KEY_LEAVESTATUSMSG	"LeaveStatusMsg"// Leave status msg when disconnected
#define GG_KEYDEF_LEAVESTATUSMSG 0
#define GG_KEY_LEAVESTATUS		"LeaveStatus"
#define GG_KEYDEF_LEAVESTATUS	0

#define GG_KEY_FRIENDSONLY		"FriendsOnly"	// Friend only visibility
#define GG_KEYDEF_FRIENDSONLY	0

#define GG_KEY_SHOWNOTONMYLIST		"ShowNotOnMyList"	// Show contacts having me on their list but not on my list
#define GG_KEYDEF_SHOWNOTONMYLIST	0

#define GG_KEY_SHOWINVISIBLE	"ShowInvisible" // Show invisible users when described
#define GG_KEYDEF_SHOWINVISIBLE	0

#define GG_KEY_IGNORECONF		"IgnoreConf"	// Ignore incoming conference messages
#define GG_KEYDEF_IGNORECONF	0

#define GG_KEY_IMGRECEIVE		"ReceiveImg"	// Popup image window automatically
#define GG_KEYDEF_IMGRECEIVE	1

#define GG_KEY_IMGMETHOD		"PopupImg"		// Popup image window automatically
#define GG_KEYDEF_IMGMETHOD		1

// Hidden option
#define GG_KEY_STARTINVISIBLE	"StartInvisible"// Starts as invisible
#define GG_KEYDEF_STARTINVISIBLE 0

#define GG_KEY_MSGACK			"MessageAck"		// Acknowledge when sending msg
#define GG_KEYDEF_MSGACK		1

#define GG_KEY_MANUALHOST		"ManualHost"	// Specify by hand server host/port
#define GG_KEYDEF_MANUALHOST	0
// #define GG_KEY_SERVERHOST		"ServerHost"	// Host (depreciated)
// #define GG_KEY_SERVERPORT		"ServerPort"	// Port (depreciated)
#define GG_KEY_SSLCONN			"SSLConnection" // Use SSL/TLS for connections
#define GG_KEYDEF_SSLCONN		0
#define GG_KEY_SERVERHOSTS		"ServerHosts"	// NL separated list of hosts for server connection
#define GG_KEYDEF_SERVERHOSTS	"217.17.41.83\r\n217.17.41.85\r\n217.17.41.88\r\n217.17.41.89\r\n217.17.41.92\r\n217.17.41.93"


#define GG_KEY_CLIENTIP 		"IP"			// Contact IP (by notify)
#define GG_KEY_CLIENTPORT		"ClientPort"	// Contact port
#define GG_KEY_CLIENTVERSION "ClientVersion"	// Contact app version

#define GG_KEY_DIRECTCONNS		"DirectConns"	// Use direct connections
#define GG_KEYDEF_DIRECTCONNS	1
#define GG_KEY_DIRECTPORT		"DirectPort"	// Direct connections port
#define GG_KEYDEF_DIRECTPORT	1550

#define GG_KEY_FORWARDING		"Forwarding"	// Use forwarding
#define GG_KEYDEF_FORWARDING	0
#define GG_KEY_FORWARDHOST		"ForwardHost"	// Forwarding host (firewall)
#define GG_KEY_FORWARDPORT		"ForwardPort"	// Forwarding port (firewall port)
#define GG_KEYDEF_FORWARDPORT	1550			// Forwarding port (firewall port)

#define GG_KEY_GC_POLICY_UNKNOWN		"GCPolicyUnknown"
#define GG_KEYDEF_GC_POLICY_UNKNOWN 	1

#define GG_KEY_GC_COUNT_UNKNOWN 		"GCCountUnknown"
#define GG_KEYDEF_GC_COUNT_UNKNOWN		5

#define GG_KEY_GC_POLICY_TOTAL			"GCPolicyTotal"
#define GG_KEYDEF_GC_POLICY_TOTAL		1

#define GG_KEY_GC_COUNT_TOTAL			"GCCountTotal"
#define GG_KEYDEF_GC_COUNT_TOTAL		10

#define GG_KEY_GC_POLICY_DEFAULT		"GCPolicyDefault"
#define GG_KEYDEF_GC_POLICY_DEFAULT 	0

#define GG_KEY_DELETEUSER		"DeleteUser"	// When user is deleted

#define GG_KEY_APPARENT 		"ApparentMode"	// Visible list

#define GG_KEY_TIMEDEVIATION	"TimeDeviation" // Max time deviation for connections (seconds)
#define GG_KEYDEF_TIMEDEVIATION	300

// chpassdlgproc() multipurpose dialog proc modes
#define GG_USERUTIL_PASS	0
#define GG_USERUTIL_CREATE	1
#define GG_USERUTIL_REMOVE	2
#define GG_USERUTIL_EMAIL	3

#define WinVerMajor()			LOBYTE(LOWORD(GetVersion()))
#define WinVerMinor()			HIBYTE(LOWORD(GetVersion()))
#define IsWinVer2000Plus()		(WinVerMajor()>=5)
#define IsWinVerXPPlus()		(WinVerMajor()>=5 && LOWORD(GetVersion())!=5)
#define LocalEventUnhook(hook)	if(hook) UnhookEvent(hook)

// Mutex names
#define GG_IMG_DLGMUTEX "GG_dlgmutex"
#define GG_IMG_TABMUTEX "GG_tabmutex"
#define GG_IMG_QUEUEMUTEX "GG_queuemutex"

// Some MSVC compatibility with gcc
#ifdef _MSC_VER
#ifndef strcasecmp
#define strcasecmp _strcmpi
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#endif

// Global variables
/////////////////////////////////////////////////

extern HINSTANCE hInstance;
extern DWORD gMirandaVersion;
extern int ggStatus;
extern int ggDesiredStatus;
extern GGTHREAD *ggThread;
extern list_t ggThreadList;
extern HANDLE hNetlib;
#ifdef __GG_LIBGADU_HAVE_OPENSSL
extern HANDLE hLibSSL;
extern HANDLE hLibEAY;
#else
#define hLibSSL FALSE
#define hLibEAY FALSE
#endif
extern pthread_mutex_t threadMutex; 	// Used when modifying thread structure
extern pthread_mutex_t modeMsgsMutex;	// Used when modifying away msgs structure
extern char *ggTokenid;
extern char *ggTokenval;
struct gg_status_msgs
{
	char *szOnline;
	char *szAway;
	char *szInvisible;
	char *szOffline;
};
extern struct gg_status_msgs ggModeMsg;
extern uin_t nextUIN;
extern int ggListRemove;
extern int ggGCEnabled;
extern list_t ggGCList;

// Screen saver
#ifndef SPI_GETSCREENSAVERRUNNING
#define SPI_GETSCREENSAVERRUNNING 114
#endif

/////////////////////////////////////////////////
// Methods

/* Helper functions */
const char *http_error_string(int h);
unsigned long crc_get(char *mem);
int status_m2gg(int status, int descr);
int status_gg2s(int status);
char *gg_status2db(int status, const char *suffix);
char *ws_strerror(int code);
uint32_t swap32(uint32_t x);
const char *gg_version2string(int v);

/* Global GG functions */
void gg_refreshblockedicon();
void gg_notifyuser(HANDLE hContact, int refresh);
void gg_setalloffline();
void gg_disconnect();
void gg_cleanupthreads();
int gg_refreshstatus(int status);
HANDLE gg_getcontact(uin_t uin, int create, int inlist, char *nick);
void gg_registerservices();
void *__stdcall gg_mainthread(void *empty);
int gg_isonline();
int gg_netlog(const char *fmt, ...);
int gg_netsend(HANDLE s, char *data, int datalen);
void gg_broadcastnewstatus(int s);
int gg_userdeleted(WPARAM wParam, LPARAM lParam);
int gg_dbsettingchanged(WPARAM wParam, LPARAM lParam);
void gg_notifyall();
void gg_changecontactstatus(uin_t uin, int status, const char *idescr, int time, uint32_t remote_ip, uint16_t remote_port, uint32_t version);
char *gg_getstatusmsg(int status);
void gg_dccstart(GGTHREAD *thread);
void gg_waitdcc(GGTHREAD *thread);
void gg_dccconnect(uin_t uin);
int gg_recvfile(WPARAM wParam, LPARAM lParam);
int gg_sendfile(WPARAM wParam, LPARAM lParam);
int gg_fileallow(WPARAM wParam, LPARAM lParam);
int gg_filedeny(WPARAM wParam, LPARAM lParam);
int gg_filecancel(WPARAM wParam, LPARAM lParam);
int gg_gettoken();
void gg_parsecontacts(char *contacts);
int gg_getinfo(WPARAM wParam, LPARAM lParam);
void gg_remindpassword(uin_t uin, const char *email);
void gg_dccwait(GGTHREAD *thread);
void *gg_img_loadpicture(struct gg_event* e, HANDLE hContact, char *szFileName);
int gg_img_releasepicture(void *img);
int gg_img_display(HANDLE hContact, void *img);

/* Misc module initializers & destroyers */
void gg_import_init();
void gg_chpass_init();
void gg_userinfo_init();
void gg_userinfo_destroy();

/* Keep-alive module */
void gg_keepalive_init();
void gg_keepalive_destroy();

/* Image reception functions */
int gg_img_init();
int gg_img_destroy();
int gg_img_shutdown();
int gg_img_recvimage(WPARAM wParam, LPARAM lParam);
int gg_img_sendimage(WPARAM wParam, LPARAM lParam);
int gg_img_sendonrequest(struct gg_event* e);
BOOL gg_img_opened(uin_t uin);
void *__stdcall gg_img_dlgthread(void *empty);

/* UI page initializers */
int gg_options_init(WPARAM wParam, LPARAM lParam);
int gg_details_init(WPARAM wParam, LPARAM lParam);

/* Groupchat functions */
int gg_gc_init();
int gg_gc_destroy();
char * gg_gc_getchat(uin_t sender, uin_t *recipients, int recipients_count);
GGGC *gg_gc_lookup(char *id);
int gg_gc_changenick(HANDLE hContact, char *pszNick);
#define UIN2ID(uin,id) itoa(uin,id,10)

// Debug functions
#ifdef DEBUGMODE
const char *ggdebug_eventtype(struct gg_event *e);
#endif

/* SSL functions */
#ifdef __GG_LIBGADU_HAVE_OPENSSL
BOOL gg_ssl_init();
void gg_ssl_uninit();
#else
#define gg_ssl_init()
#define gg_ssl_uninit()
#endif

#ifdef __cplusplus
}
#endif
#endif
