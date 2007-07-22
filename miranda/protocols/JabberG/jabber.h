/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

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

// this plugin is for Miranda 0.7 only
#define MIRANDA_VER 0x0700

#if defined(UNICODE) && !defined(_UNICODE)
	#define _UNICODE
#endif

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)
#define NEWTSTR_ALLOCA(A) (A==NULL)?NULL:_tcscpy((TCHAR*)alloca(sizeof(TCHAR)*(_tcslen(A)+1)),A)

#include <tchar.h>
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
#include <m_system_cpp.h>
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
#include <m_chat.h>
#include <m_clc.h>
#include <m_button.h>
#include <m_avatars.h>
#include <m_idle.h>
#include <win2k.h>

#include "../../plugins/zlib/zlib.h"

#include "jabber_xml.h"
#include "jabber_byte.h"
#include "jabber_ibb.h"

#if !defined(OPENFILENAME_SIZE_VERSION_400)
	#define OPENFILENAME_SIZE_VERSION_400 sizeof(OPENFILENAME)
#endif

/*******************************************************************
 * Global constants
 *******************************************************************/
#define JABBER_DEFAULT_PORT 5222
#define JABBER_IQID "mir_"
#define JABBER_MAX_JID_LEN  256

// registered db event types
#define JABBER_DB_EVENT_TYPE_CHATSTATES          25368
#define JS_DB_GETEVENTTEXT_CHATSTATES            "/GetEventText25368"
#define JABBER_DB_EVENT_CHATSTATES_GONE          1

// User-defined message
#define WM_JABBER_REGDLG_UPDATE        WM_USER + 100
#define WM_JABBER_AGENT_REFRESH        WM_USER + 101
#define WM_JABBER_TRANSPORT_REFRESH    WM_USER + 102
#define WM_JABBER_REGINPUT_ACTIVATE    WM_USER + 103
#define WM_JABBER_REFRESH              WM_USER + 104
#define WM_JABBER_CHECK_ONLINE         WM_USER + 105
#define WM_JABBER_CHANGED              WM_USER + 106
#define WM_JABBER_ACTIVATE             WM_USER + 107
#define WM_JABBER_SET_FONT             WM_USER + 108
#define WM_JABBER_FLASHWND             WM_USER + 109
#define WM_JABBER_GC_MEMBER_ADD        WM_USER + 110
#define WM_JABBER_GC_FORCE_QUIT        WM_USER + 111
#define WM_JABBER_SHUTDOWN             WM_USER + 112
#define WM_JABBER_SMILEY               WM_USER + 113
#define WM_JABBER_JOIN                 WM_USER + 114
#define WM_JABBER_ADD_TO_ROSTER        WM_USER + 115
#define WM_JABBER_ADD_TO_BOOKMARKS     WM_USER + 116

// Error code
#define JABBER_ERROR_REDIRECT                    302
#define JABBER_ERROR_BAD_REQUEST                 400
#define JABBER_ERROR_UNAUTHORIZED                401
#define JABBER_ERROR_PAYMENT_REQUIRED            402
#define JABBER_ERROR_FORBIDDEN                   403
#define JABBER_ERROR_NOT_FOUND                   404
#define JABBER_ERROR_NOT_ALLOWED                 405
#define JABBER_ERROR_NOT_ACCEPTABLE              406
#define JABBER_ERROR_REGISTRATION_REQUIRED       407
#define JABBER_ERROR_REQUEST_TIMEOUT             408
#define JABBER_ERROR_CONFLICT                    409
#define JABBER_ERROR_INTERNAL_SERVER_ERROR       500
#define JABBER_ERROR_NOT_IMPLEMENTED             501
#define JABBER_ERROR_REMOTE_SERVER_ERROR         502
#define JABBER_ERROR_SERVICE_UNAVAILABLE         503
#define JABBER_ERROR_REMOTE_SERVER_TIMEOUT       504

