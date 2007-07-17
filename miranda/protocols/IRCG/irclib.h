/*
IRC plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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

#ifndef _IRC_H_
#define	_IRC_H_

#pragma warning (disable: 4786)

#include <string>
#include <vector>
#include <map>
#include <set>

//#ifdef IRC_SSL
#include <openssl/ssl.h>
//#include "../../openssl-0.9.7d/include/openssl/err.h"
//#endif

void DoIdent(HANDLE hConnection, DWORD dwRemoteIP, void* extra);
void DoIncomingDcc(HANDLE hConnection, DWORD dwRemoteIP, void* extra);
VOID CALLBACK DCCTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);
unsigned long ConvertIPToInteger(char * IP);
char* ConvertIntegerToIP(unsigned long int_ip_addr);

////////////////////////////////////////////////////////////////////
namespace irc {
////////////////////////////////////////////////////////////////////

typedef std::string String;
#if defined( _UNICODE )
	typedef std::wstring TString;
#else
	typedef std::string TString;
#endif

typedef struct {
	DWORD   dwAdr;
	DWORD   dwSize;
	DWORD   iType;
	TString sToken;
	int     iPort;
	BOOL    bTurbo;
	BOOL    bSSL;
	BOOL    bSender;
	BOOL    bReverse;
	String  sPath;
	String  sFile;
	String  sFileAndPath;
	String  sHostmask;
	HANDLE  hContact;
	TString sContactName;
}
	DCCINFO;

//#ifdef IRC_SSL
// OpenSSL stuff
typedef int			(*tSSL_library_init)		(void);
typedef SSL_CTX*	(*tSSL_CTX_new)				(SSL_METHOD *meth);
typedef SSL*		(*tSSL_new)					(SSL_CTX *ctx);
typedef int			(*tSSL_set_fd)				(SSL *ssl, int fd);
typedef int			(*tSSL_connect)				(SSL *ssl);
typedef int			(*tSSL_read)				(SSL *ssl, void *buf, int num);
typedef int			(*tSSL_write)				(SSL *ssl, const void *buf, int num);
typedef SSL_METHOD*	(*tSSLv23_method)			(void);
typedef int			(*tSSL_get_error)			(SSL *s, int retcode);
typedef void		(*tSSL_load_error_strings)	(void);
typedef int			(*tSSL_shutdown)			(SSL *ssl);
typedef void		(*tSSL_CTX_free)			(SSL_CTX *ctx);
typedef void		(*tSSL_free)				(SSL *ssl);

//typedef char		(*tERR_error_string)		(unsigned long e, char *buf);
//typedef const char*	(*tERR_lib_error_string)	(unsigned long e);
//typedef const char*	(*tERR_func_error_string)	(unsigned long e);
//typedef const char*	(*tERR_reason_error_string)	(unsigned long e);

//static tERR_error_string			pERR_error_string;
//static tERR_lib_error_string		pERR_lib_error_string;
//static tERR_func_error_string		pERR_func_error_string;
//static tERR_reason_error_string		pERR_reason_error_string;
//#endif 

class CIrcMessage
{
public :
	struct Prefix
	{
		TString sNick, sUser, sHost;
	}
		prefix;

	TString sCommand;
	std::vector<TString> parameters;
	bool m_bIncoming;
	bool m_bNotify;
	CIrcMessage() : m_bIncoming(false), m_bNotify(true) {} // default constructor
	CIrcMessage(const TCHAR* lpszCmdLine, bool bIncoming=false, bool bNotify = true); // parser constructor
	CIrcMessage(const CIrcMessage& m); // copy constructor

	void Reset();

	CIrcMessage& operator = (const CIrcMessage& m);
	CIrcMessage& operator = (const TCHAR* lpszCmdLine);

	TString AsString() const;

private :
	void ParseIrcCommand(const TCHAR* lpszCmdLine);
};

////////////////////////////////////////////////////////////////////

struct IIrcSessionMonitor
{
	virtual void OnIrcMessage(const CIrcMessage* pmsg) = 0;
};

////////////////////////////////////////////////////////////////////

struct CIrcSessionInfo
{
	String  sServer;
	TString sServerName;
	TString sNick;
	TString sUserID;
	TString sFullName;
	String  sPassword;
	TString sIdentServerType;
	TString sNetwork;
	bool bIdentServer;
	bool bNickFlag;
	int iSSL;
	unsigned int iIdentServerPort;
	unsigned int iPort;

	CIrcSessionInfo();
	CIrcSessionInfo(const CIrcSessionInfo& si);

	void Reset();
};

////////////////////////////////////////////////////////////////////

//#ifdef IRC_SSL
// Handles to the SSL libraries
static SSL_CTX* m_ssl_ctx;	// SSL context, valid for all threads	

class CSSLSession { 	// OpenSSL

public:
	SSL* m_ssl;
	int nSSLConnected;
	CSSLSession(void) : nSSLConnected(0) {
		m_ssl_ctx = 0;
	}
	~CSSLSession();
	int SSLInit();
	int SSLConnect(HANDLE con);
	int SSLDisconnect(void);

};

//#endif
////////////////////////////////////////////////////////////////////

class CIrcDefaultMonitor; // foreward
class CDccSession; // forward

//#include <string.h>

typedef std::map<HANDLE, CDccSession*> DccSessionMap;
typedef std::pair<HANDLE, CDccSession*> DccSessionPair;

class CIrcSession
{
public :
	friend class CIrcDefaultMonitor;

	CIrcSession(IIrcSessionMonitor* pMonitor = NULL);
	virtual ~CIrcSession();

	void AddIrcMonitor(IIrcSessionMonitor* pMonitor);
	void RemoveMonitor(IIrcSessionMonitor* pMonitor);

	void AddDCCSession(HANDLE hContact, CDccSession * dcc);
	void AddDCCSession(DCCINFO*  pdci, CDccSession * dcc);
	void RemoveDCCSession(HANDLE hContact);
	void RemoveDCCSession(DCCINFO*  pdci);
	
	CDccSession* FindDCCSession(HANDLE hContact);
	CDccSession* FindDCCSession(DCCINFO* pdci);
	CDccSession* FindDCCSendByPort(int iPort);
	CDccSession* FindDCCRecvByPortAndName(int iPort, const TCHAR* szName);
	CDccSession* FindPassiveDCCSend(int iToken);
	CDccSession* FindPassiveDCCRecv(TString sName, TString sToken);
	
	void DisconnectAllDCCSessions(bool Shutdown);
	void CheckDCCTimeout(void);

	bool Connect(const CIrcSessionInfo& info);
	void Disconnect(void);
	void KillIdent(void);

	CIrcSessionInfo& GetInfo() const
				{ return (CIrcSessionInfo&)m_info; }

	int NLSend(const unsigned char* buf, int cbBuf);
	int NLSend(const TCHAR* fmt, ...);
	int NLSendNoScript( const unsigned char* buf, int cbBuf);
	int NLReceive(unsigned char* buf, int cbBuf);
	void InsertIncomingEvent(TCHAR* pszRaw);

	operator bool() const { return con!= NULL; }

	// send-to-stream operators
	friend CIrcSession& operator << (CIrcSession& os, const CIrcMessage& m);


protected :
//	Socket m_socket;
	CIrcSessionInfo m_info;
//#ifdef IRC_SSL
	CSSLSession sslSession;
//#endif
	HANDLE con;
	HANDLE hBindPort;
	void DoReceive();
	DccSessionMap m_dcc_chats;
	DccSessionMap m_dcc_xfers;


private :
	std::set<IIrcSessionMonitor*> m_monitors;
//	HANDLE m_hThread;
	CRITICAL_SECTION m_cs; // protect m_monitors
	CRITICAL_SECTION m_dcc; // protect the dcc objects

	void Notify(const CIrcMessage* pmsg);
	static void __cdecl ThreadProc(void *pparam);
};


__inline CIrcSession& operator << (CIrcSession& os, const CIrcMessage& m)
{
	if( os )
	{
		os.NLSend(m.AsString().c_str());
		if (!m.sCommand.empty() && m.sCommand != _T("QUIT") && m.m_bNotify)
			os.Notify(&m);
	}
	return os;
}

////////////////////////////////////////////////////////////////////
typedef struct			
{
	void * pthis;
	CIrcMessage * pmsg;
} XTHREADS;

class CIrcMonitor :
	public IIrcSessionMonitor
{
public :
	typedef bool (CIrcMonitor::*PfnIrcMessageHandler)(const CIrcMessage* pmsg);
	struct LessString
	{
		bool operator()(const TCHAR* s1, const TCHAR* s2) const
			{ return _tcsicmp(s1, s2) < 0; }
	};
	typedef std::map<const TCHAR*, PfnIrcMessageHandler, LessString> HandlersMap;
	struct IrcCommandsMapsListEntry
	{
		HandlersMap* pHandlersMap;
		IrcCommandsMapsListEntry* pBaseHandlersMap;
	};

	CIrcMonitor(CIrcSession& session);
	virtual ~CIrcMonitor();
	static IrcCommandsMapsListEntry m_handlersMapsListEntry;
	static HandlersMap m_handlers;

	PfnIrcMessageHandler FindMethod(const TCHAR* lpszName);
	PfnIrcMessageHandler FindMethod(IrcCommandsMapsListEntry* pMapsList, const TCHAR* lpszName);

	static void __stdcall OnCrossThreadsMessage(void * p);
	virtual void OnIrcMessage(const CIrcMessage* pmsg);
	CIrcSession& m_session;

	virtual IrcCommandsMapsListEntry* GetIrcCommandsMap() 
				{ return &m_handlersMapsListEntry; }

	virtual void OnIrcAll(const CIrcMessage* pmsg) {}
	virtual void OnIrcDefault(const CIrcMessage* pmsg) {}
	virtual void OnIrcDisconnected() {}
protected :


private :

};

// define an IRC command-to-member map.
// put that macro inside the class definition (.H file)
#define	DEFINE_IRC_MAP()	\
protected :	\
	virtual IrcCommandsMapsListEntry* GetIrcCommandsMap()	\
				{ return &m_handlersMapsListEntry; }	\
	static CIrcMonitor::IrcCommandsMapsListEntry m_handlersMapsListEntry;	\
	static CIrcMonitor::HandlersMap m_handlers;	\
private :	\
protected :

// IRC command-to-member map's declaration. 
// add this macro to the class's .CPP file
#define	DECLARE_IRC_MAP(this_class, base_class)	\
	CIrcMonitor::HandlersMap this_class##::m_handlers;	\
	CIrcMonitor::IrcCommandsMapsListEntry this_class##::m_handlersMapsListEntry	\
		= { &this_class##::m_handlers, &base_class##::m_handlersMapsListEntry };

// map actual member functions to their associated IRC command.
// put any number of this macro in the class's constructor.
#define	IRC_MAP_ENTRY(class_name, name, member)	\
	m_handlers[_T(name)] = (PfnIrcMessageHandler)&class_name##::member;

////////////////////////////////////////////////////////////////////

class CIrcDefaultMonitor : public CIrcMonitor
{
public :
	CIrcDefaultMonitor(CIrcSession& session);

	DEFINE_IRC_MAP()

protected :
	bool OnIrc_NICK(const CIrcMessage* pmsg);
	bool OnIrc_PING(const CIrcMessage* pmsg);
	bool OnIrc_YOURHOST(const CIrcMessage* pmsg);
	bool OnIrc_WELCOME(const CIrcMessage* pmsg);
};

////////////////////////////////////////////////////////////////////


class CDccSession{
protected:
	HANDLE con;			// connection handle	
	HANDLE hBindPort;	// handle for listening port


	static int nDcc;	// number of dcc objects
	DWORD iTotal;		// total bytes sent/received

	int iPacketSize;	// size of outgoing packets
	int iGlobalToken;

	PROTOFILETRANSFERSTATUS pfts; // structure used to setup and update the filetransfer dialogs of miranda
	char * file[2];		// used with the PROTOFILETRANSFER struct

	int SetupConnection();	
	void DoSendFile();
	void DoReceiveFile();
	void DoChatReceive();
	int NLSend( const unsigned char* buf, int cbBuf);
	int NLReceive(const unsigned char* buf, int cbBuf);
	static void __cdecl ThreadProc(void *pparam);
	static void __cdecl ConnectProc(void *pparam);

public:
	
	CDccSession(DCCINFO * pdci); // constructor
	~CDccSession();					// destructor

	time_t tLastPercentageUpdate;	// time of last update of the filetransfer dialog
	time_t tLastActivity;			// time of last in/out activity of the object
	time_t tLastAck;				// last acked filesize

	HANDLE hEvent;					// Manual object
	long dwWhatNeedsDoing;	// Set to indicate what FILERESUME_ action is chosen by the user
	char * NewFileName;				// contains new file name if FILERESUME_RENAME chosen
	long dwResumePos;		// position to resume from if FILERESUME_RESUME

	int iToken;						// used to identify (find) objects in reverse dcc filetransfers

	DCCINFO * di;	// details regarding the filetrasnfer

	int Connect();					
	void SetupPassive(DWORD adr, DWORD port);
	int SendStuff(const TCHAR* fmt);
	int IncomingConnection(HANDLE hConnection, DWORD dwIP);
	int Disconnect();


};
////////////////////////////////////////////////////////////////////
}; // end of namespace irc
////////////////////////////////////////////////////////////////////



#endif // _IRC_H_


