/*

Jabber Protocol Plugin for Miranda IM
Copyright (C) 2002-2004  Santithorn Bunchua

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

#ifndef _JABBER_H_
#define _JABBER_H_

#include <malloc.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

/*******************************************************************
 * Global header files
 *******************************************************************/
#define _WIN32_WINNT 0x500
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_netlib.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_clist.h>
#include <m_clui.h>
#include <m_options.h>
#include <m_userinfo.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_utils.h>
#include <m_message.h>
#include <m_skin.h>
#include "jabber_xml.h"
#include "jabber_byte.h"

/*******************************************************************
 * Global constants
 *******************************************************************/
#define JABBER_DEFAULT_PORT 5222
#define JABBER_IQID "keh_"
// User-defined message
#define WM_JABBER_REGDLG_UPDATE				WM_USER + 100
#define WM_JABBER_AGENT_REFRESH				WM_USER + 101
#define WM_JABBER_TRANSPORT_REFRESH			WM_USER + 102
#define WM_JABBER_REGINPUT_ACTIVATE			WM_USER + 103
#define WM_JABBER_REFRESH					WM_USER + 104
#define WM_JABBER_CHECK_ONLINE				WM_USER + 105
#define WM_JABBER_CHANGED					WM_USER + 106
#define WM_JABBER_ACTIVATE					WM_USER + 107
#define WM_JABBER_SET_FONT					WM_USER + 108
#define WM_JABBER_FLASHWND					WM_USER + 109
#define WM_JABBER_GC_MEMBER_ADD				WM_USER + 110
#define WM_JABBER_GC_FORCE_QUIT				WM_USER + 111
#define WM_JABBER_SHUTDOWN					WM_USER + 112
#define WM_JABBER_SMILEY					WM_USER + 113
// Error code
#define JABBER_ERROR_REDIRECT				302
#define JABBER_ERROR_BAD_REQUEST			400
#define JABBER_ERROR_UNAUTHORIZED			401
#define JABBER_ERROR_PAYMENT_REQUIRED		402
#define JABBER_ERROR_FORBIDDEN				403
#define JABBER_ERROR_NOT_FOUND				404
#define JABBER_ERROR_NOT_ALLOWED			405
#define JABBER_ERROR_NOT_ACCEPTABLE			406
#define JABBER_ERROR_REGISTRATION_REQUIRED	407
#define JABBER_ERROR_REQUEST_TIMEOUT		408
#define JABBER_ERROR_CONFLICT				409
#define JABBER_ERROR_INTERNAL_SERVER_ERROR	500
#define JABBER_ERROR_NOT_IMPLEMENTED		501
#define JABBER_ERROR_REMOTE_SERVER_ERROR	502
#define JABBER_ERROR_SERVICE_UNAVAILABLE	503
#define JABBER_ERROR_REMOTE_SERVER_TIMEOUT	504
// Vcard flag
#define JABBER_VCEMAIL_HOME			1
#define JABBER_VCEMAIL_WORK			2
#define JABBER_VCEMAIL_INTERNET		4
#define JABBER_VCEMAIL_X400			8
#define JABBER_VCTEL_HOME			1
#define JABBER_VCTEL_WORK			2
#define JABBER_VCTEL_VOICE			4
#define JABBER_VCTEL_FAX			8
#define JABBER_VCTEL_PAGER			16
#define JABBER_VCTEL_MSG			32
#define JABBER_VCTEL_CELL			64
#define JABBER_VCTEL_VIDEO			128
#define JABBER_VCTEL_BBS			256
#define JABBER_VCTEL_MODEM			512
#define JABBER_VCTEL_ISDN			1024
#define JABBER_VCTEL_PCS			2048
// File transfer setting
#define JABBER_OPTION_FT_DIRECT		0	// Direct connection
#define JABBER_OPTION_FT_PASS		1	// Use PASS server
#define JABBER_OPTION_FT_PROXY		2	// Use proxy with local port forwarding
// Font style saved in DB
#define JABBER_FONT_BOLD			1
#define JABBER_FONT_ITALIC			2
// Font for groupchat log dialog
#define JABBER_GCLOG_NUM_FONT		6	// 6 fonts (0:send, 1:msg, 2:time, 3:nick, 4:sys, 5:/me)
// Old SDK don't have this
#ifndef SPI_GETSCREENSAVERRUNNING
#define SPI_GETSCREENSAVERRUNNING 114
#endif
#define IDC_STATIC (-1)
// Icon list
enum {
	JABBER_IDI_GCOWNER = 0,
	JABBER_IDI_GCADMIN,
	JABBER_IDI_GCMODERATOR,
	JABBER_IDI_GCVOICE,
	JABBER_ICON_TOTAL
};

