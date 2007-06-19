////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"

static HANDLE hHookGCUserEvent = NULL;
static HANDLE hHookGCMenuBuild = NULL;
static HANDLE hEventGGGetChat = NULL;

#define GG_GC_GETCHAT "%s/GCGetChat"
#define GGS_OPEN_CONF "%s/OpenConf"
#define GGS_CLEAR_IGNORED "%s/ClearIgnored"

int ggGCEnabled = FALSE;
int ggGCID = 0;

int gg_gc_menu(WPARAM wParam, LPARAM lParam);
int gg_gc_event(WPARAM wParam, LPARAM lParam);
int gg_gc_egetchat(WPARAM wParam, LPARAM lParam);
int gg_gc_openconf(WPARAM wParam, LPARAM lParam);
int gg_gc_clearignored(WPARAM wParam, LPARAM lParam);

list_t ggGCList = NULL;

////////////////////////////////////////////////////////////////////////////////
// Inits Gadu-Gadu groupchat module using chat.dll

int gg_gc_init()
{
	// Chat.dll required Miranda version 0.4 or higher
	if(gMirandaVersion && gMirandaVersion >= PLUGIN_MAKE_VERSION(0, 4, 0, 0) && ServiceExists(MS_GC_REGISTER))
	{
		char service[64];
		GCREGISTER gcr = {0};
		CLISTMENUITEM mi;

		// Register Gadu-Gadu proto
		gcr.cbSize = sizeof(GCREGISTER);
		gcr.dwFlags = 0;
		gcr.iMaxText = 0;
		gcr.nColors = 0;
		gcr.pColors = 0;
		gcr.pszModuleDispName = GG_PROTO;
		gcr.pszModule = GG_PROTO;
#ifdef DEBUGMODE
		gg_netlog("gg_gc_getchat(): Trying to register groupchat plugin...");
#endif
		CallService(MS_GC_REGISTER, 0, (LPARAM)&gcr);
		hHookGCUserEvent = HookEvent(ME_GC_EVENT, gg_gc_event);
		//hHookGCMenuBuild = HookEvent(ME_GC_BUILDMENU, gg_gc_menu);
		ggGCEnabled = TRUE;
		// create & hook event
		mir_snprintf(service, 64, GG_GC_GETCHAT, GG_PROTO);
		if(hEventGGGetChat = CreateHookableEvent(service))
			SetHookDefaultForHookableEvent(hEventGGGetChat, gg_gc_egetchat);
#ifdef DEBUGMODE
		else
			gg_netlog("gg_gc_getchat(): Cannot create event hook for room creation.");
		gg_netlog("gg_gc_getchat(): Registered with groupchat plugin.");
#endif

		ZeroMemory(&mi,sizeof(mi));
		mi.cbSize = sizeof(mi);

		// Conferencing
		mir_snprintf(service, sizeof(service), GGS_OPEN_CONF, GG_PROTO);
		CreateServiceFunction(service, gg_gc_openconf);
		mi.pszPopupName = GG_PROTONAME;
		mi.popupPosition = 500090000;
		mi.position = 500090000;
		mi.hIcon = LoadIconEx(IDI_CONFERENCE);
		mi.pszName = Translate("Open &conference...");
		mi.pszService = service;
		CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);

		mir_snprintf(service, sizeof(service), GGS_CLEAR_IGNORED, GG_PROTO);
		CreateServiceFunction(service, gg_gc_clearignored);
		mi.pszPopupName = GG_PROTONAME;
		mi.popupPosition = 500090000;
		mi.position = 500090000;
		mi.hIcon = NULL;
		mi.pszName = Translate("&Clear ignored conferences");
		mi.pszService = service;
		CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);
	}
#ifdef DEBUGMODE
	else
		gg_netlog("gg_gc_getchat(): Cannot register with groupchat plugin !!!");
#endif

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Releases Gadu-Gadu groupchat module using chat.dll
int gg_gc_destroy()
{
	list_t l;
	for(l = ggGCList; l; l = l->next)
	{
		GGGC *chat = (GGGC *)l->data;
		if(chat->recipients) free(chat->recipients);
	}
	list_destroy(ggGCList, 1); ggGCList = NULL;
	LocalEventUnhook(hHookGCUserEvent);
	LocalEventUnhook(hHookGCMenuBuild);
	if(hEventGGGetChat)
		DestroyHookableEvent(hEventGGGetChat);

	return 1;
}

