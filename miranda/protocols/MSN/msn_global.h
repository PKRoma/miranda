/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright(C) 2002-2004 George Hazan (modification) and Richard Hughes (original)

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

#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <commctrl.h>

#include <malloc.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <newpluginapi.h>

#include <m_clc.h>
#include <m_clist.h>
#include <m_message.h>
#include <m_options.h>
#include <m_protocols.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_skin.h>
#include <m_system.h>
#include <m_userinfo.h>
#include <m_utils.h>

#include <m_database.h>
#include <m_langpack.h>
#include <m_netlib.h>
#include <m_popup.h>

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
#define ERR_ALREADY_IN_THE_MODE          218
#define ERR_ALREADY_IN_OPPOSITE_LIST     219
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
#define MSN_GUID_LEN					40

#define MSN_DEFAULT_PORT         1863
#define MSN_DEFAULT_LOGIN_SERVER "messenger.hotmail.com"
#define MSN_DEFAULT_GATEWAY      "gateway.messenger.hotmail.com"
#define MSN_USER_AGENT           "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)"

#define MSN_BLOCK        "/BlockCommand"
#define MSN_INVITE       "/InviteCommand"
#define MSN_NETMEETING   "/NetMeeting"
#define MSN_VIEW_PROFILE "/ViewProfile"

#define MENU_ITEMS_COUNT 2
#define MS_SET_NICKNAME  "/SetNicknameCommand"
#define MS_GOTO_INBOX    "/GotoInbox"
#define MS_EDIT_PROFILE  "/EditProfile"
#define MS_SET_AVATAR    "/SetAvatar"
#define MS_VIEW_STATUS   "/ViewMsnStatus"

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN plugin functions

BOOL		__stdcall	MSN_WS_Init( void );
void		__stdcall	MSN_WS_CleanUp( void );
int		__stdcall	MSN_WS_Send( HANDLE, char* data, int datalen );
int		__stdcall	MSN_WS_Recv( HANDLE, char* data, long datalen );

#define	MSN_ALLOW_MSGBOX		1
#define	MSN_ALLOW_ENTER		2
#define	MSN_HOTMAIL_POPUP		4
#define  MSN_SHOW_ERROR       8
void __stdcall MSN_ShowPopup( const char* nickname, const char* msg, int flags );

LONG		__stdcall	MSN_SendPacket( HANDLE, const char* cmd, const char* params, ... );
char*		__stdcall	MirandaStatusToMSN( int status );
int		__stdcall	MSNStatusToMiranda( const char* status );
void		__stdcall	UrlDecode( char*str );
void		__stdcall	UrlEncode( const char* src, char* dest, int cbDest );
void		__stdcall	Utf8Decode( char* str, int maxSize = 0, wchar_t** = NULL );
char*		__stdcall	Utf8Encode( const char* src );
char*    __stdcall	Utf8EncodeUcs2( const wchar_t* src );

HANDLE	__stdcall	MSN_HContactFromEmail( const char* msnEmail, const char* msnNick, int addIfNeeded, int temporary );
int		__stdcall	MSN_AddContact( char* uhandle,char* nick ); //returns clist ID
int		__stdcall	MSN_AddUser( HANDLE hContact, const char* email, int flags );
void		__stdcall	MSN_AddAuthRequest( HANDLE hContact, const char *email, const char *nick );
int		__stdcall	MSN_ContactFromHandle( char* uhandle ); //get cclist id from Uhandle
void		__stdcall	MSN_DebugLog( const char* fmt, ... );
void		__stdcall	MSN_HandleFromContact( unsigned long uin, char* uhandle );
int		__stdcall	MSN_GetMyHostAsString( char* parBuf, int parBufSize );

