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
#include "../../include/m_history.h"

static LONG sttChatID = 0;
extern HANDLE hInitChat;

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
	char tEmail[ MSN_MAX_EMAIL_LEN ], tNick[ 1024 ];
	mir_snprintf( szName, sizeof( szName ), "%s%s", Translate("MSN Chat #"), info->mChatID );

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
	gce.pszStatus = Translate("Me");
	MSN_CallService(MS_GC_EVENT, NULL, (LPARAM)&gce);

	gcd.iType = GC_EVENT_JOIN;
	gce.pszStatus, Translate("Me");
	MSN_GetStaticString( "Nick", NULL, tNick, sizeof tNick );
	gce.pszNick = tNick;
	MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof tEmail );
	gce.pszUID = tEmail;
	gce.time = 0;
	gce.bIsMe = TRUE;
	gce.bAddToLog = FALSE;
	MSN_CallService(MS_GC_EVENT, NULL, (LPARAM)&gce);

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

void MSN_ChatStart(ThreadData* info) {
	if ( info->mChatID[0] == 0 ) {
		NotifyEventHooks( hInitChat, (WPARAM)info, 0 );

		// add all participants onto the list
		GCDEST gcd = {0};
		gcd.pszModule = msnProtocolName;
		gcd.pszID = info->mChatID;
		gcd.iType = GC_EVENT_JOIN;

		GCEVENT gce = {0};
		gce.cbSize = sizeof(GCEVENT);
		gce.pDest = &gcd;
		gce.pszStatus = Translate("Others");
		gce.time = time(NULL);
		gce.bIsMe = FALSE;
		gce.bAddToLog = TRUE;

		for ( int j=0; j < info->mJoinedCount; j++ ) {
			if (( long )info->mJoinedContacts[j] > 0 ) {
				gce.pszNick = MSN_GetContactName( info->mJoinedContacts[j] );

				char tEmail[ MSN_MAX_EMAIL_LEN ];
				if ( !MSN_GetStaticString( "e-mail", info->mJoinedContacts[j], tEmail, sizeof tEmail )) {
					gce.pszUID = tEmail;
					MSN_CallService( MS_GC_EVENT, NULL, ( LPARAM )&gce );
	}	}	}	}
}

void KillChatSession(char* id, GCHOOK* gch) {
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

void InviteUser(ThreadData* info) {
	HMENU tMenu = ::CreatePopupMenu();
	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );

	// add the heading
	::AppendMenu(tMenu, MF_STRING|MF_GRAYED|MF_DISABLED, (UINT_PTR)0, TranslateT("&Invite user..."));
	::AppendMenu(tMenu, MF_SEPARATOR, (UINT_PTR)1, NULL);

	// generate a list of contact
	while ( hContact != NULL ) {
		if ( !lstrcmpA( msnProtocolName, ( char* )MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact,0 ))) {
			if (DBGetContactSettingByte(hContact, msnProtocolName, "ChatRoom", 0) == 0) {
				if (MSN_GetWord(hContact, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
					BOOL alreadyInSession = FALSE;
					for ( int j=0; j < info->mJoinedCount; j++ ) {
						if (info->mJoinedContacts[j] == hContact) {
							alreadyInSession = TRUE;
							break;
						}
					}
					if (!alreadyInSession)
						::AppendMenu(tMenu, MF_STRING, (UINT_PTR)hContact, MSN_GetContactNameT(hContact));
				}
		}	}
		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT,( WPARAM )hContact, 0 );
	}

	HWND tWindow = CreateWindow(_T("EDIT"),_T(""),0,1,1,1,1,NULL,NULL,hInst,NULL);

	POINT pt;
	::GetCursorPos ( &pt );
	HANDLE hInvitedUser = (HANDLE)::TrackPopupMenu( tMenu, TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, tWindow, NULL );
	::DestroyMenu( tMenu );
	::DestroyWindow( tWindow );

	if ( !hInvitedUser )
		return;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( !MSN_GetStaticString( "e-mail", ( HANDLE )hInvitedUser, tEmail, sizeof( tEmail ))) {
		info->sendPacket( "CAL", tEmail );
		MSN_ChatStart(info);
}	}

