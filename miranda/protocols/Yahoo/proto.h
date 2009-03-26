
struct CYahooProto;
typedef void ( __cdecl CYahooProto::*YThreadFunc )( void* );
typedef int  ( __cdecl CYahooProto::*YEventFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CYahooProto::*YServiceFunc )( WPARAM, LPARAM );
typedef int  ( __cdecl CYahooProto::*YServiceFuncParam )( WPARAM, LPARAM, LPARAM );

struct yahoo_local_account
{
	char yahoo_id[255];
	char password[255];
	int id;
	int fd;
	int status;
	char *msg;
	int  rpkts;
};

struct CYahooProto : public PROTO_INTERFACE
{
	typedef PROTO_INTERFACE CSuper;

				CYahooProto( const char*, const TCHAR* );
				~CYahooProto();

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

	//====| Events |======================================================================
	int __cdecl OnContactDeleted( WPARAM, LPARAM );
	int __cdecl OnIdleEvent( WPARAM, LPARAM );
	int __cdecl OnModulesLoadedEx( WPARAM, LPARAM );
	int __cdecl OnOptionsInit( WPARAM, LPARAM );
	int __cdecl OnSettingChanged( WPARAM, LPARAM );

	int __cdecl OnABCommand( WPARAM, LPARAM );
	int __cdecl OnCalendarCommand( WPARAM, LPARAM );
	int __cdecl OnEditMyProfile( WPARAM, LPARAM );
	int __cdecl OnGotoMailboxCommand( WPARAM, LPARAM );
	int __cdecl OnRefreshCommand( WPARAM, LPARAM );
	int __cdecl OnShowMyProfileCommand( WPARAM, LPARAM );
	int __cdecl OnShowProfileCommand( WPARAM, LPARAM );

	//====| Services |====================================================================
	int __cdecl  GetName( WPARAM, LPARAM );
	int __cdecl  GetStatus( WPARAM, LPARAM );
	int __cdecl  GetUnreadEmailCount( WPARAM, LPARAM );
	int __cdecl  SendNudge( WPARAM, LPARAM );
	int __cdecl  SetMyAvatar( WPARAM, LPARAM );

	void   BroadcastStatus(int s);
	void   LoadYahooServices( void );
	void   MenuInit( void );

	//====| Data |========================================================================
	struct yahoo_local_account *ylad;

	BOOL   m_bLoggedIn;

	char*  m_szModuleName;
	char*  m_startMsg;

	//====| avatar.cpp |==================================================================
	void __cdecl send_avt_thread(void *psf);
	void __cdecl recv_avatarthread(void *pavt);

	int  __cdecl GetAvatarInfo( WPARAM, LPARAM );
	int  __cdecl GetAvatarCaps( WPARAM, LPARAM );
	int  __cdecl GetMyAvatar( WPARAM, LPARAM );

	void   ext_got_picture(const char *me, const char *who, const char *pic_url, int cksum, int type);
	void   ext_got_picture_checksum(const char *me, const char *who, int cksum);
	void   ext_got_picture_update(const char *me, const char *who, int buddy_icon);
	void   ext_got_picture_status(const char *me, const char *who, int buddy_icon);
	void   ext_got_picture_upload(const char *me, const char *url, unsigned int ts);
	void   ext_got_avatar_share(int buddy_icon);
	
	void   reset_avatar(HANDLE hContact);
	void   request_avatar(const char* who);

	void   SendAvatar(const char *szFile);
	void   GetAvatarFileName(HANDLE hContact, char* pszDest, int cbLen, int type);

	//====| filetransfer.cpp |============================================================
	void __cdecl recv_filethread(void *psf);

	void   ext_got_file(const char *me, const char *who, const char *url, long expires, const char *msg, const char *fname, unsigned long fesize, const char *ft_token, int y7);
	void   ext_got_files(const char *me, const char *who, const char *ft_token, int y7, YList* files);
	void   ext_got_file7info(const char *me, const char *who, const char *url, const char *fname, const char *ft_token);
	void   ext_ft7_send_file(const char *me, const char *who, const char *filename, const char *token, const char *ft_token);

	//====| icolib.cpp |==================================================================
	void   IconsInit( void );
	HICON  LoadIconEx( const char* name );

	//====| ignore.cpp |==================================================================
	const YList* GetIgnoreList(void);
	void  IgnoreBuddy(const char *buddy, int ignore);
	int   BuddyIgnored(const char *who);

	//====| im.cpp |======================================================================
	void   ext_got_im(const char *me, const char *who, int protocol, const char *msg, long tm, int stat, int utf8, int buddy_icon, const char *seqn=NULL, int sendn=0);

	void   send_msg(const char *id, int protocol, const char *msg, int utf8);

	void __cdecl im_sendacksuccess(HANDLE hContact);
	void __cdecl im_sendackfail(HANDLE hContact);
	void __cdecl im_sendackfail_longmsg(HANDLE hContact);

