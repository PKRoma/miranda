/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-7 Boris Krasnovskiy.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

// this plugin is for Miranda 0.6 only
#define MIRANDA_VER 0x0700

#if defined(UNICODE) && !defined(_UNICODE)
	#define _UNICODE
#endif

#if _MSC_VER < 1400
	#include <malloc.h>
#endif

#if defined(_DEBUG) && !defined(__GNUC__)
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#if _MSC_VER >= 1400
	#include <malloc.h>
#endif

#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <commctrl.h>

#include <ctype.h>
#include <process.h>
#include <stdio.h>
#include <time.h>

#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <newpluginapi.h>

#include <m_clc.h>
#include <m_clist.h>
#include <m_clui.h>
#include <m_idle.h>
#include <m_icolib.h>
#include <m_message.h>
#include <m_options.h>
#include <m_protocols.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_skin.h>
#include <m_system.h>
#include <m_system_cpp.h>
#include <m_userinfo.h>
#include <m_utils.h>
#include <win2k.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_netlib.h>
#include <m_popup.h>
#include <m_chat.h>
#include <m_avatars.h>

#include "sdk/m_proto_listeningto.h"
#include "sdk/m_folders.h"
#include "sdk/m_metacontacts.h"

#include "ezxml.h"

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN error codes

#define ERR_SYNTAX_ERROR                 200
#define ERR_INVALID_PARAMETER            201
#define ERR_INVALID_USER                 205
#define ERR_FQDN_MISSING                 206
#define ERR_ALREADY_LOGIN                207
#define ERR_INVALID_USERNAME             208
#define ERR_INVALID_FRIENDLY_NAME        209
#define ERR_LIST_FULL                    210
#define ERR_ALREADY_THERE                215
#define ERR_NOT_ON_LIST                  216
#define ERR_NOT_ONLINE                   217
#define ERR_ALREADY_IN_THE_MODE          218
#define ERR_ALREADY_IN_OPPOSITE_LIST     219
#define ERR_CONTACT_LIST_FAILED          241
#define ERR_SWITCHBOARD_FAILED           280
#define ERR_NOTIFY_XFR_FAILED            281
#define ERR_REQUIRED_FIELDS_MISSING      300
#define ERR_NOT_LOGGED_IN                302
#define ERR_INTERNAL_SERVER              500
#define ERR_DB_SERVER                    501
#define ERR_FILE_OPERATION               510
#define ERR_MEMORY_ALLOC                 520
#define ERR_SERVER_BUSY                  600
#define ERR_SERVER_UNAVAILABLE           601
#define ERR_PEER_NS_DOWN                 602
#define ERR_DB_CONNECT                   603
#define ERR_SERVER_GOING_DOWN            604
#define ERR_CREATE_CONNECTION            707
#define ERR_INVALID_LOCALE               710
#define ERR_BLOCKING_WRITE               711
#define ERR_SESSION_OVERLOAD             712
#define ERR_USER_TOO_ACTIVE              713
#define ERR_TOO_MANY_SESSIONS            714
#define ERR_NOT_EXPECTED                 715
#define ERR_BAD_FRIEND_FILE              717
#define ERR_AUTHENTICATION_FAILED        911
#define ERR_NOT_ALLOWED_WHEN_OFFLINE     913
#define ERR_NOT_ACCEPTING_NEW_USERS      920
#define ERR_EMAIL_ADDRESS_NOT_VERIFIED   924

/////////////////////////////////////////////////////////////////////////////////////////
//	Global definitions

#define MSN_MAX_EMAIL_LEN        128
#define MSN_GUID_LEN			  40

#define MSN_DEFAULT_PORT         1863
#define MSN_DEFAULT_GATEWAY_PORT   80
const char MSN_DEFAULT_LOGIN_SERVER[] = "messenger.hotmail.com";
const char MSN_DEFAULT_GATEWAY[] =      "gateway.messenger.hotmail.com";
const char MSN_USER_AGENT[] =           "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)";

#define MSN_BLOCK        "/BlockCommand"
#define MSN_INVITE       "/InviteCommand"
#define MSN_NETMEETING   "/NetMeeting"
#define MSN_VIEW_PROFILE "/ViewProfile"
#define MSN_SEND_NUDGE	 "/SendNudge"

