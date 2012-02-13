/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-11  George Hazan
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

Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#ifndef _JABBER_H_
#define _JABBER_H_

#ifdef _MSC_VER
	#pragma warning(disable:4706 4121 4127)
#endif

#define MIRANDA_VER 0x0A00

#include "m_stdhdr.h"

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)
#define NEWTSTR_ALLOCA(A) (A==NULL)?NULL:_tcscpy((TCHAR*)alloca(sizeof(TCHAR)*(_tcslen(A)+1)),A)

#define LISTFOREACH(var__, obj__, list__)	\
	for (int var__ = 0; (var__ = obj__->ListFindNext(list__, var__)) >= 0; ++var__)
#define LISTFOREACH_NODEF(var__, obj__, list__)	\
	for (var__ = 0; (var__ = obj__->ListFindNext(list__, var__)) >= 0; ++var__)

/*******************************************************************
 * Global header files
 *******************************************************************/
#define _WIN32_WINNT 0x501
#define _WIN32_IE 0x501
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <process.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_system_cpp.h>
#include <m_netlib.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_protoint.h>
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
#include <m_clistint.h>
#include <m_button.h>
#include <m_avatars.h>
#include <m_idle.h>
#include <m_timezones.h>
#include <m_jabber.h>
#include <win2k.h>

#include "../../plugins/zlib/zlib.h"

#include "resource.h"
#include "version.h"

#include "MString.h"

#include "jabber_xml.h"
#include "jabber_byte.h"
#include "jabber_ibb.h"
#include "jabber_db_utils.h"
#include "ui_utils.h"

struct CJabberProto;

class CJabberDlgBase: public CProtoDlgBase<CJabberProto>
{
	typedef CProtoDlgBase<CJabberProto> CSuper;
protected:
	__inline CJabberDlgBase(CJabberProto *proto, int idDialog, HWND parent, bool show_label=true ) :
		CSuper( proto, idDialog, parent, show_label )
	{
	}

	int Resizer(UTILRESIZECONTROL *urc)
	{
		switch (urc->wId) {
		case IDC_HEADERBAR:
			urc->rcItem.right = urc->dlgNewSize.cx;
			return 0;
		}

		return CSuper::Resizer(urc);
	}
};

#if !defined(OPENFILENAME_SIZE_VERSION_400)
	#define OPENFILENAME_SIZE_VERSION_400 sizeof(OPENFILENAME)
#endif

/*******************************************************************
 * Global constants
 *******************************************************************/

#define GLOBAL_SETTING_PREFIX	"JABBER"
#define GLOBAL_SETTING_MODULE	"JABBER"

#define JABBER_DEFAULT_PORT 5222
#define JABBER_IQID "mir_"
#define JABBER_MAX_JID_LEN  1024

#define JABBER_GC_MSG_QUIT				LPGENT("I'm happy Miranda IM user. Get it at http://miranda-im.org/.")
#define JABBER_GC_MSG_SLAP				LPGENT("/me slaps %s around a bit with a large trout")

// registered db event types
#define JABBER_DB_EVENT_TYPE_CHATSTATES          2000
#define JS_DB_GETEVENTTEXT_CHATSTATES            "/GetEventText2000"
#define JABBER_DB_EVENT_CHATSTATES_GONE          1
#define JABBER_DB_EVENT_TYPE_PRESENCE            2001
#define JS_DB_GETEVENTTEXT_PRESENCE              "/GetEventText2001"
#define JABBER_DB_EVENT_PRESENCE_SUBSCRIBE       1
#define JABBER_DB_EVENT_PRESENCE_SUBSCRIBED      2
#define JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBE     3
#define JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBED    4
#define JABBER_DB_EVENT_PRESENCE_ERROR           5

