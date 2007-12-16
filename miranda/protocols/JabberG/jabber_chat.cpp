/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_chat.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_iq.h"
#include "resource.h"
#include "jabber_caps.h"

extern HANDLE hInitChat;

/////////////////////////////////////////////////////////////////////////////////////////
// One string entry dialog

struct JabberEnterStringParams
{
	TCHAR* caption;
	TCHAR* result;
	size_t resultLen;
};

static int sttEnterStringResizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch (urc->wId)
	{
	case IDC_TOPIC:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
	case IDOK:
	case IDCANCEL:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

BOOL CALLBACK JabberEnterStringDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
	{
		//SetWindowPos( hwndDlg, HWND_TOPMOST ,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE );
		TranslateDialogDefault( hwndDlg );
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_OTHER_RENAME));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadSkinnedIcon(SKINICON_OTHER_RENAME));
		JabberEnterStringParams* params = ( JabberEnterStringParams* )lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )params );
		SetWindowText( hwndDlg, params->caption );
		SetDlgItemText( hwndDlg, IDC_TOPIC, params->result );
		SetTimer(hwndDlg, 1000, 50, NULL);
		return TRUE;
	}
	case WM_TIMER:
	{
		KillTimer(hwndDlg,1000);
		EnableWindow(GetParent(hwndDlg), TRUE);
		return TRUE;
	}
	case WM_SIZE:
	{
		UTILRESIZEDIALOG urd = {0};
		urd.cbSize = sizeof(urd);
		urd.hInstance = hInst;
		urd.hwndDlg = hwndDlg;
		urd.lpTemplate = MAKEINTRESOURCEA(IDD_GROUPCHAT_INPUT);
		urd.pfnResizer = sttEnterStringResizer;
		CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
		break;
	}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
		{	JabberEnterStringParams* params = ( JabberEnterStringParams* )GetWindowLong( hwndDlg, GWL_USERDATA );
			GetDlgItemText( hwndDlg, IDC_TOPIC, params->result, params->resultLen );
			params->result[ params->resultLen-1 ] = 0;
			EndDialog( hwndDlg, 1 );
			break;
		}
		case IDCANCEL:
			EndDialog( hwndDlg, 0 );
			break;
	}	}

	return FALSE;
}

BOOL JabberEnterString( TCHAR* caption, TCHAR* result, size_t resultLen )
{
	JabberEnterStringParams params = { caption, result, resultLen };
	return DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_INPUT ), GetForegroundWindow(), JabberEnterStringDlgProc, LPARAM( &params ));
}

BOOL JabberEnterString( TCHAR* result, size_t resultLen )
{
	TCHAR *szCaption = mir_tstrdup( result );
	result[ 0 ] = _T('\0');
	JabberEnterStringParams params = { szCaption, result, resultLen };
	BOOL bRetVal = DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_INPUT ), GetForegroundWindow(), JabberEnterStringDlgProc, LPARAM( &params ));
	mir_free( szCaption );
	return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Global definitions

enum {
	IDM_CANCEL,

	IDM_ROLE, IDM_AFFLTN,

	IDM_CONFIG, IDM_NICK, IDM_DESTROY, IDM_INVITE, IDM_BOOKMARKS, IDM_LEAVE, IDM_TOPIC,
	IDM_LST_PARTICIPANT, IDM_LST_MODERATOR,
	IDM_LST_MEMBER, IDM_LST_ADMIN, IDM_LST_OWNER, IDM_LST_BAN,

	IDM_MESSAGE, IDM_VCARD, IDM_INFO, IDM_KICK,
	IDM_SET_VISITOR, IDM_SET_PARTICIPANT, IDM_SET_MODERATOR,
	IDM_SET_NONE, IDM_SET_MEMBER, IDM_SET_ADMIN, IDM_SET_OWNER, IDM_SET_BAN,
	IDM_CPY_NICK, IDM_CPY_TOPIC, IDM_CPY_RJID,

};

struct TRoleOrAffiliationInfo
{
	int value;
	int id;
	TCHAR *title_en;
	TCHAR *title_active_en;
	int min_role;
	int min_affiliation;

	TCHAR *title;
	TCHAR *title_active;

	BOOL check(JABBER_RESOURCE_STATUS *me, JABBER_RESOURCE_STATUS *him)
	{
		if (me->affiliation == AFFILIATION_OWNER) return TRUE;
		if (me == him) return FALSE;
		if (me->affiliation <= him->affiliation) return FALSE;
		if (me->role < this->min_role) return FALSE;
		if (me->affiliation < this->min_affiliation) return FALSE;
		return TRUE;
	}
	void translate()
	{
		this->title = TranslateTS(this->title_en);
		this->title_active = TranslateTS(this->title_active_en);
	}
};

static TRoleOrAffiliationInfo sttAffiliationItems[] =
{
	{ AFFILIATION_NONE,		IDM_SET_NONE,			LPGENT("None"),			LPGENT("None *"),			ROLE_NONE,		AFFILIATION_ADMIN	},
	{ AFFILIATION_MEMBER,	IDM_SET_MEMBER,			LPGENT("Member"),		LPGENT("Member *"),			ROLE_NONE,		AFFILIATION_ADMIN	},
	{ AFFILIATION_ADMIN,	IDM_SET_ADMIN,			LPGENT("Admin"),		LPGENT("Admin *"),			ROLE_NONE,		AFFILIATION_OWNER	},
	{ AFFILIATION_OWNER,	IDM_SET_OWNER,			LPGENT("Owner"),		LPGENT("Owner *"),			ROLE_NONE,		AFFILIATION_OWNER	},
};

static TRoleOrAffiliationInfo sttRoleItems[] =
{
	{ ROLE_VISITOR,			IDM_SET_VISITOR,		LPGENT("Visitor"),		LPGENT("Visitor *"),		ROLE_MODERATOR,	AFFILIATION_NONE	},
	{ ROLE_PARTICIPANT,		IDM_SET_PARTICIPANT,	LPGENT("Participant"),	LPGENT("Participant *"),	ROLE_MODERATOR,	AFFILIATION_NONE	},
	{ ROLE_MODERATOR,		IDM_SET_MODERATOR,		LPGENT("Moderator"),	LPGENT("Moderator *"),		ROLE_MODERATOR,	AFFILIATION_ADMIN	},
};

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGcInit - initializes the new chat

static TCHAR* sttRoles[] = { _T("Other"), _T("Visitors"), _T("Participants"), _T("Moderators") };