#define MS_GOTO_INBOX		"/GotoInbox"
#define MS_EDIT_PROFILE		"/EditProfile"
#define MS_VIEW_STATUS		"/ViewMsnStatus"
#define MS_SET_NICKNAME_UI  "/SetNicknameUI"

#define MSN_SET_NICKNAME            "/SetNickname"

#define MSN_GETUNREAD_EMAILCOUNT	"/GetUnreadEmailCount"
/////////////////////////////////////////////////////////////////////////////////////////
//	MSN plugin functions

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)
#define NEWTSTR_ALLOCA(A) (A==NULL)?NULL:_tcscpy((TCHAR*)alloca(sizeof(TCHAR)*(_tcslen(A)+1)),A)

#define	MSN_ALLOW_MSGBOX    1
#define	MSN_ALLOW_ENTER	    2
#define	MSN_HOTMAIL_POPUP   4
#define MSN_SHOW_ERROR      8
#define	MSN_ALERT_POPUP	    16
void  MSN_ShowPopup( const TCHAR* nickname, const TCHAR* msg, int flags, const char* url );
void  MSN_ShowPopup( const HANDLE hContact, const TCHAR* msg, int flags );

LONG		MSN_SendPacket( HANDLE, const char* cmd, const char* params, ... );
char*		MirandaStatusToMSN( int status );
WORD		MSNStatusToMiranda( const char* status );
void        HtmlDecode( char* str );
char*       HtmlEncode( const char* str );
WCHAR*      HtmlEncodeW( const WCHAR* str );
#if defined( _UNICODE )
	#define  HtmlEncodeT HtmlEncodeW
#else
	#define  HtmlEncodeT HtmlEncode
#endif
bool		txtParseParam (const char* szData, const char* presearch, const char* start, const char* finish, char* param, const int size);
char*		MSN_Base64Decode( const char* str );

void     	UrlDecode( char*str );
void     	UrlEncode( const char* src, char* dest, int cbDest );

HANDLE      MSN_HContactFromEmail( const char* msnEmail, const char* msnNick, int addIfNeeded, int temporary );
HANDLE      MSN_HContactFromEmailT( const TCHAR* msnEmail );
HANDLE      MSN_HContactById( const char* szGuid );

bool		MSN_IsMyContact( HANDLE hContact );
bool		MSN_IsMeByContact( HANDLE hContact, char* szEmail  = NULL );
bool        MSN_AddUser( HANDLE hContact, const char* email, int netId, int flags );
void        MSN_AddAuthRequest( HANDLE hContact, const char *email, const char *nick );
void        MSN_DebugLog( const char* fmt, ... );

void     __cdecl     MSN_ConnectionProc( HANDLE hNewConnection, DWORD dwRemoteIP, void* );
void        MSN_GoOffline( void );
char*		MSN_GetAvatarHash(char* szContext);
void        MSN_GetAvatarFileName( HANDLE hContact, char* pszDest, size_t cbLen );
void        MSN_GetCustomSmileyFileName( HANDLE hContact, char* pszDest, size_t cbLen, char* SmileyName, int Type);
LPTSTR      MSN_GetErrorText( DWORD parErrorCode );
void        MSN_SendStatusMessage( const char* msg );
long		MSN_SendSMS(char* tel, char* txt);
void        MSN_SetServerStatus( int newStatus );
void        LoadOptions( void );

void		MSN_SetNicknameUtf( char* nickname );
void		MSN_SendNicknameUtf( char* nickname );
#define		MSN_SendNicknameA(a) MSN_SendNicknameUtf(mir_utf8encode(a))
#define		MSN_SendNicknameT(a) MSN_SendNicknameUtf(mir_utf8encodeT(a))

#if defined( _DEBUG )
#define MSN_CallService CallService
#else
int         MSN_CallService( const char* szSvcName, WPARAM wParam, LPARAM lParam );
#endif