int MSN_GCEventHook(WPARAM wParam,LPARAM lParam) {
	GCHOOK *gch = (GCHOOK*) lParam;
	char S[512] = "";

	if(gch) {
		if (!lstrcmpiA(gch->pDest->pszModule, msnProtocolName)) {
			char *p = new char[lstrlenA(gch->pDest->pszID)+1];
			lstrcpyA(p, gch->pDest->pszID);
			switch (gch->pDest->iType) {
			case GC_USER_TERMINATE: {
				int chatID = atoi( p );
				ThreadData* thread = MSN_GetThreadByContact((HANDLE)-chatID);
				if ( thread != NULL ) {
					// open up srmm dialog when quit while 1 person left
					if ( thread->mJoinedCount == 1 ) {
						// switch back to normal session
						thread->mJoinedContacts[0] = thread->mJoinedContacts[1];
						thread->mJoinedContacts = ( HANDLE* )realloc( thread->mJoinedContacts, sizeof( HANDLE ) );
						MSN_CallService(MS_MSG_SENDMESSAGE, (WPARAM)thread->mJoinedContacts[0], 0);
						thread->mChatID[0] = 0;
					}
					else thread->sendPacket( "OUT", NULL );
				}
				break;
			}
			case GC_USER_MESSAGE:
				if ( gch && gch->pszText && lstrlenA( gch->pszText ) > 0 ) {
					rtrim( gch->pszText ); // remove the ending linebreak

					{	char* pszMsg = UnEscapeChatTags(gch->pszText);
						
						CCSDATA ccs = {0};
						ccs.hContact = (HANDLE)-atoi(p);
						ccs.wParam = 0;
						ccs.lParam = (LPARAM)pszMsg;
						CallProtoService(msnProtocolName, PSS_MESSAGE, (WPARAM)0, (LPARAM)&ccs);
						free( pszMsg );
					}

					GCDEST gcd = {0};
					GCEVENT gce = {0};

					gcd.pszModule = msnProtocolName;
					gcd.pszID = p;
					gcd.iType = GC_EVENT_MESSAGE;

					char tEmail[ MSN_MAX_EMAIL_LEN ], tNick[ 1024 ];
					gce.cbSize = sizeof(GCEVENT);
					gce.pDest = &gcd;
					MSN_GetStaticString( "Nick", NULL, tNick, sizeof tNick );
					gce.pszNick = tNick;
					MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof tEmail );
					gce.pszUID = tEmail;
					gce.time = time(NULL);
					gce.pszText = gch->pszText;
					gce.bAddToLog = TRUE;
					gce.bIsMe = TRUE;
					MSN_CallService(MS_GC_EVENT, NULL, (LPARAM)&gce);
				}
				break;
			case GC_USER_CHANMGR: {
				int chatID = atoi(p);
				ThreadData* thread = MSN_GetThreadByContact((HANDLE)-chatID);
				if ( thread != NULL ) {
					InviteUser(thread);
				}
				break;
			}
			case GC_USER_PRIVMESS: {
				HANDLE hContact = MSN_HContactFromEmail((char*)gch->pszUID, NULL, 0, 0);
				MSN_CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
				break;
			}
			case GC_USER_LOGMENU:
				switch(gch->dwData) {
				case 10: {
					int chatID = atoi(p);
					ThreadData* thread = MSN_GetThreadByContact((HANDLE)-chatID);
					if ( thread != NULL ) {
						InviteUser(thread);
					}
					break;
				}
				case 20:
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
				case 110:
					KillChatSession(p, gch);
					break;
				}
				break;
			}
/*	haven't implemented in chat.dll
			case GC_USER_TYPNOTIFY: {
				int chatID = atoi(p);
				ThreadData* thread = MSN_GetThreadByContact((HANDLE)-chatID);
				for ( int j=0; j < thread->mJoinedCount; j++ ) {
					if (( long )thread->mJoinedContacts[j] > 0 )
						CallService(MS_PROTO_SELFISTYPING, (WPARAM) thread->mJoinedContacts[j], (LPARAM) PROTOTYPE_SELFTYPING_ON);
				}
					break;
				}
*/
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

	if ( gcmi ) {
		if (!lstrcmpiA(gcmi->pszModule, msnProtocolName)) {
			if(gcmi->Type == MENU_ON_LOG) {
				static struct gc_item Item[] = {
					{Translate("&Invite user..."), 10, MENU_ITEM, FALSE},
					{Translate("&Leave chat session"), 20, MENU_ITEM, FALSE}
				};
				gcmi->nItems = sizeof(Item)/sizeof(Item[0]);
				gcmi->Item = &Item[0];
			}
			if(gcmi->Type == MENU_ON_NICKLIST) {
				char tEmail[ MSN_MAX_EMAIL_LEN ];
				MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof tEmail );
				if (!lstrcmpA(tEmail, (char *)gcmi->pszUID)) {
					static struct gc_item Item[] = {
						{Translate("User &details"), 10, MENU_ITEM, FALSE},
						{Translate("User &history"), 20, MENU_ITEM, FALSE},
						{Translate(""), 100, MENU_SEPARATOR, FALSE},
						{Translate("&Leave chat session"), 110, MENU_ITEM, FALSE}
					};
					gcmi->nItems = sizeof(Item)/sizeof(Item[0]);
					gcmi->Item = &Item[0];
				}
				else {
					static struct gc_item Item[] = {
						{Translate("User &details"), 10, MENU_ITEM, FALSE},
						{Translate("User &history"), 20, MENU_ITEM, FALSE}
					};
					gcmi->nItems = sizeof(Item)/sizeof(Item[0]);
					gcmi->Item = &Item[0];
	}	}	}	}

	return 0;
}
