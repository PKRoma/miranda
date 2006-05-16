#include "aim.h"

static TList *rooms = NULL, *inviteList = NULL;
static pthread_mutex_t roomMutex, inviteMutex;
static HANDLE hGChatPreHook, hGChatHook, hGChatEvtHook, hMenuGroupChat, hGChatDelete, hGChatMenu, hInviteList, hGChatShowM;

struct aim_gchat_chatinfo
{
    char szRoom[128];
    char szUser[128];
    char szMsg[256];
    char chatid[32];
};

typedef struct
{
    int dwRoom;
    char *szRoom;
}
TChatRoom;

static void aim_gchat_event(char *room, int type, const char *text, const char *nick, const char *status, const char *info)
{
    GCDEST gcd;
    GCEVENT gce;

    ZeroMemory(&gce, sizeof(gce));
    ZeroMemory(&gcd, sizeof(gcd));
    gce.cbSize = sizeof(gce);
    gce.pDest = &gcd;
    gce.pszText = text;
    gce.pszNick = nick;
    gce.pszUID = nick ? aim_util_normalize((char *) nick) : NULL;
    gce.pszStatus = status;
    gce.pszUserInfo = info;
    gce.bIsMe = nick ? (aim_util_isme((char *) nick) ? TRUE : FALSE) : FALSE;
    gce.bAddToLog = TRUE;
    gce.time = time(NULL);
    gcd.pszModule = AIM_PROTO;
    gcd.pszID = room;
    gcd.iType = type;
    CallServiceSync(MS_GC_EVENT, 0, (LPARAM) & gce);
}

static void aim_gchat_event_control(int event, char *room)
{
    GCDEST gcd;
    GCEVENT gce;

    ZeroMemory(&gce, sizeof(gce));
    ZeroMemory(&gcd, sizeof(gcd));
    gce.cbSize = sizeof(gce);
    gce.pDest = &gcd;
    gcd.pszModule = AIM_PROTO;
    gcd.pszID = room;
    gcd.iType = GC_EVENT_CONTROL;
    CallServiceSync(MS_GC_EVENT, (WPARAM) event, (LPARAM) & gce);
}

static void aim_gchat_storechat(char *szRoom)
{
    DBVARIANT dbv;
    int i;
    char buf[256], nbuf[256];

    if (!strcmp(szRoom, MIRANDANAME))
        return;
    for (i = 0; i < 10; i++) {
        mir_snprintf(buf, sizeof(buf), "%s%d", AIM_GCHAT_PREFIX, i);
        if (!DBGetContactSetting(NULL, AIM_PROTO, buf, &dbv)) {
            if (!strcmp(dbv.pszVal, szRoom)) {
                DBFreeVariant(&dbv);
                return;
            }
            DBFreeVariant(&dbv);
        }
    }
    for (i = 8; i >= 0; i--) {
        mir_snprintf(buf, sizeof(buf), "%s%d", AIM_GCHAT_PREFIX, i);
        if (!DBGetContactSetting(NULL, AIM_PROTO, buf, &dbv)) {
            mir_snprintf(nbuf, sizeof(nbuf), "%s%d", AIM_GCHAT_PREFIX, i + 1);
            DBWriteContactSettingString(NULL, AIM_PROTO, nbuf, dbv.pszVal);
            DBFreeVariant(&dbv);
        }
    }
    mir_snprintf(buf, sizeof(buf), "%s0", AIM_GCHAT_PREFIX, 0);
    DBWriteContactSettingString(NULL, AIM_PROTO, buf, szRoom);
}

static TChatRoom *aim_gchat_find(int dwRoom)
{
    TList *n;
    TChatRoom *r;

    pthread_mutex_lock(&roomMutex);
    for (n = rooms; n != NULL; n = n->next) {
        if (n->data) {
            r = (TChatRoom *) n->data;
            if (r->dwRoom == dwRoom) {
                pthread_mutex_unlock(&roomMutex);
                return r;
            }
        }
    }
    pthread_mutex_unlock(&roomMutex);
    return NULL;
}

