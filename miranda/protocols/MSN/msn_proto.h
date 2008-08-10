/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2008 Boris Krasnovskiy.

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

#ifndef _MSN_PROTO_H_
#define _MSN_PROTO_H_

#include <m_protoint.h>

struct CMsnProto;
typedef void ( __cdecl CMsnProto::*MsnThreadFunc )( void* );
typedef int  ( __cdecl CMsnProto::*MsnEventFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CMsnProto::*MsnServiceFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CMsnProto::*MsnServiceFuncParam )( WPARAM, LPARAM, LPARAM );

struct CMsnProto : public PROTO_INTERFACE
{
        CMsnProto( const char*, const TCHAR* );
        ~CMsnProto();

        __inline void* operator new( size_t size )
        {	return calloc( 1, size );
        }
        __inline void operator delete( void* p )
        {	free( p );
        }

    //====================================================================================
    // PROTO_INTERFACE
    //====================================================================================

    virtual	HANDLE __cdecl AddToList( int flags, PROTOSEARCHRESULT* psr );
    virtual	HANDLE __cdecl AddToListByEvent( int flags, int iContact, HANDLE hDbEvent );

    virtual	int    __cdecl Authorize( HANDLE hDbEvent );
    virtual	int    __cdecl AuthDeny( HANDLE hDbEvent, const char* szReason );
    virtual	int    __cdecl AuthRecv( HANDLE hContact, PROTORECVEVENT* );
    virtual	int    __cdecl AuthRequest( HANDLE hContact, const char* szMessage );

    virtual	HANDLE __cdecl ChangeInfo( int iInfoType, void* pInfoData );