// User-defined message
#define WM_JABBER_REGDLG_UPDATE        (WM_PROTO_LAST + 100)
#define WM_JABBER_AGENT_REFRESH        (WM_PROTO_LAST + 101)
#define WM_JABBER_TRANSPORT_REFRESH    (WM_PROTO_LAST + 102)
#define WM_JABBER_REGINPUT_ACTIVATE    (WM_PROTO_LAST + 103)
#define WM_JABBER_REFRESH              WM_PROTO_REFRESH
#define WM_JABBER_CHECK_ONLINE         WM_PROTO_CHECK_ONLINE
#define WM_JABBER_ACTIVATE             WM_PROTO_ACTIVATE
#define WM_JABBER_CHANGED              (WM_PROTO_LAST + 106)
#define WM_JABBER_SET_FONT             (WM_PROTO_LAST + 108)
#define WM_JABBER_FLASHWND             (WM_PROTO_LAST + 109)
#define WM_JABBER_GC_MEMBER_ADD        (WM_PROTO_LAST + 110)
#define WM_JABBER_GC_FORCE_QUIT        (WM_PROTO_LAST + 111)
#define WM_JABBER_SHUTDOWN             (WM_PROTO_LAST + 112)
#define WM_JABBER_SMILEY               (WM_PROTO_LAST + 113)
#define WM_JABBER_JOIN                 (WM_PROTO_LAST + 114)
#define WM_JABBER_ADD_TO_ROSTER        (WM_PROTO_LAST + 115)
#define WM_JABBER_ADD_TO_BOOKMARKS     (WM_PROTO_LAST + 116)
#define WM_JABBER_REFRESH_VCARD        (WM_PROTO_LAST + 117)


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

// Icon list
enum {
	JABBER_IDI_GCOWNER = 0,
	JABBER_IDI_GCADMIN,
	JABBER_IDI_GCMODERATOR,
	JABBER_IDI_GCVOICE,
	JABBER_ICON_TOTAL
};

// Services and Events
#define JS_PARSE_XMPP_URI          "/ParseXmppURI"

// XEP-0224 support (Attention/Nudge)
#define JS_SEND_NUDGE              "/SendNudge"
#define JE_NUDGE                   "/Nudge"

// Called when contact changes custom status and extra icon is set to clist_mw
//wParam = hContact    // contact changing status
//lParam = hIcon       // HANDLE to clist extra icon set as custom status
#define JE_CUSTOMSTATUS_EXTRAICON_CHANGED "/XStatusExtraIconChanged"
#define JE_CUSTOMSTATUS_CHANGED						"/XStatusChanged"

#define LR_BIGICON                 0x40

#define JS_SENDXML                 "/SendXML" // Warning: This service is obsolete. Use IJabberNetInterface::SendXmlNode() instead.
#define JS_GETADVANCEDSTATUSICON   "/GetAdvancedStatusIcon"
#define JS_GETCUSTOMSTATUSICON     "/GetXStatusIcon"
#define JS_GETXSTATUS              "/GetXStatus"
#define JS_SETXSTATUS              "/SetXStatus"

#define JS_HTTP_AUTH               "/HttpAuthRequest"
#define JS_INCOMING_NOTE_EVENT     "/IncomingNoteEvent"

#define DBSETTING_DISPLAY_UID      "display_uid"
#define DBSETTING_XSTATUSID        "XStatusId"
#define DBSETTING_XSTATUSNAME      "XStatusName"
#define DBSETTING_XSTATUSMSG       "XStatusMsg"

#define ADVSTATUS_MOOD             "mood"
#define ADVSTATUS_ACTIVITY         "activity"
#define ADVSTATUS_TUNE             "tune"

#define ADVSTATUS_VAL_ID           "id"
#define ADVSTATUS_VAL_ICON         "icon"
#define ADVSTATUS_VAL_TITLE        "title"
#define ADVSTATUS_VAL_TEXT         "text"

struct CJabberHttpAuthParams
{
	enum {IQ = 1, MSG = 2} m_nType;
	TCHAR *m_szFrom;
	TCHAR *m_szIqId;
	TCHAR *m_szThreadId;
	TCHAR *m_szId;
	TCHAR *m_szMethod;
	TCHAR *m_szUrl;
	CJabberHttpAuthParams()
	{
		ZeroMemory(this, sizeof(CJabberHttpAuthParams));
	}
	~CJabberHttpAuthParams()
	{
		Free();
	}
	void Free()
	{
		mir_free(m_szFrom);
		mir_free(m_szIqId);
		mir_free(m_szThreadId);
		mir_free(m_szId);
		mir_free(m_szMethod);
		mir_free(m_szUrl);
		ZeroMemory(this, sizeof(CJabberHttpAuthParams));
	}
};

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

#include "jabber_caps.h"

#define JABBER_LOGIN_ROSTER				0x0001
#define JABBER_LOGIN_BOOKMARKS			0x0002
#define JABBER_LOGIN_SERVERINFO			0x0004
#define JABBER_LOGIN_BOOKMARKS_AJ		0x0008

struct ThreadData
{
	ThreadData( CJabberProto* _ppro, JABBER_SESSION_TYPE parType );
	~ThreadData();