HANDLE      MSN_CreateProtoServiceFunction( const char*, MIRANDASERVICE );
void        MSN_DeleteSetting( HANDLE hContact, const char* valueName );
void        MSN_EnableMenuItems( bool );
void		MSN_FreeVariant( DBVARIANT* dbv );
char*       MSN_GetContactName( HANDLE hContact );
TCHAR*      MSN_GetContactNameT( HANDLE hContact );
DWORD       MSN_GetDword( HANDLE hContact, const char* valueName, DWORD parDefltValue );
DWORD       MSN_GetByte( const char* valueName, int parDefltValue );
int         MSN_GetStaticString( const char* valueName, HANDLE hContact, char* dest, unsigned dest_len );
int         MSN_GetStringT( const char* valueName, HANDLE hContact, DBVARIANT* dbv );
WORD        MSN_GetWord( HANDLE hContact, const char* valueName, int parDefltValue );
int         MSN_SendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam );
DWORD       MSN_SetByte( const char* valueName, BYTE parValue );
DWORD       MSN_SetDword( HANDLE hContact, const char* valueName, DWORD parValue );
DWORD       MSN_SetWord( HANDLE hContact, const char* valueName, WORD parValue );
DWORD       MSN_SetString( HANDLE hContact, const char* valueName, const char* parValue );
DWORD       MSN_SetStringT( HANDLE hContact, const char* valueName, const TCHAR* parValue );
DWORD       MSN_SetStringUtf( HANDLE hContact, const char* valueName, char* parValue );
void        MSN_ShowError( const char* msgtext, ... );
char*       MSN_Translate( const char* str );

HANDLE      GetIconHandle( int iconId );
HICON       LoadIconEx( const char* );
void        ReleaseIconEx( const char* );

int         addCachedMsg( const char* id, const char* msg, const size_t offset, 
						 const size_t portion, const size_t totsz, const bool bychunk );
bool        getCachedMsg( const int idx, char*& msg, size_t& size );
bool        getCachedMsg( const char* id, char*& msg, size_t& size );
void        clearCachedMsg( int idx = -1 );
void        CachedMsg_Uninit( void );

void        MsnInitIcons( void );
void        MsnInitMenus( void );

void        InitCustomFolders(void);

char*       httpParseHeader(char* buf, unsigned& status);
int         sttDivideWords( char* parBuffer, int parMinItems, char** parDest );
void		sttNotificationMessage( char* msgBody, bool isInitial );
int			MSN_SendOIM(const char* szEmail, const char* msg);
void		MSN_MakeDigest(const char* chl, char* dgst);
int			MSN_GetPassportAuth( char* authChallengeInfo );
char*		getNewUuid(void);

TCHAR* EscapeChatTags(const TCHAR* pszText);
TCHAR* UnEscapeChatTags(TCHAR* str_in);

void   overrideStr( TCHAR*& dest, const TCHAR* src, bool unicode, const TCHAR* def = NULL );
void   replaceStr( char*& dest, const char* src );
char*  rtrim( char* string );
wchar_t* rtrim( wchar_t* string );
char* arrayToHex(BYTE* data, size_t datasz);

extern LONG (WINAPI *MyInterlockedIncrement)(PLONG pVal);

/////////////////////////////////////////////////////////////////////////////////////////
// PNG library interface

typedef struct _tag_PopupData
{
	unsigned flags;
	HICON hIcon;
	char* url;
} PopupData;

/////////////////////////////////////////////////////////////////////////////////////////
//	MIME headers processing

class MimeHeaders
{
public:

	MimeHeaders();
	MimeHeaders( unsigned );
	~MimeHeaders();

	void clear(void);
	char*	readFromBuffer( char* pSrc );
	const char* find( const char* fieldName );
	const char* operator[]( const char* fieldName ) { return find( fieldName ); }
	char* decodeMailBody(char* msgBody);

	static wchar_t* decode(const char* val);

	void  addString( const char* name, const char* szValue, unsigned flags = 0 );
	void    addLong( const char* name, long lValue );
	void   addULong( const char* name, unsigned lValue );
	void	addBool( const char* name, bool lValue );

	size_t  getLength( void );
	char* writeToBuffer( char* pDest );

private:
	typedef struct tag_MimeHeader
	{
		const char* name;
		const char* value;
		unsigned flags;
	} MimeHeader;

