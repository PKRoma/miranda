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

#include "m_protoint.h"

#include "jabber_disco.h"
#include "jabber_rc.h"
#include "jabber_privacy.h"
#include "jabber_search.h"
#include "jabber_iq.h"

struct CJabberProto;
typedef int ( __cdecl CJabberProto::*JEventFunc )( WPARAM, LPARAM );
typedef int ( __cdecl CJabberProto::*JServiceFunc )( WPARAM, LPARAM );
typedef int ( __cdecl CJabberProto::*JServiceFuncParam )( WPARAM, LPARAM, LPARAM );

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

struct ROSTERREQUSERDATA
{
	HWND hwndDlg;
	BYTE bRRAction;
	BOOL bReadyToDownload;
	BOOL bReadyToUpload;
};

struct CJabberProto : public PROTO_INTERFACE
{
				CJabberProto( const char* );
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

	virtual	DWORD  __cdecl GetCaps( int type );
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

	//====| Events |======================================================================
	void __cdecl OnAddContactForever( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	int  __cdecl OnContactDeleted( WPARAM, LPARAM );
	int  __cdecl OnDbSettingChanged( WPARAM, LPARAM );
	int  __cdecl OnIdleChanged( WPARAM, LPARAM );
	int  __cdecl OnModernOptInit( WPARAM, LPARAM );
	int  __cdecl OnModernToolbarInit( WPARAM, LPARAM );
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

	int  __cdecl JabberGcEventHook( WPARAM, LPARAM );
	int  __cdecl JabberGcMenuHook( WPARAM, LPARAM );
	int  __cdecl JabberGcInit( WPARAM, LPARAM );

	int  __cdecl CListMW_ExtraIconsRebuild( WPARAM, LPARAM );
	int  __cdecl CListMW_ExtraIconsApply( WPARAM, LPARAM );
	int  __cdecl CListMW_BuildStatusItems( WPARAM, LPARAM );
	int  __cdecl OnBuildPrivacyMenu( WPARAM, LPARAM );

	//====| Data |========================================================================

	HANDLE hNetlibUser;

	ThreadData* jabberThreadInfo;
	HANDLE      jabberThreadHandle;

	TCHAR* jabberJID;
	char*  streamId;
	DWORD  jabberLocalIP;
	BOOL   jabberConnected;
	BOOL   jabberOnline;
	int    jabberSearchID;
	time_t jabberLoggedInTime;
	time_t jabberIdleStartTime;
	UINT   jabberCodePage;

	CRITICAL_SECTION modeMsgMutex;
	JABBER_MODEMSGS modeMsgs;
	BOOL modeMsgStatusChangePending;

	CJabberMoodIcon xstatusIcons[ NUM_XMODES + 1 ]; // moods + none
	HANDLE hHookExtraIconsRebuild;
	HANDLE hHookStatusBuild;
	HANDLE hHookExtraIconsApply;
	int    jabberXStatus;
	BOOL   bXStatusMenuBuilt;

	BOOL   jabberChangeStatusMessageOnly;
	BOOL   jabberSendKeepAlive;
	BOOL   jabberPepSupported;
	BOOL   jabberChatDllPresent;

	HWND   hwndJabberAgents;
	HWND   hwndAgentReg;
	HWND   hwndAgentRegInput;
	HWND   hwndAgentManualReg;
	HWND   hwndRegProgress;
	HWND   hwndJabberVcard;
	HWND   hwndJabberChangePassword;
	HWND   hwndJabberGroupchat;
	HWND   hwndJabberJoinGroupchat;
	HWND   hwndMucVoiceList;
	HWND   hwndMucMemberList;
	HWND   hwndMucModeratorList;
	HWND   hwndMucBanList;
	HWND   hwndMucAdminList;
	HWND   hwndMucOwnerList;
	HWND   hwndJabberBookmarks;
	HWND   hwndJabberAddBookmark;
	HWND   hwndJabberInfo;
	HWND   hwndPrivacyRule;
	HWND   hwndServiceDiscovery;

	CJabberDlgPrivacyLists *dlgPrivacyLists;

	// Service and event handles
	HANDLE heventNudge;
	HANDLE heventRawXMLIn;
	HANDLE heventRawXMLOut;
	HANDLE heventXStatusIconChanged;
	HANDLE heventXStatusChanged;

	// Transports list
	LIST<TCHAR> jabberTransports;

	XmlState xmlState;
	char *xmlStreamToBeInitialized;

	CJabberIqManager m_iqManager;
	CJabberAdhocManager m_adhocManager;
	CJabberClientCapsManager m_clientCapsManager;
	CPrivacyListManager m_privacyListManager;
	CJabberSDManager m_SDManager;

	HWND hwndJabberConsole;
	HANDLE hThreadConsole;
	UINT sttJabberConsoleThreadId;

	LIST<JABBER_LIST_ITEM> roster;
	CRITICAL_SECTION csLists;
	BOOL LIST_INITIALISED;

	CRITICAL_SECTION csIqList;
	JABBER_IQ_FUNC *iqList;
	int iqCount;
	int iqAlloced;
	
	char* jabberVcardPhotoFileName;

	TCHAR tszPhotoFileName[MAX_PATH];
	BOOL bPhotoChanged;

	HANDLE hMenuRoot;
	HANDLE hMenuRequestAuth;
	HANDLE hMenuGrantAuth;
	HANDLE hMenuRevokeAuth;
	HANDLE hMenuJoinLeave;
	HANDLE hMenuConvert;
	HANDLE hMenuRosterAdd;
	HANDLE hMenuLogin;
	HANDLE hMenuRefresh;
	HANDLE hMenuAgent;
	HANDLE hMenuChangePassword;
	HANDLE hMenuGroupchat;
	HANDLE hMenuBookmarks;
	HANDLE hMenuAddBookmark;

	HANDLE hMenuCommands;
	HANDLE hMenuPrivacyLists;
	HANDLE hMenuRosterControl;
	HANDLE hMenuServiceDiscovery;
	HANDLE hMenuSDMyTransports;
	HANDLE hMenuSDTransports;
	HANDLE hMenuSDConferences;

	HANDLE hMenuResourcesRoot;
	HANDLE hMenuResourcesActive;
	HANDLE hMenuResourcesServer;

	HWND hwndCommandWindow;

	int iqIdRegGetReg;
	int iqIdRegSetReg;

	int sttBrowseMode;
	DWORD sttLastRefresh;
	DWORD sttLastAutoDisco;

	/*******************************************************************
	* Function declarations
	*******************************************************************/

	void   JabberUpdateDialogs( BOOL bEnable );

	//---- jabber_adhoc.cpp --------------------------------------------------------------

	int    __cdecl JabberContactMenuRunCommands(WPARAM wParam, LPARAM lParam);

	HWND   sttGetWindowFromIq( XmlNode *iqNode );
	void   JabberHandleAdhocCommandRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	BOOL   IsRcRequestAllowedByACL( CJabberIqInfo* pInfo );
		  
	int    JabberAdhocSetStatusHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
	int    JabberAdhocOptionsHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
	int    JabberAdhocForwardHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
	int    JabberAdhocLockWSHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
	int    JabberAdhocQuitMirandaHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession );
		  
