#ifndef _JABBER_PROTO_H_
#define _JABBER_PROTO_H_

#include "m_protoint.h"

struct CAimProto;
typedef void ( __cdecl CAimProto::*AimThreadFunc )( void* );
typedef int  ( __cdecl CAimProto::*AimEventFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CAimProto::*AimServiceFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CAimProto::*AimServiceFuncParam )( WPARAM, LPARAM, LPARAM );

struct CAimProto : public PROTO_INTERFACE
{
				CAimProto( const char*, const TCHAR* );
				~CAimProto();

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

	int  __cdecl GetName(WPARAM wParam, LPARAM lParam);
	int  __cdecl GetStatus(WPARAM wParam, LPARAM lParam);
	int  __cdecl GetAvatarInfo(WPARAM wParam, LPARAM lParam);

    int  __cdecl GetAvatar(WPARAM wParam, LPARAM lParam);
    int  __cdecl SetAvatar(WPARAM wParam, LPARAM lParam);
    int  __cdecl GetAvatarCaps(WPARAM wParam, LPARAM lParam);

	int  __cdecl ManageAccount(WPARAM wParam, LPARAM lParam);
	int  __cdecl CheckMail(WPARAM wParam, LPARAM lParam);
	int  __cdecl InstantIdle(WPARAM wParam, LPARAM lParam);
	int  __cdecl JoinChat(WPARAM wParam, LPARAM lParam);
	int  __cdecl GetHTMLAwayMsg(WPARAM wParam, LPARAM lParam);
	int  __cdecl GetProfile(WPARAM wParam, LPARAM lParam);
	int  __cdecl EditProfile(WPARAM wParam, LPARAM lParam);
	int  __cdecl AddToServerList(WPARAM wParam, LPARAM lParam);
    int  __cdecl BlockBuddy(WPARAM wParam, LPARAM lParam);
    int  __cdecl LeaveChat(WPARAM wParam, LPARAM lParam);

