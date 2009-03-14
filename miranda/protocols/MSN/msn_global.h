/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2009 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// this plugin is for Miranda 0.8 only
#define MIRANDA_VER 0x0800

#include "m_stdhdr.h"

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
#include <m_clistint.h>
#include <m_clui.h>
#include <m_idle.h>
#include <m_icolib.h>
#include <m_message.h>
#include <m_options.h>
#include <m_protocols.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_protoint.h>
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
#define ERR_LIST_UNAVAILABLE             508
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
#define MS_EDIT_ALERTS		"/EditAlerts"
#define MS_VIEW_STATUS		"/ViewMsnStatus"
#define MS_SET_NICKNAME_UI  "/SetNicknameUI"

#define MSN_SET_NICKNAME            "/SetNickname"

#define MSN_GETUNREAD_EMAILCOUNT	"/GetUnreadEmailCount"
/////////////////////////////////////////////////////////////////////////////////////////
//	MSN plugin functions

struct CMsnProto;

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)
#define NEWTSTR_ALLOCA(A) (A==NULL)?NULL:_tcscpy((TCHAR*)alloca(sizeof(TCHAR)*(_tcslen(A)+1)),A)

#define	MSN_ALLOW_MSGBOX    1
#define	MSN_ALLOW_ENTER	    2
#define	MSN_HOTMAIL_POPUP   4
#define MSN_SHOW_ERROR      8
#define	MSN_ALERT_POPUP	    16

void        HtmlDecode( char* str );
char*       HtmlEncode( const char* str );
bool		txtParseParam (const char* szData, const char* presearch, const char* start, const char* finish, char* param, const int size);
void		stripBBCode( char* src );
char*		MSN_Base64Decode( const char* str );

void     	UrlDecode( char* str );
void     	UrlEncode( const char* src, char* dest, size_t cbDest );

void		__cdecl MSN_ConnectionProc( HANDLE hNewConnection, DWORD dwRemoteIP, void* );

char*		MSN_GetAvatarHash(char* szContext);
void        MSN_GetAvatarFileName( HANDLE hContact, char* pszDest, size_t cbLen );
int			MSN_GetImageFormat(void* buf, const char** ext);

#define		MSN_SendNicknameA(a) MSN_SendNicknameUtf(mir_utf8encode(a))
#define		MSN_SendNicknameT(a) MSN_SendNicknameUtf(mir_utf8encodeT(a))

#if defined( _DEBUG )
#define MSN_CallService CallService
#else
int         MSN_CallService( const char* szSvcName, WPARAM wParam, LPARAM lParam );
#endif

void        MSN_EnableMenuItems( bool );
void		MSN_FreeVariant( DBVARIANT* dbv );
TCHAR*      MSN_GetContactNameT( HANDLE hContact );
char*       MSN_Translate( const char* str );
unsigned    MSN_GenRandom(void);

HANDLE      GetIconHandle( int iconId );
HICON       LoadIconEx( const char* );
void        ReleaseIconEx( const char* );

void        MsnInitIcons( void );

char*       httpParseHeader(char* buf, unsigned& status);
int         sttDivideWords( char* parBuffer, int parMinItems, char** parDest );
void		MSN_MakeDigest(const char* chl, char* dgst);
char*		getNewUuid(void);

TCHAR* EscapeChatTags(const TCHAR* pszText);
TCHAR* UnEscapeChatTags(TCHAR* str_in);

void   overrideStr( TCHAR*& dest, const TCHAR* src, bool unicode, const TCHAR* def = NULL );
void   replaceStr( char*& dest, const char* src );
char*  rtrim( char* string );
wchar_t* rtrim( wchar_t* string );
char* arrayToHex(BYTE* data, size_t datasz);

LONG WINAPI MyInterlockedIncrement(PLONG pVal);

/////////////////////////////////////////////////////////////////////////////////////////
// PNG library interface

typedef struct _tag_PopupData
{
	unsigned flags;
	HICON hIcon;
	char* url;
	CMsnProto* proto;
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
	filetransfer(CMsnProto* prt);
	~filetransfer( void );

	void close( void );
	void complete( void );
	int  create( void );
	int openNext( void );

	CMsnProto* proto;

	PROTOFILETRANSFERSTATUS std;

	bool        bCanceled;		// flag to interrupt a transfer
	bool        bCompleted;		// was a FT ever completed?
	
	int			fileId;			// handle of file being transferring (r/w)

	HANDLE		hLockHandle;

	TInfoType	tType;
	TInfoType	tTypeReq;
	time_t		ts;

	unsigned    p2p_sessionid;	// session id
	unsigned    p2p_acksessid;	// acknowledged session id
	unsigned    p2p_sendmsgid;  // send message id
	unsigned    p2p_byemsgid;   // bye message id
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

	CMsnProto* proto;
};


#pragma pack(1)

typedef struct _tag_HFileContext
{
	unsigned len; 
	unsigned ver;
	unsigned __int64 dwSize;
	unsigned type;
	wchar_t wszFileName[ MAX_PATH ];
	char unknown[30];
	unsigned id;
	char unknown2[64];
} HFileContext;

