#ifndef CONNECTION_H
#define CONNECTION_H
#include <stdio.h>
#include <windows.h>
#include <newpluginapi.h>
#include <m_netlib.h>
#include <m_database.h>
#include "defines.h"
#include "packets.h"
#include "commands.h"
#include "thread.h"
#include "utility.h"
#include "snac.h"
#define HOOKEVENT_SIZE 6
extern char* COOKIE;
extern int COOKIE_LENGTH;
extern char* CWD;//current working directory
extern char* AIM_PROTOCOL_NAME;
extern char* GROUP_ID_KEY;
extern char* ID_GROUP_KEY;
extern char* FILE_TRANSFER_KEY;
extern CRITICAL_SECTION modeMsgsMutex;
extern CRITICAL_SECTION statusMutex;
extern CRITICAL_SECTION connectionMutex;
class oscar_data
{
public:
    char *username;
    char *password;
    int seqno;
	int state;
	int packet_offset;
	unsigned int status;//current status
	int initial_status;//start up status
	char* szModeMsg;//away message
	bool requesting_HTML_ModeMsg;
	bool request_HTML_profile;
	bool buddy_list_received;
	HINSTANCE hInstance;
	HANDLE hServerConn;
	HANDLE hServerPacketRecver;
	HANDLE hNetlib;
	HANDLE hNetlibPeer;
	HANDLE hDirectBoundPort;
	HANDLE hHTMLAwayContextMenuItem;
	HANDLE hookEvent[HOOKEVENT_SIZE];
	int hTimer;
	unsigned int hookEvent_size;
	unsigned long InternalIP;
	unsigned short LocalPort;
	HANDLE current_rendezvous_accept_user;//hack
	int dc_thread_amt;
}extern conn;
HANDLE aim_connect(char* server);
HANDLE aim_peer_connect(char* ip,unsigned short port);
void CALLBACK aim_keepalive_connection(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime);
void aim_connection_authorization();
void aim_protocol_negotiation();
#endif
