////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003 Adam Strzelecki <ono+miranda@java.pl>
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

#ifdef __DEBUG__
#define DEBUGMODE               // Debug Mode
#endif

#if _WIN32_WINNT < 0x0500
#define _WIN32_WINNT 0x0500
#endif

// Windows headers
#include <windows.h>
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

// Plugin headers
#include "pthread.h"
#include "resource.h"
#include <winsock2.h>

// libgadu headers
#include "libgadu/libgadu.h"
#include "dynstuff.h"

#ifdef __cplusplus
extern "C" {
#endif
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

// Main strings
extern char *ggProto;
extern char *ggProtoName;
extern char *ggProtoError;
#define GG_PROTO 		 ggProto     	// Protocol ID
#define GG_PROTONAME	 ggProtoName	// Protocol Name
#define GG_PROTOERROR	 ggProtoError  	// Protocol Error
#define GGDEF_PROTO  	 "GG"			// Default Proto
#define GGDEF_PROTONAME  "Gadu-Gadu"	// Default ProtoName

// Process handles / seqs
#define GG_SEQ_INFO	        	100
#define GG_SEQ_SEARCH           200
#define GG_SEQ_AWAYMSG          300
#define GG_SEQ_GETNICK	        400
#define GG_SEQ_CHINFO	        500

// URLs
#define GG_LOSTPW	        	"http://ono.no/"

// Services
#define GGS_IMPORT_SERVER       "%s/ImportFromServer"
#define GGS_REMOVE_SERVER       "%s/RemoveFromServer"
#define GGS_IMPORT_TEXT         "%s/ImportFromText"
#define GGS_EXPORT_SERVER       "%s/ExportFromServer"
#define GGS_EXPORT_TEXT         "%s/ExportFromText"
#define GGS_CHPASS              "%s/ChangePassword"

#define GGS_SENDIMAGE           "%s/SendImage"
#define GGS_RECVIMAGE           "%s/RecvImage"

// Keys
#define GG_PLUGINVERSION  		"Version"		// Plugin version.. user for cleanup from previous versions

#define GG_KEY_UIN				"UIN"           // Uin - unique number
#define GG_KEY_PASSWORD			"Password"      // Password
#define GG_KEY_EMAIL			"e-mail"		// E-mail
#define GG_KEY_STATUS			"Status"        // Status
#define GG_KEY_STARTUPSTATUS	"StartupStatus" // Status used when starting up
#define GG_KEY_NICK				"Nick"          // Nick
#define GG_KEY_STATUSDESCR		"StatusDescr"   // Users status description

#define GG_KEY_KEEPALIVE		"KeepAlive"     // Keep-alive support
#define GG_KEYDEF_KEEPALIVE		1

#define GG_KEY_SHOWCERRORS		"ShowCErrors"   // Show connection errors
#define GG_KEYDEF_SHOWCERRORS	1

#define GG_KEY_ARECONNECT		"AReconnect"    // Automatically reconnect
#define GG_KEYDEF_ARECONNECT	0

#define GG_KEY_LEAVESTATUSMSG	"LeaveStatusMsg"// Leave status msg when disconnected
#define GG_KEYDEF_LEAVESTATUSMSG 0
#define GG_KEY_LEAVESTATUS      "LeaveStatus"
#define GG_KEYDEF_LEAVESTATUS   0

#define GG_KEY_FRIENDSONLY		"FriendsOnly"   // Friend only visibility
#define GG_KEYDEF_FRIENDSONLY	0

#define GG_KEY_SHOWINVISIBLE	"ShowInvisible" // Show invisible users when described
#define GG_KEYDEF_SHOWINVISIBLE	0

#define GG_KEY_IGNORECONF	    "IgnoreConf"    // Ignore incoming conference messages
#define GG_KEYDEF_IGNORECONF	1

#define GG_KEY_POPUPIMG	        "PopupImg"      // Popup image window automatically
#define GG_KEYDEF_POPUPIMG	    1

// Hidden option
#define GG_KEY_STARTINVISIBLE   "StartInvisible"// Starts as invisible
#define GG_KEYDEF_STARTINVISIBLE 0

#define GG_KEY_MSGACK			"MessageAck"   		// Acknowledge when sending msg
#define GG_KEYDEF_MSGACK		1

#define GG_KEY_MANUALHOST		"ManualHost"    // Specify by hand server host/port
#define GG_KEYDEF_MANUALHOST	0
#define GG_KEY_SERVERHOST		"ServerHost"    // Host
#define GG_KEY_SERVERPORT		"ServerPort"    // Port
#define GG_KEY_SSLCONN			"SSLConnection" // Use SSL/TLS for connections
#define GG_KEYDEF_SSLCONN		1


#define GG_KEY_CLIENTIP 		"IP"      		// Contact IP (by notify)
#define GG_KEY_CLIENTPORT		"ClientPort"    // Contact port
#define GG_KEY_CLIENTVERSION "ClientVersion"    // Contact app version

#define GG_KEY_DIRECTCONNS		"DirectConns"   // Use direct connections
#define GG_KEYDEF_DIRECTCONNS	1
#define GG_KEY_DIRECTPORT		"DirectPort"    // Direct connections port
#define GG_KEYDEF_DIRECTPORT	1550

#define GG_KEY_FORWARDING		"Forwarding"    // Use forwarding
#define GG_KEYDEF_FORWARDING	0
#define GG_KEY_FORWARDHOST		"ForwardHost"   // Forwarding host (firewall)
#define GG_KEY_FORWARDPORT		"ForwardPort"   // Forwarding port (firewall port)
#define GG_KEYDEF_FORWARDPORT	1550    		// Forwarding port (firewall port)

#define GG_KEY_DELETEUSER		"DeleteUser"    // When user is deleted

#define GG_KEY_APPARENT         "ApparentMode"  // Visible list

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
extern int ggStatus;
extern int ggDesiredStatus;
extern int ggRunning;
extern HANDLE hNetlib;
extern HANDLE hLibSSL;
extern HANDLE hLibEAY;
extern pthread_t serverThreadId;
extern pthread_t dccServerThreadId;
extern pthread_mutex_t connectionHandleMutex;
extern pthread_mutex_t modeMsgsMutex;
extern pthread_mutex_t dccWatchesMutex;
extern struct gg_session *ggSess;
extern struct gg_dcc *ggDcc;
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

// Screen saver
#ifndef SPI_GETSCREENSAVERRUNNING
#define SPI_GETSCREENSAVERRUNNING 114
#endif

// Methods
/////////////////////////////////////////////////

const char *http_error_string(int h);
unsigned long crc_get(char *mem);
void gg_refreshblockedicon();
void gg_notifyuser(HANDLE hContact, int refresh);
void gg_setalloffline();
void gg_disconnect();
int gg_refreshstatus(int status);
int status_m2gg(int status, int descr);
int status_gg2s(int status);
void gg_initimport();
void gg_initchpass();
HANDLE gg_getcontact(uin_t uin, int create, int inlist, char *nick);
void gg_initkeepalive();
void gg_destroykeepalive();
int gg_optionsinit(WPARAM wParam, LPARAM lParam);
int gg_detailsinit(WPARAM wParam, LPARAM lParam);
void gg_registerservices();
void *__stdcall gg_mainthread(void *empty);
void gg_inituserinfo();
void gg_destroyuserinfo();
int gg_isonline();
int gg_netlog(const char *fmt, ...);
int gg_netsend(HANDLE s, char *data, int datalen);
void gg_broadcastnewstatus(int s);
int gg_userdeleted(WPARAM wParam, LPARAM lParam);
int gg_dbsettingchanged(WPARAM wParam, LPARAM lParam);
void gg_notifyall();
void gg_changecontactstatus(uin_t uin, int status, const char *idescr, int time, uint32_t remote_ip, uint16_t remote_port, uint32_t version);
char *StatusModeToDbSetting(int status, const char *suffix);
char *gg_getstatusmsg(int status);
void gg_dccstart(HANDLE event);
void gg_dccstop();
void gg_dccconnect(uin_t uin);
int gg_recvfile(WPARAM wParam, LPARAM lParam);
int gg_sendfile(WPARAM wParam, LPARAM lParam);
int gg_fileallow(WPARAM wParam, LPARAM lParam);
int gg_filedeny(WPARAM wParam, LPARAM lParam);
int gg_filecancel(WPARAM wParam, LPARAM lParam);
int gg_gettoken();
void gg_parsecontacts(char *contacts);
int gg_img_load();
int gg_img_unload();
int gg_img_recvimage(WPARAM wParam, LPARAM lParam);
int gg_img_sendimage(WPARAM wParam, LPARAM lParam);
int gg_img_sendonrequest(struct gg_event* e);
BOOL gg_img_opened(uin_t uin);
void *__stdcall gg_img_dlgthread(void *empty);

// Debug functions
#ifdef DEBUGMODE
const char *ggdebug_eventtype(struct gg_event *e);
#endif

#ifdef __cplusplus
}
#endif
#endif
