
/*
IRC plugin for Miranda IM

Copyright (C) 2003 J�rgen Persson

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

extern PREFERENCES *prefs;
extern char * IRCPROTONAME;


BOOL CList_AddDCCChat(String name, String hostmask, unsigned long adr, int port) 
{
	CLISTEVENT cle = {0};
	HANDLE hContact;
	HANDLE hc;
	char szNick[256];
	char szService[256];
	bool bFlag = false;

	CONTACT usertemp ={(char *)name.c_str(), NULL, NULL, false, false, true};
	hc = CList_FindContact(&usertemp);
	if(hc && DBGetContactSettingByte(hc,"CList", "NotOnList", 0) == 0
		&& DBGetContactSettingByte(hc,"CList", "Hidden", 0) == 0)
	{
		bFlag = true;
	}

	String contactname = name + DCCSTRING;

	CONTACT user ={(char *)contactname.c_str(), NULL, NULL, false, false, true};
	hContact = CList_AddContact(&user, false, false);
	DBWriteContactSettingByte(hContact, IRCPROTONAME, "DCC", 1);

	DCCINFO * pdci = new DCCINFO;
	ZeroMemory(pdci, sizeof(DCCINFO));
	pdci->sHostmask = hostmask;
	pdci->hContact = hContact;
	pdci->dwAdr = (DWORD) adr;
	pdci->iPort = port;
	pdci->iType = DCC_CHAT;
	pdci->bSender = false;
	pdci->sContactName = name;

	if(prefs->DCCChatAccept == 3 || prefs->DCCChatAccept == 2 && bFlag)
	{
		CDccSession * dcc = new CDccSession(pdci);

		CDccSession * olddcc = g_ircSession.FindDCCSession(hContact);
		if (olddcc)
			olddcc->Disconnect();

		g_ircSession.AddDCCSession(hContact, dcc);

		dcc->Connect();

	}
	else 
	{
		cle.cbSize=sizeof(cle);
		cle.hContact=(HANDLE)hContact;
		cle.hDbEvent=(HANDLE)"dccchat";	
		cle.flags = 0;
		cle.hIcon=LoadIconEx(IDI_DCC,"dcc",GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON));
		mir_snprintf(szService, sizeof(szService),"%s/DblClickEvent", IRCPROTONAME);
		cle.pszService= szService ;
		mir_snprintf(szNick, sizeof(szNick), Translate("CTCP chat request from %s"), name.c_str());
		cle.pszTooltip= szNick;
		cle.lParam = (LPARAM)pdci;

		if(CallService(MS_CLIST_GETEVENT, (WPARAM)hContact, (LPARAM)0))
			CallService(MS_CLIST_REMOVEEVENT, (WPARAM)hContact, (LPARAM)"dccchat");
		CallService(MS_CLIST_ADDEVENT,(WPARAM) hContact,(LPARAM) &cle);
	}
	return TRUE;
}

HANDLE CList_AddContact(struct CONTACT_TYPE * user, bool InList, bool SetOnline)
{
	if (user->name == NULL)
		return 0;
	HANDLE hContact = CList_FindContact(user);
	if (hContact ) 
	{
		if (InList)
			DBDeleteContactSetting(hContact, "CList", "NotOnList");
		DBWriteContactSettingString(hContact, IRCPROTONAME, "Nick", user->name);
		DBDeleteContactSetting(hContact, "CList", "Hidden");
		if (SetOnline && DBGetContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE)== ID_STATUS_OFFLINE)
			DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_ONLINE);
		return hContact;
	}
	
	// here we create a new one since no one is to be found
	hContact = (HANDLE) CallService(MS_DB_CONTACT_ADD, 0, 0);
	if (hContact)
	{
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM) hContact, (LPARAM) IRCPROTONAME);

		if (InList)
			DBDeleteContactSetting(hContact, "CList", "NotOnList");
		else
			DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
		DBDeleteContactSetting(hContact, "CList", "Hidden");
		DBWriteContactSettingString(hContact, IRCPROTONAME, "Nick", user->name);
		DBWriteContactSettingString(hContact, IRCPROTONAME, "Default", user->name);
		DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", SetOnline ? ID_STATUS_ONLINE:ID_STATUS_OFFLINE);
		return hContact;
	}
	return false;
}

HANDLE CList_SetOffline(struct CONTACT_TYPE * user)
{
	DBVARIANT dbv;
    HANDLE hContact = CList_FindContact(user);
	if (hContact ) 
	{
		if (!DBGetContactSetting(hContact, IRCPROTONAME, "Default", &dbv) && dbv.type == DBVT_ASCIIZ)
		{
			DBWriteContactSettingString(hContact, IRCPROTONAME, "User", "");
			DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", "");
			DBWriteContactSettingString(hContact, IRCPROTONAME, "Nick", dbv.pszVal);
			DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE);
			DBFreeVariant(&dbv);
			return hContact;
		}
	}

	return 0;
}
bool CList_SetAllOffline(BYTE ChatsToo)
{
    HANDLE hContact;
    char *szProto;
	DBVARIANT dbv;


	g_ircSession.DisconnectAllDCCSessions(false);


    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) 
	{
       szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
       if (szProto != NULL && !lstrcmpi(szProto, IRCPROTONAME)) 
	   {
			if(DBGetContactSettingByte(hContact, IRCPROTONAME, "ChatRoom", 0) == 0)
			{
				if (DBGetContactSettingByte(hContact, IRCPROTONAME, "DCC", 0) != 0)
				{
					if(ChatsToo)
						DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE);
				}
				else if (!DBGetContactSetting(hContact, IRCPROTONAME, "Default", &dbv) && dbv.type == DBVT_ASCIIZ)
				{
					DBWriteContactSettingString(hContact, IRCPROTONAME, "Nick", dbv.pszVal);
					DBWriteContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE);
					DBFreeVariant(&dbv);
				}
				DBDeleteContactSetting(hContact, IRCPROTONAME, "IP");
				DBWriteContactSettingString(hContact, IRCPROTONAME, "User", "");
				DBWriteContactSettingString(hContact, IRCPROTONAME, "Host", "");
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	return true;
}

HANDLE CList_FindContact (struct CONTACT_TYPE * user) 
{
	if (!user || !user->name)
		return 0;
	char * lowercasename = new char [lstrlen(user->name)+2];
	lstrcpyn(lowercasename, user->name, lstrlen(user->name)+1);
	CharLower(lowercasename);
	HANDLE hContact;
	char *szProto;
	DBVARIANT dbv1;
	DBVARIANT dbv2;	
	DBVARIANT dbv3;	
	DBVARIANT dbv4;	
	DBVARIANT dbv5;	
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) 
	{
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (szProto != NULL && !lstrcmpi(szProto, IRCPROTONAME)) 
		{
			if(DBGetContactSettingByte(hContact, IRCPROTONAME, "ChatRoom", 0) == 0)
			{
				HANDLE hContact_temp =NULL;
				char * DBDefault = NULL;
				char * DBNick = NULL;
				char * DBWildcard = NULL;
				char * DBUser = NULL;
				char * DBHost = NULL;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "Default", &dbv1) && dbv1.type == DBVT_ASCIIZ) DBDefault = dbv1.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "Nick", &dbv2)&& dbv2.type == DBVT_ASCIIZ) DBNick = dbv2.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "UWildcard", &dbv3)&& dbv3.type == DBVT_ASCIIZ) DBWildcard = dbv3.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "UUser", &dbv4)&& dbv4.type == DBVT_ASCIIZ) DBUser = dbv4.pszVal;
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "UHost", &dbv5)&& dbv5.type == DBVT_ASCIIZ) DBHost = dbv5.pszVal;
				
				if (DBWildcard)
					CharLower(DBWildcard);
				if (IsChannel(user->name)){
					if(DBDefault && !lstrcmpi(DBDefault, user->name))
						hContact_temp = (HANDLE)-1;
				}
				else if (user->ExactNick && DBNick && !lstrcmpi(DBNick, user->name))
					hContact_temp = hContact;
				
				else if (user->ExactOnly && DBDefault && !lstrcmpi(DBDefault, user->name))
					hContact_temp = hContact;
			
				else if (user->ExactWCOnly)
				{
					if(DBWildcard && !lstrcmpi(DBWildcard, lowercasename) 
						|| (DBWildcard && !lstrcmpi(DBNick, lowercasename) && !WCCmp(DBWildcard, lowercasename) )
						|| (!DBWildcard && !lstrcmpi(DBNick, lowercasename))		)
					{
						hContact_temp = hContact;
					}
				}
				else if(strchr(user->name, ' ') == 0)
				{
					if ((DBDefault && !lstrcmpi(DBDefault, user->name) || DBNick && !lstrcmpi(DBNick, user->name) || DBWildcard && WCCmp(DBWildcard, lowercasename))
						&& (WCCmp(DBUser, user->user) && WCCmp(DBHost, user->host)))
					{
							hContact_temp = hContact;
					}
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

				if (hContact_temp != NULL) {
					delete [] lowercasename;
					if(hContact_temp != (HANDLE)-1)
						return hContact_temp;
					return 0;
				}
			}
			
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	delete [] lowercasename;
	return 0;
}







