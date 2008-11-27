#include "aim.h"

static const COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

void CAimProto::chat_register(void)
{
    GCREGISTER gcr = {0};
	gcr.cbSize = sizeof(gcr);
	gcr.dwFlags = GC_TYPNOTIF | GC_CHANMGR;
	gcr.iMaxText = 0;
	gcr.nColors = 16;
	gcr.pColors = (COLORREF*)crCols;
	gcr.pszModuleDispName = m_szModuleName;
	gcr.pszModule = m_szModuleName;
	CallServiceSync(MS_GC_REGISTER, 0, (LPARAM)&gcr);

	HookProtoEvent(ME_GC_EVENT, &CAimProto::OnGCEvent);
//	HookProtoEvent(ME_GC_BUILDMENU, &CAimProto::MSN_GCMenuHook );
}

void CAimProto::chat_start(const char* id)
{
    TCHAR* idt = mir_a2t(id);

	GCSESSION gcw = {0};
	gcw.cbSize = sizeof(gcw);
	gcw.dwFlags = GC_TCHAR;
	gcw.iType = GCW_CHATROOM;
	gcw.pszModule = m_szModuleName;
	gcw.ptszName = idt;
	gcw.ptszID = idt;
	CallServiceSync(MS_GC_NEWSESSION, 0, (LPARAM)&gcw);

	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_ADDGROUP };
	gcd.ptszID = idt;

    GCEVENT gce = {0};
	gce.cbSize = sizeof(gce);
	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
	gce.ptszStatus = TranslateT("Me");
	CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

	gcd.iType = GC_EVENT_ADDGROUP;
	gce.ptszStatus = TranslateT("Others");
	CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

	gcd.iType = GC_EVENT_CONTROL;
	CallServiceSync(MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gce);
	CallServiceSync(MS_GC_EVENT, SESSION_ONLINE,   (LPARAM)&gce);
	CallServiceSync(MS_GC_EVENT, WINDOW_VISIBLE,   (LPARAM)&gce);

    mir_free(idt);
}

void CAimProto::chat_event(const char* id, const char* sn, int evt, const TCHAR* msg)
{
    TCHAR* idt = mir_a2t(id);
    TCHAR* snt = mir_a2t(sn);

    GCDEST gcd = { m_szModuleName, { NULL },  evt };
	gcd.ptszID = idt;

	GCEVENT gce = {0};
	gce.cbSize = sizeof(gce);
    gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
	gce.pDest = &gcd;
	gce.ptszNick = snt;
	gce.ptszUID = snt;
    gce.bIsMe = stricmp(sn, username) == 0;
    gce.ptszStatus = gce.bIsMe ? TranslateT("Me") : TranslateT("Others");
    gce.ptszText = msg;
	gce.time = time(NULL);
	CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

    mir_free(snt);
    mir_free(idt);
}

void CAimProto::chat_leave(const char* id)
{
    TCHAR* idt = mir_a2t(id);

    GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_CONTROL };
    gcd.ptszID = idt;

    GCEVENT gce = {0};
    gce.cbSize = sizeof(GCEVENT);
    gce.dwFlags = GC_TCHAR;
    gce.pDest = &gcd;
    CallServiceSync(MS_GC_EVENT, SESSION_OFFLINE, (LPARAM)&gce);
    CallServiceSync(MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce);

    mir_free(idt);
}


int CAimProto::OnGCEvent(WPARAM wParam,LPARAM lParam) 
{
	GCHOOK *gch = (GCHOOK*) lParam;
	if (!gch) return 1;

	if (strcmp(gch->pDest->pszModule, m_szModuleName)) return 0;

    char* id = mir_t2a(gch->pDest->ptszID);
    chat_list_item* item = find_chat_by_id(id);
    mir_free(id);

    switch (gch->pDest->iType) 
    {
		case GC_SESSION_TERMINATE: 
            if (item)
		    {
			    aim_sendflap(item->hconn,0x04,0,NULL,item->seqno);
			    Netlib_Shutdown(item->hconn);
		    }
			break;

        case GC_USER_MESSAGE:
			if (gch->ptszText && _tcslen(gch->ptszText)&& item) 
			{
                char* msg = mir_utf8encodeT(gch->ptszText);
                if (is_utf(msg))
                {
                    wchar_t* msgw = mir_utf8decodeW(msg);
                    aim_chat_send_message(item->hconn, item->seqno, (char*)msgw, true);
                    mir_free(msgw);
                }
                else
                    aim_chat_send_message(item->hconn, item->seqno, msg, false);

                mir_free(msg);
			}
			break;
		case GC_USER_CHANMGR: 
			break;

        case GC_USER_PRIVMESS:
            {
                char* sn = mir_t2a(gch->ptszUID);
                HANDLE hContact = find_contact(sn);
                mir_free(sn);
			    CallService(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
            }
			break;

        case GC_USER_LOGMENU:
			break;
		
        case GC_USER_NICKLISTMENU: 
			break;

        case GC_USER_TYPNOTIFY: 
			break;
	}

	return 0;
}

void   __cdecl CAimProto::chatnav_request_thread( void* param )
{
    if (!wait_conn(hChatNavConn, hChatNavEvent, 0x0d))
        return;

	chatnav_param* par = (chatnav_param*)param;
    if (par->isroom)
    {
        chat_list_item *item = find_chat_by_id(par->id);
        if (item == NULL)
        {
            chat_rooms.insert(new chat_list_item(par->id));
            aim_chatnav_create(hChatNavConn, chatnav_seqno, par->id);
        }
    }
    else
        aim_chatnav_room_info(hChatNavConn, chatnav_seqno, par->id, par->exchange, par->instance);

    delete par;
}

chat_list_item* CAimProto::find_chat_by_cid(unsigned short cid)
{
    chat_list_item* item = NULL;
    for(int i=0; i<chat_rooms.getCount(); ++i)
    {
        if (chat_rooms[i].cid == cid)
        {
            item = &chat_rooms[i];
            break;
        }
    }
    return item;
}

chat_list_item* CAimProto::find_chat_by_id(char* id)
{
    chat_list_item* item = NULL;
    for(int i=0; i<chat_rooms.getCount(); ++i)
    {
        if (strcmp(chat_rooms[i].id, id) == 0)
        {
            item = &chat_rooms[i];
            break;
        }
    }
    return item;
}

chat_list_item* CAimProto::find_chat_by_conn(HANDLE conn)
{
    chat_list_item* item = NULL;
    for(int i=0; i<chat_rooms.getCount(); ++i)
    {
        if (chat_rooms[i].hconn == conn) 
        {
            item = &chat_rooms[i];
            break;
        }
    }
    return item;
}

void CAimProto::remove_chat_by_ptr(chat_list_item* item)
{
    for(int i=0; i<chat_rooms.getCount(); ++i)
    {
        if (&chat_rooms[i] == item) 
        {
            chat_rooms.remove(i);
            break;
        }
    }
}

void CAimProto::shutdown_chat_conn(void)
{
    for(int i=0; i<chat_rooms.getCount(); ++i)
    {
        chat_list_item& item = chat_rooms[i];
        if (item.hconn)
        {
		    aim_sendflap(item.hconn,0x04,0,NULL,item.seqno);
		    Netlib_Shutdown(item.hconn);
        }
    }
}

void CAimProto::close_chat_conn(void)
{
    for(int i=0; i<chat_rooms.getCount(); ++i)
    {
        chat_list_item& item = chat_rooms[i];
        if (item.hconn)
        {
		    Netlib_CloseHandle(item.hconn);
            item.hconn = NULL;
        }
    }
}