	//====| proto.cpp |===================================================================
	void __cdecl get_status_thread(HANDLE hContact);
	void __cdecl get_info_thread(HANDLE hContact);

	//====| search.cpp |==================================================================
	void __cdecl search_simplethread(void *snsearch);
	void __cdecl searchadv_thread(void *pHWND);

	void   ext_got_search_result(int found, int start, int total, YList *contacts);

	//====| server.cpp |==================================================================
	void __cdecl server_main(void *empty);

	//====| services.cpp |================================================================
	void   logoff_buddies();

	void   OpenURL(const char *url, int autoLogin);

	int __cdecl  SetCustomStatCommand( WPARAM, LPARAM );

	//====| util.cpp |====================================================================
	DWORD  GetByte( const char* valueName, int parDefltValue );
	DWORD  SetByte( const char* valueName, int parValue );

	DWORD  GetDword( const char* valueName, DWORD parDefltValue );
	DWORD  SetDword( const char* valueName, DWORD parValue );

	WORD   GetWord( HANDLE hContact, const char* valueName, int parDefltValue );
	DWORD  SetWord( HANDLE hContact, const char* valueName, int parValue );

	DWORD  SetString( HANDLE hContact, const char* valueName, const char* parValue );
	DWORD  SetStringUtf( HANDLE hContact, const char* valueName, const char* parValue );

	DWORD  Set_Protocol( HANDLE hContact, int protocol );

	int    SendBroadcast( HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam );

	int    ShowNotification(const char *title, const char *info, DWORD flags);
	void   ShowError(const char *title, const char *buff);
	int    ShowPopup( const char* nickname, const char* msg, const char *szURL );

	#ifdef __GNUC__
		int DebugLog( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
	#else
		int DebugLog( const char *fmt, ... );
	#endif

	//====| yahoo.cpp |===================================================================
	HANDLE add_buddy( const char *yahoo_id, const char *yahoo_name, int protocol, DWORD flags );
	const char *find_buddy( const char *yahoo_id);
	HANDLE getbuddyH(const char *yahoo_id);
	void   remove_buddy(const char *who, int protocol);

	void   logout();

	void   accept(const char *who, int protocol);
	void   reject(const char *who, int protocol, const char *msg);
	void   sendtyping(const char *who, int protocol, int stat);
	void   set_status(int myyahooStatus, char *msg, int away);
	void   stealth(const char *buddy, int add);

	int    ext_connect(const char *h, int p, int type);
	int    ext_connect_async(const char *host, int port, int type, yahoo_connect_callback callback, void *data);

	void   ext_send_http_request(const char *method, const char *url, const char *cookies, long content_length, yahoo_get_fd_callback callback, void *callback_data);
	char * ext_send_https_request(struct yahoo_data *yd, const char *host, const char *path);

	void   ext_status_changed(const char *who, int protocol, int stat, const char *msg, int away, int idle, int mobile, int utf8);
	void   ext_status_logon(const char *who, int protocol, int stat, const char *msg, int away, int idle, int mobile, int cksum, int buddy_icon, long client_version, int utf8);
	void   ext_got_audible(const char *me, const char *who, const char *aud, const char *msg, const char *aud_hash);
	void   ext_got_calendar(const char *url, int type, const char *msg, int svc);
	void   ext_got_stealth(char *stealthlist);
	void   ext_got_buddies(YList * buds);
	void   ext_rejected(const char *who, const char *msg);
	void   ext_buddy_added(char *myid, char *who, char *group, int status, int auth);
	void   ext_contact_added(const char *myid, const char *who, const char *fname, const char *lname, const char *msg, int protocol);
	void   ext_typing_notify(const char *me, const char *who, int protocol, int stat);
	void   ext_game_notify(const char *me, const char *who, int stat, const char *msg);
	void   ext_mail_notify(const char *from, const char *subj, int cnt);
	void   ext_system_message(const char *me, const char *who, const char *msg);
	void   ext_got_identities(const char *fname, const char *lname, YList * ids);
	void   ext_got_ping(const char *errormsg);
	void   ext_error(const char *err, int fatal, int num);
	void   ext_login_response(int succ, const char *url);
	void   ext_login(int login_mode);

	void   AddBuddy( const char *who, int protocol, const char *group, const char *msg);


private:
	int    m_startStatus;
	int    m_unreadMessages;

	int    poll_loop;
	long   lLastSend;

	HANDLE hYahooNudge;
	HANDLE m_hNetlibUser;

	void   YCreateService( const char* szService, YServiceFunc serviceProc );
	void   YCreateServiceParam( const char* szService, YServiceFuncParam serviceProc, LPARAM lParam );
	HANDLE YCreateHookableEvent( const char* szService );
	HANDLE YForkThread( YThreadFunc pFunc, void *param );
	void   YHookEvent( const char* szEvent, YEventFunc handler );
};

extern LIST<CYahooProto> g_instances;