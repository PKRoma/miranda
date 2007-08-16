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

// This file holds functions that are called upon receiving
// certain commands from the server.

#include "irc.h"
#include <algorithm>

using namespace irc;

int				ChannelNumber =0;
String			WhoReply = "";
bool			nickflag;
bool			bPerformDone = false;
int				NoOfChannels = 0;
HWND			whois_hWnd = NULL;
HWND			list_hWnd = NULL;
HWND			nick_hWnd = NULL;
HWND			manager_hWnd = NULL;
UINT_PTR		InitTimer;
UINT_PTR		IdentTimer;
int				ManualWhoisCount = 0;
String			sNamesList;
String			sUserModes;
String			sUserModePrefixes;
String			sChannelPrefixes;
String			sChannelModes;
String			WhoisAwayReply;
String			sTopic;
String			sTopicName;
String			NamesToWho = "";
String			ChannelsToWho = "";
String			NamesToUserhost = "";

extern char *	pszIgnoreFile;
extern String	StatusMessage;
extern char *	pszPerformFile;
extern char		mirandapath[MAX_PATH];
extern HANDLE	hMenuQuick ;			
extern HANDLE	hMenuServer ;			
extern HANDLE	hMenuJoin ;			
extern HANDLE	hMenuNick ;				
extern HANDLE	hMenuList ;	
extern int		ManualWhoisCount;	
extern bool		bTempDisableCheck ;
extern bool		bTempForceCheck ;
extern int		iTempCheckTime ;
extern int		OldStatus;
extern int		GlobalStatus;
extern PREFERENCES *prefs;
extern CIrcSession g_ircSession;
extern char *	IRCPROTONAME;
extern UINT_PTR	OnlineNotifTimer;
extern UINT_PTR	OnlineNotifTimer3;
extern UINT_PTR	KeepAliveTimer = 0;
extern CRITICAL_SECTION m_resolve;
extern GETEVENTFUNC			pfnAddEvent;
HWND			IgnoreWndHwnd = NULL;

extern HANDLE					hNetlib;	


static VOID CALLBACK IdentTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	KillChatTimer(IdentTimer);
	if(OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING) 
		return;

	if (g_ircSession && prefs->IdentTimer)
		g_ircSession.KillIdent();

	return;
}


static VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	KillChatTimer(InitTimer);
	if(OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING) 
		return;

	if (prefs->ForceVisible)
		PostIrcMessage("/MODE %s -i", g_ircSession.GetInfo().sNick.c_str());

	if (lstrlen(prefs->MyHost)== 0 && g_ircSession)
	{
		DoUserhostWithReason(2, ("S"+ g_ircSession.GetInfo().sNick).c_str(), true, "%s", g_ircSession.GetInfo().sNick.c_str());
	}

	return;
}


VOID CALLBACK KeepAliveTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	if(!prefs->SendKeepAlive || (OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING)) 
	{
		KillChatTimer(KeepAliveTimer);

		return;
	}

	char temp2[270];
	if(g_ircSession.GetInfo().sServerName != "")
		mir_snprintf(temp2, 269, "PING %s", g_ircSession.GetInfo().sServerName.c_str());
	else
		mir_snprintf(temp2, 269, "PING %u", time(0));

	if (g_ircSession)
		g_ircSession << CIrcMessage(temp2, false, false);

	return;
}
VOID CALLBACK OnlineNotifTimerProc3(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	if(!prefs->ChannelAwayNotification || OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING || (!prefs->AutoOnlineNotification && !bTempForceCheck) || bTempDisableCheck) 
	{
		KillChatTimer(OnlineNotifTimer3);
		ChannelsToWho == "";
		return;
	}

	String name = GetWord((char *)ChannelsToWho.c_str(), 0);

	if (name == "" )
	{
		ChannelsToWho = "";
		int count = (int)CallService(MS_GC_GETSESSIONCOUNT, 0, (LPARAM)IRCPROTONAME);
		for (int i = 0; i < count ; i++)
		{
			GC_INFO gci = {0};
			gci.Flags = BYINDEX|NAME|TYPE|COUNT;
			gci.iItem = i;
			gci.pszModule = IRCPROTONAME;
			if(!CallService(MS_GC_GETINFO, 0, (LPARAM)&gci) && gci.iType == GCW_CHATROOM)
			{
				if(gci.iCount <= prefs->OnlineNotificationLimit)
					ChannelsToWho += (String)gci.pszName + " ";
			}
		}
	}

	if(ChannelsToWho == "") {
		SetChatTimer(OnlineNotifTimer3, 60*1000, OnlineNotifTimerProc3);
		return;
	}
	name = GetWord((char *)ChannelsToWho.c_str(), 0);
	DoUserhostWithReason(2, "S" + name, true, "%s", name.c_str());
	String temp = GetWordAddress(ChannelsToWho.c_str(), 1);
	ChannelsToWho = temp;
	if (iTempCheckTime)
		SetChatTimer(OnlineNotifTimer3, iTempCheckTime*1000, OnlineNotifTimerProc3);
	else
		SetChatTimer(OnlineNotifTimer3, prefs->OnlineNotificationTime*1000, OnlineNotifTimerProc3);
	return;


}

VOID CALLBACK OnlineNotifTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{

	if(OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING || (!prefs->AutoOnlineNotification && !bTempForceCheck) || bTempDisableCheck) 
	{
		KillChatTimer(OnlineNotifTimer);
		NamesToWho == "";

		return;
	}

	String name = GetWord((char *)NamesToWho.c_str(), 0);
	String name2 = GetWord((char *)NamesToUserhost.c_str(), 0);

	if (name == "" && name2 == "")
	{
		HANDLE hContact;
		DBVARIANT dbv;
		char *szProto;

		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact) 
		{
		   szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		   if (szProto != NULL && !lstrcmpi(szProto, IRCPROTONAME)) 
		   {
			   BYTE bRoom = DBGetContactSettingByte(hContact, IRCPROTONAME, "ChatRoom", 0);
			   if(bRoom == 0)
			   {
				   BYTE bDCC = DBGetContactSettingByte(hContact, IRCPROTONAME, "DCC", 0);
				   BYTE bHidden = DBGetContactSettingByte(hContact,"CList", "Hidden", 0);
					if(bDCC == 0 &&  bHidden == 0)
					{
						if (!DBGetContactSetting(hContact, IRCPROTONAME, "Default", &dbv) && dbv.type == DBVT_ASCIIZ) 
						{
							BYTE bAdvanced = DBGetContactSettingByte(hContact,IRCPROTONAME, "AdvancedMode", 0) ;

							
							if (!bAdvanced)
							{
								DBFreeVariant(&dbv);
								if (!DBGetContactSetting(hContact, IRCPROTONAME, "Nick", &dbv) && dbv.type == DBVT_ASCIIZ) 
								{	
									NamesToUserhost += (String)dbv.pszVal + " ";
									DBFreeVariant(&dbv);
								}
							}
							else
							{
								DBFreeVariant(&dbv);
								DBVARIANT dbv2;
								
								char * DBNick = NULL;
								char * DBWildcard = NULL;
								if (!DBGetContactSetting(hContact, IRCPROTONAME, "Nick", &dbv) && dbv.type == DBVT_ASCIIZ) DBNick = dbv.pszVal;
								if (!DBGetContactSetting(hContact, IRCPROTONAME, "UWildcard", &dbv2) && dbv2.type == DBVT_ASCIIZ) DBWildcard = dbv2.pszVal;

								if (DBNick && (!DBWildcard ||!WCCmp(CharLower(DBWildcard), CharLower(DBNick)))) 
									NamesToWho += (String)DBNick + " ";
								else if(DBWildcard)
									NamesToWho += (String)DBWildcard + " ";

								if(DBNick)
									DBFreeVariant(&dbv);

								if(DBWildcard)
									DBFreeVariant(&dbv2);
							}
						}
					}
				}
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
	}

	if (NamesToWho == "" && NamesToUserhost == "")
	{
		SetChatTimer(OnlineNotifTimer, 60*1000, OnlineNotifTimerProc);
		return;
	}

	name = GetWord((char *)NamesToWho.c_str(), 0);
	name2 = GetWord((char *)NamesToUserhost.c_str(), 0);
	String temp;
	if(name != "")
	{
		DoUserhostWithReason(2, "S" + name, true, "%s", name.c_str());
		temp = GetWordAddress(NamesToWho.c_str(), 1);
		NamesToWho = temp;
	}

	if(name2 != "")
	{
		String params;
		for(int i = 0; i < 2; i++)
		{
			params = "";
			for(int j = 0; j < 5; j++)
			{
				params += GetWord(NamesToUserhost.c_str(), i *5 + j) + " ";
			}
			if(params[0] != ' ')
				DoUserhostWithReason(1, (String)"S" + params, true, params);
		}
		temp = GetWordAddress(NamesToUserhost.c_str(), 15);
		NamesToUserhost  = temp;
	}

	if (iTempCheckTime)
		SetChatTimer(OnlineNotifTimer, iTempCheckTime*1000, OnlineNotifTimerProc);
	else
		SetChatTimer(OnlineNotifTimer, prefs->OnlineNotificationTime*1000, OnlineNotifTimerProc);

	return;
	

}


static int AddOutgoingMessageToDB(HANDLE hContact, char *msg)
{
	if (OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING)
		return 0;

	String S = msg;
	S = DoColorCodes((char*)S.c_str(), TRUE, FALSE);
	DBEVENTINFO dbei;
	ZeroMemory(&dbei, sizeof(dbei));
	dbei.cbSize = sizeof(dbei);
	dbei.szModule = IRCPROTONAME;
	dbei.timestamp = (DWORD)time(NULL);
	dbei.flags = DBEF_SENT;
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.cbBlob = strlen(S.c_str()) + 1;
	dbei.pBlob = (PBYTE) S.c_str();
	CallService(MS_DB_EVENT_ADD, (WPARAM) hContact, (LPARAM) & dbei);

	return 1;
 
}

void __cdecl ResolveIPThread(LPVOID di)
{
	EnterCriticalSection(&m_resolve);

	IPRESOLVE * ipr = (IPRESOLVE *) di;

	if ( ipr != NULL && (ipr->iType == IP_AUTO && lstrlen(prefs->MyHost) == 0 || ipr->iType == IP_MANUAL) ) 
	{
		IN_ADDR in;

		struct hostent* myhost = gethostbyname( ipr->pszAdr );

		if( myhost)
		{
			memcpy( &in, myhost->h_addr, 4 );
			if(ipr->iType == IP_AUTO)
				mir_snprintf( prefs->MyHost, sizeof( prefs->MyHost ), "%s", inet_ntoa( in ));
			else
				mir_snprintf( prefs->MySpecifiedHostIP, sizeof( prefs->MySpecifiedHostIP ), "%s", inet_ntoa( in ));
		}
	}
	delete ipr;
	LeaveCriticalSection(&m_resolve);

}

DECLARE_IRC_MAP(CMyMonitor, CIrcDefaultMonitor)

CMyMonitor::CMyMonitor() : irc::CIrcDefaultMonitor(g_ircSession)
{
	IRC_MAP_ENTRY(CMyMonitor, "JOIN", OnIrc_JOIN)
	IRC_MAP_ENTRY(CMyMonitor, "QUIT", OnIrc_QUIT)
	IRC_MAP_ENTRY(CMyMonitor, "KICK", OnIrc_KICK)
	IRC_MAP_ENTRY(CMyMonitor, "MODE", OnIrc_MODE)
	IRC_MAP_ENTRY(CMyMonitor, "NICK", OnIrc_NICK)
	IRC_MAP_ENTRY(CMyMonitor, "PART", OnIrc_PART)
	IRC_MAP_ENTRY(CMyMonitor, "PRIVMSG", OnIrc_PRIVMSG)
	IRC_MAP_ENTRY(CMyMonitor, "TOPIC", OnIrc_TOPIC)
	IRC_MAP_ENTRY(CMyMonitor, "NOTICE", OnIrc_NOTICE)
	IRC_MAP_ENTRY(CMyMonitor, "PING", OnIrc_PINGPONG)
	IRC_MAP_ENTRY(CMyMonitor, "PONG", OnIrc_PINGPONG)
	IRC_MAP_ENTRY(CMyMonitor, "INVITE", OnIrc_INVITE)
	IRC_MAP_ENTRY(CMyMonitor, "ERROR", OnIrc_ERROR)
	IRC_MAP_ENTRY(CMyMonitor, "001", OnIrc_WELCOME)
	IRC_MAP_ENTRY(CMyMonitor, "002", OnIrc_YOURHOST)
	IRC_MAP_ENTRY(CMyMonitor, "005", OnIrc_SUPPORT)
	IRC_MAP_ENTRY(CMyMonitor, "254", OnIrc_NOOFCHANNELS)
	IRC_MAP_ENTRY(CMyMonitor, "263", OnIrc_TRYAGAIN)
	IRC_MAP_ENTRY(CMyMonitor, "301", OnIrc_WHOIS_AWAY)
	IRC_MAP_ENTRY(CMyMonitor, "302", OnIrc_USERHOST_REPLY)
	IRC_MAP_ENTRY(CMyMonitor, "305", OnIrc_BACKFROMAWAY)
	IRC_MAP_ENTRY(CMyMonitor, "306", OnIrc_SETAWAY)
	IRC_MAP_ENTRY(CMyMonitor, "307", OnIrc_WHOIS_AUTH)
	IRC_MAP_ENTRY(CMyMonitor, "310", OnIrc_WHOIS_OTHER)
	IRC_MAP_ENTRY(CMyMonitor, "311", OnIrc_WHOIS_NAME)
	IRC_MAP_ENTRY(CMyMonitor, "312", OnIrc_WHOIS_SERVER)
	IRC_MAP_ENTRY(CMyMonitor, "313", OnIrc_WHOIS_OTHER)
	IRC_MAP_ENTRY(CMyMonitor, "315", OnIrc_WHO_END)
	IRC_MAP_ENTRY(CMyMonitor, "317", OnIrc_WHOIS_IDLE)
	IRC_MAP_ENTRY(CMyMonitor, "318", OnIrc_WHOIS_END)
	IRC_MAP_ENTRY(CMyMonitor, "319", OnIrc_WHOIS_CHANNELS)
	IRC_MAP_ENTRY(CMyMonitor, "320", OnIrc_WHOIS_AUTH)
	IRC_MAP_ENTRY(CMyMonitor, "321", OnIrc_LISTSTART)
	IRC_MAP_ENTRY(CMyMonitor, "322", OnIrc_LIST)
	IRC_MAP_ENTRY(CMyMonitor, "323", OnIrc_LISTEND)
	IRC_MAP_ENTRY(CMyMonitor, "324", OnIrc_MODEQUERY)
	IRC_MAP_ENTRY(CMyMonitor, "330", OnIrc_WHOIS_AUTH)
	IRC_MAP_ENTRY(CMyMonitor, "332", OnIrc_INITIALTOPIC)
	IRC_MAP_ENTRY(CMyMonitor, "333", OnIrc_INITIALTOPICNAME)
	IRC_MAP_ENTRY(CMyMonitor, "352", OnIrc_WHO_REPLY)
	IRC_MAP_ENTRY(CMyMonitor, "353", OnIrc_NAMES)
	IRC_MAP_ENTRY(CMyMonitor, "366", OnIrc_ENDNAMES)
	IRC_MAP_ENTRY(CMyMonitor, "367", OnIrc_BANLIST)
	IRC_MAP_ENTRY(CMyMonitor, "368", OnIrc_BANLISTEND)
	IRC_MAP_ENTRY(CMyMonitor, "346", OnIrc_BANLIST)
	IRC_MAP_ENTRY(CMyMonitor, "347", OnIrc_BANLISTEND)
	IRC_MAP_ENTRY(CMyMonitor, "348", OnIrc_BANLIST)
	IRC_MAP_ENTRY(CMyMonitor, "349", OnIrc_BANLISTEND)
	IRC_MAP_ENTRY(CMyMonitor, "371", OnIrc_WHOIS_OTHER)
	IRC_MAP_ENTRY(CMyMonitor, "376", OnIrc_ENDMOTD)
	IRC_MAP_ENTRY(CMyMonitor, "401", OnIrc_WHOIS_NO_USER)
	IRC_MAP_ENTRY(CMyMonitor, "403", OnIrc_JOINERROR)
	IRC_MAP_ENTRY(CMyMonitor, "416", OnIrc_WHOTOOLONG)
	IRC_MAP_ENTRY(CMyMonitor, "421", OnIrc_UNKNOWN)
	IRC_MAP_ENTRY(CMyMonitor, "422", OnIrc_ENDMOTD)
	IRC_MAP_ENTRY(CMyMonitor, "433", OnIrc_NICK_ERR)
	IRC_MAP_ENTRY(CMyMonitor, "471", OnIrc_JOINERROR)
	IRC_MAP_ENTRY(CMyMonitor, "473", OnIrc_JOINERROR)
	IRC_MAP_ENTRY(CMyMonitor, "474", OnIrc_JOINERROR)
	IRC_MAP_ENTRY(CMyMonitor, "475", OnIrc_JOINERROR)
}

CMyMonitor::~CMyMonitor()
{
}

bool CMyMonitor::OnIrc_WELCOME(const CIrcMessage* pmsg)
{
	CIrcDefaultMonitor::OnIrc_WELCOME(pmsg);

	if (pmsg->m_bIncoming && pmsg->parameters.size() > 1)
	{
		static char host[1024];
		int i = 0;
		String word = GetWord(pmsg->parameters[1].c_str(), i);
		while(word != "")
		{
			if(strchr(word.c_str(), '!') && strchr(word.c_str(), '@'))
			{
				lstrcpyn(host, word.c_str(), 1024);
				char * p1 = strchr(host, '@');
				if(p1)
				{
					p1++;
					IPRESOLVE * ipr = new IPRESOLVE;
					ipr->iType = IP_AUTO;
					ipr->pszAdr = p1;
					mir_forkthread(ResolveIPThread, ipr);
				}

			}
			i++;
			word = GetWord(pmsg->parameters[1].c_str(), i);


		}
	}			
		
	ShowMessage(pmsg); 
	return true;
}
bool CMyMonitor::OnIrc_WHOTOOLONG(const CIrcMessage* pmsg)
{
	String command = GetNextUserhostReason(2);
	if(command[0] == 'U')
		ShowMessage(pmsg); 

	return true;
}
bool CMyMonitor::OnIrc_BACKFROMAWAY(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming)
	{
		int Temp = OldStatus;
		OldStatus = ID_STATUS_ONLINE;
		ProtoBroadcastAck(IRCPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp,ID_STATUS_ONLINE);

		if (prefs->Perform)
			DoPerform("Event: Available");
	}			
		
	ShowMessage(pmsg); 
	return true;
}

