//MSN Messenger Support for Miranda ICQ
//Copyright 2000, Tristan Van de Vreede


#include <windows.h>
#include <winsock.h>

#define MSN_NEWUSERURL "http://login.hotmail.passport.com/cgi-bin/register/default.asp?id=956&ru=http://messenger.msn.com/download/passportdone.asp"

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
	int MSN_WS_SendData(SOCKET s,char*data);
	int MSN_WS_Recv(SOCKET *s,char*data,long datalen);
	unsigned long MSN_WS_ResolveName(char*name);
	void MSN_WS_Close(SOCKET *s,BOOL transfering);//transfering if true will prevent saying 'offline''
													//used when transfering NS servers
	
	void MSN_AddContactByUhandle(HWND hwnd);
	int MSN_GetIntStatus(char *state);
	
	int MSN_Login(char*server,int port); //TRUE/false ret
	
	int MSN_SSConnect(char*server,int port,int sesid);

	void MSN_Logout();
	void MSN_Disconnect();

	void MSN_ChangeStatus(char*substat);
	void MSN_RemoveContact(char* uhandle);
	
	
	void MSN_DebugLog(char*msg);
	void MSN_DebugLogEx(char *msg,char*msg2,char*msg3);
	void MSN_GetWd(char*src,int wordno,char*out);

	int MSN_SendPacket(SOCKET s,char*cmd,char*params);

	void MSN_HandlePacketStream(char*data,SOCKET *replys,BOOL ss,int sesid);
	void MSN_HandlePacket(char*data,SOCKET *replys);
	
	void MSN_HandleSSPacket(char*data,SOCKET *replys,int sesid);//Switch board svr
	
	void MSN_main();

	int MSN_AddContact(char* uhandle,char*nick); //returns clist ID
	int MSN_ContactFromHandle(char*uhandle); //get cclist id from Uhandle
	void MSN_HandleFromContact(unsigned long uin,char*uhandle);
	int MSN_MSNStatetoICQState(char*state); //	get a ICQ sttae from a msn strate

	//sesion stuff
	BOOL MSN_SendUserMessage(char *destuhandle,char*msg);
	void MSN_RemoveSession(int id);
	int MSN_CreateSession();
	void MSN_KillAllSessions();
	void MSN_RequestSBSession();

	//MIMIE FFUNCS
	void MSN_MIME_GetContentType(char*src,char*ct);


	char *str_to_UTF8(unsigned char *in);

	#ifndef _MSN_GLOBAL_H

		#define _MSN_GLOBAL_H

		extern int		MSN_ccount;
		
	#endif