int gg_gc_menu(WPARAM wParam, LPARAM lParam)
{
	return 1;
}

GGGC *gg_gc_lookup(char *id)
{
	GGGC *chat;
	list_t l;

	for(l = ggGCList; l; l = l->next)
	{
		chat = (GGGC *)l->data;
		if(chat && !strcmp(chat->id, id))
			return chat;
	}

	return NULL;
}

int gg_gc_event(WPARAM wParam, LPARAM lParam)
{
	GCHOOK *gch = (GCHOOK *)lParam;
	GGGC *chat = NULL;
	uin_t uin;

	// Check if we got our protocol, and fields are set
	if(!gch
		|| !gch->pDest
		|| !gch->pDest->pszID
		|| !gch->pDest->pszModule
		|| lstrcmpi(gch->pDest->pszModule, GG_PROTO)
		|| !(uin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0))
		|| !(chat = gg_gc_lookup(gch->pDest->pszID)))
		return 0;

	// Window terminated
	if(gch->pDest->iType == SESSION_TERMINATE)
	{
		HANDLE hContact = NULL;
#ifdef DEBUGMODE
		gg_netlog("gg_gc_event(): Terminating chat %x, id %s from chat window...", chat, gch->pDest->pszID);
#endif
		// Destroy chat entry
		free(chat->recipients);
		list_remove(&ggGCList, chat, 1);
		// Remove contact from contact list (duh!) should be done by chat.dll !!
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact)
		{
			DBVARIANT dbv;
			if(!DBGetContactSetting(hContact, GG_PROTO, "ChatRoomID", &dbv))
			{
				if(dbv.pszVal && !strcmp(gch->pDest->pszID, dbv.pszVal))
					CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
			}
			hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
		}
		return 1;
	}

	// Message typed / send only if online
	if(gg_isonline() && (gch->pDest->iType == GC_USER_MESSAGE) && gch->pszText)
	{
		char id[32];
		DBVARIANT dbv;
		GCDEST gcdest = {GG_PROTO, gch->pDest->pszID, GC_EVENT_MESSAGE};
		GCEVENT gcevent = {sizeof(GCEVENT), &gcdest};
		int lc;
		time_t t = time(NULL);

		UIN2ID(uin, id);

		gcevent.pszUID = id;
		gcevent.pszText = gch->pszText;
		if(!DBGetContactSetting(NULL, GG_PROTO, "Nick", &dbv))
			gcevent.pszNick = dbv.pszVal;
		else
			gcevent.pszNick = Translate("Me");

		// Get rid of CRLF at back
		lc = strlen(gch->pszText) - 1;
		while(lc >= 0 && (gch->pszText[lc] == '\n' || gch->pszText[lc] == '\r')) gch->pszText[lc --] = 0;
		gcevent.time = time(NULL);
		gcevent.bIsMe = 1;
		gcevent.dwFlags = GCEF_ADDTOLOG;
#ifdef DEBUGMODE
		gg_netlog("gg_gc_event(): Sending conference message to room %s, \"%s\".", gch->pDest->pszID, gch->pszText);
#endif
		CallService(MS_GC_EVENT, 0, (LPARAM)&gcevent);
		if(gcevent.pszNick == dbv.pszVal) DBFreeVariant(&dbv);
		gg_send_message_confer(ggThread->sess, GG_CLASS_CHAT, chat->recipients_count, chat->recipients, gch->pszText);
		return 1;
	}

	// Privmessage selected
	if(gch->pDest->iType == GC_USER_PRIVMESS)
	{
		HANDLE hContact = NULL;
		if((uin = atoi(gch->pszUID)) && (hContact = gg_getcontact(uin, 1, 0, NULL)))
			CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, (LPARAM)0);
	}

#ifdef DEBUGMODE
	gg_netlog("gg_gc_event(): Unhandled event %d, chat %x, uin %d, text \"%s\".", gch->pDest->iType, chat, uin, gch->pszText);
#endif

	return 0;
}

typedef struct _gg_gc_echat
{
	uin_t sender;
	uin_t *recipients;
	int recipients_count;
	char * chat_id;
} gg_gc_echat;

