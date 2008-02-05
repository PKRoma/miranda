/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan

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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_iq.h"
#include "resource.h"
#include "jabber_caps.h"

#include <m_addcontact.h>

/////////////////////////////////////////////////////////////////////////////////////////
// Global definitions

enum {
	IDM_CANCEL,

	IDM_ROLE, IDM_AFFLTN,

	IDM_CONFIG, IDM_NICK, IDM_DESTROY, IDM_INVITE, IDM_BOOKMARKS, IDM_LEAVE, IDM_TOPIC,
	IDM_LST_PARTICIPANT, IDM_LST_MODERATOR,
	IDM_LST_MEMBER, IDM_LST_ADMIN, IDM_LST_OWNER, IDM_LST_BAN,

	IDM_MESSAGE, IDM_SLAP, IDM_VCARD, IDM_INFO, IDM_KICK,
	IDM_RJID, IDM_RJID_ADD, IDM_RJID_VCARD, IDM_RJID_COPY,
	IDM_SET_VISITOR, IDM_SET_PARTICIPANT, IDM_SET_MODERATOR,
	IDM_SET_NONE, IDM_SET_MEMBER, IDM_SET_ADMIN, IDM_SET_OWNER, IDM_SET_BAN,
	IDM_CPY_NICK, IDM_CPY_TOPIC, IDM_CPY_RJID,

	IDM_LINK0, IDM_LINK1, IDM_LINK2, IDM_LINK3, IDM_LINK4, IDM_LINK5, IDM_LINK6, IDM_LINK7, IDM_LINK8, IDM_LINK9,
};

struct TRoleOrAffiliationInfo
{
	int value;
	int id;
	TCHAR *title_en;
	int min_role;
	int min_affiliation;

	TCHAR *title;

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
	}
};

static TRoleOrAffiliationInfo sttAffiliationItems[] =
{
	{ AFFILIATION_NONE,		IDM_SET_NONE,			LPGENT("None"),			ROLE_NONE,		AFFILIATION_ADMIN	},
	{ AFFILIATION_MEMBER,	IDM_SET_MEMBER,			LPGENT("Member"),		ROLE_NONE,		AFFILIATION_ADMIN	},
	{ AFFILIATION_ADMIN,	IDM_SET_ADMIN,			LPGENT("Admin"),		ROLE_NONE,		AFFILIATION_OWNER	},
	{ AFFILIATION_OWNER,	IDM_SET_OWNER,			LPGENT("Owner"),		ROLE_NONE,		AFFILIATION_OWNER	},
};

static TRoleOrAffiliationInfo sttRoleItems[] =
{
	{ ROLE_VISITOR,			IDM_SET_VISITOR,		LPGENT("Visitor"),		ROLE_MODERATOR,	AFFILIATION_NONE	},
	{ ROLE_PARTICIPANT,		IDM_SET_PARTICIPANT,	LPGENT("Participant"),	ROLE_MODERATOR,	AFFILIATION_NONE	},
	{ ROLE_MODERATOR,		IDM_SET_MODERATOR,		LPGENT("Moderator"),	ROLE_MODERATOR,	AFFILIATION_ADMIN	},
};

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGcInit - initializes the new chat

static TCHAR* sttStatuses[] = { _T("Visitors"), _T("Participants"), _T("Moderators"), _T("Owners") };

int JabberGcGetStatus(JABBER_GC_AFFILIATION a, JABBER_GC_ROLE r)
{
	switch (a)
	{
	case AFFILIATION_OWNER:		return 3;

	default:
		switch (r)
		{
		case ROLE_MODERATOR:	return 2;
		case ROLE_PARTICIPANT:	return 1;
		default:				return 0;
		}
	}

	return 0;
}

int JabberGcGetStatus(JABBER_RESOURCE_STATUS *r)
{
	return JabberGcGetStatus(r->affiliation, r->role);
}