static TChatRoom *aim_gchat_find_by_name(char *szRoom)
{
    TList *n;
    TChatRoom *r;

    if (!szRoom)
        return NULL;
    pthread_mutex_lock(&roomMutex);
    for (n = rooms; n != NULL; n = n->next) {
        if (n->data) {
            r = (TChatRoom *) n->data;
            if (!strcmp(r->szRoom, szRoom)) {
                pthread_mutex_unlock(&roomMutex);
                return r;
            }
        }
    }
    pthread_mutex_unlock(&roomMutex);
    return NULL;
}

static void aim_gchat_delete(int dwRoom)
{
    TChatRoom *r = aim_gchat_find(dwRoom);
    if (r) {
        pthread_mutex_lock(&roomMutex);
        aim_gchat_event_control(WINDOW_TERMINATE, r->szRoom);
        rooms = tlist_remove(rooms, (void *) r);
        if (aim_util_isonline()) {
            char snd[MSG_LEN * 2];

            mir_snprintf(snd, sizeof(snd), "toc_chat_leave %d", dwRoom);
            aim_toc_sflapsend(snd, -1, TYPE_DATA);
        }
        free(r->szRoom);
        free(r);
        pthread_mutex_unlock(&roomMutex);
    }
}

void aim_gchat_delete_by_contact(HANDLE hContact)
{
    DBVARIANT dbv;

	if (!ServiceExists(MS_GC_REGISTER)) return;
    if (!DBGetContactSetting(hContact, AIM_PROTO, "ChatRoomID", &dbv)) {
        TChatRoom *r = aim_gchat_find_by_name(dbv.pszVal);

        if (r) {
            aim_gchat_delete(r->dwRoom);
        }
        DBFreeVariant(&dbv);
    }
}

static void aim_ghcat_deleteall()
{
    TList *n;
    TChatRoom *r;

    pthread_mutex_lock(&roomMutex);
    if (rooms) {
        for (n = rooms; n != NULL; n = n->next) {
            if (n->data) {
                r = (TChatRoom *) n->data;
                free(r->szRoom);
                free(r);
            }
        }
        free(rooms);
        rooms = NULL;
        aim_gchat_event_control(WINDOW_TERMINATE, NULL);
    }
    pthread_mutex_unlock(&roomMutex);
}

static void aim_gchat_addroom(TChatRoom * r)
{
    pthread_mutex_lock(&roomMutex);
    rooms = tlist_append(rooms, (void *) r);
    pthread_mutex_unlock(&roomMutex);
}

static BOOL CALLBACK aim_gchat_invitedlg(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            struct aim_gchat_chatinfo *info;
            char msg[256];

            TranslateDialogDefault(hwndDlg);
            aim_util_winlistadd(hInviteList, hwndDlg);
            info = (struct aim_gchat_chatinfo *) lParam;
            aim_util_striphtml(msg, info->szMsg, sizeof(msg));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT)));
            SetWindowText(GetDlgItem(hwndDlg, IDC_BUDDY), info->szUser);
            SetWindowText(GetDlgItem(hwndDlg, IDC_ROOM), info->szRoom);
            SetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE), msg);
            SetFocus(GetDlgItem(hwndDlg, IDOK));
            break;
        }
        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            break;
        case WM_DESTROY:
            aim_util_winlistdel(hInviteList, hwndDlg);
            break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                case IDOK:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

static void __cdecl aim_gchat_chatinvitethread(void *cinfo)
{
    struct aim_gchat_chatinfo *info = (struct aim_gchat_chatinfo *) cinfo;

    if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_GCHATINVITE), NULL, aim_gchat_invitedlg, (LPARAM) info) == IDOK) {
        char snd[MSG_LEN * 2];
        mir_snprintf(snd, sizeof(snd), "toc_chat_accept %s", info->chatid);
        aim_toc_sflapsend(snd, -1, TYPE_DATA);
    }
    pthread_mutex_lock(&inviteMutex);
    inviteList = tlist_remove(inviteList, (void *) info);
    pthread_mutex_unlock(&inviteMutex);
    free(info);
}