/*******************************************************************
 * Macro definitions
 *******************************************************************/
#ifdef _DEBUG
void JabberLog(const char* fmt, ...);
#else
#define JabberLog()
#pragma warning(disable:4002)
#endif
#define JabberSendInvisiblePresence()	JabberSendVisibleInvisiblePresence(TRUE)
#define JabberSendVisiblePresence()		JabberSendVisibleInvisiblePresence(FALSE)

/*******************************************************************
 * Global data structures and data type definitions
 *******************************************************************/
typedef HANDLE JABBER_SOCKET;

typedef enum {
	JABBER_SESSION_NORMAL,
	JABBER_SESSION_REGISTER
} JABBER_SESSION_TYPE;

struct ThreadData {
	HANDLE hThread;
	JABBER_SESSION_TYPE type;

	char username[128];
	char password[128];
	char server[128];
	char manualHost[128];
	char resource[128];
	char fullJID[256];
	WORD port;
	JABBER_SOCKET s;
	BOOL useSSL;

	char newPassword[128];

	HWND reg_hwndDlg;
	BOOL reg_done;
};

typedef struct {
	char* szOnline;
	char* szAway;
	char* szNa;
	char* szDnd;
	char* szFreechat;
} JABBER_MODEMSGS;

typedef struct {
	char username[128];
	char password[128];
	char server[128];
	char manualHost[128];
	WORD port;
	BOOL useSSL;
} JABBER_REG_ACCOUNT;

typedef enum { FT_SI, FT_OOB, FT_BYTESTREAM } JABBER_FT_TYPE;
typedef enum { FT_CONNECTING, FT_INITIALIZING, FT_RECEIVING, FT_DONE, FT_ERROR, FT_DENIED } JABBER_FILE_STATE;

typedef struct {
	HANDLE hContact;
	JABBER_FT_TYPE type;
	JABBER_SOCKET s;
	JABBER_FILE_STATE state;
	char* jid;
	int fileId;
	char* iqId;
	char* sid;

	// For type == FT_BYTESTREAM
	JABBER_BYTE_TRANSFER *jbt;

	// Used by file receiving only
	char* httpHostName;
	WORD httpPort;
	char* httpPath;
	char* szSavePath;
	long fileReceivedBytes;
	long fileTotalSize;
	char* fullFileName;
	char* fileName;

	// Used by file sending only
	HANDLE hFileEvent;
	int fileCount;
	char* *files;
	long *fileSize;
	//char* httpPath;			// Name of the requested file
	//long fileTotalSize;		// Size of the current file (file being sent)
	long allFileTotalSize;
	long allFileReceivedBytes;
	char* szDescription;
	int currentFile;
} JABBER_FILE_TRANSFER;

typedef struct {
	PROTOSEARCHRESULT hdr;
	char jid[256];
} JABBER_SEARCH_RESULT;

typedef struct {
	char face[LF_FACESIZE];		// LF_FACESIZE is from LOGFONT struct
	BYTE style;
	char size;	// signed
	BYTE charset;
	COLORREF color;
} JABBER_GCLOG_FONT;

typedef struct {
	int id;
	char* name;
} JABBER_FIELD_MAP;

typedef enum {
	MUC_VOICELIST,
	MUC_MEMBERLIST,
	MUC_MODERATORLIST,
	MUC_BANLIST,
	MUC_ADMINLIST,
	MUC_OWNERLIST
} JABBER_MUC_JIDLIST_TYPE;