bool CMyMonitor::OnIrc_SETAWAY(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming)
	{
		int Temp = OldStatus;
		OldStatus =  ID_STATUS_AWAY;
		ProtoBroadcastAck(IRCPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp,ID_STATUS_AWAY);

		if (prefs->Perform)
			switch (GlobalStatus)
			{
				case ID_STATUS_AWAY :
				{
					DoPerform(	"Event: Away"	);
				} break;
				case ID_STATUS_NA :
				{
					DoPerform("Event: N/A"	);
				} break;
				case ID_STATUS_DND :
				{
					DoPerform("Event: DND"	);
				} break;
				case ID_STATUS_OCCUPIED :
				{
					DoPerform("Event: Occupied"	);
				} break;

				default :
				{
					GlobalStatus = ID_STATUS_AWAY;
					DoPerform("Event: Away"	);
				} break;
			}
    }	
		
	ShowMessage(pmsg); 
	return true;
}
bool CMyMonitor::OnIrc_JOIN(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.size() > 0 && pmsg->m_bIncoming && pmsg->prefix.sNick != g_ircSession.GetInfo().sNick) {
		String host = pmsg->prefix.sUser + "@" + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_JOIN, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), NULL, "Normal", host.c_str(), NULL, true, false); 
	}
	else
		ShowMessage(pmsg); 

	return true;
}

bool CMyMonitor::OnIrc_QUIT(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming) 
	{
		String host = pmsg->prefix.sUser + "@" + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_QUIT, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters.size()>0?pmsg->parameters[0].c_str():NULL, NULL, host.c_str(), NULL, true, false);
		struct CONTACT_TYPE user ={(char *)pmsg->prefix.sNick.c_str(), (char *)pmsg->prefix.sUser.c_str(), (char *)pmsg->prefix.sHost.c_str(), false, false, false};
		CList_SetOffline(&user);
		if (pmsg->prefix.sNick == g_ircSession.GetInfo().sNick)
		{
			GCDEST gcd = {0};
			GCEVENT gce = {0};

			gce.cbSize = sizeof(GCEVENT);
			gcd.pszID = NULL;
			gcd.pszModule = IRCPROTONAME;
			gcd.iType = GC_EVENT_CONTROL;
			gce.pDest = &gcd;
			CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);

		}

	}
	else 
		ShowMessage(pmsg);


	
	return true;
}

bool CMyMonitor::OnIrc_PART(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.size() > 0 && pmsg->m_bIncoming)
	{
		String host = pmsg->prefix.sUser + "@" + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_PART, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), pmsg->parameters.size()>1?pmsg->parameters[1].c_str():NULL, NULL, host.c_str(), NULL, true, false); 
		if (pmsg->prefix.sNick == g_ircSession.GetInfo().sNick)
		{
			GCDEST gcd = {0};
			GCEVENT gce = {0};

			String S = MakeWndID(pmsg->parameters[0]);
			gce.cbSize = sizeof(GCEVENT);
			gcd.pszID = (char *)S.c_str();
			gcd.pszModule = IRCPROTONAME;
			gcd.iType = GC_EVENT_CONTROL;
			gce.pDest = &gcd;
			CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);
	
		}
	}
	else
		ShowMessage(pmsg); 

	return true;
}

bool CMyMonitor::OnIrc_KICK(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.size() > 1)
		DoEvent(GC_EVENT_KICK, pmsg->parameters[0].c_str(), pmsg->parameters[1].c_str(), pmsg->parameters.size()>2?pmsg->parameters[2].c_str():NULL, pmsg->prefix.sNick.c_str(), NULL, NULL, true, false); 
	else
		ShowMessage(pmsg); 

	if (pmsg->parameters[1] == g_ircSession.GetInfo().sNick)
	{
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		String S = MakeWndID(pmsg->parameters[0]);
		gce.cbSize = sizeof(GCEVENT);
		gcd.pszID = (char *)S.c_str();
		gcd.pszModule = IRCPROTONAME;
		gcd.iType = GC_EVENT_CONTROL;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);

		if (prefs->RejoinIfKicked) {
			CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, pmsg->parameters[0].c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if (wi && wi->pszPassword)
				PostIrcMessage("/JOIN %s %s", (char *)pmsg->parameters[0].c_str(), wi->pszPassword);
			else
				PostIrcMessage("/JOIN %s", (char *)pmsg->parameters[0].c_str());
		}
	}


	return true;
}
bool CMyMonitor::OnIrc_MODEQUERY(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.size() > 2 && pmsg->m_bIncoming && IsChannel(pmsg->parameters[1])) 
	{

		String sPassword = "";
		String sLimit = "";
		bool bAdd =false;
		int iParametercount = 3;
		char* p1 = (char*)pmsg->parameters[2].c_str();
		
		while (*p1 != '\0')
		{
			if (*p1 == '+')
				bAdd = true;
			if (*p1 == '-')
				bAdd = false;
			if (*p1 == 'l' && bAdd)
			{
				if ((int)pmsg->parameters.size() > iParametercount)
					sLimit = pmsg->parameters[iParametercount];
				iParametercount++;
			}
			if (*p1 == 'k' && bAdd)
			{
				if ((int)pmsg->parameters.size() > iParametercount)
					sPassword = pmsg->parameters[iParametercount];
				iParametercount++;
			}

			p1++;
		}
		AddWindowItemData(pmsg->parameters[1].c_str(), sLimit != ""?sLimit.c_str():0, pmsg->parameters[2].c_str(), sPassword != ""?sPassword.c_str():0, 0);
		
	}
	ShowMessage(pmsg);
	return true;
}
bool CMyMonitor::OnIrc_MODE(const CIrcMessage* pmsg)
{	
	bool flag = false; 
	bool bContainsValidModes = false;
	String sModes = "";
	String sParams = "";

	if (pmsg->parameters.size() > 1 && pmsg->m_bIncoming) 
	{

		if(IsChannel(pmsg->parameters[0]))
		{
			bool bAdd =false;
			int iParametercount = 2;
			char* p1 = (char*)pmsg->parameters[1].c_str();
			
			while (*p1 != '\0')
			{
				if (*p1 == '+')
				{
					bAdd = true;
					sModes += "+";
				}
				if (*p1 == '-')
				{
					bAdd = false;
					sModes += "-";
				}
				if (*p1 == 'l' && bAdd && iParametercount < (int)pmsg->parameters.size())
				{
					bContainsValidModes = true;
					sModes += "l";
					sParams += " " + pmsg->parameters[iParametercount];
					iParametercount++;
				}
				if (*p1 == 'b' || *p1 == 'k' && iParametercount < (int)pmsg->parameters.size())
				{
					bContainsValidModes = true;
					sModes += *p1;
					sParams += " " + pmsg->parameters[iParametercount];
					iParametercount++;
				}

				if(strchr(sUserModes.c_str(), *p1))
				{
					String sStatus = ModeToStatus(*p1);
					if((int)pmsg->parameters.size() > iParametercount)
					{
						DoEvent(bAdd?GC_EVENT_ADDSTATUS:GC_EVENT_REMOVESTATUS, pmsg->parameters[0].c_str(), pmsg->parameters[iParametercount].c_str(), pmsg->prefix.sNick.c_str(), sStatus.c_str(), NULL, NULL, prefs->OldStyleModes?false:true, false); 
						iParametercount++;
					}
				}
				else if (*p1 != 'b' && *p1 != ' ' && *p1 != '+' && *p1 != '-')
				{
					bContainsValidModes = true;
					if(*p1 != 'l' && *p1 != 'k')
						sModes += *p1;
					flag = true;
				}

				p1++;
			}

			if(prefs->OldStyleModes)
			{
				String sMessage;
				char temp[256]; *temp = '\0';
				mir_snprintf(temp, 255, Translate(	"%s sets mode %s" ), pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str());
				sMessage = temp;
				for(int i=2; i < (int)pmsg->parameters.size(); i++)
				{
					sMessage = sMessage  +" "+ pmsg->parameters[i];
				}
				DoEvent(GC_EVENT_INFORMATION, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), sMessage.c_str(), NULL, NULL, NULL, true, false); 
			}
			else if(bContainsValidModes)
			{
				for(int i=iParametercount; i < (int)pmsg->parameters.size(); i++)
				{
					sParams += " "+ pmsg->parameters[i];
				}


				char temp[4000]; *temp = '\0';
				mir_snprintf(temp, 3999, Translate(	"%s sets mode %s%s" ), pmsg->prefix.sNick.c_str(), sModes.c_str(), sParams.c_str());
				DoEvent(GC_EVENT_INFORMATION, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
			}

			if (flag)
				PostIrcMessage("/MODE %s", pmsg->parameters[0].c_str());
		}
		else
		{
				String sMessage;
				char temp[256]; *temp = '\0';
				mir_snprintf(temp, 255, Translate(	"%s sets mode %s" ), pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str());
				sMessage = temp;
				for(int i=2; i < (int)pmsg->parameters.size(); i++)
				{
					sMessage = sMessage  +" "+ pmsg->parameters[i];
				}
				DoEvent(GC_EVENT_INFORMATION, "Network Log", pmsg->prefix.sNick.c_str(), sMessage.c_str(), NULL, NULL, NULL, true, false); 

		}
	}
	else
		ShowMessage(pmsg); 

	return true;
}

