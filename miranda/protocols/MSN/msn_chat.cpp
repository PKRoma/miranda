/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

#include "msn_global.h"
#include "..\..\include\m_history.h"

static LONG sttChatID = 0;

int MSN_ChatInit( WPARAM wParam, LPARAM lParam )
{
	ThreadData *info = (ThreadData*)wParam;
	GCWINDOW gcw = {0};
	GCDEST gcd = {0};
	GCEVENT gce = {0};

	InterlockedIncrement( &sttChatID );
	ltoa( sttChatID, info->mChatID, 10 );

	info->mJoinedContacts = ( HANDLE* )realloc(info->mJoinedContacts, sizeof(HANDLE)*(++info->mJoinedCount));
	info->mJoinedContacts[info->mJoinedCount - 1] = info->mJoinedContacts[0];
	info->mJoinedContacts[0] = ( HANDLE )-sttChatID;

	char szName[ 512 ];
	_snprintf( szName, sizeof( szName ), "%s%s", Translate("MSN Chat #"), info->mChatID );

	gcw.cbSize = sizeof(GCWINDOW);
	gcw.iType = GCW_CHATROOM;
	gcw.pszModule = msnProtocolName;
	gcw.pszName = szName;
	gcw.pszID = info->mChatID;
	gcw.pszStatusbarText = NULL;
	gcw.bDisableNickList = FALSE;
	MSN_CallService(MS_GC_NEWCHAT, NULL, (LPARAM)&gcw);

	gce.cbSize = sizeof(GCEVENT);
	gcd.pszModule = msnProtocolName;
	gcd.pszID = info->mChatID;
	gcd.iType = GC_EVENT_ADDGROUP;
	gce.pDest = &gcd;
	gce.pszStatus = Translate("Myself");
	MSN_CallService(MS_GC_EVENT, NULL, (LPARAM)&gce);

	DBVARIANT dbv, dbv2;
	gcd.iType = GC_EVENT_JOIN;
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	gce.pszStatus, Translate("Myself");
	DBGetContactSetting(NULL, msnProtocolName, "Nick", &dbv);
	gce.pszNick = dbv.pszVal;
	DBGetContactSetting(NULL, msnProtocolName, "e-mail", &dbv2);
	gce.pszUID = dbv2.pszVal;
	gce.time = 0;
	gce.bIsMe = TRUE;
	gce.bAddToLog = FALSE;
	MSN_CallService(MS_GC_EVENT, NULL, (LPARAM)&gce);
	DBFreeVariant(&dbv);
	DBFreeVariant(&dbv2);

	gcd.iType = GC_EVENT_ADDGROUP;
	gce.pDest = &gcd;
	gce.pszStatus = Translate("Others");
	MSN_CallService(MS_GC_EVENT, NULL, (LPARAM)&gce);

	gce.cbSize = sizeof(GCEVENT);
	gcd.iType = GC_EVENT_CONTROL;
	gce.pDest = &gcd;
	MSN_CallService(MS_GC_EVENT, WINDOW_INITDONE, (LPARAM)&gce);
	MSN_CallService(MS_GC_EVENT, WINDOW_ONLINE, (LPARAM)&gce);
	MSN_CallService(MS_GC_EVENT, WINDOW_VISIBLE, (LPARAM)&gce);
	return 0;
}

void KillChatSession(char* id, GCHOOK* gch) {
	int chatID = atoi(id);
	ThreadData* thread = MSN_GetThreadByContact((HANDLE)-chatID);
	GCDEST gcd = {0};
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	gcd.pszModule = gch->pDest->pszModule;
	gcd.pszID = gch->pDest->pszID;
	gcd.iType = GC_EVENT_CONTROL;
	MSN_CallService(MS_GC_EVENT, WINDOW_OFFLINE, (LPARAM)&gce);
	MSN_CallService(MS_GC_EVENT, WINDOW_TERMINATE, (LPARAM)&gce);
}

