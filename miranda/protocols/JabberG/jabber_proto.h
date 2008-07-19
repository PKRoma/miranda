/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
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

File name      : $URL: https://miranda.svn.sourceforge.net/svnroot/miranda/trunk/miranda/protocols/JabberG/jabber_list.h $
Revision       : $Revision: 6992 $
Last change on : $Date: 2007-12-29 03:47:51 +0300 (Сб, 29 дек 2007) $
Last change by : $Author: m_mluhov $

*/

#ifndef _JABBER_PROTO_H_
#define _JABBER_PROTO_H_

#include "jabber_disco.h"
#include "jabber_rc.h"
#include "jabber_privacy.h"
#include "jabber_search.h"
#include "jabber_iq.h"

struct CJabberProto;
typedef void ( __cdecl CJabberProto::*JThreadFunc )( void* );
typedef int  ( __cdecl CJabberProto::*JEventFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CJabberProto::*JServiceFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CJabberProto::*JServiceFuncParam )( WPARAM, LPARAM, LPARAM );

enum TJabberGcLogInfoType { INFO_BAN, INFO_STATUS, INFO_CONFIG, INFO_AFFILIATION, INFO_ROLE };

// for JabberEnterString
enum { JES_MULTINE, JES_COMBO, JES_RICHEDIT };

typedef UNIQUE_MAP<TCHAR,TCharKeyCmp> U_TCHAR_MAP;

#define JABBER_DEFAULT_RECENT_COUNT 10

#define NUM_XMODES 61

struct CJabberMoodIcon
{
	HANDLE	hItem;
	HANDLE	hIcon;
	HANDLE   hCListXStatusIcon;
};

struct JABBER_IQ_FUNC
{
	int iqId;                  // id to match IQ get/set with IQ result
	JABBER_IQ_PROCID procId;   // must be unique in the list, except for IQ_PROC_NONE which can have multiple entries
	JABBER_IQ_PFUNC func;      // callback function
	time_t requestTime;        // time the request was sent, used to remove relinquent entries
};

struct JABBER_GROUPCHAT_INVITE_INFO
{
	TCHAR* roomJid;
	TCHAR* from;
	TCHAR* reason;
	TCHAR* password;
};

struct ROSTERREQUSERDATA
{
	HWND hwndDlg;
	BYTE bRRAction;
	BOOL bReadyToDownload;
	BOOL bReadyToUpload;
};

struct TFilterInfo
{
	enum Type { T_JID, T_XMLNS, T_ANY, T_OFF };

	volatile BOOL msg, presence, iq;
	volatile Type type;

	CRITICAL_SECTION csPatternLock;
	TCHAR pattern[256];
};

struct CJabberProto : public PROTO_INTERFACE
{
	typedef PROTO_INTERFACE CSuper;

				CJabberProto( const char*, const TCHAR* );
				~CJabberProto();

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

	virtual	int    __cdecl Authorize( HANDLE hContact );
	virtual	int    __cdecl AuthDeny( HANDLE hContact, const char* szReason );
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