////////////////////////////////////////////////////////////////////////////////
// This is main groupchat initialization routine
int gg_gc_egetchat(WPARAM wParam, LPARAM lParam)
{
	gg_gc_echat *gc = (gg_gc_echat *)lParam;
	uin_t sender, *recipients;
	int recipients_count;
	list_t l; int i;
	GGGC *chat;
	char *senderName, status[256], *name, id[32];
	uin_t uin; DBVARIANT dbv;
	GCSESSION gcwindow;
	GCDEST gcdest = {GG_PROTO, 0, GC_EVENT_ADDGROUP};
	GCEVENT gcevent = {sizeof(GCEVENT), &gcdest};

	if(!gc)
	{
#ifdef DEBUGMODE
		gg_netlog("gg_gc_egetchat(): No valid event struct found.");
#endif
		return 0;
	}

	sender = gc->sender;
	recipients = gc->recipients;
	recipients_count = gc->recipients_count;

#ifdef DEBUGMODE
	gg_netlog("gg_gc_egetchat(): Count %d.", recipients_count);
#endif
	if(!recipients) return 0;

	// Look for existing chat
	for(l = ggGCList; l; l = l->next)
	{
		GGGC *chat = (GGGC *)l->data;
		if(!chat) continue;

		if(chat->recipients_count == recipients_count + (sender ? 1 : 0))
		{
			int i, j, found = 0, sok = (sender == 0);
			if(!sok) for(i = 0; i < chat->recipients_count; i++)
				if(sender == chat->recipients[i])
				{
					sok = 1;
					break;
				}
			if(sok)
				for(i = 0; i < chat->recipients_count; i++)
					for(j = 0; j < recipients_count; j++)
						if(recipients[j] == chat->recipients[i]) found++;
			// Found all recipients
			if(found == recipients_count)
			{
#ifdef DEBUGMODE
				if(chat->ignore)
					gg_netlog("gg_gc_egetchat(): Ignoring existing id %s, size %d.", chat->id, chat->recipients_count);
				else
					gg_netlog("gg_gc_egetchat(): Returning existing id %s, size %d.", chat->id, chat->recipients_count);
#endif
				gc->chat_id = chat->id;
				return !(chat->ignore);
			}
		}
	}

	// Make new uin list to chat mapping
	chat = (GGGC *)malloc(sizeof(GGGC));
	UIN2ID(ggGCID ++, chat->id); chat->ignore = FALSE;

	// Check groupchat policy (new) / only for incoming
	if(sender)
	{
		int unknown = (gg_getcontact(sender, 0, 0, NULL) == NULL),
			unknownSender = unknown;
		for(i = 0; i < recipients_count; i++)
			if(!gg_getcontact(recipients[i], 0, 0, NULL))
				unknown ++;
		if((DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_DEFAULT, GG_KEYDEF_GC_POLICY_DEFAULT) == 2) ||
		   (DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_TOTAL, GG_KEYDEF_GC_POLICY_TOTAL) == 2 &&
			recipients_count >= DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_COUNT_TOTAL, GG_KEYDEF_GC_COUNT_TOTAL)) ||
		   (DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_UNKNOWN, GG_KEYDEF_GC_POLICY_UNKNOWN) == 2 &&
			unknown >= DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_COUNT_UNKNOWN, GG_KEYDEF_GC_COUNT_UNKNOWN)))
			chat->ignore = TRUE;
		if(!chat->ignore && ((DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_DEFAULT, GG_KEYDEF_GC_POLICY_DEFAULT) == 1) ||
		   (DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_TOTAL, GG_KEYDEF_GC_POLICY_TOTAL) == 1 &&
			recipients_count >= DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_COUNT_TOTAL, GG_KEYDEF_GC_COUNT_TOTAL)) ||
		   (DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_POLICY_UNKNOWN, GG_KEYDEF_GC_POLICY_UNKNOWN) == 1 &&
			unknown >= DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_GC_COUNT_UNKNOWN, GG_KEYDEF_GC_COUNT_UNKNOWN))))
		{
			char *senderName = unknownSender ?
				Translate("Unknown") : ((char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) gg_getcontact(sender, 0, 0, NULL), 0));
			char error[256];
			mir_snprintf(error, sizeof(error), Translate("%s has initiated conference with %d participants (%d unknowns).\nDo you want do participate ?"),
				senderName, recipients_count + 1, unknown);
			chat->ignore = (MessageBox(
				NULL,
				error,
				GG_PROTONAME,
				MB_OKCANCEL | MB_ICONEXCLAMATION
				) != IDOK);
		}
		if(chat->ignore)
		{
			// Copy recipient list
			chat->recipients_count = recipients_count + (sender ? 1 : 0);
			chat->recipients = (uin_t *)calloc(chat->recipients_count, sizeof(uin_t));
			for(i = 0; i < recipients_count; i++)
				chat->recipients[i] = recipients[i];
			if(sender) chat->recipients[i] = sender;
	#ifdef DEBUGMODE
			gg_netlog("gg_gc_egetchat(): Ignoring new chat %s, count %d.", chat->id, chat->recipients_count);
	#endif
			list_add(&ggGCList, chat, 0);
			gc->chat_id = chat->id;
			return 0;
		}
	}

	// Create new chat window
	senderName = sender ? (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) gg_getcontact(sender, 1, 0, NULL), 0) : NULL;
	mir_snprintf(status, 255, sender ? Translate("%s initiated the conference.") : Translate("This is my own conference."), senderName);
	gcwindow.cbSize		= sizeof(GCSESSION);
	gcwindow.iType		= GCW_CHATROOM;
	gcwindow.pszModule	= GG_PROTO;
	gcwindow.pszName	= sender ? senderName : Translate("Conference");
	gcwindow.pszID		= chat->id;
	gcwindow.pszStatusbarText = status;
	gcwindow.dwFlags = 0;
	gcwindow.dwItemData = (DWORD)chat;

	// Here we put nice new hash sign
	name = calloc(strlen(gcwindow.pszName) + 2, sizeof(char));
	*name = '#'; strcpy(name + 1, gcwindow.pszName);
	gcwindow.pszName = name;
	// Create new room
	if(CallService(MS_GC_NEWSESSION, 0, (LPARAM) &gcwindow))
	{
#ifdef DEBUGMODE
		gg_netlog("gg_gc_egetchat(): Cannot create new chat window %s.", chat->id);
#endif
		free(name);
		free(chat);
		return 0;
	}
	free(name);

	gcdest.pszID = chat->id;
	gcevent.pszUID = id;
	gcevent.dwFlags = GCEF_ADDTOLOG;
	gcevent.time = 0;

	// Add normal group
	gcevent.pszStatus = Translate("Participants");
	CallService(MS_GC_EVENT, 0, (LPARAM)&gcevent);
	gcdest.iType = GC_EVENT_JOIN;

	// Add myself
	if(uin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0))
	{
		UIN2ID(uin, id);
		if(!DBGetContactSetting(NULL, GG_PROTO, "Nick", &dbv))
			gcevent.pszNick = dbv.pszVal;
		else
			gcevent.pszNick = Translate("Me");
		gcevent.bIsMe = 1;
		CallService(MS_GC_EVENT, 0, (LPARAM)&gcevent);
#ifdef DEBUGMODE
		gg_netlog("gg_gc_egetchat(): Myself %s: %s (%s) to the list...", gcevent.pszUID, gcevent.pszNick, gcevent.pszStatus);
#endif
		if(gcevent.pszNick == dbv.pszVal) DBFreeVariant(&dbv);
	}