	HANDLE hThread;
	JABBER_SESSION_TYPE type;
	
	// network support
	JABBER_SOCKET s;
	BOOL  useSSL;
	HANDLE iomutex; // protects i/o operations
	CJabberProto* proto;

	// XEP-0138 (Compression support)
	BOOL     useZlib;
	z_stream zStreamIn,zStreamOut;
	bool     zRecvReady;
	int      zRecvDatalen;
	char*    zRecvData;

	void     xmpp_client_query( void );

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
	JabberCapsBits jabberServerCaps;
	BOOL bBookmarksLoaded;
	DWORD	dwLoginRqs;

	// connection & login data
	TCHAR username[512];
	TCHAR password[512];
	char  server[128];
	char  manualHost[128];
	TCHAR resource[128];
	TCHAR fullJID[JABBER_MAX_JID_LEN];
	WORD  port;
	TCHAR newPassword[512];

	void  close( void );
	void  shutdown( void );
	int   recv( char* buf, size_t len );
	int   send( char* buffer, int bufsize );
	int   send( const char* fmt, ... );
	int   send( HXML node );

	int   recvws( char* buffer, size_t bufsize, int flags );
	int   sendws( char* buffer, size_t bufsize, int flags );
};

struct JABBER_MODEMSGS
{
	TCHAR* szOnline;
	TCHAR* szAway;
	TCHAR* szNa;
	TCHAR* szDnd;
	TCHAR* szFreechat;
};

struct JABBER_REG_ACCOUNT
{
	TCHAR username[512];
	TCHAR password[512];
	char server[128];
	char manualHost[128];
	WORD port;
	BOOL useSSL;
};

typedef enum { FT_SI, FT_OOB, FT_BYTESTREAM, FT_IBB } JABBER_FT_TYPE;
typedef enum { FT_CONNECTING, FT_INITIALIZING, FT_RECEIVING, FT_DONE, FT_ERROR, FT_DENIED } JABBER_FILE_STATE;

struct filetransfer
{
	filetransfer( CJabberProto* proto );
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

	// For type == FT_BYTESTREAM
	JABBER_BYTE_TRANSFER *jbt;

	JABBER_IBB_TRANSFER *jibb;

	// Used by file receiving only
	char* httpHostName;
	WORD httpPort;
	TCHAR* httpPath;
	unsigned __int64 dwExpectedRecvFileSize;

	// Used by file sending only
	HANDLE hFileEvent;
	unsigned __int64 *fileSize;
	TCHAR* szDescription;
	
	CJabberProto* ppro;
};

struct JABBER_SEARCH_RESULT
{
	PROTOSEARCHRESULT hdr;
	TCHAR jid[JABBER_MAX_JID_LEN];
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
	~JABBER_MUC_JIDLIST_INFO();

	JABBER_MUC_JIDLIST_TYPE type;
	TCHAR* roomJid;	// filled-in by the WM_JABBER_REFRESH code
	HXML   iqNode;
	CJabberProto* ppro;

	TCHAR* type2str( void ) const;
};

typedef void ( CJabberProto::*JABBER_FORM_SUBMIT_FUNC )( HXML values, void *userdata );

class _A2T
{
	TCHAR* buf;

public:
	_A2T( const char* s ) : buf( mir_a2t( s )) {}
	_A2T( const char* s, int cp ) : buf( mir_a2t_cp( s, cp )) {}
	~_A2T() { mir_free(buf); }

	__forceinline operator TCHAR*() const
	{	return buf;
	}
};

//---- jabber_treelist.c ------------------------------------------------

typedef struct TTreeList_ItemInfo *HTREELISTITEM;
enum { TLM_TREE, TLM_REPORT };

//---- proto frame ------------------------------------------------

class CJabberInfoFrameItem;

struct CJabberInfoFrame_Event
{
	enum { CLICK, DESTROY } m_event;
	const char *m_pszName;
	LPARAM m_pUserData;
};

class CJabberInfoFrame
{
public:
	CJabberInfoFrame(CJabberProto *proto);
	~CJabberInfoFrame();

	void CreateInfoItem(char *pszName, bool bCompact=false, LPARAM pUserData=0);
	void SetInfoItemCallback(char *pszName, void (CJabberProto::*onEvent)(CJabberInfoFrame_Event *));
	void UpdateInfoItem(char *pszName, HANDLE hIcolibItem, TCHAR *pszText);
	void ShowInfoItem(char *pszName, bool bShow);
	void RemoveInfoItem(char *pszName);