	virtual	int    __cdecl GetAwayMsg( HANDLE hContact );
	virtual	int    __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt );
	virtual	int    __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg );
	virtual	int    __cdecl SetAwayMsg( int m_iStatus, const char* msg );

	virtual	int    __cdecl UserIsTyping( HANDLE hContact, int type );

	virtual	int    __cdecl OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam );

	//====| Services |====================================================================
	int  __cdecl SvcCreateAccMgrUI(WPARAM wParam, LPARAM lParam);

	//====| Events |======================================================================
	void __cdecl OnAddContactForever( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	int  __cdecl OnContactDeleted( WPARAM, LPARAM );
	int  __cdecl OnDbSettingChanged( WPARAM, LPARAM );
	int  __cdecl OnIdleChanged( WPARAM, LPARAM );
	int  __cdecl OnModernOptInit( WPARAM, LPARAM );
	int  __cdecl OnModulesLoadedEx( WPARAM, LPARAM );
	int  __cdecl OnOptionsInit( WPARAM, LPARAM );
	int  __cdecl OnPreShutdown( WPARAM, LPARAM );
	int  __cdecl OnPrebuildContactMenu( WPARAM, LPARAM );
	int  __cdecl OnMsgUserTyping( WPARAM, LPARAM );
	int  __cdecl OnProcessSrmmIconClick( WPARAM, LPARAM );
	int  __cdecl OnProcessSrmmEvent( WPARAM, LPARAM );
	int  __cdecl OnReloadIcons( WPARAM, LPARAM );
	void __cdecl OnRenameContact( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	void __cdecl OnRenameGroup( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	int  __cdecl OnUserInfoInit( WPARAM, LPARAM );
	int  __cdecl OnBuildStatusMenu( WPARAM, LPARAM );

	int  __cdecl JabberGcEventHook( WPARAM, LPARAM );
	int  __cdecl JabberGcMenuHook( WPARAM, LPARAM );
	int  __cdecl JabberGcInit( WPARAM, LPARAM );

	int  __cdecl CListMW_ExtraIconsRebuild( WPARAM, LPARAM );
	int  __cdecl CListMW_ExtraIconsApply( WPARAM, LPARAM );

	//====| Data |========================================================================

	ThreadData* m_ThreadInfo;

	HANDLE m_hNetlibUser;
	PVOID  m_sslCtx;

	HANDLE m_hThreadHandle;

	TCHAR* m_szJabberJID;
	char*  m_szStreamId;
	DWORD  m_dwJabberLocalIP;
	BOOL   m_bJabberConnected;
	BOOL   m_bJabberOnline;
	int    m_nJabberSearchID;
	time_t m_tmJabberLoggedInTime;
	time_t m_tmJabberIdleStartTime;
	UINT   m_nJabberCodePage;

	CMString m_szCurrentEntityCapsHash;

	CRITICAL_SECTION m_csModeMsgMutex;
	JABBER_MODEMSGS m_modeMsgs;
	BOOL m_bModeMsgStatusChangePending;

	CJabberMoodIcon m_xstatusIcons[ NUM_XMODES + 1 ]; // moods + none
	HANDLE m_hHookExtraIconsRebuild;
	HANDLE m_hHookStatusBuild;
	HANDLE m_hHookExtraIconsApply;
	int    m_nJabberXStatus;
	BOOL   m_bXStatusMenuBuilt;

	BOOL   m_bChangeStatusMessageOnly;
	BOOL   m_bSendKeepAlive;
	BOOL   m_bPepSupported;

	HWND   m_hwndAgentRegInput;
	HWND   m_hwndRegProgress;
	HWND   m_hwndJabberChangePassword;
	HWND   m_hwndMucVoiceList;
	HWND   m_hwndMucMemberList;
	HWND   m_hwndMucModeratorList;
	HWND   m_hwndMucBanList;
	HWND   m_hwndMucAdminList;
	HWND   m_hwndMucOwnerList;
	HWND   m_hwndJabberAddBookmark;
	HWND   m_hwndPrivacyRule;

	CJabberDlgBase *m_pDlgPrivacyLists;
	CJabberDlgBase *m_pDlgBookmarks;
	CJabberDlgBase *m_pDlgServiceDiscovery;
	CJabberDlgBase *m_pDlgJabberJoinGroupchat;

	HANDLE m_windowList;

	// Service and event handles
	HANDLE m_hEventNudge;
	HANDLE m_hEventRawXMLIn;
	HANDLE m_hEventRawXMLOut;
	HANDLE m_hEventXStatusIconChanged;
	HANDLE m_hEventXStatusChanged;

	// Transports list
	LIST<TCHAR> m_lstTransports;

	XmlState m_xmlState;
	char *m_szXmlStreamToBeInitialized;

	CJabberIqManager m_iqManager;
	CJabberAdhocManager m_adhocManager;
	CJabberClientCapsManager m_clientCapsManager;
	CPrivacyListManager m_privacyListManager;
	CJabberSDManager m_SDManager;

	//HWND m_hwndConsole;
	CJabberDlgBase *m_pDlgConsole;
	HANDLE m_hThreadConsole;
	UINT m_dwConsoleThreadId;

	// proto frame
	CJabberInfoFrame *m_pInfoFrame;

	LIST<JABBER_LIST_ITEM> m_lstRoster;
	CRITICAL_SECTION m_csLists;
	BOOL m_bListInitialised;

	CRITICAL_SECTION m_csIqList;
	JABBER_IQ_FUNC *m_ppIqList;
	int m_nIqCount;
	int m_nIqAlloced;

	HANDLE m_hMenuRoot;
	HANDLE m_hMenuRequestAuth;
	HANDLE m_hMenuGrantAuth;
	HANDLE m_hMenuRevokeAuth;
	HANDLE m_hMenuJoinLeave;
	HANDLE m_hMenuConvert;
	HANDLE m_hMenuRosterAdd;
	HANDLE m_hMenuLogin;
	HANDLE m_hMenuRefresh;
	HANDLE m_hMenuAgent;
	HANDLE m_hMenuChangePassword;
	HANDLE m_hMenuGroupchat;
	HANDLE m_hMenuBookmarks;
	HANDLE m_hMenuAddBookmark;

	HANDLE m_hMenuCommands;
	HANDLE m_hMenuPrivacyLists;
	HANDLE m_hMenuRosterControl;
	HANDLE m_hMenuServiceDiscovery;
	HANDLE m_hMenuSDMyTransports;
	HANDLE m_hMenuSDTransports;
	HANDLE m_hMenuSDConferences;

	HANDLE m_hMenuResourcesRoot;
	HANDLE m_hMenuResourcesActive;
	HANDLE m_hMenuResourcesServer;

	HWND m_hwndCommandWindow;

	int m_nIqIdRegGetReg;
	int m_nIqIdRegSetReg;

	int m_nSDBrowseMode;
	DWORD m_dwSDLastRefresh;
	DWORD m_dwSDLastAutoDisco;

	int m_privacyMenuServiceAllocated;

	TFilterInfo m_filterInfo;

	/*******************************************************************
	* Function declarations
	*******************************************************************/

	void   JabberUpdateDialogs( BOOL bEnable );

	//---- jabber_adhoc.cpp --------------------------------------------------------------

	int    __cdecl ContactMenuRunCommands(WPARAM wParam, LPARAM lParam);

	HWND   GetWindowFromIq( XmlNode *iqNode );
	void   HandleAdhocCommandRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	BOOL   IsRcRequestAllowedByACL( CJabberIqInfo* pInfo );
		  
	int    AdhocSetStatusHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
	int    AdhocOptionsHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
	int    AdhocForwardHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
	int    AdhocLockWSHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
	int    AdhocQuitMirandaHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
	int    AdhocLeaveGroupchatsHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
		  
	void   OnIqResult_ListOfCommands( XmlNode *iqNode, void *userdata );
	void   OnIqResult_CommandExecution( XmlNode *iqNode, void *userdata );
	int    AdHoc_RequestListOfCommands( TCHAR * szResponder, HWND hwndDlg );
	int    AdHoc_ExecuteCommand( HWND hwndDlg, TCHAR * jid, struct JabberAdHocData* dat );
	int    AdHoc_SubmitCommandForm(HWND hwndDlg, JabberAdHocData * dat, char * action);
	int    AdHoc_AddCommandRadio(HWND hFrame, TCHAR * labelStr, int id, int ypos, int value);
	int    AdHoc_OnJAHMCommandListResult( HWND hwndDlg, XmlNode * iqNode, JabberAdHocData* dat );
	int    AdHoc_OnJAHMProcessResult( HWND hwndDlg, XmlNode *workNode, JabberAdHocData* dat );

	void   ContactMenuAdhocCommands( struct CJabberAdhocStartupParams* param );
	
	//---- jabber_bookmarks.c ------------------------------------------------------------

	int    __cdecl OnMenuHandleBookmarks( WPARAM wParam, LPARAM lParam );

	int    AddEditBookmark( JABBER_LIST_ITEM* item );

	//---- jabber_byte.c -----------------------------------------------------------------

	void   __cdecl ByteSendThread( JABBER_BYTE_TRANSFER *jbt );
	void   __cdecl ByteReceiveThread( JABBER_BYTE_TRANSFER *jbt );

	void   IqResultProxyDiscovery( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   ByteInitiateResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   ByteSendViaProxy( JABBER_BYTE_TRANSFER *jbt );
	int    ByteSendParse( HANDLE hConn, JABBER_BYTE_TRANSFER *jbt, char* buffer, int datalen );
	void   IqResultStreamActivate( XmlNode* iqNode, void* userdata );
	int    ByteReceiveParse( HANDLE hConn, JABBER_BYTE_TRANSFER *jbt, char* buffer, int datalen );
	int    ByteSendProxyParse( HANDLE hConn, JABBER_BYTE_TRANSFER *jbt, char* buffer, int datalen );

	//---- jabber_caps.cpp ---------------------------------------------------------------

	JabberCapsBits GetTotalJidCapabilites( TCHAR *jid );
	JabberCapsBits GetResourceCapabilites( TCHAR *jid, BOOL appendBestResource );

	//---- jabber_chat.cpp ---------------------------------------------------------------

	void   GcLogCreate( JABBER_LIST_ITEM* item );
	void   GcLogUpdateMemberStatus( JABBER_LIST_ITEM* item, TCHAR* nick, TCHAR* jid, int action, XmlNode* reason, int nStatusCode = -1 );
	void   GcLogShowInformation( JABBER_LIST_ITEM *item, JABBER_RESOURCE_STATUS *user, TJabberGcLogInfoType type );
	void   GcQuit( JABBER_LIST_ITEM* jid, int code, XmlNode* reason );
		  
	void   FilterList(HWND hwndList);
	void   ResetListOptions(HWND hwndList);
	void   InviteUser(TCHAR *room, TCHAR *pUser, TCHAR *text);
		  
	void   AdminSet( const TCHAR* to, const char* ns, const char* szItem, const TCHAR* itemVal, const char* var, const TCHAR* varVal );
	void   AdminGet( const TCHAR* to, const char* ns, const char* var, const TCHAR* varVal, JABBER_IQ_PFUNC foo );
	void   AddMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* str );
	void   DeleteMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* jid );

	//---- jabber_console.cpp ------------------------------------------------------------

	int    __cdecl OnMenuHandleConsole( WPARAM wParam, LPARAM lParam );
	void   __cdecl ConsoleThread( void* );

	void   ConsoleInit( void );
	void   ConsoleUninit( void );
	
	bool   FilterXml(XmlNode *node, DWORD flags);
	bool   RecursiveCheckFilter(XmlNode *node, DWORD flags);

	//---- jabber_disco.cpp --------------------------------------------------------------

	void   LaunchServiceDiscovery(TCHAR *jid);
	int    __cdecl OnMenuHandleServiceDiscovery( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuHandleServiceDiscoveryMyTransports( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuHandleServiceDiscoveryTransports( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuHandleServiceDiscoveryConferences( WPARAM wParam, LPARAM lParam );

	void   OnIqResultServiceDiscoveryInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqResultServiceDiscoveryItems( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqResultServiceDiscoveryRootInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqResultServiceDiscoveryRoot( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqResultServiceDiscoveryRootItems( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	BOOL   SendInfoRequest(CJabberSDNode* pNode, XmlNode* parent);
	BOOL   SendBothRequests(CJabberSDNode* pNode, XmlNode* parent);
	void   PerformBrowse(HWND hwndDlg);
	BOOL   IsNodeRegistered(CJabberSDNode *pNode);
	void   ApplyNodeIcon(HTREELISTITEM hItem, CJabberSDNode *pNode);
	BOOL   SyncTree(HTREELISTITEM hIndex, CJabberSDNode* pNode);
	void   ServiceDiscoveryShowMenu(CJabberSDNode *node, HTREELISTITEM hItem, POINT pt);
		  
	int    SetupServiceDiscoveryDlg( TCHAR* jid );
		  
	void   OnIqResultCapsDiscoInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
		  
	void   RegisterAgent( HWND hwndDlg, TCHAR* jid );

	//---- jabber_file.cpp ---------------------------------------------------------------

	int    FileReceiveParse( filetransfer* ft, char* buffer, int datalen );
	int    FileSendParse( JABBER_SOCKET s, filetransfer* ft, char* buffer, int datalen );
		  
	void   UpdateChatUserStatus( wchar_t* chat_jid, wchar_t* jid, wchar_t* nick, int role, int affil, int status, BOOL update_nick );
		  
	void   GroupchatJoinRoomByJid(HWND hwndParent, TCHAR *jid);
		  
	void   RenameParticipantNick( JABBER_LIST_ITEM* item, TCHAR* oldNick, XmlNode *itemNode );
	void   AcceptGroupchatInvite( TCHAR* roomJid, TCHAR* reason, TCHAR* password );

	//---- jabber_form.c -----------------------------------------------------------------

	void   FormCreateDialog( XmlNode *xNode, TCHAR* defTitle, JABBER_FORM_SUBMIT_FUNC pfnSubmit, void *userdata );
	
	//---- jabber_ft.c -------------------------------------------------------------------

	void   __cdecl FileReceiveThread( filetransfer* ft );
	void   __cdecl FileServerThread( filetransfer* ft );

	void   FtCancel( filetransfer* ft );
	void   FtInitiate( TCHAR* jid, filetransfer* ft );
	void   FtHandleSiRequest( XmlNode *iqNode );
	void   FtAcceptSiRequest( filetransfer* ft );
	void   FtAcceptIbbRequest( filetransfer* ft );
	void   FtHandleBytestreamRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	BOOL   FtHandleIbbRequest( XmlNode *iqNode, BOOL bOpen );
	
	//---- jabber_groupchat.c ------------------------------------------------------------

	int    __cdecl OnMenuHandleJoinGroupchat( WPARAM wParam, LPARAM lParam );
	void   __cdecl GroupchatInviteAcceptThread( JABBER_GROUPCHAT_INVITE_INFO *inviteInfo );

	void   GroupchatJoinRoom( const TCHAR* server, const TCHAR* room, const TCHAR* nick, const TCHAR* password, bool autojoin=false );
	void   GroupchatProcessPresence( XmlNode *node, void *userdata );
	void   GroupchatProcessMessage( XmlNode *node, void *userdata );
	void   GroupchatProcessInvite( TCHAR* roomJid, TCHAR* from, TCHAR* reason, TCHAR* password );
	void   GroupchatJoinDlg( TCHAR* roomJid );
	void   OnIqResultDiscovery(XmlNode *iqNode, void *userdata, CJabberIqInfo *pInfo);

	//---- jabber_icolib.cpp -------------------------------------------------------------

	void   IconsInit( void );
	HANDLE GetIconHandle( int iconId );
	HICON  LoadIconEx( const char* name );
	int    LoadAdvancedIcons(int iID);
	int    GetTransportStatusIconIndex(int iID, int Status);
	BOOL   DBCheckIsTransportedContact(const TCHAR* jid, HANDLE hContact);
	void   CheckAllContactsAreTransported( void );
	int    __cdecl JGetAdvancedStatusIcon(WPARAM wParam, LPARAM lParam );

	//---- jabber_iq.c -------------------------------------------------------------------

	JABBER_IQ_PFUNC JabberIqFetchFunc( int iqId );

	void   __cdecl ExpirerThread( void* );

	void   IqInit();
	void   IqUninit();
	void   IqAdd( unsigned int iqId, JABBER_IQ_PROCID procId, JABBER_IQ_PFUNC func );
	void   IqRemove( int index );
	void   IqExpire();
		  
	void   OnIqResultBind( XmlNode *iqNode, void *userdata );
	void   OnIqResultDiscoBookmarks( XmlNode *iqNode, void *userdata );
	void   OnIqResultSetBookmarks( XmlNode *iqNode, void *userdata );
	void   OnIqResultExtSearch( XmlNode *iqNode, void *userdata );
	void   OnIqResultGetAuth( XmlNode *iqNode, void *userdata );
	void   OnIqResultGetAvatar( XmlNode *iqNode, void *userdata );
	void   OnIqResultGetMuc( XmlNode *iqNode, void *userdata );
	void   OnIqResultGetRegister( XmlNode *iqNode, void *userdata );
	void   OnIqResultGetRoster( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   OnIqResultGetVcard( XmlNode *iqNode, void *userdata );
	void   OnIqResultMucGetAdminList( XmlNode *iqNode, void *userdata );
	void   OnIqResultMucGetBanList( XmlNode *iqNode, void *userdata );
	void   OnIqResultMucGetMemberList( XmlNode *iqNode, void *userdata );
	void   OnIqResultMucGetModeratorList( XmlNode *iqNode, void *userdata );
	void   OnIqResultMucGetOwnerList( XmlNode *iqNode, void *userdata );
	void   OnIqResultMucGetVoiceList( XmlNode *iqNode, void *userdata );
	void   OnIqResultNestedRosterGroups( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqResultSession( XmlNode *iqNode, void *userdata );
	void   OnIqResultSetAuth( XmlNode *iqNode, void *userdata );
	void   OnIqResultSetPassword( XmlNode *iqNode, void *userdata );
	void   OnIqResultSetRegister( XmlNode *iqNode, void *userdata );
	void   OnIqResultSetSearch( XmlNode *iqNode, void *userdata );
	void   OnIqResultSetVcard( XmlNode *iqNode, void *userdata );
	void   OnIqResultEntityTime( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   OnIqResultLastActivity( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   OnIqResultVersion( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   OnProcessLoginRq( ThreadData* info, DWORD rq );
	void   OnLoggedIn( ThreadData* info );

	//---- jabber_iq_handlers.cpp --------------------------------------------------------

	void   OnIqRequestVersion( XmlNode* node, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqRequestLastActivity( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   OnIqRequestPing( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   OnIqRequestTime( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   OnIqRequestAvatar( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   OnSiRequest( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   OnRosterPushRequest( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   OnIqRequestOOB( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   OnIqHttpAuth( XmlNode* node, void* userdata, CJabberIqInfo* pInfo );
	BOOL   AddClistHttpAuthEvent( CJabberHttpAuthParams *pParams );
		  
	void   __cdecl IbbSendThread( JABBER_IBB_TRANSFER *jibb );
	void   __cdecl IbbReceiveThread( JABBER_IBB_TRANSFER *jibb );

	void   OnIbbInitiateResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   OnIbbCloseResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   OnFtHandleIbbIq( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	BOOL   OnIbbRecvdData( TCHAR *data, TCHAR *sid, TCHAR *seq );
		  
	void   OnFtSiResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	BOOL   FtIbbSend( int blocksize, void *userdata );
	BOOL   FtSend( HANDLE hConn, void *userdata );
	void   FtSendFinal( BOOL success, void *userdata );
	int    FtReceive( HANDLE hConn, void *userdata, char* buffer, int datalen );
	void   FtReceiveFinal( BOOL success, void *userdata );

	//---- jabber_list.cpp ---------------------------------------------------------------

	JABBER_LIST_ITEM *ListAdd( JABBER_LIST list, const TCHAR* jid );
	JABBER_LIST_ITEM *ListGetItemPtr( JABBER_LIST list, const TCHAR* jid );
	JABBER_LIST_ITEM *ListGetItemPtrFromIndex( int index );

	void   ListWipe( void );
	int    ListExist( JABBER_LIST list, const TCHAR* jid );

	BOOL   ListLock();
	BOOL   ListUnlock();

	void   ListRemove( JABBER_LIST list, const TCHAR* jid );
	void   ListRemoveList( JABBER_LIST list );
	void   ListRemoveByIndex( int index );
	int    ListFindNext( JABBER_LIST list, int fromOffset );

	JABBER_RESOURCE_STATUS *CJabberProto::ListFindResource( JABBER_LIST list, const TCHAR* jid );
	int    ListAddResource( JABBER_LIST list, const TCHAR* jid, int status, const TCHAR* statusMessage, char priority = 0 );
	void   ListRemoveResource( JABBER_LIST list, const TCHAR* jid );
	TCHAR* ListGetBestResourceNamePtr( const TCHAR* jid );
	TCHAR* ListGetBestClientResourceNamePtr( const TCHAR* jid );

	void   SetMucConfig( XmlNode* node, void *from );
	void   OnIqResultMucGetJidList( XmlNode *iqNode, JABBER_MUC_JIDLIST_TYPE listType );
		  
	void   OnIqResultServerDiscoInfo( XmlNode* iqNode, void* userdata );
	void   OnIqResultGetVcardPhoto( const TCHAR* jid, XmlNode* n, HANDLE hContact, BOOL& hasPhoto );
	void   SetBookmarkRequest (XmlNodeIq& iqId);

	//---- jabber_menu.cpp ---------------------------------------------------------------

	int    __cdecl OnMenuConvertChatContact( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuRosterAdd( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuHandleRequestAuth( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuHandleGrantAuth( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuJoinLeave( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuTransportLogin( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuTransportResolve( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuBookmarkAdd( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuRevokeAuth( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnMenuHandleResource(WPARAM wParam, LPARAM lParam, LPARAM res);
	int    __cdecl OnMenuSetPriority(WPARAM wParam, LPARAM lParam, LPARAM dwDelta);

	void   MenuInit( void );
	void   MenuUninit( void );

	void   MenuHideSrmmIcon(HANDLE hContact);
	void   MenuUpdateSrmmIcon(JABBER_LIST_ITEM *item);
	
	void   AuthWorker( HANDLE hContact, char* authReqType );

	void   BuildPriorityMenu(WPARAM, LPARAM);
	void   UpdatePriorityMenu(short priority);

	HANDLE m_hMenuPriorityRoot;
	short  m_priorityMenuVal;
	bool   m_priorityMenuValSet;

	//---- jabber_misc.c -----------------------------------------------------------------

	int    __cdecl OnGetEventTextChatStates( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnGetEventTextPresence( WPARAM wParam, LPARAM lParam );

	void   AddContactToRoster( const TCHAR* jid, const TCHAR* nick, const TCHAR* grpName );
	void   DBAddAuthRequest( TCHAR* jid, TCHAR* nick );
	BOOL   AddDbPresenceEvent(HANDLE hContact, BYTE btEventType);
	HANDLE DBCreateContact( TCHAR* jid, TCHAR* nick, BOOL temporary, BOOL stripResource );
	void   GetAvatarFileName( HANDLE hContact, char* pszDest, int cbLen );
	void   ResolveTransportNicks( TCHAR* jid );
	void   SetServerStatus( int iNewStatus );
	void   FormatMirVer(JABBER_RESOURCE_STATUS *resource, TCHAR *buf, int bufSize);
	void   UpdateMirVer(JABBER_LIST_ITEM *item);
	void   UpdateMirVer(HANDLE hContact, JABBER_RESOURCE_STATUS *resource);
	void   SetContactOfflineStatus( HANDLE hContact );
	void   InitCustomFolders( void );

	//---- jabber_opt.cpp ----------------------------------------------------------------

	CJabberDlgBase::CreateParam		OptCreateAccount;
	CJabberDlgBase::CreateParam		OptCreateGc;
	CJabberDlgBase::CreateParam		OptCreateAdvanced;

	int    __cdecl OnMenuHandleRosterControl( WPARAM wParam, LPARAM lParam );

	void   _RosterExportToFile(HWND hwndDlg);
	void   _RosterImportFromFile(HWND hwndDlg);
	void   _RosterSendRequest(HWND hwndDlg, BYTE rrAction);
	void   _RosterHandleGetRequest( XmlNode* node, void* userdata );

	//---- jabber_password.cpp --------------------------------------------------------------
	
	int    __cdecl OnMenuHandleChangePassword( WPARAM wParam, LPARAM lParam );

	//---- jabber_privacy.cpp ------------------------------------------------------------
	ROSTERREQUSERDATA rrud;

	int    __cdecl menuSetPrivacyList( WPARAM wParam, LPARAM lParam, LPARAM iList );
	int    __cdecl OnMenuHandlePrivacyLists( WPARAM wParam, LPARAM lParam );

	void   BuildPrivacyMenu( WPARAM, LPARAM );

	void   QueryPrivacyLists( ThreadData *pThreadInfo = NULL );

	void   OnIqRequestPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqResultPrivacyList( XmlNode* iqNode, void* userdata );
	void   OnIqResultPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqResultPrivacyListActive( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqResultPrivacyListDefault( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnIqResultPrivacyListModify( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );

	//---- jabber_proto.cpp --------------------------------------------------------------

	void   __cdecl BasicSearchThread( struct JABBER_SEARCH_BASIC *jsb );
	void   __cdecl GetAwayMsgThread( void* hContact );
	void   __cdecl SendMessageAckThread( void* hContact );

	HANDLE AddToListByJID( const TCHAR* newJid, DWORD flags );
	void   WindowSubscribe(HWND hwnd);
	void   WindowUnsubscribe(HWND hwnd);
	void   WindowNotify(UINT msg, bool async = false);

	void   InfoFrame_OnSetup(CJabberInfoFrame_Event *evt);
	void   InfoFrame_OnTransport(CJabberInfoFrame_Event *evt);

	//---- jabber_rc.cpp -----------------------------------------------------------------

	int    RcGetUnreadEventsCount( void );

	//---- jabber_search.cpp -------------------------------------------------------------

	void   SearchReturnResults( HANDLE id, void* pvUsersInfo, U_TCHAR_MAP* pmAllFields );
	void   OnIqResultAdvancedSearch( XmlNode *iqNode, void *userdata );
	void   OnIqResultGetSearchFields( XmlNode *iqNode, void *userdata );
	int    SearchRenewFields( HWND hwndDlg, JabberSearchData * dat);
	void   SearchDeleteFromRecent( TCHAR* szAddr, BOOL deleteLastFromDB = TRUE );
	void   SearchAddToRecent( TCHAR* szAddr, HWND hwndDialog = NULL );

	//---- jabber_std.cpp ----------------------------------------------

	void   JCreateService( const char* szService, JServiceFunc serviceProc );
	void   JCreateServiceParam( const char* szService, JServiceFuncParam serviceProc, LPARAM lParam );
	HANDLE JCreateHookableEvent( const char* szService );
	void   JForkThread( JThreadFunc, void* );
	HANDLE JForkThreadEx( JThreadFunc, void*, UINT* threadID = NULL );

	void   JDeleteSetting( HANDLE hContact, const char* valueName );
	DWORD  JGetByte( const char* valueName, int parDefltValue );
	DWORD  JGetByte( HANDLE hContact, const char* valueName, int parDefltValue );
	char*  JGetContactName( HANDLE hContact );
	DWORD  JGetDword( HANDLE hContact, const char* valueName, DWORD parDefltValue );
	int    JGetStaticString( const char* valueName, HANDLE hContact, char* dest, int dest_len );
	int    JGetStringUtf( HANDLE hContact, char* valueName, DBVARIANT* dbv );
	int    JGetStringT( HANDLE hContact, char* valueName, DBVARIANT* dbv );
	WORD   JGetWord( HANDLE hContact, const char* valueName, int parDefltValue );
	void   JHookEvent( const char*, JEventFunc );
	int    JSendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam );
	DWORD  JSetByte( const char* valueName, int parValue );
	DWORD  JSetByte( HANDLE hContact, const char* valueName, int parValue );
	DWORD  JSetDword( HANDLE hContact, const char* valueName, DWORD parValue );
	DWORD  JSetString( HANDLE hContact, const char* valueName, const char* parValue );
	DWORD  JSetStringT( HANDLE hContact, const char* valueName, const TCHAR* parValue );
	DWORD  JSetStringUtf( HANDLE hContact, const char* valueName, const char* parValue );
	DWORD  JSetWord( HANDLE hContact, const char* valueName, int parValue );

	TCHAR* JGetStringCrypt( HANDLE hContact, char* valueName );
	DWORD  JSetStringCrypt( HANDLE hContact, char* valueName, const TCHAR* parValue );

	//---- jabber_svc.c ------------------------------------------------------------------

	void   EnableMenuItems( BOOL bEnable );

	int    __cdecl JabberGetName( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberGetAvatar( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberGetAvatarCaps( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberGetAvatarInfo( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberGetStatus( WPARAM wParam, LPARAM lParam );
	int    __cdecl ServiceSendXML( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberSetAvatar( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberSendNudge( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberGCGetToolTipText( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberServiceParseXmppURI( WPARAM wParam, LPARAM lParam );
	int    __cdecl OnHttpAuthRequest( WPARAM wParam, LPARAM lParam );

	BOOL SendHttpAuthReply( CJabberHttpAuthParams *pParams, BOOL bAuthorized );

	//---- jabber_thread.c ----------------------------------------------

	void   __cdecl ServerThread( ThreadData* info );
		  
	void   OnProcessFailure( XmlNode *node, void *userdata );
	void   OnProcessError( XmlNode *node, void *userdata );
	void   OnProcessSuccess( XmlNode *node, void *userdata );
	void   OnProcessChallenge( XmlNode *node, void *userdata );
	void   OnProcessProceed( XmlNode *node, void *userdata );	
	void   OnProcessCompressed( XmlNode *node, void *userdata );
	void   OnProcessMessage( XmlNode *node, void *userdata );
	void   OnProcessPresence( XmlNode *node, void *userdata );
	void   OnProcessPresenceCapabilites( XmlNode *node );
	void   OnProcessPubsubEvent( XmlNode *node );

	void   UpdateJidDbSettings( TCHAR *jid );
	HANDLE CreateTemporaryContact( TCHAR *szJid, JABBER_LIST_ITEM* chatItem );

	void   PerformRegistration( ThreadData* info );
	void   PerformIqAuth( ThreadData* info );
	void   OnProcessFeatures( XmlNode *node, void *userdata );

	void   xmlStreamInitialize( char *which );
	void   xmlStreamInitializeNow(ThreadData* info);

	void   OnProcessIq( XmlNode *node, void *userdata );
	void   OnProcessRegIq( XmlNode *node, void *userdata );
	void   OnPingReply( XmlNode* node, void* userdata, CJabberIqInfo* pInfo );

	//---- jabber_util.c -----------------------------------------------------------------

	JABBER_RESOURCE_STATUS* ResourceInfoFromJID( TCHAR* jid );

	void   SerialInit( void );
	void   SerialUninit( void );
	int    SerialNext( void );

	HANDLE HContactFromJID( const TCHAR* jid , BOOL bStripResource = 3);
	HANDLE ChatRoomHContactFromJID( const TCHAR* jid );
	void   Log( const char* fmt, ... );
	void   SendVisibleInvisiblePresence( BOOL invisible );
	void   SendPresenceTo( int status, TCHAR* to, XmlNode* extra );
	void   SendPresence( int m_iStatus, bool bSendToAll );
	void   StringAppend( char* *str, int *sizeAlloced, const char* fmt, ... );
	TCHAR* GetClientJID( const TCHAR* jid, TCHAR*, size_t );
	TCHAR* GetXmlLang( void );
	void   RebuildStatusMenu( void );
	void   RebuildInfoFrame( void );

	void   ComboLoadRecentStrings(HWND hwndDlg, UINT idcCombo, char *param, int recentCount=JABBER_DEFAULT_RECENT_COUNT);
	void   ComboAddRecentString(HWND hwndDlg, UINT idcCombo, char *param, TCHAR *string, int recentCount=JABBER_DEFAULT_RECENT_COUNT);
	BOOL   EnterString(TCHAR *result, size_t resultLen, TCHAR *caption=NULL, int type=0, char *windowName=NULL, int recentCount=JABBER_DEFAULT_RECENT_COUNT, int timeout=0);

	//---- jabber_vcard.c -----------------------------------------------

	int    m_vCardUpdates;
	HWND   m_hwndPhoto;
	bool   m_bPhotoChanged;
	char   m_szPhotoFileName[MAX_PATH];
	void   OnUserInfoInit_VCard( WPARAM, LPARAM );

	void   GroupchatJoinByHContact( HANDLE hContact, bool autojoin=false );
	int    SendGetVcard( const TCHAR* jid );
	void   AppendVcardFromDB( XmlNode* n, char* tag, char* key );
	void   SetServerVcard( BOOL bPhotoChanged, char* szPhotoFileName );
	void   SaveVcardToDB( HWND hwndPage, int iPage );

	//---- jabber_ws.c -------------------------------------------------

	JABBER_SOCKET WsConnect( char* host, WORD port );

	BOOL   WsInit(void);
	void   WsUninit(void);
	int    WsSend( JABBER_SOCKET s, char* data, int datalen, int flags );
	int    WsRecv( JABBER_SOCKET s, char* data, long datalen, int flags );

	//---- jabber_xml.c ------------------------------------------------------------------

	int    OnXmlParse( XmlState *xmlState, char* buffer );
	void   OnConsoleProcessXml(XmlNode *node, DWORD flags);

	//---- jabber_xmlns.c ----------------------------------------------------------------

	void   OnHandleDiscoInfoRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   OnHandleDiscoItemsRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );

	//---- jabber_xstatus.c --------------------------------------------------------------

	int    __cdecl OnSetListeningTo( WPARAM wParam, LPARAM lParams );
	int    __cdecl OnGetXStatusIcon( WPARAM wParam, LPARAM lParams );
	int    __cdecl OnGetXStatus( WPARAM wParam, LPARAM lParams );
	int    __cdecl OnSetXStatus( WPARAM wParam, LPARAM lParams );
	int    __cdecl OnMenuSetXStatus( WPARAM wParam, LPARAM lParam, LPARAM param );

	void BuildXStatusItems( WPARAM, LPARAM );

	HICON  GetXStatusIcon(int bStatus, UINT flags);
	void   JabberUpdateContactExtraIcon( HANDLE hContact );

	void   RegisterAdvStatusSlot(const char *pszSlot);
	void   ResetAdvStatus(HANDLE hContact, const char *pszSlot);
	void   WriteAdvStatus(HANDLE hContact, const char *pszSlot, const char *pszMode, const char *pszIcon, const TCHAR *pszTitle, const TCHAR *pszText);
	char*  ReadAdvStatusA(HANDLE hContact, const char *pszSlot, const char *pszValue);
	TCHAR* ReadAdvStatusT(HANDLE hContact, const char *pszSlot, const char *pszValue);

	BOOL   SendPepActivity( char* szFirstNode, char* szSecondNode, TCHAR* szText );
	BOOL   SendPepMood( int nMoodNumber, TCHAR* szMoodText );
	BOOL   SendPepTune( TCHAR* szArtist, TCHAR* szLength, TCHAR* szSource, TCHAR* szTitle, TCHAR* szTrack, TCHAR* szUri );
			
	void   XStatusInit( void );
	void   XStatusUninit( void );
	void   InitXStatusIcons( void );
			
	void   SetContactMood( HANDLE hContact, const char* moodName, const TCHAR* moodText );
	void   SetContactTune( HANDLE hContact,  TCHAR* szArtist, TCHAR* szLength, TCHAR* szSource, TCHAR* szTitle, TCHAR* szTrack, TCHAR* szUri );
	void   SetContactActivity( HANDLE hContact, char* szFirstNode, char* szSecondNode, TCHAR* szText );

	void InfoFrame_OnUserMood(CJabberInfoFrame_Event *evt);
	void InfoFrame_OnUserActivity(CJabberInfoFrame_Event *evt);

	int m_xsActivity;

private:
	XmlState xmlState;
	char*    xmlStreamToBeInitialized;

	CRITICAL_SECTION m_csSerial;
	unsigned int m_nSerial;

	HANDLE m_hInitChat;
	HANDLE m_hPrebuildStatusMenu;
	
	int     m_nMenuResourceItems;
	HANDLE* m_phMenuResourceItems;

	HANDLE* m_phIconLibItems;
};

extern LIST<CJabberProto> g_Instances;

#endif