	unsigned	mCount;
	unsigned	mAllocCount;
	MimeHeader* mVals;

	unsigned allocSlot(void);
};

/////////////////////////////////////////////////////////////////////////////////////////
//	File transfer helper

struct ThreadData;

struct HReadBuffer
{
	HReadBuffer( ThreadData* info, int iStart = 0 );
	~HReadBuffer();

	BYTE* surelyRead( int parBytes );

	ThreadData* owner;
	BYTE*			buffer;
	int			totalDataSize;
	int			startOffset;
};

enum TInfoType
{
	SERVER_DISPATCH,
	SERVER_NOTIFICATION,
	SERVER_SWITCHBOARD,
	SERVER_FILETRANS,
	SERVER_P2P_DIRECT
};


struct filetransfer
{
	filetransfer();
	~filetransfer( void );

	void close( void );
	void complete( void );
	int  create( void );
	int openNext( void );

	PROTOFILETRANSFERSTATUS std;

	bool        bCanceled;		// flag to interrupt a transfer
	bool        bCompleted;		// was a FT ever completed?
	bool        inmemTransfer;	//	flag: the file being received is to be stored in memory

	union {
		int      fileId;			// handle of file being transferring (r/w)
		char*	 fileBuffer;		// buffer of memory to handle the file
	};

	HANDLE		hLockHandle;

	TInfoType	tType;
	TInfoType	tTypeReq;
	time_t		ts;

	unsigned    p2p_sessionid;	// session id
	unsigned    p2p_acksessid;	// acknowledged session id
	unsigned    p2p_sendmsgid; // send message id
	unsigned    p2p_byemsgid;  // bye message id
	unsigned    p2p_waitack;    // invite message id
	unsigned    p2p_ackID;		// number of ack's state
	unsigned    p2p_appID;		// application id: 1 = avatar, 2 = file transfer
	int         p2p_type;		// application id: 1 = avatar, 2 = file transfer, 3 = custom emoticon
	char*       p2p_branch;		// header Branch: field
	char*       p2p_callID;		// header Call-ID: field
	char*       p2p_dest;		// destination e-mail address
	char*       p2p_object;     // MSN object for a transfer

	//---- receiving a file
	wchar_t*    wszFileName;	// file name in Unicode, for receiving
	char*       szInvcookie;	// cookie for receiving
	
	unsigned __int64 lstFilePtr;
};

struct directconnection
{
	directconnection( filetransfer* ft );
	~directconnection();

	char* calcHashedNonce( UUID* nonce );
	char* mNonceToText( void );
	char* mNonceToHash( void ) { return calcHashedNonce( mNonce ); }
	void  xNonceToBin( UUID* nonce );

	UUID* mNonce;
	char* xNonce;
	char* callId;

	time_t ts;

	bool useHashedNonce;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	Thread handling functions and datatypes

typedef void ( __cdecl* pThreadFunc )( void* );

struct TQueueItem
{
	TQueueItem* next;
	int  datalen;
	char data[1];
};

#define MSG_DISABLE_HDR      1
#define MSG_REQUIRE_ACK      2
#define MSG_RTL              4

struct ThreadData
{
	ThreadData();
	~ThreadData();

	TInfoType      mType;            // thread type
	pThreadFunc    mFunc;            // thread entry point
	char           mServer[80];      // server name

	HANDLE         s;	               // NetLib connection for the thread
	HANDLE		   mIncomingBoundPort; // Netlib listen for the thread
	HANDLE         hWaitEvent;
	WORD           mIncomingPort;
	TCHAR          mChatID[10];
	bool           mIsMainThread;
	int            mWaitPeriod;

	//----| for gateways |----------------------------------------------------------------
	char           mSessionID[ 50 ]; // Gateway session ID
	char           mGatewayIP[ 80 ]; // Gateway IP address
	int            mGatewayTimeout;
	char*          mReadAheadBuffer;
	char*          mReadAheadBufferPtr;
	int            mEhoughData;
	bool           sessionClosed;
	bool		   termPending;

	TQueueItem*	   mFirstQueueItem;
	unsigned       numQueueItems;
	HANDLE         hQueueMutex;

