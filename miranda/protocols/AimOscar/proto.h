/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2011 Boris Krasnovskiy

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

#ifndef _AIM_PROTO_H_
#define _AIM_PROTO_H_

#include "m_protoint.h"

struct CAimProto;
typedef void    ( __cdecl CAimProto::*AimThreadFunc )( void* );
typedef int     ( __cdecl CAimProto::*AimEventFunc )( WPARAM, LPARAM );
typedef INT_PTR ( __cdecl CAimProto::*AimServiceFunc )( WPARAM, LPARAM );
typedef INT_PTR ( __cdecl CAimProto::*AimServiceFuncParam )( WPARAM, LPARAM, LPARAM );

struct CAimProto : public PROTO_INTERFACE
{
	CAimProto( const char*, const TCHAR* );
	~CAimProto();

	__inline void* operator new( size_t size )
	{ return calloc( 1, size ); }
	__inline void operator delete( void* p )
	{ free( p ); }

	//====================================================================================
	// PROTO_INTERFACE
	//====================================================================================

	virtual	HANDLE __cdecl AddToList( int flags, PROTOSEARCHRESULT* psr );
	virtual	HANDLE __cdecl AddToListByEvent( int flags, int iContact, HANDLE hDbEvent );

	virtual	int    __cdecl Authorize( HANDLE hContact );
	virtual	int    __cdecl AuthDeny( HANDLE hContact, const TCHAR* szReason );
	virtual	int    __cdecl AuthRecv( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl AuthRequest( HANDLE hContact, const TCHAR* szMessage );

	virtual	HANDLE __cdecl ChangeInfo( int iInfoType, void* pInfoData );

	virtual	HANDLE __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szPath );
	virtual	int    __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer );
	virtual	int    __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szReason );
	virtual	int    __cdecl FileResume( HANDLE hTransfer, int* action, const PROTOCHAR** szFilename );

	virtual	DWORD_PTR __cdecl GetCaps( int type, HANDLE hContact = NULL );
	virtual	HICON  __cdecl GetIcon( int iconIndex );
	virtual	int    __cdecl GetInfo( HANDLE hContact, int infoType );

	virtual	HANDLE __cdecl SearchBasic( const PROTOCHAR* id );
	virtual	HANDLE __cdecl SearchByEmail( const PROTOCHAR* email );
	virtual	HANDLE __cdecl SearchByName( const PROTOCHAR* nick, const PROTOCHAR* firstName, const PROTOCHAR* lastName );
	virtual	HWND   __cdecl SearchAdvanced( HWND owner );
	virtual	HWND   __cdecl CreateExtendedSearchUI( HWND owner );

	virtual	int    __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvFile( HANDLE hContact, PROTOFILEEVENT* );
	virtual	int    __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* );
	virtual	int    __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* );

	virtual	int    __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList );
	virtual	HANDLE __cdecl SendFile( HANDLE hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles );
	virtual	int    __cdecl SendMsg( HANDLE hContact, int flags, const char* msg );
	virtual	int    __cdecl SendUrl( HANDLE hContact, int flags, const char* url );

	virtual	int    __cdecl SetApparentMode( HANDLE hContact, int mode );
	virtual	int    __cdecl SetStatus( int iNewStatus );

	virtual	HANDLE __cdecl GetAwayMsg( HANDLE hContact );
	virtual	int    __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt );
	virtual	int    __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg );
	virtual	int    __cdecl SetAwayMsg( int m_iStatus, const TCHAR* msg );

	virtual	int    __cdecl UserIsTyping( HANDLE hContact, int type );

	virtual	int    __cdecl OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam );

	//====| Services |====================================================================
	INT_PTR  __cdecl SvcCreateAccMgrUI(WPARAM wParam, LPARAM lParam);

	INT_PTR  __cdecl GetAvatarInfo(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetMyAwayMsg(WPARAM wParam,LPARAM lParam);

	INT_PTR  __cdecl GetAvatar(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl SetAvatar(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetAvatarCaps(WPARAM wParam, LPARAM lParam);

	INT_PTR  __cdecl ManageAccount(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl InstantIdle(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl JoinChatUI(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetHTMLAwayMsg(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl GetProfile(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl EditProfile(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl AddToServerList(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl BlockBuddy(WPARAM wParam, LPARAM lParam);

	INT_PTR  __cdecl OnJoinChat(WPARAM wParam, LPARAM lParam);
	INT_PTR  __cdecl OnLeaveChat(WPARAM wParam, LPARAM lParam);

	//====| Events |======================================================================
	int  __cdecl OnContactDeleted(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnGroupChange(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnIdleChanged(WPARAM wParam, LPARAM lParam);
	int  __cdecl OnWindowEvent(WPARAM wParam, LPARAM lParam);
	int  __cdecl OnModulesLoaded( WPARAM wParam, LPARAM lParam );
	int  __cdecl OnOptionsInit(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnPreBuildContactMenu(WPARAM wParam,LPARAM lParam);
//    int  __cdecl OnPreShutdown(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnDbSettingChanged(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnUserInfoInit(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnGCEvent(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnGCMenuHook(WPARAM wParam,LPARAM lParam);

	//====| Data |========================================================================
	CRITICAL_SECTION SendingMutex;
	CRITICAL_SECTION connMutex;

	char* COOKIE;
	int COOKIE_LENGTH;
	char* MAIL_COOKIE;
	int MAIL_COOKIE_LENGTH;
	char* AVATAR_COOKIE;
	int AVATAR_COOKIE_LENGTH;
	char* CHATNAV_COOKIE;
	int CHATNAV_COOKIE_LENGTH;
	char* ADMIN_COOKIE;
	int ADMIN_COOKIE_LENGTH;

	char *username;
	unsigned short seqno;//main connection sequence number
	int state;//m_iStatus of the connection; e.g. whether connected or not
	unsigned short port;

	//Some bools to keep track of different things
	bool request_HTML_profile;
	bool request_away_message;
	bool extra_icons_loaded;
	bool idle;
	bool instantidle;
	bool list_received;

	//Some main connection stuff
	HANDLE hServerConn;//handle to the main connection
	HANDLE hNetlib;//handle to netlib
	unsigned long internal_ip;  // our ip
	unsigned long detected_ip;  // our ip
	unsigned short local_port;  // our port
	
	//Peer connection stuff
	HANDLE hNetlibPeer;//handle to the peer netlib
	HANDLE hDirectBoundPort;//direct connection listening port

	//Handles for the context menu items
	HGENMENU hMenuRoot;
	HGENMENU hHTMLAwayContextMenuItem;
	HGENMENU hAddToServerListContextMenuItem;
	HGENMENU hReadProfileMenuItem;
	HGENMENU hBlockContextMenuItem;
	HGENMENU hMainMenu[3];

	//Some mail connection stuff
	HANDLE hMailConn;
	unsigned short mail_seqno;
	
	//avatar connection stuff
	bool init_cst_fld_ran;
	unsigned short avatar_seqno;
	unsigned short avatar_id_sm;
	unsigned short avatar_id_lg;
	HANDLE hAvatarConn;
	HANDLE hAvatarEvent;
	HANDLE hAvatarsFolder;

	ft_list_type ft_list;

	//chatnav connection stuff
	unsigned short chatnav_seqno;
	HANDLE hChatNavConn;
	HANDLE hChatNavEvent;
	char MAX_ROOMS;

	OBJLIST<chat_list_item> chat_rooms;

	//admin connection stuff
	unsigned short admin_seqno;
	HANDLE hAdminConn;
	HANDLE hAdminEvent;

	// privacy settings
	unsigned long pd_flags;
	unsigned short pd_info_id;
	char pd_mode;

	// prefernces
	unsigned short pref1_id;
	unsigned long pref1_flags;
	unsigned long pref1_set_flags;
	unsigned long pref2_len;
	unsigned long pref2_set_len;
	char *pref2_flags;
	char *pref2_set_flags;

	BdList allow_list;
	BdList block_list;

	BdList group_list;

	//away message retrieval stuff
	char *modeMsgs[9];
	char *last_status_msg; 

	//////////////////////////////////////////////////////////////////////////////////////
	// avatars.cpp

	char *hash_sm, *hash_lg;

	void   __cdecl avatar_request_thread( void* param );
	void   __cdecl avatar_upload_thread( void* param );

	void   avatar_request_handler(HANDLE hContact, char* hash, unsigned char type);
	void   avatar_retrieval_handler(const char* sn, const char* hash, const char* data, int data_len);
	int    get_avatar_filename(HANDLE hContact, char* pszDest, size_t cbLen, const char *ext);
	void   init_custom_folders(void);

	//////////////////////////////////////////////////////////////////////////////////////
	// away.cpp

	void   __cdecl get_online_msg_thread( void* arg );

	int    aim_set_away(HANDLE hServerConn, unsigned short &seqno, const char *msg, bool set);//user info
	int    aim_set_statusmsg(HANDLE hServerConn,unsigned short &seqno,const char *msg);//user info
	int    aim_set_status(HANDLE hServerConn,unsigned short &seqno,unsigned long status_type);
	int    aim_query_away_message(HANDLE hServerConn,unsigned short &seqno,const char* sn);

	char**  get_status_msg_loc(int status);

	//////////////////////////////////////////////////////////////////////////////////////
	// chat.cpp

	void   __cdecl chatnav_request_thread( void* param );

	void chat_register(void);
	void chat_start(const char* id, unsigned short exchange);
	void chat_event(const char* id, const char* sn, int evt, const TCHAR* msg = NULL);
	void chat_leave(const char* id);

	chat_list_item* find_chat_by_cid(unsigned short cid);
	chat_list_item* find_chat_by_id(char* id);
	chat_list_item* find_chat_by_conn(HANDLE conn);
	void remove_chat_by_ptr(chat_list_item* item);
	void shutdown_chat_conn(void);
	void close_chat_conn(void);

	//////////////////////////////////////////////////////////////////////////////////////
	// client.cpp

	int    aim_send_connection_packet(HANDLE hServerConn,unsigned short &seqno,char *buf);
	int    aim_authkey_request(HANDLE hServerConn,unsigned short &seqno);
	int    aim_auth_request(HANDLE hServerConn,unsigned short &seqno,const char* key,const char* language,
							const char* country, const char* username, const char* password);
	int    aim_send_cookie(HANDLE hServerConn,unsigned short &seqno,int cookie_size,char * cookie);
	int    aim_send_service_request(HANDLE hServerConn,unsigned short &seqno);
	int    aim_new_service_request(HANDLE hServerConn,unsigned short &seqno,unsigned short service);
	int    aim_request_rates(HANDLE hServerConn,unsigned short &seqno);
	int    aim_request_icbm(HANDLE hServerConn,unsigned short &seqno);
	int    aim_request_offline_msgs(HANDLE hServerConn,unsigned short &seqno);
	int    aim_set_icbm(HANDLE hServerConn,unsigned short &seqno);
	int    aim_set_profile(HANDLE hServerConn,unsigned short &seqno,char* amsg);//user info
	int    aim_request_list(HANDLE hServerConn,unsigned short &seqno);
	int    aim_activate_list(HANDLE hServerConn,unsigned short &seqno);
	int    aim_accept_rates(HANDLE hServerConn,unsigned short &seqno);
	int    aim_set_caps(HANDLE hServerConn,unsigned short &seqno);
	int    aim_client_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_mail_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_avatar_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_chatnav_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_chat_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_send_message(HANDLE hServerConn,unsigned short &seqno,const char* sn,char* amsg,bool auto_response, bool blast);
	int    aim_query_profile(HANDLE hServerConn,unsigned short &seqno,char* sn);
	int    aim_delete_contact(HANDLE hServerConn,unsigned short &seqno,char* sn,unsigned short item_id,unsigned short group_id,unsigned short list, bool nil);
	int    aim_add_contact(HANDLE hServerConn,unsigned short &seqno,const char* sn,unsigned short item_id,unsigned short group_id,unsigned short list,char* nick=NULL, char* note=NULL);
	int    aim_mod_group(HANDLE hServerConn,unsigned short &seqno,const char* name,unsigned short group_id,char* members,unsigned short members_length);
	int    aim_mod_buddy(HANDLE hServerConn,unsigned short &seqno,const char* sn,unsigned short buddy_id,unsigned short group_id,char* nick,char* note);
	int    aim_ssi_update(HANDLE hServerConn, unsigned short &seqno, bool start);
	int    aim_ssi_update_preferences(HANDLE hServerConn, unsigned short &seqno);
	int    aim_keepalive(HANDLE hServerConn,unsigned short &seqno);
	int    aim_send_file(HANDLE hServerConn,unsigned short &seqno,unsigned long ip, unsigned short port, bool force_proxy, file_transfer *ft);//used when requesting a regular file transfer
	int    aim_file_ad(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie,bool deny,unsigned short max_ver);
	int    aim_typing_notification(HANDLE hServerConn,unsigned short &seqno,char* sn,unsigned short type);
	int    aim_set_idle(HANDLE hServerConn,unsigned short &seqno,unsigned long seconds);
	int    aim_request_mail(HANDLE hServerConn,unsigned short &seqno);
	int    aim_activate_mail(HANDLE hServerConn,unsigned short &seqno);
	int    aim_request_avatar(HANDLE hServerConn,unsigned short &seqno,const char* sn, unsigned short bart_type, const char* hash, unsigned short hash_size);//family 0x0010
	int    aim_set_avatar_hash(HANDLE hServerConn,unsigned short &seqno, char flags, unsigned short bart_type, unsigned short &id, char size, const char* hash);
	int    aim_delete_avatar_hash(HANDLE hServerConn,unsigned short &seqno, char flags, unsigned short bart_type, unsigned short &id);
	int    aim_upload_avatar(HANDLE hServerConn,unsigned short &seqno, unsigned short bart_type, const char* avatar, unsigned short avatar_size);
	int    aim_search_by_email(HANDLE hServerConn,unsigned short &seqno, const char* email);
	int    aim_set_pd_info(HANDLE hServerConn, unsigned short &seqno);
	int    aim_block_buddy(HANDLE hServerConn, unsigned short &seqno, bool remove, const char* sn, unsigned short item_id);
	int    aim_chatnav_request_limits(HANDLE hServerConn,unsigned short &seqno);
	int    aim_chatnav_create(HANDLE hServerConn,unsigned short &seqno, char* room, unsigned short exchage);
	int    aim_chatnav_room_info(HANDLE hServerConn,unsigned short &seqno, char* chat_cookie, unsigned short exchange, unsigned short instance);  
	int    aim_chat_join_room(HANDLE hServerConn,unsigned short &seqno, char* chat_cookie, unsigned short exchange, unsigned short instance,unsigned short id);
	int    aim_chat_send_message(HANDLE hServerConn,unsigned short &seqno, char *amsg);
	int    aim_chat_invite(HANDLE hServerConn,unsigned short &seqno, char* chat_cookie, unsigned short exchange, unsigned short instance, char* sn, char* msg);
	int    aim_chat_deny(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie);
	int    aim_admin_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_admin_format_name(HANDLE hServerConn,unsigned short &seqno, const char* sn);
	int    aim_admin_change_password(HANDLE hServerConn,unsigned short &seqno, const char* cur_pw, const char* new_pw);
	int    aim_admin_change_email(HANDLE hServerConn,unsigned short &seqno, const char* email);
	int    aim_admin_request_info(HANDLE hServerConn,unsigned short &seqno, const unsigned short &type);
	int    aim_admin_account_confirm(HANDLE hServerConn,unsigned short &seqno);

	//////////////////////////////////////////////////////////////////////////////////////
	// connection.cpp

	void    aim_connection_authorization( void );

	void   __cdecl aim_protocol_negotiation( void* );
	void   __cdecl aim_mail_negotiation( void* );
	void   __cdecl aim_avatar_negotiation( void* );
	void   __cdecl aim_chatnav_negotiation( void* );
	void   __cdecl aim_chat_negotiation( void* );
	void   __cdecl aim_admin_negotiation( void* );

	int    LOG(const char *fmt, ...);
	HANDLE aim_connect(const char* server, unsigned short port, bool use_ssl, const char* host = NULL);
	HANDLE aim_peer_connect(const char* ip, unsigned short port);
	HANDLE aim_peer_connect(unsigned long ip, unsigned short port);

	//////////////////////////////////////////////////////////////////////////////////////
	// direct_connect.cpp

	void   __cdecl aim_dc_helper( void* );

	//////////////////////////////////////////////////////////////////////////////////////
	// error.cpp

	void   login_error(unsigned short error);
	void   get_error(unsigned short error);
	void   admin_error(unsigned short error);

	//////////////////////////////////////////////////////////////////////////////////////
	// file.cpp

	int    sending_file(file_transfer *ft, HANDLE hServerPacketRecver, NETLIBPACKETRECVER &packetRecv);
	int    receiving_file(file_transfer *ft, HANDLE hServerPacketRecver, NETLIBPACKETRECVER &packetRecv);
	void   report_file_error(TCHAR* fname);
	void   shutdown_file_transfers(void);

	//////////////////////////////////////////////////////////////////////////////////////
	// packets.cpp

	int    aim_sendflap(HANDLE conn, char type,unsigned short length,const char *buf, unsigned short &seqno);

	//////////////////////////////////////////////////////////////////////////////////////
	// proto.cpp

	void   __cdecl basic_search_ack_success( void* p );
	void   __cdecl email_search_ack_success( void* p );

	//////////////////////////////////////////////////////////////////////////////////////
	// proxy.cpp

	void   __cdecl aim_proxy_helper( void* );

	//////////////////////////////////////////////////////////////////////////////////////
	// server.cpp

	void   snac_md5_authkey(SNAC &snac,HANDLE hServerConn,unsigned short &seqno, const char* username, const char* password);
	int    snac_authorization_reply(SNAC &snac);
	void   snac_supported_families(SNAC &snac, HANDLE hServerConn,unsigned short &seqno);
	void   snac_supported_family_versions(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0001
	void   snac_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0001
	void   snac_mail_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);// family 0x0001
	void   snac_avatar_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);// family 0x0001
	void   snac_chatnav_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);// family 0x0001
	void   snac_chat_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);// family 0x0001
	void   snac_admin_rate_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);// family 0x0001
	void   snac_service_redirect(SNAC &snac);// family 0x0001
	void   snac_self_info(SNAC &snac);//family 0x0001
	void   snac_icbm_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0004
	void   snac_user_online(SNAC &snac);
	void   snac_user_offline(SNAC &snac);
	void   snac_error(SNAC &snac);//family 0x0003 or x0004
	void   snac_contact_list(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);
	void   snac_message_accepted(SNAC &snac);//family 0x004
	void   snac_received_message(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0004
	void   snac_file_decline(SNAC &snac);//family 0x0004
	void   snac_received_info(SNAC &snac);//family 0x0002
	void   snac_typing_notification(SNAC &snac);//family 0x004
	void   snac_list_modification_ack(SNAC &snac);//family 0x0013
	void   snac_mail_response(SNAC &snac);//family 0x0018
	void   snac_retrieve_avatar(SNAC &snac);//family 0x0010
	void   snac_upload_reply_avatar(SNAC &snac);//family 0x0010
	void   snac_email_search_results(SNAC &snac);//family 0x000A
	void   snac_chatnav_info_response(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x000D
	void   snac_chat_joined_left_users(SNAC &snac,chat_list_item* item);//family 0x000E
	void   snac_chat_received_message(SNAC &snac,chat_list_item* item);//family 0x000E
	void   snac_admin_account_infomod(SNAC &snac);//family 0x0007
	void   snac_admin_account_confirm(SNAC &snac);//family 0x0007

	void process_ssi_list(SNAC &snac, int &offset);
	void modify_ssi_list(SNAC &snac, int &offset);
	void delete_ssi_list(SNAC &snac, int &offset);

	//////////////////////////////////////////////////////////////////////////////////////
	// themes.cpp

	void   InitMainMenus(void);
	void   InitContactMenus(void);
	void   RemoveMainMenus(void);
	void   RemoveContactMenus(void);

	//////////////////////////////////////////////////////////////////////////////////////
	// thread.cpp

	void   __cdecl accept_file_thread( void* );

	//////////////////////////////////////////////////////////////////////////////////////
	// utilities.cpp

	struct msg_ack_param { HANDLE hContact; int id; bool success; };

	void   __cdecl msg_ack_success(void*);
	void   __cdecl start_connection(void*);

	void   broadcast_status(int status);
	bool   wait_conn(HANDLE& hConn, HANDLE& hEvent, unsigned short service);
	bool   is_my_contact(HANDLE hContact);
	HANDLE find_chat_contact(const char * room);
	HANDLE contact_from_sn(const char* sn, bool addIfNeeded = false, bool temporary = false);
	void   add_contact_to_group(HANDLE hContact, const char* group);
	void   set_local_nick(HANDLE hContact, char* nick, char* note);
	void   upload_nicks(void);
	void   update_server_group(const char* group, unsigned short group_id);
	void   offline_contacts(void);
	void   offline_contact(HANDLE hContact, bool remove_settings);
	void   execute_cmd(const char* arg);
	unsigned short get_default_port(void);

	int    open_contact_file(const char* sn, const char* file, const char* mode, char* &path, bool contact_dir);
	void   write_away_message(const char* sn, const char* msg, bool utf);
	void   write_profile(const char* sn, const char* msg, bool utf);

	unsigned short getBuddyId(HANDLE hContact, int i);
	unsigned short getGroupId(HANDLE hContact, int i);
	void setBuddyId(HANDLE hContact, int i, unsigned short id);
	void setGroupId(HANDLE hContact, int i, unsigned short id);
	int deleteBuddyId(HANDLE hContact, int i);
	int deleteGroupId(HANDLE hContact, int i);

	unsigned short search_for_free_item_id(HANDLE hbuddy);
	unsigned short* get_members_of_group(unsigned short group_id, unsigned short& size);

	void   ShowPopup( const char* msg, int flags, char* url = 0 );

	//////////////////////////////////////////////////////////////////////////////////////
	HANDLE CreateProtoEvent(const char* szEvent);
	void   CreateProtoService(const char* szService, AimServiceFunc serviceProc);
	void   CreateProtoServiceParam(const char* szService, AimServiceFuncParam serviceProc, LPARAM lParam);
	void   HookProtoEvent(const char* szEvent, AimEventFunc pFunc);
	void   ForkThread(AimThreadFunc, void*);

	int    deleteSetting(HANDLE hContact, const char* setting);

	bool   getBool(HANDLE hContact, const char* name, bool defaultValue);
	int    getByte(const char* name, BYTE defaultValue );
	int    getByte(HANDLE hContact, const char* name, BYTE defaultValue);
	int    getDword(const char* name, DWORD defaultValue);
	int    getDword(HANDLE hContact, const char* name, DWORD defaultValue);
	int    getString(const char* name, DBVARIANT*);
	int    getString(HANDLE hContact, const char* name, DBVARIANT*);
	int    getTString(const char* name, DBVARIANT*);
	int    getTString(HANDLE hContact, const char* name, DBVARIANT*);
	WORD   getWord(const char* name, WORD defaultValue);
	WORD   getWord(HANDLE hContact, const char* name, WORD defaultValue);
	char*  getSetting(HANDLE hContact, const char* setting);
	char*  getSetting(const char* setting);

	void   setByte(const char* name, BYTE value);
	void   setByte(HANDLE hContact, const char* name, BYTE value);
	void   setDword(const char* name, DWORD value);
	void   setDword(HANDLE hContact, const char* name, DWORD value);
	void   setString(const char* name, const char* value);
	void   setString(HANDLE hContact, const char* name, const char* value);
	void   setTString(const char* name, const TCHAR* value);
	void   setTString(HANDLE hContact, const char* name, const TCHAR* value);
	void   setWord(const char* name, WORD value);
	void   setWord(HANDLE hContact, const char* name, WORD value);

	int    sendBroadcast(HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam);
};

#endif