// Vcard flags
#define JABBER_VCEMAIL_HOME                        1
#define JABBER_VCEMAIL_WORK                        2
#define JABBER_VCEMAIL_INTERNET                    4
#define JABBER_VCEMAIL_X400                        8

#define JABBER_VCTEL_HOME                     0x0001
#define JABBER_VCTEL_WORK                     0x0002
#define JABBER_VCTEL_VOICE                    0x0004
#define JABBER_VCTEL_FAX                      0x0008
#define JABBER_VCTEL_PAGER                    0x0010
#define JABBER_VCTEL_MSG                      0x0020
#define JABBER_VCTEL_CELL                     0x0040
#define JABBER_VCTEL_VIDEO                    0x0080
#define JABBER_VCTEL_BBS                      0x0100
#define JABBER_VCTEL_MODEM                    0x0200
#define JABBER_VCTEL_ISDN                     0x0400
#define JABBER_VCTEL_PCS                      0x0800

// File transfer setting
#define JABBER_OPTION_FT_DIRECT    0	// Direct connection
#define JABBER_OPTION_FT_PASS      1	// Use PASS server
#define JABBER_OPTION_FT_PROXY     2	// Use proxy with local port forwarding

// Font style saved in DB
#define JABBER_FONT_BOLD           1
#define JABBER_FONT_ITALIC         2

// Font for groupchat log dialog
#define JABBER_GCLOG_NUM_FONT      6	// 6 fonts ( 0:send, 1:msg, 2:time, 3:nick, 4:sys, 5:/me )

// Old SDK don't have this
#ifndef SPI_GETSCREENSAVERRUNNING
#define SPI_GETSCREENSAVERRUNNING 114
#endif
#define IDC_STATIC ( -1 )

// Icon list
enum {
	JABBER_IDI_GCOWNER = 0,
	JABBER_IDI_GCADMIN,
	JABBER_IDI_GCMODERATOR,
	JABBER_IDI_GCVOICE,
	JABBER_ICON_TOTAL
};

// Services and Events
#define JE_RAWXMLIN                "/RawXMLIn"
#define JE_RAWXMLOUT               "/RawXMLOut"

#define JS_SENDXML                 "/SendXML"
#define JS_GETADVANCEDSTATUSICON   "/GetAdvancedStatusIcon"
#define JS_GETXSTATUS              "/GetXStatus"
#define JS_SETXSTATUS              "/SetXStatus"

#define DBSETTING_XSTATUSID        "XStatusID"
#define DBSETTING_XSTATUSNAME      "XStatusName"
#define DBSETTING_XSTATUSMSG       "XStatusMsg"

/*******************************************************************
 * Global data structures and data type definitions
 *******************************************************************/
typedef HANDLE JABBER_SOCKET;

enum JABBER_SESSION_TYPE
{
	JABBER_SESSION_NORMAL,
	JABBER_SESSION_REGISTER
};

#define CAPS_BOOKMARK         0x0001
#define CAPS_BOOKMARKS_LOADED 0x8000

#define ZLIB_CHUNK_SIZE 2048

struct ThreadData
{
	ThreadData( JABBER_SESSION_TYPE parType );
	~ThreadData();

	HANDLE hThread;
	JABBER_SESSION_TYPE type;

	// network support
	JABBER_SOCKET s;
	BOOL  useSSL;
	PVOID ssl;
	CRITICAL_SECTION iomutex; // protects i/o operations

	// XEP-0138 (Compression support)
	BOOL     useZlib;
	z_stream zStreamIn,zStreamOut;
	bool     zRecvReady;
	int      zRecvDatalen;
	char*    zRecvData;

	BOOL     zlibInit( void );
	void     zlibUninit();
   int      zlibSend( char* data, int datalen );
   int      zlibRecv( char* data, long datalen );

	// for nick names resolving
	int    resolveID;
	HANDLE resolveContact;