	//----| for switchboard servers only |------------------------------------------------
	int            mCaller;
	char           mCookie[130];     // for switchboard servers only
	HANDLE         mInitialContact;  // initial switchboard contact
	HANDLE*        mJoinedContacts;  //	another contacts
	int            mJoinedCount;     // another contacts count
	LONG           mTrid;            // current message ID
	UINT           mTimerId;         // typing notifications timer id
	bool           firstMsgRecv;

	//----| for file transfers only |-----------------------------------------------------
	filetransfer*  mMsnFtp;          // file transfer block

	//----| internal data buffer |--------------------------------------------------------
	int            mBytesInData;     // bytes available in data buffer
	char           mData[8192];      // data buffer for connection

	//----| methods |---------------------------------------------------------------------
	void           applyGatewayData( HANDLE hConn, bool isPoll );
	void           getGatewayUrl( char* dest, int destlen, bool isPoll );
	void           processSessionData( const char* );
	void           startThread( pThreadFunc );

	int            send( char data[], int datalen );
	int            recv( char* data, long datalen );
	int            recv_dg( char* data, long datalen );
	bool           isTimeout( void );
	char*          httpTransact(char* szCommand, size_t cmdsz, size_t& bdysz);

	void           sendCaps( void );
	LONG           sendMessage( int msgType, const char* email, int netId, const char* msg, int parFlags );
	LONG           sendRawMessage( int msgType, const char* data, int datLen );
	LONG           sendPacket( const char* cmd, const char* fmt, ... );
};

void		 MSN_CloseConnections( void );
void		 MSN_CloseThreads( void );
int			 MSN_ContactJoined( ThreadData* parInfo, HANDLE hContact );
int			 MSN_ContactLeft( ThreadData* parInfo, HANDLE hContact );
void		 MSN_InitThreads( void );
int			 MSN_GetChatThreads( ThreadData** parResult );
int          MSN_GetActiveThreads( ThreadData** );
ThreadData*  MSN_GetThreadByConnection( HANDLE hConn );
ThreadData*	 MSN_GetThreadByContact( HANDLE hContact, TInfoType type = SERVER_SWITCHBOARD );
ThreadData*  MSN_GetP2PThreadByContact( HANDLE hContact );
void         MSN_StartP2PTransferByContact( HANDLE hContact );
ThreadData*	 MSN_GetThreadByPort( WORD wPort );
ThreadData*  MSN_GetUnconnectedThread( HANDLE hContact );
ThreadData*  MSN_GetOtherContactThread( ThreadData* thread );
ThreadData*  MSN_GetThreadByTimer( UINT timerId );
void		 MSN_StartStopTyping( ThreadData* info, bool start );
void		 MSN_SendTyping( ThreadData* info, const char* email, int netId  );
ThreadData*	 MSN_StartSB(HANDLE hContact, bool& isOffline);

/////////////////////////////////////////////////////////////////////////////////////////
// MSN P2P session support

#define MSN_APPID_AVATAR		1
#define MSN_APPID_AVATAR2   	12
#define MSN_APPID_FILE			2

#define MSN_APPID_CUSTOMSMILEY  3
#define MSN_APPID_CUSTOMANIMATEDSMILEY  4

#define MSN_TYPEID_FTPREVIEW		0
#define MSN_TYPEID_FTNOPREVIEW		1
#define MSN_TYPEID_CUSTOMSMILEY		2
#define MSN_TYPEID_DISPLAYPICT		3
#define MSN_TYPEID_BKGNDSHARING		4
#define MSN_TYPEID_BKGNDIMG			5
#define MSN_TYPEID_WINK				8



void  p2p_clearDormantSessions( void );
void  p2p_cancelAllSessions( void );
void  p2p_redirectSessions( HANDLE hContact );

void  p2p_invite( HANDLE hContact, int iAppID, filetransfer* ft = NULL );
void  p2p_processMsg( ThreadData* info, char* msgbody );
void  p2p_sendStatus( filetransfer* ft, long lStatus );
void  p2p_sendBye( filetransfer* ft );
void  p2p_sendCancel( filetransfer* ft );
void  p2p_sendRedirect( filetransfer* ft );

void  p2p_sendFeedStart( filetransfer* ft );

void  p2p_registerSession( filetransfer* ft );
void  p2p_unregisterSession( filetransfer* ft );
void  p2p_sessionComplete( filetransfer* ft );

filetransfer*  p2p_getAvatarSession( HANDLE hContact );
filetransfer*  p2p_getThreadSession( HANDLE hContact, TInfoType mType );
filetransfer*  p2p_getSessionByID( unsigned id );
filetransfer*  p2p_getSessionByUniqueID( unsigned id );
filetransfer*  p2p_getSessionByCallID( const char* CallID );

bool  p2p_sessionRegistered( filetransfer* ft );
bool  p2p_isAvatarOnly( HANDLE hContact );
unsigned p2p_getMsgId( HANDLE hContact, int inc );

void  p2p_registerDC( directconnection* ft );
void  p2p_unregisterDC( directconnection* dc );
directconnection*  p2p_getDCByCallID( const char* CallID );

void ft_startFileSend( ThreadData* info, const char* Invcommand, const char* Invcookie );

void MSN_ChatStart(ThreadData* info);
void MSN_KillChatSession(TCHAR* id);

/////////////////////////////////////////////////////////////////////////////////////////
//	Message queue

#define MSGQUE_RAW	1

struct MsgQueueEntry
{
	HANDLE			hContact;
	char*			message;
	int             msgType;
	int				msgSize;
	filetransfer*	ft;
	int				seq;
	int				allocatedToThread;
	time_t			ts;
	int				flags;
};

int		 MsgQueue_Add( HANDLE hContact, int msgType, const char* msg, int msglen, filetransfer* ft = NULL, int flags = 0 );
HANDLE	 MsgQueue_CheckContact( HANDLE hContact, time_t tsc = 0 );
HANDLE	 MsgQueue_GetNextRecipient( void );
bool	 MsgQueue_GetNext( HANDLE hContact, MsgQueueEntry& retVal );
int		 MsgQueue_NumMsg( HANDLE hContact );
void     MsgQueue_Clear( HANDLE hContact = NULL, bool msg = false );

/////////////////////////////////////////////////////////////////////////////////////////
//	User lists

#define	LIST_FL		0x0001
#define	LIST_AL		0x0002
#define	LIST_BL		0x0004
#define	LIST_RL		0x0008
#define LIST_PL		0x0010

#define	LIST_REMOVE	0x0100

int		 Lists_Add( int list, int netId, const char* email );
bool	 Lists_IsInList( int list, const char* email );
int		 Lists_GetMask( const char* email );
int		 Lists_GetNetId( const char* email );
void	 Lists_Remove( int list, const char* email );
void	 Lists_Wipe( void );

void	 MSN_CreateContList(void);
void	 MSN_CleanupLists(void);
void	 MSN_FindYahooUser(const char* email);
void	 MSN_RefreshContactList(void);

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN server groups

void   MSN_AddGroup( const char* pName, const char* pId, bool init );
void   MSN_DeleteGroup( const char* pId );
void   MSN_FreeGroups( void );
LPCSTR MSN_GetGroupById( const char* pId );
LPCSTR MSN_GetGroupByName( const char* pName );
void   MSN_SetGroupName( const char* pId, const char* pName );

void   MSN_MoveContactToGroup( HANDLE hContact, const char* grpName );
void   MSN_RenameServerGroup( LPCSTR szId, const char* newName );
void   MSN_DeleteServerGroup( LPCSTR szId );
void   MSN_RemoveEmptyGroups( void );
void   MSN_SyncContactToServerGroup( HANDLE hContact, const char* szContId, ezxml_t cgrp );
void   MSN_UploadServerGroups( char* group );

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN SOAP Address Book

void MSN_SharingFindMembership(void);
bool MSN_SharingAddDelMember(const char* szEmail, const char* szRole, const char* szMethod);

void MSN_ABGetFull(void);
bool MSN_ABAddDelContactGroup(const char* szCntId, const char* szGrpId, const char* szMethod);
void MSN_ABAddGroup(const char* szGrpName);
void MSN_ABRenameGroup(const char* szGrpName, const char* szGrpId);
void MSN_ABUpdateNick(const char* szNick, const char* szCntId);
bool MSN_ABContactAdd(const char* szEmail, const char* szNick, const int typeId, const bool search);

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN plugin options

typedef struct _tag_MYOPTIONS
{
	COLORREF	BGColour;
	COLORREF	TextColour;
	bool		UseWinColors;

	bool		EnableSounds;

	bool		UseGateway;
	bool		UseProxy;
	bool		ShowErrorsAsPopups;
	bool		AwayAsBrb;
	bool		SlowSend;
	bool		ManageServer;

	DWORD		PopupTimeoutHotmail;
	DWORD		PopupTimeoutOther;

	char		szEmail[MSN_MAX_EMAIL_LEN];
}
MYOPTIONS;

extern MYOPTIONS MyOptions;

/////////////////////////////////////////////////////////////////////////////////////////
//	Windows error class

struct TWinErrorCode
{
	WINAPI	TWinErrorCode();
	WINAPI	~TWinErrorCode();