int CJabberProto::JabberGcInit( WPARAM wParam, LPARAM lParam )
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
	gcw.pszModule = m_szProtoName;
	gcw.ptszName = szNick;
	gcw.ptszID = item->jid;
	gcw.dwFlags = GC_TCHAR;
	CallServiceSync( MS_GC_NEWSESSION, NULL, (LPARAM)&gcw );

	HANDLE hContact = JabberHContactFromJID( item->jid );
	if ( hContact != NULL ) {
		DBVARIANT dbv;
		if ( !DBGetContactSettingTString( hContact, m_szProtoName, "MyNick", &dbv )) {
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

	GCDEST gcd = { m_szProtoName, NULL, GC_EVENT_ADDGROUP };
	gcd.ptszID = item->jid;
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	gce.dwFlags = GC_TCHAR;
	for (i = SIZEOF(sttStatuses)-1; i >= 0; i-- ) {
		gce.ptszStatus = TranslateTS( sttStatuses[i] );
		CallServiceSync( MS_GC_EVENT, NULL, ( LPARAM )&gce );
	}

	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	gcd.iType = GC_EVENT_CONTROL;
	CallServiceSync( MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gce );
	CallServiceSync( MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce );
	return 0;
}

void CJabberProto::JabberGcLogCreate( JABBER_LIST_ITEM* item )
{
	if ( item->bChatActive )
		return;

	NotifyEventHooks( hInitChat, (WPARAM)item, 0 );
}

void CJabberProto::JabberGcLogShowInformation( JABBER_LIST_ITEM *item, JABBER_RESOURCE_STATUS *user, TJabberGcLogInfoType type )
{
	if (!item || !user || (item->bChatActive != 2)) return;

	TCHAR buf[256] = {0};

	switch (type)
	{
		case INFO_BAN:
			if (JGetByte("GcLogBans", TRUE))
			{
				mir_sntprintf(buf, SIZEOF(buf), TranslateT("User %s in now banned."), user->resourceName);
			}
			break;
//		case INFO_STATUS:
//			if (JGetByte("GcLogStatuses", FALSE))
//			{
//				mir_sntprintf(buf, SIZEOF(buf), TranslateT("User %s changed status to %s."), user->resourceName, TranslateT("Online"));
//			}
//			break;
		case INFO_CONFIG:
			if (JGetByte("GcLogConfig", FALSE))
			{
				mir_sntprintf(buf, SIZEOF(buf), TranslateT("Room configuration was changed."));
			}
			break;
		case INFO_AFFILIATION:
			if (JGetByte("GcLogAffiliations", FALSE))
			{
				TCHAR *name = NULL;
				switch (user->affiliation)
				{
					case AFFILIATION_NONE:		name = TranslateT("None"); break;
					case AFFILIATION_MEMBER:	name = TranslateT("Member"); break;
					case AFFILIATION_ADMIN:		name = TranslateT("Admin"); break;
					case AFFILIATION_OWNER:		name = TranslateT("Owner"); break;
					case AFFILIATION_OUTCAST:	name = TranslateT("Outcast"); break;
				}
				if (name) mir_sntprintf(buf, SIZEOF(buf), TranslateT("Affiliation of %s was changed to '%s'."), user->resourceName, name);
			}
			break;
		case INFO_ROLE:
			if (JGetByte("GcLogRoles", FALSE))
			{
				TCHAR *name = NULL;
				switch (user->role)
				{
					case ROLE_NONE:			name = TranslateT("None"); break;
					case ROLE_VISITOR:		name = TranslateT("Visitor"); break;
					case ROLE_PARTICIPANT:	name = TranslateT("Participant"); break;
					case ROLE_MODERATOR:    name = TranslateT("Moderator"); break;
				}
				if (name) mir_sntprintf(buf, SIZEOF(buf), TranslateT("Role of %s was changed to '%s'."), user->resourceName, name);
			}
			break;
	}

	if (*buf)
	{
		GCDEST gcd = { m_szProtoName, 0, 0 };
		gcd.ptszID = item->jid;
		GCEVENT gce = {0};
		gce.cbSize = sizeof(GCEVENT);
		gce.ptszNick = user->resourceName;
		gce.ptszUID = user->resourceName;
		gce.ptszText = buf;
		gce.dwFlags = GC_TCHAR|GCEF_ADDTOLOG;
		gce.pDest = &gcd;
		gce.time = time(0);
		gcd.iType = GC_EVENT_INFORMATION;
		CallServiceSync( MS_GC_EVENT, NULL, ( LPARAM )&gce );
	}
}

void CJabberProto::JabberGcLogUpdateMemberStatus( JABBER_LIST_ITEM* item, TCHAR* nick, TCHAR* jid, int action, XmlNode* reason, int nStatusCode )
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

	GCDEST gcd = { m_szProtoName, 0, 0 };
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
				gce.ptszStatus = TranslateTS( sttStatuses[JabberGcGetStatus(&JS)] );
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

void CJabberProto::JabberGcQuit( JABBER_LIST_ITEM* item, int code, XmlNode* reason )
{
	DBVARIANT dbvMessage = {0};
	TCHAR *szMessage = NULL;

	TCHAR* szReason = NULL;
	if ( reason != NULL && reason->text != NULL )
		szReason = reason->text;

	GCDEST gcd = { m_szProtoName, NULL, GC_EVENT_CONTROL };
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
		if (!DBGetContactSettingTString( NULL, m_szProtoName, "GcMsgQuit", &dbvMessage))
			szMessage = dbvMessage.ptszVal;
		else
			szMessage = TranslateTS(JABBER_GC_MSG_QUIT);
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
		if (szMessage) {
			p.addChild("status", szMessage);
			if (szMessage == dbvMessage.ptszVal)
				DBFreeVariant(&dbvMessage);
		}
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

static void sttShowGcMenuItem(GCMENUITEMS *items, DWORD id, int type)
{
	for (int i = 0; i < items->nItems; ++i)
		if (!id || (items->Item[i].dwID == id))
			items->Item[i].uType = type;
}

static void sttSetupGcMenuItems(GCMENUITEMS *items, DWORD *ids, bool disabled)
{
	for ( ; *ids; ++ids)
		sttSetupGcMenuItem(items, *ids, disabled);
}

static void sttShowGcMenuItems(GCMENUITEMS *items, DWORD *ids, int type)
{
	for ( ; *ids; ++ids)
		sttShowGcMenuItem(items, *ids, type);
}

int CJabberProto::JabberGcMenuHook( WPARAM wParam, LPARAM lParam )
{
	GCMENUITEMS* gcmi= ( GCMENUITEMS* )lParam;
	if ( gcmi == NULL )
		return 0;

	if ( lstrcmpiA( gcmi->pszModule, m_szProtoName ))
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
		static TCHAR url_buf[1024] = {0};
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

			{ TranslateT("&Room options"),			0,						MENU_NEWPOPUP		},
			{ TranslateT("View/change &topic"),		IDM_TOPIC,				MENU_POPUPITEM		},
			{ TranslateT("Add to &bookmarks"),		IDM_BOOKMARKS,			MENU_POPUPITEM		},
			{ TranslateT("&Configure..."),			IDM_CONFIG,				MENU_POPUPITEM		},
			{ TranslateT("&Destroy room"),			IDM_DESTROY,			MENU_POPUPITEM		},

			{ NULL,									0,						MENU_SEPARATOR		},

			{ TranslateT("Lin&ks"),					0,						MENU_NEWPOPUP		},
			{ NULL,									IDM_LINK0,				0					},
			{ NULL,									IDM_LINK1,				0					},
			{ NULL,									IDM_LINK2,				0					},
			{ NULL,									IDM_LINK3,				0					},
			{ NULL,									IDM_LINK4,				0					},
			{ NULL,									IDM_LINK5,				0					},
			{ NULL,									IDM_LINK6,				0					},
			{ NULL,									IDM_LINK7,				0					},
			{ NULL,									IDM_LINK8,				0					},
			{ NULL,									IDM_LINK9,				0					},

			{ TranslateT("Copy room &JID"),			IDM_CPY_RJID,			MENU_ITEM			},
			{ TranslateT("Copy room topic"),		IDM_CPY_TOPIC,			MENU_ITEM			},
			{ NULL,									0,						MENU_SEPARATOR		},
			{ TranslateT("&Leave chat session"),	IDM_LEAVE,				MENU_ITEM			},
		};

		gcmi->nItems = sizeof( sttLogListItems ) / sizeof( sttLogListItems[0] );
		gcmi->Item = sttLogListItems;

		static DWORD sttModeratorItems[] = { IDM_LST_PARTICIPANT, 0 };
		static DWORD sttAdminItems[] = { IDM_LST_MODERATOR, IDM_LST_MEMBER, IDM_LST_ADMIN, IDM_LST_OWNER, IDM_LST_BAN, 0 };
		static DWORD sttOwnerItems[] = { IDM_CONFIG, IDM_DESTROY, 0 };
		
		sttSetupGcMenuItem(gcmi, 0, FALSE);

		int idx = IDM_LINK0;
		if (item->itemResource.statusMessage && *item->itemResource.statusMessage)
		{
			TCHAR *bufPtr = url_buf;
			for (TCHAR *p = _tcsstr(item->itemResource.statusMessage, _T("http://")); p && *p; p = _tcsstr(p+1, _T("http://")))
			{
				lstrcpyn(bufPtr, p, SIZEOF(url_buf) - (bufPtr - url_buf));
				sttFindGcMenuItem(gcmi, idx)->pszDesc = bufPtr;
				sttFindGcMenuItem(gcmi, idx)->uType = MENU_POPUPITEM;
				for ( ; *bufPtr && !_istspace(*bufPtr); ++bufPtr) ;
				*bufPtr++ = 0;

				if (++idx > IDM_LINK9) break;
			}
		}
		for ( ; idx <= IDM_LINK9; ++idx)
			sttFindGcMenuItem(gcmi, idx)->uType = 0;

		if (!GetAsyncKeyState(VK_CONTROL))
		{
			//sttFindGcMenuItem(gcmi, IDM_DESTROY)->uType = 0;

			if (me)
			{
				sttSetupGcMenuItems(gcmi, sttModeratorItems, (me->role < ROLE_MODERATOR));
				sttSetupGcMenuItems(gcmi, sttAdminItems, (me->affiliation < AFFILIATION_ADMIN));
				sttSetupGcMenuItems(gcmi, sttOwnerItems, (me->affiliation < AFFILIATION_OWNER));
			}
			if (jabberThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE)
				sttSetupGcMenuItem(gcmi, IDM_BOOKMARKS, FALSE);
		} else
		{
			//sttFindGcMenuItem(gcmi, IDM_DESTROY)->uType = MENU_ITEM;
		}
	} else
	if ( gcmi->Type == MENU_ON_NICKLIST )
	{
		static TCHAR sttRJidBuf[JABBER_MAX_JID_LEN] = {0};
		static struct gc_item sttListItems[] =
		{
			{ TranslateT("&Slap"),					IDM_SLAP,				MENU_ITEM			},	// 0
			{ TranslateT("&User details"),			IDM_VCARD,				MENU_ITEM			},	// 1
			{ TranslateT("Member &info"),			IDM_INFO,				MENU_ITEM			},	// 2

			{ sttRJidBuf,							0,						MENU_NEWPOPUP		},	// 3 -> accessed explicitly by index!!!
			{ TranslateT("User &details"),			IDM_RJID_VCARD,			MENU_POPUPITEM		},
			{ TranslateT("&Add to roster"),			IDM_RJID_ADD,			MENU_POPUPITEM		},
			{ TranslateT("&Copy to clipboard"),		IDM_RJID_COPY,			MENU_POPUPITEM		},

			{ NULL,									0,						MENU_SEPARATOR		},

			{ TranslateT("Set &role"),				IDM_ROLE,				MENU_NEWPOPUP		},
			{ TranslateT("&Visitor"),				IDM_SET_VISITOR,		MENU_POPUPITEM		},
			{ TranslateT("&Participant"),			IDM_SET_PARTICIPANT,	MENU_POPUPITEM		},
			{ TranslateT("&Moderator"),				IDM_SET_MODERATOR,		MENU_POPUPITEM		},

			{ TranslateT("Set &affiliation"),		IDM_AFFLTN,				MENU_NEWPOPUP		},
			{ TranslateT("&None"),					IDM_SET_NONE,			MENU_POPUPITEM		},
			{ TranslateT("&Member"),				IDM_SET_MEMBER,			MENU_POPUPITEM		},
			{ TranslateT("&Admin"),					IDM_SET_ADMIN,			MENU_POPUPITEM		},
			{ TranslateT("&Owner"),					IDM_SET_OWNER,			MENU_POPUPITEM		},
			{ NULL,									0,						MENU_POPUPSEPARATOR	},
			{ TranslateT("Outcast (&ban)"),			IDM_SET_BAN,			MENU_POPUPITEM		},

			{ TranslateT("&Kick"),					IDM_KICK,				MENU_ITEM			},
			{ NULL,									0,						MENU_SEPARATOR		},
			{ TranslateT("Copy &nickname"),			IDM_CPY_NICK,			MENU_ITEM			},
			{ TranslateT("Copy real &JID"),			IDM_CPY_RJID,			MENU_ITEM			},
		};

		gcmi->nItems = SIZEOF(sttListItems);
		gcmi->Item = sttListItems;

		static DWORD sttRJidItems[] = { IDM_RJID_VCARD, IDM_RJID_ADD, IDM_RJID_COPY, 0 };

		if (me && him)
		{
			int i;
			BOOL force = GetAsyncKeyState(VK_CONTROL);
			sttSetupGcMenuItem(gcmi, 0, FALSE);

			for (i = 0; i < SIZEOF(sttAffiliationItems); ++i)
			{
				struct gc_item *item = sttFindGcMenuItem(gcmi, sttAffiliationItems[i].id);
				item->uType = (him->affiliation == sttAffiliationItems[i].value) ? MENU_POPUPCHECK : MENU_POPUPITEM;
				item->bDisabled = !(force || sttAffiliationItems[i].check(me, him));
			}
			for (i = 0; i < SIZEOF(sttRoleItems); ++i)
			{
				struct gc_item *item = sttFindGcMenuItem(gcmi, sttRoleItems[i].id);
				item->uType = (him->role == sttRoleItems[i].value) ? MENU_POPUPCHECK : MENU_POPUPITEM;
				item->bDisabled = !(force || sttRoleItems[i].check(me, him));
			}

			if (him->szRealJid && *him->szRealJid)
			{
				mir_sntprintf(sttRJidBuf, SIZEOF(sttRJidBuf), TranslateT("Real &JID: %s"), him->szRealJid);
				if (TCHAR *tmp = _tcsrchr(sttRJidBuf, _T('/'))) *tmp = 0;

				if (HANDLE hContact = JabberHContactFromJID(him->szRealJid))
				{
					gcmi->Item[3].uType = MENU_HMENU;
					gcmi->Item[3].dwID = CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM)hContact, 0);
					sttShowGcMenuItems(gcmi, sttRJidItems, 0);
				} else
				{
					gcmi->Item[3].uType = MENU_NEWPOPUP;
					sttShowGcMenuItems(gcmi, sttRJidItems, MENU_POPUPITEM);
				}

				sttSetupGcMenuItem(gcmi, IDM_CPY_RJID, FALSE);
			} else
			{
				gcmi->Item[3].uType = 0;
				sttShowGcMenuItems(gcmi, sttRJidItems, 0);

				sttSetupGcMenuItem(gcmi, IDM_CPY_RJID, TRUE);
			}

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
			gcmi->Item[2].uType = 0;
			sttShowGcMenuItems(gcmi, sttRJidItems, 0);
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Conference invitation dialog

static void FilterList(CJabberProto* ppro, HWND hwndList)
{
	for	(HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
			hContact;
			hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0))
	{
		char *proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (!proto || lstrcmpA(proto, ppro->m_szProtoName) || DBGetContactSettingByte(hContact, proto, "ChatRoom", 0))
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

static void InviteUser(CJabberProto* ppro, TCHAR *room, TCHAR *pUser, TCHAR *text)
{
	int iqId = ppro->JabberSerialNext();

	XmlNode m( "message" ); m.addAttr( "to", room ); m.addAttrID( iqId );
	XmlNode* x = m.addChild( "x" ); x->addAttr( "xmlns", _T("http://jabber.org/protocol/muc#user"));
	XmlNode* i = x->addChild( "invite" ); i->addAttr( "to", pUser ); 
	if ( text[0] != 0 )
		i->addChild( "reason", text );
	ppro->jabberThreadInfo->send( m );
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
	CJabberProto* ppro;
};

static BOOL CALLBACK JabberGcLogInviteDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	JabberGcLogInviteDlgData *data = (JabberGcLogInviteDlgData *)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			// use new operator to properly construct LIST object
			data = ( JabberGcLogInviteDlgData* )lParam;
			SetWindowLong(hwndDlg, GWL_USERDATA, lParam );

			RECT dlgRect, scrRect;
			GetWindowRect( hwndDlg, &dlgRect );
			SystemParametersInfo( SPI_GETWORKAREA, 0, &scrRect, 0 );
			SetWindowPos( hwndDlg, HWND_TOPMOST, (scrRect.right/2)-(dlgRect.right/2), (scrRect.bottom/2)-(dlgRect.bottom/2), 0, 0, SWP_NOSIZE );
			TranslateDialogDefault( hwndDlg );
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )data->ppro->LoadIconEx( "group" ));
			SetDlgItemText( hwndDlg, IDC_ROOM, ( TCHAR* )lParam );

			SetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE,
				GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE)|CLS_HIDEOFFLINE|CLS_CHECKBOXES|CLS_HIDEEMPTYGROUPS|CLS_USEGROUPS|CLS_GREYALTERNATE|CLS_GROUPCHECKBOXES);
			SendMessage(GetDlgItem(hwndDlg, IDC_CLIST), CLM_SETEXSTYLE, CLS_EX_DISABLEDRAGDROP|CLS_EX_TRACKSELECT, 0);
			ResetListOptions(GetDlgItem(hwndDlg, IDC_CLIST));
			FilterList(data->ppro, GetDlgItem(hwndDlg, IDC_CLIST));

			SendDlgItemMessage(hwndDlg, IDC_ADDJID, BUTTONSETASFLATBTN, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_ADDJID, BM_SETIMAGE, IMAGE_ICON, (LPARAM)data->ppro->LoadIconEx("addroster"));
		}
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_ADDJID:
			{
				TCHAR buf[JABBER_MAX_JID_LEN];
				GetWindowText(GetDlgItem(hwndDlg, IDC_NEWJID), buf, SIZEOF(buf));
				SetWindowText(GetDlgItem(hwndDlg, IDC_NEWJID), _T(""));

				HANDLE hContact = data->ppro->JabberHContactFromJID(buf);
				if ( hContact ) {
					int hItem = SendDlgItemMessage( hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0 );
					if ( hItem )
						SendDlgItemMessage( hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, hItem, 1 );
					break;
				}

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
						if ( !lstrcmpA(proto, data->ppro->m_szProtoName) && !DBGetContactSettingByte(hContact, proto, "ChatRoom", 0)) {
							if (int hItem = SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0)) {
								if ( SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0 )) {
									DBVARIANT dbv={0};
									data->ppro->JGetStringT(hContact, "jid", &dbv);
									if (dbv.ptszVal && ( dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_WCHAR ))
										InviteUser(data->ppro, room, dbv.ptszVal, text);
									JFreeVariant(&dbv);
					}	}	}	}

					// invite others
					for (int i = 0; i < data->newJids.getCount(); ++i)
						if (SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)data->newJids[i]->hItem, 0))
							InviteUser(data->ppro, room, data->newJids[i]->jid, text);
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
				if ( data )
					FilterList(data->ppro, GetDlgItem(hwndDlg,IDC_CLIST));
				break;
			case CLN_LISTREBUILT:
				if ( data )
					FilterList(data->ppro, GetDlgItem(hwndDlg,IDC_CLIST));
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
		delete data;
		SetWindowLong( hwndDlg, GWL_USERDATA, NULL );
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Context menu processing