static int aim_gchat_inviteservice(WPARAM wParam, LPARAM lParam)
{
    CLISTEVENT *cle = (CLISTEVENT *) lParam;
    struct aim_gchat_chatinfo *info;

    if (!cle)
        return 0;
    info = (struct aim_gchat_chatinfo *) cle->lParam;
    if (!info)
        return 0;
    pthread_create(aim_gchat_chatinvitethread, (void *) info);
    return 0;
}

void aim_gchat_joinrequest(char *room, int exchange)
{
    char snd[MSG_LEN * 2];
    char szRoom[MSG_LEN];
	
	if (!ServiceExists(MS_GC_REGISTER)) return;
    mir_snprintf(szRoom, sizeof(szRoom), "%s", room);
    aim_util_escape(szRoom);
    LOG(LOG_DEBUG, "Sending join request for \"%s\", exchange=%d", room, exchange);
    mir_snprintf(snd, sizeof(snd), "toc_chat_join %d \"%s\"", exchange, szRoom);
    aim_toc_sflapsend(snd, -1, TYPE_DATA);
}

void aim_gchat_chatinvite(char *szRoom, char *szUser, char *chatid, char *msg)
{
	if (!ServiceExists(MS_GC_REGISTER)) return;
	{
		struct aim_gchat_chatinfo *info = (struct aim_gchat_chatinfo *) malloc(sizeof(struct aim_gchat_chatinfo));
		HANDLE hContact;
		char szService[256], szTip[256];
		CLISTEVENT cle;

		ZeroMemory(info, sizeof(info));
		// Check to see if we are ignoring invites
		if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GI, AIM_KEY_GI_DEF))
			return;

		// Only accept chat invites from contacts in your list
		hContact = aim_buddy_get(szUser, 0, 0, 0, NULL);
		if (hContact == NULL)
			return;

		lstrcpyn(info->szRoom, szRoom, sizeof(info->szRoom));
		lstrcpyn(info->szUser, szUser, sizeof(info->szUser));
		lstrcpyn(info->szMsg, msg, sizeof(info->szMsg));
		lstrcpyn(info->chatid, chatid, sizeof(info->chatid));
		pthread_mutex_lock(&inviteMutex);
		inviteList = tlist_append(inviteList, (void *) info);
		pthread_mutex_unlock(&inviteMutex);

		ZeroMemory(&cle, sizeof(cle));
		cle.cbSize = sizeof(cle);
		cle.hContact = (HANDLE) hContact;
		cle.hDbEvent = (HANDLE) NULL;
		cle.lParam = (LPARAM) info;
		cle.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT));
		mir_snprintf(szService, sizeof(szService), "%s/AIM/GroupChatInviteCList", AIM_PROTO);
		cle.pszService = szService;
		mir_snprintf(szTip, sizeof(szTip), Translate("Chat invitation from %s"),
					 (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0));
		cle.pszTooltip = szTip;
		CallServiceSync(MS_CLIST_ADDEVENT, (WPARAM) hContact, (LPARAM) & cle);
	}
}

void aim_gchat_updatestatus()
{
    CLISTMENUITEM cli;

	if (!ServiceExists(MS_GC_REGISTER)) return;
    ZeroMemory(&cli, sizeof(cli));
    cli.cbSize = sizeof(cli);
    if (!aim_util_isonline()) {
        aim_ghcat_deleteall();
        cli.flags = CMIM_FLAGS | CMIF_GRAYED;
    }
    else {
        cli.flags = CMIM_FLAGS;
    }
    CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMenuGroupChat, (LPARAM) & cli);
}

