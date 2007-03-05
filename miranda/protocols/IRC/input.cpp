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

#include "irc.h"
extern PREFERENCES *	prefs;
extern String			StatusMessage;
extern char *			IRCPROTONAME;
extern PLUGININFOEX		pluginInfo;
extern HWND				list_hWnd;
extern int				NoOfChannels;
extern DWORD			mirVersion;
bool					bEcho = true;
extern int				ManualWhoisCount ;
bool					bTempDisableCheck = false;
bool					bTempForceCheck = false;
extern int				iTempCheckTime ;
extern UINT_PTR			OnlineNotifTimer;
extern UINT_PTR			OnlineNotifTimer3;
extern HWND				manager_hWnd;

static String FormatMsg(String text)
{
	char temp[30];
	lstrcpyn(temp, GetWord(text.c_str(), 0).c_str(), 29);
	CharLower(temp);
	String command =temp;
	String S = "";
	if (command == "/quit" || command == "/away") 
		S =GetWord(text.c_str(), 0) + (String) " :" + GetWordAddress(text.c_str(), 1);
	else if (command == "/privmsg" || command == "/part" || command == "/topic" || command == "/notice") {
		S =GetWord(text.c_str(), 0) + (String)" " + GetWord(text.c_str(), 1) ;
		if (!GetWord(text.c_str(), 2).empty())
			S += (String) " :" + (String)(GetWordAddress(text.c_str(), 1) +1 + lstrlen (GetWord(text.c_str(), 1).c_str()));
	}
	else if (command == "/kick")
		S =GetWord(text.c_str(), 0) + (String)" " + GetWord(text.c_str(), 1) + (String)" " + GetWord(text.c_str(), 2)+ (String) " :" + GetWordAddress(text.c_str(), 3);
	else 
		S = GetWordAddress(text.c_str(), 0);
	S.erase(0,1);
	return S;
}

static String AddCR(String text)
{

	text = ReplaceString(text, "\n", "\r\n");
	text = ReplaceString(text, "\r\r", "\r");

	return text;
}

static String DoAlias(char *text, char *window)
{
	String Messageout = "";
	char * p1 = text;
	char * p2 = text;
	bool LinebreakFlag = false;
	p2 = strstr(p1, "\r\n");
	if (!p2)
		p2 = strchr(p1, '\0');
	if (p1 == p2)
		return (String)text;
	do {
		if (LinebreakFlag )
			Messageout += "\r\n";
		char * line = new char[p2-p1 +1]; 
		lstrcpyn(line, p1, p2-p1+1);
		char *test = line;
		while (*test == ' ')
			test++;
		if ( *test == '/')
		{
			lstrcpyn(line, GetWordAddress(line, 0), p2-p1+1);
			String S = line ;
			delete [] line;
			line = new char[S.length() +2]; 
			lstrcpyn(line, S.c_str(), S.length()+1);
			char * p3 = strstr(prefs->Alias, (GetWord(line, 0)+ " ").c_str());
			if( p3 != prefs->Alias) {
				p3 = strstr(prefs->Alias, ((String)"\r\n" + GetWord(line, 0)+ " ").c_str());
				if (p3) p3 +=2;
			}
			if (p3 != NULL) {
				char * p4 = strstr(p3, "\r\n");
				if (!p4)
					p4 = strchr(p3, '\0');
				char * alias = new char[p4-p3 +1] ;
				lstrcpyn(alias, p3, p4-p3+1);
				String S = ReplaceString(alias, "##", window);
				S = ReplaceString(S.c_str(), "$?", "%question");

				for (int index = 1; index <8; index++){
					char str[5];
					mir_snprintf(str, sizeof(str), "#$%u", index);
					if (!GetWord(line, index).empty() && IsChannel(GetWord(line, index)))
						S = ReplaceString(S, str, (char *)GetWord(line, index).c_str());
					else
						S = ReplaceString(S, str, (char *)((String)"#" +GetWord(line, index)).c_str());
				}
				for (int index2 = 1; index2 <8; index2++){
					char str[5];
					mir_snprintf(str, sizeof(str), "$%u-", index2);
					S = ReplaceString(S, str, GetWordAddress(line, index2));
				}
				for (int index3 = 1; index3 <8; index3++){
					char str[5];
					mir_snprintf(str, sizeof(str), "$%u", index3);
					S = ReplaceString(S, str, (char *)GetWord(line, index3).c_str());
				}
				Messageout += GetWordAddress((char *)S.c_str(), 1);
				delete []alias;
			} else {
				Messageout += line;
			}
		} else {
			Messageout += line;
		}
		p1 = p2;
		if (*p1 == '\r')
			p1 += 2;
		p2 = strstr(p1, "\r\n");
		if (!p2)
			p2 = strchr(p1, '\0');
		delete [] line;
		LinebreakFlag = true;
	} while ( *p1 != '\0');
	return Messageout;
}