void		__cdecl		MSN_ConnectionProc( HANDLE hNewConnection, DWORD dwRemoteIP );
void		__stdcall	MSN_GoOffline( void );
void		__stdcall	MSN_GetAvatarFileName( HANDLE hContact, char* pszDest, int cbLen );
LPTSTR	__stdcall   MSN_GetErrorText( DWORD parErrorCode );
int		__stdcall	MSN_SendNickname(char *email, char *nickname);
void		__stdcall	MSN_SetServerStatus( int newStatus );
char*		__stdcall	MSN_StoreLen( char* dest, char* last );
void		__stdcall	LoadOptions( void );

HANDLE	__stdcall	MSN_CreateProtoServiceFunction( const char*, MIRANDASERVICE );
int		__stdcall	MSN_CallService( const char* szSvcName, WPARAM wParam, LPARAM lParam );
void     __stdcall   MSN_EnableMenuItems( BOOL );
void     __fastcall	MSN_FreeVariant( DBVARIANT* dbv );
char*		__stdcall	MSN_GetContactName( HANDLE hContact );
DWORD		__stdcall	MSN_GetDword( HANDLE hContact, const char* valueName, DWORD parDefltValue );
DWORD		__stdcall	MSN_GetByte( const char* valueName, int parDefltValue );
int		__stdcall	MSN_GetStaticString( const char* valueName, HANDLE hContact, char* dest, int dest_len );
WORD     __stdcall   MSN_GetWord( HANDLE hContact, const char* valueName, int parDefltValue );
int		__stdcall	MSN_SendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam );
DWORD		__stdcall	MSN_SetByte( const char* valueName, int parValue );
DWORD		__stdcall	MSN_SetDword( HANDLE hContact, const char* valueName, DWORD parValue );
DWORD		__stdcall	MSN_SetWord( HANDLE hContact, const char* valueName, int parValue );
DWORD		__stdcall	MSN_SetString( HANDLE hContact, const char* valueName, const char* parValue );
void     __cdecl		MSN_ShowError( const char* msgtext, ... );
char*		__stdcall	MSN_Translate( const char* str );

VOID		CALLBACK MSNMainTimerProc( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime );
LRESULT	CALLBACK NullWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
DWORD		WINAPI	MsnShowMailThread( LPVOID );

int IsWinver( void );
void strdel( char* parBuffer, int len );

/////////////////////////////////////////////////////////////////////////////////////////
// PNG library interface

BOOL __stdcall MSN_LoadPngModule( void );

typedef	BOOL ( *pfnConvertPng2dib )( char*, size_t, BITMAPINFOHEADER** );
extern pfnConvertPng2dib png2dibConvertor;

typedef	BOOL ( *pfnConvertDib2png )( BITMAPINFO*, BYTE* pDiData, BYTE* result, long* resLen );
extern pfnConvertDib2png dib2pngConvertor;

typedef	DWORD ( *pfnGetVer )();
extern pfnGetVer getver;

/////////////////////////////////////////////////////////////////////////////////////////
//	MIME headers processing

struct MimeHeader
{
	char* name;
	char*	value;
};

struct MimeHeaders
{
	MimeHeaders();
	MimeHeaders( int );
	~MimeHeaders();

	char*	readFromBuffer( const char* pSrc );
	const char* operator[]( const char* fieldName );

	void  addString( const char* name, const char* szValue );
	void	addLong( const char* name, long lValue );

	int   getLength( void );
	char* writeToBuffer( char* pDest );

	int			mCount;
	MimeHeader* mVals;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	File transfer helper

struct ThreadData;

struct P2P_Header
{
	DWORD		mSessionID;
	DWORD		mID;
	__int64	mOffset;
	__int64  mTotalSize;
	DWORD		mPacketLen;
	DWORD		mFlags;
	DWORD		mAckSessionID;
	DWORD		mAckUniqueID;
	__int64	mAckDataSize;
};

struct HReadBuffer
{
	HReadBuffer( ThreadData*, int );
	~HReadBuffer();

	BYTE* surelyRead( HANDLE s, int parBytes );

	ThreadData* owner;
	BYTE*			buffer;
	int			totalDataSize;
	int			startOffset;
};

struct filetransfer
{
	filetransfer();
	~filetransfer( void );

	void close();

	PROTOFILETRANSFERSTATUS std;