void CJabberProto::JabberAdminSet( const TCHAR* to, const char* ns, const char* szItem, const TCHAR* itemVal, const char* var, const TCHAR* varVal )
{
	XmlNodeIq iq( "set", NOID, to );
	XmlNode* query = iq.addQuery( ns );
	XmlNode* item = query->addChild( "item" ); item->addAttr( szItem, itemVal ); item->addAttr( var, varVal );
	jabberThreadInfo->send( iq );
}

void CJabberProto::JabberAdminGet( const TCHAR* to, const char* ns, const char* var, const TCHAR* varVal, JABBER_IQ_PFUNC foo )
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
	CJabberProto* ppro;
	JABBER_LIST_ITEM *item;
	JABBER_RESOURCE_STATUS *me, *him;
};

static LRESULT CALLBACK sttUserInfoDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TUserInfoData *dat = (TUserInfoData *)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
	{
		int i, idx;
		TCHAR buf[256];

		TranslateDialogDefault(hwndDlg);

		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		dat = (TUserInfoData *)lParam;

		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)dat->ppro->LoadIconEx("group"));

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

		SendDlgItemMessage(hwndDlg, IDC_ICO_STATUS, STM_SETICON, (WPARAM)LoadSkinnedProtoIcon(dat->ppro->m_szProtoName, dat->him->status), 0);

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
		if (!dat)break;

		switch ( LOWORD( wParam )) {
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
						dat->ppro->JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "affiliation", _T("none"));
						break;
					case AFFILIATION_MEMBER:
						dat->ppro->JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "affiliation",  _T("member"));
						break;
					case AFFILIATION_ADMIN:
						dat->ppro->JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "affiliation", _T("admin"));
						break;
					case AFFILIATION_OWNER:
						dat->ppro->JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "affiliation", _T("owner"));
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

				switch (value) {
				case ROLE_VISITOR:
					dat->ppro->JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "role", _T("visitor"));
					break;
				case ROLE_PARTICIPANT:
					dat->ppro->JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "role", _T("participant"));
					break;
				case ROLE_MODERATOR:
					dat->ppro->JabberAdminSet(dat->item->jid, xmlnsAdmin, "nick", dat->him->resourceName, "role", _T("moderator"));
					break;
				}
			}
			break;
		}
		break;

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