bool CMyMonitor::OnIrc_NICK(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.size() > 0) 
	{
		bool bIsMe = pmsg->prefix.sNick.c_str() == g_ircSession.GetInfo().sNick?true:false;
		CIrcDefaultMonitor::OnIrc_NICK(pmsg);
		String host = pmsg->prefix.sUser + "@" + pmsg->prefix.sHost;
		DoEvent(GC_EVENT_NICK, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters[0].c_str(), NULL, host.c_str(), NULL, true, bIsMe); 
		DoEvent(GC_EVENT_CHUID, NULL, pmsg->prefix.sNick.c_str(), pmsg->parameters[0].c_str(), NULL, NULL, NULL, true, false); 
		struct CONTACT_TYPE user ={(char *)pmsg->prefix.sNick.c_str(), (char *)pmsg->prefix.sUser.c_str(), (char *)pmsg->prefix.sHost.c_str(), false, false, false};
		HANDLE hContact = CList_FindContact(&user);
		if (hContact) 
		{
			if (DBGetContactSettingWord(hContact,IRCPROTONAME, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
				DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_ONLINE);
			DBWriteContactSettingString(hContact, IRCPROTONAME, "Nick", pmsg->parameters[0].c_str());
			DBWriteContactSettingString(hContact, IRCPROTONAME, "User", pmsg->prefix.sUser.c_str());
			DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", pmsg->prefix.sHost.c_str());
		}
	}
	else
		ShowMessage(pmsg);

	return true;
}


bool CMyMonitor::OnIrc_NOTICE(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.size() > 1) 
	{
		if (IsCTCP(pmsg))
			return true;

		if(!prefs->Ignore || !IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'n'))
		{
			String S;
			String S2;
			if(pmsg->prefix.sNick.length() > 0)
				S = pmsg->prefix.sNick + " (" +m_session.GetInfo().sNetwork + ")";
			else
				S = m_session.GetInfo().sNetwork;

			if(IsChannel(pmsg->parameters[0]))
			{
				S2 = pmsg->parameters[0].c_str();
			}
			else
			{
				String S3;
				GC_INFO gci = {0};
				gci.Flags = BYID|TYPE;
				gci.pszModule = IRCPROTONAME;

				S3 = GetWord(pmsg->parameters[1].c_str(), 0);

				if(S3[0] == '[' && S3[1] == '#' && S3[S3.length()-1] == ']')
				{
					String Wnd;
					S3.erase(S3.length()-1, 1);
					S3.erase(0,1);
					Wnd = MakeWndID(S3);
					gci.pszID = (char *) Wnd.c_str();
					if(!CallService(MS_GC_GETINFO, 0, (LPARAM)&gci) && gci.iType == GCW_CHATROOM)
						S2 = GetWord(gci.pszID, 0);
					else
						S2 = "";
				}
				else
					S2 = "";
			}
			DoEvent(GC_EVENT_NOTICE, S2 != "" ? S2.c_str() : NULL, S.c_str(), pmsg->parameters[1].c_str(), NULL, NULL, NULL, true, false); 
		}
	}
	else
		ShowMessage(pmsg);

	return true;
}

bool CMyMonitor::OnIrc_YOURHOST(const CIrcMessage* pmsg)
{
	if(pmsg->m_bIncoming) 
		CIrcDefaultMonitor::OnIrc_YOURHOST(pmsg);
	
	ShowMessage(pmsg);

	return true;
}


bool CMyMonitor::OnIrc_INVITE(const CIrcMessage* pmsg)
{
	if(pmsg->m_bIncoming && (prefs->Ignore && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'i')))
		return true;

	if(pmsg->m_bIncoming && prefs->JoinOnInvite && pmsg->parameters.size() >1 && lstrcmpi(pmsg->parameters[0].c_str(), m_session.GetInfo().sNick.c_str()) == 0) 
		PostIrcMessage( "/JOIN %s", pmsg->parameters[1].c_str());

	ShowMessage(pmsg);

	return true;
}


bool CMyMonitor::OnIrc_PINGPONG(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->sCommand == "PING")
		CIrcDefaultMonitor::OnIrc_PING(pmsg);

	return true;
}

bool CMyMonitor::OnIrc_PRIVMSG(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.size() > 1)
	{
		if (IsCTCP(pmsg))
			return true;

		String mess = pmsg->parameters[1];
		bool bIsChannel = IsChannel(pmsg->parameters[0]);

		if (pmsg->m_bIncoming &&!bIsChannel)
		{
			CCSDATA ccs; 
			PROTORECVEVENT pre;

			mess = DoColorCodes((char*)mess.c_str(), TRUE, FALSE);
			ccs.szProtoService = PSR_MESSAGE;

			struct CONTACT_TYPE user ={(char *)pmsg->prefix.sNick.c_str(), (char *)pmsg->prefix.sUser.c_str(), (char *)pmsg->prefix.sHost.c_str(), false, false, false};

			if (CallService(MS_IGNORE_ISIGNORED, NULL, IGNOREEVENT_MESSAGE)) 
			{
				if (!CList_FindContact(&user))
					return true;
			}

			if((prefs->Ignore && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'q')))
			{
				HANDLE hContact = CList_FindContact(&user);
				if (!hContact || (hContact && DBGetContactSettingByte(hContact,"CList", "Hidden", 0) == 1))
					return true;
			}

			ccs.hContact = CList_AddContact(&user, false, true);
			ccs.wParam = 0;
			ccs.lParam = (LPARAM) & pre;
			pre.flags = 0;
			pre.timestamp = (DWORD)time(NULL);
			pre.szMessage = (char *)mess.c_str();
			pre.lParam = 0;
			DBWriteContactSettingString(ccs.hContact, IRCPROTONAME, "User", pmsg->prefix.sUser.c_str());
			DBWriteContactSettingString(ccs.hContact, IRCPROTONAME, "Host", pmsg->prefix.sHost.c_str());
			CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
			return true;
		}
		
		if (bIsChannel)
		{
			if(!(pmsg->m_bIncoming && prefs->Ignore && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'm')))
			{
				if(!pmsg->m_bIncoming)
					mess = ReplaceString(mess, "%%", "%");
				DoEvent(GC_EVENT_MESSAGE, pmsg->parameters[0].c_str(), pmsg->m_bIncoming?pmsg->prefix.sNick.c_str():m_session.GetInfo().sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming?false:true); 				
			}
			return true;
		}
	}

	ShowMessage(pmsg);

	return true;

}

