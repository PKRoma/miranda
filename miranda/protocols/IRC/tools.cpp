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
extern char * IRCPROTONAME;
extern String sChannelPrefixes;
extern String sUserModes;
extern String sUserModePrefixes;
extern PREFERENCES * prefs;
extern GETEVENTFUNC			pfnAddEvent;


struct FORK_ARG {
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	unsigned (__stdcall *threadcodeex)(void*);
	void *arg;
};





void AddToUHTemp(String sCommand)
{
	DBVARIANT dbv;

	if (!DBGetContactSetting(NULL, IRCPROTONAME, "UHTemp", &dbv) && dbv.type == DBVT_ASCIIZ) 
	{
		sCommand = (String)dbv.pszVal + (String)" " + sCommand;
		DBFreeVariant(&dbv);
	}

	DBWriteContactSettingString(NULL, IRCPROTONAME, "UHTemp", sCommand.c_str());

	return;
}

void AddToJTemp(String sCommand)
{
	DBVARIANT dbv;

	if (!DBGetContactSetting(NULL, IRCPROTONAME, "JTemp", &dbv) && dbv.type == DBVT_ASCIIZ) 
	{
		sCommand = (String)dbv.pszVal + (String)" " + sCommand;
		DBFreeVariant(&dbv);
	}

	DBWriteContactSettingString(NULL, IRCPROTONAME, "JTemp", sCommand.c_str());

	return;
}




void __cdecl forkthread_r(void *param)
{	
	struct FORK_ARG *fa=(struct FORK_ARG*)param;
	void (*callercode)(void*)=fa->threadcode;
	void *arg=fa->arg;

	CallService(MS_SYSTEM_THREAD_PUSH,0,0);

	SetEvent(fa->hEvent);

	__try {
		callercode(arg);
	} __finally {
		CallService(MS_SYSTEM_THREAD_POP,0,0);
	} 

	return;
}

unsigned long forkthread (	void (__cdecl *threadcode)(void*),unsigned long stacksize,void *arg)
{
	unsigned long rc;
	struct FORK_ARG fa;

	fa.hEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
	fa.threadcode=threadcode;
	fa.arg=arg;

	rc=_beginthread(forkthread_r,stacksize,&fa);

	if ((unsigned long)-1L != rc) {
		WaitForSingleObject(fa.hEvent,INFINITE);
	} 
	CloseHandle(fa.hEvent);

	return rc;
}




String GetWord(const char * text, int index)
{
	if(!text || !lstrlen(text))
		return (String)"";

	char * p1 = (char *)text;
	char * p2 = NULL;
	String S = "";

	while (*p1 == ' ')
		p1++;

	if (*p1 != '\0')
	{

		for (int i =0; i < index; i++) 
		{
			p2 = strchr(p1, ' ');

			if (!p2)
				p2 = strchr (p1, '\0');
			else
				while (*p2 == ' ')
					p2++;

			p1 = p2;
		}



		p2 = strchr(p1, ' ');
		if(!p2)
			p2 = strchr(p1, '\0');

		if (p1 != p2)
		{
			char * pszTemp = new char[p2-p1+1];
			lstrcpyn(pszTemp, p1, p2-p1+1);

			S = pszTemp;
			delete [] pszTemp;
		}

	}

	return S;


}



char * GetWordAddress(const char * text, int index)
{
	if(!text || !lstrlen(text))
		return (char *)text;

	char *temp = (char*)text;

	while (*temp == ' ')
		temp++;

	if (index == 0)
		return (char *)temp;


	for (int i =0; i < index; i++) {
		temp = strchr(temp, ' ');
		if (!temp)
			temp = strchr (text, '\0');
		else
			while (*temp == ' ')
				temp++;
		text = temp;
	}

	return temp;
}




String RemoveLinebreaks(String Message)
{

	while (Message.find("\r\n\r\n", 0) != string::npos)
		Message = ReplaceString(Message, "\r\n\r\n", "\r\n");

	if (Message.find("\r\n", 0) == 0)
		Message.erase(0,2);

	if (Message.length() >1 && Message.rfind("\r\n", Message.length()) == Message.length()-2)
		Message.erase(Message.length()-2, 2);

	return Message;
}



String ReplaceString (String text, char * replaceme, char * newword)
{
	if (text != "" && replaceme != NULL)
	{
		int i = 0;

		while (text.find(replaceme, i) != string::npos )
		{
			i = text.find(replaceme, i);
			text.erase(i,lstrlen(replaceme));
			text.insert(i, newword);
			i = i + lstrlen(newword);
		}

	}

	return text;
}

