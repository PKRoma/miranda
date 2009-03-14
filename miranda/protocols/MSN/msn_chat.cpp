/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2009 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "msn_global.h"
#include "msn_proto.h"
#include <m_history.h>

static LONG sttChatID = 0;

HANDLE CMsnProto::MSN_GetChatInernalHandle(HANDLE hContact)
{
	HANDLE result = hContact;
	int type = getByte(hContact, "ChatRoom", 0);
	if (type != 0) 
	{
		DBVARIANT dbv;
		if (getString(hContact, "ChatRoomID", &dbv) == 0)
		{
			result = (HANDLE)(-atol(dbv.pszVal));
			MSN_FreeVariant(&dbv);
		}	
	}
	return result;
}


int CMsnProto::MSN_ChatInit( WPARAM wParam, LPARAM )
{
	ThreadData *info = (ThreadData*)wParam;
	MyInterlockedIncrement( &sttChatID );
	_ltot( sttChatID, info->mChatID, 10 );

	info->mJoinedContacts = ( HANDLE* )mir_realloc(info->mJoinedContacts, sizeof(HANDLE)*(++info->mJoinedCount));
	info->mJoinedContacts[info->mJoinedCount - 1] = info->mJoinedContacts[0];
	info->mJoinedContacts[0] = ( HANDLE )-sttChatID;

	TCHAR szName[ 512 ];
	mir_sntprintf( szName, SIZEOF( szName ), _T(TCHAR_STR_PARAM) _T(" %s%s"), 
		m_szModuleName, TranslateT( "Chat #" ), info->mChatID );

	GCSESSION gcw = {0};
	gcw.cbSize = sizeof(GCSESSION);
	gcw.dwFlags = GC_TCHAR;
	gcw.iType = GCW_CHATROOM;
	gcw.pszModule = m_szModuleName;
	gcw.ptszName = szName;
	gcw.ptszID = info->mChatID;
	CallServiceSync( MS_GC_NEWSESSION, 0, (LPARAM)&gcw );

	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_ADDGROUP };
	gcd.ptszID = info->mChatID;
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
	gce.ptszStatus = TranslateT("Me");
	CallServiceSync( MS_GC_EVENT, 0, (LPARAM)&gce );

	DBVARIANT dbv;
	int bError = getTString( "Nick", &dbv );
    gce.ptszNick = bError ? _T("") : dbv.ptszVal;

	gcd.iType = GC_EVENT_JOIN;
	gce.ptszUID = mir_a2t(MyOptions.szEmail);
	gce.time = 0;
	gce.bIsMe = TRUE;
	CallServiceSync( MS_GC_EVENT, 0, (LPARAM)&gce );

	gcd.iType = GC_EVENT_ADDGROUP;
	gce.ptszStatus = TranslateT("Others");
	CallServiceSync( MS_GC_EVENT, 0, (LPARAM)&gce );

	gcd.iType = GC_EVENT_CONTROL;
	CallServiceSync( MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gce );
	CallServiceSync( MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce );
	CallServiceSync( MS_GC_EVENT, WINDOW_VISIBLE, (LPARAM)&gce );

	if ( !bError )
		MSN_FreeVariant( &dbv );
	mir_free((TCHAR*)gce.ptszUID );
	return 0;
}

void CMsnProto::MSN_ChatStart(ThreadData* info)
{
	if ( info->mChatID[0] != 0 )
		return;

	MSN_StartStopTyping(info, false);

	NotifyEventHooks( hInitChat, (WPARAM)info, 0 );

	// add all participants onto the list
	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_JOIN };
	gcd.ptszID = info->mChatID;

	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
	gce.pDest = &gcd;
	gce.ptszStatus = TranslateT("Others");
	gce.time = time(NULL);
	gce.bIsMe = FALSE;

	for ( int j=0; j < info->mJoinedCount; j++ ) {
		if (( long )info->mJoinedContacts[j] <= 0 )
			continue;

		gce.ptszNick = MSN_GetContactNameT( info->mJoinedContacts[j] );

		DBVARIANT dbv;
		if ( !getTString( info->mJoinedContacts[j], "e-mail", &dbv )) {
			gce.ptszUID = dbv.ptszVal;
			CallServiceSync( MS_GC_EVENT, 0, ( LPARAM )&gce );
}	}	}

void CMsnProto::MSN_KillChatSession( TCHAR* id )
{
	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_CONTROL };
	gcd.ptszID = id;
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
	CallServiceSync( MS_GC_EVENT, SESSION_OFFLINE, (LPARAM)&gce );
	CallServiceSync( MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce );
}