int JabberGcInit( WPARAM wParam, LPARAM lParam )
{
	int i;
	JABBER_LIST_ITEM* item = ( JABBER_LIST_ITEM* )wParam;
	GCSESSION gcw = {0};
	GCEVENT gce = {0};

	// translate string for menus (this can't be done in initializer)
	for (i = 0; i < SIZEOF(sttAffiliationItems); ++i) sttAffiliationItems[i].translate();
	for (i = 0; i < SIZEOF(sttRoleItems); ++i) sttRoleItems[i].translate();

	TCHAR* szNick = JabberNickFromJID( item->jid );
	gcw.cbSize = sizeof(GCSESSION);
	gcw.iType = GCW_CHATROOM;
	gcw.pszModule = jabberProtoName;
	gcw.ptszName = szNick;
	gcw.ptszID = item->jid;
	gcw.dwFlags = GC_TCHAR;
	CallServiceSync( MS_GC_NEWSESSION, NULL, (LPARAM)&gcw );

	HANDLE hContact = JabberHContactFromJID( item->jid );
	if ( hContact != NULL ) {
		DBVARIANT dbv;
		if ( !DBGetContactSettingTString( hContact, jabberProtoName, "MyNick", &dbv )) {
			if ( !lstrcmp( dbv.ptszVal, szNick ))
				JDeleteSetting( hContact, "MyNick" );
			else
				JSetStringT( hContact, "MyNick", item->nick );
			JFreeVariant( &dbv );
		}
		else JSetStringT( hContact, "MyNick", item->nick );
	}
	mir_free( szNick );

	item->bChatActive = TRUE;

	GCDEST gcd = { jabberProtoName, NULL, GC_EVENT_ADDGROUP };
	gcd.ptszID = item->jid;
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	gce.dwFlags = GC_TCHAR;
	for (i = SIZEOF(sttRoles)-1; i >= 0; i-- ) {
		gce.ptszStatus = TranslateTS( sttRoles[i] );
		CallServiceSync( MS_GC_EVENT, NULL, ( LPARAM )&gce );
	}

	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	gcd.iType = GC_EVENT_CONTROL;
	CallServiceSync( MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gce );
	CallServiceSync( MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce );
	return 0;
}

void JabberGcLogCreate( JABBER_LIST_ITEM* item )
{
	if ( item->bChatActive )
		return;

	NotifyEventHooks( hInitChat, (WPARAM)item, 0 );
}

void JabberGcLogUpdateMemberStatus( JABBER_LIST_ITEM* item, TCHAR* nick, TCHAR* jid, int action, XmlNode* reason, int nStatusCode )
{
	int statusToSet = 0;
	TCHAR* szReason = NULL;
	if ( reason != NULL && reason->text != NULL )
		szReason = reason->text;

	if ( !szReason ) {
		if ( nStatusCode == 322 )
			szReason = TranslateT( "because room is now members-only" );
		else if ( nStatusCode == 301 )
			szReason = TranslateT( "user banned" );
	}

	TCHAR* myNick = (item->nick == NULL) ? NULL : mir_tstrdup( item->nick );
	if ( myNick == NULL )
		myNick = JabberNickFromJID( jabberJID );

	GCDEST gcd = { jabberProtoName, 0, 0 };
	gcd.ptszID = item->jid;
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.ptszNick = nick;
	gce.ptszUID = nick;
	if (jid != NULL)
		gce.ptszUserInfo = jid;
	gce.ptszText = szReason;
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
 	if ( item->bChatActive == 2 ) {
		gce.dwFlags |= GCEF_ADDTOLOG;
		gce.time = time(0);
	}

	switch( gcd.iType = action ) {
	case GC_EVENT_PART:  break;
	case GC_EVENT_KICK:
		gce.ptszStatus = TranslateT( "Moderator" );
		break;
	default:
		for ( int i=0; i < item->resourceCount; i++ ) {
			JABBER_RESOURCE_STATUS& JS = item->resource[i];
			if ( !lstrcmp( nick, JS.resourceName )) {
				if ( action != GC_EVENT_JOIN ) {
					switch( action ) {
					case 0:
						gcd.iType = GC_EVENT_ADDSTATUS;
					case GC_EVENT_REMOVESTATUS:
						gce.dwFlags &= ~GCEF_ADDTOLOG;
					}
					gce.ptszText = TranslateT( "Moderator" );
				}
				gce.ptszStatus = TranslateTS( sttRoles[ JS.role ] );
				gce.bIsMe = ( lstrcmp( nick, myNick ) == 0 );
				statusToSet = JS.status;
				break;
	}	}	}

	CallServiceSync( MS_GC_EVENT, NULL, ( LPARAM )&gce );

	if ( statusToSet != 0 ) {
		gce.ptszText = nick;
		if ( statusToSet == ID_STATUS_AWAY || statusToSet == ID_STATUS_NA || statusToSet == ID_STATUS_DND )
			gce.dwItemData = 3;
		else
			gce.dwItemData = 1;
		gcd.iType = GC_EVENT_SETSTATUSEX;
		CallServiceSync( MS_GC_EVENT, NULL, ( LPARAM )&gce );

		gce.ptszUID = nick;
		gce.dwItemData = statusToSet;
		gcd.iType = GC_EVENT_SETCONTACTSTATUS;
		CallServiceSync( MS_GC_EVENT, NULL, ( LPARAM )&gce );
	}

	mir_free( myNick );
}