char * IrcLoadFile(char * szPath){
	char * szContainer = NULL;
	DWORD dwSiz = 0;
	FILE *hFile = fopen(szPath,"rb");	

	if (hFile != NULL )
	{
		fseek(hFile,0,SEEK_END); // seek to end
		dwSiz = ftell(hFile); // size
		fseek(hFile,0,SEEK_SET); // seek back to original pos
		szContainer = new char [dwSiz+1];
		fread(szContainer, 1, dwSiz, hFile);
		szContainer[dwSiz] = '\0';
		fclose(hFile);

		return szContainer;
	}

	return 0;
}




int WCCmp(char* wild, char *string)
{
	if ( wild == NULL || !lstrlen(wild) || string == NULL || !lstrlen(string))
		return 1;
	char *cp, *mp;
	
	while ((*string) && (*wild != '*')) 
	{
		if ((*wild != *string) && (*wild != '?')) {
			return 0;
		}
		wild++;
		string++;
	}
		
	while (*string) 
	{
		if (*wild == '*') {
			if (!*++wild) {
				return 1;
			}
			mp = wild;
			cp = string+1;
		} else if ((*wild == *string) || (*wild == '?')) {
			wild++;
			string++;
		} else {
			wild = mp;
			string = cp++;
		}
	}
		
	while (*wild == '*') {
		wild++;
	}
	return !*wild;
}	

bool IsChannel(String sName) 
{
	if(strchr(sChannelPrefixes.c_str(), sName[0]))
		return true;

	return false;
}

char* my_strstri(char *s1, char *s2) 
{ 
	int i,j,k; 
	for(i=0;s1[i];i++) 
		for(j=i,k=0;tolower(s1[j])==tolower(s2[k]);j++,k++) 
			if(!s2[k+1]) 
				return (s1+i); 

	return NULL; 
} 

