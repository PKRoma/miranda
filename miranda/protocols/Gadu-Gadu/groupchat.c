////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003 Adam Strzelecki <ono+miranda@java.pl>
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

int ggGCEnabled = FALSE;
int ggGCID = 0;

int gg_gc_menu(WPARAM wParam, LPARAM lParam);
int gg_gc_event(WPARAM wParam, LPARAM lParam);
int gg_gc_egetchat(WPARAM wParam, LPARAM lParam);
int gg_gc_openconf(WPARAM wParam, LPARAM lParam);

list_t chats = NULL;

typedef struct _gg_gc_chat
{
    uin_t *recipients;
    int recipients_count;
    char id[32];
} gg_gc_chat;

////////////////////////////////////////////////////////////////////////////////
// Inits Gadu-Gadu groupchat module using chat.dll

int gg_gc_load()
{
    // Chat.dll required Miranda version 0.4 or higher
	if(gMirandaVersion && gMirandaVersion >= PLUGIN_MAKE_VERSION(0, 4, 0, 0) && ServiceExists(MS_GC_REGISTER))
	{
        // Register Gadu-Gadu proto
		GCREGISTER gcr = {0};
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
        char event[64]; snprintf(event, 64, GG_GC_GETCHAT, GG_PROTO);
        if(hEventGGGetChat = CreateHookableEvent(event))
            SetHookDefaultForHookableEvent(hEventGGGetChat, gg_gc_egetchat);
#ifdef DEBUGMODE
        else
            gg_netlog("gg_gc_getchat(): Cannot create event hook for room creation.");
        gg_netlog("gg_gc_getchat(): Registered with groupchat plugin.");
#endif

        CLISTMENUITEM mi;
        ZeroMemory(&mi,sizeof(mi));
        mi.cbSize = sizeof(mi);
        char service[64];

        // Import from server item
        snprintf(service, sizeof(service), GGS_OPEN_CONF, GG_PROTO);
        CreateServiceFunction(service, gg_gc_openconf);
        mi.pszPopupName = GG_PROTONAME;
        mi.popupPosition = 500090000;
        mi.position = 500090000;
        mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CONFERENCE));
        mi.pszName = Translate("Open &conference...");
        mi.pszService = service;
        CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);
	}
#ifdef DEBUGMODE
    else
        gg_netlog("gg_gc_getchat(): Cannot register with groupchat plugin !!!");
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Releases Gadu-Gadu groupchat module using chat.dll
int gg_gc_unload()
{
    list_t l;
    for(l = chats; l; l = l->next)
    {
        gg_gc_chat *chat = (gg_gc_chat *)l->data;
        if(chat->recipients) free(chat->recipients);
    }
    list_destroy(chats, 1); chats = NULL;
    LocalEventUnhook(hHookGCUserEvent);
    LocalEventUnhook(hHookGCMenuBuild);
    if(hEventGGGetChat) DestroyHookableEvent(hEventGGGetChat);
}

int gg_gc_menu(WPARAM wParam, LPARAM lParam)
{
    return 1;
}

int gg_gc_event(WPARAM wParam, LPARAM lParam)
{
    GCHOOK *gch = (GCHOOK *)lParam;
    if(!gch) return 0;
    gg_gc_chat *chat = NULL;

    uin_t uin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0);

    // Lookup for chat structure if not found
    if(!chat && gch->pDest->pszID)
    {
        list_t l; int i;
        for(l = chats; l; l = l->next)
        {
            gg_gc_chat *tchat = (gg_gc_chat *)l->data;
            if(tchat && !strcmp(tchat->id, gch->pDest->pszID))
            {
                chat = tchat; break;
            }
        }
    }

    // Window terminated
    if(gch->pDest && (gch->pDest->iType == GC_USER_TERMINATE) && gch->pDest->pszID)
    {
#ifdef DEBUGMODE
        gg_netlog("gg_gc_event(): Terminating window and chat %x, id %s...", chat, gch->pDest->pszID);
#endif
        // Remove contact from contact list (duh!) should be done by chat.dll !!
        HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
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
        if(chat)
        {
            free(chat->recipients);
            list_remove(&chats, chat, 1);
        }
        return 1;
    }

    // Message typed
    if(gch->pDest && gch->pDest->iType == GC_USER_MESSAGE && chat && uin && ggSess && gch->pszText)
    {
        char id[32]; UIN2ID(uin, id);
        DBVARIANT dbv;
        GCDEST gcdest = {GG_PROTO, gch->pDest->pszID, GC_EVENT_MESSAGE};
        GCEVENT gcevent = {sizeof(GCEVENT), &gcdest};
        gcevent.pszUID = id;
        gcevent.pszText = gch->pszText;
        if(!DBGetContactSetting(NULL, GG_PROTO, "Nick", &dbv))
            gcevent.pszNick = dbv.pszVal;
        else
            gcevent.pszNick = Translate("Me");
        // Get rid of CRLF at back
        int lc = strlen(gch->pszText) - 1;
        while(lc >= 0 && (gch->pszText[lc] == '\n' || gch->pszText[lc] == '\r')) gch->pszText[lc --] = 0;
        time_t t = time(NULL);
        gcevent.time = time(NULL);
        gcevent.bIsMe = 1;
        gcevent.bAddToLog = 1;
#ifdef DEBUGMODE
        gg_netlog("gg_gc_event(): Sending conference message to room %s, \"%s\".", gch->pDest->pszID, gch->pszText);
#endif
        CallService(MS_GC_EVENT, 0, (LPARAM)&gcevent);
        if(gcevent.pszNick == dbv.pszVal) DBFreeVariant(&dbv);
        gg_send_message_confer(ggSess, GG_CLASS_CHAT, chat->recipients_count, chat->recipients, gch->pszText);
        return 1;
    }

    // Privmessage selected
    if(gch->pDest && gch->pDest->iType == GC_USER_PRIVMESS && gch->pszUID)
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