bool CMyMonitor::IsCTCP(const CIrcMessage* pmsg)
{
	// is it a ctcp command, i e is the first and last characer of a PRIVMSG or NOTICE text ASCII 1
	if(pmsg->parameters[1].length() >3 && pmsg->parameters[1][0] == 1 && pmsg->parameters[1][pmsg->parameters[1].length()-1] == 1 )
	{
		// set mess to contain the ctcp command, excluding the leading and trailing  ASCII 1
		String mess = pmsg->parameters[1];
		mess.erase(0,1);
		mess.erase(mess.length()-1,1);
		
		// exploit???
		if(mess.find(1) != string::npos || mess.find("%newl") != string::npos )
		{
			char temp[4096];
			mir_snprintf(temp, 4096, Translate("CTCP ERROR: Malformed CTCP command received from %s!%s@%s. Possible attempt to take control of your irc client registered"), pmsg->prefix.sNick.c_str(), pmsg->prefix.sUser.c_str(), pmsg->prefix.sHost.c_str());
			DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
			return true;
		}

		// extract the type of ctcp command
		String ocommand = GetWord(mess.c_str(), 0);
		String command = GetWord(mess.c_str(), 0);
		transform (command.begin(),command.end(), command.begin(), tolower);

		// should it be ignored?
		if(prefs->Ignore)
		{
			if(IsChannel(pmsg->parameters[0]))
			{
				if (command == "action" && IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'm'))
					return true;

			}
			else
			{
				if (command == "action")
				{
					if(IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'q'))
						return true;
				}
				else if (command == "dcc")
				{
					if( IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'd'))
						return true;
				}
				else if (IsIgnored(pmsg->prefix.sNick, pmsg->prefix.sUser, pmsg->prefix.sHost, 'c'))
					return true;
			}
		}

		if(pmsg->sCommand == "PRIVMSG")
		{
			// incoming ACTION
			if (command == "action")
			{
				mess.erase(0,6);

				if(IsChannel(pmsg->parameters[0]) )
				{
					if(mess.length() >1)
					{
						mess.erase(0,1);
						if(!pmsg->m_bIncoming)
							mess = ReplaceString(mess, "%%", "%");

						DoEvent(GC_EVENT_ACTION, pmsg->parameters[0].c_str(), pmsg->m_bIncoming?pmsg->prefix.sNick.c_str():m_session.GetInfo().sNick.c_str(), mess.c_str(), NULL, NULL, NULL, true, pmsg->m_bIncoming?false:true); 
					}
				}
				else if (pmsg->m_bIncoming)
				{
					mess.insert(0, pmsg->prefix.sNick.c_str());
					mess.insert(0, "* ");
					mess.insert(mess.length(), " *");
					CIrcMessage msg = *pmsg;
					msg.parameters[1] = mess;
					OnIrc_PRIVMSG(&msg);
				}
			}
			// incoming FINGER
			else if (pmsg->m_bIncoming && command == "finger")
			{
				PostIrcMessage("/NOTICE %s \001FINGER %s (%s)\001", pmsg->prefix.sNick.c_str(), prefs->Name, prefs->UserID);
				char temp[300];
				mir_snprintf(temp, sizeof(temp), Translate("CTCP FINGER requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, "Network log", NULL, temp, NULL, NULL, NULL, true, false); 

			}

			// incoming VERSION
			else if (pmsg->m_bIncoming && command == "version")
			{
				PostIrcMessage("/NOTICE %s \001VERSION Miranda IM %s (IRC v %s), (c) J Persson 2003 - 2005\001", pmsg->prefix.sNick.c_str(), "%mirver", "%version");
				char temp[300];
				mir_snprintf(temp, sizeof(temp), Translate("CTCP VERSION requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, "Network log", NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming SOURCE
			else if (pmsg->m_bIncoming && command == "source")
			{
				PostIrcMessage("/NOTICE %s \001SOURCE Get Miranda IRC here: http://miranda-im.org/ \001", pmsg->prefix.sNick.c_str());
				char temp[300];
				mir_snprintf(temp, sizeof(temp), Translate("CTCP SOURCE requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, "Network log", NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming USERINFO
			else if (pmsg->m_bIncoming && command == "userinfo")
			{
				PostIrcMessage("/NOTICE %s \001USERINFO %s\001", pmsg->prefix.sNick.c_str(), prefs->UserInfo);
				char temp[300];
				mir_snprintf(temp, sizeof(temp), Translate("CTCP USERINFO requested by %s") , pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, "Network log", NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming PING
			else if (pmsg->m_bIncoming && command == "ping")
			{
				PostIrcMessage("/NOTICE %s \001%s\001", pmsg->prefix.sNick.c_str(), mess.c_str());
				char temp[300];
				mir_snprintf(temp, sizeof(temp), Translate("CTCP PING requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, "Network log", NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming TIME
			else if (pmsg->m_bIncoming && command == "time")
			{
				time_t tim;
				tim = time(NULL);
				char temp[300];
				lstrcpyn(temp,ctime(&tim), 25);
				PostIrcMessage("/NOTICE %s \001TIME %s\001", pmsg->prefix.sNick.c_str(), temp);
				mir_snprintf(temp, sizeof(temp), Translate("CTCP TIME requested by %s"), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, "Network log", NULL, temp, NULL, NULL, NULL, true, false); 
			}

			// incoming DCC request... lots of stuff happening here...
			else if (pmsg->m_bIncoming && command == "dcc")
			{
				String type = GetWord(mess.c_str(), 1);
				transform (type.begin(),type.end(), type.begin(), tolower);

				// components of a dcc message
				String sFile = "";
				DWORD dwAdr = 0;
				int iPort = 0;
				DWORD dwSize = 0;
				String sToken = "";

				// 1. separate the dcc command into the correct pieces
				if(type == "chat" || type == "send")
				{
					// if the filename is surrounded by quotes, do this
					if(GetWord(mess.c_str(), 2)[0] == '\"')
					{
						int end = 0;
						int begin = mess.find('\"', 0);
						if(begin >= 0)
						{
							end = mess.find('\"', begin + 1); 
							if (end >= 0)
							{
								sFile = mess.substr(begin+1, end-begin-1);

								begin = mess.find(' ', end);
								if(begin >= 0)
								{
									String rest = mess.substr(begin, mess.length());
									dwAdr = atoi(GetWord(rest.c_str(), 0).c_str());
									iPort = atoi(GetWord(rest.c_str(), 1).c_str());
									dwSize = atoi(GetWord(rest.c_str(), 2).c_str());
									sToken = GetWord(rest.c_str(), 3);
								}
							}
						}
					}
					else if (GetWord(mess.c_str(), type == "chat"?4:5) != "") // ... or try another method of separating the dcc command
					{
						int index = type == "chat"?4:5;
						bool bFlag = false;

						// look for the part of the ctcp command that contains adress, port and size
						while (!bFlag && GetWord(mess.c_str(), index) != "")
						{
							String sTemp;
							
							if(type == "chat")
								sTemp = GetWord(mess.c_str(), index-1) + GetWord(mess.c_str(), index);
							else	
								sTemp = GetWord(mess.c_str(), index-2) + GetWord(mess.c_str(), index-1) + GetWord(mess.c_str(), index);
							// if all characters are number it indicates we have found the adress, port and size parameters
							int ind = 0;

							while (sTemp[ind] != '\0')
							{
								if(!isdigit(sTemp[ind]))
									break;
								ind++;
							}
							if(sTemp[ind] == '\0' && GetWord(mess.c_str(), index + ((type == "chat")?1:2)) == "")
								bFlag = true;
							index++;
						}
						if(bFlag)
						{
							char * p1 = strdup(GetWordAddress(mess.c_str(), 2));
							char * p2 = GetWordAddress(p1, index-5);
							
							if(type == "send")
							{
								if(p2 > p1)
								{
									p2--;
									while( p2 != p1 && *p2 == ' ')
									{
										*p2 = '\0';
										p2--;
									}
									sFile = p1;
								}
							}
							else
								sFile = "chat";

							delete [] p1;

							dwAdr = atoi(GetWord(mess.c_str(), index- (type == "chat"?2:3)).c_str());
							iPort = atoi(GetWord(mess.c_str(), index-(type == "chat"?1:2)).c_str());
							dwSize = atoi(GetWord(mess.c_str(), index-1).c_str());
							sToken = GetWord(mess.c_str(), index);
										
						}
					} 
				}
				else if (type == "accept" || type == "resume")
				{
					// if the filename is surrounded by quotes, do this
					if(GetWord(mess.c_str(), 2)[0] == '\"')
					{
						int end = 0;
						int begin = mess.find('\"', 0);
						if(begin >= 0)
						{
							end = mess.find('\"', begin + 1); 
							if (end >= 0)
							{
								sFile = mess.substr(begin+1, end);

								begin = mess.find(' ', end);
								if(begin >= 0)
								{
									String rest = mess.substr(begin, mess.length());
									iPort = atoi(GetWord(rest.c_str(), 0).c_str());
									dwSize = atoi(GetWord(rest.c_str(), 1).c_str());
									sToken = GetWord(rest.c_str(), 2);
								}
							}
						}
					}
					else if (GetWord(mess.c_str(), 4) != "") // ... or try another method of separating the dcc command
					{

						int index = 4;
						bool bFlag = false;

						// look for the part of the ctcp command that contains adress, port and size
						while (!bFlag && GetWord(mess.c_str(), index) != "")
						{
							String sTemp = GetWord(mess.c_str(), index-1) + GetWord(mess.c_str(), index);
							
							// if all characters are number it indicates we have found the adress, port and size parameters
							int ind = 0;

							while (sTemp[ind] != '\0')
							{
								if(!isdigit(sTemp[ind]))
									break;
								ind++;
							}
							if(sTemp[ind] == '\0' && GetWord(mess.c_str(), index + 2) == "")
								bFlag = true;
							index++;
						}
						if(bFlag)
						{
							char * p1 = strdup(GetWordAddress(mess.c_str(), 2));
							char * p2 = GetWordAddress(p1, index-4);
							
							if(p2 > p1)
							{
								p2--;
								while( p2 != p1 && *p2 == ' ')
								{
									*p2 = '\0';
									p2--;
								}
								sFile = p1;
							}

							delete [] p1;

							iPort = atoi(GetWord(mess.c_str(), index-2).c_str());
							dwSize = atoi(GetWord(mess.c_str(), index-1).c_str());
							sToken = GetWord(mess.c_str(), index);
										
						}

					} 
					


				} // /// end separating dcc commands

				// 2. Check for malformed dcc commands or other errors
				if(type == "chat" || type == "send")
				{
					char szTemp[256];
					szTemp[0] = '\0';

					unsigned long ulAdr = 0;
					if (prefs->ManualHost)
						ulAdr = ConvertIPToInteger(prefs->MySpecifiedHostIP);
					else
						ulAdr = ConvertIPToInteger(prefs->IPFromServer?prefs->MyHost:prefs->MyLocalHost);

					if(type == "chat" && !prefs->DCCChatEnabled)
						mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC: Chat request from %s denied"),pmsg->prefix.sNick.c_str());

					else if(type == "send" && !prefs->DCCFileEnabled)
						mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC: File transfer request from %s denied"),pmsg->prefix.sNick.c_str());

					else if(type == "send" && !iPort && ulAdr == 0)
						mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC: Reverse file transfer request from %s denied [No local IP]"),pmsg->prefix.sNick.c_str());

					if(sFile == "" || dwAdr == 0 || dwSize == 0 || iPort == 0 && sToken == "")
						mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC ERROR: Malformed CTCP request from %s [%s]"),pmsg->prefix.sNick.c_str(), mess.c_str());
				
					if(lstrlen(szTemp))
					{
						DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
						return true;
					}

					// remove path from the filename if the remote client (stupidly) sent it
					String sFileCorrected = sFile;
					int i = sFile.rfind("\\", sFile.length());
					if (i != string::npos)
						sFileCorrected = sFile.substr(i+1, sFile.length());
					sFile = sFileCorrected;


				}
				else if(type == "accept" || type == "resume")
				{
					char szTemp[256];
					szTemp[0] = '\0';

					if(type == "resume" && !prefs->DCCFileEnabled)
						mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC: File transfer resume request from %s denied"),pmsg->prefix.sNick.c_str());

					if(sToken == "" && iPort == 0 || sFile == "")
						mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC ERROR: Malformed CTCP request from %s [%s]"),pmsg->prefix.sNick.c_str(), mess.c_str());
				
					if(lstrlen(szTemp))
					{
						DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
						return true;
					}

					// remove path from the filename if the remote client (stupidly) sent it
					String sFileCorrected = sFile;
					int i = sFile.rfind("\\", sFile.length());
					if (i != string::npos)
						sFileCorrected = sFile.substr(i+1, sFile.length());
					sFile = sFileCorrected;


				}

				// 3. Take proper actions considering type of command

				// incoming chat request
				if(type == "chat")
				{
					CONTACT user ={(char *)pmsg->prefix.sNick.c_str(), 0, 0, false, false, true};
					HANDLE hContact = CList_FindContact(&user);

					// check if it should be ignored
					if(prefs->DCCChatIgnore == 1 
						|| prefs->DCCChatIgnore == 2 
						&& hContact && DBGetContactSettingByte(hContact,"CList", "NotOnList", 0) == 0
						&& DBGetContactSettingByte(hContact,"CList", "Hidden", 0) == 0)
					{
						String host = pmsg->prefix.sUser + "@" + pmsg->prefix.sHost;
						CList_AddDCCChat(pmsg->prefix.sNick, host, dwAdr, iPort); // add a CHAT event to the clist
					}
					else
					{
						char szTemp[512];
						mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC: Chat request from %s denied"),pmsg->prefix.sNick.c_str());
						DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
					}

				}

				// remote requested that the file should be resumed
				if(type == "resume")
				{
					CDccSession * dcc = NULL;

					if (sToken =="")
						dcc = g_ircSession.FindDCCSendByPort(iPort);
					else
						dcc = g_ircSession.FindPassiveDCCSend(atoi(sToken.c_str())); // reverse ft

					if(dcc)
					{
						InterlockedExchange(&dcc->dwWhatNeedsDoing, (long)FILERESUME_RESUME);
						InterlockedExchange(&dcc->dwResumePos, dwSize); // dwSize is the resume position
						PostIrcMessage("/PRIVMSG %s \001DCC ACCEPT %s\001", pmsg->prefix.sNick.c_str(), GetWordAddress(mess.c_str(), 2));
					}
				}

				// remote accepted your request for a file resume
				if(type == "accept")
				{
					CDccSession * dcc = NULL;

					if (sToken =="")
						dcc = g_ircSession.FindDCCRecvByPortAndName(iPort, (char *)pmsg->prefix.sNick.c_str());
					else
						dcc = g_ircSession.FindPassiveDCCRecv(pmsg->prefix.sNick, sToken); // reverse ft

					if(dcc)
					{
						InterlockedExchange(&dcc->dwWhatNeedsDoing, (long)FILERESUME_RESUME);
						InterlockedExchange(&dcc->dwResumePos, dwSize);	// dwSize is the resume position					
						SetEvent(dcc->hEvent);
					}
				}

				if (type == "send")
				{
					String sTokenBackup = sToken;
					bool bTurbo = false; // TDCC indicator

					if(sToken != "" && sToken[sToken.length()-1] == 'T')
					{
						bTurbo = true;
						sToken.erase(sToken.length()-1,1);
					}

					// if a token exists and the port is non-zero it is the remote
					// computer telling us that is has accepted to act as server for
					// a reverse filetransfer. The plugin should connect to that computer
					// and start sedning the file (if the token is valid). Compare to DCC RECV
					if(sToken != "" && iPort)
					{
						CDccSession * dcc = g_ircSession.FindPassiveDCCSend(atoi(sToken.c_str()));

						if(dcc)
						{
							dcc->SetupPassive(dwAdr, iPort);
							dcc->Connect();
						}
					}

					else 
					{
						struct CONTACT_TYPE user ={(char *)pmsg->prefix.sNick.c_str(), (char *)pmsg->prefix.sUser.c_str(), (char *)pmsg->prefix.sHost.c_str(), false, false, false};
						if (CallService(MS_IGNORE_ISIGNORED, NULL, IGNOREEVENT_FILE)) 
						{
							if (!CList_FindContact(&user))
								return true;
						}

						HANDLE hContact = CList_AddContact(&user, false, true);
						if(hContact)
						{
							char * szBlob;
							CCSDATA ccs; 
							PROTORECVEVENT pre;
							ccs.szProtoService = PSR_FILE;
							DCCINFO * di = new DCCINFO;
							
							ZeroMemory(di, sizeof(DCCINFO));
							di->hContact = hContact;
							di->sFile = sFile;
							di->dwSize = dwSize;
							di->sContactName = pmsg->prefix.sNick;
							di->dwAdr = dwAdr;
							di->iPort = iPort;
							di->iType = DCC_SEND;
							di->bSender = false;
							di->bTurbo = bTurbo;
							di->bSSL = false;
							di->bReverse = (iPort == 0 && sToken !="")?true:false;
							if(di->bReverse)
								di->sToken = sTokenBackup;

							pre.flags = 0;
							pre.timestamp = (DWORD)time(NULL);

							szBlob = (char *) malloc(sizeof(DWORD) + di->sFile.length() + 3);
							*((PDWORD) szBlob) = (DWORD) di;
							strcpy(szBlob + sizeof(DWORD), di->sFile.c_str());
							strcpy(szBlob + sizeof(DWORD) + di->sFile.length() + 1, " ");

							DBWriteContactSettingString(hContact, IRCPROTONAME, "User", pmsg->prefix.sUser.c_str());
							DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", pmsg->prefix.sHost.c_str());

							pre.szMessage = szBlob;
							pre.lParam = 0;
							ccs.hContact = hContact;
							ccs.wParam = 0;
							ccs.lParam = (LPARAM) & pre;
							CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
						}

					}
				} // end type == "send"

			}
			else if (pmsg->m_bIncoming)
			{
				char temp[300];
				mir_snprintf(temp, sizeof(temp), Translate("CTCP %s requested by %s"), ocommand.c_str(), pmsg->prefix.sNick.c_str());
				DoEvent(GC_EVENT_INFORMATION, "Network log", NULL, temp, NULL, NULL, NULL, true, false); 
			}
		}

		// handle incoming ctcp in notices. This technique is used for replying to CTCP queries
		else if(pmsg->sCommand == "NOTICE")
		{
			char szTemp[255]; 
			szTemp[0] = '\0';

			// if the whois window is visible and the ctcp reply belongs to the user in it, then show the reply in the whois window
			if(whois_hWnd && IsWindowVisible(whois_hWnd))
			{
				SendMessage(GetDlgItem(whois_hWnd, IDC_INFO_NICK), WM_GETTEXT, 255, (LPARAM) szTemp);
				if(lstrcmpi(szTemp, pmsg->prefix.sNick.c_str()) == 0)
				{
					if (pmsg->m_bIncoming && (command == "version" ||command == "userinfo" || command == "time"))
					{
						SetActiveWindow(whois_hWnd);
						SetDlgItemText(whois_hWnd, IDC_REPLY, DoColorCodes(GetWordAddress(mess.c_str(), 1), TRUE, FALSE));
						return true;
					}
					if (pmsg->m_bIncoming && command == "ping")
					{
						SetActiveWindow(whois_hWnd);
						int s = (int)time(0) - (int)atol(GetWordAddress(mess.c_str(), 1));
						char szTemp[30];
						if (s==1)
							mir_snprintf(szTemp, sizeof(szTemp), "%u second", s); 
						else
							mir_snprintf(szTemp, sizeof(szTemp), "%u seconds", s); 

						SetDlgItemText(whois_hWnd, IDC_REPLY, DoColorCodes(szTemp, TRUE, FALSE));
						return true;
					}

				}
			}

			char temp[300];	

			//... else show the reply in the current window
			if (pmsg->m_bIncoming && command == "ping")
			{
				int s = (int)time(0) - (int)atol(GetWordAddress(mess.c_str(), 1));
				mir_snprintf(temp, sizeof(temp), Translate("CTCP PING reply from %s: %u sec(s)"), pmsg->prefix.sNick.c_str(), s); 
				DoEvent(GC_EVENT_INFORMATION, NULL, NULL, temp, NULL, NULL, NULL, true, false); 
			}
			else
			{
				mir_snprintf(temp, sizeof(temp), Translate("CTCP %s reply from %s: %s"), ocommand.c_str(), pmsg->prefix.sNick.c_str(), GetWordAddress(mess.c_str(), 1));	
				DoEvent(GC_EVENT_INFORMATION, NULL, NULL, temp, NULL, NULL, NULL, true, false); 
			}

		}		
		return true;
	}
	return false;
}

bool CMyMonitor::OnIrc_NAMES(const CIrcMessage* pmsg)
{

	if(pmsg->m_bIncoming && pmsg->parameters.size()>3)
	{
		sNamesList += pmsg->parameters[3] + " ";
	}

	ShowMessage(pmsg);
	
	return true;
	
}

bool CMyMonitor::OnIrc_ENDNAMES(const CIrcMessage* pmsg)
{
	HWND hActiveWindow = GetActiveWindow();
	if(pmsg->m_bIncoming && pmsg->parameters.size()>1)
	{
		String name = "a";
		int i = 0;
		BOOL bFlag = false;

		// Is the user on the names list?
		while (name != "")
		{
			name = GetWord(sNamesList.c_str(), i);
			i++;
			if(name != "")
			{
				int index = 0;

				while(strchr(sUserModePrefixes.c_str(), name[index]))
					index++;

				if(!lstrcmpi(name.substr(index, name.length()).c_str(), m_session.GetInfo().sNick.c_str()))
				{
					bFlag = true;
					break;
				}
			}
		}
		if(bFlag)
		{
			String sChanName = pmsg->parameters[1];
			if (sChanName[0] == '@' || sChanName[0] == '*' || sChanName[0] == '=')
				sChanName.erase(0,1);


			// Add a new chat window
			GCSESSION gcw = {0};
			String sID = MakeWndID(sChanName) ;

			gcw.cbSize = sizeof(GCSESSION);
			gcw.iType = GCW_CHATROOM;
			gcw.pszID = sID.c_str();
			gcw.pszModule = IRCPROTONAME;
			gcw.pszName = sChanName.c_str();
			if(!CallService(MS_GC_NEWSESSION, 0, (LPARAM)&gcw))
	
			{
				DBVARIANT dbv;
				GCDEST gcd = {0};
				GCEVENT gce = {0};
				String sTemp;
				int i = 0;

				PostIrcMessage( "/MODE %s", sChanName.c_str());

				gcd.pszID = (char*)sID.c_str();
				gcd.pszModule = IRCPROTONAME;
				gcd.iType = GC_EVENT_ADDGROUP;
				gce.time = 0;

				//register the statuses
				gce.cbSize = sizeof(GCEVENT);
				gce.pDest = &gcd;

				gce.pszStatus = "Owner";
				CallChatEvent(0, (LPARAM)&gce);
				gce.pszStatus = "Admin";
				CallChatEvent(0, (LPARAM)&gce);
				gce.pszStatus = "Op";
				CallChatEvent(0, (LPARAM)&gce);
				gce.pszStatus = "Halfop";
				CallChatEvent(0, (LPARAM)&gce);
				gce.pszStatus = "Voice";
				CallChatEvent(0, (LPARAM)&gce);
				gce.pszStatus = "Normal";
				CallChatEvent(0, (LPARAM)&gce);

				i = 0;
				sTemp = GetWord(sNamesList.c_str(), i);

				// Fill the nicklist
				while(sTemp != "")
				{
					String sStat;
					String sTemp2 = sTemp;
					sStat = PrefixToStatus(sTemp[0]);
					
					// fix for networks like freshirc where they allow more than one prefix
					while(PrefixToStatus(sTemp[0]) != "Normal")
						sTemp.erase(0,1);
					
					gcd.iType = GC_EVENT_JOIN;
					gce.pszUID = sTemp.c_str();
					gce.pszNick = sTemp.c_str();
					gce.pszStatus = sStat.c_str();
					BOOL bIsMe = (lstrcmpi(gce.pszNick, m_session.GetInfo().sNick.c_str()) == 0)?TRUE:FALSE;
					gce.dwFlags = 0;
					gce.bIsMe = bIsMe;
					gce.time = bIsMe?time(0):0;

					CallChatEvent(0, (LPARAM)&gce);

					// fix for networks like freshirc where they allow more than one prefix
					if(PrefixToStatus(sTemp2[0]) != "Normal")
					{
						sTemp2.erase(0,1);
						sStat = PrefixToStatus(sTemp2[0]);
						while(sStat != "Normal")
						{
							DoEvent(GC_EVENT_ADDSTATUS, (char*)sID.c_str(), sTemp.c_str(), "system", sStat.c_str(), NULL, NULL, false, false, 0); 
							sTemp2.erase(0,1);
							sStat = PrefixToStatus(sTemp2[0]);
						}

					}

					i++;
					sTemp = GetWord(sNamesList.c_str(), i);
				}
				//Set the item data for the window
				{
					FreeWindowItemData(sChanName, NULL);

					CHANNELINFO * wi = new CHANNELINFO;
					wi->pszLimit = 0;
					wi->pszMode = 0;
					wi->pszPassword = 0;
					wi->pszTopic = 0;
					DoEvent(GC_EVENT_SETITEMDATA, sChanName.c_str(), NULL, NULL, NULL, NULL, (DWORD)wi, false, false, 0);

					if(sTopic != "" && !lstrcmpi(GetWord(sTopic.c_str(), 0).c_str() , sChanName.c_str()))
					{
						DoEvent(GC_EVENT_TOPIC, sChanName.c_str(), sTopicName==""?NULL:sTopicName.c_str(), GetWordAddress(sTopic.c_str(), 1), NULL, NULL, NULL, true, false);
						AddWindowItemData(sChanName, 0, 0, 0, GetWordAddress(sTopic.c_str(), 1));
						sTopic = "";
						sTopicName = "";
					}
				}
				
				gcd.pszID = (char*)sID.c_str();
				gcd.iType = GC_EVENT_CONTROL;
				gce.cbSize = sizeof(GCEVENT);
				gce.dwFlags = 0;
				gce.bIsMe = false;
				gce.dwItemData = false;
				gce.pszNick = NULL;
				gce.pszStatus = NULL;
				gce.pszText = NULL;
				gce.pszUID = NULL;
				gce.pszUserInfo = NULL;
				gce.time = time(0);
				gce.pDest = &gcd;
	
				if (!DBGetContactSetting(NULL, IRCPROTONAME, "JTemp", &dbv) && dbv.type == DBVT_ASCIIZ) 
				{
					String command = "a";
					String save = "";
					int i = 0;

					while(command != "")
					{
						command = GetWord(dbv.pszVal, i);
						i++;
						if(command != "")
						{
							String S = command.substr(1, command.length());
							if( !lstrcmpi((char *)sChanName.c_str(), (char *)S.c_str()))
								break;

							save += command;
							save += " ";
						}
					}

					if (command != "")
					{
						save += GetWordAddress(dbv.pszVal, i);
						switch (command[0])
						{
						case 'M':
							CallChatEvent( WINDOW_HIDDEN, (LPARAM)&gce);
							break;
						case 'X':
							CallChatEvent( WINDOW_MAXIMIZE, (LPARAM)&gce);
							break;
						default:
							CallChatEvent( SESSION_INITDONE, (LPARAM)&gce);
							break;
						}

					}
					else 
						CallChatEvent( SESSION_INITDONE, (LPARAM)&gce);


					if (save == "")
						DBDeleteContactSetting(NULL, IRCPROTONAME, "JTemp");
					else
						DBWriteContactSettingString(NULL, IRCPROTONAME, "JTemp", save.c_str());
					DBFreeVariant(&dbv);
				}
				else 
					CallChatEvent( SESSION_INITDONE, (LPARAM)&gce);

				{

					gcd.iType = GC_EVENT_CONTROL;

					gce.pDest = &gcd;
					CallChatEvent( SESSION_ONLINE, (LPARAM)&gce);
					

				}
		
			
			}

//			if(prefs->AutoOnlineNotification && !bTempDisableCheck || bTempForceCheck)
//				DoUserhostWithReason(2, "S", true, "%s", sChanName.c_str());
//			SetChatTimer(OnlineNotifTimer3, prefs->OnlineNotificationTime*500, OnlineNotifTimerProc3);
					
		}
	}

	sNamesList = "";

	ShowMessage(pmsg);

	return true;
}

bool CMyMonitor::OnIrc_INITIALTOPIC(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming&& pmsg->parameters.size() >2)
	{
		AddWindowItemData(pmsg->parameters[1].c_str(), 0, 0, 0, (char*)pmsg->parameters[2].c_str());
		sTopic = pmsg->parameters[1] + " " + pmsg->parameters[2];
		sTopicName = "";
	}
	ShowMessage(pmsg);

	return true;
}
bool CMyMonitor::OnIrc_INITIALTOPICNAME(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming&& pmsg->parameters.size() >2)
	{
		sTopicName = pmsg->parameters[2];
	}
	ShowMessage(pmsg);

	return true;
}
bool CMyMonitor::OnIrc_TOPIC(const CIrcMessage* pmsg)
{
	if (pmsg->parameters.size() > 1 && pmsg->m_bIncoming) 
	{
		DoEvent(GC_EVENT_TOPIC, pmsg->parameters[0].c_str(), pmsg->prefix.sNick.c_str(), pmsg->parameters[1].c_str(), NULL, NULL, NULL, true, false);
		AddWindowItemData(pmsg->parameters[0].c_str(), 0, 0, 0, (char*)pmsg->parameters[1].c_str());
	}

	ShowMessage(pmsg);

	return true;
}

bool CMyMonitor::OnIrc_LISTSTART(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming) {
		if( list_hWnd == NULL) 
			list_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_LIST),NULL,ListWndProc);
		ChannelNumber = 0;
	}
	ShowMessage(pmsg);
	return true;
}
bool CMyMonitor::OnIrc_LIST(const CIrcMessage* pmsg)
{

	if (pmsg->m_bIncoming==1 && list_hWnd && pmsg->parameters.size() > 2)
	{

		ChannelNumber++;
		LVITEM lvItem;
		HWND hListView = GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW);
		lvItem.iItem = ListView_GetItemCount(hListView); 
		lvItem.mask = LVIF_TEXT |LVIF_PARAM;
		lvItem.iSubItem = 0;
		lvItem.pszText = (char *)pmsg->parameters[1].c_str();
		lvItem.lParam = lvItem.iItem;
		lvItem.iItem = ListView_InsertItem(hListView,&lvItem);
		lvItem.mask = LVIF_TEXT;
		lvItem.iSubItem =1;
		lvItem.pszText = (char *)pmsg->parameters[pmsg->parameters.size()-2].c_str();
		ListView_SetItem(hListView,&lvItem);

		char * temp= new char [lstrlen(pmsg->parameters[pmsg->parameters.size()-1].c_str())+1];
		strcpy(temp, (char *)pmsg->parameters[pmsg->parameters.size()-1].c_str());
		char * find = strstr(temp , "[+");
		char *find2 = strstr(temp , "]");
		char * save = temp;
		if (find == temp && find2 != NULL && find +8 >= find2)
		{
			temp = strstr(temp, "]");
			if (lstrlen(temp) > 1)
			{
				temp++;
				temp[0] = '\0';
				lvItem.iSubItem =2;
				lvItem.pszText = save;
				ListView_SetItem(hListView,&lvItem);
				temp[0] = ' ';
				temp++;
			}
			else
				temp =save;
		}
		
		lvItem.iSubItem =3;
		String S = DoColorCodes(temp, TRUE, FALSE);
		lvItem.pszText = (char *)S.c_str();
		ListView_SetItem(hListView,&lvItem);
		temp = save;
		delete []temp;
	

		int percent = 100;
		if (NoOfChannels > 0)
			percent = (int)(ChannelNumber*100) / NoOfChannels;
		char text[50];
		if ( percent < 100)
		mir_snprintf(text, 49, Translate(	"Downloading list (%u%%) - %u channels"	), percent, ChannelNumber);
		else
			mir_snprintf(text, 49, Translate(	"Downloading list - %u channels"	), ChannelNumber);
		SetDlgItemText(list_hWnd, IDC_TEXT, text);
	}
	
	return true;


}
bool CMyMonitor::OnIrc_LISTEND(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && list_hWnd)
	{

		EnableWindow(GetDlgItem(list_hWnd, IDC_JOIN), true);
		ListView_SetSelectionMark(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), 0);		
		ListView_SetColumnWidth(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), 1, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), 2, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), 3, LVSCW_AUTOSIZE);
		SendMessage(list_hWnd, IRC_UPDATELIST, 0, 0);

		char text[90];
		mir_snprintf(text, 40, Translate(	"Done: %u channels"	), ChannelNumber);
		int percent = 100;
		if (NoOfChannels > 0)
			percent = (int)(ChannelNumber*100) / NoOfChannels;
		if (percent <70) {
			lstrcat(text, " ");
			lstrcat(text, Translate(	"(probably truncated by server)"	));
		}
		SetDlgItemText(list_hWnd, IDC_TEXT, text);
	}
	ShowMessage(pmsg);
	return true;
}
bool CMyMonitor::OnIrc_BANLIST(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.size() >2)
	{

		if(manager_hWnd && (
			IsDlgButtonChecked(manager_hWnd, IDC_RADIO1) && pmsg->sCommand == "367"
			|| IsDlgButtonChecked(manager_hWnd, IDC_RADIO2) && pmsg->sCommand == "346"
			|| IsDlgButtonChecked(manager_hWnd, IDC_RADIO3) && pmsg->sCommand == "348"
			) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO1)) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO2)) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO3))
			)
		{
			String S = pmsg->parameters[2];
			if(pmsg->parameters.size() > 3)
			{
				S += "   - ";
				S += pmsg->parameters[3];
				if (pmsg->parameters.size() > 4)
				{
					S += " -  ( ";
					time_t time;
					time = StrToInt(pmsg->parameters[4].c_str());
					S += ctime(&time);
					S = ReplaceString(S.c_str(), "\n", " ");
					S += ")";
				}
			}

			SendDlgItemMessage(manager_hWnd, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)S.c_str());
		}
		
	}
	ShowMessage(pmsg);
	return true;
}
bool CMyMonitor::OnIrc_BANLISTEND(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.size() >1)
	{

		if(manager_hWnd && (
			IsDlgButtonChecked(manager_hWnd, IDC_RADIO1) && pmsg->sCommand == "368"
			|| IsDlgButtonChecked(manager_hWnd, IDC_RADIO2) && pmsg->sCommand == "347"
			|| IsDlgButtonChecked(manager_hWnd, IDC_RADIO3) && pmsg->sCommand == "349"
			) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO1)) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO2)) && !IsWindowEnabled(GetDlgItem(manager_hWnd, IDC_RADIO3))
			)
		{
			if (strchr(sChannelModes.c_str(), 'b'))
				EnableWindow(GetDlgItem(manager_hWnd, IDC_RADIO1), true);
			if (strchr(sChannelModes.c_str(), 'I'))
				EnableWindow(GetDlgItem(manager_hWnd, IDC_RADIO2), true);
			if (strchr(sChannelModes.c_str(), 'e'))
				EnableWindow(GetDlgItem(manager_hWnd, IDC_RADIO3), true);
			if (!IsDlgButtonChecked(manager_hWnd, IDC_NOTOP))
				EnableWindow(GetDlgItem(manager_hWnd, IDC_ADD), true);
		}

	}
	ShowMessage(pmsg);
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_NAME(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.size() > 5 && ManualWhoisCount > 0) 
	{
		if(whois_hWnd==NULL)
			whois_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_INFO),NULL,InfoWndProc);
		if (SendMessage(GetDlgItem(whois_hWnd, IDC_INFO_NICK), CB_FINDSTRINGEXACT, -1, (LPARAM) pmsg->parameters[1].c_str()) == CB_ERR)	
			SendMessage(GetDlgItem(whois_hWnd, IDC_INFO_NICK), CB_ADDSTRING, 0, (LPARAM) pmsg->parameters[1].c_str());	
		int i = SendMessage(GetDlgItem(whois_hWnd, IDC_INFO_NICK), CB_FINDSTRINGEXACT, -1, (LPARAM) pmsg->parameters[1].c_str());
		SendMessage(GetDlgItem(whois_hWnd, IDC_INFO_NICK), CB_SETCURSEL, i, 0);	
		SetWindowText(GetDlgItem(whois_hWnd, IDC_CAPTION), pmsg->parameters[1].c_str());	

		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_NAME), pmsg->parameters[5].c_str());	
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_ADDRESS), pmsg->parameters[3].c_str());
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_ID), pmsg->parameters[2].c_str());
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_CHANNELS), '\0');
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_SERVER), '\0');
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_AWAY2), '\0');
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_AUTH), '\0');
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_OTHER), '\0');
		SetWindowText(GetDlgItem(whois_hWnd, IDC_REPLY), '\0');
		SetWindowText(whois_hWnd, Translate("User information"));
		EnableWindow(GetDlgItem(whois_hWnd, ID_INFO_QUERY), true);
		ShowWindow(whois_hWnd, SW_SHOW);
		if (IsIconic(whois_hWnd))
			ShowWindow(whois_hWnd, SW_SHOWNORMAL);
		SendMessage(whois_hWnd, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(whois_hWnd, NULL, TRUE);
	}

	ShowMessage(pmsg);

	return true;
}