char * DoColorCodes (const char * text, bool bStrip, bool bReplacePercent)
{
	static char szTemp[4000];
	szTemp[0] = '\0';
	char * p = &szTemp[0];
	bool bBold = false;
	bool bUnderline = false;
	bool bItalics = false;

	if(!text)
		return szTemp;

	while (*text != '\0')
	{
		int iFG = -1;
		int iBG = -1;

		switch(*text)
		{
		case '%': //escape
			*p = '%'; 
			p++;
			if(bReplacePercent)
			{
				*p = '%'; 
				p++;
			}
			text++;
			break;

		case 2: //bold
			if(!bStrip)
			{
				*p = '%'; 
				p++;
				*p = bBold?'B':'b'; 
				p++;
			}
			bBold = !bBold;
			text++;
			break;

		case 15: //reset
			if(!bStrip)
			{
				*p = '%'; 
				p++;
				*p = 'r'; 
				p++;
			}
			bUnderline = false;
			bBold = false;
			text++;
			break;

		case 22: //italics
			if(!bStrip)
			{
				*p = '%'; 
				p++;
				*p = bItalics?'I':'i'; 
				p++;
			}
			bItalics = !bItalics;
			text++;
			break;

		case 31: //underlined
			if(!bStrip)
			{
				*p = '%'; 
				p++;
				*p = bUnderline?'U':'u'; 
				p++;
			}
			bUnderline = !bUnderline;
			text++;
			break;

		case 3: //colors
			text++;

			// do this if the colors should be reset to default
			if(*text <= 47 || *text >= 58 || *text == '\0')
			{
				if(!bStrip)
				{
					*p = '%'; p++;
					*p = 'C'; p++;
					*p = '%'; p++;
					*p = 'F'; p++;
				}
				break;
			}
			else // some colors should be set... need to find out who
			{
				char szTemp[3];

				// fix foreground index
				if(text[1] > 47 && text[1] < 58 && text[1] != '\0')
					lstrcpyn(szTemp, text, 3);
				else
					lstrcpyn(szTemp, text, 2);
				text += lstrlen(szTemp); 
				iFG = atoi(szTemp);

				// fix background color
				if(*text == ',' && text[1] > 47 && text[1] < 58 && text[1] != '\0')
				{
					text++;

					if(text[1] > 47 && text[1] < 58 && text[1] != '\0')
						lstrcpyn(szTemp, text, 3);
					else
						lstrcpyn(szTemp, text, 2);
					text += lstrlen(szTemp); 
					iBG = atoi(szTemp);
				}


			}
			
			if( iFG >= 0 && iFG != 99)
				while( iFG > 15)
					iFG -= 16;
			if( iBG >= 0 && iBG != 99)
				while( iBG > 15 )
					iBG -= 16;

			// create tag for chat.dll
			if(!bStrip)
			{
				char szTemp[10];
				if(iFG >= 0 && iFG != 99)
				{
					*p = '%'; p++;
					*p = 'c';p++;

					mir_snprintf(szTemp, sizeof(szTemp), "%03u%03u%03u", GetRValue(prefs->colors[iFG]), GetGValue(prefs->colors[iFG]), GetBValue(prefs->colors[iFG]));
					for (int i = 0; i<9; i++)
					{
						*p = szTemp[i]; p++;
					}
				}
				else if (iFG == 99)
				{
					*p = '%'; p++;
					*p = 'C';p++;
				}
				if(iBG >= 0 && iBG != 99)
				{
					*p = '%'; p++;
					*p = 'f';p++;

					mir_snprintf(szTemp, sizeof(szTemp), "%03u%03u%03u", GetRValue(prefs->colors[iBG]), GetGValue(prefs->colors[iBG]), GetBValue(prefs->colors[iBG]));
					for (int i = 0; i<9; i++)
					{
						*p = szTemp[i]; p++;
					}
				}
				else if (iBG == 99)
				{
					*p = '%'; p++;
					*p = 'F';p++;
				}
			}
		




/*
			if(*text > 47 && *text < 58 && *text != '\0')
			{
				if (text[1] != '\0' && text[1] > 47 && text[1] < 58 && *text >47 && *text < 50)
				{
					iText = (*text-48)*10 + (text[1]-48) ;//ASCII to int
					text++; text ++;
					if (*text != '\0' && text[1] != '\0' && *text == ',' && text[1] >47 && text[1] < 58)
					{
						if (text[2] != '\0'  && text[2] > 47 && text[2] < 58 && text[1] >47 && text[1] < 50)
						{
							iBG = (text[1]-48)*10 + (text[2]-48);
							text++;text++;text++;
						}
						else
						{
							iBG = text[1]-48;
							text++; text++;
						}
					}
				}
				else 
				{
					iText = *text-48;
					text++;
					if (*text != '\0' && text[1] != '\0' && *text == ',' && text[1] >47 && text[1] < 58)
					{
						if (text[2] != '\0'  && text[2] > 47 && text[2] < 58 && text[1] >47 && text[1] < 50)
						{
							iBG = (text[1]-48)*10 + (text[2]-48);
							text++;text++;text++;
						}
						else
						{
							iBG = text[1]-48;
							text++; text++;
						}
					}
				}
				
				if(!bStrip && iText >= 0 && iText < 16)
				{
					char szTemp[10];
					mir_snprintf(szTemp, sizeof(szTemp), "%03u%03u%03u", GetRValue(prefs->colors[iText]), GetGValue(prefs->colors[iText]), GetBValue(prefs->colors[iText]));
					*p = '%'; p++;
					*p = 'c';p++;
					for (int i = 0; i<9; i++)
					{
						*p = szTemp[i]; p++;
					}
				}
				
				if(!bStrip && iBG >= 0 && iBG < 16)
				{
					char szTemp[10];
					mir_snprintf(szTemp, sizeof(szTemp), "%03u%03u%03u", GetRValue(prefs->colors[iBG]), GetGValue(prefs->colors[iBG]), GetBValue(prefs->colors[iBG]));
					*p = '%'; p++;
					*p = 'f';p++;
					for (int i = 0; i<9; i++)
					{
						*p = szTemp[i]; p++;
					}
				}
				

			}
			else if (!bStrip)
			{
				*p = '%'; p++;
				*p = 'C'; p++;
				*p = '%'; p++;
				*p = 'F'; p++;
			}
*/
			break;

		default:
			*p = *text;
			p++;
			text++;
			break;
			
		}
		char * o = szTemp;
	}
	*p = '\0';

	return szTemp;

}



int DoEvent(int iEvent, const char* pszWindow, const char * pszNick, const char* pszText, const char* pszStatus, const char* pszUserInfo, DWORD dwItemData, bool bAddToLog, bool bIsMe)
{						   
	GCDEST gcd = {0};
	GCEVENT gce = {0};
	String sID;
	String sText = "";
	extern bool bEcho;

	if(iEvent == GC_EVENT_INFORMATION && bIsMe && !bEcho)
			return false;

	if(pszText)
	{
		if (iEvent != GC_EVENT_SENDMESSAGE)
			sText = DoColorCodes(pszText, FALSE, TRUE);
		else
			sText = pszText;
	}

	if (pszWindow)
	{
		if(lstrcmpi(pszWindow, "Network log"))
			sID = pszWindow + (String)" - " + g_ircSession.GetInfo().sNetwork;
		else
			sID = pszWindow;
		gcd.pszID = (char *)sID.c_str();
	}
	else
		gcd.pszID = NULL;


	gcd.pszModule = IRCPROTONAME;
	gcd.iType = iEvent;

	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	gce.pszStatus = (char *)pszStatus;
	gce.bAddToLog = bAddToLog;
	gce.pszNick = (char *)pszNick;
	gce.pszUID = (char *)pszNick;
	gce.pszUserInfo = prefs->ShowAddresses?(char *)pszUserInfo:NULL;

	if(sText != "")
		gce.pszText = (char *)sText.c_str();

	gce.dwItemData = dwItemData;
	gce.time = time(NULL);
	gce.bIsMe = bIsMe;
	if(pfnAddEvent)
		return pfnAddEvent(0, (LPARAM)&gce);
	else
		return CallService(MS_GC_EVENT, 0, (LPARAM)&gce);


}