	char*		WINAPI getText();

	long		mErrorCode;
	char*		mErrorText;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	External variables

#define MSN_NUM_MODES 7

struct MSN_StatusMessage
{
	unsigned  m_mode;
	char*     m_msg;
};

extern  MSN_StatusMessage    msnModeMsgs[ MSN_NUM_MODES ];

extern  LISTENINGTOINFO      msnCurrentMedia;

extern	ThreadData*	msnNsThread;
extern	bool        msnLoggedIn;

extern	char*	    msnProtocolName;
extern	char*       msnExternalIP;
extern	unsigned	msnStatusMode,
					msnDesiredStatus;

extern	char*       msnProtChallenge;
extern	char*       msnProductID;

extern	char*		ModuleName;
extern	char*	    mailsoundname;
extern	char*	    alertsoundname;

extern	char*       sid;
extern	char*       passport;
extern	char*       urlId;
extern	char*       MSPAuth;
extern  char*       profileURL;
extern  char*       profileURLId;
extern  char*       rru;
extern  unsigned	langpref;
extern	char*       abchMigrated;

extern char pAuthToken[];
extern char tAuthToken[]; 
extern char pContAuthToken[];
extern char tContAuthToken[]; 

extern	HANDLE		hNetlibUser;
extern	HINSTANCE	hInst;
extern	unsigned	msnOtherContactsBlocked;

extern	bool		msnHaveChatDll;

///////////////////////////////////////////////////////////////////////////////
// UTF8 encode helper

class UTFEncoder 
{
private:
	char* m_body;

public:
	UTFEncoder( const char* pSrc ) :
		m_body( mir_utf8encode( pSrc )) {}

