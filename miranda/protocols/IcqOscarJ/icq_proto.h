// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2009 Joe Kucera, George Hazan
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Protocol Interface declarations
//
// -----------------------------------------------------------------------------

#ifndef _ICQ_PROTO_H_
#define _ICQ_PROTO_H_

#include "m_system_cpp.h"
#include "m_protoint.h"

#define LISTSIZE 100

#define XSTATUS_COUNT 32

struct CIcqProto;
typedef void ( CIcqProto::*IcqRateFunc )( void );

typedef void    ( __cdecl CIcqProto::*IcqThreadFunc )( void* );
typedef int     ( __cdecl CIcqProto::*IcqEventFunc )( WPARAM, LPARAM );
typedef INT_PTR ( __cdecl CIcqProto::*IcqServiceFunc )( WPARAM, LPARAM );
typedef INT_PTR ( __cdecl CIcqProto::*IcqServiceFuncParam )( WPARAM, LPARAM, LPARAM );

// for InfoUpdate
struct userinfo
{
  DWORD dwUin;
	HANDLE hContact;
  time_t queued;
};

struct CIcqProto : public PROTO_INTERFACE
{
				CIcqProto( const char*, const TCHAR* );
				~CIcqProto();

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

	virtual	HANDLE __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath );
	virtual	int    __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer );
	virtual	int    __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason );
	virtual	int    __cdecl FileResume( HANDLE hTransfer, int* action, const char** szFilename );

	virtual	DWORD_PTR __cdecl GetCaps( int type, HANDLE hContact = NULL );
	virtual	HICON  __cdecl GetIcon( int iconIndex );
	virtual	int    __cdecl GetInfo( HANDLE hContact, int infoType );

	virtual	HANDLE __cdecl SearchBasic( const char* id );
	virtual	HANDLE __cdecl SearchByEmail( const char* email );
	virtual	HANDLE __cdecl SearchByName(const char *nick, const char *firstName, const char *lastName);
	virtual	HWND   __cdecl SearchAdvanced( HWND owner );
	virtual	HWND   __cdecl CreateExtendedSearchUI( HWND owner );

	virtual	int    __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvFile( HANDLE hContact, PROTORECVFILE* );
	virtual	int    __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* );

	virtual	int    __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList );
	virtual	HANDLE __cdecl SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles );
	virtual	int    __cdecl SendMsg( HANDLE hContact, int flags, const char* msg );
	virtual	int    __cdecl SendUrl( HANDLE hContact, int flags, const char* url );

	virtual	int    __cdecl SetApparentMode( HANDLE hContact, int mode );
	virtual	int    __cdecl SetStatus( int iNewStatus );

	virtual	HANDLE __cdecl GetAwayMsg( HANDLE hContact );
	virtual	int    __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt );
	virtual	int    __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg );
	virtual	int    __cdecl SetAwayMsg( int m_iStatus, const char* msg );

	virtual	int    __cdecl UserIsTyping( HANDLE hContact, int type );

	virtual	int    __cdecl OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam );

	//====| Services |====================================================================
	INT_PTR  __cdecl AddServerContact(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetInfoSetting(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl ChangeInfoEx(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetAvatarCaps(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetAvatarInfo(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetMyAvatar(WPARAM wParam, LPARAM lParam);	
	INT_PTR  __cdecl GetXStatus(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetXStatusEx(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetXStatusIcon(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GrantAuthorization(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl menuXStatus(WPARAM wParam,LPARAM lParam,LPARAM fParam);
	INT_PTR  __cdecl RecvAuth(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl RequestAdvStatusIconIdx(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl RequestAuthorization(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl RequestXStatusDetails(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl RevokeAuthorization(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl SendSms(WPARAM wParam, LPARAM lParam);	
	INT_PTR  __cdecl SendYouWereAdded(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl SetMyAvatar(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl SetNickName(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl SetPassword(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl SetXStatus(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl SetXStatusEx(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl ShowXStatusDetails(WPARAM wParam, LPARAM lParam);

	INT_PTR  __cdecl OnCreateAccMgrUI(WPARAM, LPARAM);

    //====| Events |======================================================================
	void __cdecl OnAddContactForever( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	int  __cdecl OnContactDeleted( WPARAM, LPARAM );
	int  __cdecl OnDbSettingChanged( WPARAM, LPARAM );
	int  __cdecl OnIdleChanged( WPARAM, LPARAM );
	int  __cdecl OnModernOptInit( WPARAM, LPARAM );
	int  __cdecl OnModulesLoaded( WPARAM, LPARAM );
	int  __cdecl OnOptionsInit( WPARAM, LPARAM );
	int  __cdecl OnPreShutdown( WPARAM, LPARAM );
	int  __cdecl OnPreBuildContactMenu( WPARAM, LPARAM );
	int  __cdecl OnMsgUserTyping( WPARAM, LPARAM );
	int  __cdecl OnProcessSrmmIconClick( WPARAM, LPARAM );
	int  __cdecl OnProcessSrmmEvent( WPARAM, LPARAM );
	int  __cdecl OnReloadIcons( WPARAM, LPARAM );
	void __cdecl OnRenameContact( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	void __cdecl OnRenameGroup( DBCONTACTWRITESETTING* cws, HANDLE hContact );
	int  __cdecl OnUserInfoInit( WPARAM, LPARAM );

	int  __cdecl CListMW_ExtraIconsRebuild( WPARAM, LPARAM );
	int  __cdecl CListMW_ExtraIconsApply( WPARAM, LPARAM );
	int  __cdecl OnPreBuildStatusMenu( WPARAM, LPARAM );

	//====| Data |========================================================================
	IcqIconHandle m_hIconProtocol;
  HANDLE m_hContactMenuItems[5];
	HANDLE m_hServerNetlibUser, m_hDirectNetlibUser;
	HANDLE hsmsgrequest, hxstatuschanged, hxstatusiconchanged;

	BYTE m_bGatewayMode;
	BYTE m_bSecureLogin;
	BYTE m_bAimEnabled;
	BYTE m_bUtfEnabled;
	WORD m_wAnsiCodepage;
	BYTE m_bDCMsgEnabled;
	BYTE m_bTempVisListEnabled;
	BYTE m_bSsiEnabled;
	BYTE m_bSsiSimpleGroups;
	BYTE m_bAvatarsEnabled;
	BYTE m_bXStatusEnabled;
	
	CRITICAL_SECTION localSeqMutex;
	CRITICAL_SECTION connectionHandleMutex;

	int   m_bIdleAllow;
	DWORD m_dwLocalUIN;

	char m_szPassword[16];
	BYTE m_bRememberPwd;

	int cheekySearchId;
	DWORD cheekySearchUin;
	char* cheekySearchUid;

	/*******************************************************************
	* Function declarations
	*******************************************************************/

	//----| capabilities.cpp |------------------------------------------------------------
	// Deletes all oscar capabilities for a given contact.
	void ClearAllContactCapabilities(HANDLE hContact);

	// Deletes one or many oscar capabilities for a given contact.
	void ClearContactCapabilities(HANDLE hContact, DWORD fdwCapabilities);

	// Sets one or many oscar capabilities for a given contact.
	void SetContactCapabilities(HANDLE hContact, DWORD fdwCapabilities);

	// Returns true if the given contact supports the requested capabilites.
	BOOL CheckContactCapabilities(HANDLE hContact, DWORD fdwCapabilities);

	// Scans a binary buffer for oscar capabilities and adds them to the db.
	void AddCapabilitiesFromBuffer(HANDLE hContact, BYTE* pbyBuffer, int nLength);

	//----| chan_01login.cpp |------------------------------------------------------------
	void   handleLoginChannel(BYTE *buf, WORD datalen, serverthread_info *info);

	//----| chan_02data.cpp |-------------------------------------------------------------
	void   handleDataChannel(unsigned char *buf, WORD wLen, serverthread_info *info);

	int    unpackSnacHeader(snac_header* pSnacHeader, unsigned char **pBuffer, WORD* pwBufferLength);
	void   LogFamilyError(WORD wFamily, WORD wError);

	//----| chan_03error.cpp |------------------------------------------------------------
	void   handleErrorChannel(unsigned char *buf, WORD datalen);

	//----| chan_04close.cpp |------------------------------------------------------------
	void   handleCloseChannel(unsigned char *buf, WORD datalen, serverthread_info *info);
	void   handleLoginReply(unsigned char *buf, WORD datalen, serverthread_info *info);
	void   handleMigration(serverthread_info *info);
	void   handleSignonError(WORD wError);

	int    connectNewServer(serverthread_info *info);

	//----| chan_05ping.cpp |-------------------------------------------------------------
	void   __cdecl KeepAliveThread(void* arg);

	void   handlePingChannel(unsigned char *buf, WORD wLen);

  unsigned __cdecl icq_keepAliveThread(void* arg);

	void   StartKeepAlive(serverthread_info* info);
	void   StopKeepAlive(serverthread_info* info);

	//----| cookies.cpp |-----------------------------------------------------------------
	CRITICAL_SECTION cookieMutex; // we want this in avatar thread, used as queue lock
	LIST<icq_cookie_info> cookies;
	WORD   wCookieSeq;

	DWORD  AllocateCookie(BYTE bType, WORD wIdent, HANDLE hContact, void *pvExtra);
	void   FreeCookie(DWORD dwCookie);
	void   FreeCookieByData(BYTE bType, void *pvExtra);
	void   ReleaseCookie(DWORD dwCookie);
	DWORD  GenerateCookie(WORD wIdent);

	int    GetCookieType(DWORD dwCookie);

	int    FindCookie(DWORD wCookie, HANDLE *phContact, void **ppvExtra);
	int    FindCookieByData(void *pvExtra, DWORD *pdwCookie, HANDLE *phContact);
  int    FindCookieByType(BYTE bType, DWORD *pdwCookie, HANDLE *phContact, void **ppvExtra);
	int    FindMessageCookie(DWORD dwMsgID1, DWORD dwMsgID2, DWORD *pdwCookie, HANDLE *phContact, cookie_message_data **ppvExtra);

	void   InitMessageCookie(cookie_message_data *pCookie);
	cookie_message_data* CreateMessageCookie(WORD bMsgType, BYTE bAckType);
	cookie_message_data* CreateMessageCookieData(BYTE bMsgType, HANDLE hContact, DWORD dwUin, int bUseSrvRelay);

	void   RemoveExpiredCookies(void);

	//----| directpackets.cpp |-----------------------------------------------------------
	void   icq_sendDirectMsgAck(directconnect* dc, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, char* szCap);
	DWORD  icq_sendGetAwayMsgDirect(HANDLE hContact, int type);
	void   icq_sendAwayMsgReplyDirect(directconnect* dc, WORD wCookie, BYTE msgType, const char** szMsg);
	void   icq_sendFileAcceptDirect(HANDLE hContact, filetransfer* ft);
	void   icq_sendFileDenyDirect(HANDLE hContact, filetransfer* ft, const char *szReason);
	int    icq_sendFileSendDirectv7(filetransfer *ft, const char* pszFiles);
	int    icq_sendFileSendDirectv8(filetransfer *ft, const char *pszFiles);
	DWORD  icq_SendDirectMessage(HANDLE hContact, const char *szMessage, int nBodyLength, WORD wPriority, cookie_message_data *pCookieData, char *szCap);
	void   icq_sendXtrazRequestDirect(HANDLE hContact, DWORD dwCookie, char* szBody, int nBodyLen, WORD wType);
	void   icq_sendXtrazResponseDirect(HANDLE hContact, WORD wCookie, char* szBody, int nBodyLen, WORD wType);

	//----| fam_01service.cpp |-----------------------------------------------------------
  HANDLE m_hNotifyNameInfoEvent;

	void   handleServiceFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader, serverthread_info *info);
	char*  buildUinList(int subtype, WORD wMaxLen, HANDLE *hContactResume);
	void   sendEntireListServ(WORD wFamily, WORD wSubtype, int listType);
	void   setUserInfo(void);
	void   handleServUINSettings(int nPort, serverthread_info *info);

	//----| fam_02location.cpp |----------------------------------------------------------
	void   handleLocationFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);
	void   handleLocationUserInfoReply(BYTE* buf, WORD wLen, DWORD dwCookie);

	//----| fam_03buddy.cpp |-------------------------------------------------------------
	void   handleBuddyFam(BYTE* pBuffer, WORD wBufferLength, snac_header* pSnacHeader, serverthread_info *info);
	void   handleReplyBuddy(BYTE* buf, WORD wPackLen);
	void   handleUserOffline(BYTE* buf, WORD wPackLen);
	void   handleUserOnline(BYTE* buf, WORD wPackLen, serverthread_info* info);
  void   parseStatusNote(DWORD dwUin, char *szUid, HANDLE hContact, oscar_tlv_chain *pChain);
	void   handleNotifyRejected(BYTE* buf, WORD wPackLen);

	//----| fam_04message.cpp |-----------------------------------------------------------
	icq_mode_messages m_modeMsgs;
	CRITICAL_SECTION  m_modeMsgsMutex;

	void   handleMsgFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader);

	void   handleReplyICBM(BYTE *buf, WORD wLen, WORD wFlags, DWORD dwRef);
	void   handleRecvServMsg(BYTE *buf, WORD wLen, WORD wFlags, DWORD dwRef);
	void   handleRecvServMsgType1(BYTE *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwMsgID1, DWORD dwMsgID2, DWORD dwRef);
	void   handleRecvServMsgType2(BYTE *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwTS1, DWORD dwTS2);
	void   handleRecvServMsgType4(BYTE *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwMsgID1, DWORD dwMsgID2, DWORD dwRef);
	void   handleRecvServMsgError(BYTE *buf, WORD wLen, WORD wFlags, DWORD dwRef);
	void   handleRecvMsgResponse(BYTE *buf, WORD wLen, WORD wFlags, DWORD dwRef);
	void   handleServerAck(BYTE *buf, WORD wLen, WORD wFlags, DWORD dwRef);
	void   handleStatusMsgReply(const char *szPrefix, HANDLE hContact, DWORD dwUin, WORD wVersion, int bMsgType, WORD wCookie, const char *szMsg);
	void   handleTypingNotification(BYTE *buf, WORD wLen, WORD wFlags, DWORD dwRef);
	void   handleMissedMsg(BYTE *buf, WORD wLen, WORD wFlags, DWORD dwRef);
	void   handleOffineMessagesReply(BYTE *buf, WORD wLen, WORD wFlags, DWORD dwRef);
	void   handleRecvServMsgContacts(BYTE *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwID1, DWORD dwID2, WORD wCommand);
	void   handleRuntimeError(WORD wError);

	void   parseTLV2711(DWORD dwUin, HANDLE hContact, DWORD dwID1, DWORD dwID2, WORD wAckType, oscar_tlv* tlv);
	void   parseServerGreeting(BYTE *pDataBuf, WORD wLen, WORD wMsgLen, DWORD dwUin, BYTE bFlags, WORD wStatus, WORD wCookie, WORD wAckType, DWORD dwID1, DWORD dwID2, WORD wVersion);

	HANDLE handleMessageAck(DWORD dwUin, WORD wCookie, WORD wVersion, int type, WORD wMsgLen, PBYTE buf, BYTE bFlags);
	void   handleMessageTypes(DWORD dwUin, DWORD dwTimestamp, DWORD dwMsgID, DWORD dwMsgID2, WORD wCookie, WORD wVersion, int type, int flags, WORD wAckType, DWORD dwDataLen, WORD wMsgLen, char *pMsg, BOOL bThruDC, message_ack_params *pAckParams);
	void   sendMessageTypesAck(HANDLE hContact, int bUnicode, message_ack_params *pArgs);
	void   sendTypingNotification(HANDLE hContact, WORD wMTNCode);

	int    unpackPluginTypeId(BYTE **pBuffer, WORD *pwLen, int *pTypeId, WORD *pFunctionId, BOOL bThruDC);

	char*  convertMsgToUserSpecificUtf(HANDLE hContact, const char *szMsg);

	//----| fam_09bos.cpp |---------------------------------------------------------------
	void   handleBosFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);
	void   handlePrivacyRightsReply(unsigned char *pBuffer, WORD wBufferLength);
	void   makeContactTemporaryVisible(HANDLE hContact);

	//----| fam_0alookup.cpp |------------------------------------------------------------
	void   handleLookupFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);

	void   handleLookupEmailReply(BYTE* buf, WORD wLen, DWORD dwCookie);
	void   ReleaseLookupCookie(DWORD dwCookie, cookie_search *pCookie);

	//----| fam_0bstatus.cpp |------------------------------------------------------------
	void   handleStatusFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);

	//----| fam_13servclist.cpp |---------------------------------------------------------
	BOOL   bIsSyncingCL;
	
	WORD   m_wServerListLimits[0x20];
	WORD   m_wServerListGroupMaxContacts;
	WORD   m_wServerListRecordNameMaxLength;

	void   handleServCListFam(BYTE *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, serverthread_info *info);
  void   handleServerCListRightsReply(BYTE *buf, WORD wLen);
	void   handleServerCListAck(cookie_servlist_action* sc, WORD wError);
	void   handleServerCListReply(BYTE *buf, WORD wLen, WORD wFlags, serverthread_info *info);
  void   handleServerCListItemAdd(const char *szRecordName, WORD wGroupId, WORD wItemId, WORD wItemType, oscar_tlv_chain *pItemData);
  void   handleServerCListItemUpdate(const char *szRecordName, WORD wGroupId, WORD wItemId, WORD wItemType, oscar_tlv_chain *pItemData);
  void   handleServerCListItemDelete(const char *szRecordName, WORD wGroupId, WORD wItemId, WORD wItemType, oscar_tlv_chain *pItemData);
	void   handleRecvAuthRequest(BYTE *buf, WORD wLen);
	void   handleRecvAuthResponse(BYTE *buf, WORD wLen);
	void   handleRecvAdded(BYTE *buf, WORD wLen);

	HANDLE HContactFromRecordName(const char *szRecordName, int *bAdded);

	void   icq_sendServerBeginOperation(int bImport);
	void   icq_sendServerEndOperation();
	void   sendRosterAck(void);

	int    getServerDataFromItemTLV(oscar_tlv_chain* pChain, unsigned char *buf);
	DWORD  updateServerGroupData(WORD wGroupId, void *groupData, int groupSize, DWORD dwOperationFlags);
	void   updateServAvatarHash(BYTE *pHash, int size);
	void   updateServVisibilityCode(BYTE bCode);

	//----| fam_15icqserver.cpp |---------------------------------------------------------
	void   handleIcqExtensionsFam(BYTE *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);

	void   handleExtensionError(BYTE *buf, WORD wPackLen);
	void   handleExtensionServerInfo(BYTE *buf, WORD wPackLen, WORD wFlags);
	void   handleExtensionMetaResponse(BYTE *databuf, WORD wPacketLen, WORD wCookie, WORD wFlags);

  int    parseUserInfoRecord(HANDLE hContact, oscar_tlv *pData, UserInfoRecordItem pRecordDef[], int nRecordDef, int nMaxRecords);

  void   handleDirectoryQueryResponse(BYTE *databuf, WORD wPacketLen, WORD wCookie, WORD wReplySubtype, WORD wFlags);
  void   handleDirectoryUpdateResponse(BYTE *databuf, WORD wPacketLen, WORD wCookie, WORD wReplySubtype);

  void   parseDirectoryUserDetailsData(HANDLE hContact, oscar_tlv_chain *cDetails, DWORD dwCookie, cookie_directory_data *pCookieData, WORD wReplySubType);
  void   parseDirectorySearchData(oscar_tlv_chain *cDetails, DWORD dwCookie, cookie_directory_data *pCookieData, WORD wReplySubType);

	void   parseSearchReplies(unsigned char *databuf, WORD wPacketLen, WORD wCookie, WORD wReplySubtype, BYTE bResultCode);
	void   parseUserInfoUpdateAck(unsigned char *databuf, WORD wPacketLen, WORD wCookie, WORD wReplySubtype, BYTE bResultCode);

	void   ReleaseSearchCookie(DWORD dwCookie, cookie_search *pCookie);

	//----| fam_17signon.cpp |------------------------------------------------------------
	void   handleAuthorizationFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, serverthread_info *info);
	void   handleAuthKeyResponse(BYTE *buf, WORD wPacketLen, serverthread_info *info);

	void   sendClientAuth(const char* szKey, WORD wKeyLen, BOOL bSecure);

	//----| icq_avatars.cpp |-------------------------------------------------------------
	struct avatarthreadstartinfo* currentAvatarThread; 
	struct avatarrequest* pendingRequests;
	int    m_pendingAvatarsStart;
	int    bAvatarsFolderInited;
	HANDLE hAvatarsFolder;
	BOOL   AvatarsReady; // states if avatar connection established and ready for requests

	int    sendAvatarPacket(icq_packet* pPacket, avatarthreadstartinfo* atsi /*= currentAvatarThread*/);

	void   __cdecl AvatarThread(avatarthreadstartinfo *atsi);
	int    handleAvatarPackets(BYTE *buf, int buflen, avatarthreadstartinfo *atsi);

	void   handleAvatarLogin(BYTE *buf, WORD datalen, avatarthreadstartinfo *atsi);
	void   handleAvatarData(BYTE *pBuffer, WORD wBufferLength, avatarthreadstartinfo *atsi);

	void   handleAvatarServiceFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader, avatarthreadstartinfo *atsi);
	void   handleAvatarFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader, avatarthreadstartinfo *atsi);

  void   handleAvatarOwnerHash(WORD wItemID, BYTE bFlags, BYTE *pData, BYTE nDataLen);
	void   handleAvatarContactHash(DWORD dwUIN, char *szUID, HANDLE hContact, BYTE *pHash, int nHashLen, WORD wOldStatus);

	void   InitAvatars();
	avatarrequest *ReleaseAvatarRequestInQueue(avatarrequest *request);

	char*  loadMyAvatarFileName(void);
	void   GetFullAvatarFileName(int dwUin, char* szUid, int dwFormat, char* pszDest, int cbLen);
	void   GetAvatarFileName(int dwUin, char* szUid, char* pszDest, int cbLen);
	int    IsAvatarChanged(HANDLE hContact, BYTE* pHash, int nHashLen);

	int    GetAvatarData(HANDLE hContact, DWORD dwUin, char *szUid, BYTE *hash, unsigned int hashlen, char *file);
	int    SetAvatarData(HANDLE hContact, WORD wRef, char* data, unsigned int datalen);

	void   StartAvatarThread(HANDLE hConn, char* cookie, WORD cookieLen);
	void   StopAvatarThread();

	//----| icq_clients.cpp |-------------------------------------------------------------
	char*  detectUserClient(HANDLE hContact, DWORD dwUin, WORD wUserClass, WORD wVersion, DWORD dwFT1, DWORD dwFT2, DWORD dwFT3, DWORD dwOnlineSince, BYTE bDirectFlag, DWORD dwDirectCookie, DWORD dwWebPort, BYTE* caps, WORD wLen, BYTE* bClientId, char* szClientBuf);

	//----| icq_db.cpp |------------------------------------------------------------------
	HANDLE AddEvent(HANDLE hContact, WORD wType, DWORD dwTime, DWORD flags, DWORD cbBlob, PBYTE pBlob);
	void   CreateResidentSetting(const char* szSetting);
	HANDLE FindFirstContact();
	HANDLE FindNextContact(HANDLE hContact);
	int    IsICQContact(HANDLE hContact);

	int    getSetting(HANDLE hContact, const char* szSetting, DBVARIANT *dbv);
	BYTE   getSettingByte(HANDLE hContact, const char* szSetting, BYTE bDef);
	WORD   getSettingWord(HANDLE hContact, const char* szSetting, WORD wDef);
	DWORD  getSettingDword(HANDLE hContact, const char* szSetting, DWORD dwDef);
  double getSettingDouble(HANDLE hContact, const char *szSetting, double dDef);
	int    getSettingString(HANDLE hContact, const char* szSetting, DBVARIANT *dbv);
	int    getSettingStringStatic(HANDLE hContact, const char* valueName, char* dest, int dest_len);
	char*  getSettingStringUtf(HANDLE hContact, const char *szModule, const char *szSetting, char *szDef);
	char*  getSettingStringUtf(HANDLE hContact, const char *szSetting, char *szDef);
	int    getContactUid(HANDLE hContact, DWORD *pdwUin, uid_str* ppszUid);
	DWORD  getContactUin(HANDLE hContact);
	WORD   getContactStatus(HANDLE hContact);
	char*  getContactCListGroup(HANDLE hContact);

	int    deleteSetting(HANDLE hContact, const char* szSetting);

	int    setSettingByte(HANDLE hContact, const char* szSetting, BYTE bValue);	
	int    setSettingWord(HANDLE hContact, const char* szSetting, WORD wValue);
	int    setSettingDword(HANDLE hContact, const char* szSetting, DWORD dwValue);
  int    setSettingDouble(HANDLE hContact, const char *szSetting, double dValue);
	int    setSettingString(HANDLE hContact, const char* szSetting, const char* szValue);
	int    setSettingStringUtf(HANDLE hContact, const char *szModule, const char* szSetting, const char* szValue);
	int    setSettingStringUtf(HANDLE hContact, const char* szSetting, const char* szValue);
	int    setSettingBlob(HANDLE hContact, const char *szSetting, const BYTE *val, const int cbVal);
  int    setContactHidden(HANDLE hContact, BYTE bHidden);

	//----| icq_direct.cpp |--------------------------------------------------------------
	CRITICAL_SECTION directConnListMutex;
	LIST<directconnect> directConns;

	CRITICAL_SECTION expectedFileRecvMutex;
	LIST<filetransfer> expectedFileRecvs;

	void   __cdecl icq_directThread(struct directthreadstartinfo* dtsi);

	void   handleDirectPacket(directconnect* dc, PBYTE buf, WORD wLen);
	void   sendPeerInit_v78(directconnect* dc);
	void   sendPeerInitAck(directconnect* dc);
	void   sendPeerMsgInit(directconnect* dc, DWORD dwSeq);
	void   sendPeerFileInit(directconnect* dc);
	int    sendDirectPacket(directconnect* dc, icq_packet* pkt);

	void   CloseContactDirectConns(HANDLE hContact);
	directconnect* FindFileTransferDC(filetransfer* ft);
	filetransfer*  FindExpectedFileRecv(DWORD dwUin, DWORD dwTotalSize);
	BOOL   IsDirectConnectionOpen(HANDLE hContact, int type, int bPassive);
	void   OpenDirectConnection(HANDLE hContact, int type, void* pvExtra);
	void   CloseDirectConnection(directconnect *dc);
	int    SendDirectMessage(HANDLE hContact, icq_packet *pkt);

	//----| icq_directmsg.cpp |-----------------------------------------------------------
	void   handleDirectMessage(directconnect* dc, PBYTE buf, WORD wLen);
	void   handleDirectGreetingMessage(directconnect* dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wStatus, WORD wFlags, char* pszText);

	//----| icq_filerequests.cpp |--------------------------------------------------------
	filetransfer* CreateFileTransfer(HANDLE hContact, DWORD dwUin, int nVersion);

	void   handleFileAck(PBYTE buf, WORD wLen, DWORD dwUin, DWORD dwCookie, WORD wStatus, char* pszText);
	void   handleFileRequest(PBYTE buf, WORD wLen, DWORD dwUin, DWORD dwCookie, DWORD dwID1, DWORD dwID2, char* pszDescription, int nVersion, BOOL bDC);
	void   handleDirectCancel(directconnect *dc, PBYTE buf, WORD wLen, WORD wCommand, DWORD dwCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText);

	void   icq_CancelFileTransfer(HANDLE hContact, filetransfer* ft);

	//----| icq_filetransfer.cpp |--------------------------------------------------------
	void   icq_AcceptFileTransfer(HANDLE hContact, filetransfer *ft);
	void   icq_sendFileResume(filetransfer *ft, int action, const char *szFilename);
	void   icq_InitFileSend(filetransfer *ft);

	void   handleFileTransferPacket(directconnect *dc, PBYTE buf, WORD wLen);
	void   handleFileTransferIdle(directconnect *dc);

	//----| icq_infoupdate.cpp |----------------------------------------------------------
	CRITICAL_SECTION infoUpdateMutex;
	HANDLE hInfoQueueEvent;
	int    nInfoUserCount;
	int    bInfoPendingUsers;
	BOOL   bInfoUpdateEnabled;
	BOOL   bInfoUpdateRunning;
	HANDLE hInfoThread;
	userinfo m_infoUpdateList[LISTSIZE];

	void   __cdecl InfoUpdateThread(void*);

	void   icq_InitInfoUpdate(void);           // Queues all outdated users
	BOOL   icq_QueueUser(HANDLE hContact);     // Queue one UIN to the list for updating
	void   icq_DequeueUser(DWORD dwUin);       // Remove one UIN from the list
	void   icq_RescanInfoUpdate();             // Add all outdated contacts to the list
	void   icq_InfoUpdateCleanup(void);        // Clean up on exit
	void   icq_EnableUserLookup(BOOL bEnable); // Enable/disable user info lookups

	//----| icq_log.cpp |-----------------------------------------------------------------
	void   __cdecl icq_LogMessageThread(void* arg);

	void   icq_LogMessage(int level, const char *szMsg);
	void   icq_LogUsingErrorCode(int level, DWORD dwError, const char *szMsg);  //szMsg is optional
	void   icq_LogFatalParam(const char *szMsg, WORD wError);

	//----| icq_packet.cpp |--------------------------------------------------------------
	void   ppackLETLVLNTSfromDB(PBYTE *buf, int *buflen, const char *szSetting, WORD wType);
	void   ppackLETLVWordLNTSfromDB(PBYTE *buf, int *buflen, WORD w, const char *szSetting, WORD wType);
	void   ppackLETLVLNTSBytefromDB(PBYTE *buf, int *buflen, const char *szSetting, BYTE b, WORD wType);

  void   ppackTLVStringFromDB(PBYTE *buf, int *buflen, const char *szSetting, WORD wType);
  void   ppackTLVStringUtfFromDB(PBYTE *buf, int *buflen, const char *szSetting, WORD wType);
  void   ppackTLVDateFromDB(PBYTE *buf, int *buflen, const char *szSettingYear, const char *szSettingMonth, const char *szSettingDay, WORD wType);

  int    ppackTLVWordStringItemFromDB(PBYTE *buf, int *buflen, const char *szSetting, WORD wTypeID, WORD wTypeData, WORD wID);

	BOOL   unpackUID(BYTE **ppBuf, WORD *pwLen, DWORD *pdwUIN, uid_str *ppszUID);

	//----| icq_popups.cpp |--------------------------------------------------------------
	int    ShowPopUpMsg(HANDLE hContact, const char *szTitle, const char *szMsg, BYTE bType);

	//----| icq_proto.cpp |--------------------------------------------------------------
	void   __cdecl CheekySearchThread( void* );

  void   __cdecl GetAwayMsgThread( void *pStatusData );

  char*  PrepareStatusNote(int nStatus);

	//----| icq_rates.cpp |---------------------------------------------------------------
	CRITICAL_SECTION ratesMutex;
	rates  *m_rates;

  CRITICAL_SECTION ratesListsMutex;  // we need to be thread safe

	rate_record **pendingList1; // rate queue for xtraz requests
	int    pendingListSize1;

	rate_record **pendingList2; // rate queue for msg responses
	int    pendingListSize2;

	int    handleRateItem(rate_record *item, BOOL bAllowDelay);
	
	void   __cdecl rateDelayThread(struct rate_delay_args* pArgs);
	void   InitDelay(int nDelay, IcqRateFunc delaycode);
	void   RatesTimer1( void );
	void   RatesTimer2( void );

	//----| icq_server.cpp |--------------------------------------------------------------
	HANDLE hServerConn;
	WORD   wListenPort;
	WORD   wLocalSequence;
	UINT   serverThreadId;
	HANDLE serverThreadHandle;

	__inline bool icqOnline() const
	{	return (m_iStatus != ID_STATUS_OFFLINE && m_iStatus != ID_STATUS_CONNECTING);
	}

	void   __cdecl SendPacketAsyncThread(icq_packet* pArgs);
	void   __cdecl ServerThread(serverthread_start_info* infoParam);

	void   icq_serverDisconnect(BOOL bBlock);
	void   icq_login(const char* szPassword);

	int    handleServerPackets(unsigned char* buf, int len, serverthread_info* info);
	void   sendServPacket(icq_packet* pPacket);
	void   sendServPacketAsync(icq_packet *pPacket);
	int    IsServerOverRate(WORD wFamily, WORD wCommand, int nLevel);

	//----| icq_servlist.cpp |------------------------------------------------------------
	HANDLE hHookSettingChanged;
	HANDLE hHookContactDeleted;
	HANDLE hHookCListGroupChange;
	CRITICAL_SECTION servlistMutex;

	DWORD* pwIDList;
	int    nIDListCount;
	int    nIDListSize;

	// server-list update board
	CRITICAL_SECTION servlistQueueMutex;
	int    servlistQueueCount;
	int    servlistQueueSize;
	ssiqueueditems **servlistQueueList;
	int    servlistQueueState;
	HANDLE servlistQueueThreadHandle;
	int    servlistEditCount;

	void   servlistBeginOperation(int operationCount, int bImport);
	void   servlistEndOperation(int operationCount);

	void   __cdecl servlistQueueThread(void* queueState);

	void   servlistQueueAddGroupItem(servlistgroupitem* pGroupItem, int dwTimeout);
	int    servlistHandlePrimitives(DWORD dwOperation);
	void   servlistProcessLogin();

	void   servlistPostPacket(icq_packet* packet, DWORD dwCookie, DWORD dwOperation, DWORD dwTimeout);
	void   servlistPostPacketDouble(icq_packet* packet1, DWORD dwCookie, DWORD dwOperation, DWORD dwTimeout, icq_packet* packet2, WORD wAction2);

	// server-list pending queue
	int    servlistPendingCount;
	int    servlistPendingSize;
	servlistpendingitem** servlistPendingList;

	int    servlistPendingFindItem(int nType, HANDLE hContact, const char *pszGroup);
	void   servlistPendingAddItem(servlistpendingitem* pItem);
	servlistpendingitem* servlistPendingRemoveItem(int nType, HANDLE hContact, const char *pszGroup);

	void   servlistPendingAddContactOperation(HANDLE hContact, LPARAM param, PENDING_CONTACT_CALLBACK callback, DWORD flags);
	void   servlistPendingAddGroupOperation(const char *pszGroup, LPARAM param, PENDING_GROUP_CALLBACK callback, DWORD flags);
	int    servlistPendingAddContact(HANDLE hContact, WORD wContactID, WORD wGroupID, LPARAM param, PENDING_CONTACT_CALLBACK callback, int bDoInline, LPARAM operationParam = 0, PENDING_CONTACT_CALLBACK operationCallback = NULL);
	int    servlistPendingAddGroup(const char *pszGroup, WORD wGroupID, LPARAM param, PENDING_GROUP_CALLBACK callback, int bDoInline, LPARAM operationParam = 0, PENDING_GROUP_CALLBACK operationCallback = NULL);
	void   servlistPendingRemoveContact(HANDLE hContact, WORD wContactID, WORD wGroupID, int nResult);
	void   servlistPendingRemoveGroup(const char *pszGroup, WORD wGroupID, int nResult);
	void   servlistPendingFlushOperations();

	// server-list support functions
	int    nJustAddedCount;
	int    nJustAddedSize;
	HANDLE* pdwJustAddedList;

	void   AddJustAddedContact(HANDLE hContact);
	BOOL   IsContactJustAdded(HANDLE hContact);
	void   FlushJustAddedContacts();

	WORD   GenerateServerId(int bGroupId);
	WORD   GenerateServerIdPair(int bGroupId, int wCount);
	void   ReserveServerID(WORD wID, int bGroupId);
	void   FreeServerID(WORD wID, int bGroupId);
	BOOL   CheckServerID(WORD wID, unsigned int wCount);
	void   FlushServerIDs();
	void   LoadServerIDs();

	void*  collectGroups(int *count);
	void*  collectBuddyGroup(WORD wGroupID, int *count);
	char*  getServListGroupName(WORD wGroupID);
	void   setServListGroupName(WORD wGroupID, const char *szGroupName);
	WORD   getServListGroupLinkID(const char *szPath);
	void   setServListGroupLinkID(const char *szPath, WORD wGroupID);
	int    IsServerGroupsDefined();
	char*  getServListGroupCListPath(WORD wGroupId);
	char*  getServListUniqueGroupName(const char *szGroupName, int bAlloced);

	int    servlistCreateGroup_gotParentGroup(const char *szGroup, WORD wGroupID, LPARAM param, int nResult);
	int    servlistCreateGroup_Ready(const char *szGroup, WORD groupID, LPARAM param, int nResult);
	void   servlistCreateGroup(const char* szGroupPath, LPARAM param, PENDING_GROUP_CALLBACK callback);
	int    servlistAddContact_gotGroup(const char *szGroup, WORD wGroupID, LPARAM lParam, int nResult);
	int    servlistAddContact_Ready(HANDLE hContact, WORD wContactID, WORD wGroupID, LPARAM lParam, int nResult);
	void   servlistAddContact(HANDLE hContact, const char *pszGroup);
	int    servlistRemoveContact_Ready(HANDLE hContact, WORD contactID, WORD groupID, LPARAM lParam, int nResult);
	void   servlistRemoveContact(HANDLE hContact);
	int    servlistMoveContact_gotTargetGroup(const char *szGroup, WORD wNewGroupID, LPARAM lParam, int nResult);
	int    servlistMoveContact_Ready(HANDLE hContact, WORD contactID, WORD groupID, LPARAM lParam, int nResult);
	void   servlistMoveContact(HANDLE hContact, const char *pszNewGroup);
	int    servlistUpdateContact_Ready(HANDLE hContact, WORD contactID, WORD groupID, LPARAM lParam, int nResult);
	void   servlistUpdateContact(HANDLE hContact);
	int    servlistRenameGroup_Ready(const char *szGroup, WORD wGroupID, LPARAM lParam, int nResult);
	void   servlistRenameGroup(char *szGroup, WORD wGroupId, char *szNewGroup);
	int    servlistRemoveGroup_Ready(const char *szGroup, WORD groupID, LPARAM lParam, int nResult);
	void   servlistRemoveGroup(const char *szGroup, WORD wGroupId);
	void   removeGroupPathLinks(WORD wGroupID);
	int    getServListGroupLevel(WORD wGroupId);

	void   resetServContactAuthState(HANDLE hContact, DWORD dwUin);

	void   FlushSrvGroupsCache();
	int    getCListGroupHandle(const char *szGroup);
	int    getCListGroupExists(const char *szGroup);
	int    moveContactToCListGroup(HANDLE hContact, const char *szGroup); /// TODO: this should be DB function

	DWORD  icq_sendServerItem(DWORD dwCookie, WORD wAction, WORD wGroupId, WORD wItemId, const char *szName, BYTE *pTLVs, int nTlvLength, WORD wItemType, DWORD dwOperation, DWORD dwTimeout, void **doubleObject);
	DWORD  icq_sendServerContact(HANDLE hContact, DWORD dwCookie, WORD wAction, WORD wGroupId, WORD wContactId, DWORD dwOperation, DWORD dwTimeout, void **doubleObject);
	DWORD  icq_sendSimpleItem(DWORD dwCookie, WORD wAction, DWORD dwUin, char* szUID, WORD wGroupId, WORD wItemId, WORD wItemType, DWORD dwOperation, DWORD dwTimeout);
	DWORD  icq_sendServerGroup(DWORD dwCookie, WORD wAction, WORD wGroupId, const char *szName, void *pContent, int cbContent, DWORD dwOperationFlags);

	DWORD  icq_modifyServerPrivacyItem(HANDLE hContact, DWORD dwUin, char *szUid, WORD wAction, DWORD dwOperation, WORD wItemId, WORD wType);
	DWORD  icq_removeServerPrivacyItem(HANDLE hContact, DWORD dwUin, char *szUid, WORD wItemId, WORD wType);
	DWORD  icq_addServerPrivacyItem(HANDLE hContact, DWORD dwUin, char *szUid, WORD wItemId, WORD wType);

	time_t dwLastCListGroupsChange;

	int __cdecl ServListDbSettingChanged(WPARAM wParam, LPARAM lParam);
	int __cdecl ServListDbContactDeleted(WPARAM wParam, LPARAM lParam);
	int __cdecl ServListCListGroupChange(WPARAM wParam, LPARAM lParam);

	//----| stdpackets.cpp |----------------------------------------------------------
	void   icq_sendCloseConnection();

	void   icq_requestnewfamily(WORD wFamily, void (CIcqProto::*familyhandler)(HANDLE hConn, char* cookie, WORD cookieLen));

	void   icq_setidle(int bAllow);
	void   icq_setstatus(WORD wStatus, const char *szStatusNote = NULL);
	DWORD  icq_sendGetInfoServ(HANDLE, DWORD, int);
	DWORD  icq_sendGetAimProfileServ(HANDLE hContact, char *szUid);
	DWORD  icq_sendGetAwayMsgServ(HANDLE, DWORD, int, WORD);
	DWORD  icq_sendGetAwayMsgServExt(HANDLE hContact, DWORD dwUin, int type, WORD wVersion);
	DWORD  icq_sendGetAimAwayMsgServ(HANDLE hContact, char *szUID, int type);
	void   icq_sendSetAimAwayMsgServ(const char *szMsg);

	DWORD  icq_sendCheckSpamBot(HANDLE hContact, DWORD dwUIN, const char *szUID);

	void   icq_sendFileSendServv7(filetransfer* ft, const char *szFiles);
	void   icq_sendFileSendServv8(filetransfer* ft, const char *szFiles, int nAckType);

	void   icq_sendFileAcceptServ(DWORD dwUin, filetransfer *ft, int nAckType);
	void   icq_sendFileAcceptServv7(DWORD dwUin, DWORD TS1, DWORD TS2, DWORD dwCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted, int nAckType);
	void   icq_sendFileAcceptServv8(DWORD dwUin, DWORD TS1, DWORD TS2, DWORD dwCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted, int nAckType);

	void   icq_sendFileDenyServ(DWORD dwUin, filetransfer* ft, const char *szReason, int nAckType);

	DWORD  icq_sendAdvancedSearchServ(BYTE *fieldsBuffer,int bufferLen);
	DWORD  icq_changeUserPasswordServ(const char *szPassword);
  DWORD  icq_changeUserDirectoryInfoServ(const BYTE *pData, WORD wDataLen, BYTE bRequestType);
	void   icq_sendGenericContact(DWORD dwUin, const char *szUid, WORD wFamily, WORD wSubType);
	void   icq_sendNewContact(DWORD dwUin, const char *szUid);
	void   icq_sendChangeVisInvis(HANDLE hContact, DWORD dwUin, char* szUID, int list, int add);
	void   icq_sendEntireVisInvisList(int);
	void   icq_sendAwayMsgReplyServ(DWORD, DWORD, DWORD, WORD, WORD, BYTE, char **);
	DWORD  icq_sendSMSServ(const char *szPhoneNumber, const char *szMsg);
	void   icq_sendMessageCapsServ(DWORD dwUin);
	void   icq_sendRevokeAuthServ(DWORD dwUin, char *szUid);
	void   icq_sendGrantAuthServ(DWORD dwUin, const char *szUid, const char *szMsg);
	void   icq_sendAuthReqServ(DWORD dwUin, char* szUid, const char *szMsg);
	void   icq_sendAuthResponseServ(DWORD dwUin, char* szUid,int auth,const char *szReason);
	void   icq_sendYouWereAddedServ(DWORD,DWORD);

	DWORD  sendDirectorySearchPacket(const BYTE *pSearchData, WORD wDataLen, WORD wPage, BOOL bOnlineUsersOnly);
  DWORD  sendTLVSearchPacket(BYTE bType, char* pSearchDataBuf, WORD wSearchType, WORD wInfoLen, BOOL bOnlineUsersOnly);
	void   sendOwnerInfoRequest(void);
  DWORD  sendUserInfoMultiRequest(BYTE *pRequestData, WORD wDataLen, int nItems);

	DWORD  icq_SendChannel1Message(DWORD dwUin, char *szUID, HANDLE hContact, char *pszText, cookie_message_data *pCookieData);
	DWORD  icq_SendChannel1MessageW(DWORD dwUin, char *szUID, HANDLE hContact, WCHAR *pszText, cookie_message_data *pCookieData); // UTF-16
	DWORD  icq_SendChannel2Message(DWORD dwUin, HANDLE hContact, const char *szMessage, int nBodyLength, WORD wPriority, cookie_message_data *pCookieData, char *szCap);
	DWORD  icq_SendChannel2Contacts(DWORD dwUin, char *szUid, HANDLE hContact, const char *pData, WORD wDataLen, const char *pNames, WORD wNamesLen, cookie_message_data *pCookieData);
	DWORD  icq_SendChannel4Message(DWORD dwUin, HANDLE hContact, BYTE bMsgType, WORD wMsgLen, const char *szMsg, cookie_message_data *pCookieData);

	void   icq_sendAdvancedMsgAck(DWORD, DWORD, DWORD, WORD, BYTE, BYTE);
	void   icq_sendContactsAck(DWORD dwUin, char *szUid, DWORD dwMsgID1, DWORD dwMsgID2);
		  
	void   icq_sendReverseReq(directconnect *dc, DWORD dwCookie, cookie_message_data *pCookie);
	void   icq_sendReverseFailed(directconnect* dc, DWORD dwMsgID1, DWORD dwMsgID2, DWORD dwCookie);
		  
	void   icq_sendXtrazRequestServ(DWORD dwUin, DWORD dwCookie, char* szBody, int nBodyLen, cookie_message_data *pCookieData);
	void   icq_sendXtrazResponseServ(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szBody, int nBodyLen, int nType);

	DWORD  SearchByUin(DWORD dwUin);
	DWORD  SearchByNames(const char *pszNick, const char *pszFirstName, const char *pszLastName, WORD wPage);
	DWORD  SearchByMail(const char *pszEmail);
		   
	DWORD  icq_searchAimByEmail(const char* pszEmail, DWORD dwSearchId);
		  
	void   oft_sendFileRequest(DWORD dwUin, char *szUid, oscar_filetransfer *ft, const char *pszFiles, DWORD dwLocalInternalIP);
	void   oft_sendFileAccept(DWORD dwUin, char *szUid, oscar_filetransfer *ft);
	void   oft_sendFileDeny(DWORD dwUin, char *szUid, oscar_filetransfer *ft);
	void   oft_sendFileCancel(DWORD dwUin, char *szUid, oscar_filetransfer *ft);
	void   oft_sendFileResponse(DWORD dwUin, char *szUid, oscar_filetransfer *ft, WORD wResponse);
	void   oft_sendFileRedirect(DWORD dwUin, char *szUid, oscar_filetransfer *ft, DWORD dwIP, WORD wPort, int bProxy);
		    
	//---- | icq_svcs.cpp |----------------------------------------------------------------
	HANDLE AddToListByUIN(DWORD dwUin, DWORD dwFlags);
	HANDLE AddToListByUID(char *szUID, DWORD dwFlags);

	void   ICQAddRecvEvent(HANDLE hContact, WORD wType, PROTORECVEVENT* pre, DWORD cbBlob, PBYTE pBlob, DWORD flags);

	//----| icq_uploadui.cpp |------------------------------------------------------------
	void   ShowUploadContactsDialog(void);

	//----| icq_xstatus.cpp |-------------------------------------------------------------
	int    m_bHideXStatusUI;
	int    bStatusMenu;
	HANDLE hHookExtraIconsRebuild;
	HANDLE hHookStatusBuild;
	HANDLE hHookExtraIconsApply;
  int    bXStatusExtraIconsReady;
	HANDLE hXStatusExtraIcons[XSTATUS_COUNT];
	IcqIconHandle hXStatusIcons[XSTATUS_COUNT];
	HANDLE hXStatusItems[XSTATUS_COUNT + 1];

	int    hXStatusCListIcons[XSTATUS_COUNT];
	BOOL   bXStatusCListIconsValid[XSTATUS_COUNT];

	void   InitXStatusItems(BOOL bAllowStatus);
	BYTE   getContactXStatus(HANDLE hContact);
	DWORD  sendXStatusDetailsRequest(HANDLE hContact, int bForced);
	DWORD  requestXStatusDetails(HANDLE hContact, BOOL bAllowDelay);
	HICON  getXStatusIcon(int bStatus, UINT flags);
	void   setXStatusEx(BYTE bXStatus, BYTE bQuiet);
	void   setContactExtraIcon(HANDLE hContact, HANDLE hIcon);
	void   handleXStatusCaps(HANDLE hContact, BYTE *caps, int capsize, char *moods, int moodsize);
	void   updateServerCustomStatus(int fullUpdate);

	void   InitXStatusIcons();
  void   UninitXStatusIcons();

	//----| icq_xtraz.cpp |---------------------------------------------------------------
	void   handleXtrazNotify(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szMsg, int nMsgLen, BOOL bThruDC);
	void   handleXtrazNotifyResponse(DWORD dwUin, HANDLE hContact, WORD wCookie, char* szMsg, int nMsgLen);
		  
	void   handleXtrazInvitation(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szMsg, int nMsgLen, BOOL bThruDC);
	void   handleXtrazData(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szMsg, int nMsgLen, BOOL bThruDC);

	DWORD  SendXtrazNotifyRequest(HANDLE hContact, char* szQuery, char* szNotify, int bForced);
	void   SendXtrazNotifyResponse(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szResponse, int nResponseLen, BOOL bThruDC);

	//----| init.cpp |--------------------------------------------------------------------
	void   UpdateGlobalSettings();

	//----| loginpassword.cpp |-----------------------------------------------------------
	void   RequestPassword();

	//----| oscar_filetransfer.cpp |------------------------------------------------------
	CRITICAL_SECTION oftMutex;
	int fileTransferCount;
	basic_filetransfer** fileTransferList;

	oscar_filetransfer* CreateOscarTransfer();
	filetransfer *CreateIcqFileTransfer();
	void   ReleaseFileTransfer(void *ft);
	void   SafeReleaseFileTransfer(void **ft);
	oscar_filetransfer* FindOscarTransfer(HANDLE hContact, DWORD dwID1, DWORD dwID2);

	oscar_listener* CreateOscarListener(oscar_filetransfer *ft, NETLIBNEWCONNECTIONPROC_V2 handler);
	void   ReleaseOscarListener(oscar_listener **pListener);

	void   OpenOscarConnection(HANDLE hContact, oscar_filetransfer *ft, int type);
	void   CloseOscarConnection(oscar_connection *oc);
	int    CreateOscarProxyConnection(oscar_connection *oc);

	int    getFileTransferIndex(void *ft);
	int    IsValidFileTransfer(void *ft);
	int    IsValidOscarTransfer(void *ft);

	void   handleRecvServMsgOFT(BYTE *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwID1, DWORD dwID2, WORD wCommand);
	void   handleRecvServResponseOFT(BYTE *buf, WORD wLen, DWORD dwUin, char *szUID, void* ft);

	HANDLE oftInitTransfer(HANDLE hContact, DWORD dwUin, char *szUid, char** files, const char* pszDesc);
	HANDLE oftFileAllow(HANDLE hContact, HANDLE hTransfer, const char* szPath);
	DWORD  oftFileDeny(HANDLE hContact, HANDLE hTransfer, const char* reazon);
	DWORD  oftFileCancel(HANDLE hContact, HANDLE hTransfer);
	void   oftFileResume(oscar_filetransfer *ft, int action, const char *szFilename);

	void   sendOscarPacket(oscar_connection *oc, icq_packet *packet);
	void   handleOFT2FramePacket(oscar_connection *oc, WORD datatype, BYTE *pBuffer, WORD wLen);
	void   sendOFT2FramePacket(oscar_connection *oc, WORD datatype);

	void   proxy_sendInitTunnel(oscar_connection *oc);
	void   proxy_sendJoinTunnel(oscar_connection *oc, WORD wPort);

	//----| stdpackets.cpp |--------------------------------------------------------------
	void   __cdecl oft_connectionThread(struct oscarthreadstartinfo *otsi);

	int    oft_handlePackets(oscar_connection *oc, unsigned char *buf, int len);
	int    oft_handleFileData(oscar_connection *oc, unsigned char *buf, int len);
	int    oft_handleProxyData(oscar_connection *oc, unsigned char *buf, int len);
	void   oft_sendFileData(oscar_connection *oc);
	void   oft_sendPeerInit(oscar_connection *oc);
	void   oft_sendFileReply(DWORD dwUin, char *szUid, oscar_filetransfer *ft, WORD wResult);

	//----| upload.cpp |------------------------------------------------------------------
	int    StringToListItemId(const char *szSetting,int def);

	//----| utilities.cpp |---------------------------------------------------------------
	int    BroadcastAck(HANDLE hContact,int type,int result,HANDLE hProcess,LPARAM lParam);
	char*  ConvertMsgToUserSpecificAnsi(HANDLE hContact, const char* szMsg);
	WORD   GetMyStatusFlags();
	char*  GetUserPassword(BOOL bAlways);
	DWORD  ReportGenericSendError(HANDLE hContact, int nType, const char* szErrorMsg);
	void   SetCurrentStatus(int nStatus);

	void   ForkThread( IcqThreadFunc pFunc, void* arg );
	HANDLE ForkThreadEx( IcqThreadFunc pFunc, void* arg, UINT* threadID = NULL );
	
	void   __cdecl ProtocolAckThread(icq_ack_args* pArguments);
	void   SendProtoAck(HANDLE hContact, DWORD dwCookie, int nAckResult, int nAckType, char* pszMessage);

	HANDLE CreateProtoEvent(const char* szEvent);
	void   CreateProtoService(const char* szService, IcqServiceFunc serviceProc);
	void   CreateProtoServiceParam(const char* szService, IcqServiceFuncParam serviceProc, LPARAM lParam);
	HANDLE HookProtoEvent(const char* szEvent, IcqEventFunc pFunc);

	int    NetLog_Server(const char *fmt,...);
	int    NetLog_Direct(const char *fmt,...);
	int    NetLog_Uni(BOOL bDC, const char *fmt,...);

	CRITICAL_SECTION contactsCacheMutex;
	LIST<icq_contacts_cache> contactsCache;

	void   AddToContactsCache(HANDLE hContact, DWORD dwUin, const char *szUid);
	void   DeleteFromContactsCache(HANDLE hContact);
	void   InitContactsCache();
	void   UninitContactsCache();

	void   AddToSpammerList(DWORD dwUIN);
	BOOL   IsOnSpammerList(DWORD dwUIN);

	HANDLE NetLib_BindPort(NETLIBNEWCONNECTIONPROC_V2 pFunc, void* lParam, WORD* pwPort, DWORD* pdwIntIP);

	HANDLE HandleFromCacheByUid(DWORD dwUin, const char *szUid);
	HANDLE HContactFromUIN(DWORD dwUin, int *Added);
	HANDLE HContactFromUID(DWORD dwUin, const char *szUid, int *Added);
	HANDLE HContactFromAuthEvent(HANDLE hEvent);

	void   ResetSettingsOnListReload();
	void   ResetSettingsOnConnect();
	void   ResetSettingsOnLoad();

  int    IsMetaInfoChanged(HANDLE hContact);

  char   *setStatusNoteText, *setStatusMoodData;
  void   __cdecl SetStatusNoteThread(void *pArguments);
  int    SetStatusNote(const char *szStatusNote, DWORD dwDelay, int bForced);
  int    SetStatusMood(const char *szMoodData, DWORD dwDelay);

	BOOL   writeDbInfoSettingString(HANDLE hContact, const char* szSetting, char** buf, WORD* pwLength);
	BOOL   writeDbInfoSettingWord(HANDLE hContact, const char *szSetting, char **buf, WORD* pwLength);
	BOOL   writeDbInfoSettingWordWithTable(HANDLE hContact, const char *szSetting, const FieldNamesItem *table, char **buf, WORD* pwLength);
	BOOL   writeDbInfoSettingByte(HANDLE hContact, const char *pszSetting, char **buf, WORD* pwLength);
	BOOL   writeDbInfoSettingByteWithTable(HANDLE hContact, const char *szSetting, const FieldNamesItem *table, char **buf, WORD* pwLength);

  void   writeDbInfoSettingTLVStringUtf(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv);
  void   writeDbInfoSettingTLVString(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv);
  void   writeDbInfoSettingTLVWord(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv);
  void   writeDbInfoSettingTLVByte(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv);
  void   writeDbInfoSettingTLVDouble(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv);
  void   writeDbInfoSettingTLVDate(HANDLE hContact, const char *szSettingYear, const char *szSettingMonth, const char *szSettingDay, oscar_tlv_chain *chain, WORD wTlv);
  void   writeDbInfoSettingTLVTimestamp(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv);
  void   writeDbInfoSettingTLVBlob(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv);

  char** MirandaStatusToAwayMsg(int nStatus);

	BOOL   validateStatusMessageRequest(HANDLE hContact, WORD byMessageType);
};

#endif