	void LockUpdates();
	void Update();

private:
	CJabberProto *m_proto;
	HWND m_hwnd;
	int m_frameId;
	bool m_compact;
	OBJLIST<CJabberInfoFrameItem> m_pItems;
	int m_hiddenItemCount;
	int m_clickedItem;
	bool m_bLocked;
	int m_nextTooltipId;
	HWND m_hwndToolTip;

	HANDLE m_hhkFontsChanged;
	HFONT m_hfntTitle, m_hfntText;
	COLORREF m_clTitle, m_clText, m_clBack;

	static void InitClass();
	static LRESULT CALLBACK GlobalWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

	void ReloadFonts();
	void UpdateSize();

	void RemoveTooltip(int id);
	void SetToolTip(int id, RECT *rc, TCHAR *pszText);

	void PaintSkinGlyph(HDC hdc, RECT *rc, char **glyphs, COLORREF fallback);
	void PaintCompact(HDC hdc);
	void PaintNormal(HDC hdc);

	enum
	{
		SZ_FRAMEPADDING = 2,	// padding inside frame
		SZ_LINEPADDING = 0,		// line height will be incremented by this value
		SZ_LINESPACING = 0,		// between lines
		SZ_ICONSPACING = 2,		// between icon and text
	};
};

#include "jabber_list.h"
#include "jabber_proto.h"

/*******************************************************************
 * Global variables
 *******************************************************************/
extern HINSTANCE hInst;
extern BOOL   jabberChatDllPresent;

extern HANDLE hExtraMood;
extern HANDLE hExtraActivity;

// Theme API
extern BOOL (WINAPI *JabberAlphaBlend)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
extern BOOL (WINAPI *JabberIsThemeActive)();
extern HRESULT (WINAPI *JabberDrawThemeParentBackground)(HWND, HDC, RECT *);

extern const TCHAR xmlnsOwner[];

extern int g_cbCountries;
extern struct CountryListEntry* g_countries;

/*******************************************************************
 * Function declarations
 *******************************************************************/

//---- jabber_treelist.c ------------------------------------------------

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

void JabberFormCreateUI( HWND hwndStatic, HXML xNode, int *formHeight, BOOL bCompact = FALSE );
void JabberFormDestroyUI(HWND hwndStatic);
void JabberFormSetInstruction( HWND hwndForm, const TCHAR *text );
HJFORMLAYOUT JabberFormCreateLayout(HWND hwndStatic); // use mir_free to destroy
HJFORMCTRL JabberFormAppendControl(HWND hwndStatic, HJFORMLAYOUT layout_info, TJabberFormControlType type, const TCHAR *labelStr, const TCHAR *valueStr);
void JabberFormAddListItem(HJFORMCTRL item, const TCHAR *text, bool selected);
void JabberFormLayoutControls(HWND hwndStatic, HJFORMLAYOUT layout_info, int *formHeight);

void JabberFormCreateDialog( HXML xNode, TCHAR* defTitle, JABBER_FORM_SUBMIT_FUNC pfnSubmit, void *userdata );

HXML JabberFormGetData( HWND hwndStatic, HXML xNode );

//---- jabber_icolib.c ----------------------------------------------

void   g_IconsInit();
HANDLE g_GetIconHandle( int iconId );
HICON  g_LoadIconEx( const char* name, bool big = false );
void g_ReleaseIcon( HICON hIcon );

void ImageList_AddIcon_Icolib( HIMAGELIST hIml, HICON hIcon );
void WindowSetIcon(HWND hWnd, CJabberProto *proto, const char* name);
void WindowFreeIcon(HWND hWnd);

int    ReloadIconsEventHook(WPARAM wParam, LPARAM lParam);

//---- jabber_libstr.c ----------------------------------------------

void  __stdcall replaceStr( char*& dest, const char* src );
void  __stdcall replaceStr( WCHAR*& dest, const WCHAR* src );
int lstrcmp_null(const TCHAR *s1, const TCHAR *s2);
char* __stdcall rtrim( char *string );
#if defined( _UNICODE )
	TCHAR* __stdcall rtrim( TCHAR *string );
#endif

//---- jabber_menu.c ------------------------------------------------

void   g_MenuInit();
void   g_MenuUninit();
int    g_OnModernToolbarInit(WPARAM, LPARAM);

//---- jabber_misc.c ------------------------------------------------