void JabberGcQuit( JABBER_LIST_ITEM* item, int code, XmlNode* reason )
{
	TCHAR* szReason = NULL;
	if ( reason != NULL && reason->text != NULL )
		szReason = reason->text;

	GCDEST gcd = { jabberProtoName, NULL, GC_EVENT_CONTROL };
	gcd.ptszID = item->jid;
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.ptszUID = item->jid;
	gce.ptszText = szReason;
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;

	if ( code != 307 ) {
		CallServiceSync( MS_GC_EVENT, SESSION_TERMINATE, ( LPARAM )&gce );
		CallServiceSync( MS_GC_EVENT, WINDOW_CLEARLOG, ( LPARAM )&gce );
	}
	else {
		TCHAR* myNick = JabberNickFromJID( jabberJID );
		JabberGcLogUpdateMemberStatus( item, myNick, NULL, GC_EVENT_KICK, reason );
		mir_free( myNick );
		CallServiceSync( MS_GC_EVENT, SESSION_OFFLINE, ( LPARAM )&gce );
	}

	DBDeleteContactSetting( JabberHContactFromJID( item->jid ), "CList", "Hidden" );
	item->bChatActive = FALSE;

	if ( jabberOnline ) {
		XmlNode p( "presence" ); p.addAttr( "to", item->jid ); p.addAttr( "type", "unavailable" );
		jabberThreadInfo->send( p );
		JabberListRemove( LIST_CHATROOM, item->jid );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// Context menu hooks

static struct gc_item *sttFindGcMenuItem(GCMENUITEMS *items, DWORD id)
{
	for (int i = 0; i < items->nItems; ++i)
		if (items->Item[i].dwID == id)
			return items->Item + i;
	return NULL;
}

static void sttSetupGcMenuItem(GCMENUITEMS *items, DWORD id, bool disabled)
{
	for (int i = 0; i < items->nItems; ++i)
		if (!id || (items->Item[i].dwID == id))
			items->Item[i].bDisabled = disabled;
}

static void sttSetupGcMenuItems(GCMENUITEMS *items, DWORD *ids, bool disabled)
{
	for ( ; *ids; ++ids)
		sttSetupGcMenuItem(items, *ids, disabled);
}

int JabberGcMenuHook( WPARAM wParam, LPARAM lParam )
{
	GCMENUITEMS* gcmi= ( GCMENUITEMS* )lParam;
	if ( gcmi == NULL )
		return 0;

	if ( lstrcmpiA( gcmi->pszModule, jabberProtoName ))
		return 0;

	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_CHATROOM, gcmi->pszID );
	if ( item == NULL )
		return 0;

	JABBER_RESOURCE_STATUS *me = NULL, *him = NULL;
	for ( int i=0; i < item->resourceCount; i++ ) {
		JABBER_RESOURCE_STATUS& p = item->resource[i];
		if ( !lstrcmp( p.resourceName, item->nick   ))  me = &p;
		if ( !lstrcmp( p.resourceName, gcmi->pszUID ))  him = &p;
	}

	if ( gcmi->Type == MENU_ON_LOG )
	{
		static struct gc_item sttLogListItems[] = 
		{
			{ TranslateT("Change &nickname"),		IDM_NICK,				MENU_ITEM			},
			{ TranslateT("&Invite a user"),			IDM_INVITE,				MENU_ITEM			},
			{ NULL,									0,						MENU_SEPARATOR		},

			{ TranslateT("&Roles"),					IDM_ROLE,				MENU_NEWPOPUP		},
			{ TranslateT("&Participant list"),		IDM_LST_PARTICIPANT,	MENU_POPUPITEM		},
			{ TranslateT("&Moderator list"),		IDM_LST_MODERATOR,		MENU_POPUPITEM		},

			{ TranslateT("&Affiliations"),			IDM_AFFLTN,				MENU_NEWPOPUP		},
			{ TranslateT("&Member list"),			IDM_LST_MEMBER,			MENU_POPUPITEM		},
			{ TranslateT("&Admin list"),			IDM_LST_ADMIN,			MENU_POPUPITEM		},
			{ TranslateT("&Owner list"),			IDM_LST_OWNER,			MENU_POPUPITEM		},
			{ NULL,									0,						MENU_POPUPSEPARATOR	},
			{ TranslateT("Outcast list (&ban)"),	IDM_LST_BAN,			MENU_POPUPITEM		},

			{ NULL,									0,						MENU_SEPARATOR		},
			{ TranslateT("Set &topic"),				IDM_TOPIC,				MENU_ITEM			},
			{ TranslateT("Room &configuration"),	IDM_CONFIG,				MENU_ITEM			},
			{ TranslateT("&Destroy room"),			IDM_DESTROY,			MENU_ITEM			},
			{ NULL,									0,						MENU_SEPARATOR		},
			{ TranslateT("Copy room &JID"),			IDM_CPY_RJID,			MENU_ITEM			},
			{ TranslateT("Copy room topic"),		IDM_CPY_TOPIC,			MENU_ITEM			},
			{ NULL,									0,						MENU_SEPARATOR		},
			{ TranslateT("Add to &bookmarks"),		IDM_BOOKMARKS,			MENU_ITEM			},
			{ TranslateT("&Leave chat session"),	IDM_LEAVE,				MENU_ITEM			},
		};

		gcmi->nItems = sizeof( sttLogListItems ) / sizeof( sttLogListItems[0] );
		gcmi->Item = sttLogListItems;

		static DWORD sttModeratorItems[] = { IDM_LST_PARTICIPANT, 0 };
		static DWORD sttAdminItems[] = { IDM_LST_MODERATOR, IDM_LST_MEMBER, IDM_LST_ADMIN, IDM_LST_OWNER, IDM_LST_BAN, 0 };
		static DWORD sttOwnerItems[] = { IDM_CONFIG, IDM_DESTROY, 0 };
		
		sttSetupGcMenuItem(gcmi, 0, FALSE);

		if (!GetAsyncKeyState(VK_CONTROL))
		{
			if (me)
			{
				sttSetupGcMenuItems(gcmi, sttModeratorItems, (me->role < ROLE_MODERATOR));
				sttSetupGcMenuItems(gcmi, sttAdminItems, (me->affiliation < AFFILIATION_ADMIN));
				sttSetupGcMenuItems(gcmi, sttOwnerItems, (me->affiliation < AFFILIATION_OWNER));
			}
			if (jabberThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE)
				sttSetupGcMenuItem(gcmi, IDM_BOOKMARKS, FALSE);
		}
	} else
	if ( gcmi->Type == MENU_ON_NICKLIST )
	{
		static struct gc_item sttListItems[] =
		{
			{ TranslateT( "&User details" ),		IDM_VCARD,				MENU_ITEM			},
			{ TranslateT( "Member &info" ),			IDM_INFO,				MENU_ITEM			},
			{ NULL,									0,						MENU_SEPARATOR		},

			{ TranslateT("Set &role"),				IDM_ROLE,				MENU_NEWPOPUP		},
			{ NULL /* Visitor */,					IDM_SET_VISITOR,		MENU_POPUPITEM		},
			{ NULL /* Participant */,				IDM_SET_PARTICIPANT,	MENU_POPUPITEM		},
			{ NULL /* Moderator */,					IDM_SET_MODERATOR,		MENU_POPUPITEM		},

			{ TranslateT("Set &affiliation"),		IDM_AFFLTN,				MENU_NEWPOPUP		},
			{ NULL /* None */,						IDM_SET_NONE,			MENU_POPUPITEM		},
			{ NULL /* Member */,					IDM_SET_MEMBER,			MENU_POPUPITEM		},
			{ NULL /* Admin */,						IDM_SET_ADMIN,			MENU_POPUPITEM		},
			{ NULL /* Owner */,						IDM_SET_OWNER,			MENU_POPUPITEM		},
			{ NULL,									0,						MENU_POPUPSEPARATOR	},
			{ TranslateT("Outcast (&ban)"),			IDM_SET_BAN,			MENU_POPUPITEM		},

			{ TranslateT("&Kick"),					IDM_KICK,				MENU_ITEM			},
			{ NULL,									0,						MENU_SEPARATOR		},
			{ TranslateT("Copy &nickname"),			IDM_CPY_NICK,			MENU_ITEM			},
			{ TranslateT("Copy real &JID"),			IDM_CPY_RJID,			MENU_ITEM			},
		};

		gcmi->nItems = SIZEOF(sttListItems);
		gcmi->Item = sttListItems;

		if (me && him)
		{
			int i;
			BOOL force = GetAsyncKeyState(VK_CONTROL);
			sttSetupGcMenuItem(gcmi, 0, FALSE);

			for (i = 0; i < SIZEOF(sttAffiliationItems); ++i)
			{
				struct gc_item *item = sttFindGcMenuItem(gcmi, sttAffiliationItems[i].id);
				item->pszDesc =
					(him->affiliation == sttAffiliationItems[i].value) ?
						sttAffiliationItems[i].title_active :
						sttAffiliationItems[i].title;
				item->bDisabled = !(force || sttAffiliationItems[i].check(me, him));
			}
			for (i = 0; i < SIZEOF(sttRoleItems); ++i)
			{
				struct gc_item *item = sttFindGcMenuItem(gcmi, sttRoleItems[i].id);
				item->pszDesc =
					(him->role == sttRoleItems[i].value) ?
						sttRoleItems[i].title_active :
						sttRoleItems[i].title;
				item->bDisabled = !(force || sttRoleItems[i].check(me, him));
			}

			if (him->szRealJid)
				sttSetupGcMenuItem(gcmi, IDM_CPY_RJID, FALSE);

			if (!force)
			{
				if (me->role < ROLE_MODERATOR || (me->affiliation <= him->affiliation))
					sttSetupGcMenuItem(gcmi, IDM_KICK, TRUE);

				if ((me->affiliation < AFFILIATION_ADMIN) ||
					(me->affiliation == AFFILIATION_ADMIN) && (me->affiliation <= him->affiliation))
					sttSetupGcMenuItem(gcmi, IDM_SET_BAN, TRUE);
			}
		} else
		{
			sttSetupGcMenuItem(gcmi, 0, TRUE);
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Conference invitation dialog

static void FilterList(HWND hwndList)
{
	for	(HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
			hContact;
			hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0))
	{
		char *proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (!proto || lstrcmpA(proto, jabberProtoName) || DBGetContactSettingByte(hContact, proto, "ChatRoom", 0))
			if (int hItem = SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0))
				SendMessage(hwndList, CLM_DELETEITEM, (WPARAM)hItem, 0);
}	}
 
static void ResetListOptions(HWND hwndList)
{
	int i;
	SendMessage(hwndList,CLM_SETBKBITMAP,0,(LPARAM)(HBITMAP)NULL);
	SendMessage(hwndList,CLM_SETBKCOLOR,GetSysColor(COLOR_WINDOW),0);
	SendMessage(hwndList,CLM_SETGREYOUTFLAGS,0,0);
	SendMessage(hwndList,CLM_SETLEFTMARGIN,4,0);
	SendMessage(hwndList,CLM_SETINDENT,10,0);
	SendMessage(hwndList,CLM_SETHIDEEMPTYGROUPS,1,0);
	SendMessage(hwndList,CLM_SETHIDEOFFLINEROOT,1,0);
	for ( i=0; i <= FONTID_MAX; i++ )
		SendMessage( hwndList, CLM_SETTEXTCOLOR, i, GetSysColor( COLOR_WINDOWTEXT ));
}

static void InviteUser(TCHAR *room, TCHAR *pUser, TCHAR *text)
{
	int iqId = JabberSerialNext();

	XmlNode m( "message" ); m.addAttr( "to", room ); m.addAttrID( iqId );
	XmlNode* x = m.addChild( "x" ); x->addAttr( "xmlns", _T("http://jabber.org/protocol/muc#user"));
	XmlNode* i = x->addChild( "invite" ); i->addAttr( "to", pUser ); 
	if ( text[0] != 0 )
		i->addChild( "reason", text );
	jabberThreadInfo->send( m );
}

struct JabberGcLogInviteDlgJidData
{
	int hItem;
	TCHAR jid[JABBER_MAX_JID_LEN];
};

struct JabberGcLogInviteDlgData 
{
	JabberGcLogInviteDlgData(const TCHAR *room2):
		newJids(1), room(mir_tstrdup(room2)) {}
	~JabberGcLogInviteDlgData()
	{
		for (int i = 0; i < newJids.getCount(); ++i)
			mir_free(newJids[i]);
		mir_free(room);
	}

	LIST<JabberGcLogInviteDlgJidData> newJids;
	TCHAR *room;
};

static BOOL CALLBACK JabberGcLogInviteDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		{
			RECT dlgRect, scrRect;
			GetWindowRect( hwndDlg, &dlgRect );
			SystemParametersInfo( SPI_GETWORKAREA, 0, &scrRect, 0 );
			SetWindowPos( hwndDlg, HWND_TOPMOST, (scrRect.right/2)-(dlgRect.right/2), (scrRect.bottom/2)-(dlgRect.bottom/2), 0, 0, SWP_NOSIZE );
			TranslateDialogDefault( hwndDlg );
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIconEx( "group" ));
			SetDlgItemText( hwndDlg, IDC_ROOM, ( TCHAR* )lParam );

			SetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE,
				GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE)|CLS_HIDEOFFLINE|CLS_CHECKBOXES|CLS_HIDEEMPTYGROUPS|CLS_USEGROUPS|CLS_GREYALTERNATE|CLS_GROUPCHECKBOXES);
			SendMessage(GetDlgItem(hwndDlg, IDC_CLIST), CLM_SETEXSTYLE, CLS_EX_DISABLEDRAGDROP|CLS_EX_TRACKSELECT, 0);
			ResetListOptions(GetDlgItem(hwndDlg, IDC_CLIST));
			FilterList(GetDlgItem(hwndDlg, IDC_CLIST));

			SendDlgItemMessage(hwndDlg, IDC_ADDJID, BUTTONSETASFLATBTN, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_ADDJID, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx("addroster"));

			// use new operator to properly construct LIST object
			JabberGcLogInviteDlgData *data = new JabberGcLogInviteDlgData((TCHAR *)lParam);
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)data);
		}
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_ADDJID:
			{
				TCHAR buf[JABBER_MAX_JID_LEN];
				GetWindowText(GetDlgItem(hwndDlg, IDC_NEWJID), buf, SIZEOF(buf));
				SetWindowText(GetDlgItem(hwndDlg, IDC_NEWJID), _T(""));

				HANDLE hContact = JabberHContactFromJID(buf);
				if ( hContact ) {
					int hItem = SendDlgItemMessage( hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0 );
					if ( hItem )
						SendDlgItemMessage( hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, hItem, 1 );
					break;
				}

				JabberGcLogInviteDlgData *data = (JabberGcLogInviteDlgData *)GetWindowLong(hwndDlg, GWL_USERDATA);

				int i;
				for (i = 0; i < data->newJids.getCount(); ++i)
					if (!lstrcmp(data->newJids[i]->jid, buf))
						break;
				if (i != data->newJids.getCount())
					break;

				JabberGcLogInviteDlgJidData *jidData = (JabberGcLogInviteDlgJidData *)mir_alloc(sizeof(JabberGcLogInviteDlgJidData));
				lstrcpy(jidData->jid, buf);
				CLCINFOITEM cii = {0};
				cii.cbSize = sizeof(cii);
				cii.flags = CLCIIF_CHECKBOX;
				mir_sntprintf(buf, SIZEOF(buf), _T("%s (%s)"), jidData->jid, TranslateT("not on roster"));
				cii.pszText = buf;
				jidData->hItem = SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_ADDINFOITEM,0,(LPARAM)&cii);
				SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, jidData->hItem, 1);
				data->newJids.insert(jidData);
			}
			break;

		case IDC_INVITE:
			{
				JabberGcLogInviteDlgData *data = (JabberGcLogInviteDlgData *)GetWindowLong(hwndDlg, GWL_USERDATA);
				TCHAR* room = data->room;
				if ( room != NULL ) {
					TCHAR text[256];
					GetDlgItemText( hwndDlg, IDC_REASON, text, SIZEOF( text ));
					HWND hwndList = GetDlgItem(hwndDlg, IDC_CLIST);

					// invite users from roster
					for	(HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
							hContact;
							hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0)) {
						char *proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
						if ( !lstrcmpA(proto, jabberProtoName) && !DBGetContactSettingByte(hContact, proto, "ChatRoom", 0)) {
							if (int hItem = SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0)) {
								if ( SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0 )) {
									DBVARIANT dbv={0};
									JGetStringT(hContact, "jid", &dbv);
									if (dbv.ptszVal && ( dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_WCHAR ))
										InviteUser(room, dbv.ptszVal, text);
									JFreeVariant(&dbv);
					}	}	}	}

					// invite others
					for (int i = 0; i < data->newJids.getCount(); ++i)
						if (SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)data->newJids[i]->hItem, 0))
							InviteUser(room, data->newJids[i]->jid, text);
			}	}
			// Fall through
		case IDCANCEL:
		case IDCLOSE:
			DestroyWindow( hwndDlg );
			return TRUE;
		}
		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->idFrom == IDC_CLIST) {
			switch (((LPNMHDR)lParam)->code) {
			case CLN_NEWCONTACT:
				FilterList(GetDlgItem(hwndDlg,IDC_CLIST));
				break;
			case CLN_LISTREBUILT:
				FilterList(GetDlgItem(hwndDlg,IDC_CLIST));
				break;
			case CLN_OPTIONSCHANGED:
				ResetListOptions(GetDlgItem(hwndDlg,IDC_CLIST));
				break;
		}	}
		break;

	case WM_CLOSE:
		DestroyWindow( hwndDlg );
		break;

	case WM_DESTROY:
		JabberGcLogInviteDlgData *data = (JabberGcLogInviteDlgData *)GetWindowLong(hwndDlg, GWL_USERDATA);
		delete data;
		SetWindowLong( hwndDlg, GWL_USERDATA, NULL );
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Context menu processing