	// features & registration
	HWND  reg_hwndDlg;
	BOOL  reg_done, bIsSessionAvailable;
	class TJabberAuth* auth;
	int	caps; // capabilities

	// connection & login data
	TCHAR username[128];
	char  password[128];
	char  server[128];
	char  manualHost[128];
	TCHAR resource[128];
	TCHAR fullJID[256];
	WORD  port;
	char  newPassword[128];

	void  close( void );
	int   recv( char* buf, size_t len );
	int   send( char* buffer, int bufsize );
	int   send( const char* fmt, ... );
	int   send( struct XmlNode& node );

	int   recvws( char* buffer, size_t bufsize, int flags );
	int   sendws( char* buffer, size_t bufsize, int flags );
};

struct JABBER_MODEMSGS
{
	char* szOnline;
	char* szAway;
	char* szNa;
	char* szDnd;
	char* szFreechat;
};

struct JABBER_REG_ACCOUNT
{
	TCHAR username[128];
	TCHAR password[128];
	char server[128];
	char manualHost[128];
	WORD port;
	BOOL useSSL;
};

typedef enum { FT_SI, FT_OOB, FT_BYTESTREAM, FT_IBB } JABBER_FT_TYPE;
typedef enum { FT_CONNECTING, FT_INITIALIZING, FT_RECEIVING, FT_DONE, FT_ERROR, FT_DENIED } JABBER_FILE_STATE;

struct filetransfer
{
	filetransfer();
	~filetransfer();

	void close();
	void complete();
	int  create();

	PROTOFILETRANSFERSTATUS std;

//	HANDLE hContact;
	JABBER_FT_TYPE type;
	JABBER_SOCKET s;
	JABBER_FILE_STATE state;
	TCHAR* jid;
	int    fileId;
	TCHAR* iqId;
	TCHAR* sid;
	int    bCompleted;
	HANDLE hWaitEvent;
	WCHAR* wszFileName;

	// For type == FT_BYTESTREAM
	JABBER_BYTE_TRANSFER *jbt;

	JABBER_IBB_TRANSFER *jibb;

	// Used by file receiving only
	char* httpHostName;
	WORD httpPort;
	char* httpPath;
	DWORD dwExpectedRecvFileSize;

	// Used by file sending only
	HANDLE hFileEvent;
	long *fileSize;
	char* szDescription;
};

struct JABBER_SEARCH_RESULT
{
	PROTOSEARCHRESULT hdr;
	TCHAR jid[256];
};

struct JABBER_GCLOG_FONT
{
	char face[LF_FACESIZE];		// LF_FACESIZE is from LOGFONT struct
	BYTE style;
	char size;	// signed
	BYTE charset;
	COLORREF color;
};

struct JABBER_FIELD_MAP
{
	int id;
	char* name;
};

enum JABBER_MUC_JIDLIST_TYPE
{
	MUC_VOICELIST,
	MUC_MEMBERLIST,
	MUC_MODERATORLIST,
	MUC_BANLIST,
	MUC_ADMINLIST,
	MUC_OWNERLIST
};

struct JABBER_MUC_JIDLIST_INFO
{
	JABBER_MUC_JIDLIST_TYPE type;
	TCHAR* roomJid;	// filled-in by the WM_JABBER_REFRESH code
	XmlNode *iqNode;

	TCHAR* type2str( void ) const;
};

typedef void ( *JABBER_FORM_SUBMIT_FUNC )( XmlNode* values, void *userdata );

#include "jabber_list.h"

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

extern ThreadData* jabberThreadInfo;
extern TCHAR* jabberJID;
extern char*  streamId;
extern DWORD  jabberLocalIP;
extern BOOL   jabberConnected;
extern BOOL   jabberOnline;
extern int    jabberStatus;
extern int    jabberDesiredStatus;
extern int    jabberSearchID;
extern time_t jabberLoggedInTime;
extern time_t jabberIdleStartTime;