static void sttNickListHook( CJabberProto* ppro, JABBER_LIST_ITEM* item, GCHOOK* gch )
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

	if ((gch->dwData >= CLISTMENUIDMIN) && (gch->dwData <= CLISTMENUIDMAX))
	{
		if (him->szRealJid && *him->szRealJid)
			if (HANDLE hContact = ppro->JabberHContactFromJID(him->szRealJid))
				CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(gch->dwData, MPCF_CONTACTMENU), (LPARAM)hContact);
		return;
	}

	switch( gch->dwData ) {
	case IDM_SLAP:
	{
		if ( ppro->jabberOnline ) {
			DBVARIANT dbv = {0};
			TCHAR *szMessage = DBGetContactSettingTString( NULL, ppro->m_szProtoName, "GcMsgSlap", &dbv) ?
				NEWTSTR_ALLOCA(TranslateTS(JABBER_GC_MSG_SLAP)) : dbv.ptszVal;

			TCHAR buf[256];
			// do not use snprintf to avoid possible problems with % symbol
			if (TCHAR *p = _tcsstr(szMessage, _T("%s")))
			{
				*p = 0;
				mir_sntprintf(buf, SIZEOF(buf), _T("%s%s%s"), szMessage, him->resourceName, p+2);
			} else
			{
				lstrcpyn(buf, szMessage, SIZEOF(buf));
			}

			XmlNode m( "message" ); m.addAttr( "to", item->jid ); m.addAttr( "type", "groupchat" );
			XmlNode* b = m.addChild( "body", buf );
			if ( b->sendText != NULL )
				UnEscapeChatTags( b->sendText );
			ppro->jabberThreadInfo->send( m );

			if (szMessage == dbv.ptszVal)
				DBFreeVariant(&dbv);
		}
		break;
	}
	case IDM_VCARD:
	{
		HANDLE hContact;
		JABBER_SEARCH_RESULT jsr;
		mir_sntprintf(jsr.jid, SIZEOF(jsr.jid), _T("%s/%s"), item->jid, him->resourceName );
		jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
		
		JABBER_LIST_ITEM* item = ppro->JabberListAdd( LIST_VCARD_TEMP, jsr.jid );
		item->bUseResource = TRUE;
		ppro->JabberListAddResource( LIST_VCARD_TEMP, jsr.jid, him->status, him->statusMessage, him->priority );

		hContact = ( HANDLE )CallProtoService( ppro->m_szProtoName, PS_ADDTOLIST, PALF_TEMPORARY, ( LPARAM )&jsr );
		CallService( MS_USERINFO_SHOWDIALOG, ( WPARAM )hContact, 0 );
		break;
	}
	case IDM_INFO:
	{
		TUserInfoData *dat = (TUserInfoData *)mir_alloc(sizeof(TUserInfoData));
		dat->me = me;
		dat->him = him;
		dat->item = item;
		dat->ppro = ppro;
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
			TCHAR *resourceName_copy = mir_tstrdup(him->resourceName); // copy resource name to prevent possible crash if user list rebuilds
			if ( ppro->JabberEnterString(szBuffer, SIZEOF(szBuffer), NULL, JES_MULTINE, "gcReason_" ))
			{
				XmlNodeIq iq( "set", NOID, item->jid );
				XmlNode* query = iq.addQuery( xmlnsAdmin );
				XmlNode* item = query->addChild( "item" ); item->addAttr( "nick", resourceName_copy ); item->addAttr( "role", "none" );
				item->addChild( "reason", szBuffer );
				ppro->jabberThreadInfo->send( iq );
			}
			mir_free(resourceName_copy);
		}
		dwLastBanKickTime = GetTickCount();
		break;
	}

	case IDM_SET_VISITOR:
		if (him->role != ROLE_VISITOR)
			ppro->JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "role", _T("visitor"));
		break;
	case IDM_SET_PARTICIPANT:
		if (him->role != ROLE_PARTICIPANT)
			ppro->JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "role", _T("participant"));
		break;
	case IDM_SET_MODERATOR:
		if (him->role != ROLE_MODERATOR)
			ppro->JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "role", _T("moderator"));
		break;

	case IDM_SET_NONE:
		if (him->affiliation != AFFILIATION_NONE)
			ppro->JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "affiliation", _T("none"));
		break;
	case IDM_SET_MEMBER:
		if (him->affiliation != AFFILIATION_MEMBER)
			ppro->JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "affiliation",  _T("member"));
		break;
	case IDM_SET_ADMIN:
		if (him->affiliation != AFFILIATION_ADMIN)
			ppro->JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "affiliation", _T("admin"));
		break;
	case IDM_SET_OWNER:
		if (him->affiliation != AFFILIATION_OWNER)
			ppro->JabberAdminSet(item->jid, xmlnsAdmin, "nick", him->resourceName, "affiliation", _T("owner"));
		break;

	case IDM_SET_BAN:
		if ((GetTickCount() - dwLastBanKickTime) > BAN_KICK_INTERVAL)
		{
			dwLastBanKickTime = GetTickCount();
			mir_sntprintf( szBuffer, SIZEOF(szBuffer), _T("%s %s"), TranslateT( "Reason to ban" ), him->resourceName );
			TCHAR *resourceName_copy = mir_tstrdup(him->resourceName); // copy resource name to prevent possible crash if user list rebuilds
			if ( ppro->JabberEnterString(szBuffer, SIZEOF(szBuffer), NULL, JES_MULTINE, "gcReason_" ))
			{
				XmlNodeIq iq( "set", NOID, item->jid );
				XmlNode* query = iq.addQuery( xmlnsAdmin );
				XmlNode* item = query->addChild( "item" ); item->addAttr( "nick", resourceName_copy ); item->addAttr( "affiliation", "outcast" );
				item->addChild( "reason", szBuffer );
				ppro->jabberThreadInfo->send( iq );
			}
			mir_free(resourceName_copy);
		}
		dwLastBanKickTime = GetTickCount();
		break;

	case IDM_CPY_NICK:
		JabberCopyText((HWND)CallService(MS_CLUI_GETHWND, 0, 0), him->resourceName);
		break;
	case IDM_RJID_COPY:
	case IDM_CPY_RJID:
		JabberCopyText((HWND)CallService(MS_CLUI_GETHWND, 0, 0), him->szRealJid);
		break;

	case IDM_RJID_VCARD:
		if (him->szRealJid && *him->szRealJid)
		{
			HANDLE hContact;
			JABBER_SEARCH_RESULT jsr;
			jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
			mir_sntprintf(jsr.jid, SIZEOF(jsr.jid), _T("%s"), him->szRealJid);
			if (TCHAR *tmp = _tcsrchr(jsr.jid, _T('/'))) *tmp = 0;
			
			JABBER_LIST_ITEM* item = ppro->JabberListAdd( LIST_VCARD_TEMP, jsr.jid );
			item->bUseResource = TRUE;
			ppro->JabberListAddResource( LIST_VCARD_TEMP, jsr.jid, him->status, him->statusMessage, him->priority );

			hContact = ( HANDLE )CallProtoService( ppro->m_szProtoName, PS_ADDTOLIST, PALF_TEMPORARY, ( LPARAM )&jsr );
			CallService( MS_USERINFO_SHOWDIALOG, ( WPARAM )hContact, 0 );
			break;
		}

	case IDM_RJID_ADD:
		if (him->szRealJid && *him->szRealJid)
		{
			JABBER_SEARCH_RESULT jsr={0};
			jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
			mir_sntprintf(jsr.jid, SIZEOF(jsr.jid), _T("%s"), him->szRealJid);
			if (TCHAR *tmp = _tcsrchr(jsr.jid, _T('/'))) *tmp = 0;

			ADDCONTACTSTRUCT acs={0};
			acs.handleType = HANDLE_SEARCHRESULT;
			acs.szProto = ppro->m_szProtoName;
			acs.psr = (PROTOSEARCHRESULT *)&jsr;
			CallService(MS_ADDCONTACT_SHOW, (WPARAM)CallService(MS_CLUI_GETHWND, 0, 0), (LPARAM)&acs);
			break;
		}
	}
}