	bool		   bCanceled;		// flag to interrupt a transfer
	bool			bCompleted;		// was a FT ever completed?
	bool			inmemTransfer;	//	flag: the file being received is to be stored in memory

	union {
		int		fileId;			// handle of file being transferring (r/w)
		char*		fileBuffer;		// buffer of memory to handle the file
	};

	bool			mOwnsThread,	// thread was created specifically for that file transfer
					mIsDirect;

	WORD			mIncomingPort;
	HANDLE		mIncomingBoundPort;
	HANDLE		hWaitEvent;

	long			p2p_sessionid;	// session id
	long			p2p_msgid;		// message id
	long			p2p_acksessid;	// acknowledged session id
	int			p2p_ackID;		// number of ack's state
	int			p2p_appID;		// application id: 1 = avatar, 2 = file transfer
	P2P_Header	p2p_hdr;			// saved header
	char*			p2p_branch;		// header Branch: field
	char*			p2p_callID;		// header Call-ID: field
	char*			p2p_dest;		// destination e-mail address

	//---- receiving a file
	wchar_t*    wszFileName;	// file name in Unicode, for receiving
	char*		   szInvcookie;	// cookie for receiving
};

/////////////////////////////////////////////////////////////////////////////////////////
//	Thread handling functions and datatypes

#define MAX_THREAD_COUNT 64

typedef void ( __cdecl* pThreadFunc )( void* );

enum TInfoType
{
	SERVER_DISPATCH,
	SERVER_NOTIFICATION,
	SERVER_SWITCHBOARD,
	SERVER_FILETRANS
};

struct TQueueItem
{
	TQueueItem* next;
	char data[1];
};

#define	MSG_DISABLE_HDR		1
#define	MSG_REQUIRE_ACK		2

struct ThreadData
{
	ThreadData();
	~ThreadData();

	TInfoType      mType;            // thread type
	char           mServer[80];      // server name

	HANDLE         s;	               // NetLib connection for the thread

	//----| for gateways |----------------------------------------------------------------
	char           mSessionID[ 50 ]; // Gateway session ID
	char           mGatewayIP[ 80 ]; // Gateway IP address
	int            mGatewayTimeout;
	char*				mReadAheadBuffer;
	int				mEhoughData;
	TQueueItem*		mFirstQueueItem;

	//----| for switchboard servers only |------------------------------------------------
	int            mCaller;
	char           mCookie[130];     // for switchboard servers only
	HANDLE			mInitialContact;  // initial switchboard contact
	HANDLE*        mJoinedContacts;  //	another contacts
	int            mJoinedCount;     // another contacts count
	int				mMessageCount;		// message counter
	LONG				mTrid;				// current message ID

	//----| for file transfers only |-----------------------------------------------------
	filetransfer*  ft;               // file transfer block
	DWORD          mTotalSend;       // number of bytes wriiten

	//----| internal data buffer |--------------------------------------------------------
	int            mBytesInData;     // bytes available in data buffer
	char           mData[4096];      // data buffer for connection

	//----| methods |---------------------------------------------------------------------
	void           applyGatewayData( HANDLE hConn, bool isPoll );
	void				getGatewayUrl( char* dest, int destlen, bool isPoll );
	void				processSessionData( const char* );
	void           startThread( pThreadFunc );