static void JabberAdminSet( const TCHAR* to, const char* ns, const char* szItem, const TCHAR* itemVal, const char* var, const TCHAR* varVal )
{
	XmlNodeIq iq( "set", NOID, to );
	XmlNode* query = iq.addQuery( ns );
	XmlNode* item = query->addChild( "item" ); item->addAttr( szItem, itemVal ); item->addAttr( var, varVal );
	jabberThreadInfo->send( iq );
}

static void JabberAdminGet( const TCHAR* to, const char* ns, const char* var, const TCHAR* varVal, JABBER_IQ_PFUNC foo )
{
	int id = JabberSerialNext();
	JabberIqAdd( id, IQ_PROC_NONE, foo );

	XmlNodeIq iq( "get", id, to );
	XmlNode* query = iq.addQuery( ns );
	XmlNode* item = query->addChild( "item" ); item->addAttr( var, varVal );
	jabberThreadInfo->send( iq );
}

// Member info dialog
struct TUserInfoData
{
	JABBER_LIST_ITEM *item;
	JABBER_RESOURCE_STATUS *me, *him;
};

static LRESULT CALLBACK sttUserInfoDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		int i, idx;
		TCHAR buf[256];

		TranslateDialogDefault(hwndDlg);

		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		TUserInfoData *dat = (TUserInfoData *)lParam;

		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIconEx("group"));

		LOGFONT lf;
		GetObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_GETFONT, 0, 0), sizeof(lf), &lf);
		lf.lfWeight = FW_BOLD;
		HFONT hfnt = CreateFontIndirect(&lf);
		SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_SETFONT, (WPARAM)hfnt, TRUE);
		SendDlgItemMessage(hwndDlg, IDC_TXT_NICK, WM_SETFONT, (WPARAM)hfnt, TRUE);

		SendDlgItemMessage(hwndDlg, IDC_BTN_AFFILIATION, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_FILE));
		SendDlgItemMessage(hwndDlg, IDC_BTN_AFFILIATION, BUTTONSETASFLATBTN, 0, 0);
		SendDlgItemMessage(hwndDlg, IDC_BTN_AFFILIATION, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Apply"), BATF_TCHAR);

		SendDlgItemMessage(hwndDlg, IDC_BTN_ROLE, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_FILE));
		SendDlgItemMessage(hwndDlg, IDC_BTN_ROLE, BUTTONSETASFLATBTN, 0, 0);
		SendDlgItemMessage(hwndDlg, IDC_BTN_ROLE, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Apply"), BATF_TCHAR);

		SendDlgItemMessage(hwndDlg, IDC_ICO_STATUS, STM_SETICON, (WPARAM)LoadSkinnedProtoIcon(jabberProtoName, dat->him->status), 0);

		mir_sntprintf(buf, SIZEOF(buf), _T("%s %s"), TranslateT("Member Info:"), dat->him->resourceName);
		SetWindowText(hwndDlg, buf);

		mir_sntprintf(buf, SIZEOF(buf), _T("%s %s %s"), dat->him->resourceName, TranslateT("from"), dat->item->jid);
		SetDlgItemText(hwndDlg, IDC_DESCRIPTION, buf);

		SetDlgItemText(hwndDlg, IDC_TXT_NICK, dat->him->resourceName);
		SetDlgItemText(hwndDlg, IDC_TXT_JID, dat->him->szRealJid ? dat->him->szRealJid : TranslateT("Real JID not available"));
		SetDlgItemText(hwndDlg, IDC_TXT_STATUS, dat->him->statusMessage);

		for (i = 0; i < SIZEOF(sttRoleItems); ++i)
		{
			if ((sttRoleItems[i].value == dat->him->role) || sttRoleItems[i].check(dat->me, dat->him))
			{
				SendDlgItemMessage(hwndDlg, IDC_TXT_ROLE, CB_SETITEMDATA,
					idx = SendDlgItemMessage(hwndDlg, IDC_TXT_ROLE, CB_ADDSTRING, 0, (LPARAM)sttRoleItems[i].title),
					sttRoleItems[i].value);
				if (sttRoleItems[i].value == dat->him->role)
					SendDlgItemMessage(hwndDlg, IDC_TXT_ROLE, CB_SETCURSEL, idx, 0);
			}
		}
		for (i = 0; i < SIZEOF(sttAffiliationItems); ++i)
		{
			if ((sttAffiliationItems[i].value == dat->him->affiliation) || sttAffiliationItems[i].check(dat->me, dat->him))
			{
				SendDlgItemMessage(hwndDlg, IDC_TXT_AFFILIATION, CB_SETITEMDATA,
					idx = SendDlgItemMessage(hwndDlg, IDC_TXT_AFFILIATION, CB_ADDSTRING, 0, (LPARAM)sttAffiliationItems[i].title),
					sttAffiliationItems[i].value);
				if (sttAffiliationItems[i].value == dat->him->affiliation)
					SendDlgItemMessage(hwndDlg, IDC_TXT_AFFILIATION, CB_SETCURSEL, idx, 0);
			}
		}

		EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_ROLE), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_AFFILIATION), FALSE);

		break;
	}

	case WM_COMMAND:
	{
		TUserInfoData *dat = (TUserInfoData *)GetWindowLong(hwndDlg, GWL_USERDATA);
		if (!dat)break;

		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			PostMessage(hwndDlg, WM_CLOSE, 0, 0);
			break;

		case IDC_TXT_AFFILIATION:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int value = SendDlgItemMessage(hwndDlg, IDC_TXT_AFFILIATION, CB_GETITEMDATA,
					SendDlgItemMessage(hwndDlg, IDC_TXT_AFFILIATION, CB_GETCURSEL, 0, 0), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_AFFILIATION), dat->him->affiliation != value);
			}
			break;

		case IDC_BTN_AFFILIATION:
			{
				int value = SendDlgItemMessage(hwndDlg, IDC_TXT_AFFILIATION, CB_GETITEMDATA,
					SendDlgItemMessage(hwndDlg, IDC_TXT_AFFILIATION, CB_GETCURSEL, 0, 0), 0);
				if (dat->him->affiliation == value) break;

				switch (value)
				{
					case AFFILIATION_NONE:
						JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "affiliation", _T("none"));
						break;
					case AFFILIATION_MEMBER:
						JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "affiliation",  _T("member"));
						break;
					case AFFILIATION_ADMIN:
						JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "affiliation", _T("admin"));
						break;
					case AFFILIATION_OWNER:
						JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "affiliation", _T("owner"));
						break;
				}
			}
			break;

		case IDC_TXT_ROLE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int value = SendDlgItemMessage(hwndDlg, IDC_TXT_ROLE, CB_GETITEMDATA,
					SendDlgItemMessage(hwndDlg, IDC_TXT_ROLE, CB_GETCURSEL, 0, 0), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_ROLE), dat->him->role != value);
			}
			break;

		case IDC_BTN_ROLE:
			{
				int value = SendDlgItemMessage(hwndDlg, IDC_TXT_ROLE, CB_GETITEMDATA,
					SendDlgItemMessage(hwndDlg, IDC_TXT_ROLE, CB_GETCURSEL, 0, 0), 0);
				if (dat->him->role == value) break;

				switch (value)
				{
					case ROLE_VISITOR:
						JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "role", _T("visitor"));
						break;
					case ROLE_PARTICIPANT:
						JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "role", _T("participant"));
						break;
					case ROLE_MODERATOR:
						JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "role", _T("moderator"));
						break;
				}
			}
			break;
		}
		break;
	}

	case WM_CTLCOLORSTATIC:
		if ( ((HWND)lParam == GetDlgItem(hwndDlg, IDC_WHITERECT)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TITLE)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DESCRIPTION)) )
			return (BOOL)GetStockObject(WHITE_BRUSH);
		return FALSE;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;

	case WM_DESTROY:
	{
		TUserInfoData *dat = (TUserInfoData *)GetWindowLong(hwndDlg, GWL_USERDATA);
		if (!dat)break;
		SetWindowLong(hwndDlg, GWL_USERDATA, 0);
		mir_free(dat);
		break;
	}
	}
	return FALSE;
}