typedef struct {
	JABBER_MUC_JIDLIST_TYPE type;
	char* roomJid;	// filled-in by the WM_JABBER_REFRESH code
	XmlNode *iqNode;
	HWND hwndAddJid;
} JABBER_MUC_JIDLIST_INFO;

typedef struct {
	char* roomJid;		// stored in UTF-8
	int hSplitterPos;	// position from right edge
	int vSplitterPos;	// position from bottom edge
	int hSplitterMinAbove;
	int hSplitterMinBelow;
	int vSplitterMinRight;
	int vSplitterMinLeft;
	WNDPROC oldEditWndProc;
	WNDPROC oldHSplitterWndProc;
	WNDPROC oldVSplitterWndProc;
	HWND hwndSetTopic;
	HWND hwndChangeNick;
	HWND hwndKickReason;
	HWND hwndBanReason;
	HWND hwndDestroyReason;
	int nFlash;
} JABBER_GCLOG_INFO;

typedef enum {
	MUC_SETTOPIC,
	MUC_CHANGENICK,
	MUC_KICKREASON,
	MUC_BANREASON,
	MUC_DESTROYREASON,
	MUC_ADDJID
} JABBER_GCLOG_INPUT_TYPE;

typedef struct {
	JABBER_GCLOG_INPUT_TYPE type;
	JABBER_GCLOG_INFO *gcLogInfo;
	JABBER_MUC_JIDLIST_INFO *jidListInfo;
	char* nick;
} JABBER_GCLOG_INPUT_INFO;

typedef void (*JABBER_FORM_SUBMIT_FUNC)(char* submitStr, void *userdata);
typedef void (__cdecl *JABBER_THREAD_FUNC)(void *);
/*******************************************************************
 * Global variables
 *******************************************************************/
extern HINSTANCE hInst;
extern HANDLE hMainThread;
extern DWORD jabberMainThreadId;
extern char* jabberProtoName;
extern char* jabberModuleName;
extern HANDLE hNetlibUser;
extern HMODULE hLibSSL;
extern PVOID jabberSslCtx;

extern struct ThreadData *jabberThreadInfo;
extern char* jabberJID;
extern char* streamId;
extern DWORD jabberLocalIP;
extern BOOL jabberConnected;
extern BOOL jabberOnline;
extern int jabberStatus;
extern int jabberDesiredStatus;
//extern char* jabberModeMsg;
extern CRITICAL_SECTION modeMsgMutex;
extern JABBER_MODEMSGS modeMsgs;
extern BOOL modeMsgStatusChangePending;
extern BOOL jabberChangeStatusMessageOnly;
extern BOOL jabberSendKeepAlive;
extern HICON jabberIcon[JABBER_ICON_TOTAL];

extern HANDLE hEventSettingChanged;
extern HANDLE hEventContactDeleted;

extern HANDLE hMenuAgent;
extern HANDLE hMenuChangePassword;
extern HANDLE hMenuGroupchat;
extern HANDLE hMenuRequestAuth;
extern HANDLE hMenuGrantAuth;

extern HANDLE hWndListGcLog;

extern HWND hwndJabberAgents;
extern HWND hwndAgentReg;
extern HWND hwndAgentRegInput;
extern HWND hwndAgentManualReg;
extern HWND hwndRegProgress;
extern HWND hwndJabberVcard;
extern HWND hwndJabberChangePassword;
extern HWND hwndJabberGroupchat;
extern HWND hwndJabberJoinGroupchat;
extern HWND hwndMucVoiceList;
extern HWND hwndMucMemberList;
extern HWND hwndMucModeratorList;
extern HWND hwndMucBanList;
extern HWND hwndMucAdminList;
extern HWND hwndMucOwnerList;

/*******************************************************************
 * Function declarations
 *******************************************************************/