static String DoIdentifiers (String text, const char * window)
{
	SYSTEMTIME		time;
	char			str[800];
	int				i = 0;

	GetLocalTime(&time);
	text = ReplaceString(text, "%mnick", prefs->Nick);
	text = ReplaceString(text, "%anick", prefs->AlternativeNick);
	text = ReplaceString(text, "%awaymsg", (char *)StatusMessage.c_str());
	text = ReplaceString(text, "%module", IRCPROTONAME);
	text = ReplaceString(text, "%name", prefs->Name);
	text = ReplaceString(text, "%newl", "\r\n");
	text = ReplaceString(text, "%network", (char*)g_ircSession.GetInfo().sNetwork.c_str());
	text = ReplaceString(text, "%me", (char *)g_ircSession.GetInfo().sNick.c_str());
   
    mir_snprintf(str,511,"%d.%d.%d.%d",(mirVersion>>24)&0xFF,(mirVersion>>16)&0xFF,(mirVersion>>8)&0xFF,mirVersion&0xFF);
	text = ReplaceString(text, "%mirver", str);

    mir_snprintf(str,511,"%d.%d.%d.%d",(pluginInfo.version>>24)&0xFF,(pluginInfo.version>>16)&0xFF,(pluginInfo.version>>8)&0xFF,pluginInfo.version&0xFF);
	text = ReplaceString(text, "%version", str);
	str[0] = (char)3; str[1] = '\0'	;
	text = ReplaceString(text, "%color", str);
	str[0] = (char)2;
	text = ReplaceString(text, "%bold", str);
	str[0] = (char)31;
	text = ReplaceString(text, "%underline", str);
	str[0] = (char)22;
	text = ReplaceString(text, "%italics", str);


	return text;
}