static BOOL CALLBACK aim_gchat_invitereqdlg(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE dat = NULL;
    dat = (HANDLE) GetWindowLong(hwndDlg, GWL_USERDATA);
    switch (msg) {
        case WM_INITDIALOG:
        {
            int found = 0;
            DBVARIANT dbv;

            dat = (HANDLE) lParam;
            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) dat);
            TranslateDialogDefault(hwndDlg);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT)));
            if (!DBGetContactSetting(dat, AIM_PROTO, AIM_KEY_NK, &dbv)) {
                SetWindowText(GetDlgItem(hwndDlg, IDC_BUDDY), dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else if (!DBGetContactSetting(dat, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                SetWindowText(GetDlgItem(hwndDlg, IDC_BUDDY), dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else
                EndDialog(hwndDlg, IDCANCEL);
            SetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE), Translate("Join me in this chat."));
            {
                TList *n;
                TChatRoom *r;
                int item;

                pthread_mutex_lock(&roomMutex);
                for (n = rooms; n != NULL; n = n->next) {
                    if (n->data) {
                        r = (TChatRoom *) n->data;
                        if (r->szRoom) {
                            found = 1;
                            item = SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_ADDSTRING, 0, (LPARAM) r->szRoom);
                            SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_SETITEMDATA, item, (LPARAM) r->dwRoom);
                        }
                    }
                }
                pthread_mutex_unlock(&roomMutex);
            }
            if (!found) {
                EndDialog(hwndDlg, IDCANCEL);
            }
            SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_SETCURSEL, 0, 0);
            SetFocus(GetDlgItem(hwndDlg, IDOK));
            break;
        }
        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            break;
        case WM_DESTROY:
            break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDOK:
                {
                    char szMsg[128];
                    int groupid;
                    DBVARIANT dbv;

                    GetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE), szMsg, sizeof(szMsg));
                    groupid = SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_GETCURSEL, 0, 0), 0);
                    if (!DBGetContactSetting(dat, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                        char buf[MSG_LEN * 2], nmsg[128];

                        mir_snprintf(nmsg, sizeof(nmsg), "%s", szMsg);
                        aim_util_escape(szMsg);
                        mir_snprintf(buf, sizeof(buf), "toc_chat_invite %d \"%s\" %s", groupid, nmsg, dbv.pszVal);
                        aim_toc_sflapsend(buf, -1, TYPE_DATA);
                        DBFreeVariant(&dbv);
                    }
                    else {
                        EndDialog(hwndDlg, IDCANCEL);
                    }
                }               // fall-through
                case IDCANCEL:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

static int aim_gchat_invitereq(WPARAM wParam, LPARAM lParam)
{
    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_GCHATINVITEREQ), NULL, aim_gchat_invitereqdlg, (LPARAM) wParam);
    return 0;
}

#define DM_RESETROOMLIST	(WM_USER+1)
static BOOL CALLBACK aim_gchat_joinproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            char ex[10];

            TranslateDialogDefault(hwndDlg);
            mir_snprintf(ex, sizeof(ex), "%d", AIM_GROUPCHAT_DEFEXCHANGE);
            SetDlgItemText(hwndDlg, IDC_EXCHANGE, ex);
            SendDlgItemMessage(hwndDlg, IDC_EXCHANGESPIN, UDM_SETRANGE, 0, MAKELONG(6, 4));
            SendDlgItemMessage(hwndDlg, IDC_EXCHANGESPIN, UDM_SETPOS, 0, MAKELONG(AIM_GROUPCHAT_DEFEXCHANGE, 0));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT)));
            SendMessage(hwndDlg, DM_RESETROOMLIST, 0, 0);
            break;
        }
        case DM_RESETROOMLIST:
        {
			int i;
            SendDlgItemMessage(hwndDlg, IDC_CHATNAME, CB_RESETCONTENT, 0, 0);
            {
                DBVARIANT dbv;
                char buf[64];

                for (i = 0; i < 10; i++) {
                    mir_snprintf(buf, sizeof(buf), "%s%d", AIM_GCHAT_PREFIX, i);
                    if (!DBGetContactSetting(NULL, AIM_PROTO, buf, &dbv)) {
                        SendDlgItemMessage(hwndDlg, IDC_CHATNAME, CB_ADDSTRING, 0, (LPARAM) dbv.pszVal);
                        DBFreeVariant(&dbv);
                    }
					else break;
                }
            }
			if (i==0)
				SendDlgItemMessage(hwndDlg, IDC_CHATNAME, CB_ADDSTRING, 0, (LPARAM) MIRANDANAME);
            SendDlgItemMessage(hwndDlg, IDC_CHATNAME, CB_SETCURSEL, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_CHATNAME, EM_SETSEL, 0, -1);
            SetFocus(GetDlgItem(hwndDlg, IDC_CHATNAME));
            break;
        }
        case WM_CLOSE:
        {
            EndDialog(hwndDlg, 0);
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDC_HISTORY:
                {
                    if (MessageBox
                        (hwndDlg, Translate("Are you sure you want to delete the recent groupchat list?"), Translate("Confirm Delete"),
                         MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        char buf[256];
                        int i;

                        for (i = 0; i < 10; i++) {
                            mir_snprintf(buf, sizeof(buf), "%s%d", AIM_GCHAT_PREFIX, i);
                            DBDeleteContactSetting(NULL, AIM_PROTO, buf);
                        }
                        SendMessage(hwndDlg, DM_RESETROOMLIST, 0, 0);
                    }
                    break;
                }
                case IDC_CHATNAME:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int i;
                        if (i = (int) SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0) != CB_ERR) {
                            EnableWindow(GetDlgItem(hwndDlg, IDOK), (int) SendMessage((HWND) lParam, CB_GETLBTEXTLEN, i, 0));
                        }
                    }
                    else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_EDITUPDATE)
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), GetWindowTextLength((HWND) lParam));
                    break;
                case IDOK:
                {
                    char szRoom[128];
                    int exchange;

                    GetWindowText(GetDlgItem(hwndDlg, IDC_CHATNAME), szRoom, sizeof(szRoom));
                    exchange = GetDlgItemInt(hwndDlg, IDC_EXCHANGE, NULL, TRUE);
                    aim_gchat_joinrequest(szRoom, exchange);
                }
                    // fall-through
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    break;
            }
        }
    }
    return FALSE;
}