void   JabberChatDllError( void );
int    JabberCompareJids( const TCHAR* jid1, const TCHAR* jid2 );
void   JabberContactListCreateGroup( TCHAR* groupName );
TCHAR* EscapeChatTags(TCHAR* pszText);
TCHAR* UnEscapeChatTags(TCHAR* str_in);

//---- jabber_adhoc.cpp	---------------------------------------------

struct CJabberAdhocStartupParams
{
	TCHAR* m_szJid;
	TCHAR* m_szNode;
	CJabberProto* m_pProto;

	CJabberAdhocStartupParams( CJabberProto* proto, TCHAR* szJid, TCHAR* szNode = NULL )
	{
		m_pProto = proto;
		m_szJid = mir_tstrdup( szJid );
		m_szNode = szNode ? mir_tstrdup( szNode ) : NULL;
	}
	~CJabberAdhocStartupParams()
	{
		if ( m_szJid )
			mir_free( m_szJid );
		if ( m_szNode )
			mir_free( m_szNode );
	}
};

struct JabberAdHocData
{
	CJabberProto* proto;
	int    CurrentHeight;
	int    curPos;
	int    frameHeight;
	RECT   frameRect;
	HXML   AdHocNode;
	HXML   CommandsNode;
	TCHAR* ResponderJID;
};

//---- jabber_std.cpp -------------------------------------------------------------------

void  __fastcall JFreeVariant( DBVARIANT* dbv );
char* __fastcall JTranslate( const char* str );

#if defined( _DEBUG )
	#define JCallService CallService
#else
	INT_PTR __stdcall  JCallService( const char* szSvcName, WPARAM wParam, LPARAM lParam );
#endif

//---- jabber_util.cpp ------------------------------------------------------------------

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

TCHAR*        __stdcall JabberNickFromJID( const TCHAR* jid );
TCHAR*                  JabberPrepareJid( LPCTSTR jid );
char*         __stdcall JabberUrlDecode( char* str );
void          __stdcall JabberUrlDecodeW( WCHAR* str );
char*         __stdcall JabberUrlEncode( const char* str );
char*         __stdcall JabberSha1( char* str );
TCHAR*        __stdcall JabberStrFixLines( const TCHAR* str );
char*         __stdcall JabberUnixToDos( const char* str );
WCHAR*        __stdcall JabberUnixToDosW( const WCHAR* str );
void          __stdcall JabberHttpUrlDecode( TCHAR* str );
TCHAR*        __stdcall JabberHttpUrlEncode( const TCHAR* str );
int           __stdcall JabberCombineStatus( int status1, int status2 );
TCHAR*        __stdcall JabberErrorStr( int errorCode );
TCHAR*        __stdcall JabberErrorMsg( HXML errorNode, int* errorCode = NULL );
void          __stdcall JabberUtfToTchar( const char* str, size_t cbLen, LPTSTR& dest );
char*         __stdcall JabberBase64Encode( const char* buffer, int bufferLen );
char*         __stdcall JabberBase64Decode( const char* buffer, int *resultLen );
char*         __stdcall JabberBase64DecodeW( const WCHAR* buffer, int *resultLen );
time_t        __stdcall JabberIsoToUnixTime( const TCHAR* stamp );
void          __stdcall JabberStringAppend( char* *str, int *sizeAlloced, const char* fmt, ... );
TCHAR*        __stdcall JabberStripJid( const TCHAR* jid, TCHAR* dest, size_t destLen );
int           __stdcall JabberGetPictureType( const char* buf );
int           __stdcall JabberGetPacketID( HXML n );

#if defined( _UNICODE )
	#define JabberUnixToDosT JabberUnixToDosW
	#define JabberBase64DecodeT JabberBase64DecodeW
#else
	#define JabberUnixToDosT JabberUnixToDos
	#define JabberBase64DecodeT JabberBase64Decode
#endif

const TCHAR *JabberStrIStr( const TCHAR *str, const TCHAR *substr);
void JabberCopyText(HWND hwnd, TCHAR *text);
void JabberBitmapPremultiplyChannels(HBITMAP hBitmap);
CJabberProto *JabberChooseInstance(bool bIsLink=false);

//---- jabber_xml.cpp -------------------------------------------------------------------

void  strdel( char* parBuffer, int len );

//---- jabber_userinfo.cpp --------------------------------------------------------------

void JabberUserInfoUpdate( HANDLE hContact );

//---- jabber_iq_handlers.cpp
BOOL GetOSDisplayString(LPTSTR pszOS, int BUFSIZE);

#endif