static BOOL DoHardcodedCommand(String text, char * window, HANDLE hContact)
{

	char temp[30];
	lstrcpyn(temp, GetWord(text.c_str(), 0).c_str(), 29);
	CharLower(temp);
	String command =temp;
	String one =GetWord(text.c_str(), 1);
	String two =GetWord(text.c_str(), 2);
	String three =GetWord(text.c_str(), 3);
	String therest =GetWordAddress(text.c_str(), 4);

	if (command == "/servershow" || command == "/serverhide")
	{
		if(prefs->UseServer)
		{
			GCEVENT gce = {0}; 
			GCDEST gcd = {0};
			gcd.iType = GC_EVENT_CONTROL;
			gcd.pszID = "Network log";
			gcd.pszModule = IRCPROTONAME;
			gce.cbSize = sizeof(GCEVENT);
			gce.pDest = &gcd;
			CallChatEvent( command == "/servershow"?WINDOW_VISIBLE:WINDOW_HIDDEN, (LPARAM)&gce);
		}
		return true;
	}

	else if (command == "/clear" )
	{
		GCEVENT gce = {0}; 
		GCDEST gcd = {0};
		String S;
		if (one != "")
		{
			if(one == "server")
				S = "Network log";
			else
				S = MakeWndID(one);
		}
		else if(lstrcmpi(window, "Network log") == 0)
			S = window;
		else
			S = MakeWndID(window);
		gce.cbSize = sizeof(GCEVENT);
		gcd.iType = GC_EVENT_CONTROL;
		gcd.pszModule = IRCPROTONAME;
		gce.pDest = &gcd;
		gcd.pszID = (char *)S.c_str();
		CallChatEvent( WINDOW_CLEARLOG, (LPARAM)&gce);

		return true;
	}
	else if (command == "/ignore" )
	{
		if (g_ircSession)
		{
			String IgnoreFlags = "";
			char temp[500];
			if (one == "")
			{
				if (prefs->Ignore)
					DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate(	"Ignore system is enabled"	), NULL, NULL, NULL, true, false); 
				else
					DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate(	"Ignore system is disabled"	), NULL, NULL, NULL, true, false); 
				return true;
			}
			if (!lstrcmpi(one.c_str(),"on"))
			{
				prefs->Ignore = 1;
				DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate(	"Ignore system is enabled"	), NULL, NULL, NULL, true, false); 
				return true;
			}
			if (!lstrcmpi(one.c_str(), "off"))
			{
				prefs->Ignore = 0;
				DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate(	"Ignore system is disabled"	), NULL, NULL, NULL, true, false); 
				return true;
			}
			if (!strchr(one.c_str(), '!') && !strchr(one.c_str(), '@'))
				one += "!*@*";
			if(two != "" && two[0] == '+')
			{
				if (strchr(two.c_str(), 'q'))
					IgnoreFlags += "q";
				if (strchr(two.c_str(), 'n'))
					IgnoreFlags += "n";
				if (strchr(two.c_str(), 'i'))
					IgnoreFlags += "i";
				if (strchr(two.c_str(), 'd'))
					IgnoreFlags += "d";
				if (strchr(two.c_str(), 'c'))
					IgnoreFlags += "c";
				if (strchr(two.c_str(), 'm'))
					IgnoreFlags += "m";
			}
			else
				IgnoreFlags = "qnidc";

			String Network;
			if(three == "")
				Network = g_ircSession.GetInfo().sNetwork;
			else
				Network = three;

			AddIgnore(one, IgnoreFlags.c_str(), Network);
			mir_snprintf(temp, 499, Translate(	"%s on %s is now ignored (+%s)"	), one.c_str(), Network.c_str(), IgnoreFlags.c_str());
			DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
		}
		return true;
	}
	else if (command == "/unignore" )
	{
		if (!strchr(one.c_str(), '!') && !strchr(one.c_str(), '@'))
			one += "!*@*";
		char temp[500];
		if (RemoveIgnore(one))
			mir_snprintf(temp, 499, Translate(	"%s is not ignored now"	), one.c_str());
		else
			mir_snprintf(temp, 499, Translate(	"%s was not ignored"	), one.c_str());
		DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
		return true;
	}

	else if(command == "/userhost")
	{
 		if (one.empty())
			return true;

		DoUserhostWithReason(1, "U", false, temp);
 		return false;
 	}
 
	else if (command == "/joinx")
	{
		if (one.empty())
			return true;

		AddToJTemp("X"+one);

		PostIrcMessage( "/JOIN %s", GetWordAddress(text.c_str(), 1));
		return true;
	}

	else if (command == "/joinm")
	{
		if (one.empty())
			return true;

		AddToJTemp("M"+one);

		PostIrcMessage( "/JOIN %s", GetWordAddress(text.c_str(), 1));
		return true;
	}
	else if (command == "/nusers")
	{
		char szTemp[40];
		String S = MakeWndID(window);
			GC_INFO gci = {0};
			gci.Flags = BYID|NAME|COUNT;
			gci.pszModule = IRCPROTONAME;
			gci.pszID = (char *)S.c_str();
			if(!CallService(MS_GC_GETINFO, 0, (LPARAM)&gci))
			{
				_snprintf(szTemp, 35, "users: %u", gci.iCount);
			}

		DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
		return true;
	}
	else if (command == "/echo")
	{
		if (one.empty())
			return true;

		if (!lstrcmpi(one.c_str(), "on"))
		{
			bEcho = TRUE;
			DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate("Outgoing commands are shown"), NULL, NULL, NULL, true, false); 
		}
		
		if (!lstrcmpi(one.c_str(), "off"))
		{
			DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate("Outgoing commands are not shown"), NULL, NULL, NULL, true, false); 
			bEcho = FALSE;
		}

		return true;
	}
	
	else if (command == "/buddycheck")
	{
		if (one.empty())
		{
			if ((prefs->AutoOnlineNotification && !bTempDisableCheck) || bTempForceCheck)
				DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate(	"The buddy check function is enabled" ), NULL, NULL, NULL, true, false); 
			else
				DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate(	"The buddy check function is disabled" ), NULL, NULL, NULL, true, false); 
			return true;
		}
		if (!lstrcmpi(one.c_str(), "on"))
		{
			bTempForceCheck = true;
			bTempDisableCheck = false;
			DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate(	"The buddy check function is enabled" ), NULL, NULL, NULL, true, false); 
			SetChatTimer(OnlineNotifTimer, 500, OnlineNotifTimerProc);
			if(prefs->ChannelAwayNotification)
				SetChatTimer(OnlineNotifTimer3, 1500, OnlineNotifTimerProc3);
		}
		if (!lstrcmpi(one.c_str(), "off"))
		{
			bTempForceCheck = false;
			bTempDisableCheck = true;
			DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate(	"The buddy check function is disabled" ), NULL, NULL, NULL, true, false); 
			KillChatTimer(OnlineNotifTimer);
			KillChatTimer(OnlineNotifTimer3);
		}
		if (!lstrcmpi(one.c_str(), "time") && !two.empty())
		{
			iTempCheckTime = StrToInt(two.c_str());
			if (iTempCheckTime < 10 && iTempCheckTime != 0)
				iTempCheckTime = 10;

			if (iTempCheckTime == 0)
				DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), Translate(	"The time interval for the buddy check function is now at default setting" ), NULL, NULL, NULL, true, false); 
			else
			{
				char temp[200];
				mir_snprintf(temp, 199, Translate(	"The time interval for the buddy check function is now %u seconds" ), iTempCheckTime);
				DoEvent(GC_EVENT_INFORMATION, NULL, g_ircSession.GetInfo().sNick.c_str(), temp, NULL, NULL, NULL, true, false); 
			}
		}
		return true;

	}
	else if (command == "/whois")
	{
		if (one.empty())
			return false;
		ManualWhoisCount++;
		return false;
	}

	else if (command == "/channelmanager")
	{
		if (window && !hContact && IsChannel(window))
		{
			if (g_ircSession)
			{
				if(manager_hWnd != NULL)
				{
					SetActiveWindow(manager_hWnd);
					SendMessage(manager_hWnd, WM_CLOSE, 0, 0);
				}
				if(manager_hWnd == NULL)
				{
					manager_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_CHANMANAGER),NULL,ManagerWndProc);
					SetWindowText(manager_hWnd, Translate("Channel Manager"));
					SetWindowText(GetDlgItem(manager_hWnd, IDC_CAPTION), window);
					SendMessage(manager_hWnd, IRC_INITMANAGER, 1, (LPARAM)window);
				}
			}
		}
		return true;
	}

	else if (command == "/who")
	{
		if (one.empty())
			return true;
		DoUserhostWithReason(2, "U", false, "%s", one.c_str());
		return false;
	}

	else if (command == "/hop")
	{
		if (!IsChannel(window))
			return true;

		PostIrcMessage( "/PART %s", window);

		if ((one.empty() || !IsChannel(one))) 
		{

			CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if (wi && wi->pszPassword)
				PostIrcMessage( "/JOIN %s %s", window, wi->pszPassword);
			else
				PostIrcMessage( "/JOIN %s", window);

			return true;
		}
		GCEVENT gce = {0}; 
		GCDEST gcd = {0};
		gcd.iType = GC_EVENT_CONTROL;
		String S = MakeWndID(window);
		gcd.pszID = (char*) S.c_str();
		gcd.pszModule = IRCPROTONAME;
		gce.cbSize = sizeof(GCEVENT);
		gce.pDest = &gcd;

		CallChatEvent( SESSION_TERMINATE, (LPARAM)&gce);

		PostIrcMessage( "/JOIN %s", GetWordAddress(text.c_str(), 1));

		return true;
	}
	
	else if (command == "/list")
	{
		if (list_hWnd == NULL)
			list_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_LIST),NULL,ListWndProc);
		SetActiveWindow(list_hWnd);
		int minutes = (int)NoOfChannels/4000;
		int minutes2 = (int)NoOfChannels/9000;
		char text[250];
		mir_snprintf(text, 249, Translate(	"This command is not recommended on a network of this size!\r\nIt will probably cause high CPU usage and/or high bandwidth\r\nusage for around %u to %u minute(s).\r\n\r\nDo you want to continue?"	), minutes2, minutes);
		if (NoOfChannels < 4000 || (NoOfChannels >= 4000 && MessageBox(NULL, text, Translate("IRC warning") , MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2) == IDYES)) {
			ListView_DeleteAllItems	(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW));
			PostIrcMessage( "/lusers");
			return false;
		}
		SetDlgItemText(list_hWnd, IDC_TEXT, Translate(	"Aborted"	)); 
		return true;
	}
	
	else if (command == "/me")
	{
		if (one.empty())
			return true;
		char szTemp[4000];
		mir_snprintf(szTemp, sizeof(szTemp), "\1ACTION %s\1", GetWordAddress(text.c_str(), 1));
		PostIrcMessageWnd(window, hContact, szTemp);
		return true;
	}

	else if (command == "/ame")
	{
		if (one.empty())
			return true;

		String S ;
		S = "/ME " + DoIdentifiers((String)GetWordAddress(text.c_str(), 1), window);
		S = ReplaceString(S, "%", "%%");
		DoEvent(GC_EVENT_SENDMESSAGE, NULL, NULL, S.c_str(), NULL, NULL, NULL, FALSE, FALSE);
		return true;
	}
	
	else if (command == "/amsg")
	{
		if (one.empty())
			return true;
		String S ;
		S = DoIdentifiers((String)GetWordAddress(text.c_str(), 1), window);
		S = ReplaceString(S, "%", "%%");
		DoEvent(GC_EVENT_SENDMESSAGE, NULL, NULL, S.c_str(), NULL, NULL, NULL, FALSE, FALSE);
		return true;
	}

	else if (command == "/msg")
	{
		if (one.empty() || two.empty())
			return true;

		char szTemp[4000];
		mir_snprintf(szTemp, sizeof(szTemp), "/PRIVMSG %s", GetWordAddress(text.c_str(), 1));

		PostIrcMessageWnd(window, hContact, szTemp);
		return true;
	}

	else if (command == "/query")
	{
		if (one.empty() || IsChannel(one.c_str()))
			return true;

		struct CONTACT_TYPE user ={(char *)one.c_str(), NULL, NULL, false, false, false};
		HANDLE hContact2 = CList_AddContact(&user, false, false);

		if (hContact2){

			DBVARIANT dbv1;
			
			if(DBGetContactSettingByte(hContact, IRCPROTONAME, "AdvancedMode", 0) == 0)
				DoUserhostWithReason(1, ("S" + one).c_str(), true, one.c_str());
			else
			{
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "UWildcard", &dbv1) && dbv1.type == DBVT_ASCIIZ)
				{
					DoUserhostWithReason(2, ((String)"S" + dbv1.pszVal).c_str(), true, dbv1.pszVal);
					DBFreeVariant(&dbv1);
				}
				else
				{
					DoUserhostWithReason(2, ((String)"S" + one).c_str(), true, one.c_str());
				}
			}
			
			CallService(MS_MSG_SENDMESSAGE,(WPARAM)hContact2,0);

		}

		if (!two.empty()) 
		{
			char szTemp[4000];
			mir_snprintf(szTemp, sizeof(szTemp), "/PRIVMSG %s", GetWordAddress(text.c_str(), 1));
			PostIrcMessageWnd(window, hContact, szTemp);
		}
		return true;
	}	
	else if (command == "/ctcp")
	{
		if (one.empty() || two.empty())
			return true;

		char szTemp[1000];

		unsigned long ulAdr = 0;
		if (prefs->ManualHost)
			ulAdr = ConvertIPToInteger(prefs->MySpecifiedHostIP);
		else
			ulAdr = ConvertIPToInteger(prefs->IPFromServer?prefs->MyHost:prefs->MyLocalHost);

		if(lstrcmpi(two.c_str(), "dcc") != 0 || ulAdr) // if it is not dcc or if it is dcc and a local ip exist
		{
			if(lstrcmpi(two.c_str(), "ping") == 0)
				mir_snprintf(szTemp, sizeof(szTemp), "/PRIVMSG %s \001%s %u\001", one.c_str(), two.c_str(), time(0));
			else
				mir_snprintf(szTemp, sizeof(szTemp), "/PRIVMSG %s \001%s\001", one.c_str(), GetWordAddress(text.c_str(), 2));
			PostIrcMessageWnd(window, hContact, szTemp);
		}

		if(lstrcmpi(two.c_str(), "dcc") != 0)
		{
			mir_snprintf(szTemp, sizeof(szTemp), Translate("CTCP %s request sent to %s"), two.c_str(), one.c_str());
			DoEvent(GC_EVENT_INFORMATION, "Network Log", g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
		}

		return true;
	}		
	else if (command == "/dcc")
	{
		if (one.empty() || two.empty())
			return true;
		if(lstrcmpi(one.c_str(), "send") == 0)
		{
			char szTemp[1000];

			unsigned long ulAdr = 0;
			if (prefs->ManualHost)
				ulAdr = ConvertIPToInteger(prefs->MySpecifiedHostIP);
			else
				ulAdr = ConvertIPToInteger(prefs->IPFromServer?prefs->MyHost:prefs->MyLocalHost);

			if(ulAdr)
			{
				struct CONTACT_TYPE user ={(char *)two.c_str(), NULL, NULL, false, false, true};
				HANDLE hContact = CList_AddContact(&user, false, false);

				if(hContact)
				{
					DBVARIANT dbv1;
					String s;

					
					if(DBGetContactSettingByte(hContact, IRCPROTONAME, "AdvancedMode", 0) == 0)
						DoUserhostWithReason(1, ("S" + two).c_str(), true, two.c_str());
					else
					{
						if (!DBGetContactSetting(hContact, IRCPROTONAME, "UWildcard", &dbv1) && dbv1.type == DBVT_ASCIIZ)
						{
							DoUserhostWithReason(2, ((String)"S" + dbv1.pszVal).c_str(), true, dbv1.pszVal);
							DBFreeVariant(&dbv1);
						}
						else
						{
							DoUserhostWithReason(2, ((String)"S" + two).c_str(), true, two.c_str());
						}
					}
					
					if(three == "")
						CallService(MS_FILE_SENDFILE, (WPARAM)hContact, 0);
					else
					{
						String temp = GetWordAddress(text.c_str(), 3);
						char * pp[2];
						char * p = (char *)temp.c_str();
						pp[0] = p;
						pp[1] = NULL;
						CallService(MS_FILE_SENDSPECIFICFILES, (WPARAM)hContact, (LPARAM)pp);
					}
				}
			}
			else
			{

				mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC ERROR: Unable to automatically resolve external IP"));
				DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
			}
			return true;
		}
		if(lstrcmpi(one.c_str(), "chat") == 0)
		{
			char szTemp[1000];

			unsigned long ulAdr = 0;
			if (prefs->ManualHost)
				ulAdr = ConvertIPToInteger(prefs->MySpecifiedHostIP);
			else
				ulAdr = ConvertIPToInteger(prefs->IPFromServer?prefs->MyHost:prefs->MyLocalHost);

			if(ulAdr)
			{
				String contact = two + DCCSTRING;
				struct CONTACT_TYPE user ={(char *)contact.c_str(), NULL, NULL, false, false, true};
				HANDLE hContact = CList_AddContact(&user, false, false);
				DBWriteContactSettingByte(hContact, IRCPROTONAME, "DCC", 1);

				int iPort = 0;

				if(hContact)
				{
					DCCINFO * dci = new DCCINFO;
					ZeroMemory(dci, sizeof(DCCINFO));
					dci->hContact = hContact;
					dci->sContactName = two;
					dci->iType = DCC_CHAT;
					dci->bSender = true;

					CDccSession * dcc = new CDccSession(dci);

					CDccSession * olddcc = g_ircSession.FindDCCSession(hContact);
					if (olddcc)
						olddcc->Disconnect();
					g_ircSession.AddDCCSession(hContact, dcc);
					iPort = dcc->Connect();
				}

				if (iPort != 0)
				{
					PostIrcMessage("/CTCP %s DCC CHAT chat %u %u", two.c_str(), ulAdr, iPort);
					mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC CHAT request sent to %s"), two.c_str(), one.c_str());
					DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
				}
				else
				{
					mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC ERROR: Unable to bind port"));
					DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
				}

			}
			else
			{
				mir_snprintf(szTemp, sizeof(szTemp), Translate("DCC ERROR: Unable to automatically resolve external IP"));
				DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false); 
			}

		}
		return true;
	}		
	return false;
}