bool CMyMonitor::OnIrc_WHOIS_CHANNELS(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() >2 && ManualWhoisCount > 0)
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_CHANNELS), pmsg->parameters[2].c_str());
	ShowMessage(pmsg);
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_AWAY(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() >2 && ManualWhoisCount > 0)
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_AWAY2), pmsg->parameters[2].c_str());
	if (ManualWhoisCount <1 && pmsg->m_bIncoming && pmsg->parameters.size() >2)
		WhoisAwayReply = pmsg->parameters[2];
	ShowMessage(pmsg);
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_OTHER(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() >2 && ManualWhoisCount > 0) {
		char temp[1024];
		char temp2[1024];
		GetDlgItemText(whois_hWnd, IDC_INFO_OTHER, temp, 1000);
		lstrcat(temp, "%s\r\n");
		mir_snprintf(temp2, 1020, temp, pmsg->parameters[2].c_str()); 
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_OTHER), temp2);
	}
	ShowMessage(pmsg);
	return true;
}
bool CMyMonitor::OnIrc_WHOIS_END(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.size() >1 && ManualWhoisCount < 1) {
		struct CONTACT_TYPE user ={(char *)pmsg->parameters[1].c_str(), NULL, NULL, false, false, true};
		HANDLE hContact = CList_FindContact(&user);
		if (hContact)
			ProtoBroadcastAck(IRCPROTONAME, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM) WhoisAwayReply.c_str());

	}
	ManualWhoisCount--;
	if (ManualWhoisCount < 0)
		ManualWhoisCount = 0;
	ShowMessage(pmsg);
	return true;
}
bool CMyMonitor::OnIrc_WHOIS_IDLE(const CIrcMessage* pmsg)
{
	int D = 0;
	int H = 0;
	int M = 0;
	int S = 0;
	if (pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() >2 && ManualWhoisCount > 0){
		S = StrToInt(pmsg->parameters[2].c_str());
		D = S/(60*60*24);
		S -= (D * 60 * 60 *24);
		H = S/(60*60);
		S -= (H * 60 * 60);
		M = S/60;
		S -= (M * 60 );
		char temp[100];
		if (D)
			mir_snprintf(temp ,99, "%ud, %uh, %um, %us", D, H, M, S);
		else if (H)
			mir_snprintf(temp ,99,"%uh, %um, %us", H, M, S);
		else if (M)
			mir_snprintf(temp ,99,"%um, %us", M, S);
		else if (S)
			mir_snprintf(temp ,99,"%us", S);

		char temp2[256];
		GetWindowText(GetDlgItem(whois_hWnd, IDC_CAPTION), temp2, 255);
		char temp3[256];
		mir_snprintf(temp3, 255 ,"%s (idle %s)", temp2, temp);
		SetWindowText(GetDlgItem(whois_hWnd, IDC_CAPTION), temp3);
	
	
	}
	ShowMessage(pmsg);
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_SERVER(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() >2 && ManualWhoisCount > 0)
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_SERVER), pmsg->parameters[2].c_str());
	ShowMessage(pmsg);
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_AUTH(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && whois_hWnd && pmsg->parameters.size() >2 && ManualWhoisCount > 0)
		if (pmsg->sCommand == "330")
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_AUTH), pmsg->parameters[2].c_str());
		else if (pmsg->parameters[2] == "is an identified user" || pmsg->parameters[2] == "is a registered nick")
		SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_AUTH), pmsg->parameters[2].c_str());
		else
			OnIrc_WHOIS_OTHER(pmsg);
	ShowMessage(pmsg);
	return true;
}