int MSN_GCEventHook(WPARAM wParam,LPARAM lParam) {
	GCHOOK *gch = (GCHOOK*) lParam;
	char S[512] = "";

	if(gch) {
		if (!lstrcmpi(gch->pDest->pszModule, msnProtocolName)) {
			char *p = new char[lstrlen(gch->pDest->pszID)+1];
			lstrcpy(p, gch->pDest->pszID);
//			char *p2 = strstr(p1, " - ");
//			if (p2)
//				*p2 = '\0';
			switch (gch->pDest->iType) {
			case GC_USER_TERMINATE: {
				int chatID = atoi(p);
				ThreadData* thread = MSN_GetThreadByContact((HANDLE)-chatID);
				// open up srmm dialog when quit while 1 person left
				if ( thread->mJoinedCount == 1 ) {
					// switch back to normal session
					thread->mJoinedContacts[0] = thread->mJoinedContacts[1];
					thread->mJoinedContacts = ( HANDLE* )realloc( thread->mJoinedContacts, sizeof( HANDLE ) );
					MSN_CallService(MS_MSG_SENDMESSAGE, (WPARAM)thread->mJoinedContacts[0], 0);
					thread->mChatID[0] = 0;
				}
				else thread->sendPacket( "OUT", NULL );
				break;
			}
			case GC_USER_MESSAGE:
				if(gch && gch->pszText && lstrlen(gch->pszText) > 0) {
					CCSDATA ccs = {0};

					// remove the ending linebreak
					while (gch->pszText[lstrlen(gch->pszText)-1] == '\r' || gch->pszText[lstrlen(gch->pszText)-1] == '\n') {
						gch->pszText[lstrlen(gch->pszText)-1] = '\0';
					}

					ccs.hContact = (HANDLE)-atoi(p);
					ccs.wParam = 0;
					ccs.lParam = (LPARAM)gch->pszText;
					CallProtoService(msnProtocolName, PSS_MESSAGE, (WPARAM)0, (LPARAM)&ccs);

					GCDEST gcd = {0};
					GCEVENT gce = {0};
					DBVARIANT dbv, dbv2;

					gcd.pszModule = msnProtocolName;
					gcd.pszID = p;
					gcd.iType = GC_EVENT_MESSAGE;

					gce.cbSize = sizeof(GCEVENT);
					gce.pDest = &gcd;
					DBGetContactSetting(NULL, msnProtocolName, "Nick", &dbv);
					gce.pszNick = dbv.pszVal;
					DBGetContactSetting(NULL, msnProtocolName, "e-mail", &dbv2);
					gce.pszUID = dbv2.pszVal;
					gce.time = time(NULL);
					gce.pszText = gch->pszText;
					gce.bAddToLog = TRUE;
					MSN_CallService(MS_GC_EVENT, NULL, (LPARAM)&gce);
					DBFreeVariant(&dbv);
					DBFreeVariant(&dbv2);
				}
				break;
			case GC_USER_CHANMGR:
				break;
			case GC_USER_PRIVMESS: {
				HANDLE hContact = MSN_HContactFromEmail((char*)gch->pszUID, NULL, 0, 0);
				MSN_CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
				break;
			}
			case GC_USER_LOGMENU:
				switch(gch->dwData) {
				case 10:
					KillChatSession(p, gch);
					break;
				}
				break;
			case GC_USER_NICKLISTMENU: {
				HANDLE hContact = MSN_HContactFromEmail((char*)gch->pszUID, NULL, 0, 0);

				switch(gch->dwData) {
				case 10:
					MSN_CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)hContact, 0);
					break;
				case 20:
					MSN_CallService(MS_HISTORY_SHOWCONTACTHISTORY, (WPARAM)hContact, 0);
					break;
				}
				break;
			}			
/*
				if(gch && gch->pszText && lstrlen(gch->pszText)> 0)
				{
					char * pszText = new char[lstrlen(gch->pszText)+1000];
					lstrcpy(pszText, gch->pszText);
					DoChatFormatting(pszText);
					PostIrcMessageWnd(p1, NULL, pszText);
					delete []pszText;
				}
				break;
			case GC_USER_CHANMGR:
				PostIrcMessageWnd(p1, NULL, "/CHANNELMANAGER");
				break;
			case GC_USER_PRIVMESS:
				char szTemp[4000];
				_snprintf(szTemp, sizeof(szTemp), "/QUERY %s", gch->pszUID);
				PostIrcMessageWnd(p1, NULL, szTemp);
				break;
			case GC_USER_LOGMENU:
				switch(gch->dwData)
				{
				case 1:
					PostIrcMessageWnd(p1, NULL, "/CHANNELMANAGER");
					break;
				case 2:
					PostIrcMessage( "/PART %s", p1);
					GCEVENT gce; 
					GCDEST gcd;
					S = MakeWndID(p1);
					gce.cbSize = sizeof(GCEVENT);
					gcd.iType = GC_EVENT_CONTROL;
					gcd.pszModule = IRCPROTONAME;
					gce.pDest = &gcd;
					gcd.pszID = (char *)S.c_str();
					MSN_CallService(MS_GC_EVENT, WINDOW_TERMINATE, (LPARAM)&gce);
					break;
				case 3:
					PostIrcMessageWnd(p1, NULL, "/SERVERSHOW");
					break;
				default:break;
				}break;
			case GC_USER_NICKLISTMENU:
				switch(gch->dwData)
				{
				case 1:
					PostIrcMessage( "/MODE %s +o %s", p1, gch->pszUID);
					break;
				case 2:
					PostIrcMessage( "/MODE %s -o %s", p1, gch->pszUID);
					break;
				case 3:
					PostIrcMessage( "/MODE %s +v %s", p1, gch->pszUID);
					break;
				case 4:
					PostIrcMessage( "/MODE %s -v %s", p1, gch->pszUID);
					break;
				case 5:
					PostIrcMessage( "/KICK %s %s", p1, gch->pszUID);
					break;
				case 6:
					PostIrcMessage( "/KICK %s %s %%question=\"%s\",\"%s\",\"%s\"", p1, gch->pszUID, Translate("Please enter the reason"), Translate("Kick"), Translate("Jerk") );
					break;
				case 7:
					AddToUHTemp("B" + (String)p1);
					_snprintf(szTemp, sizeof(szTemp), "USERHOST %s", gch->pszUID);
					if (g_ircSession)
						g_ircSession << CIrcMessage(szTemp, false, false);
					break;
				case 8:
					AddToUHTemp("K" + (String)p1);
					_snprintf(szTemp, sizeof(szTemp), "USERHOST %s", gch->pszUID);
					if (g_ircSession)
						g_ircSession << CIrcMessage(szTemp, false, false);
					break;
				case 9:
					AddToUHTemp("L" + (String)p1);
					_snprintf(szTemp, sizeof(szTemp), "USERHOST %s", gch->pszUID);
					if (g_ircSession)
						g_ircSession << CIrcMessage(szTemp, false, false);
					break;
				case 10:
					PostIrcMessage( "/WHOIS %s", gch->pszUID);
					break;
				case 11:
					AddToUHTemp("I");
					_snprintf(szTemp, sizeof(szTemp), "USERHOST %s", gch->pszUID);
					if (g_ircSession)
						g_ircSession << CIrcMessage(szTemp, false, false);
					break;
				case 12:
					AddToUHTemp("J");
					_snprintf(szTemp, sizeof(szTemp), "USERHOST %s", gch->pszUID);
					if (g_ircSession)
						g_ircSession << CIrcMessage(szTemp, false, false);
					break;
				case 13:
					PostIrcMessage( "/DCC CHAT %s", gch->pszUID);
					break;
				case 14:
					PostIrcMessage( "/DCC SEND %s", gch->pszUID);
					break;
				case 30:
					{
					PROTOSEARCHRESULT psr;
					ZeroMemory(&psr, sizeof(psr));
					psr.cbSize = sizeof(psr);
					psr.nick = (char *)gch->pszUID;
					ADDCONTACTSTRUCT acs;
					ZeroMemory(&acs, sizeof(acs));
					acs.handleType =HANDLE_SEARCHRESULT;
					acs.szProto = IRCPROTONAME;
					acs.psr = &psr;
					MSN_CallService(MS_ADDCONTACT_SHOW, (WPARAM)NULL, (LPARAM)&acs);
					}break;
					break;
				default:
					break;
				}
*/
				break;
			default:
				break;
			}
			delete[]p;
		}

	}
	return 0;
}