#ifdef DEBUGMODE
	else
		gg_netlog("gg_gc_egetchat(): Myself adding failed with uin %d !!!", uin);
#endif

	// Copy recipient list
	chat->recipients_count = recipients_count + (sender ? 1 : 0);
	chat->recipients = (uin_t *)calloc(chat->recipients_count, sizeof(uin_t));
	for(i = 0; i < recipients_count; i++)
		chat->recipients[i] = recipients[i];
	if(sender) chat->recipients[i] = sender;

	// Add contacts
	for(i = 0; i < chat->recipients_count; i++)
	{
		HANDLE hContact = gg_getcontact(chat->recipients[i], 1, 0, NULL);
		UIN2ID(chat->recipients[i], id);
		if(hContact && (name = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0)))
			gcevent.pszNick = name;
		else
			gcevent.pszNick = Translate("'Unknown'");
		gcevent.bIsMe = 0;
#ifdef DEBUGMODE
		gg_netlog("gg_gc_egetchat(): Added %s: %s (%s) to the list...", gcevent.pszUID, gcevent.pszNick, gcevent.pszStatus);
#endif
		CallService(MS_GC_EVENT, 0, (LPARAM)&gcevent);
	}
	gcdest.iType = GC_EVENT_CONTROL;
	CallService(MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gcevent);
	CallService(MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gcevent);