static int aim_gchat_join(WPARAM wParam, LPARAM lParam)
{
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_GROUPCHATJOIN), NULL, aim_gchat_joinproc);
    return 0;
}

static int aim_gchat_recvevent(WPARAM wParam, LPARAM lParam)
{
    GCHOOK *gch = (GCHOOK *) lParam;

    if (!gch || !gch->pDest)
        return 0;
    if (!strcmpi(gch->pDest->pszModule, AIM_PROTO)) {
        switch (gch->pDest->iType) {
            case GC_USER_MESSAGE:
            {
                if (gch && gch->pszText) {
                    char snd[MSG_LEN * 2], msg[MSG_LEN * 2];
                    TChatRoom *r = aim_gchat_find_by_name(gch->pDest->pszID);

                    if (!r)
                        break;
                    while (gch->pszText[strlen(gch->pszText) - 1] == '\r' || gch->pszText[strlen(gch->pszText) - 1] == '\n') {
                        gch->pszText[strlen(gch->pszText) - 1] = '\0';
                    }
                    mir_snprintf(msg, sizeof(msg), "%s", gch->pszText);
                    aim_util_escape(msg);
                    mir_snprintf(snd, sizeof(snd), "toc_chat_send %d \"%s\"", r->dwRoom, msg);
                    aim_toc_sflapsend(snd, -1, TYPE_DATA);
                }
                break;
            }
			case GC_USER_PRIVMESS:
			{
				if (gch->pszUID) {
					if (aim_util_isme(gch->pszUID))
						break;
					if (ServiceExists(MS_MSG_SENDMESSAGE)) {
						HANDLE hContact = aim_buddy_get(gch->pszUID, 1, 0, 0, NULL);
						
						if (hContact) 
							CallServiceSync(MS_MSG_SENDMESSAGE, (WPARAM) hContact, 0);
					}
				}
				break;
			}
        }
    }
    return 0;
}

static void __cdecl aim_gchat_create_t(void *room)
{
    TChatRoom *r = (TChatRoom *) room;
    if (r) {
        GCWINDOW gcw;

        ZeroMemory(&gcw, sizeof(gcw));
        gcw.cbSize = sizeof(gcw);
        gcw.iType = GCW_CHATROOM;
        gcw.pszModule = AIM_PROTO;
        gcw.pszName = r->szRoom;
        gcw.pszID = r->szRoom;
        CallServiceSync(MS_GC_NEWCHAT, 0, (LPARAM) & gcw);
        aim_gchat_event(r->szRoom, GC_EVENT_ADDGROUP, 0, 0, Translate("Users"), 0);
        aim_gchat_event_control(WINDOW_INITDONE, r->szRoom);
        aim_gchat_event_control(WINDOW_ONLINE, r->szRoom);
    }
}