extern CRITICAL_SECTION modeMsgMutex;
extern JABBER_MODEMSGS modeMsgs;
extern BOOL modeMsgStatusChangePending;

extern BOOL   jabberChangeStatusMessageOnly;
extern BOOL   jabberSendKeepAlive;
extern BOOL   jabberPepSupported;
extern BOOL   jabberChatDllPresent;
extern JabberCapsBits jabberServerCaps;

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
extern HWND hwndJabberBookmarks;
extern HWND hwndJabberAddBookmark;
extern HWND hwndJabberInfo;
extern HWND hwndPrivacyLists;
extern HWND hwndPrivacyRule;
extern HWND hwndServiceDiscovery;


extern const char xmlnsOwner[], xmlnsAdmin[];
// Service and event handles
extern HANDLE heventRawXMLIn;
extern HANDLE heventRawXMLOut;

// Transports list
extern LIST<TCHAR> jabberTransports;

/*******************************************************************
 * Function declarations
 *******************************************************************/

void JabberUpdateDialogs( BOOL bEnable );

//---- jabber_chat.cpp ----------------------------------------------

void JabberGcLogCreate( JABBER_LIST_ITEM* item );
void JabberGcLogUpdateMemberStatus( JABBER_LIST_ITEM* item, TCHAR* nick, TCHAR* jid, int action, XmlNode* reason );
void JabberGcQuit( JABBER_LIST_ITEM* jid, int code, XmlNode* reason );

//---- jabber_file.c ------------------------------------------------

void __cdecl JabberFileReceiveThread( filetransfer* ft );
void __cdecl JabberFileServerThread( filetransfer* ft );

//---- jabber_treelist.c ------------------------------------------------

typedef struct TTreeList_ItemInfo *HTREELISTITEM;

enum { TLM_TREE, TLM_REPORT };

void TreeList_Create(HWND hwnd);
void TreeList_Destroy(HWND hwnd);
void TreeList_Reset(HWND hwnd);
void TreeList_SetMode(HWND hwnd, int mode);
HTREELISTITEM TreeList_GetActiveItem(HWND hwnd);
void TreeList_SetSortMode(HWND hwnd, int col, BOOL descending);
void TreeList_SetFilter(HWND hwnd, TCHAR *filter);
HTREELISTITEM TreeList_AddItem(HWND hwnd, HTREELISTITEM hParent, TCHAR *text, LPARAM data);
void TreeList_ResetItem(HWND hwnd, HTREELISTITEM hParent);
void TreeList_MakeFakeParent(HTREELISTITEM hItem, BOOL flag);
void TreeList_AppendColumn(HTREELISTITEM hItem, TCHAR *text);
int TreeList_AddIcon(HWND hwnd, HICON hIcon, int iOverlay);
void TreeList_SetIcon(HTREELISTITEM hItem, int iIcon, int iOverlay);
LPARAM TreeList_GetData(HTREELISTITEM hItem);
HTREELISTITEM TreeList_GetRoot(HWND hwnd);
int TreeList_GetChildrenCount(HTREELISTITEM hItem);
HTREELISTITEM TreeList_GetChild(HTREELISTITEM hItem, int i);
void sttTreeList_RecursiveApply(HTREELISTITEM hItem, void (*func)(HTREELISTITEM, LPARAM), LPARAM data);
void TreeList_Update(HWND hwnd);
BOOL TreeList_ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT idc, BOOL *result);

//---- jabber_form.c ------------------------------------------------

enum TJabberFormControlType
{
	JFORM_CTYPE_NONE, JFORM_CTYPE_TEXT_PRIVATE, JFORM_CTYPE_TEXT_MULTI,
	JFORM_CTYPE_BOOLEAN, JFORM_CTYPE_LIST_SINGLE, JFORM_CTYPE_LIST_MULTI,
	JFORM_CTYPE_FIXED, JFORM_CTYPE_HIDDEN, JFORM_CTYPE_TEXT_SINGLE
};