	//====| Events |======================================================================
	int  __cdecl OnContactDeleted(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnExtraIconsRebuild( WPARAM wParam, LPARAM lParam );
	int  __cdecl OnExtraIconsApply( WPARAM wParam, LPARAM lParam );
	int  __cdecl OnIdleChanged( WPARAM wParam, LPARAM lParam );
	int  __cdecl OnModulesLoaded( WPARAM wParam, LPARAM lParam );
	int  __cdecl OnOptionsInit(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnPreBuildContactMenu(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnPreShutdown(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnSettingChanged(WPARAM wParam,LPARAM lParam);
	int  __cdecl OnUserInfoInit(WPARAM wParam,LPARAM lParam);
    int  __cdecl OnGCEvent(WPARAM wParam,LPARAM lParam);
    int  __cdecl OnGCMenuHook(WPARAM wParam,LPARAM lParam);

	//====| Data |========================================================================
	char* CWD;//current working directory
	char* GROUP_ID_KEY;
	char* ID_GROUP_KEY;
	char* FILE_TRANSFER_KEY;
	
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
	bool checking_mail;
	bool list_received;

	//Some main connection stuff
	HANDLE hServerConn;//handle to the main connection
	HANDLE hServerPacketRecver;//handle to the listening device
	HANDLE hNetlib;//handle to netlib
	unsigned long InternalIP;// our ip
	unsigned short LocalPort;// our port
	
	//Peer connection stuff
	HANDLE hNetlibPeer;//handle to the peer netlib
	HANDLE hDirectBoundPort;//direct connection listening port
	HANDLE current_rendezvous_accept_user;//hack

	//Handles for the context menu items
	HANDLE hMenuRoot;
	HANDLE hHTMLAwayContextMenuItem;
	HANDLE hAddToServerListContextMenuItem;
	HANDLE hReadProfileMenuItem;
    HANDLE hBlockContextMenuItem;
    HANDLE hLeaveChatMenuItem;

	//Some mail connection stuff
	HANDLE hMailConn;
	unsigned short mail_seqno;
	
	//avatar connection stuff
	unsigned short avatar_seqno;
    unsigned short avatar_id;
	HANDLE hAvatarConn;
	HANDLE hAvatarEvent;
    HANDLE hAvatarsFolder;

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

    OBJLIST<PDList> allow_list;
    OBJLIST<PDList> block_list;

	//away message retrieval stuff
    char* modeMsgs[9];

	//Some Icon handles
	HANDLE bot_icon;
	HANDLE icq_icon;
	HANDLE aol_icon;
	HANDLE hiptop_icon;
	HANDLE admin_icon;
	HANDLE confirmed_icon;
	HANDLE unconfirmed_icon;

    //////////////////////////////////////////////////////////////////////////////////////
	// avatars.cpp

	void   __cdecl avatar_request_thread( void* param );
    void   __cdecl avatar_upload_thread( void* param );

	void   avatar_request_handler(HANDLE hContact, char* hash, int hash_size);
	void   avatar_retrieval_handler(const char* sn, const char* hash, const char* data, int data_len);
    void   get_avatar_filename(HANDLE hContact, char* pszDest, size_t cbLen, const char *ext);

	//////////////////////////////////////////////////////////////////////////////////////
	// away.cpp

    void   __cdecl get_online_msg_thread( void* arg );

    int    aim_set_away(HANDLE hServerConn,unsigned short &seqno,const char *msg);//user info
    int    aim_set_statusmsg(HANDLE hServerConn,unsigned short &seqno,const char *msg);//user info
	int    aim_query_away_message(HANDLE hServerConn,unsigned short &seqno,const char* sn);

    char**  getStatusMsgLoc( int status );

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
	int    aim_set_profile(HANDLE hServerConn,unsigned short &seqno,char *msg);//user info
	int    aim_set_invis(HANDLE hServerConn,unsigned short &seqno,const char* m_iStatus,const char* status_flag);
	int    aim_request_list(HANDLE hServerConn,unsigned short &seqno);
	int    aim_activate_list(HANDLE hServerConn,unsigned short &seqno);
	int    aim_accept_rates(HANDLE hServerConn,unsigned short &seqno);
	int    aim_set_caps(HANDLE hServerConn,unsigned short &seqno);
	int    aim_client_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_mail_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_avatar_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_chatnav_ready(HANDLE hServerConn,unsigned short &seqno);
	int    aim_chat_ready(HANDLE hServerConn,unsigned short &seqno);
    int    aim_send_message(HANDLE hServerConn,unsigned short &seqno,const char* sn,char* msg,bool uni,bool auto_response);
	int    aim_query_profile(HANDLE hServerConn,unsigned short &seqno,char* sn);
	int    aim_delete_contact(HANDLE hServerConn,unsigned short &seqno,char* sn,unsigned short item_id,unsigned short group_id,unsigned short list);
	int    aim_add_contact(HANDLE hServerConn,unsigned short &seqno,const char* sn,unsigned short item_id,unsigned short group_id,unsigned short list);
	int    aim_mod_group(HANDLE hServerConn,unsigned short &seqno,char* name,unsigned short group_id,char* members,unsigned short members_length);
    int    aim_ssi_update(HANDLE hServerConn, unsigned short &seqno, bool start);
	int    aim_keepalive(HANDLE hServerConn,unsigned short &seqno);
	int    aim_send_file(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie,unsigned long ip, unsigned short port, bool force_proxy, unsigned short request_num ,char* file_name,unsigned long total_bytes,char* descr);//used when requesting a regular file transfer
	int    aim_send_file_proxy(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie, char* file_name,unsigned long total_bytes,char* descr,unsigned long proxy_ip, unsigned short port);
	int    aim_file_redirected_request(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie);
	int    aim_file_proxy_request(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie,char request_num,unsigned long proxy_ip, unsigned short port);
	int    aim_accept_file(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie);
	int    aim_file_deny(HANDLE hServerConn,unsigned short &seqno,char* sn,char* icbm_cookie);
	int    aim_typing_notification(HANDLE hServerConn,unsigned short &seqno,char* sn,unsigned short type);
	int    aim_set_idle(HANDLE hServerConn,unsigned short &seqno,unsigned long seconds);
	int    aim_request_mail(HANDLE hServerConn,unsigned short &seqno);
	int    aim_request_avatar(HANDLE hServerConn,unsigned short &seqno,const char* sn, const char* hash, unsigned short hash_size);//family 0x0010
    int    aim_set_avatar_hash(HANDLE hServerConn,unsigned short &seqno, char flags, char size, const char* hash);
    int    aim_upload_avatar(HANDLE hServerConn,unsigned short &seqno, const char* avatar, unsigned short avatar_size);
	int    aim_search_by_email(HANDLE hServerConn,unsigned short &seqno, const char* email);
    int    aim_set_pd_info(HANDLE hServerConn, unsigned short &seqno);
    int    aim_block_buddy(HANDLE hServerConn, unsigned short &seqno, bool remove, const char* sn, unsigned short item_id);
	int	   aim_chatnav_request_limits(HANDLE hServerConn,unsigned short &seqno);
	int	   aim_chatnav_create(HANDLE hServerConn,unsigned short &seqno, char* room, unsigned short exchage);
    int    aim_chatnav_room_info(HANDLE hServerConn,unsigned short &seqno, char* chat_cookie, unsigned short exchange, unsigned short instance);  
    int	   aim_chat_join_room(HANDLE hServerConn,unsigned short &seqno, char* chat_cookie, unsigned short exchange, unsigned short instance,unsigned short id);
	int	   aim_chat_send_message(HANDLE hServerConn,unsigned short &seqno, char* msg, bool uni);
	int	   aim_chat_invite(HANDLE hServerConn,unsigned short &seqno, char* chat_cookie, unsigned short exchange, unsigned short instance, char* sn, char* msg);
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

	void   sending_file(HANDLE hContact, HANDLE hNewConnection);
	void   receiving_file(HANDLE hContact, HANDLE hNewConnection);

	//////////////////////////////////////////////////////////////////////////////////////
	// packets.cpp

	int    aim_writesnac(unsigned short service, unsigned short subgroup,unsigned short &offset,char* out, unsigned short id=0);
	int    aim_writetlv(unsigned short type,unsigned short size, const char* value,unsigned short &offset,char* out);
	int    aim_sendflap(HANDLE conn, char type,unsigned short length,const char *buf, unsigned short &seqno);
	void   aim_writefamily(const char *buf,unsigned short &offset,char* out);
	void   aim_writegeneric(unsigned short size,const char *buf,unsigned short &offset,char* out);
	void   aim_writebartid(unsigned short type, unsigned char flags, unsigned short size,const char *buf,unsigned short &offset,char* out);
    void   aim_writechar(unsigned char val, unsigned short &offset,char* out);
    void   aim_writeshort(unsigned short val, unsigned short &offset,char* out);
    void   aim_writelong(unsigned long val, unsigned short &offset,char* out);

	//////////////////////////////////////////////////////////////////////////////////////
	// proto.cpp

	void   __cdecl basic_search_ack_success( void* p );
	void   __cdecl email_search_ack_success( void* p );
	void   __cdecl SetStatusWorker( void* arg );

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
	void   snac_icbm_limitations(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0004
	void   snac_user_online(SNAC &snac);
	void   snac_user_offline(SNAC &snac);
	void   snac_error(SNAC &snac);//family 0x0003 or x0004
	void   snac_contact_list(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);
	void   snac_message_accepted(SNAC &snac);//family 0x004
	void   snac_received_message(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x0004
	void   snac_busted_payload(SNAC &snac);//family 0x0004
	void   snac_received_info(SNAC &snac);//family 0x0002
	void   snac_typing_notification(SNAC &snac);//family 0x004
	void   snac_list_modification_ack(SNAC &snac);//family 0x0013
	void   snac_mail_response(SNAC &snac);//family 0x0018
	void   snac_retrieve_avatar(SNAC &snac);//family 0x0010
	void   snac_email_search_results(SNAC &snac);//family 0x000A
	void   snac_chatnav_info_response(SNAC &snac,HANDLE hServerConn,unsigned short &seqno);//family 0x000D
	void   snac_chat_joined_left_users(SNAC &snac,chat_list_item* item);//family 0x000E
	void   snac_chat_received_message(SNAC &snac,chat_list_item* item);//family 0x000E
    void   snac_admin_account_infomod(SNAC &snac);//family 0x0007
    void   snac_admin_account_confirm(SNAC &snac);//family 0x0007

    //////////////////////////////////////////////////////////////////////////////////////
	// themes.cpp

	void   InitIcons(void);
	void   InitMenus(void);
	HICON  LoadIconEx(const char* name);
	HANDLE GetIconHandle(const char* name);
	void   ReleaseIconEx(const char* name);

	//////////////////////////////////////////////////////////////////////////////////////
	// thread.cpp

	void   __cdecl accept_file_thread( void* );
	void   __cdecl redirected_file_thread( void* );
	void   __cdecl proxy_file_thread( void* );

	//////////////////////////////////////////////////////////////////////////////////////
	// utilities.cpp

	void   __cdecl msg_ack_success( void* );

	void   broadcast_status(int status);
	void   start_connection(int initial_status);
    bool   wait_conn(HANDLE& hConn, HANDLE& hEvent, unsigned short service);
    bool   is_my_contact(HANDLE hContact);
	HANDLE find_contact(const char * sn);
    HANDLE find_chat_contact(const char * room);
	HANDLE add_contact(char* buddy);
	void   add_contacts_to_groups();
	void   add_contact_to_group(HANDLE hContact, const char* group);
	void   offline_contacts();
	void   offline_contact(HANDLE hContact, bool remove_settings);
	void   add_AT_icons();
	void   remove_AT_icons();
	void   add_ES_icons();
	void   remove_ES_icons();
	void   execute_cmd(const char* arg);

	FILE*  open_contact_file(const char* sn, const char* file, const char* mode, char* &path, bool contact_dir);
	void   write_away_message(const char* sn, const char* msg, bool utf);
	void   write_profile(const char* sn, const char* msg, bool utf);

	unsigned short search_for_free_group_id(char *name);
	unsigned short search_for_free_item_id(HANDLE hbuddy);
	char*  get_members_of_group( WORD group_id, WORD& size);

    unsigned short get_free_list_item_id(OBJLIST<PDList> & list);
    unsigned short find_list_item_id(OBJLIST<PDList> & list, char* sn);
    void   remove_list_item_id(OBJLIST<PDList> & list, unsigned short id);

	void   create_cookie(HANDLE hContact);
	void   read_cookie(HANDLE hContact,char* cookie);
	void   write_cookie(HANDLE hContact,char* cookie);

	void   load_extra_icons();

	void   ShowPopup( const char* title, const char* msg, int flags, char* url = 0 );

	//////////////////////////////////////////////////////////////////////////////////////
	HANDLE CreateProtoEvent(const char* szEvent);
	void   CreateProtoService(const char* szService, AimServiceFunc serviceProc);
	void   CreateProtoServiceParam(const char* szService, AimServiceFuncParam serviceProc, LPARAM lParam);
	void   HookProtoEvent(const char* szEvent, AimEventFunc pFunc);
	void   ForkThread( AimThreadFunc, void* );

	int    deleteSetting( HANDLE hContact, const char* setting );

	int    getByte( const char* name, BYTE defaultValue );
	int    getByte( HANDLE hContact, const char* name, BYTE defaultValue );
	int    getDword( const char* name, DWORD defaultValue );
	int    getDword( HANDLE hContact, const char* name, DWORD defaultValue );
	int    getString( const char* name, DBVARIANT* );
	int    getString( HANDLE hContact, const char* name, DBVARIANT* );
	int    getTString( const char* name, DBVARIANT* );
	int    getTString( HANDLE hContact, const char* name, DBVARIANT* );
	WORD   getWord( const char* name, WORD defaultValue );
	WORD   getWord( HANDLE hContact, const char* name, WORD defaultValue );
    char*  getSetting(HANDLE hContact, const char* setting);

	void   setByte( const char* name, BYTE value );
	void   setByte( HANDLE hContact, const char* name, BYTE value );
	void   setDword( const char* name, DWORD value );
	void   setDword( HANDLE hContact, const char* name, DWORD value );
	void   setString( const char* name, const char* value );
	void   setString( HANDLE hContact, const char* name, const char* value );
	void   setTString( const char* name, const TCHAR* value );
	void   setTString( HANDLE hContact, const char* name, const TCHAR* value );
	void   setWord( const char* name, WORD value );
	void   setWord( HANDLE hContact, const char* name, WORD value );

    int    sendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam );
};

#endif