bool CMyMonitor::OnIrc_WHOIS_NO_USER(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.size() >2 && !IsChannel(pmsg->parameters[1]))
	{
		if (whois_hWnd) {
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_NICK), pmsg->parameters[2].c_str());
			SendMessage(GetDlgItem(whois_hWnd, IDC_INFO_NICK), CB_SETEDITSEL, 0,MAKELPARAM(0,-1));
			SetWindowText(GetDlgItem(whois_hWnd, IDC_CAPTION), pmsg->parameters[2].c_str());	
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_NAME), '\0');	
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_ADDRESS), '\0');
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_ID), '\0');
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_CHANNELS), '\0');
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_SERVER), '\0');
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_AWAY2), '\0');
			SetWindowText(GetDlgItem(whois_hWnd, IDC_INFO_AUTH), '\0');
			SetWindowText(GetDlgItem(whois_hWnd, IDC_REPLY), '\0');
			EnableWindow(GetDlgItem(whois_hWnd, ID_INFO_QUERY), false);
		}
		
		struct CONTACT_TYPE user ={(char *)pmsg->parameters[1].c_str(), NULL, NULL, false, false, false};
		HANDLE hContact = CList_FindContact(&user);
		if (hContact) 
		{
			AddOutgoingMessageToDB(hContact, (char*)((String)"> "  + pmsg->parameters[2] + (String)": " + pmsg->parameters[1]).c_str() );
			DBVARIANT dbv;
			if (!DBGetContactSetting(hContact, IRCPROTONAME, "Default", &dbv) && dbv.type == DBVT_ASCIIZ ) 
			{
				DBWriteContactSettingString(hContact, IRCPROTONAME, "Nick", dbv.pszVal);
				DBVARIANT dbv2;
				if(DBGetContactSettingByte(hContact, IRCPROTONAME, "AdvancedMode", 0) == 0)
					DoUserhostWithReason(1, ((String)"S" + dbv.pszVal).c_str(), true, dbv.pszVal);
				else
				{
					if (!DBGetContactSetting(hContact, IRCPROTONAME, "UWildcard", &dbv2) && dbv2.type == DBVT_ASCIIZ)
					{
						DoUserhostWithReason(2, ((String)"S" + dbv2.pszVal).c_str(), true, dbv2.pszVal);
						DBFreeVariant(&dbv2);
					}
					else
					{
						DoUserhostWithReason(2, ((String)"S" + dbv.pszVal).c_str(), true, dbv.pszVal);
					}
				}
				DBWriteContactSettingString(hContact, IRCPROTONAME, "User", "");
				DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", "");
				DBFreeVariant(&dbv);
			}
		}
	}

	ShowMessage(pmsg);
	return true;
}
bool CMyMonitor::OnIrc_NICK_ERR(const CIrcMessage* pmsg)
{
	if ( (nickflag || lstrlen(prefs->AlternativeNick)== 0) && pmsg->m_bIncoming && pmsg->parameters.size() >2) 
	{
		if (nick_hWnd == NULL)
		{
			nick_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_NICK),NULL,NickWndProc);
		}
		SetWindowText(GetDlgItem(nick_hWnd, IDC_CAPTION), Translate(	"Change nickname" ));
		SetWindowText(GetDlgItem(nick_hWnd, IDC_TEXT), pmsg->parameters[2].c_str());
		SetWindowText(GetDlgItem(nick_hWnd, IDC_ENICK), pmsg->parameters[1].c_str());
		SendMessage(GetDlgItem(nick_hWnd, IDC_ENICK), CB_SETEDITSEL, 0,MAKELPARAM(0,-1));
	}
	else if (pmsg->m_bIncoming )
	{
		char m[40];
		mir_snprintf(m, 40, "NICK %s", prefs->AlternativeNick);
		if( g_ircSession )
			g_ircSession << irc::CIrcMessage(m);

		nickflag =true;

	}
	ShowMessage(pmsg);
 	return true;
}

bool CMyMonitor::OnIrc_JOINERROR(const CIrcMessage* pmsg)
{
	
	if(pmsg->m_bIncoming) 
	{
		DBVARIANT dbv;		
		if (!DBGetContactSetting(NULL, IRCPROTONAME, "JTemp", &dbv) && dbv.type == DBVT_ASCIIZ) 
		{
			String command = "a";
			String save = "";
			int i = 0;

			while(command != "")
			{
				command = GetWord(dbv.pszVal, i);
				i++;

				if(command != "" && pmsg->parameters[0] == command.substr(1, command.length()))
				{
					save += command;
					save += " ";
				}
			}
			DBFreeVariant(&dbv);

			if (save == "")
				DBDeleteContactSetting(NULL, IRCPROTONAME, "JTemp");
			else
				DBWriteContactSettingString(NULL, IRCPROTONAME, "JTemp", save.c_str());
		}
	}

	ShowMessage(pmsg);

	return true;
}
bool CMyMonitor::OnIrc_UNKNOWN(const CIrcMessage* pmsg)
{
	if(pmsg->m_bIncoming && pmsg->parameters.size() > 0)
	{
		if(pmsg->parameters[0] == "WHO" && GetNextUserhostReason(2) !="U")
			return true;
		if(pmsg->parameters[0] == "USERHOST" && GetNextUserhostReason(1) !="U")
			return true;
	}
	ShowMessage(pmsg);
	return true;
}
bool CMyMonitor::OnIrc_ENDMOTD(const CIrcMessage* pmsg)
{
	if(pmsg->m_bIncoming && !bPerformDone)
		DoOnConnect(pmsg);
	ShowMessage(pmsg);
	return true;
}

bool CMyMonitor::OnIrc_NOOFCHANNELS(const CIrcMessage* pmsg)
{
	if (pmsg->m_bIncoming && pmsg->parameters.size() >1)
		NoOfChannels = StrToInt(pmsg->parameters[1].c_str());

	if (pmsg->m_bIncoming && !bPerformDone) {
		DoOnConnect(pmsg);
	}
	ShowMessage(pmsg);
	return true;
}