	void   JabberIqResult_ListOfCommands( XmlNode *iqNode, void *userdata );
	void   JabberIqResult_CommandExecution( XmlNode *iqNode, void *userdata );
	int    JabberAdHoc_RequestListOfCommands( TCHAR * szResponder, HWND hwndDlg );
	int    JabberAdHoc_ExecuteCommand( HWND hwndDlg, TCHAR * jid, struct JabberAdHocData* dat );
	int    JabberAdHoc_SubmitCommandForm(HWND hwndDlg, JabberAdHocData * dat, char * action);
	int    JabberAdHoc_AddCommandRadio(HWND hFrame, TCHAR * labelStr, int id, int ypos, int value);
	int    JabberAdHoc_OnJAHMCommandListResult( HWND hwndDlg, XmlNode * iqNode, JabberAdHocData* dat );
	int    JabberAdHoc_OnJAHMProcessResult( HWND hwndDlg, XmlNode *workNode, JabberAdHocData* dat );

	void   JabberContactMenuAdhocCommands( struct CJabberAdhocStartupParams* param );
	
	//---- jabber_bookmarks.c ------------------------------------------------------------

	int    __cdecl JabberMenuHandleBookmarks( WPARAM wParam, LPARAM lParam );

	int    JabberAddEditBookmark( JABBER_LIST_ITEM* item );