typedef struct TJabberFormControlInfo *HJFORMCTRL;
typedef struct TJabberFormLayoutInfo *HJFORMLAYOUT;

void JabberFormCreateUI( HWND hwndStatic, XmlNode *xNode, int *formHeight, BOOL bCompact = FALSE );
void JabberFormDestroyUI(HWND hwndStatic);
void JabberFormSetInstruction( HWND hwndForm, TCHAR *text );
HJFORMLAYOUT JabberFormCreateLayout(HWND hwndStatic); // use mir_free to destroy
HJFORMCTRL JabberFormAppendControl(HWND hwndStatic, HJFORMLAYOUT layout_info, TJabberFormControlType type, TCHAR *labelStr, TCHAR *valueStr);
void JabberFormAddListItem(HJFORMCTRL item, TCHAR *text, bool selected);
void JabberFormLayoutControls(HWND hwndStatic, HJFORMLAYOUT layout_info, int *formHeight);

void JabberFormCreateDialog( XmlNode *xNode, TCHAR* defTitle, JABBER_FORM_SUBMIT_FUNC pfnSubmit, void *userdata );

XmlNode* JabberFormGetData( HWND hwndStatic, XmlNode *xNode );

//---- jabber_ft.c --------------------------------------------------

void JabberFtCancel( filetransfer* ft );
void JabberFtInitiate( TCHAR* jid, filetransfer* ft );
void JabberFtHandleSiRequest( XmlNode *iqNode );
void JabberFtAcceptSiRequest( filetransfer* ft );
void JabberFtAcceptIbbRequest( filetransfer* ft );
void JabberFtHandleBytestreamRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
BOOL JabberFtHandleIbbRequest( XmlNode *iqNode, BOOL bOpen );

//---- jabber_groupchat.c -------------------------------------------

int JabberMenuHandleGroupchat( WPARAM wParam, LPARAM lParam );
void JabberGroupchatJoinRoom( const TCHAR* server, const TCHAR* room, const TCHAR* nick, const TCHAR* password );
void JabberGroupchatProcessPresence( XmlNode *node, void *userdata );
void JabberGroupchatProcessMessage( XmlNode *node, void *userdata );
void JabberGroupchatProcessInvite( TCHAR* roomJid, TCHAR* from, TCHAR* reason, TCHAR* password );


//---- jabber_bookmarks.c -------------------------------------------
int JabberMenuHandleBookmarks( WPARAM wParam, LPARAM lParam );
int JabberAddEditBookmark( WPARAM wParam, LPARAM lParam );



//---- jabber_icolib.c ----------------------------------------------

void   JabberCheckAllContactsAreTransported( void );
BOOL   JabberDBCheckIsTransportedContact(const TCHAR* jid, HANDLE hContact);
int    ReloadIconsEventHook(WPARAM wParam, LPARAM lParam);
int    JGetAdvancedStatusIcon(WPARAM wParam, LPARAM lParam);
void   JabberIconsInit( void );
HICON  LoadIconEx( const char* name );
HANDLE __stdcall GetIconHandle( int iconId );

//---- jabber_libstr.c ----------------------------------------------

void  __stdcall replaceStr( char*& dest, const char* src );
void  __stdcall replaceStr( WCHAR*& dest, const WCHAR* src );
char* __stdcall rtrim( char *string );
#if defined( _UNICODE )
	TCHAR* __stdcall rtrim( TCHAR *string );
#endif

//---- jabber_misc.c ------------------------------------------------