bool CMyMonitor::OnIrc_ERROR(const CIrcMessage* pmsg)
{
	if	(pmsg->m_bIncoming && !prefs->DisableErrorPopups ) {
		MIRANDASYSTRAYNOTIFY msn;
		msn.cbSize = sizeof(MIRANDASYSTRAYNOTIFY);
		msn.szProto = IRCPROTONAME;
		msn.szInfoTitle = Translate(	"IRC error" );
		String S;
		if (pmsg->parameters.size() > 0) 
			S = DoColorCodes(pmsg->parameters[0].c_str(), TRUE, FALSE);
		else
			S = Translate(	"Unknown" );
		msn.szInfo = (char *)S.c_str();
		msn.dwInfoFlags = NIIF_ERROR;
		msn.uTimeout = 15000;
		CallService(MS_CLIST_SYSTRAY_NOTIFY, (WPARAM)NULL,(LPARAM) &msn);
	}
	ShowMessage(pmsg);
	return true;	
}
bool CMyMonitor::OnIrc_WHO_END(const CIrcMessage* pmsg)
{
	String command = GetNextUserhostReason(2);
	if(command[0] == 'S') 
	{
		if(pmsg->m_bIncoming && pmsg->parameters.size() > 1 )
		{
			// is it a channel?
			if(IsChannel(pmsg->parameters[1]))
			{
				String S = "";
				String SS = WhoReply;
				String User = GetWord(WhoReply.c_str(), 0);
				while (!User.empty()) 
				{
					if(GetWord(WhoReply.c_str(), 3)[0] == 'G')
					{
						S += User;
						S += "\t";
					}
					SS = GetWordAddress(WhoReply.c_str(), 4);
					if(SS.empty())
						break;
					WhoReply = SS;
					User = GetWord(WhoReply.c_str(), 0);
				}
				DoEvent(GC_EVENT_SETSTATUSEX, pmsg->parameters[1].c_str(), NULL, S == ""?NULL:S.c_str(), NULL, NULL, GC_SSE_TABDELIMITED, FALSE, FALSE); 

				return true;
			}


			/// if it is not a channel
			char * UserList = new char [lstrlen(WhoReply.c_str())+2];
			lstrcpyn(UserList, WhoReply.c_str(), lstrlen(WhoReply.c_str())+1);
			char * p1= UserList;
			WhoReply = "";
			struct CONTACT_TYPE user ={(char *)pmsg->parameters[1].c_str(), NULL, NULL, false, true, false};
			HANDLE hContact = CList_FindContact(&user);

			if (hContact && DBGetContactSettingByte(hContact, IRCPROTONAME, "AdvancedMode", 0) == 1) 
			{
				char * DBDefault = NULL;
				char * DBNick = NULL;
				char * DBWildcard = NULL;
				char * DBUser = NULL;
				char * DBHost = NULL;
				char * DBManUser = NULL;
				char * DBManHost = NULL;
				DBVARIANT dbv1;
				DBVARIANT dbv2;	
				DBVARIANT dbv3;	
				DBVARIANT dbv4;	
				DBVARIANT dbv5;	
				DBVARIANT dbv6;	
				DBVARIANT dbv7;	
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "Default", &dbv1) && dbv1.type == DBVT_ASCIIZ) DBDefault = dbv1.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "Nick", &dbv2) && dbv2.type == DBVT_ASCIIZ) DBNick = dbv2.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "UWildcard", &dbv3) && dbv3.type == DBVT_ASCIIZ) DBWildcard = dbv3.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "UUser", &dbv4) && dbv4.type == DBVT_ASCIIZ) DBUser = dbv4.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "UHost", &dbv5) && dbv5.type == DBVT_ASCIIZ) DBHost = dbv5.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "User", &dbv6) && dbv6.type == DBVT_ASCIIZ) DBManUser = dbv6.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "Host", &dbv7) && dbv7.type == DBVT_ASCIIZ) DBManHost = dbv7.pszVal;
				if (DBWildcard)
					CharLower(DBWildcard);

				String nick;
				String user;
				String host;
				String away = GetWord(p1, 3);

				while (away != "") 
				{
					nick = GetWord(p1, 0);
					user = GetWord(p1, 1);
					host = GetWord(p1, 2);
					if ((DBWildcard && WCCmp(DBWildcard, (char *)nick.c_str()) || DBNick && !lstrcmpi(DBNick, nick.c_str()) || DBDefault && !lstrcmpi(DBDefault, nick.c_str()))
						&& (WCCmp(DBUser, (char *)user.c_str()) &&WCCmp(DBHost, (char *)host.c_str())))
					{
						if (away[0] == 'G' && DBGetContactSettingWord(hContact,IRCPROTONAME, "Status", ID_STATUS_OFFLINE) != ID_STATUS_AWAY)
							DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_AWAY);
						else if (away[0] == 'H' && DBGetContactSettingWord(hContact,IRCPROTONAME, "Status", ID_STATUS_OFFLINE) != ID_STATUS_ONLINE)
							DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_ONLINE);

						if ((DBNick && lstrcmpi(nick.c_str(), DBNick)) || !DBNick)
							DBWriteContactSettingString(hContact, IRCPROTONAME, "Nick", nick.c_str());
						if ((DBManUser && lstrcmpi(user.c_str(), DBManUser)) || !DBManUser)
							DBWriteContactSettingString(hContact, IRCPROTONAME, "User", user.c_str());
						if ((DBManHost &&lstrcmpi(host.c_str(), DBManHost)) || !DBManHost)
							DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", host.c_str());

						if (DBDefault)
							DBFreeVariant(&dbv1);
						if (DBNick)
							DBFreeVariant(&dbv2);
						if (DBWildcard)
							DBFreeVariant(&dbv3);
						if (DBUser)
							DBFreeVariant(&dbv4);
						if (DBHost)
							DBFreeVariant(&dbv5);
						if (DBManUser)
							DBFreeVariant(&dbv6);
						if (DBManHost)
							DBFreeVariant(&dbv7);

						delete []UserList;
						return true;

					}
					p1 = GetWordAddress(p1, 4);
					away = GetWord(p1, 3);
				}
				if (DBWildcard && DBNick && !WCCmp(CharLower(DBWildcard), CharLower(DBNick)))
				{
					DBWriteContactSettingString(hContact, IRCPROTONAME, "Nick", DBDefault);

					
					DoUserhostWithReason(2, ((String)"S" + DBWildcard).c_str(), true, DBWildcard);
							
					DBWriteContactSettingString(hContact, IRCPROTONAME, "User", "");
					DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", "");

					if (DBDefault)
						DBFreeVariant(&dbv1);
					if (DBNick)
						DBFreeVariant(&dbv2);
					if (DBWildcard)
						DBFreeVariant(&dbv3);
					if (DBUser)
						DBFreeVariant(&dbv4);
					if (DBHost)
						DBFreeVariant(&dbv5);
					if (DBManUser)
						DBFreeVariant(&dbv6);
					if (DBManHost)
						DBFreeVariant(&dbv7);
					
					delete [] UserList;
					return true;
				}
				if (DBGetContactSettingWord(hContact,IRCPROTONAME, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE)
				{
					DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE);
					DBWriteContactSettingString(hContact, IRCPROTONAME, "Nick", DBDefault);
					DBWriteContactSettingString(hContact, IRCPROTONAME, "User", "");
					DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", "");
				}

				if (DBDefault)
					DBFreeVariant(&dbv1);
				if (DBNick)
					DBFreeVariant(&dbv2);
				if (DBWildcard)
					DBFreeVariant(&dbv3);
				if (DBUser)
					DBFreeVariant(&dbv4);
				if (DBHost)
					DBFreeVariant(&dbv5);
				if (DBManUser)
					DBFreeVariant(&dbv6);
				if (DBManHost)
					DBFreeVariant(&dbv7);
			}
			delete []UserList;
		}
		
	}
	else
		ShowMessage(pmsg);
	return true;
}

bool CMyMonitor::OnIrc_WHO_REPLY(const CIrcMessage* pmsg)
{
	String command = PeekAtReasons(2);
	if(pmsg->m_bIncoming && pmsg->parameters.size() > 6 && command[0] == 'S') {
		WhoReply += pmsg->parameters[5] + (String)" " +pmsg->parameters[2] + (String)" " + pmsg->parameters[3] + (String)" " + pmsg->parameters[6] +  (String)" ";
		if(lstrcmpi(pmsg->parameters[5].c_str(), m_session.GetInfo().sNick.c_str()) ==0)
		{
			static char host[1024];
			lstrcpyn(host, pmsg->parameters[3].c_str(), 1024);
			IPRESOLVE * ipr = new IPRESOLVE;
			ipr->iType = IP_AUTO;
			ipr->pszAdr = host;
			mir_forkthread(ResolveIPThread, ipr);
		}
	}
	if (command[0] == 'U')
		ShowMessage(pmsg);
	return true;
}
bool CMyMonitor::OnIrc_TRYAGAIN(const CIrcMessage* pmsg)
{
	String command = "";
	if(pmsg->m_bIncoming && pmsg->parameters.size() > 1 ) 
	{
		if(pmsg->parameters[1] == "WHO")
		{
			command = GetNextUserhostReason(2);
		}
		if(pmsg->parameters[1] == "USERHOST")
		{
			command = GetNextUserhostReason(1);
		}
	}
	if (command[0] == 'U')
		ShowMessage(pmsg);
	return true;
}

bool CMyMonitor::OnIrc_USERHOST_REPLY(const CIrcMessage* pmsg)
{
	String command = "";
	if(pmsg->m_bIncoming)
	{
		// Get command
		command = GetNextUserhostReason(1);
		if(command != "" && command != "U" && pmsg->parameters.size() > 1)
		{
			CONTACT finduser = {NULL, NULL, NULL, false, false, false};
			char * params = NULL;
			char * next = NULL;
			char * p1 = NULL;
			char * p2 = NULL;
			char awaystatus = 0;
			String sTemp = "";
			String host = "";
			String user = "";
			String nick = "";
			String mask = "";
			String mess = "";
			String channel;
			int i;
			int j;

			// Status-check pre-processing: Setup check-list
			std::vector<String> checklist;
			if(command[0] == 'S') 
			{
				j = 0;
				sTemp = GetWord(command.c_str(), 0);
				sTemp.erase(0,1);
				while(sTemp != "")
				{
					checklist.push_back(sTemp);
					j++;
					sTemp = GetWord(command.c_str(), j);
				}

				/*
				params = new char[command.length()+1];
				lstrcpyn(params, command.c_str() + 1, command.length());
				for(p1 = GetWordAddress(params, 0); *p1; p1 = GetWordAddress(p1, 1)) {
					p2 = next = GetWordAddress(p1, 1);
					while(*(p2 - 1) == ' ') 
						p2--;
					*p2 = '\0';
					checklist.push_back(p1);
				}
				if(params) 
					delete [] params;
					*/
			}

			// Cycle through results
//			params = new char[pmsg->parameters[1].length()+2];
//			lstrcpyn(params, pmsg->parameters[1].c_str(), pmsg->parameters[1].length()+1);
//			for(p1 = GetWordAddress(params, 0); *p1; p1 = next)
			j = 0;
			sTemp = GetWord(pmsg->parameters[1].c_str(), j);
			while (sTemp != "")
			{
//				p2 = next = GetWordAddress(p1, 1);
//				while(*(p2 - 1) == ' ') p2--;
//				*p2 = '\0';
				p1 = new char[sTemp.length()+1];
				lstrcpyn(p1, sTemp.c_str(), sTemp.length()+1);
				p2 = p1;

				// Pull out host, user and nick
				p2 = strchr(p1, '@');
				if (p2)
				{
					*p2 = '\0';
					p2++;
					host = p2;
				}
				p2 = strchr(p1, '=');
				if (p2)
				{
					*p2 = '\0';
					p2++;
					awaystatus = *p2;
					p2++;
					user = p2;
					nick = p1;
				}
				mess = "";
				mask = nick + "!" + user + "@" + host;
				if(host == "" || user == "" || nick == "")
				{
					if(p1)
						delete[] p1;
					continue;
				}

				// Do command
				switch (command[0])
				{
				case 'S':	// Status check
					HANDLE hContact;
					finduser.name = (char *)nick.c_str();
					finduser.host = (char *)host.c_str();
					finduser.user = (char *)user.c_str();

					hContact = CList_FindContact(&finduser);

					if(hContact && DBGetContactSettingByte(hContact, IRCPROTONAME, "AdvancedMode", 0) == 0)
					{
						DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", awaystatus == '-'? ID_STATUS_AWAY : ID_STATUS_ONLINE);

						DBWriteContactSettingString(hContact, IRCPROTONAME, "User", user.c_str());
						DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", host.c_str());

							// If user found, remove from checklist
						for(i = 0; i < (int)checklist.size(); i++)
							if(checklist[i] == nick)
								checklist.erase(checklist.begin() + i);
					}
					break;

				case 'I':	// Ignore
					mess = "/IGNORE %question=\"";
					mess += Translate("Please enter the hostmask (nick!user@host)\nNOTE! Contacts on your contact list are never ignored");
					mess += (String)"\",\"" + Translate("Ignore") + "\",\"*!*@" + host + "\"";
					if(prefs->IgnoreChannelDefault)
						mess += " +qnidcm";
					else
						mess += " +qnidc";
					break;

				case 'J':	// Unignore
					mess = "/UNIGNORE *!*@" + host;
					break;

				case 'B':	// Ban
					channel = (command.c_str() + 1);
					mess = "/MODE " + channel + " +b *!*@" + host;
					break;

				case 'K':	// Ban & Kick
					channel = (command.c_str() + 1);
					mess = "/MODE " + channel + " +b *!*@" + host + "%newl/KICK "+ channel + " " + nick;
					break;

				case 'L':	// Ban & Kick with reason
					channel = (command.c_str() + 1);
					mess = "/MODE " + channel + " +b *!*@" + host + "%newl/KICK "+ channel + " " + nick + (String)" %question=\"";
					mess += (String)Translate("Please enter the reason") + "\",\"" + Translate("Ban'n Kick") + "\",\"" + Translate("Jerk") + "\"";
					break;
				}
				
				if(p1)
					delete [] p1;

				// Post message
				if (mess != "")
					PostIrcMessageWnd(NULL, NULL,(char *)mess.c_str());
				j++;
				sTemp = GetWord(pmsg->parameters[1].c_str(), j);
			}
			
			// Cleanup and exit
//			if(params) delete [] params;

			// Status-check post-processing: make buddies in ckeck-list offline
			if(command[0] == 'S')
			{
				for(i = 0; i < (int)checklist.size(); i++) {
					finduser.name = (char *)checklist[i].c_str();
					finduser.ExactNick = true;
					CList_SetOffline(&finduser);
				}
			}
			return true;
		}
	}
	if(!pmsg->m_bIncoming || command =="U")
		ShowMessage(pmsg);
	return true;

}
bool CMyMonitor::OnIrc_SUPPORT(const CIrcMessage* pmsg)
{
	static const char* lpszFmt = "Try server %99[^ ,], port %19s";
	char szAltServer[100];
	char szAltPort[20];
	if( pmsg->parameters.size() > 1 && sscanf(pmsg->parameters[1].c_str(), lpszFmt, &szAltServer, &szAltPort) == 2 )
	{
		ShowMessage(pmsg);
		lstrcpyn(prefs->ServerName, szAltServer, 101);
		lstrcpyn(prefs->PortStart, szAltPort, 9);

		NoOfChannels = 0;
		ConnectToServer();
		return true;
	}

	if (pmsg->m_bIncoming && !bPerformDone) {
		DoOnConnect(pmsg);
	}

	
	if (pmsg->m_bIncoming && pmsg->parameters.size() >0)
	{
		String S;
		for( int i = 0; i < (int)pmsg->parameters.size();i++)
		{
			char * temp = new char[lstrlen(pmsg->parameters[i].c_str())+1];
			strcpy(temp, pmsg->parameters[i].c_str());
			if (strstr(temp, "CHANTYPES="))
			{
				char * p1 = strchr(temp, '=');
				p1++;
				if ( lstrlen(p1) > 0)
					sChannelPrefixes = p1;
			}
			if (strstr(temp, "CHANMODES="))
			{
				char * p1 = strchr(temp, '=');
				p1++;
				if ( lstrlen(p1) > 0)
					sChannelModes= p1;
			}
			if (strstr(temp, "PREFIX="))
			{
				char * p1 = strchr(temp, '(');
				char * p2 = strchr(temp, ')');
				if (p1 && p2)
				{
					p1++;
					if (p1 != p2)
						sUserModes = p1;
						sUserModes = sUserModes.substr(0, p2-p1);
					p2++;
					if (*p2 != '\0')
						sUserModePrefixes = p2;
				}
				else {
					p1= strchr(temp, '=');
					p1++;
					sUserModePrefixes= p1;
					for (int i =0; i < (int)sUserModePrefixes.length()+1; i++){
						if (sUserModePrefixes[i]	     == '@')
							sUserModes[i] = 'o';
						else if (sUserModePrefixes[i] == '+')
							sUserModes[i] = 'v';
						else if (sUserModePrefixes[i] == '-')
							sUserModes[i] = 'u';
						else if (sUserModePrefixes[i] == '%')
							sUserModes[i] = 'h';
						else if (sUserModePrefixes[i] == '!')
							sUserModes[i] = 'a';
						else if (sUserModePrefixes[i] == '*')
							sUserModes[i] = 'q';
						else if (sUserModePrefixes[i] == '\0')
							sUserModes[i] = '\0';
						else  
							sUserModes[i] = '_';
					}
				}


	
			}

			delete[]temp;
		}

	}

	ShowMessage(pmsg);
	return true;
}
void CMyMonitor::OnIrcDefault(const CIrcMessage* pmsg)
{
	CIrcDefaultMonitor::OnIrcDefault(pmsg);
	ShowMessage(pmsg);
	
}