#ifdef DEBUGMODE
	gg_netlog("gg_gc_egetchat(): Returning new chat window %s, count %d.", chat->id, chat->recipients_count);
#endif
	list_add(&ggGCList, chat, 0);
	gc->chat_id = chat->id;
	return 1;
}

char * gg_gc_getchat(uin_t sender, uin_t *recipients, int recipients_count)
{
	gg_gc_echat gc = {sender, recipients, recipients_count, NULL};
#ifdef DEBUGMODE
	gg_netlog("gg_gc_getchat(): Calling hooked event.");
#endif
	if(!NotifyEventHooks(hEventGGGetChat, 0, (LPARAM)&gc))
	{
#ifdef DEBUGMODE
		gg_netlog("gg_gc_getchat(): Hooked returned zero.");
#endif
		return NULL;
	}
	return gc.chat_id;
}

static BOOL CALLBACK gg_gc_openconfdlg(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	// static HANDLE hItemAll;

	switch(message)
	{

	case WM_INITDIALOG:
		{
			CLCINFOITEM cii = {0};
			LOGFONT lf;
			HFONT hBoldFont, hNormalFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_NAME, WM_GETFONT, 0, 0);

			TranslateDialogDefault(hwndDlg);

			// Add the "All contacts" item
			//~ cii.cbSize = sizeof(cii);
			//~ cii.flags = CLCIIF_GROUPFONT | CLCIIF_CHECKBOX;
			//~ cii.pszText = Translate("** All contacts **");
			//~ hItemAll = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM)&cii);

			// Make bold title font
			GetObject(hNormalFont, sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;
			hBoldFont = CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg, IDC_NAME, WM_SETFONT, (WPARAM)hBoldFont, 0);
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)hBoldFont);
		}

		return TRUE;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{

			case IDOK:
				{
					HWND hwndList = GetDlgItem(hwndDlg, IDC_CLIST);
					int count = 0, i = 0;
					// Check if connected
					if (!gg_isonline())
					{
						MessageBox(NULL,
							Translate("You have to be connected to open new conference."),
							GG_PROTOERROR, MB_OK | MB_ICONSTOP
						);
					}
					else if(hwndList)
					{
						// Check count of contacts
						HANDLE hItem, hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
						while (hContact)
						{
							hItem = (HANDLE)SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
							if (hItem && SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0) &&
									DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0))
								count++;
							hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
						}

						// Create new participiants table
						if(count)
						{
							uin_t *participants = calloc(count, sizeof(uin_t));
#ifdef DEBUGMODE
							gg_netlog("gg_gc_getchat(): Opening new conference for %d contacts.", count);
#endif
							hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
							while (hContact && i < count)
							{
								hItem = (HANDLE)SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
								if (hItem && SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0))
									participants[i++] = DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0);
								hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
							}
							if(count > i) i = count;
							gg_gc_getchat(0, participants, count);
							free(participants);
						}
					}
				}

			case IDCANCEL:
				DestroyWindow(hwndDlg);
				break;

			}
		}
		break;

	case WM_NOTIFY:
		{
			switch(((NMHDR*)lParam)->idFrom)
			{

			case IDC_CLIST:
				{
					switch(((NMHDR*)lParam)->code)
					{

					case CLN_OPTIONSCHANGED:
						{
							int i;

							SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETLEFTMARGIN, 2, 0);
							SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKBITMAP, 0, (LPARAM)(HBITMAP)NULL);
							SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKCOLOR, GetSysColor(COLOR_WINDOW), 0);
							SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETGREYOUTFLAGS, 0, 0);
							for (i=0; i<=FONTID_MAX; i++)
								SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETTEXTCOLOR, i, GetSysColor(COLOR_WINDOWTEXT));
						}
						break;

					case CLN_LISTREBUILT:
						{
							HANDLE hContact;
							HANDLE hItem;
							char* szProto;

							// Delete non-icq contacts
							hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
							while (hContact)
							{
								hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0);
								if (hItem)
								{
									szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
									if (szProto == NULL || lstrcmp(szProto, GG_PROTO) || !DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0))
										SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_DELETEITEM, (WPARAM)hItem, 0);
								}
								hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
							}
						}
						break;

					case CLN_CHECKCHANGED:
						{
							HANDLE hItem, hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
							HWND hwndList = GetDlgItem(hwndDlg, IDC_CLIST);
							int count = 0;
							while (hContact && hwndList)
							{
								hItem = (HANDLE)SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
								if (hItem && SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0) &&
										DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0))
									count++;
								hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
							}
							EnableWindow(GetDlgItem(hwndDlg, IDOK), count >= 2);
						}
						break;

					}
				}
				break;

			}
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;

	case WM_DESTROY:
		{
			HFONT hBoldFont = (HFONT)GetWindowLong(hwndDlg, GWL_USERDATA);
			if(hBoldFont) DeleteObject(hBoldFont);
		}
		break;

	case WM_CTLCOLORSTATIC:
		if((GetDlgItem(hwndDlg, IDC_WHITERECT) == (HWND)lParam) ||
			(GetDlgItem(hwndDlg, IDC_LOGO) == (HWND)lParam))
		{
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
		}
		else if((GetDlgItem(hwndDlg, IDC_NAME) == (HWND)lParam) ||
			(GetDlgItem(hwndDlg, IDC_SUBNAME) == (HWND)lParam))
		{
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (BOOL)GetStockObject(NULL_BRUSH);
		}
		break;
	}


	return FALSE;

}