void CMsnProto::InviteUser(ThreadData* info) {
	HMENU tMenu = ::CreatePopupMenu();
	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );

	// add the heading
	::AppendMenu(tMenu, MF_STRING|MF_GRAYED|MF_DISABLED, (UINT_PTR)0, TranslateT("&Invite user..."));
	::AppendMenu(tMenu, MF_SEPARATOR, (UINT_PTR)1, NULL);

	// generate a list of contact
	while ( hContact != NULL ) {
		if ( MSN_IsMyContact( hContact )) {
			if (getByte(hContact, "ChatRoom", 0) == 0) {
				if (getWord(hContact, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
					bool alreadyInSession = false;
					for ( int j=0; j < info->mJoinedCount; j++ ) {
						if (info->mJoinedContacts[j] == hContact) {
							alreadyInSession = true;
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
	if ( !getStaticString( hInvitedUser, "e-mail", tEmail, sizeof( tEmail ))) {
		info->sendPacket( "CAL", tEmail );
		MSN_ChatStart(info);
}	}

int CMsnProto::MSN_GCEventHook(WPARAM, LPARAM lParam) 
{
	GCHOOK *gch = (GCHOOK*) lParam;
	if ( !gch )
		return 1;

	if ( _stricmp(gch->pDest->pszModule, m_szModuleName )) return 0;

	HANDLE hChatContact = (HANDLE)-_ttoi( gch->pDest->ptszID );

    switch (gch->pDest->iType) {
		case GC_SESSION_TERMINATE: {
			ThreadData* thread = MSN_GetThreadByContact(hChatContact);
			if ( thread != NULL ) {
				// open up srmm dialog when quit while 1 person left
				if ( thread->mJoinedCount == 1 ) {
					// switch back to normal session
					thread->mJoinedContacts[0] = thread->mJoinedContacts[1];
					thread->mJoinedContacts = ( HANDLE* )mir_realloc( thread->mJoinedContacts, sizeof( HANDLE ) );
					MSN_CallService(MS_MSG_SENDMESSAGE, (WPARAM)thread->mJoinedContacts[0], 0);
					thread->mChatID[0] = 0;
				}
				else thread->sendPacket( "OUT", NULL );
			}
			break;
		}
		case GC_USER_MESSAGE:
			if ( gch->pszText && strlen( gch->pszText ) > 0 ) 
			{
				bool isOffline;
				ThreadData* thread = MSN_StartSB(hChatContact, isOffline);

				if (thread)
				{
					rtrim( gch->ptszText ); // remove the ending linebreak
					TCHAR* pszMsg = UnEscapeChatTags( NEWTSTR_ALLOCA( gch->ptszText ));
					char* msg = mir_utf8encodeT(pszMsg);

					thread->sendMessage( 'N', NULL, NETID_MSN, msg, 0 );

					mir_free(msg);

					DBVARIANT dbv;
					int bError = getTString( "Nick", &dbv );

					GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_MESSAGE };
					gcd.ptszID = gch->pDest->ptszID;

					GCEVENT gce = {0};
					gce.cbSize = sizeof(GCEVENT);
					gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
					gce.pDest = &gcd;
                    gce.ptszNick = bError ? _T("") : dbv.ptszVal;
					gce.ptszUID = mir_a2t(MyOptions.szEmail);
					gce.time = time(NULL);
					gce.ptszText = gch->ptszText;
					gce.bIsMe = TRUE;
					CallServiceSync( MS_GC_EVENT, 0, (LPARAM)&gce );

					mir_free((void*)gce.ptszUID);
					if ( !bError )
						MSN_FreeVariant( &dbv );
				}
			}
			break;
		case GC_USER_CHANMGR: {
			ThreadData* thread = MSN_GetThreadByContact(hChatContact);
			if ( thread != NULL ) {
				InviteUser(thread);
			}
			break;
		}
		case GC_USER_PRIVMESS: {
			char *email = mir_t2a(gch->ptszUID);
			HANDLE hContact = MSN_HContactFromEmail(email, NULL, false, false);
			MSN_CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
			mir_free(email);
			break;
		}
		case GC_USER_LOGMENU:
			switch(gch->dwData) {
			case 10: {
				ThreadData* thread = MSN_GetThreadByContact(hChatContact);
				if ( thread != NULL )
					InviteUser( thread );

				break;
			}
			case 20:
				MSN_KillChatSession( gch->pDest->ptszID );
				break;
			}
			break;
		case GC_USER_NICKLISTMENU: {
			char *email = mir_t2a(gch->ptszUID);
			HANDLE hContact = MSN_HContactFromEmail( email, email, false, false );
			mir_free(email);

			switch(gch->dwData) {
			case 10:
				MSN_CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)hContact, 0);
				break;
			case 20:
				MSN_CallService(MS_HISTORY_SHOWCONTACTHISTORY, (WPARAM)hContact, 0);
				break;
			case 110:
				MSN_KillChatSession( gch->pDest->ptszID );
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
	}

	return 0;
}

int CMsnProto::MSN_GCMenuHook(WPARAM, LPARAM lParam) 
{
	GCMENUITEMS *gcmi= (GCMENUITEMS*) lParam;

	if ( gcmi == NULL || _stricmp(gcmi->pszModule, m_szModuleName )) return 0;

	if ( gcmi->Type == MENU_ON_LOG ) 
    {
		static const struct gc_item Items[] = {
			{ TranslateT("&Invite user..."), 10, MENU_ITEM, FALSE },
			{ TranslateT("&Leave chat session"), 20, MENU_ITEM, FALSE }
		};
		gcmi->nItems = SIZEOF(Items);
		gcmi->Item = (gc_item*)Items;
	}
	else if ( gcmi->Type == MENU_ON_NICKLIST ) 
    {
        char* email = mir_t2a(gcmi->pszUID);
		if ( !strcmp(MyOptions.szEmail, email)) 
        {
			static const struct gc_item Items[] = {
				{ TranslateT("User &details"), 10, MENU_ITEM, FALSE },
				{ TranslateT("User &history"), 20, MENU_ITEM, FALSE },
				{ _T(""), 100, MENU_SEPARATOR, FALSE },
				{ TranslateT("&Leave chat session"), 110, MENU_ITEM, FALSE }
			};
			gcmi->nItems = SIZEOF(Items);
			gcmi->Item = (gc_item*)Items;
		}
		else 
        {
			static const struct gc_item Items[] = {
				{ TranslateT("User &details"), 10, MENU_ITEM, FALSE },
				{ TranslateT("User &history"), 20, MENU_ITEM, FALSE }
			};
			gcmi->nItems = SIZEOF(Items);
			gcmi->Item = (gc_item*)Items;
        }
        mir_free(email);
    }

	return 0;
}