void CMyMonitor::OnIrcDisconnected()
{
	CIrcDefaultMonitor::OnIrcDisconnected();
	StatusMessage = "";
	DBDeleteContactSetting(NULL, IRCPROTONAME, "JTemp");
	bTempDisableCheck = false;
	bTempForceCheck = false;
	iTempCheckTime = 0;

	prefs->MyHost[0] = '\0';

	int Temp = OldStatus;
	KillChatTimer(OnlineNotifTimer);
	KillChatTimer(OnlineNotifTimer3);
	KillChatTimer(KeepAliveTimer);
	KillChatTimer(InitTimer);
	KillChatTimer(IdentTimer);
	OldStatus = ID_STATUS_OFFLINE;
	ProtoBroadcastAck(IRCPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp, ID_STATUS_OFFLINE);
	DoEvent(GC_EVENT_CHANGESESSIONAME, "Network log", NULL, Translate("Offline"), NULL, NULL, NULL, FALSE, TRUE); 

	String sDisconn = "\0035\002";
	sDisconn += Translate("*Disconnected*");
	DoEvent(GC_EVENT_INFORMATION, "Network log", NULL, sDisconn.c_str(), NULL, NULL, NULL, true, false); 

	{
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		gce.cbSize = sizeof(GCEVENT);
		gcd.pszID = NULL;
		gcd.pszModule = IRCPROTONAME;
		gcd.iType = GC_EVENT_CONTROL;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_OFFLINE, (LPARAM)&gce);
	}

	if(!Miranda_Terminated())
		CList_SetAllOffline(prefs->DisconnectDCCChats);
	DBWriteContactSettingString(NULL,IRCPROTONAME,"Nick",prefs->Nick);
	
	CLISTMENUITEM clmi;
	memset( &clmi, 0, sizeof( clmi ));
	clmi.cbSize = sizeof( clmi );
	clmi.flags = CMIM_FLAGS | CMIF_GRAYED;
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuJoin, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuList, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuNick, ( LPARAM )&clmi );
}

bool DoOnConnect(const CIrcMessage *pmsg)
{
	bPerformDone= true;
	nickflag = true;	

 	CLISTMENUITEM clmi;
	memset( &clmi, 0, sizeof( clmi ));
	clmi.cbSize = sizeof( clmi );
	clmi.flags = CMIM_FLAGS;
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuJoin, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuList, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuNick, ( LPARAM )&clmi );

	int Temp = OldStatus;
	OldStatus = ID_STATUS_ONLINE;
	ProtoBroadcastAck(IRCPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp,ID_STATUS_ONLINE);

	if (StatusMessage != "")
	{
		String S = "/AWAY ";
		S = S + StatusMessage;
		S = ReplaceString( (char *)S.c_str(), "\r\n", " ");
		PostIrcMessage((char *)S.c_str());
	}
	if(prefs->Perform)
	{
		DoPerform("ALL NETWORKS");
		if (g_ircSession)
			DoPerform((char*)g_ircSession.GetInfo().sNetwork.c_str());
	}

	if(prefs->RejoinChannels)
	{
		int count = (int)CallService(MS_GC_GETSESSIONCOUNT, 0, (LPARAM)IRCPROTONAME);

		for (int i = 0; i < count ; i++)
		{
			GC_INFO gci = {0};
			gci.Flags = BYINDEX|DATA|NAME|TYPE;
			gci.iItem = i;
			gci.pszModule = IRCPROTONAME;
			if(!CallService(MS_GC_GETINFO, 0, (LPARAM)&gci) && gci.iType == GCW_CHATROOM)
			{
				CHANNELINFO* wi = (CHANNELINFO *)gci.dwItemData;
				if (wi && wi->pszPassword)
					PostIrcMessage("/JOIN %s %s", gci.pszName, wi->pszPassword);
				else
					PostIrcMessage("/JOIN %s", gci.pszName);
			}
		}
	}

	DoEvent(GC_EVENT_CHANGESESSIONAME, "Network log", NULL, g_ircSession.GetInfo().sNetwork.c_str(), NULL, NULL, NULL, FALSE, TRUE); 
	DoEvent(GC_EVENT_ADDGROUP, "Network log", NULL, NULL, "Normal", NULL, NULL, FALSE, TRUE); 
	{
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		gce.cbSize = sizeof(GCEVENT);
		gcd.pszID = "Network log";
		gcd.pszModule = IRCPROTONAME;
		gcd.iType = GC_EVENT_CONTROL;
		gce.pDest = &gcd;
		CallChatEvent( SESSION_ONLINE, (LPARAM)&gce);

	}
	SetChatTimer(InitTimer, 1*1000, TimerProc);
	if(prefs->IdentTimer)
		SetChatTimer(IdentTimer, 60*1000, IdentTimerProc);
	if (prefs->SendKeepAlive)
		SetChatTimer(KeepAliveTimer, 60*1000, KeepAliveTimerProc);
	if (prefs->AutoOnlineNotification && !bTempDisableCheck || bTempForceCheck)
	{
		SetChatTimer(OnlineNotifTimer, 1000, OnlineNotifTimerProc);
		if(prefs->ChannelAwayNotification)
			SetChatTimer(OnlineNotifTimer3, 3000, OnlineNotifTimerProc3);
	}
	return 0;

}
static void __cdecl AwayWarningThread(LPVOID di)
{
	MessageBox(NULL, Translate("The usage of /AWAY in your perform buffer is restricted\n as IRC sends this command automatically."), Translate("IRC Error"), MB_OK);
}
int DoPerform(char * event)
{
	if (!pszPerformFile)
		return 0;
	char * search = new char[lstrlen (event)+14];
	mir_snprintf(search, lstrlen (event)+13, "NETWORK: %s", event);
	char * p1 = my_strstri(pszPerformFile, search);
	if (p1 != NULL)
	{
		char * p2 = strchr(p1, '\n');
		if(!p2)
			return 0;
		p2 ++;
		p1 = strstr(p2, "\nNETWORK: ");
		if(!p1)
			p1 = strchr(p2, '\0');
		else
			p1--;
		while(p1 > p2 && ( p1[-1] == ' ' || p1[-1] == '\n' || p1[-1] == '\r' || p1[-1] == '\0'))
			p1--;
		if(p2 >= p1)
			return 0;
		char * DoThis = new char[p1-p2+1];
		lstrcpyn(DoThis, p2, p1-p2+1);

		if(!my_strstri(DoThis, "/away"))
			PostIrcMessageWnd(NULL, NULL, (char *)DoThis);
		else
			mir_forkthread(AwayWarningThread, NULL  );
		delete [] DoThis;
		delete [] search;
		return 1;
	}
	delete [] search;
	return 0;
}



char * IsIgnored(String nick, String address, String host, char type) 
{ 
	String user = nick + "!" + address + "@" + host;
	return IsIgnored(user, type);
}

char * IsIgnored(String user, char type) 
{ 
	if (pszIgnoreFile)
	{
		char * p1 = pszIgnoreFile;
		char * p2 = p1;
		char * pTemp = NULL;
		while (*p1 != '\0')
		{
			while(*p1 == '\r' || *p1 == '\n')
				p1++;
			if (*p1 == '\0')
				return 0;
			p2 = strstr(p1, "\r\n");
			if (!p2)
				p2 = strchr(p1, '\0');
			pTemp = p2;
			while (pTemp > p1 && (*pTemp == '\r' || *pTemp == '\n' ||*pTemp == '\0' || *pTemp == ' '))
				pTemp--;
			pTemp++;

			char * p3= new char[pTemp-p1+1];
			lstrcpyn(p3, p1, pTemp-p1+1);

			if(type == '\0')
			{	
				if(!lstrcmpi(user.c_str(),(char *)GetWord(p3, 0).c_str()))
				{
					delete [] p3;
					return p1;
				}
			}
			bool bUserContainsWild = strchr(user.c_str(), '*') == NULL ?false:true || strchr(user.c_str(), '?') == NULL?false:true;
			
			if( !bUserContainsWild && WCCmp((char *)GetWord(p3, 0).c_str(), (char *)user.c_str())  
				|| bUserContainsWild && !lstrcmpi(user.c_str(),(char *)GetWord(p3, 0).c_str()))
			{
				if(GetWord(p3, 1) == "" || GetWord(p3, 1)[0] != '+')
				{
					goto IGNORELABEL;
				}
				if(!strchr(GetWord(p3, 1).c_str(), type))
				{
					goto IGNORELABEL;
				}
				if(GetWord(p3, 2) == "")
				{
					delete [] p3;
					return p1;
				}
				if (g_ircSession && !lstrcmpi(GetWordAddress(p3, 2), g_ircSession.GetInfo().sNetwork.c_str()))
				{
					delete [] p3;
					return p1;
				}


			}
IGNORELABEL:
			delete [] p3;
			p1 = p2;

		}

	}
	return NULL; 
}

bool AddIgnore(String mask, String mode, String network) 
{ 
	RemoveIgnore(mask);
	String S = mask + " +" + mode + " " + network + "\r\n";
	if (pszIgnoreFile)
		S += pszIgnoreFile;

	char filepath[MAX_PATH];
	mir_snprintf(filepath, sizeof(filepath), "%s\\%s_ignore.ini", mirandapath, IRCPROTONAME);
	FILE *hFile = fopen(filepath,"wb");
	if (hFile)
	{
		fputs(S.c_str(), hFile);
		fclose(hFile);

	}
	if (pszIgnoreFile)
		delete [] pszIgnoreFile;
	pszIgnoreFile = IrcLoadFile(filepath);
	if(IgnoreWndHwnd)
		SendMessage(IgnoreWndHwnd, IRC_REBUILDIGNORELIST, 0, 0);
	return true;
}  

bool RemoveIgnore(String mask) 
{ 
	char * p1 = IsIgnored(mask, '\0');
	if (!p1)
		return false;
	while (p1)
	{
		char * p2 = strstr(p1, "\r\n");
		if (!p2)
			p2 = strchr(p1, '\0');
		else
			p2 +=2;
		
		for (int i=0;;i++)
		{
			p1[i] = p2[i];
			if (p1[i] == '\0')
				break;
		}

		char filepath[MAX_PATH];
		mir_snprintf(filepath, sizeof(filepath), "%s\\%s_ignore.ini", mirandapath, IRCPROTONAME);
		FILE *hFile = fopen(filepath,"wb");
		if (hFile)
		{
			fputs(pszIgnoreFile, hFile);
			fclose(hFile);

		}
		if (pszIgnoreFile)
			delete [] pszIgnoreFile;
		pszIgnoreFile = IrcLoadFile(filepath);
		p1 = IsIgnored(mask, '\0');

	}
	if(IgnoreWndHwnd)
		SendMessage(IgnoreWndHwnd, IRC_REBUILDIGNORELIST, 0, 0);
	return true; 
} 