	//---- jabber_byte.c -----------------------------------------------------------------

	void   JabberByteSendThread( JABBER_BYTE_TRANSFER *jbt );
	void   JabberByteReceiveThread( JABBER_BYTE_TRANSFER *jbt );
	void   JabberIqResultProxyDiscovery( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberByteInitiateResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   JabberByteSendViaProxy( JABBER_BYTE_TRANSFER *jbt );
	int    JabberByteSendParse( HANDLE hConn, JABBER_BYTE_TRANSFER *jbt, char* buffer, int datalen );
	void   JabberIqResultStreamActivate( XmlNode* iqNode, void* userdata );
	int    JabberByteReceiveParse( HANDLE hConn, JABBER_BYTE_TRANSFER *jbt, char* buffer, int datalen );
	int    JabberByteSendProxyParse( HANDLE hConn, JABBER_BYTE_TRANSFER *jbt, char* buffer, int datalen );

	//---- jabber_caps.cpp ---------------------------------------------------------------

	JabberCapsBits JabberGetTotalJidCapabilites( TCHAR *jid );
	JabberCapsBits JabberGetResourceCapabilites( TCHAR *jid, BOOL appendBestResource );

	//---- jabber_chat.cpp ---------------------------------------------------------------

	void   JabberGcLogCreate( JABBER_LIST_ITEM* item );
	void   JabberGcLogUpdateMemberStatus( JABBER_LIST_ITEM* item, TCHAR* nick, TCHAR* jid, int action, XmlNode* reason, int nStatusCode = -1 );
	void   JabberGcLogShowInformation( JABBER_LIST_ITEM *item, JABBER_RESOURCE_STATUS *user, enum TJabberGcLogInfoType type );
	void   JabberGcQuit( JABBER_LIST_ITEM* jid, int code, XmlNode* reason );
		  
	void   FilterList(HWND hwndList);
	void   ResetListOptions(HWND hwndList);
	void   InviteUser(TCHAR *room, TCHAR *pUser, TCHAR *text);
		  
	void   JabberAdminSet( const TCHAR* to, const char* ns, const char* szItem, const TCHAR* itemVal, const char* var, const TCHAR* varVal );
	void   JabberAdminGet( const TCHAR* to, const char* ns, const char* var, const TCHAR* varVal, JABBER_IQ_PFUNC foo );
	void   JabberAddMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* str );
	void   JabberDeleteMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* jid );

	//---- jabber_console.cpp ------------------------------------------------------------

	int    __cdecl JabberMenuHandleConsole( WPARAM wParam, LPARAM lParam );

	void   JabberConsoleInit( void );
	void   JabberConsoleUninit( void );

	//---- jabber_disco.cpp --------------------------------------------------------------

	int    __cdecl JabberMenuHandleServiceDiscovery( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuHandleServiceDiscoveryMyTransports( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuHandleServiceDiscoveryTransports( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuHandleServiceDiscoveryConferences( WPARAM wParam, LPARAM lParam );

	void   JabberIqResultServiceDiscoveryInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultServiceDiscoveryItems( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultServiceDiscoveryRootInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultServiceDiscoveryRoot( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultServiceDiscoveryRootItems( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	BOOL   SendInfoRequest(CJabberSDNode* pNode, XmlNode* parent);
	BOOL   SendBothRequests(CJabberSDNode* pNode, XmlNode* parent);
	void   sttPerformBrowse(HWND hwndDlg);
	BOOL   sttIsNodeRegistered(CJabberSDNode *pNode);
	void   sttApplyNodeIcon(HTREELISTITEM hItem, CJabberSDNode *pNode);
	BOOL   SyncTree(HTREELISTITEM hIndex, CJabberSDNode* pNode);
	void   JabberServiceDiscoveryShowMenu(CJabberSDNode *node, HTREELISTITEM hItem, POINT pt);
		  
	int    JabberSetupServiceDiscoveryDlg( TCHAR* jid );
		  
	void   JabberIqResultCapsDiscoInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
		  
	void   JabberRegisterAgent( HWND hwndDlg, TCHAR* jid );

	//---- jabber_file.cpp ---------------------------------------------------------------

	int    JabberFileReceiveParse( filetransfer* ft, char* buffer, int datalen );
	int    JabberFileSendParse( JABBER_SOCKET s, filetransfer* ft, char* buffer, int datalen );
		  
	void   JabberUpdateChatUserStatus( wchar_t* chat_jid, wchar_t* jid, wchar_t* nick, int role, int affil, int status, BOOL update_nick );
		  
	void   JabberGroupchatJoinRoomByJid(HWND hwndParent, TCHAR *jid);
		  
	void   JabberRenameParticipantNick( JABBER_LIST_ITEM* item, TCHAR* oldNick, XmlNode *itemNode );
	void   JabberAcceptGroupchatInvite( TCHAR* roomJid, TCHAR* reason, TCHAR* password );

	//---- jabber_form.c -----------------------------------------------------------------

	void   JabberFormCreateDialog( XmlNode *xNode, TCHAR* defTitle, JABBER_FORM_SUBMIT_FUNC pfnSubmit, void *userdata );
	
	//---- jabber_ft.c -------------------------------------------------------------------

	void   JabberFtCancel( filetransfer* ft );
	void   JabberFtInitiate( TCHAR* jid, filetransfer* ft );
	void   JabberFtHandleSiRequest( XmlNode *iqNode );
	void   JabberFtAcceptSiRequest( filetransfer* ft );
	void   JabberFtAcceptIbbRequest( filetransfer* ft );
	void   JabberFtHandleBytestreamRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	BOOL   JabberFtHandleIbbRequest( XmlNode *iqNode, BOOL bOpen );
	void   JabberFileReceiveThread( filetransfer* ft );
	void   JabberFileServerThread( filetransfer* ft );
	
	//---- jabber_groupchat.c ------------------------------------------------------------

	int    __cdecl JabberMenuHandleJoinGroupchat( WPARAM wParam, LPARAM lParam );

	void   JabberGroupchatJoinRoom( const TCHAR* server, const TCHAR* room, const TCHAR* nick, const TCHAR* password );
	void   JabberGroupchatProcessPresence( XmlNode *node, void *userdata );
	void   JabberGroupchatProcessMessage( XmlNode *node, void *userdata );
	void   JabberGroupchatProcessInvite( TCHAR* roomJid, TCHAR* from, TCHAR* reason, TCHAR* password );
	void   JabberGroupchatJoinDlg( TCHAR* roomJid );
	void   JabberIqResultDiscovery(XmlNode *iqNode, void *userdata, CJabberIqInfo *pInfo);

	//---- jabber_icolib.cpp -------------------------------------------------------------

	void   JabberIconsInit( void );
	HANDLE GetIconHandle( int iconId );
	HICON  LoadIconEx( const char* name );
	int    LoadAdvancedIcons(int iID);
	int    GetTransportStatusIconIndex(int iID, int Status);
	BOOL   JabberDBCheckIsTransportedContact(const TCHAR* jid, HANDLE hContact);
	void   JabberCheckAllContactsAreTransported( void );
	int    __cdecl JGetAdvancedStatusIcon(WPARAM wParam, LPARAM lParam );

	//---- jabber_iq.c -------------------------------------------------------------------

	JABBER_IQ_PFUNC JabberIqFetchFunc( int iqId );

	void   JabberIqInit();
	void   JabberIqUninit();
	void   JabberIqAdd( unsigned int iqId, JABBER_IQ_PROCID procId, JABBER_IQ_PFUNC func );
	void   JabberIqRemove( int index );
	void   JabberIqExpire();
		  
	void   JabberIqResultBind( XmlNode *iqNode, void *userdata );
	void   JabberIqResultBrowseRooms( XmlNode *iqNode, void *userdata );
	void   JabberIqResultDiscoAgentInfo( XmlNode *iqNode, void *userdata );
	void   JabberIqResultDiscoAgentItems( XmlNode *iqNode, void *userdata );
	void   JabberIqResultDiscoRoomItems( XmlNode *iqNode, void *userdata );
	void   JabberIqResultDiscoBookmarks( XmlNode *iqNode, void *userdata );
	void   JabberIqResultSetBookmarks( XmlNode *iqNode, void *userdata );
	void   JabberIqResultExtSearch( XmlNode *iqNode, void *userdata );
	void   JabberIqResultGetAgents( XmlNode *iqNode, void *userdata );
	void   JabberIqResultGetAuth( XmlNode *iqNode, void *userdata );
	void   JabberIqResultGetAvatar( XmlNode *iqNode, void *userdata );
	void   JabberIqResultGetMuc( XmlNode *iqNode, void *userdata );
	void   JabberIqResultGetRegister( XmlNode *iqNode, void *userdata );
	void   JabberIqResultGetRoster( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultGetVcard( XmlNode *iqNode, void *userdata );
	void   JabberIqResultMucGetAdminList( XmlNode *iqNode, void *userdata );
	void   JabberIqResultMucGetBanList( XmlNode *iqNode, void *userdata );
	void   JabberIqResultMucGetMemberList( XmlNode *iqNode, void *userdata );
	void   JabberIqResultMucGetModeratorList( XmlNode *iqNode, void *userdata );
	void   JabberIqResultMucGetOwnerList( XmlNode *iqNode, void *userdata );
	void   JabberIqResultMucGetVoiceList( XmlNode *iqNode, void *userdata );
	void   JabberIqResultNestedRosterGroups( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultSession( XmlNode *iqNode, void *userdata );
	void   JabberIqResultSetAuth( XmlNode *iqNode, void *userdata );
	void   JabberIqResultSetPassword( XmlNode *iqNode, void *userdata );
	void   JabberIqResultSetRegister( XmlNode *iqNode, void *userdata );
	void   JabberIqResultSetSearch( XmlNode *iqNode, void *userdata );
	void   JabberIqResultSetVcard( XmlNode *iqNode, void *userdata );
	void   JabberIqResultEntityTime( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultLastActivity( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   JabberProcessIqResultVersion( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   JabberProcessLoginRq( ThreadData* info, DWORD rq );
	void   JabberOnLoggedIn( ThreadData* info );

	//---- jabber_iq_handlers.cpp --------------------------------------------------------

	void   JabberProcessIqVersion( XmlNode* node, void* userdata, CJabberIqInfo* pInfo );
	void   JabberProcessIqLast( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   JabberProcessIqPing( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   JabberProcessIqTime202( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   JabberProcessIqAvatar( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   JabberHandleSiRequest( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   JabberHandleRosterPushRequest( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
	void   JabberHandleIqRequestOOB( XmlNode* node, void* userdata, CJabberIqInfo *pInfo );
		  
	void   JabberIbbInitiateResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   JabberIbbCloseResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   JabberFtHandleIbbIq( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	void   JabberIbbSendThread( JABBER_IBB_TRANSFER *jibb );
	void   JabberIbbReceiveThread( JABBER_IBB_TRANSFER *jibb );
	BOOL   JabberIbbProcessRecvdData( TCHAR *data, TCHAR *sid, TCHAR *seq );
		  
	void   JabberFtSiResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
	BOOL   JabberFtIbbSend( int blocksize, void *userdata );
	BOOL   JabberFtSend( HANDLE hConn, void *userdata );
	void   JabberFtSendFinal( BOOL success, void *userdata );
	int    JabberFtReceive( HANDLE hConn, void *userdata, char* buffer, int datalen );
	void   JabberFtReceiveFinal( BOOL success, void *userdata );

	//---- jabber_list.cpp ---------------------------------------------------------------

	JABBER_LIST_ITEM *JabberListAdd( JABBER_LIST list, const TCHAR* jid );
	JABBER_LIST_ITEM *JabberListGetItemPtr( JABBER_LIST list, const TCHAR* jid );
	JABBER_LIST_ITEM *JabberListGetItemPtrFromIndex( int index );

	void   JabberListWipe( void );
	int    JabberListExist( JABBER_LIST list, const TCHAR* jid );

	BOOL   JabberListLock();
	BOOL   JabberListUnlock();

	void   JabberListRemove( JABBER_LIST list, const TCHAR* jid );
	void   JabberListRemoveList( JABBER_LIST list );
	void   JabberListRemoveByIndex( int index );
	int    JabberListFindNext( JABBER_LIST list, int fromOffset );

	int    JabberListAddResource( JABBER_LIST list, const TCHAR* jid, int status, const TCHAR* statusMessage, char priority = 0 );
	void   JabberListRemoveResource( JABBER_LIST list, const TCHAR* jid );
	TCHAR* JabberListGetBestResourceNamePtr( const TCHAR* jid );
	TCHAR* JabberListGetBestClientResourceNamePtr( const TCHAR* jid );

	void   JabberSetMucConfig( XmlNode* node, void *from );
	void   JabberIqResultMucGetJidList( XmlNode *iqNode, JABBER_MUC_JIDLIST_TYPE listType );
		  
	void   JabberIqResultServerDiscoInfo( XmlNode* iqNode, void* userdata );
	void   JabberIqResultGetVcardPhoto( const TCHAR* jid, XmlNode* n, HANDLE hContact, BOOL& hasPhoto );
	void   JabberSetBookmarkRequest (XmlNodeIq& iqId);

	//---- jabber_menu.cpp ---------------------------------------------------------------

	int    __cdecl JabberMenuConvertChatContact( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuRosterAdd( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuHandleRequestAuth( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuHandleGrantAuth( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuJoinLeave( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuTransportLogin( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuTransportResolve( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuBookmarkAdd( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuRevokeAuth( WPARAM wParam, LPARAM lParam );
	int    __cdecl JabberMenuHandleResource(WPARAM wParam, LPARAM lParam, LPARAM res);

	void   JabberMenuInit( void );
	void   JabberMenuUninit( void );

	void   JabberMenuHideSrmmIcon(HANDLE hContact);
	void   JabberMenuUpdateSrmmIcon(JABBER_LIST_ITEM *item);
	
	void   JabberAuthWorker( HANDLE hContact, char* authReqType );

	//---- jabber_misc.c -----------------------------------------------------------------

	int    __cdecl JabberGetEventTextChatStates( WPARAM wParam, LPARAM lParam );

	void   JabberAddContactToRoster( const TCHAR* jid, const TCHAR* nick, const TCHAR* grpName, JABBER_SUBSCRIPTION subscription );
	void   JabberDBAddAuthRequest( TCHAR* jid, TCHAR* nick );
	HANDLE JabberDBCreateContact( TCHAR* jid, TCHAR* nick, BOOL temporary, BOOL stripResource );
	void   JabberGetAvatarFileName( HANDLE hContact, char* pszDest, int cbLen );
	void   JabberResolveTransportNicks( TCHAR* jid );
	void   JabberSetServerStatus( int iNewStatus );
	void   JabberFormatMirVer(JABBER_RESOURCE_STATUS *resource, TCHAR *buf, int bufSize);
	void   JabberUpdateMirVer(JABBER_LIST_ITEM *item);
	void   JabberUpdateMirVer(HANDLE hContact, JABBER_RESOURCE_STATUS *resource);
	void   JabberSetContactOfflineStatus( HANDLE hContact );
	void   InitCustomFolders( void );

	//---- jabber_opt.cpp ----------------------------------------------------------------

	int    __cdecl JabberMenuHandleRosterControl( WPARAM wParam, LPARAM lParam );

	void   _RosterExportToFile(HWND hwndDlg);
	void   _RosterImportFromFile(HWND hwndDlg);
	void   _RosterSendRequest(HWND hwndDlg, BYTE rrAction);
	void   _RosterHandleGetRequest( XmlNode* node, void* userdata );

	//---- jabber_password.cpp --------------------------------------------------------------
	
	int    __cdecl JabberMenuHandleChangePassword( WPARAM wParam, LPARAM lParam );

	//---- jabber_privacy.cpp ------------------------------------------------------------
	ROSTERREQUSERDATA rrud;

	int    __cdecl menuSetPrivacyList( WPARAM wParam, LPARAM lParam, LPARAM iList );
	int    __cdecl JabberMenuHandlePrivacyLists( WPARAM wParam, LPARAM lParam );

	void   QueryPrivacyLists( void );
		  
	void   JabberProcessIqPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultPrivacyList( XmlNode* iqNode, void* userdata );
	void   JabberIqResultPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultPrivacyListActive( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultPrivacyListDefault( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberIqResultPrivacyListModify( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );

	//---- jabber_proto.cpp --------------------------------------------------------------

	HANDLE AddToListByJID( const TCHAR* newJid, DWORD flags );

	//---- jabber_rc.cpp -----------------------------------------------------------------

	int    JabberRcGetUnreadEventsCount( void );

	//---- jabber_search.cpp -------------------------------------------------------------

	void   JabberSearchReturnResults( HANDLE id, void* pvUsersInfo, U_TCHAR_MAP* pmAllFields );
	void   JabberIqResultAdvancedSearch( XmlNode *iqNode, void *userdata );
	void   JabberIqResultGetSearchFields( XmlNode *iqNode, void *userdata );
	int    JabberSearchRenewFields( HWND hwndDlg, JabberSearchData * dat);
	void   JabberSearchDeleteFromRecent( TCHAR* szAddr, BOOL deleteLastFromDB = TRUE );
	void   JabberSearchAddToRecent( TCHAR* szAddr, HWND hwndDialog = NULL );

	//---- jabber_std.cpp ----------------------------------------------

	void   JCreateService( const char* szService, JServiceFunc serviceProc );
	void   JCreateServiceParam( const char* szService, JServiceFuncParam serviceProc, LPARAM lParam );
	HANDLE JCreateHookableEvent( const char* szService );

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

	void   JabberEnableMenuItems( BOOL bEnable );

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

	//---- jabber_thread.c ----------------------------------------------

	void   ServerThread( ThreadData* info );
		  
	void   JabberProcessFailure( XmlNode *node, void *userdata );
	void   JabberProcessError( XmlNode *node, void *userdata );
	void   JabberProcessSuccess( XmlNode *node, void *userdata );
	void   JabberProcessChallenge( XmlNode *node, void *userdata );
	void   JabberProcessProceed( XmlNode *node, void *userdata );	
	void   JabberProcessCompressed( XmlNode *node, void *userdata );
	void   JabberProcessMessage( XmlNode *node, void *userdata );
	void   JabberProcessPresence( XmlNode *node, void *userdata );
	void   JabberProcessPresenceCapabilites( XmlNode *node );
	void   JabberProcessPubsubEvent( XmlNode *node );

	void   JabberUpdateJidDbSettings( TCHAR *jid );
	HANDLE JabberCreateTemporaryContact( TCHAR *szJid, JABBER_LIST_ITEM* chatItem );

	void   JabberPerformRegistration( ThreadData* info );
	void   JabberPerformIqAuth( ThreadData* info );
	void   JabberProcessFeatures( XmlNode *node, void *userdata );

	void   xmlStreamInitialize( char *which );
	void   xmlStreamInitializeNow(ThreadData* info);

	void   JabberProcessIq( XmlNode *node, void *userdata );
	void   JabberProcessRegIq( XmlNode *node, void *userdata );

	//---- jabber_util.c -----------------------------------------------------------------

	JABBER_RESOURCE_STATUS* JabberResourceInfoFromJID( TCHAR* jid );

	void   JabberSerialInit( void );
	void   JabberSerialUninit( void );
	int    JabberSerialNext( void );

	HANDLE JabberHContactFromJID( const TCHAR* jid , BOOL bStripResource = 3);
	HANDLE JabberChatRoomHContactFromJID( const TCHAR* jid );
	void   JabberLog( const char* fmt, ... );
	void   JabberSendVisibleInvisiblePresence( BOOL invisible );
	void   JabberSendPresenceTo( int status, TCHAR* to, XmlNode* extra );
	void   JabberSendPresence( int m_iStatus, bool bSendToAll );
	void   JabberStringAppend( char* *str, int *sizeAlloced, const char* fmt, ... );
	TCHAR* JabberGetClientJID( const TCHAR* jid, TCHAR*, size_t );
	TCHAR* JabberGetXmlLang( void );
	void   JabberRebuildStatusMenu( void );

	void   JabberComboLoadRecentStrings(HWND hwndDlg, UINT idcCombo, char *param, int recentCount=JABBER_DEFAULT_RECENT_COUNT);
	void   JabberComboAddRecentString(HWND hwndDlg, UINT idcCombo, char *param, TCHAR *string, int recentCount=JABBER_DEFAULT_RECENT_COUNT);
	BOOL   JabberEnterString(TCHAR *result, size_t resultLen, TCHAR *caption=NULL, int type=0, char *windowName=NULL, int recentCount=JABBER_DEFAULT_RECENT_COUNT);

	//---- jabber_vcard.c -----------------------------------------------

	int    __cdecl JabberMenuHandleVcard( WPARAM wParam, LPARAM lParam );

	void   JabberGroupchatJoinByHContact( HANDLE hContact );
	void   JabberUpdateVCardPhoto( char * szFileName );
	int    JabberSendGetVcard( const TCHAR* jid );
	void   AppendVcardFromDB( XmlNode* n, char* tag, char* key );
	void   SetServerVcard();
	void   SaveVcardToDB( struct VcardTab *dat );

	//---- jabber_ws.c -------------------------------------------------

	JABBER_SOCKET JabberWsConnect( char* host, WORD port );

	BOOL   JabberWsInit(void);
	void   JabberWsUninit(void);
	int    JabberWsSend( JABBER_SOCKET s, char* data, int datalen, int flags );
	int    JabberWsRecv( JABBER_SOCKET s, char* data, long datalen, int flags );

	BOOL   JabberSslInit();
	BOOL   JabberSslUninit();

	//---- jabber_xml.c ------------------------------------------------------------------

	int    JabberXmlParse( XmlState *xmlState, char* buffer );
	void   JabberConsoleProcessXml(XmlNode *node, DWORD flags);

	//---- jabber_xmlns.c ----------------------------------------------------------------

	void   JabberHandleDiscoInfoRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
	void   JabberHandleDiscoItemsRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );

	//---- jabber_xstatus.c --------------------------------------------------------------

	int    __cdecl JabberSetListeningTo( WPARAM wParam, LPARAM lParams );
	int    __cdecl JabberGetXStatusIcon( WPARAM wParam, LPARAM lParams );
	int    __cdecl JabberGetXStatus( WPARAM wParam, LPARAM lParams );
	int    __cdecl JabberSetXStatus( WPARAM wParam, LPARAM lParams );
	int    __cdecl menuSetXStatus( WPARAM wParam, LPARAM lParam, LPARAM param );

	HICON  GetXStatusIcon(int bStatus, UINT flags);
	void   JabberUpdateContactExtraIcon( HANDLE hContact );
			
	BOOL   JabberSendPepMood( int nMoodNumber, TCHAR* szMoodText );
	BOOL   JabberSendPepTune( TCHAR* szArtist, TCHAR* szLength, TCHAR* szSource, TCHAR* szTitle, TCHAR* szTrack, TCHAR* szUri );
			
	void   JabberXStatusInit( void );
	void   JabberXStatusUninit( void );
	void   InitXStatusIcons( void );
			
	void   JabberSetContactMood( HANDLE hContact, const char* moodName, const TCHAR* moodText );
	void   JabberSetContactTune( HANDLE hContact,  TCHAR* szArtist, TCHAR* szLength, TCHAR* szSource, TCHAR* szTitle, TCHAR* szTrack, TCHAR* szUri );

private:
	CRITICAL_SECTION serialMutex;
	unsigned int serial;

	HANDLE hInitChat;
	HANDLE hPrebuildStatusMenu;
	
	int     nMenuResourceItems;
	HANDLE* hMenuResourceItems;

	HANDLE* hIconLibItems;
};

#endif