static bool DoInputRequestAlias(char * text)
{
	char * p1 = strstr(text, "%question");
	if(p1)
	{
		char * infotext = NULL;
		char * title = NULL;
		char * defaulttext = NULL;
		String command = text;
		if(p1[9] == '=' && p1[10] == '\"')
		{
			infotext = &p1[11];
			p1 = strchr(infotext, '\"');
			if (p1)
			{
				*p1 = '\0';
				p1++;
				if (*p1 == ',' && p1[1] == '\"')
				{
					p1++; p1++;
					title = p1;
					p1 = strchr(title, '\"');
					if (p1)
					{
						*p1 = '\0';
						p1++;
						if (*p1 == ',' && p1[1] == '\"')
						{
							p1++; p1++;
							defaulttext = p1;
							p1 = strchr(defaulttext, '\"');
							if (p1)
							{
								*p1 = '\0';
							}

						}

					}
				}

			}
		}

		HWND question_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_QUESTION),NULL, QuestionWndProc);

		if(title)
			SetDlgItemText(question_hWnd, IDC_CAPTION, title);
		else
			SetDlgItemText(question_hWnd, IDC_CAPTION, Translate(	"Input command"	));

		if(infotext)
			SetWindowText(GetDlgItem(question_hWnd, IDC_TEXT), infotext);
		else
			SetWindowText(GetDlgItem(question_hWnd, IDC_TEXT), Translate(	"Please enter the reply"	));

		if(defaulttext)
			SetWindowText(GetDlgItem(question_hWnd, IDC_EDIT), defaulttext);

		SetDlgItemText(question_hWnd, IDC_HIDDENEDIT, command.c_str());
		PostMessage(question_hWnd, IRC_ACTIVATE, 0, 0);
		return true;
	}
	return false;
}
bool PostIrcMessage(const char * fmt, ...)
{
	if (!fmt || lstrlen(fmt) < 1 || lstrlen(fmt) > 4000)
		return 0;

	va_list marker;
	va_start(marker, fmt);
	static char szBuf[4*1024];
	vsprintf(szBuf, fmt, marker);
	va_end(marker);

	return PostIrcMessageWnd(NULL, NULL, szBuf);
}