void aim_gchat_create(int dwRoom, char *szRoom)
{
	if (!ServiceExists(MS_GC_REGISTER)) return;
    if (dwRoom && szRoom) {
        TChatRoom *r = (TChatRoom *) malloc(sizeof(TChatRoom));

        r->dwRoom = dwRoom;
        r->szRoom = _strdup(szRoom);
        aim_gchat_addroom(r);
        aim_gchat_storechat(szRoom);
        pthread_create(aim_gchat_create_t, (void *) r);
    }
}

void aim_gchat_sendmessage(int dwRoom, char *szUser, char *szMessage, int whisper)
{
	if (!ServiceExists(MS_GC_REGISTER)) return;
	{
		TChatRoom *r = aim_gchat_find(dwRoom);

		if (r) {
			char szStrip[2000];

			aim_util_striphtml(szStrip, szMessage, sizeof(szStrip));
			aim_gchat_event(r->szRoom, whisper?GC_EVENT_NOTICE:GC_EVENT_MESSAGE, szStrip, szUser, 0, 0);
		}
	}
}

void aim_gchat_updatebuddy(int dwRoom, char *szUser, int joined)
{
	if (!ServiceExists(MS_GC_REGISTER)) return;
	{
		TChatRoom *r = aim_gchat_find(dwRoom);
		
		if (r) {
			aim_gchat_event(r->szRoom, joined ? GC_EVENT_JOIN : GC_EVENT_PART, 0, szUser, Translate("Users"), 0);
		}
	}
}

static int aim_gchat_preshutdown(WPARAM wParam, LPARAM lParam)
{
    aim_ghcat_deleteall();
    aim_util_winlistbroadcast(hInviteList, WM_CLOSE, 0, 0);
    return 0;
}

static int aim_gchat_prebuildmenu(WPARAM wParam, LPARAM lParam)
{
    CLISTMENUITEM mi;

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (DBGetContactSettingByte((HANDLE) wParam, AIM_PROTO, AIM_CHAT, 0) == 0) {
        if (rooms) {
            mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE;
        }
        else {
            mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIF_HIDDEN;
        }
        CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hGChatMenu, (LPARAM) & mi);
        mi.flags = CMIM_FLAGS | CMIF_HIDDEN;
        CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hGChatDelete, (LPARAM) & mi);
        CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hGChatShowM, (LPARAM) & mi);
    }
    else {
        mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIF_HIDDEN;
        CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hGChatMenu, (LPARAM) & mi);
        mi.flags = CMIM_FLAGS;
        CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hGChatDelete, (LPARAM) & mi);
        CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hGChatShowM, (LPARAM) & mi);
    }
    return 0;
}

static int aim_gchat_menudelete(WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact = (HANDLE) wParam;

    if (!hContact)
        return 0;
    if (DBGetContactSettingByte(hContact, AIM_PROTO, AIM_CHAT, 0) == 0)
        return 0;
    aim_gchat_delete_by_contact(hContact);
    return 0;
}

static int aim_gchat_menushow(WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact = (HANDLE) wParam;
    DBVARIANT dbv;

    if (!hContact)
        return 0;
    if (DBGetContactSettingByte(hContact, AIM_PROTO, AIM_CHAT, 0) == 0)
        return 0;

    if (!DBGetContactSetting(hContact, AIM_PROTO, "ChatRoomID", &dbv)) {
        TChatRoom *r = aim_gchat_find_by_name(dbv.pszVal);

        if (r) {
            aim_gchat_event_control(WINDOW_VISIBLE, r->szRoom);
        }
        DBFreeVariant(&dbv);
    }
    return 0;
}