	LONG				sendMessage( const char* msg, int parFlags );
	LONG				sendRawMessage( int msgType, const char* data, int datLen );
	LONG				vsendPacket( const char* cmd, const char* fmt, va_list vararg );
	LONG				sendPacket( const char* cmd, const char* fmt, ... );
};

void			__stdcall MSN_CloseConnections( void );
void			__stdcall MSN_CloseThreads( void );
int			__stdcall MSN_ContactJoined( ThreadData* parInfo, HANDLE hContact );
int			__stdcall MSN_ContactLeft( ThreadData* parInfo, HANDLE hContact );
void			__stdcall MSN_InitThreads( void );
int         __stdcall MSN_GetActiveThreads( ThreadData** );
ThreadData* __stdcall MSN_GetThreadByConnection( HANDLE hConn );
ThreadData*	__stdcall MSN_GetThreadByContact( HANDLE hContact );
ThreadData*	__stdcall MSN_GetThreadByPort( WORD wPort );
ThreadData* __stdcall MSN_GetUnconnectedThread( HANDLE hContact );

/////////////////////////////////////////////////////////////////////////////////////////
// MSN P2P session support

#define MSN_APPID_AVATAR 1
#define MSN_APPID_FILE   2

void __stdcall p2p_invite( HANDLE hContact, int iAppID, filetransfer* ft = NULL );
void __stdcall p2p_processMsg( ThreadData* info, char* msgbody );
void __stdcall p2p_sendAck( filetransfer* ft, ThreadData* info, P2P_Header* hdrdata );
void __stdcall p2p_sendStatus( filetransfer* ft, ThreadData* info, long lStatus );

void __stdcall p2p_registerSession( filetransfer* ft );
void __stdcall p2p_unregisterSession( filetransfer* ft );

filetransfer* __stdcall p2p_getSessionByID( long ID );
filetransfer* __stdcall p2p_getSessionByCallID( const char* CallID );

/////////////////////////////////////////////////////////////////////////////////////////
//	Message queue

#define MSGQUE_RAW	1

struct MsgQueueEntry
{
	HANDLE			hContact;
	char*				message;
	int				msgSize;
	filetransfer*	ft;
	int				seq;
	int				allocatedToThread;
};

int		__stdcall MsgQueue_Add( HANDLE hContact, const char* msg, int msglen, filetransfer* ft = NULL );
HANDLE	__stdcall MsgQueue_CheckContact( HANDLE hContact );
HANDLE	__stdcall MsgQueue_GetNextRecipient( void );
int		__stdcall MsgQueue_GetNext( HANDLE hContact, MsgQueueEntry& retVal );

/////////////////////////////////////////////////////////////////////////////////////////
//	User lists

#define	LIST_FL		0x0001
#define	LIST_AL		0x0002
#define	LIST_BL		0x0004
#define	LIST_RL		0x0008
#define  LIST_PL		0x0010

#define	LIST_REMOVE	0x0100

#define	IsValidListCode(n)  ((n)!=0)

int		__stdcall Lists_NameToCode( const char* name );
int		__stdcall Lists_Add( int list, const char* email, const char* nick );
int		__stdcall Lists_IsInList( int list, const char* email );
int		__stdcall Lists_GetMask( const char* email );
void		__stdcall Lists_Remove( int list, const char* email );
void		__stdcall Lists_Wipe( void );

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN plugin options

typedef struct
{
	COLORREF	BGColour;
	COLORREF	TextColour;
	BOOL		UseWinColors;

	BOOL		EnableSounds;

	DWORD		PopupTimeoutHotmail;
	DWORD		PopupTimeoutOther;

	BOOL		DisableMenu;
	BOOL		UseGateway;
	BOOL		UseProxy;
	BOOL		KeepConnectionAlive;
	BOOL		ShowErrorsAsPopups;
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

extern	volatile		HANDLE	msnNSSocket;
extern	volatile		bool		msnLoggedIn;

extern	char*	         msnProtocolName;
extern	HANDLE         msnMenuItems[ MENU_ITEMS_COUNT ];
extern	int            msnSearchID;
extern	char*          msnExternalIP;
extern	int				msnStatusMode,
								msnDesiredStatus;

extern	int				msnProtVersion;
extern	char*          msnProtChallenge;
extern	char*          msnProductID;


extern	char*			   ModuleName;
extern	char*	         mailsoundname;
extern	HANDLE         msnGetInfoContact;

extern	char*          sid;
extern	char*          kv;
extern	char*          passport;
extern	char*          MSPAuth;

extern	HANDLE		   hNetlibUser;
extern	HINSTANCE	   hInst;
extern	int				msnOtherContactsBlocked;

extern   bool				msnRunningUnderNT;