void   JabberAddContactToRoster( const TCHAR* jid, const TCHAR* nick, const TCHAR* grpName, JABBER_SUBSCRIPTION subscription );
void   JabberChatDllError( void );
int    JabberCompareJids( const TCHAR* jid1, const TCHAR* jid2 );
void   JabberContactListCreateGroup( TCHAR* groupName );
void   JabberDBAddAuthRequest( TCHAR* jid, TCHAR* nick );
HANDLE JabberDBCreateContact( TCHAR* jid, TCHAR* nick, BOOL temporary, BOOL stripResource );
void   JabberGetAvatarFileName( HANDLE hContact, char* pszDest, int cbLen );
void   JabberResolveTransportNicks( TCHAR* jid );
void   JabberSetServerStatus( int iNewStatus );
TCHAR* EscapeChatTags(TCHAR* pszText);
char*  UnEscapeChatTags(char* str_in);
void   JabberUpdateMirVer(JABBER_LIST_ITEM *item);
void   JabberUpdateMirVer(HANDLE hContact, JABBER_RESOURCE_STATUS *resource);
int    JabberGetEventTextChatStates( WPARAM wParam, LPARAM lParam );

//---- jabber_adhoc.cpp	---------------------------------------------
int JabberContactMenuRunCommands(WPARAM wParam, LPARAM lParam);

//---- jabber_svc.c -------------------------------------------------

void JabberEnableMenuItems( BOOL bEnable );

//---- jabber_search.cpp -------------------------------------------------
int JabberSearchCreateAdvUI( WPARAM wParam, LPARAM lParam);
int JabberSearchByAdvanced( WPARAM wParam, LPARAM lParam );


//---- jabber_std.cpp ----------------------------------------------

#if defined( _DEBUG )
	#define JCallService CallService
#else
	int __stdcall  JCallService( const char* szSvcName, WPARAM wParam, LPARAM lParam );
#endif

HANDLE __stdcall  JCreateServiceFunction( const char* szService, MIRANDASERVICE serviceProc );
HANDLE __stdcall  JCreateHookableEvent( const char* szService );
void   __stdcall  JDeleteSetting( HANDLE hContact, const char* valueName );
DWORD  __stdcall  JGetByte( const char* valueName, int parDefltValue );
DWORD  __stdcall  JGetByte( HANDLE hContact, const char* valueName, int parDefltValue );
char*  __stdcall  JGetContactName( HANDLE hContact );
DWORD  __stdcall  JGetDword( HANDLE hContact, const char* valueName, DWORD parDefltValue );
int    __stdcall  JGetStaticString( const char* valueName, HANDLE hContact, char* dest, int dest_len );
int    __stdcall  JGetStringUtf( HANDLE hContact, char* valueName, DBVARIANT* dbv );
int    __stdcall  JGetStringT( HANDLE hContact, char* valueName, DBVARIANT* dbv );
WORD   __stdcall  JGetWord( HANDLE hContact, const char* valueName, int parDefltValue );
void   __fastcall JFreeVariant( DBVARIANT* dbv );
int    __stdcall  JSendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam );
DWORD  __stdcall  JSetByte( const char* valueName, int parValue );
DWORD  __stdcall  JSetByte( HANDLE hContact, const char* valueName, int parValue );
DWORD  __stdcall  JSetDword( HANDLE hContact, const char* valueName, DWORD parValue );
DWORD  __stdcall  JSetString( HANDLE hContact, const char* valueName, const char* parValue );
DWORD  __stdcall  JSetStringT( HANDLE hContact, const char* valueName, const TCHAR* parValue );
DWORD  __stdcall  JSetStringUtf( HANDLE hContact, const char* valueName, const char* parValue );
DWORD  __stdcall  JSetWord( HANDLE hContact, const char* valueName, int parValue );
char*  __stdcall  JTranslate( const char* str );

//---- jabber_thread.cpp -------------------------------------------

void __cdecl JabberServerThread( ThreadData* info );

//---- jabber_util.c ----------------------------------------------

struct TStringPairsElem
{
	const char *name, *value;
};

struct TStringPairs
{
	TStringPairs( char* );
	~TStringPairs();

	const char* operator[]( const char* name ) const;

	int numElems;
	TStringPairsElem* elems;
};

