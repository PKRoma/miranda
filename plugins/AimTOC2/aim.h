/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2005 Robert Rainwater

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
#ifndef AIM_H
#define AIM_H

#ifdef _DEBUG
#define DEBUGMODE
#endif

#define _WIN32_WINNT 0x0500
#define WINVER       0x0500

#include <windows.h>
#include <commctrl.h>
#include <process.h>
#include <stdio.h>
#include <time.h>
#include <richedit.h>
#include <mbstring.h>
#include <winsock.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_database.h>
#include <m_netlib.h>
#include <m_protocols.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_langpack.h>
#include <m_message.h>
#include <m_plugins.h>
#include <m_skin.h>
#include <m_utils.h>
#include <m_clist.h>
#include <m_options.h>
#include <m_userinfo.h>
#include <m_clui.h>
#include <m_addcontact.h>
#include <m_idle.h>
#include "sdkex/m_smileyadd.h"
#include "sdkex/m_chat.h"
#include "pthread.h"
#include "buddies.h"
#include "config.h"
#include "evilmode.h"
#include "filerecv.h"
#include "firstrun.h"
#include "groupchat.h"
#include "groups.h"
#include "idle.h"
#include "im.h"
#include "keepalive.h"
#include "links.h"
#include "list.h"
#include "log.h"
#include "options.h"
#include "password.h"
#include "search.h"
#include "server.h"
#include "services.h"
#include "resource.h"
#include "toc.h"
#include "userinfo.h"
#include "utils.h"

// Some hacks for gcc and sdkless people
#ifndef CFE_LINK
#define CFE_LINK 0x0020
#endif
#ifndef COLOR_HOTLIGHT
#define COLOR_HOTLIGHT 26
#endif
#ifndef FLASHW_TRAY
#define FLASHW_TRAY 0x00000002;
#endif
#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE	0x0040
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif
#define XFREE(x) if (x) free(x)

#define AIM_PROTONAME	"AIM"   // Protocol Name

// Chat
#define AIM_CHAT        "ChatRoom"

// URLs
#define AIM_REGISTER	"http://aim.aol.com/aimnew/newreg/register2.adp"
#define AIM_LOSTPW		"http://www.aim.com/help_faq/forgot_password/password.adp"
#define AIM_CHANGEPW	"https://aim.aol.com/password/change_password.adp?screenname="

// MISC
#define AIM_STR_AR		"[Auto-Response]"
#define AIM_STR_PR		"Miranda IM: Smaller, Faster, Easier\r\n\r\nhttp://miranda-im.org/"

#define AIM_SVC_EVIL    "AIM/EvilMode"

// Keys
#define AIM_KEY_UN      "SN"             // Screenname
#define AIM_KEY_PW      "Password"       // Password
#define AIM_KEY_FR      "FirstRun"       // Firstrun
#define AIM_KEY_NK      "Nick"           // Nick
#define AIM_KEY_NO      "OwnerNick"      // Nickname of owner
#define AIM_KEY_ST      "Status"         // Status
#define AIM_KEY_IS      "StartupStatus"  // Status used when starting up
#define AIM_KEY_DU      "DeleteUser"     // User is deleted
#define AIM_KEY_EV      "EvilLevel"      // Warning %
#define AIM_KEY_TP      "UserType"       // User type (aol user, etc)
#define AIM_KEY_IT      "IdleTS"         // Idle Time
#define AIM_KEY_OT      "LogonTS"        // Online Time
#define AIM_KEY_WM      "WarnMenu"       // Warn Menu Item
#define AIM_KEY_TS      "TOCServer"      // Server
#define AIM_KEY_TT      "TOCPort"        // Port
#define AIM_KEY_AS      "AuthServer"     // Server
#define AIM_KEY_AT      "AuthPort"       // Port
#define AIM_KEY_LM      "LastMessage"    // Last message received from user (used for evil warning)
#define AIM_KEY_WD      "ShowWarn"       // Show the warn dialog when warned
#define AIM_KEY_LA      "LastAwayChange" // Used for determining if autoresponse it sent
#define AIM_KEY_AR      "AutoResponse"   // Send autoresponse
#define AIM_KEY_AC      "ClistAutoResp"  // Only auto-response to users on your clist
#define AIM_KEY_AM      "ApparentMode"   // Apparent mode
#define AIM_KEY_SA		"ServerAddr"     // Server Address
#define AIM_KEY_PR		"Profile"        // Profile Info
#define AIM_KEY_SS		"SSExtra"        // Support server-side list
#define AIM_KEY_SM      "SSMode"         // Mode
#define AIM_KEY_SE      "ShowErrors"     // Show error popups
#define AIM_KEY_GI		"GroupChatIgInv" // Ignore invites
#define AIM_KEY_GM		"GroupChatMnMnu" // Main menu visibility
#define AIM_KEY_LSM		"SyncMnMnu"		 //Sync Main menu visiblity
#define AIM_KEY_GT		"GroupMin2Tray"  // Minimize to tray
#define AIM_KEY_AL		"AIMLinks"       // aim: links support
#define AIM_KEY_PM      "PasswordMenu"   // Show password menu
#define AIM_KEY_UU      "Homepage"       // User info url
#define AIM_KEY_LL      "InSSIList"      // In the SSI

#define WinVerMajor()			LOBYTE(LOWORD(GetVersion()))
#define WinVerMinor()			HIBYTE(LOWORD(GetVersion()))
#define IsWinVer2000Plus()		(WinVerMajor()>=5)
#define IsWinVerXPPlus()		(WinVerMajor()>=5 && LOWORD(GetVersion())!=5)
#define LocalEventUnhook(hook)	if(hook) UnhookEvent(hook)

extern HINSTANCE hInstance;
extern int aimStatus;
extern HANDLE hNetlib, hNetlibPeer;
extern char *szStatus;
extern char *szModeMsg;
extern char AIM_PROTO[MAX_PATH];

#endif
