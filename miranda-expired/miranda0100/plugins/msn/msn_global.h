/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include <windows.h>
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"

#define MSNPROTONAME  "MSN"

#define MSN_NEWUSERURL "http://login.hotmail.passport.com/cgi-bin/register/default.asp?id=956&ru=http://messenger.msn.com/download/passportdone.asp"
#define MSN_DEFAULT_PORT  1863
#define MSN_DEFAULT_LOGIN_SERVER "messenger.hotmail.com"	//NB: msmsgs 3.60 hardcodes 64.4.13.33

/*
//moved to miranda.h
#define MSN_UHANDLE_LEN 130
#define MSN_NICKNAME_LEN 30	
#define MSN_PASSWORD_LEN 30
#define MSN_BASEUIN (MAX_GROUPS+1)

#define MSN_AUTHINF_LEN 30
#define MSN_SID_LEN 30
#define MSN_SES_MAX_USERS 10 //how many ppl u can chat wiht in one session
typedef struct tagMSN_SESSION{
	SOCKET con;

	char authinf[MSN_AUTHINF_LEN];
	char sid[MSN_SID_LEN];

	int usercnt;
	char users[MSN_SES_MAX_USERS][MSN_UHANDLE_LEN];
}MSN_SES;

typedef struct tagMSN_INF{
	BOOL		enabled;

	int status;
	char uhandle[MSN_UHANDLE_LEN];
	char nickname[MSN_NICKNAME_LEN];
	char password[MSN_PASSWORD_LEN];

	SOCKET sDS;
	SOCKET sNS;
	//SOCKET sSS;
	MSN_SES *SS; //each msn session
	int sscnt; //how many ss instances

	BOOL netactive; //a port open, stuff happening
	BOOL logedin; //have been authed
}MSN_INFO;

//(SUB)STATUS DEFINITONS
#define MSN_STATUS_OFFLINE "FLN" 

#define MSN_STATUS_ONLINE "" 
#define MSN_STATUS_ONLINE2 "NLN" 
#define MSN_STATUS_BUSY "BSY"
#define MSN_STATUS_IDLE "IDL"
#define MSN_STATUS_BRB "BRB"
#define MSN_STATUS_AWAY "AWY"
#define MSN_STATUS_PHONE "PHN"
#define MSN_STATUS_LUNCH "LUN"

*/
//FUNCTION PROTOTYPES
BOOL MSN_WS_Init();
void MSN_WS_CleanUp();
int MSN_WS_Send(SOCKET s,char*data,int datalen);
int MSN_WS_Recv(SOCKET s,char*data,long datalen);
unsigned long MSN_WS_ResolveName(char*name,WORD *port,int defaultPort);

void MSN_AddContactByUhandle(HWND hwnd);
int MSN_GetIntStatus(char *state);

//int MSN_Login(char*server,int port); //TRUE/false ret

//int MSN_SSConnect(char*server,int port,int sesid);

//void MSN_Logout();
//void MSN_Disconnect();

void MSN_ChangeStatus(char*substat);
void MSN_RemoveContact(char* uhandle);

#define MSN_LOG_FATAL   60
#define MSN_LOG_ERROR   50
#define MSN_LOG_WARNING 40
#define MSN_LOG_MESSAGE 30
#define MSN_LOG_DEBUG   20
#define MSN_LOG_PACKETS 10
#define MSN_LOG_RAWDATA 0
void MSN_DebugLog(int level,const char *fmt,...);
LONG MSN_SendPacket(SOCKET s,const char *cmd,const char *params,...);
char *MirandaStatusToMSN(int status);
int MSNStatusToMiranda(const char *status);
void UrlDecode(char *str);
void UrlEncode(const char *src,char *dest,int cbDest);

HANDLE MSN_HContactFromEmail(const char *msnEmail,const char *msnNick,int addIfNeeded,int temporary);
int MSN_AddContact(char* uhandle,char*nick); //returns clist ID
int MSN_ContactFromHandle(char*uhandle); //get cclist id from Uhandle
void MSN_HandleFromContact(unsigned long uin,char*uhandle);

//sesion stuff
BOOL MSN_SendUserMessage(char *destuhandle,char*msg);
void MSN_RemoveSession(int id);
int MSN_CreateSession();
void MSN_KillAllSessions();
void MSN_RequestSBSession();

//MIMIE FFUNCS
void MSN_MIME_GetContentType(char*src,char*ct);

char *str_to_UTF8(unsigned char *in);

#define SERVER_DISPATCH     0
#define SERVER_NOTIFICATION 1
#define SERVER_SWITCHBOARD  2
struct ThreadData {
	int type;
	char server[80];
	int caller;         //for switchboard servers only
	char cookie[130];   //for switchboard servers only
	HANDLE hContact;    //switchboard callees only
    //the rest is filled by thread
	SOCKET s;
	char data[1024];
	int bytesInData;
};

int CmdQueue_AddProtoAck(HANDLE hContact,int type,int result,HANDLE hProcess,LPARAM lParam);
int CmdQueue_AddChainRecv(CCSDATA *ccs);
int CmdQueue_AddDbWriteSetting(HANDLE hContact,const char *szModule,const char *szSetting,DBVARIANT *dbv);
int CmdQueue_AddDbWriteSettingString(HANDLE hContact,const char *szModule,const char *szSetting,const char *pszVal);
int CmdQueue_AddDbWriteSettingWord(HANDLE hContact,const char *szModule,const char *szSetting,WORD wVal);
int CmdQueue_AddDbCreateContact(const char *email,const char *nick,int temporary,HANDLE hWaitEvent,HANDLE *phContact);

#define SBSTATUS_NEW        0
#define SBSTATUS_DNSLOOKUP  1
#define SBSTATUS_CONNECTING 2
#define SBSTATUS_AUTHENTICATING 3
#define SBSTATUS_CONNECTED  10
void Switchboards_New(SOCKET s);
void Switchboards_Delete(void);
void Switchboards_ChangeStatus(int newStatus);
int Switchboards_ContactJoined(HANDLE hContact);
int Switchboards_ContactLeft(HANDLE hContact);
SOCKET Switchboards_SocketFromHContact(HANDLE hContact);

int MsgQueue_Add(HANDLE hContact,const char *msg,DWORD flags);
HANDLE MsgQueue_GetNextRecipient(void);
char *MsgQueue_GetNext(HANDLE hContact,PDWORD pFlags,int *seq);
int MsgQueue_AllocateUniqueSeq(void);

//MSN error codes
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