static void sttNickListHook( JABBER_LIST_ITEM* item, GCHOOK* gch )
{
	JABBER_RESOURCE_STATUS *me = NULL, *him = NULL;
	for ( int i=0; i < item->resourceCount; i++ ) {
		JABBER_RESOURCE_STATUS& p = item->resource[i];
		if ( !lstrcmp( p.resourceName, item->nick  )) me = &p;
		if ( !lstrcmp( p.resourceName, gch->ptszUID )) him = &p;
	}

	if ( him == NULL || me == NULL )
		return;

	// 1 kick per second, prevents crashes...
	enum { BAN_KICK_INTERVAL = 1000 };
	static DWORD dwLastBanKickTime = 0;

	TCHAR szBuffer[ 1024 ];

	switch( gch->dwData ) {
	case IDM_VCARD:
	{
		HANDLE hContact;
		JABBER_SEARCH_RESULT jsr;
		mir_sntprintf(jsr.jid, SIZEOF(jsr.jid), _T("%s/%s"), item->jid, him->resourceName );
		jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
		
		JABBER_LIST_ITEM* item = JabberListAdd( LIST_VCARD_TEMP, jsr.jid );
		item->bUseResource = TRUE;
		JabberListAddResource( LIST_VCARD_TEMP, jsr.jid, him->status, him->statusMessage, him->priority );

		hContact = ( HANDLE )CallProtoService( jabberProtoName, PS_ADDTOLIST, PALF_TEMPORARY, ( LPARAM )&jsr );
		CallService( MS_USERINFO_SHOWDIALOG, ( WPARAM )hContact, 0 );
		break;
	}
	case IDM_INFO:
	{
		TUserInfoData *dat = (TUserInfoData *)mir_alloc(sizeof(TUserInfoData));
		dat->me = me;
		dat->him = him;
		dat->item = item;
		HWND hwndInfo = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_GROUPCHAT_INFO), NULL, (DLGPROC)sttUserInfoDlgProc, (LPARAM)dat);
		ShowWindow(hwndInfo, SW_SHOW);
		break;
	}
	case IDM_KICK:
	{
		if ((GetTickCount() - dwLastBanKickTime) > BAN_KICK_INTERVAL)
		{
			dwLastBanKickTime = GetTickCount();
			mir_sntprintf( szBuffer, SIZEOF(szBuffer), _T("%s %s"), TranslateT( "Reason to kick" ), him->resourceName );
			if ( JabberEnterString( szBuffer, SIZEOF(szBuffer)))
			{
				XmlNodeIq iq( "set", NOID, item->jid );
				XmlNode* query = iq.addQuery( xmlnsAdmin );
				XmlNode* item = query->addChild( "item" ); item->addAttr( "nick", him->resourceName ); item->addAttr( "role", "none" );
				item->addChild( "reason", szBuffer );
				jabberThreadInfo->send( iq );
			}
		}
		dwLastBanKickTime = GetTickCount();
		break;
	}

	case IDM_SET_VISITOR:
		if (him->role != ROLE_VISITOR)
			JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "role", _T("visitor"));
		break;
	case IDM_SET_PARTICIPANT:
		if (him->role != ROLE_PARTICIPANT)
			JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "role", _T("participant"));
		break;
	case IDM_SET_MODERATOR:
		if (him->role != ROLE_MODERATOR)
			JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "role", _T("moderator"));
		break;

	case IDM_SET_NONE:
		if (him->affiliation != AFFILIATION_NONE)
			JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "affiliation", _T("none"));
		break;
	case IDM_SET_MEMBER:
		if (him->affiliation != AFFILIATION_MEMBER)
			JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "affiliation",  _T("member"));
		break;
	case IDM_SET_ADMIN:
		if (him->affiliation != AFFILIATION_ADMIN)
			JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "affiliation", _T("admin"));
		break;
	case IDM_SET_OWNER:
		if (him->affiliation != AFFILIATION_OWNER)
			JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "affiliation", _T("owner"));
		break;

	case IDM_SET_BAN:
		if ((GetTickCount() - dwLastBanKickTime) > BAN_KICK_INTERVAL)
		{
			dwLastBanKickTime = GetTickCount();
			mir_sntprintf( szBuffer, SIZEOF(szBuffer), _T("%s %s"), TranslateT( "Reason to ban" ), him->resourceName );
			if ( JabberEnterString( szBuffer, SIZEOF(szBuffer)))
			{
				XmlNodeIq iq( "set", NOID, item->jid );
				XmlNode* query = iq.addQuery( xmlnsAdmin );
				XmlNode* item = query->addChild( "item" ); item->addAttr( "nick", him->resourceName ); item->addAttr( "affiliation", "outcast" );
				item->addChild( "reason", szBuffer );
				jabberThreadInfo->send( iq );
			}
		}
		dwLastBanKickTime = GetTickCount();
		break;

	case IDM_CPY_NICK:
		JabberCopyText((HWND)CallService(MS_CLUI_GETHWND, 0, 0), him->resourceName);
		break;
	case IDM_CPY_RJID:
		JabberCopyText((HWND)CallService(MS_CLUI_GETHWND, 0, 0), him->szRealJid);
		break;
	}
}