static void sttLogListHook( CJabberProto* ppro, JABBER_LIST_ITEM* item, GCHOOK* gch )
{
	TCHAR szBuffer[ 1024 ];
	TCHAR szCaption[ 1024 ];
	szBuffer[ 0 ] = _T('\0');

	switch( gch->dwData ) {
	case IDM_LST_PARTICIPANT:
		ppro->JabberAdminGet(gch->pDest->ptszID, xmlnsAdmin, "role", _T("participant"), &CJabberProto::JabberIqResultMucGetVoiceList );
		break;

	case IDM_LST_MEMBER:
		ppro->JabberAdminGet(gch->pDest->ptszID, xmlnsAdmin, "affiliation", _T("member"), &CJabberProto::JabberIqResultMucGetMemberList );
		break;

	case IDM_LST_MODERATOR:
		ppro->JabberAdminGet(gch->pDest->ptszID, xmlnsAdmin, "role", _T("moderator"), &CJabberProto::JabberIqResultMucGetModeratorList );
		break;

	case IDM_LST_BAN:
		ppro->JabberAdminGet(gch->pDest->ptszID, xmlnsAdmin, "affiliation", _T("outcast"), &CJabberProto::JabberIqResultMucGetBanList );
		break;

	case IDM_LST_ADMIN:
		ppro->JabberAdminGet(gch->pDest->ptszID, xmlnsAdmin, "affiliation", _T("admin"), &CJabberProto::JabberIqResultMucGetAdminList );
		break;

	case IDM_LST_OWNER:
		ppro->JabberAdminGet(gch->pDest->ptszID, xmlnsAdmin, "affiliation", _T("owner"), &CJabberProto::JabberIqResultMucGetOwnerList );
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
		if ( ppro->JabberEnterString( szTmpBuff, SIZEOF(szTmpBuff), szCaption, JES_RICHEDIT, "gcTopic_" )) {
			XmlNode msg( "message" ); msg.addAttr( "to", gch->pDest->ptszID ); msg.addAttr( "type", "groupchat" );
			msg.addChild( "subject", szTmpBuff );
			ppro->jabberThreadInfo->send( msg );
		}
		break;

	case IDM_NICK:
		mir_sntprintf( szCaption, SIZEOF(szCaption), _T("%s %s"), TranslateT( "Change nickname in" ), gch->pDest->ptszID );
		if ( item->nick )
			mir_sntprintf( szBuffer, SIZEOF(szBuffer), _T("%s"), item->nick );
		if ( ppro->JabberEnterString(szBuffer, SIZEOF(szBuffer), szCaption, JES_COMBO, "gcNick_" )) {
			JABBER_LIST_ITEM* item = ppro->JabberListGetItemPtr( LIST_CHATROOM, gch->pDest->ptszID );
			if ( item != NULL ) {
				TCHAR text[ 1024 ];
				mir_sntprintf( text, SIZEOF( text ), _T("%s/%s"), gch->pDest->ptszID, szBuffer );
				ppro->JabberSendPresenceTo( ppro->m_iStatus, text, NULL );
		}	}
		break;

	case IDM_INVITE:
	{	
		JabberGcLogInviteDlgData* param = new JabberGcLogInviteDlgData( gch->pDest->ptszID );
		param->ppro = ppro;
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_INVITE ), NULL, JabberGcLogInviteDlgProc, ( LPARAM )param );
		break;	
	}

	case IDM_CONFIG:
	{
		int iqId = ppro->JabberSerialNext();
		ppro->JabberIqAdd( iqId, IQ_PROC_NONE, &CJabberProto::JabberIqResultGetMuc );

		XmlNodeIq iq( "get", iqId, gch->pDest->ptszID );
		XmlNode* query = iq.addQuery( xmlnsOwner );
		ppro->jabberThreadInfo->send( iq );
		break;
	}
	case IDM_BOOKMARKS:
	{
		JABBER_LIST_ITEM* item = ppro->JabberListGetItemPtr( LIST_BOOKMARK, gch->pDest->ptszID );
		if ( item == NULL ) {
			item = ppro->JabberListGetItemPtr( LIST_CHATROOM, gch->pDest->ptszID );
			if (item != NULL) {
				item->type = _T("conference");
				HANDLE hContact = ppro->JabberHContactFromJID( item->jid );
				item->name = ( TCHAR* )JCallService( MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, GCDNF_TCHAR );
				ppro->JabberAddEditBookmark( item );
			}
		}
		break;
	}
	case IDM_DESTROY:
		mir_sntprintf( szBuffer, SIZEOF(szBuffer), _T("%s %s"), TranslateT( "Reason to destroy" ), gch->pDest->ptszID );
		if ( !ppro->JabberEnterString(szBuffer, SIZEOF(szBuffer), NULL, JES_MULTINE, "gcReason_" ))
			break;

		{	XmlNodeIq iq( "set", NOID, gch->pDest->ptszID );
			XmlNode* query = iq.addQuery( xmlnsOwner );
			query->addChild( "destroy" )->addChild( "reason", szBuffer );
			ppro->jabberThreadInfo->send( iq );
		}

	case IDM_LEAVE:
		ppro->JabberGcQuit( item, 0, 0 );
		break;

	case IDM_LINK0: case IDM_LINK1: case IDM_LINK2: case IDM_LINK3: case IDM_LINK4:
	case IDM_LINK5: case IDM_LINK6: case IDM_LINK7: case IDM_LINK8: case IDM_LINK9:
	{
		int idx = IDM_LINK0;
		for (TCHAR *p = _tcsstr(item->itemResource.statusMessage, _T("http://")); p && *p; p = _tcsstr(p+1, _T("http://")))
		{
			if (idx == gch->dwData)
			{
				char *bufPtr, *url = mir_t2a(p);
				for (bufPtr = url; *bufPtr && !isspace(*bufPtr); ++bufPtr) ;
				*bufPtr++ = 0;
				CallService(MS_UTILS_OPENURL, 1, (LPARAM)url);
				mir_free(url);
				break;
			}

			if (++idx > IDM_LINK9) break;
		}

		break;
	}

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