	~UTFEncoder() {  mir_free( m_body );	}
	const char* str() const { return m_body; }
};

#define UTF8(A) UTFEncoder(A).str()


typedef enum _tag_ConEnum
{
	conUnknown,
	conDirect,
	conUnknownNAT,
	conIPRestrictNAT,
	conPortRestrictNAT,
	conSymmetricNAT,
	conFirewall,
	conISALike
} ConEnum;

extern const char* conStr[];

typedef struct _tag_MyConnectionType
{
	unsigned intIP;
	unsigned extIP;
	ConEnum udpConType;
	ConEnum tcpConType;
	unsigned weight;
	bool upnpNAT;
	bool icf;

	const IN_ADDR GetMyExtIP(void) { return *((PIN_ADDR)&extIP); }
	const char* GetMyExtIPStr(void) { return inet_ntoa(GetMyExtIP()); }
	const char* GetMyUdpConStr(void) { return conStr[udpConType]; }
	void SetUdpCon(const char* str);
	void CalculateWeight(void);
} MyConnectionType;

extern MyConnectionType MyConnection;

/////////////////////////////////////////////////////////////////////////////////////////
// Basic SSL operation class

struct SSL_Base
{
	virtual	~SSL_Base();

	virtual  int init(void) = 0;
	virtual  char* getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs ) = 0;
};

class SSLAgent
{
private:
	SSL_Base* pAgent;

public:
	SSLAgent();
	~SSLAgent();

	char* getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs,
		unsigned& status, MimeHeaders& httpinfo, char*& htmlbody);
};