bool PostIrcMessageWnd(char* window, HANDLE hContact, const char * szBuf)
{
	DBVARIANT dbv;
	char windowname[256];

	BYTE bDCC = 0;
	
	if(hContact)
		bDCC = DBGetContactSettingByte(hContact, IRCPROTONAME, "DCC", 0);

	if (!g_ircSession && !bDCC || !szBuf || lstrlen(szBuf) < 1)
		return 0;

	if (hContact && !DBGetContactSetting(hContact, IRCPROTONAME, "Nick", &dbv) && dbv.type == DBVT_ASCIIZ)
	{
		lstrcpyn(windowname, dbv.pszVal, 255); 
		DBFreeVariant(&dbv);
	}
	else if(window)
	{
		lstrcpyn(windowname, window, 255);
	}
	else 
	{
		lstrcpyn(windowname, "Network log", 255);
	}
	if(lstrcmpi(window, "Network log") != 0)
	{
		char * p1 = strchr(windowname, ' ');
		if (p1)
			*p1 = '\0';
	}

	// remove unecessary linebreaks, and do the aliases
	String Message = szBuf;
	Message = AddCR(Message);
	Message = RemoveLinebreaks(Message);
	if(!hContact && g_ircSession)
	{
		Message = DoAlias((char *)Message.c_str(), windowname);
		if (DoInputRequestAlias((char *)Message.c_str()))
			return 1;
		Message = ReplaceString(Message, "%newl", "\r\n");
		Message = RemoveLinebreaks(Message);
	}

	if (Message.empty())
		return 0;
	int index = 0;
	
	// process the message
	while (!Message.empty()) {
		// split the text into lines, and do an automatic textsplit on long lies as well
		bool flag = false;
		String DoThis = "";
		index = Message.find("\r\n",0);
		if (index == string::npos) {
			index = Message.length();
		}
		if (index >480)
			index = 480;
		DoThis = Message.substr(0, index);
		Message.erase(0, index);
		if (Message.find("\r\n", 0) == 0)
			Message.erase(0,2);

		//do this if it's a /raw
		if (g_ircSession && (GetWord(DoThis.c_str(), 0) == "/raw" || GetWord(DoThis.c_str(), 0) == "/quote"))
		{
			if (GetWord(DoThis.c_str(), 1).empty())
				continue;
			DoThis = ReplaceString(DoThis, "%", "%%");
			String S = GetWordAddress(DoThis.c_str(), 1);
			g_ircSession << irc::CIrcMessage(S.c_str());
			continue;
		}

		// Do this if the message is not a command
		if (GetWord(DoThis.c_str(), 0)[0] != '/' || hContact) {
			if(lstrcmpi(window, "Network log") == 0 && g_ircSession.GetInfo().sServerName != "")
				DoThis = (String)"/PRIVMSG " + g_ircSession.GetInfo().sServerName + (String)" "+ DoThis;
			else
				DoThis = (String)"/PRIVMSG " + (String)windowname + (String)" "+ DoThis;
			flag = true;
		}

		// and here we send it unless the command was a hardcoded one that should not be sent
		if (!DoHardcodedCommand(DoThis, windowname, hContact)){
			if( g_ircSession || bDCC)
			{
				if (!flag && g_ircSession )
					DoThis = DoIdentifiers(DoThis, windowname);
				if (hContact)
				{
					if(flag && bDCC)
					{
						CDccSession * dcc = g_ircSession.FindDCCSession(hContact);
						if(dcc)
						{
							DoThis = FormatMsg(DoThis);
							String mess = GetWordAddress(DoThis.c_str(), 2);
							if(mess[0] == ':')
								mess.erase(0,1);
							mess += '\n';
							dcc->SendStuff((const char*) mess.c_str());

						}
						else if(g_ircSession)
						{


						}
					}
					else if(g_ircSession)
					{
						DoThis = ReplaceString(DoThis, "%", "%%");
						DoThis = FormatMsg(DoThis);
						g_ircSession << irc::CIrcMessage(DoThis.c_str(), false, false);

					}
				}
				else
				{
					DoThis = ReplaceString(DoThis, "%", "%%");
					DoThis = FormatMsg(DoThis);
					g_ircSession << irc::CIrcMessage(DoThis.c_str(), false, true);
				}
			}
		}
	}
	return 1;
}