int MSN_GCMenuHook(WPARAM wParam,LPARAM lParam) {
	GCMENUITEMS *gcmi= (GCMENUITEMS*) lParam;

	if(gcmi) {
		if (!lstrcmpi(gcmi->pszModule, msnProtocolName)) {
/*
			if(gcmi->Type == MENU_ON_LOG) {
				if(lstrcmpi(gcmi->pszID, "Network log"))
				{
					static struct gc_item Item[] = {
							{Translate("Channel &settings"), 1, MENU_ITEM, FALSE},
							{Translate("&Leave the channel"), 2, MENU_ITEM, FALSE},
							{Translate("Show the server &window"), 3, MENU_ITEM, FALSE}};
					gcmi->nItems = sizeof(Item)/sizeof(Item[0]);
					gcmi->Item = &Item[0];
				}
				else
					gcmi->nItems = 0;
			}
*/
			if(gcmi->Type == MENU_ON_LOG) {
				static struct gc_item Item[] = {
						{Translate("&Leave chat session"), 10, MENU_ITEM, FALSE}
				};
				gcmi->nItems = sizeof(Item)/sizeof(Item[0]);
				gcmi->Item = &Item[0];
			}
			if(gcmi->Type == MENU_ON_NICKLIST) {
				static struct gc_item Item[] = {
						{Translate("User &details"), 10, MENU_ITEM, FALSE},
						{Translate("User &history"), 20, MENU_ITEM, FALSE}
				};
				gcmi->nItems = sizeof(Item)/sizeof(Item[0]);
				gcmi->Item = &Item[0];
			}
		}
	}
	return 0;
}