void aim_gchat_init()
{
    GCREGISTER gcr;
    CLISTMENUITEM mi;
    char szService[MAX_PATH + 30], szTitle[256];

    pthread_mutex_init(&roomMutex);
    pthread_mutex_init(&inviteMutex);
    ZeroMemory(&gcr, sizeof(gcr));
    gcr.cbSize = sizeof(gcr);
    gcr.pszModule = AIM_PROTO;
    gcr.pszModuleDispName = AIM_PROTO;
    gcr.iMaxText = 2000;
    CallServiceSync(MS_GC_REGISTER, 0, (LPARAM) & gcr);
    hGChatPreHook = HookEvent(ME_SYSTEM_PRESHUTDOWN, aim_gchat_preshutdown);
    hGChatEvtHook = HookEvent(ME_GC_EVENT, aim_gchat_recvevent);
    hGChatHook = HookEvent(ME_CLIST_PREBUILDCONTACTMENU, aim_gchat_prebuildmenu);
    if (strcmp(AIM_PROTO, AIM_PROTONAME)) {
        mir_snprintf(szTitle, sizeof(szTitle), "%s (%s)", AIM_PROTONAME, AIM_PROTO);
    }
    else
        mir_snprintf(szTitle, sizeof(szTitle), "%s", AIM_PROTO);

    mir_snprintf(szService, sizeof(szService), "%s/AIM/GroupChatInviteCList", AIM_PROTO);
    CreateServiceFunction(szService, aim_gchat_inviteservice);

    mir_snprintf(szService, sizeof(szService), "%s/AIM/OpenGroupChat", AIM_PROTO);
    CreateServiceFunction(szService, aim_gchat_join);
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.pszPopupName = szTitle;
    mi.popupPosition = 500080000;
    if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GM, AIM_KEY_GM_DEF)) {
        mi.position = 500081000;
        mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT));
        mi.pszName = Translate("Join/Create Group Chat...");
        mi.pszService = szService;
        hMenuGroupChat = (HANDLE) CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) & mi);
    }

    mir_snprintf(szService, sizeof(szService), "%s/AIM/GroupChatInvite", AIM_PROTO);
    CreateServiceFunction(szService, aim_gchat_invitereq);
    mi.position = -2000080000;
    mi.flags = CMIF_NOTOFFLINE;
    mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT));
    mi.pszContactOwner = AIM_PROTO;
    mi.pszName = Translate("Invite to Group Chat...");
    mi.pszService = szService;
    hGChatMenu = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);

    mir_snprintf(szService, sizeof(szService), "%s/AIM/GroupChatShow", AIM_PROTO);
    CreateServiceFunction(szService, aim_gchat_menushow);
    mi.position = 500080000;
    mi.flags = CMIF_NOTOFFLINE;
    mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GSHOW));
    mi.pszContactOwner = AIM_PROTO;
    mi.pszName = Translate("&Show Channel");
    mi.pszService = szService;
    hGChatShowM = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);

    mir_snprintf(szService, sizeof(szService), "%s/AIM/GroupChatDelete", AIM_PROTO);
    CreateServiceFunction(szService, aim_gchat_menudelete);
    mi.position = 500090000;
    mi.flags = CMIF_NOTOFFLINE;
    mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GDELETE));
    mi.pszContactOwner = AIM_PROTO;
    mi.pszName = Translate("&Leave Channel");
    mi.pszService = szService;
    hGChatDelete = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);

    aim_gchat_updatestatus();
    hInviteList = aim_util_winlistalloc();
}

void aim_gchat_destroy()
{
    TList *n = inviteList;

    while (n != NULL) {
        TList *next = n->next;
        struct aim_gchat_chatinfo *info = (struct aim_gchat_chatinfo *) n->data;

        if (info) {
            if (info->szRoom)
                free(info->szRoom);
            if (info->szUser)
                free(info->szUser);
            if (info->szMsg)
                free(info->szMsg);
            if (info->chatid)
                free(info->chatid);
            free(info);
        }
        n = next;
    }
    if (inviteList)
        tlist_free(inviteList);
    LocalEventUnhook(hGChatPreHook);
    LocalEventUnhook(hGChatEvtHook);
    LocalEventUnhook(hGChatHook);
    pthread_mutex_destroy(&roomMutex);
    pthread_mutex_destroy(&inviteMutex);
}
