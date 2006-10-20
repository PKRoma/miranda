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
extern bool bMbotInstalled;
extern MM_INTERFACE		mmi;

std::vector<String> vUserhostReasons;
std::vector<String> vWhoInProgress;

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
			temp = (char *)strchr (text, '\0');
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

					mir_snprintf(szTemp, sizeof(szTemp), "%02u", iFG);
					for (int i = 0; i<2; i++)
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

					mir_snprintf(szTemp, sizeof(szTemp), "%02u", iBG);
					for (int i = 0; i<2; i++)
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


int CallChatEvent(WPARAM wParam, LPARAM lParam)
{
	
	GCEVENT * gce = (GCEVENT *)lParam;
	int iVal = 0;

	// first see if the scripting module should modify or stop this event
	if(bMbotInstalled && prefs->ScriptingEnabled && gce 
		&& gce->time != 0 && (gce->pDest->pszID == NULL 
		|| lstrlen(gce->pDest->pszID) != 0 && lstrcmpi(gce->pDest->pszID , "Network Log")) )
	{
		GCEVENT *gcevent= (GCEVENT*) lParam;
		GCEVENT *gcetemp = NULL;
		WPARAM wp = wParam;
		gcetemp = (GCEVENT *)mmi.mmi_malloc(sizeof(GCEVENT));
		gcetemp->pDest = (GCDEST *)mmi.mmi_malloc(sizeof(GCDEST));
		gcetemp->pDest->iType = gcevent->pDest->iType;
		gcetemp->dwFlags = gcevent->dwFlags;
		gcetemp->bIsMe = gcevent->bIsMe;
		gcetemp->cbSize = sizeof(GCEVENT);
		gcetemp->dwItemData = gcevent->dwItemData;
		gcetemp->time = gcevent->time;
		if(gcevent->pDest->pszID)
		{
			char * pTemp = NULL;
			gcetemp->pDest->pszID = (char *)mmi.mmi_malloc(lstrlen(gcevent->pDest->pszID) + 1);
			lstrcpyn(gcetemp->pDest->pszID, gcevent->pDest->pszID, lstrlen(gcevent->pDest->pszID) + 1);
			pTemp = strchr(gcetemp->pDest->pszID, ' ');
			if(pTemp)
				*pTemp = '\0';
		}else gcetemp->pDest->pszID = NULL;

		if(gcevent->pDest->pszModule)
		{
			gcetemp->pDest->pszModule = (char *)mmi.mmi_malloc(lstrlen(gcevent->pDest->pszModule) + 1);
			lstrcpyn(gcetemp->pDest->pszModule, gcevent->pDest->pszModule, lstrlen(gcevent->pDest->pszModule) + 1);
		}else gcetemp->pDest->pszModule = NULL;

		if(gcevent->pszText)
		{
			gcetemp->pszText = (char *)mmi.mmi_malloc(lstrlen(gcevent->pszText) + 1);
			lstrcpyn((char *)gcetemp->pszText, gcevent->pszText, lstrlen(gcevent->pszText) + 1);
		}else gcetemp->pszText = NULL;

		if(gcevent->pszUID)
		{
			gcetemp->pszUID = (char *)mmi.mmi_malloc(lstrlen(gcevent->pszUID) + 1);
			lstrcpyn((char *)gcetemp->pszUID, gcevent->pszUID, lstrlen(gcevent->pszUID) + 1);
		}else gcetemp->pszUID = NULL;

		if(gcevent->pszNick)
		{
			gcetemp->pszNick = (char *)mmi.mmi_malloc(lstrlen(gcevent->pszNick) + 1);
			lstrcpyn((char *)gcetemp->pszNick, gcevent->pszNick, lstrlen(gcevent->pszNick) + 1);
		}else gcetemp->pszNick = NULL;

		if(gcevent->pszStatus)
		{
			gcetemp->pszStatus = (char *)mmi.mmi_malloc(lstrlen(gcevent->pszStatus) + 1);
			lstrcpyn((char *)gcetemp->pszStatus, gcevent->pszStatus, lstrlen(gcevent->pszStatus) + 1);
		}else gcetemp->pszStatus = NULL;

		if(gcevent->pszUserInfo)
		{
			gcetemp->pszUserInfo = (char *)mmi.mmi_malloc(lstrlen(gcevent->pszUserInfo) + 1);
			lstrcpyn((char *)gcetemp->pszUserInfo, gcevent->pszUserInfo, lstrlen(gcevent->pszUserInfo) + 1);
		}else gcetemp->pszUserInfo = NULL;

		if(	Scripting_TriggerMSPGuiIn(&wp, gcetemp) && gcetemp)
		{
			if(gcetemp && gcetemp->pDest && gcetemp->pDest->pszID)
			{
				String sTempId = MakeWndID(gcetemp->pDest->pszID);
				mmi.mmi_realloc(gcetemp->pDest->pszID, sTempId.length() + 1);
				lstrcpyn(gcetemp->pDest->pszID, sTempId.c_str(), sTempId.length()+1); 
			}
			if(pfnAddEvent)
				iVal = pfnAddEvent(wp, (LPARAM) gcetemp);
			else
				iVal = CallService(MS_GC_EVENT, wp, (LPARAM) gcetemp);
		}
		if (gcetemp)
		{
			if(gcetemp->pszNick)
				mmi.mmi_free((void *)gcetemp->pszNick);
			if(gcetemp->pszUID)
				mmi.mmi_free((void *)gcetemp->pszUID);
			if(gcetemp->pszStatus)
				mmi.mmi_free((void *)gcetemp->pszStatus);
			if(gcetemp->pszUserInfo)
				mmi.mmi_free((void *)gcetemp->pszUserInfo);
			if(gcetemp->pszText)
				mmi.mmi_free((void *)gcetemp->pszText);
			if(gcetemp->pDest->pszID)
				mmi.mmi_free((void *)gcetemp->pDest->pszID);
			if(gcetemp->pDest->pszModule)
				mmi.mmi_free((void *)gcetemp->pDest->pszModule);
			mmi.mmi_free((void *)gcetemp->pDest);
			mmi.mmi_free((void *)gcetemp);
		}

		return iVal;
	}

	if(pfnAddEvent)
		iVal = pfnAddEvent(wParam, (LPARAM) gce);
	else
		iVal = CallService(MS_GC_EVENT, wParam, (LPARAM) gce);

	return iVal;
}