    virtual	int    __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath );
    virtual	int    __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer );
    virtual	int    __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason );
    virtual	int    __cdecl FileResume( HANDLE hTransfer, int* action, const char** szFilename );

    virtual	DWORD  __cdecl GetCaps( int type, HANDLE hContact = NULL );
    virtual	HICON  __cdecl GetIcon( int iconIndex );
    virtual	int    __cdecl GetInfo( HANDLE hContact, int infoType );

    virtual	HANDLE __cdecl SearchBasic( const char* id );
    virtual	HANDLE __cdecl SearchByEmail( const char* email );
    virtual	HANDLE __cdecl SearchByName( const char* nick, const char* firstName, const char* lastName );
    virtual	HWND   __cdecl SearchAdvanced( HWND owner );
    virtual	HWND   __cdecl CreateExtendedSearchUI( HWND owner );

    virtual	int    __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* );
    virtual	int    __cdecl RecvFile( HANDLE hContact, PROTORECVFILE* );
    virtual	int    __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* );
    virtual	int    __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* );

    virtual	int    __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList );
    virtual	int    __cdecl SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles );
    virtual	int    __cdecl SendMsg( HANDLE hContact, int flags, const char* msg );
    virtual	int    __cdecl SendUrl( HANDLE hContact, int flags, const char* url );

    virtual	int    __cdecl SetApparentMode( HANDLE hContact, int mode );
    virtual	int    __cdecl SetStatus( int iNewStatus );
    void                   SetStatusWorker( int iNewStatus );

    virtual	int    __cdecl GetAwayMsg( HANDLE hContact );
    virtual	int    __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt );
    virtual	int    __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg );
    virtual	int    __cdecl SetAwayMsg( int m_iStatus, const char* msg );

    virtual	int    __cdecl UserIsTyping( HANDLE hContact, int type );

    virtual	int    __cdecl OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam );

    //====| Services |====================================================================
    int  __cdecl SvcCreateAccMgrUI(WPARAM wParam, LPARAM lParam);

    int  __cdecl GetName(WPARAM wParam, LPARAM lParam);
    int  __cdecl GetStatus(WPARAM wParam, LPARAM lParam);
    int  __cdecl GetAvatarInfo(WPARAM wParam, LPARAM lParam);

    int  __cdecl GetAvatar(WPARAM wParam, LPARAM lParam);
    int  __cdecl SetAvatar(WPARAM wParam, LPARAM lParam);
    int  __cdecl GetAvatarCaps(WPARAM wParam, LPARAM lParam);

    int  __cdecl GetCurrentMedia(WPARAM wParam, LPARAM lParam);
    int  __cdecl SetCurrentMedia(WPARAM wParam, LPARAM lParam);

    int  __cdecl SetNickName(WPARAM wParam, LPARAM lParam);
    int  __cdecl SendNudge(WPARAM wParam, LPARAM lParam);

    int  __cdecl GetUnreadEmailCount(WPARAM wParam, LPARAM lParam);

    int  __cdecl ManageAccount(WPARAM wParam, LPARAM lParam);

    //====| Events |======================================================================
    int  __cdecl OnContactDeleted(WPARAM wParam,LPARAM lParam);
    int  __cdecl OnIdleChanged( WPARAM wParam, LPARAM lParam );
    int  __cdecl OnGroupChange( WPARAM wParam, LPARAM lParam );
    int  __cdecl OnModulesLoaded( WPARAM wParam, LPARAM lParam );
    int  __cdecl OnOptionsInit(WPARAM wParam,LPARAM lParam);
    int  __cdecl OnPrebuildContactMenu(WPARAM wParam,LPARAM lParam);
    int  __cdecl OnPreShutdown(WPARAM wParam,LPARAM lParam);
    int  __cdecl OnDbSettingChanged(WPARAM wParam,LPARAM lParam);
    int  __cdecl OnUserInfoInit(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnWindowEvent(WPARAM wParam, LPARAM lParam);

    //====| Data |========================================================================

    // Security Tokens
    char *pAuthToken, *tAuthToken; 
    char *oimSendToken;
    char *authStrToken, *authSecretToken; 
    char *authContactToken;
    char *authStorageToken;
    char *hotSecretToken, *hotAuthToken;

    CRITICAL_SECTION csLists;
    OBJLIST<MsnContact> contList;

    LIST<ServerGroupItem> grpList;

	CRITICAL_SECTION sttLock;
    OBJLIST<ThreadData> sttThreads;

    CRITICAL_SECTION sessionLock;
    OBJLIST<filetransfer> sessionList;
    OBJLIST<directconnection> dcList;

    CRITICAL_SECTION csMsgQueue;
    int msgQueueSeq;
    OBJLIST<MsgQueueEntry> msgQueueList;

	int tridUrlInbox;

	int msnPingTimeout;
	HANDLE hKeepAliveThreadEvt;

    char*    msnModeMsgs[ MSN_NUM_MODES ];

    LISTENINGTOINFO     msnCurrentMedia;
	MYOPTIONS			MyOptions;
	MyConnectionType	MyConnection;

    ThreadData*	msnNsThread;
    bool        msnLoggedIn;

    char*       msnExternalIP;
    char*		msnPreviousUUX;

    char*		ModuleName;
    char*	    mailsoundname;
    char*	    alertsoundname;

    char*       passport;
    char*       urlId;
    char*       rru;
    unsigned	langpref;
    char*       abchMigrated;

	HANDLE		msnSearchId;
    unsigned	msnOtherContactsBlocked;
    int			mUnreadMessages;
    int			mUnreadJunkEmails;

    HANDLE		hNetlibUser;
	HANDLE		hNetlibUserHttps;
	HANDLE		hInitChat;
    HANDLE		hMSNNudge;

    HANDLE		hMSNAvatarsFolder;
    HANDLE		hCustomSmileyFolder;
    bool		InitCstFldRan;
	void        InitCustomFolders(void);

    void        MSN_DebugLog( const char* fmt, ... );

	char*		getSslResult(char** parUrl, const char* parAuthInfo, const char* hdrs, unsigned& status);

    void        MSN_GoOffline( void );
    void        MSN_GetAvatarFileName( HANDLE hContact, char* pszDest, size_t cbLen );
    void        MSN_GetCustomSmileyFileName( HANDLE hContact, char* pszDest, size_t cbLen, char* SmileyName, int Type);
	
	char*		MirandaStatusToMSN( int status );
	WORD		MSNStatusToMiranda(const char *status);
	char**		GetStatusMsgLoc( int status );
    
	void        MSN_SendStatusMessage( const char* msg );
    void        MSN_SetServerStatus( int newStatus );
    long		MSN_SendSMS(char* tel, char* txt);
    void		MSN_StartStopTyping( ThreadData* info, bool start );
    void		MSN_SendTyping( ThreadData* info, const char* email, int netId  );

    void		MSN_ReceiveMessage( ThreadData* info, char* cmdString, char* params );
    int			MSN_HandleCommands( ThreadData* info, char* cmdString );
    int			MSN_HandleErrors( ThreadData* info, char* cmdString );
	void		sttProcessNotificationMessage( char* buf, unsigned len );
	void		sttProcessStatusMessage( char* buf, unsigned len, HANDLE hContact );
	void		sttProcessPage( char* buf, unsigned len );
	void		sttProcessRemove( char* buf, size_t len );
	void		sttProcessAdd( char* buf, size_t len );
	void		sttProcessYFind( char* buf, size_t len );
	void		sttCustomSmiley( const char* msgBody, char* email, char* nick, int iSmileyType );
	void		sttInviteMessage( ThreadData* info, char* msgBody, char* email, char* nick );
	void		sttSetMirVer( HANDLE hContact, DWORD dwValue );
    
    void        LoadOptions( void );

	void		MSN_ShowPopup( const TCHAR* nickname, const TCHAR* msg, int flags, const char* url, HANDLE hContact = NULL );
	void		MSN_ShowPopup( const HANDLE hContact, const TCHAR* msg, int flags );
	void		MSN_ShowError( const char* msgtext, ... );

    void		MSN_SetNicknameUtf( char* nickname );
    void		MSN_SendNicknameUtf( char* nickname );


	/////////////////////////////////////////////////////////////////////////////////////////
    // MSN Connection properties detection

	void DecryptEchoPacket(UDPProbePkt& pkt);
	void MSNatDetect(void);
	void __cdecl MSNConnDetectThread( void* );

	/////////////////////////////////////////////////////////////////////////////////////////
    // MSN menus

	HANDLE mainMenuRoot;
	HANDLE blockMenuItem;
	HANDLE menuItems[ 1 ];
	HANDLE menuItemsAll[ 6 ];

	void MsnInitMenus( void );
	void MSN_EnableMenuItems( bool parEnable );
	void MsnInvokeMyURL( bool ismail, char* url );

	int __cdecl MsnBlockCommand( WPARAM wParam, LPARAM lParam );
	int __cdecl MsnGotoInbox( WPARAM, LPARAM );
	int __cdecl MsnEditProfile( WPARAM, LPARAM );
	int __cdecl MsnInviteCommand( WPARAM wParam, LPARAM lParam );
	int __cdecl MsnSendNetMeeting( WPARAM wParam, LPARAM lParam );
	int __cdecl SetNicknameUI( WPARAM wParam, LPARAM lParam );
	int __cdecl MsnViewProfile( WPARAM wParam, LPARAM lParam );
	int __cdecl MsnViewServiceStatus( WPARAM wParam, LPARAM lParam );

	/////////////////////////////////////////////////////////////////////////////////////////
    // MSN thread functions
	
	void __cdecl msn_keepAliveThread( void* arg );
	void __cdecl MSNServerThread( void* arg );

	void __cdecl MsnFileAckThread( void* arg );
	void __cdecl MsnSearchAckThread( void* arg );
	void __cdecl sttFakeAvatarAck( void* arg );
	void __cdecl MsnFakeAck( void* arg );

	void __cdecl MsnGetAwayMsgThread( void* arg );
	void __cdecl MsnSendOim( void* arg );

	void __cdecl p2p_sendFeedThread( void* arg  );
	void __cdecl p2p_fileActiveThread( void* arg  );
	void __cdecl p2p_filePassiveThread( void* arg );

    /////////////////////////////////////////////////////////////////////////////////////////
    // MSN thread support

	void         Threads_Uninit( void );
    void		 MSN_CloseConnections( void );
    void		 MSN_CloseThreads( void );
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

    ThreadData*	 MSN_StartSB(HANDLE hContact, bool& isOffline);
	void __cdecl ThreadStub( void* arg );

    /////////////////////////////////////////////////////////////////////////////////////////
    // MSN message queue support

    int	     MsgQueue_Add( HANDLE hContact, int msgType, const char* msg, int msglen, filetransfer* ft = NULL, int flags = 0 );
    HANDLE	 MsgQueue_CheckContact( HANDLE hContact, time_t tsc = 0 );
    HANDLE	 MsgQueue_GetNextRecipient( void );
    bool	 MsgQueue_GetNext( HANDLE hContact, MsgQueueEntry& retVal );
    int	     MsgQueue_NumMsg( HANDLE hContact );
    void     MsgQueue_Clear( HANDLE hContact = NULL, bool msg = false );

    void MsgQueue_Init(void);
    void MsgQueue_Uninit(void);

    /////////////////////////////////////////////////////////////////////////////////////////
    // MSN message reassembly support

    OBJLIST<chunkedmsg> msgCache;

    int   addCachedMsg( const char* id, const char* msg, const size_t offset,
                        const size_t portion, const size_t totsz, const bool bychunk );
    bool  getCachedMsg( const int idx, char*& msg, size_t& size );
    bool  getCachedMsg( const char* id, char*& msg, size_t& size );
    void  clearCachedMsg( int idx = -1 );
    void  CachedMsg_Uninit( void );

    /////////////////////////////////////////////////////////////////////////////////////////
    // MSN P2P session support

    void  p2p_clearDormantSessions( void );
    void  p2p_cancelAllSessions( void );
    void  p2p_redirectSessions( HANDLE hContact );

    void  p2p_invite( HANDLE hContact, int iAppID, filetransfer* ft = NULL );
    void  p2p_processMsg( ThreadData* info, char* msgbody );
    void  p2p_processSIP( ThreadData* info, char* msgbody, void* hdr );
	
	void  p2p_AcceptTransfer( P2P_Header* hdrdata, MimeHeaders& tFileInfo, MimeHeaders& tFileInfo2 );
	void  p2p_InitDirectTransfer(P2P_Header* hdrdata, MimeHeaders& tFileInfo, MimeHeaders& tFileInfo2 );
	void  p2p_InitDirectTransfer2(P2P_Header* hdrdata, MimeHeaders& tFileInfo, MimeHeaders& tFileInfo2 );
	void  p2p_InitFileTransfer(P2P_Header* hdrdata, ThreadData* info, MimeHeaders& tFileInfo, MimeHeaders& tFileInfo2 );
	void  p2p_logHeader( P2P_Header* hdrdata );
	void  p2p_pictureTransferFailed(filetransfer* ft);
	void  p2p_savePicture2disk(filetransfer* ft);
    
	bool  p2p_createListener(filetransfer* ft, directconnection *dc, MimeHeaders& chdrs);
	void  p2p_startConnect(HANDLE hContact, const char* szCallID, const char* addr, const char* port);

	void  p2p_sendAbortSession( filetransfer* ft );
	void  p2p_sendAck( HANDLE hContact, P2P_Header* hdrdata );
    void  p2p_sendBye( filetransfer* ft );
    void  p2p_sendCancel( filetransfer* ft );
	void  p2p_sendMsg( HANDLE hContact, unsigned appId, P2P_Header& hdrdata, char* msgbody, size_t msgsz );
	void  p2p_sendNoCall( filetransfer* ft );
	void  p2p_sendSlp(filetransfer*	ft, MimeHeaders& pHeaders, int iKind, const char* szContent, size_t	szContLen );
    void  p2p_sendRedirect( filetransfer* ft );
	void  p2p_sendStatus( filetransfer* ft, long lStatus );

    void  p2p_sendFeedStart( filetransfer* ft );
	LONG  p2p_sendPortion( filetransfer* ft, ThreadData* T );
	void  p2p_sendRecvFileDirectly( ThreadData* info );
	bool  p2p_connectTo( ThreadData* info );
	bool  p2p_listen( ThreadData* info );

	void  p2p_registerSession( filetransfer* ft );
    void  p2p_unregisterSession( filetransfer* ft );
    void  p2p_sessionComplete( filetransfer* ft );

    void P2pSessions_Init( void );
    void P2pSessions_Uninit( void );

    filetransfer*  p2p_getAvatarSession( HANDLE hContact );
    filetransfer*  p2p_getThreadSession( HANDLE hContact, TInfoType mType );
    filetransfer*  p2p_getSessionByID( unsigned id );
    filetransfer*  p2p_getSessionByUniqueID( unsigned id );
    filetransfer*  p2p_getSessionByCallID( const char* CallID );

    bool     p2p_sessionRegistered( filetransfer* ft );
    bool     p2p_isAvatarOnly( HANDLE hContact );
    unsigned p2p_getMsgId( HANDLE hContact, int inc );

    void  p2p_registerDC( directconnection* ft );
    void  p2p_unregisterDC( directconnection* dc );
    directconnection*  p2p_getDCByCallID( const char* CallID );

    /////////////////////////////////////////////////////////////////////////////////////////
    // MSN MSNFTP file transfer

	void msnftp_invite( filetransfer *ft );
	void msnftp_sendAcceptReject( filetransfer *ft, bool acc );
    void msnftp_startFileSend( ThreadData* info, const char* Invcommand, const char* Invcookie );
	
	int  MSN_HandleMSNFTP( ThreadData *info, char *cmdString );

	void __cdecl msnftp_sendFileThread( void* arg );

    /////////////////////////////////////////////////////////////////////////////////////////
    //	MSN Chat support

    void MSN_ChatStart(ThreadData* info);
    void InviteUser(ThreadData* info);
    void MSN_KillChatSession(TCHAR* id);

    HANDLE MSN_GetChatInernalHandle(HANDLE hContact);

    int __cdecl MSN_GCEventHook(WPARAM wParam, LPARAM lParam);
    int __cdecl MSN_GCMenuHook(WPARAM wParam, LPARAM lParam);
    int __cdecl MSN_ChatInit( WPARAM wParam, LPARAM lParam );

    /////////////////////////////////////////////////////////////////////////////////////////
    //	MSN contact list

    int		 Lists_Add( int list, int netId, const char* email );
    bool	 Lists_IsInList( int list, const char* email );
    int		 Lists_GetMask( const char* email );
    int		 Lists_GetNetId( const char* email );
    void	 Lists_Remove( int list, const char* email );
    void	 Lists_Wipe( void );

    void     Lists_Init(void);
    void     Lists_Uninit(void);

    void     AddDelUserContList(const char* email, const int list, const int netId, const bool del);

    void	 MSN_CreateContList(void);
    void	 MSN_CleanupLists(void);
    void	 MSN_FindYahooUser(const char* email);
    bool	 MSN_RefreshContactList(void);

    bool	 MSN_IsMyContact( HANDLE hContact );
    bool	 MSN_IsMeByContact( HANDLE hContact, char* szEmail  = NULL );
    bool     MSN_AddUser( HANDLE hContact, const char* email, int netId, int flags );
    void     MSN_AddAuthRequest( HANDLE hContact, const char *email, const char *nick );
    void	 MSN_SetContactDb( HANDLE hContact, const char *szEmail );
    HANDLE	 MSN_HContactFromEmail( const char* msnEmail, const char* msnNick, bool addIfNeeded, bool temporary );
    HANDLE	 AddToListByEmail( const char *email, DWORD flags );

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
    //	MSN Authentication

    int		MSN_GetPassportAuth( void );
    char*	GenerateLoginBlob(char* challenge);
    char*	HotmailLogin(const char* url);
    void	FreeAuthTokens(void);

    /////////////////////////////////////////////////////////////////////////////////////////
    //	MSN Mail & Offline messaging support

    char oimDigest[64];
    char oimUID[64];
    unsigned oimMsgNum;

    int MSN_SendOIM(const char* szEmail, const char* msg);

	void getMetaData(void);
    void getOIMs(ezxml_t xmli);
    ezxml_t oimRecvHdr(const char* service, ezxml_t& tbdy, char*& httphdr);
    
    void processMailData(char* mailData);
    void sttNotificationMessage( char* msgBody, bool isInitial );


    /////////////////////////////////////////////////////////////////////////////////////////
    //	MSN SOAP Address Book

    bool MSN_SharingFindMembership(void);
    bool MSN_SharingAddDelMember(const char* szEmail, const int listId, const int netId, const char* szMethod);
    bool MSN_ABAdd(void);
    bool MSN_ABGetFull(void);
    bool MSN_ABAddDelContactGroup(const char* szCntId, const char* szGrpId, const char* szMethod);
    void MSN_ABAddGroup(const char* szGrpName);
    void MSN_ABRenameGroup(const char* szGrpName, const char* szGrpId);
    void MSN_ABUpdateNick(const char* szNick, const char* szCntId);
    void MSN_ABUpdateAttr(const char* szCntId, const char* szAttr, const char* szValue);
    void MSN_ABUpdateProperty(const char* szCntId, const char* propName, const char* propValue);
    unsigned MSN_ABContactAdd(const char* szEmail, const char* szNick, int netId, const bool search);
    void MSN_ABUpdateDynamicItem(void);

    ezxml_t abSoapHdr(const char* service, const char* scenario, ezxml_t& tbdy, char*& httphdr);
    char* GetABHost(const char* service, bool isSharing);
	void SetAbParam(HANDLE hContact, const char *name, const char *par);
    void UpdateABHost(const char* service, const char* url);

	char mycid[32];
	char mypuid[32];

	/////////////////////////////////////////////////////////////////////////////////////////
    //	MSN SOAP Roaming Storage

    bool MSN_StoreGetProfile(void);
    bool MSN_StoreUpdateNick(const char* szNick);
    bool MSN_StoreCreateProfile(void);
    bool MSN_StoreCreateRelationships(const char *szSrcId, const char *szTgtId);
    bool MSN_StoreDeleteRelationships(const char *szResId);
    bool MSN_StoreCreateDocument(const char *szName, const char *szMimeType, const char *szPicData);
    bool MSN_StoreFindDocuments(void);

    ezxml_t storeSoapHdr(const char* service, const char* scenario, ezxml_t& tbdy, char*& httphdr);
    char* GetStoreHost(const char* service);
    void UpdateStoreHost(const char* service, const char* url);

	char proresid[64];

    //////////////////////////////////////////////////////////////////////////////////////

    HANDLE CreateProtoEvent(const char* szEvent);
    void   CreateProtoService(const char* szService, MsnServiceFunc serviceProc);
    void   CreateProtoServiceParam(const char* szService, MsnServiceFuncParam serviceProc, LPARAM lParam);
    void   HookProtoEvent(const char* szEvent, MsnEventFunc pFunc);
	void   ForkThread( MsnThreadFunc pFunc, void* param );

    int    SendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam );

    void   deleteSetting( HANDLE hContact, const char* valueName );

    int    getByte( const char* name, BYTE defaultValue );
    int    getByte( HANDLE hContact, const char* name, BYTE defaultValue );
    int    getDword( const char* name, DWORD defaultValue );
    int    getDword( HANDLE hContact, const char* name, DWORD defaultValue );
    int    getStaticString( HANDLE hContact, const char* valueName, char* dest, unsigned dest_len );
    int    getString( const char* name, DBVARIANT* );
    int    getString( HANDLE hContact, const char* name, DBVARIANT* );
    int    getTString( const char* name, DBVARIANT* );
    int    getTString( HANDLE hContact, const char* name, DBVARIANT* );
    int    getWord( const char* name, WORD defaultValue );
    int    getWord( HANDLE hContact, const char* name, WORD defaultValue );

    void   setByte( const char* name, BYTE value );
    void   setByte( HANDLE hContact, const char* name, BYTE value );
    void   setDword( const char* name, DWORD value );
    void   setDword( HANDLE hContact, const char* name, DWORD value );
    void   setString( const char* name, const char* value );
    void   setString( HANDLE hContact, const char* name, const char* value );
    void   setStringUtf( HANDLE hContact, const char* name, const char* value );
    void   setTString( const char* name, const TCHAR* value );
    void   setTString( HANDLE hContact, const char* name, const TCHAR* value );
    void   setWord( const char* name, WORD value );
    void   setWord( HANDLE hContact, const char* name, WORD value );
};

extern OBJLIST<CMsnProto> g_Instances;

#endif