void          __stdcall JabberSerialInit( void );
void          __stdcall JabberSerialUninit( void );
unsigned int  __stdcall JabberSerialNext( void );
HANDLE        __stdcall JabberHContactFromJID( const TCHAR* jid , BOOL bStripResource = 3);
HANDLE        __stdcall JabberChatRoomHContactFromJID( const TCHAR* jid );
void          __stdcall JabberLog( const char* fmt, ... );
TCHAR*        __stdcall JabberNickFromJID( const TCHAR* jid );
JABBER_RESOURCE_STATUS* __stdcall JabberResourceInfoFromJID( TCHAR* jid );
char*         __stdcall JabberUrlDecode( char* str );
void          __stdcall JabberUrlDecodeW( WCHAR* str );
char*         __stdcall JabberUrlEncode( const char* str );
char*         __stdcall JabberSha1( char* str );
char*         __stdcall JabberUnixToDos( const char* str );
WCHAR*        __stdcall JabberUnixToDosW( const WCHAR* str );
void          __stdcall JabberHttpUrlDecode( char* str );
char*         __stdcall JabberHttpUrlEncode( const char* str );
int           __stdcall JabberCombineStatus( int status1, int status2 );
TCHAR*        __stdcall JabberErrorStr( int errorCode );
TCHAR*        __stdcall JabberErrorMsg( XmlNode *errorNode );
void          __stdcall JabberSendVisibleInvisiblePresence( BOOL invisible );
char*         __stdcall JabberTextEncode( const char* str );
char*         __stdcall JabberTextEncodeW( const wchar_t *str );
char*         __stdcall JabberTextDecode( const char* str );
void          __stdcall JabberUtfToTchar( const char* str, size_t cbLen, LPTSTR& dest );
char*         __stdcall JabberBase64Encode( const char* buffer, int bufferLen );
char*         __stdcall JabberBase64Decode( const TCHAR* buffer, int *resultLen );
char*         __stdcall JabberGetVersionText();
time_t        __stdcall JabberIsoToUnixTime( TCHAR* stamp );
int           __stdcall JabberCountryNameToId( TCHAR* ctry );
void          __stdcall JabberSendPresenceTo( int status, TCHAR* to, XmlNode* extra );
void          __stdcall JabberSendPresence( int iStatus, bool bSendToAll );
void          __stdcall JabberStringAppend( char* *str, int *sizeAlloced, const char* fmt, ... );
TCHAR*        __stdcall JabberGetClientJID( const TCHAR* jid, TCHAR*, size_t );
TCHAR*        __stdcall JabberStripJid( const TCHAR* jid, TCHAR* dest, size_t destLen );
int           __stdcall JabberGetPictureType( const char* buf );
int           __stdcall JabberGetPacketID( XmlNode* n );

#if defined( _UNICODE )
	#define JabberUnixToDosT JabberUnixToDosW
#else
	#define JabberUnixToDosT JabberUnixToDos
#endif

//---- jabber_vcard.c -----------------------------------------------

int JabberSendGetVcard( const TCHAR* jid );

//---- jabber_ws.c -------------------------------------------------

BOOL          JabberWsInit( void );
void          JabberWsUninit( void );
JABBER_SOCKET JabberWsConnect( char* host, WORD port );
int           JabberWsSend( JABBER_SOCKET s, char* data, int datalen, int flags );
int           JabberWsRecv( JABBER_SOCKET s, char* data, long datalen, int flags );

//---- jabber_xml.c ------------------------------------------------

char* skipSpaces( char* p, int* num = NULL );

//---- jabber_xstatus.c --------------------------------------------

void JabberXStatusInit( void );
void JabberXStatusUninit( void );

void JabberSetContactMood( HANDLE hContact, const char* moodName, const TCHAR* moodText );

///////////////////////////////////////////////////////////////////////////////
// UTF encode helper

char*    t2a( const TCHAR* src );
char*    u2a( const wchar_t* src );
wchar_t* a2u( const char* src );
TCHAR*   a2t( const char* src );

#endif