typedef struct _tad_P2P_Header
{
	unsigned          mSessionID;
	unsigned          mID;
	unsigned __int64  mOffset;
	unsigned __int64  mTotalSize;
	unsigned          mPacketLen;
	unsigned          mFlags;
	unsigned          mAckSessionID;
	unsigned          mAckUniqueID;
	unsigned __int64  mAckDataSize;
} P2P_Header;

#pragma pack()

bool p2p_IsDlFileOk(filetransfer* ft);

/////////////////////////////////////////////////////////////////////////////////////////
//	Thread handling functions and datatypes

struct TQueueItem
{
	TQueueItem* next;
	int  datalen;
	char data[1];
};

#define MSG_DISABLE_HDR      1
#define MSG_REQUIRE_ACK      2
#define MSG_RTL              4

struct CMsnProto;
typedef void ( __cdecl CMsnProto::*MsnThreadFunc )( void* );

struct ThreadData
{
	ThreadData();
	~ThreadData();

	TInfoType      mType;            // thread type
	MsnThreadFunc  mFunc;            // thread entry point
	char           mServer[80];      // server name

	HANDLE         s;	               // NetLib connection for the thread
	HANDLE		   mIncomingBoundPort; // Netlib listen for the thread
	HANDLE         hWaitEvent;
	WORD           mIncomingPort;
	TCHAR          mChatID[10];
	bool           mIsMainThread;
	int            mWaitPeriod;

	CMsnProto*     proto;

	//----| for gateways |----------------------------------------------------------------
	char           mSessionID[ 50 ]; // Gateway session ID
	char           mGatewayIP[ 80 ]; // Gateway IP address
	int            mGatewayTimeout;
	char*          mReadAheadBuffer;
	char*          mReadAheadBufferPtr;
	int            mEhoughData;
	bool           sessionClosed;
	bool		   termPending;
	bool		   gatewayType;

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
	void           startThread( MsnThreadFunc , CMsnProto *prt );

	int            send( const char data[], size_t datalen );
	int            recv( char* data, size_t datalen );
	int            recv_dg( char* data, size_t datalen );
	bool           isTimeout( void );
	char*          httpTransact(char* szCommand, size_t cmdsz, size_t& bdysz);

	void           sendCaps( void );
	LONG           sendMessage( int msgType, const char* email, int netId, const char* msg, int parFlags );
	LONG           sendRawMessage( int msgType, const char* data, int datLen );
	LONG           sendPacket( const char* cmd, const char* fmt, ... );

	int			 contactJoined( HANDLE hContact );
	int			 contactLeft( HANDLE hContact );
};


/////////////////////////////////////////////////////////////////////////////////////////
// MSN P2P session support

#define MSN_APPID_AVATAR		1
#define MSN_APPID_AVATAR2   	12
#define MSN_APPID_FILE			2
#define MSN_APPID_WEBCAM		4

#define MSN_APPID_CUSTOMSMILEY  3
#define MSN_APPID_CUSTOMANIMATEDSMILEY  4

#define MSN_TYPEID_FTPREVIEW		0
#define MSN_TYPEID_FTNOPREVIEW		1
#define MSN_TYPEID_CUSTOMSMILEY		2
#define MSN_TYPEID_DISPLAYPICT		3
#define MSN_TYPEID_BKGNDSHARING		4
#define MSN_TYPEID_BKGNDIMG			5
#define MSN_TYPEID_WINK				8



inline bool IsChatHandle(HANDLE hContact) { return (int)hContact < 0; }


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


/////////////////////////////////////////////////////////////////////////////////////////
//	User lists

struct ServerGroupItem
{
	char* id;
	char* name; // in UTF8
};

struct MsnContact
{
	char *email;
	int list;
	int netId;

	~MsnContact() { mir_free(email); }
};

#define NETID_UNKNOWN	0x0000
#define NETID_MSN		0x0001
#define NETID_LCS		0x0002
#define NETID_MOB		0x0004
#define NETID_YAHOO		0x0020
#define NETID_EMAIL		0x8000

#define	LIST_FL		0x0001
#define	LIST_AL		0x0002
#define	LIST_BL		0x0004
#define	LIST_RL		0x0008
#define LIST_PL		0x0010

#define	LIST_REMOVE	0x0100

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

#define MSN_NUM_MODES 8

const char msnProtChallenge[] = "ILTXC!4IXB5FB*PX";
const char msnProductID[] = "PROD0119GSJUC$18";
const char msnProductVer[] = "8.5.1302";
const char msnProtID[] = "MSNP15";


extern	HINSTANCE	hInst;
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

#pragma pack(1)
typedef struct _tag_UDPProbePkt
{
	unsigned char  version;
	unsigned char  serviceCode;
	unsigned short clientPort;
	unsigned	   clientIP;
	unsigned short discardPort;
	unsigned short testPort;
	unsigned	   testIP;
	unsigned       trId;
} UDPProbePkt;
#pragma pack()

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

struct chunkedmsg
{
	char* id;
	char* msg;
	size_t size;
	size_t recvsz;
	bool bychunk;

	chunkedmsg(const char* tid, const size_t totsz, const bool bychunk);
	~chunkedmsg();

	void add(const char* msg, const size_t offset, const size_t portion);
	bool get(char*& tmsg, size_t& tsize);
};