static void sttSendPrivateMessage( CJabberProto* ppro, JABBER_LIST_ITEM* item, const TCHAR* nick )
{
	TCHAR szFullJid[ 256 ];
	mir_sntprintf( szFullJid, SIZEOF(szFullJid), _T("%s/%s"), item->jid, nick );
	HANDLE hContact = ppro->JabberDBCreateContact( szFullJid, NULL, TRUE, FALSE );
	if ( hContact != NULL ) {
		for ( int i=0; i < item->resourceCount; i++ ) {
			if ( _tcsicmp( item->resource[i].resourceName, nick ) == 0 ) {
				ppro->JSetWord( hContact, "Status", item->resource[i].status );
				break;
		}	}

		DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
		ppro->JSetStringT( hContact, "Nick", nick );
		DBWriteContactSettingDword( hContact, "Ignore", "Mask1", 0 );
		JCallService( MS_MSG_SENDMESSAGE, ( WPARAM )hContact, 0 );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// General chat event processing hook

int CJabberProto::JabberGcEventHook(WPARAM wParam,LPARAM lParam)
{
	GCHOOK* gch = ( GCHOOK* )lParam;
	if ( gch == NULL )
		return 0;

	if ( lstrcmpiA( gch->pDest->pszModule, m_szProtoName ))
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
		sttSendPrivateMessage( this, item, gch->ptszUID );
		break;

	case GC_USER_LOGMENU:
		sttLogListHook( this, item, gch );
		break;

	case GC_USER_NICKLISTMENU:
		sttNickListHook( this, item, gch );
		break;

	case GC_USER_CHANMGR:
		int iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, &CJabberProto::JabberIqResultGetMuc );

		XmlNodeIq iq( "get", iqId, item->jid );
		XmlNode* query = iq.addQuery( xmlnsOwner );
		jabberThreadInfo->send( iq );
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CJabberProto::JabberAddMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* str )
{
	const char* field = _tcschr(str,'@') ? "jid" : "nick";
	TCHAR* roomJid = jidListInfo->roomJid;

	switch (jidListInfo->type) {
	case MUC_VOICELIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "role", _T("participant"));
		JabberAdminGet( roomJid, xmlnsAdmin, "role", _T("participant"), &CJabberProto::JabberIqResultMucGetVoiceList);
		break;
	case MUC_MEMBERLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "affiliation", _T("member"));
		JabberAdminGet( roomJid, xmlnsAdmin, "affiliation", _T("member"), &CJabberProto::JabberIqResultMucGetMemberList);
		break;
	case MUC_MODERATORLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "role", _T("moderator"));
		JabberAdminGet( roomJid, xmlnsAdmin, "role", _T("moderator"), &CJabberProto::JabberIqResultMucGetModeratorList);
		break;
	case MUC_BANLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "affiliation", _T("outcast"));
		JabberAdminGet( roomJid, xmlnsAdmin, "affiliation", _T("outcast"), &CJabberProto::JabberIqResultMucGetBanList);
		break;
	case MUC_ADMINLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "affiliation", _T("admin"));
		JabberAdminGet( roomJid, xmlnsAdmin, "affiliation", _T("admin"), &CJabberProto::JabberIqResultMucGetAdminList);
		break;
	case MUC_OWNERLIST:
		JabberAdminSet( roomJid, xmlnsAdmin, field, str, "affiliation", _T("owner"));
		JabberAdminGet( roomJid, xmlnsAdmin, "affiliation", _T("owner"), &CJabberProto::JabberIqResultMucGetOwnerList);
		break;
}	}

void CJabberProto::JabberDeleteMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* jid )
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