String ModeToStatus(char sMode) 
{
	if(strchr(sUserModes.c_str(), sMode))
	{

		switch(sMode)
		{
		case 'q':
			return (String)"Owner";
		case 'o':
			return (String)"Op";
		case 'v':
			return (String)"Voice";
		case 'h':
			return (String)"Halfop";
		case 'a':
			return (String)"Admin";
		default:
			return (String)"Unknown";
		}

	}

	return (String)"Normal";
}
String PrefixToStatus(char cPrefix) 
{
	int i = (int)strchr(sUserModePrefixes.c_str(), cPrefix);
	if(i)
	{
		int index = i - (int)sUserModePrefixes.c_str();
		return ModeToStatus(sUserModes[index]);
	}

	return (String)"Normal";
}

void SetChatTimer(UINT_PTR &nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc)
{
	if (nIDEvent)
		KillChatTimer(nIDEvent);

	nIDEvent = SetTimer(NULL, NULL, uElapse, lpTimerFunc);

	return;
}

void KillChatTimer(UINT_PTR &nIDEvent)
{
	if (nIDEvent)
		KillTimer(NULL, nIDEvent);

	nIDEvent = NULL;

	return;
}



int SetChannelSBText(String sWindow, CHANNELINFO * wi)
{
	String sTemp = "";
	if(wi->pszMode)
	{
		sTemp += "[";
		sTemp += wi->pszMode;
		sTemp += "]" ;
		sTemp += " ";
	}
	if(wi->pszTopic)
		sTemp += wi->pszTopic;
	sTemp = DoColorCodes(sTemp.c_str(), TRUE, FALSE);
	return DoEvent(GC_EVENT_SETSBTEXT, sWindow.c_str(), NULL, sTemp.c_str(), NULL, NULL, NULL, FALSE, FALSE);
	

}



String MakeWndID(String sWindow)
{
	String sTemp = sWindow;
	sTemp += " - ";
	if(g_ircSession)
		sTemp += g_ircSession.GetInfo().sNetwork;
	else
		sTemp += Translate("Offline");

	return sTemp;
	

}



bool FreeWindowItemData(String window, CHANNELINFO* wis)
{
	CHANNELINFO* wi;
	if(!wis)
		wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window.c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE);
	else
		wi = wis;
	if(wi)
	{
		if(wi->pszLimit)
			delete[]wi->pszLimit;
		if(wi->pszMode)
			delete[]wi->pszMode;
		if(wi->pszPassword)
			delete[]wi->pszPassword;
		if(wi->pszTopic)
			delete[]wi->pszTopic;
		delete wi;

		return true;
	}
	return false;
}



bool AddWindowItemData(String window, const char * pszLimit, const char * pszMode, const char * pszPassword, const char * pszTopic)
{
	CHANNELINFO* wi;

	wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window.c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE);
	if(wi)
	{
		if(pszLimit)
		{
			wi->pszLimit = (char *)realloc(wi->pszLimit, lstrlen(pszLimit)+1);
			lstrcpy(wi->pszLimit, pszLimit);
		}
		if(pszMode)
		{
			wi->pszMode = (char *)realloc(wi->pszMode, lstrlen(pszMode)+1);
			lstrcpy(wi->pszMode, pszMode);
		}
		if(pszPassword)
		{
			wi->pszPassword = (char *)realloc(wi->pszPassword, lstrlen(pszPassword)+1);
			lstrcpy(wi->pszPassword, pszPassword);
		}
		if(pszTopic)
		{
			wi->pszTopic = (char *)realloc(wi->pszTopic, lstrlen(pszTopic)+1);
			lstrcpy(wi->pszTopic, pszTopic);
		}

		SetChannelSBText(window, wi);

		return true;
	}
	return false;
}

void FindLocalIP(HANDLE con) // inspiration from jabber
{
		// Determine local IP
		int socket = CallService(MS_NETLIB_GETSOCKET, (WPARAM) con, 0);
		if (socket!=INVALID_SOCKET) 
		{
			struct sockaddr_in saddr;
			int len;

			len = sizeof(saddr);
			getsockname(socket, (struct sockaddr *) &saddr, &len);

			lstrcpyn(prefs->MyLocalHost, inet_ntoa(saddr.sin_addr), 49);

		}

} 