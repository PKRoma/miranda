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
	GCSESSION gcw = {0};
	gcw.cbSize = sizeof(gcw);
//	gcw.dwFlags = GC_TCHAR;
	gcw.iType = GCW_CHATROOM;
	gcw.pszModule = m_szModuleName;
	gcw.pszName = id;
	gcw.pszID = id;
	CallServiceSync(MS_GC_NEWSESSION, 0, (LPARAM)&gcw);

	GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_ADDGROUP };
	gcd.pszID = (char*)id;
	GCEVENT gce = {0};
	gce.cbSize = sizeof(gce);
//	gce.dwFlags = GC_TCHAR;
	gce.pDest = &gcd;
//	gce.pszUID = name;
	gce.pszStatus = Translate("Me");
	CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

	gcd.iType = GC_EVENT_ADDGROUP;
	gce.pszStatus = Translate("Others");
	CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

	gcd.iType = GC_EVENT_CONTROL;
	CallServiceSync(MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gce);
	CallServiceSync(MS_GC_EVENT, SESSION_ONLINE,   (LPARAM)&gce);
	CallServiceSync(MS_GC_EVENT, WINDOW_VISIBLE,   (LPARAM)&gce);
}

void CAimProto::chat_event(const char* id, const char* sn, int evt, const char* msg)
{
	GCDEST gcd = { m_szModuleName, { NULL },  evt };
	gcd.pszID = (char*)id;

	GCEVENT gce = {0};
	gce.cbSize = sizeof(gce);
    gce.dwFlags = GCEF_ADDTOLOG;
	gce.pDest = &gcd;
	gce.pszNick = sn;
	gce.pszUID = sn;
    gce.bIsMe = stricmp(sn, username) == 0;
    gce.pszStatus = Translate(gce.bIsMe ? "Me" : "Others");
    gce.pszText = msg;
	gce.time = time(NULL);
	CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);
}

void CAimProto::chat_leave(const char* id)
{
    GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_CONTROL };
    gcd.pszID = (char*)id;

    GCEVENT gce = {0};
    gce.cbSize = sizeof(GCEVENT);
    gce.pDest = &gcd;
    CallServiceSync(MS_GC_EVENT, SESSION_OFFLINE, (LPARAM)&gce);
    CallServiceSync(MS_GC_EVENT, SESSION_TERMINATE, (LPARAM)&gce);
}


int CAimProto::OnGCEvent(WPARAM wParam,LPARAM lParam) 
{
	GCHOOK *gch = (GCHOOK*) lParam;
	if (!gch) return 1;

	if (stricmp(gch->pDest->pszModule, m_szModuleName)) return 0;

    chat_list_item* item = find_chat_by_id(gch->pDest->pszID);

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
			if (gch && gch->pszText && strlen(gch->pszText)&& item) 
			{
                aim_chat_send_message(item->hconn, item->seqno, gch->pszText);
			}
			break;
		case GC_USER_CHANMGR: 
			break;

        case GC_USER_PRIVMESS:
			CallService(MS_MSG_SENDMESSAGE, (WPARAM)find_contact(gch->pszUID), 0);
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