int DoEvent(int iEvent, const char* pszWindow, const char * pszNick, 
			const char* pszText, const char* pszStatus, const char* pszUserInfo, 
			DWORD dwItemData, bool bAddToLog, bool bIsMe, time_t timestamp)
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
	gce.dwFlags =  (bAddToLog) ? GCEF_ADDTOLOG : 0;
	gce.pszNick = (char *)pszNick;
	gce.pszUID = (char *)pszNick;
	gce.pszUserInfo = prefs->ShowAddresses?(char *)pszUserInfo:NULL;

	if(sText != "")
		gce.pszText = (char *)sText.c_str();

	gce.dwItemData = dwItemData;
	if(timestamp == 1)
		gce.time = time(NULL);
	else
		gce.time = timestamp;
	gce.bIsMe = bIsMe;
	return CallChatEvent((WPARAM)0, (LPARAM)&gce);


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
	return DoEvent(GC_EVENT_SETSBTEXT, sWindow.c_str(), NULL, sTemp.c_str(), NULL, NULL, NULL, FALSE, FALSE, 0);
	

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
		wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window.c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
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

	wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window.c_str(), NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
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

void DoUserhostWithReason(int type, String reason, bool bSendCommand, String userhostparams, ...)
{
	char temp[4096];
	String S = "";
	switch(type)
	{
	case 1:
		S = "USERHOST";
		break;
	case 2:
		S = "WHO";
		break;
	default:
		S= "USERHOST";
		break;
	}

	va_list ap;
	va_start(ap, userhostparams);
	mir_vsnprintf(temp, sizeof(temp), (S + (String)" " + userhostparams).c_str(), ap);
	va_end(ap);

	// Add reason
	if (type ==1)
		vUserhostReasons.push_back(reason);
	else if (type == 2)
		vWhoInProgress.push_back(reason);

	// Do command
	if (g_ircSession && bSendCommand)
		g_ircSession << CIrcMessage(temp, false, false);
}

String GetNextUserhostReason(int type)
{
	String reason = "";
	switch(type)
	{
	case 1:
		if(!vUserhostReasons.size())
			return (String)"";

		// Get reason
		reason = vUserhostReasons.front();
		vUserhostReasons.erase(vUserhostReasons.begin());
		break;
	case 2:
		if(!vWhoInProgress.size())
			return (String)"";

		// Get reason
		reason = vWhoInProgress.front();
		vWhoInProgress.erase(vWhoInProgress.begin());
		break;
	default:break;
	}

	return reason;
}

String PeekAtReasons(int type)
{
	switch (type)
	{
	case 1:
		if(!vUserhostReasons.size())
			return (String)"";
		return vUserhostReasons.front();
		break;
	case 2:
		if(!vWhoInProgress.size())
			return (String)"";
		return vWhoInProgress.front();
		break;
	default:break;
	}
	return (String)"";

}

void ClearUserhostReasons(int type)
{
	switch (type)
	{
	case 1:
		vUserhostReasons.clear();
		break;
	case 2:
		vWhoInProgress.clear();
		break;
	default:break;
	}
	return;
}