static void sttLogListHook( JABBER_LIST_ITEM* item, GCHOOK* gch )
{
	TCHAR szBuffer[ 1024 ];
	TCHAR szCaption[ 1024 ];
	szBuffer[ 0 ] = _T('\0');

	switch( gch->dwData ) {
	case IDM_LST_PARTICIPANT:
		JabberAdminGet( gch->pDest->ptszID, xmlnsAdmin, "role", _T("participant"), JabberIqResultMucGetVoiceList );
		break;

	case IDM_LST_MEMBER:
		JabberAdminGet( gch->pDest->ptszID, xmlnsAdmin, "affiliation", _T("member"), JabberIqResultMucGetMemberList );
		break;

	case IDM_LST_MODERATOR:
		JabberAdminGet( gch->pDest->ptszID, xmlnsAdmin, "role", _T("moderator"), JabberIqResultMucGetModeratorList );
		break;

	case IDM_LST_BAN:
		JabberAdminGet( gch->pDest->ptszID, xmlnsAdmin, "affiliation", _T("outcast"), JabberIqResultMucGetBanList );
		break;

	case IDM_LST_ADMIN:
		JabberAdminGet( gch->pDest->ptszID, xmlnsAdmin, "affiliation", _T("admin"), JabberIqResultMucGetAdminList );
		break;

	case IDM_LST_OWNER:
		JabberAdminGet( gch->pDest->ptszID, xmlnsAdmin, "affiliation", _T("owner"), JabberIqResultMucGetOwnerList );
		break;

	case IDM_TOPIC:
		mir_sntprintf( szCaption, SIZEOF(szCaption), _T("%s %s"), TranslateT( "Set topic for" ), gch->pDest->ptszID );
		TCHAR szTmpBuff[ SIZEOF(szBuffer) * 2 ];
		if ( item->itemResource.statusMessage ) {
			int j = 0;
			for ( int i = 0; i < SIZEOF(szTmpBuff); i++ ) {
				if ( item->itemResource.statusMessage[ i ] != _T('\n') || ( i && item->itemResource.statusMessage[ i - 1 ] == _T('\r')))
					szTmpBuff[ j++ ] = item->itemResource.statusMessage[ i ];
				else {
					szTmpBuff[ j++ ] = _T('\r');
					szTmpBuff[ j++ ] = _T('\n');
				}
				if ( !item->itemResource.statusMessage[ i ] )
					break;
			}
		}
		else
			szTmpBuff[ 0 ] = _T('\0');
		if ( JabberEnterString( szCaption, szTmpBuff, SIZEOF(szTmpBuff))) {
			XmlNode msg( "message" ); msg.addAttr( "to", gch->pDest->ptszID ); msg.addAttr( "type", "groupchat" );
			msg.addChild( "subject", szTmpBuff );
			jabberThreadInfo->send( msg );
		}
		break;

	case IDM_NICK:
		mir_sntprintf( szCaption, SIZEOF(szCaption), _T("%s %s"), TranslateT( "Change nickname in" ), gch->pDest->ptszID );
		if ( item->nick )
			mir_sntprintf( szBuffer, SIZEOF(szBuffer), _T("%s"), item->nick );
		if ( JabberEnterString( szCaption, szBuffer, SIZEOF(szBuffer))) {
			JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_CHATROOM, gch->pDest->ptszID );
			if ( item != NULL ) {
				TCHAR text[ 1024 ];
				mir_sntprintf( text, SIZEOF( text ), _T("%s/%s"), gch->pDest->ptszID, szBuffer );
				JabberSendPresenceTo( jabberStatus, text, NULL );
		}	}
		break;

	case IDM_INVITE:
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_INVITE ), NULL, JabberGcLogInviteDlgProc, ( LPARAM )gch->pDest->pszID );
		break;

	case IDM_CONFIG:
	{
		int iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultGetMuc );

		XmlNodeIq iq( "get", iqId, gch->pDest->ptszID );
		XmlNode* query = iq.addQuery( xmlnsOwner );
		jabberThreadInfo->send( iq );
		break;
	}
	case IDM_BOOKMARKS:
	{
		JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_BOOKMARK, gch->pDest->ptszID );
		if ( item == NULL ) {
			item = JabberListGetItemPtr( LIST_CHATROOM, gch->pDest->ptszID );
			if (item != NULL) {
				item->type = _T("conference");
				HANDLE hContact = JabberHContactFromJID( item->jid );
				item->name = ( TCHAR* )JCallService( MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, GCDNF_TCHAR );
				JabberAddEditBookmark(NULL, (LPARAM) item);
			}
		}
		break;
	}
	case IDM_DESTROY:
		mir_sntprintf( szBuffer, SIZEOF(szBuffer), _T("%s %s"), TranslateT( "Reason to destroy" ), gch->pDest->ptszID );
		if ( !JabberEnterString( szBuffer, SIZEOF(szBuffer)))
			break;

		{	XmlNodeIq iq( "set", NOID, gch->pDest->ptszID );
			XmlNode* query = iq.addQuery( xmlnsOwner );
			query->addChild( "destroy" )->addChild( "reason", szBuffer );
			jabberThreadInfo->send( iq );
		}

	case IDM_LEAVE:
		JabberGcQuit( item, 0, 0 );
		break;

	case IDM_CPY_RJID:
		JabberCopyText((HWND)CallService(MS_CLUI_GETHWND, 0, 0), item->jid);
		break;
	case IDM_CPY_TOPIC:
		JabberCopyText((HWND)CallService(MS_CLUI_GETHWND, 0, 0), item->itemResource.statusMessage);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Sends a private message to a chat user