int gg_gc_egetchat(WPARAM wParam, LPARAM lParam)
{
    gg_gc_echat *gc = (gg_gc_echat *)lParam;
    if(!gc)
    {
#ifdef DEBUGMODE
        gg_netlog("gg_gc_egetchat(): No valid event struct found.");
#endif
        return 0;
    }

    uin_t sender = gc->sender;
    uin_t *recipients = gc->recipients;
    int recipients_count = gc->recipients_count;

#ifdef DEBUGMODE
    gg_netlog("gg_gc_egetchat(): Count %d.", recipients_count);
#endif
    if(!recipients) return 0;
    // Look for existing chat
    list_t l; int i;
    for(l = chats; l; l = l->next)
    {
        gg_gc_chat *chat = (gg_gc_chat *)l->data;
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
                gg_netlog("gg_gc_egetchat(): Returning existing id %s, size %d.", chat->id, chat->recipients_count);
#endif
                gc->chat_id = chat->id;
                return 1;
            }
        }
    }

    // Make new uin list to chat mapping
    gg_gc_chat *chat = (gg_gc_chat *)malloc(sizeof(gg_gc_chat));
    UIN2ID(ggGCID ++, chat->id);

    // Create new chat window
    char *senderName = sender ? (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) gg_getcontact(sender, 1, 0, NULL), 0) : NULL;
    char status[255]; snprintf(status, 255, sender ? Translate("%s initiated the conference.") : Translate("This is my own conference."), senderName);
    GCWINDOW gcwindow = {sizeof(GCWINDOW),
        GCW_CHATROOM, GG_PROTO,
        sender ? senderName : NULL,
        chat->id, status, 0, (DWORD)chat};
    if(!gcwindow.pszName) gcwindow.pszName = Translate("Conference");
    // Here we put nice new hash sign
    char *name = calloc(strlen(gcwindow.pszName) + 2, sizeof(char));
    *name = '#'; strcpy(name + 1, gcwindow.pszName);
    gcwindow.pszName = name;
    // Create new room
    if(CallService(MS_GC_NEWCHAT, 0, (LPARAM) &gcwindow))
    {
#ifdef DEBUGMODE
        gg_netlog("gg_gc_egetchat(): Cannot create new chat window %s.", chat->id);
#endif
        free(name);
        free(chat);
        return 0;
    }
    free(name);
    char id[32]; uin_t uin; DBVARIANT dbv;
    GCDEST gcdest = {GG_PROTO, chat->id, GC_EVENT_ADDGROUP};
    GCEVENT gcevent = {sizeof(GCEVENT), &gcdest};
    gcevent.pszUID = id;
    gcevent.bAddToLog = FALSE;
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
    CallService(MS_GC_EVENT, WINDOW_INITDONE, (LPARAM)&gcevent);
    CallService(MS_GC_EVENT, WINDOW_ONLINE, (LPARAM)&gcevent);

#ifdef DEBUGMODE
    gg_netlog("gg_gc_egetchat(): Returning new chat window %s, count %d.", chat->id, chat->recipients_count);
#endif
    list_add(&chats, chat, 0);
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
			TranslateDialogDefault(hwndDlg);

			// Add the "All contacts" item
			//~ cii.cbSize = sizeof(cii);
			//~ cii.flags = CLCIIF_GROUPFONT | CLCIIF_CHECKBOX;
			//~ cii.pszText = Translate("** All contacts **");
			//~ hItemAll = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM)&cii);

            // Make bold title font
            LOGFONT lf;
            HFONT hNormalFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_NAME, WM_GETFONT, 0, 0);
            GetObject(hNormalFont, sizeof(lf), &lf);
            lf.lfWeight = FW_BOLD;
            HFONT hBoldFont = CreateFontIndirect(&lf);
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
                    if (!ggSess)
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
#ifdef DEBUGMODE
                            gg_netlog("gg_gc_getchat(): Opening new conference for %d contacts.", count);
#endif
                            uin_t *participants = calloc(count, sizeof(uin_t));
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

int gg_gc_openconf(WPARAM wParam, LPARAM lParam)
{
    // Check if connected
    if (!ggSess)
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
    for(l = chats; l; l = l->next)
    {
        int i;
        gg_gc_chat *chat = (gg_gc_chat *)l->data;
        if(chat->recipients && chat->recipients_count)
            for(i = 0; i < chat->recipients_count; i++)
                // Rename this window if it's exising in the chat
                if(chat->recipients[i] == uin)
                {
                    char id[32]; UIN2ID(uin, id);
                    GCEVENT gce = { sizeof(GCEVENT) };
                    GCDEST gcd;
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