int gg_gc_clearignored(WPARAM wParam, LPARAM lParam)
{
	list_t l = ggGCList; BOOL cleared = FALSE;
	while(l)
	{
		GGGC *chat = (GGGC *)l->data;
		l = l->next;
		if(chat->ignore)
		{
			if(chat->recipients) free(chat->recipients);
			list_remove(&ggGCList, chat, 1);
			cleared = TRUE;
		}
	}
	MessageBox(
		NULL,
		cleared ?
			Translate("All ignored conferences are now unignored and the conference policy will act again.") :
			Translate("There are no ignored conferences."),
		GG_PROTONAME,
		MB_OK | MB_ICONINFORMATION
	);

	return 0;
}

int gg_gc_openconf(WPARAM wParam, LPARAM lParam)
{
	// Check if connected
	if (!gg_isonline())
	{
		MessageBox(NULL,
			Translate("You have to be connected to open new conference."),
			GG_PROTOERROR, MB_OK | MB_ICONSTOP
		);
		return 0;
	}

	CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONFERENCE), NULL, gg_gc_openconfdlg);
	return 1;
}

int gg_gc_changenick(HANDLE hContact, char *pszNick)
{
	list_t l;
	uin_t uin = DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0);
	if(!uin || !pszNick) return 0;

#ifdef DEBUGMODE
	gg_netlog("gg_gc_changenick(): Nickname for uin %d changed to %s.", uin, pszNick);
#endif

	// Lookup for chats having this nick
	for(l = ggGCList; l; l = l->next)
	{
		int i;
		GGGC *chat = (GGGC *)l->data;
		if(chat->recipients && chat->recipients_count)
			for(i = 0; i < chat->recipients_count; i++)
				// Rename this window if it's exising in the chat
				if(chat->recipients[i] == uin)
				{
					char id[32];
					GCEVENT gce = {sizeof(GCEVENT)};
					GCDEST gcd;

					UIN2ID(uin, id);
					gcd.iType = GC_EVENT_NICK;
					gcd.pszModule = GG_PROTO;
					gce.pDest = &gcd;
					gcd.pszID = chat->id;
					gce.pszUID = id;
					gce.pszText = pszNick;
#ifdef DEBUGMODE
					gg_netlog("gg_gc_changenick(): Found room %s with uin %d, sending nick change %s.", chat->id, uin, id);
#endif
					CallService(MS_GC_EVENT, 0, (LPARAM)&gce);

					break;
				}
	}

	return 1;
}