static void sttSendPrivateMessage( JABBER_LIST_ITEM* item, const TCHAR* nick )
{
	TCHAR szFullJid[ 256 ];
	mir_sntprintf( szFullJid, SIZEOF(szFullJid), _T("%s/%s"), item->jid, nick );
	HANDLE hContact = JabberDBCreateContact( szFullJid, NULL, TRUE, FALSE );
	if ( hContact != NULL ) {
		for ( int i=0; i < item->resourceCount; i++ ) {
			if ( _tcsicmp( item->resource[i].resourceName, nick ) == 0 ) {
				JSetWord( hContact, "Status", item->resource[i].status );
				break;
		}	}

		DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
		JSetStringT( hContact, "Nick", nick );
		DBWriteContactSettingDword( hContact, "Ignore", "Mask1", 0 );
		JCallService( MS_MSG_SENDMESSAGE, ( WPARAM )hContact, 0 );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// General chat event processing hook

int JabberGcEventHook(WPARAM wParam,LPARAM lParam)
{
	GCHOOK* gch = ( GCHOOK* )lParam;
	if ( gch == NULL )
		return 0;

	if ( lstrcmpiA( gch->pDest->pszModule, jabberProtoName ))
		return 0;

	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_CHATROOM, gch->pDest->ptszID );
	if ( item == NULL )
		return 0;

	switch ( gch->pDest->iType ) {
	case GC_USER_MESSAGE:
		if ( gch->pszText && lstrlen( gch->ptszText) > 0 ) {
			rtrim( gch->ptszText );

			if ( jabberOnline ) {
				XmlNode m( "message" ); m.addAttr( "to", item->jid ); m.addAttr( "type", "groupchat" );
				XmlNode* b = m.addChild( "body", gch->ptszText );
				if ( b->sendText != NULL )
					UnEscapeChatTags( b->sendText );
				jabberThreadInfo->send( m );
		}	}
		break;

	case GC_USER_PRIVMESS:
		sttSendPrivateMessage( item, gch->ptszUID );
		break;

	case GC_USER_LOGMENU:
		sttLogListHook( item, gch );
		break;

	case GC_USER_NICKLISTMENU:
		sttNickListHook( item, gch );
		break;

	case GC_USER_CHANMGR:
		int iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultGetMuc );

		XmlNodeIq iq( "get", iqId, item->jid );
		XmlNode* query = iq.addQuery( xmlnsOwner );
		jabberThreadInfo->send( iq );
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void JabberAddMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* str )
{
	const char* field = _tcschr(str,'@') ? "jid" : "nick";
	TCHAR* roomJid = jidListInfo->roomJid;

	switch (jidListInfo->type) {
	case MUC_VOICELIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "role", _T("participant"));
		JabberAdminGet( roomJid, xmlnsAdmin, "role", _T("participant"), JabberIqResultMucGetVoiceList);
		break;
	case MUC_MEMBERLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "affiliation", _T("member"));
		JabberAdminGet( roomJid, xmlnsAdmin, "affiliation", _T("member"), JabberIqResultMucGetMemberList);
		break;
	case MUC_MODERATORLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "role", _T("moderator"));
		JabberAdminGet( roomJid, xmlnsAdmin, "role", _T("moderator"), JabberIqResultMucGetModeratorList);
		break;
	case MUC_BANLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "affiliation", _T("outcast"));
		JabberAdminGet( roomJid, xmlnsAdmin, "affiliation", _T("outcast"), JabberIqResultMucGetBanList);
		break;
	case MUC_ADMINLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "affiliation", _T("admin"));
		JabberAdminGet( roomJid, xmlnsAdmin, "affiliation", _T("admin"), JabberIqResultMucGetAdminList);
		break;
	case MUC_OWNERLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "affiliation", _T("owner"));
		JabberAdminGet( roomJid, xmlnsAdmin, "affiliation", _T("owner"), JabberIqResultMucGetOwnerList);
		break;
}	}

void JabberDeleteMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* jid )
{
	TCHAR* roomJid = jidListInfo->roomJid;

	switch ( jidListInfo->type ) {
	case MUC_VOICELIST:		// change role to visitor ( from participant )
		JabberAdminSet( roomJid, xmlnsAdmin, "jid", jid, "role", _T("visitor"));
		break;
	case MUC_BANLIST:		// change affiliation to none ( from outcast )
	case MUC_MEMBERLIST:	// change affiliation to none ( from member )
		JabberAdminSet( roomJid, xmlnsAdmin, "jid", jid, "affiliation", _T("none"));
		break;
	case MUC_MODERATORLIST:	// change role to participant ( from moderator )
		JabberAdminSet( roomJid, xmlnsAdmin, "jid", jid, "role", _T("participant"));
		break;
	case MUC_ADMINLIST:		// change affiliation to member ( from admin )
		JabberAdminSet( roomJid, xmlnsAdmin, "jid", jid, "affiliation", _T("member"));
		break;
	case MUC_OWNERLIST:		// change affiliation to admin ( from owner )
		JabberAdminSet( roomJid, xmlnsAdmin, "jid", jid, "affiliation", _T("admin"));
		break;
}	}