void __cdecl JabberServerThread(struct ThreadData *info);
// jabber_ws.c
BOOL JabberWsInit(void);
void JabberWsUninit(void);
JABBER_SOCKET JabberWsConnect(char* host, WORD port);
int JabberWsSend(JABBER_SOCKET s, char* data, int datalen);
int JabberWsRecv(JABBER_SOCKET s, char* data, long datalen);
// jabber_util.c
void JabberSerialInit(void);
void JabberSerialUninit(void);
unsigned int JabberSerialNext(void);
int JabberSend(JABBER_SOCKET s, const char* fmt, ...);
HANDLE JabberHContactFromJID(const char* jid);
char* JabberNickFromJID(const char* jid);
char* JabberLocalNickFromJID(const char* jid);
void JabberUrlDecode(char* str);
void JabberUrlDecodeW(WCHAR* str);
char* JabberUrlEncode(const char* str);
void JabberUtf8Decode( char*,WCHAR** );
char* JabberUtf8Encode(const char* str);
char* JabberSha1(char* str);
char* JabberUnixToDos(const char* str);
WCHAR* JabberUnixToDosW(const WCHAR* str);
void JabberHttpUrlDecode(char* str);
char* JabberHttpUrlEncode(const char* str);
int JabberCombineStatus(int status1, int status2);
char* JabberErrorStr(int errorCode);
char* JabberErrorMsg(XmlNode *errorNode);
void JabberSendVisibleInvisiblePresence(BOOL invisible);
char* JabberTextEncode(const char* str);
char* JabberTextEncodeW(const wchar_t *str);
char* JabberTextDecode(const char* str);
char* JabberBase64Encode(const char* buffer, int bufferLen);
char* JabberBase64Decode(const char* buffer, int *resultLen);
char* JabberGetVersionText();
time_t JabberIsoToUnixTime(char* stamp);
int JabberCountryNameToId(char* ctry);
void JabberSendPresenceTo(int status, char* to, char* extra);
void JabberSendPresence(int);
char* JabberRtfEscape(char* str);
void JabberStringAppend(char* *str, int *sizeAlloced, const char* fmt, ...);
char* JabberGetClientJID(char* jid);
// jabber_misc.c
void JabberDBAddAuthRequest(char* jid, char* nick);
HANDLE JabberDBCreateContact(char* jid, char* nick, BOOL temporary, BOOL stripResource);
void JabberContactListCreateGroup(char* groupName);
unsigned long JabberForkThread(void (__cdecl *threadcode)(void*), unsigned long stacksize, void *arg);
// jabber_file.c
void JabberFileFreeFt(JABBER_FILE_TRANSFER *ft);
void __cdecl JabberFileReceiveThread(JABBER_FILE_TRANSFER *ft);
void __cdecl JabberFileServerThread(JABBER_FILE_TRANSFER *ft);
// jabber_groupchat.c
int JabberMenuHandleGroupchat(WPARAM wParam, LPARAM lParam);
void JabberGroupchatProcessPresence(XmlNode *node, void *userdata);
void JabberGroupchatProcessMessage(XmlNode *node, void *userdata);
void JabberGroupchatProcessInvite(char* roomJid, char* from, char* reason, char* password);
// jabber_groupchatlog.c
void JabberGcLogInit();
void JabberGcLogUninit();
void JabberGcLogLoadFont(JABBER_GCLOG_FONT *fontInfo);
void JabberGcLogCreate(char* jid);
void JabberGcLogAppend(char* jid, time_t timestamp, char* str);
void JabberGcLogUpdateMemberStatus(char* jid);
void JabberGcLogSetTopic(char* jid, char* str);
BOOL CALLBACK JabberGcLogInputDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
// jabber_form.c
void JabberFormCreateUI(HWND hwndStatic, XmlNode *xNode, int *formHeight);
char* JabberFormGetData(HWND hwndStatic, XmlNode *xNode);
void JabberFormCreateDialog(XmlNode *xNode, char* defTitle, JABBER_FORM_SUBMIT_FUNC pfnSubmit, void *userdata);
// jabber_ft.c
void JabberFtCancel(JABBER_FILE_TRANSFER *ft);
void JabberFtInitiate(char* jid, JABBER_FILE_TRANSFER *ft);
void JabberFtHandleSiRequest(XmlNode *iqNode);
void JabberFtAcceptSiRequest(JABBER_FILE_TRANSFER *ft);
BOOL JabberFtHandleBytestreamRequest(XmlNode *iqNode);

#include "jabber_list.h"

#endif