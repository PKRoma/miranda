/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2005 Robert Rainwater

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
#include "aim.h"

pthread_mutex_t connectionHandleMutex;
pthread_mutex_t modeMsgsMutex;

static int aim_getcaps(WPARAM wParam, LPARAM lParam)
{
    int ret = 0;
    switch (wParam) {
        case PFLAGNUM_1:
            ret = PF1_IM | PF1_BASICSEARCH | PF1_SEARCHBYNAME | PF1_SEARCHBYEMAIL | PF1_MODEMSGSEND | PF1_VISLIST | PF1_FILERECV;
            break;
        case PFLAGNUM_2:
            ret = PF2_ONLINE | PF2_SHORTAWAY | PF2_ONTHEPHONE;
            break;
        case PFLAGNUM_3:
            ret = PF2_SHORTAWAY;
            break;
        case PFLAGNUM_4:
            ret = PF4_NOCUSTOMAUTH;
            break;
        case 5:                /* this is PFLAGNUM_5 change when alpha SDK is released */
            ret = PF2_ONTHEPHONE;
            break;
        case PFLAG_UNIQUEIDTEXT:
            ret = (int) Translate("Screenname");
            break;
        case PFLAG_MAXLENOFMESSAGE:
            ret = 2000;
            break;
        case PFLAG_UNIQUEIDSETTING:
            ret = (int) AIM_KEY_UN;
            break;
    }
    return ret;
}

static int aim_getname(WPARAM wParam, LPARAM lParam)
{
    char pszName[128];

    if (strcmp(AIM_PROTO, AIM_PROTONAME)) {
        mir_snprintf(pszName, sizeof(pszName), Translate("%s (%s)"), AIM_PROTONAME, AIM_PROTO);
    }
    else
        mir_snprintf(pszName, sizeof(pszName), "%s", AIM_PROTONAME);

    lstrcpyn((char *) lParam, pszName, wParam);
    return 0;
}

static int aim_loadicon(WPARAM wParam, LPARAM lParam)
{
    UINT id;

    switch (wParam & 0xFFFF) {
        case PLI_PROTOCOL:
            if (IsWinVerXPPlus())
                id = IDI_AIMXP;
            else
                id = IDI_AIM;
            break;
        default:
            return (int) (HICON) NULL;
    }
    return (int) LoadImage(hInstance, MAKEINTRESOURCE(id), IMAGE_ICON, GetSystemMetrics(wParam & PLIF_SMALL ? SM_CXSMICON : SM_CXICON),
                           GetSystemMetrics(wParam & PLIF_SMALL ? SM_CYSMICON : SM_CYICON), 0);
}

static int aim_getstatus(WPARAM wParam, LPARAM lParam)
{
    return aimStatus;
}

static int aim_setstatus(WPARAM wParam, LPARAM lParam)
{
    int status = (int) wParam;
    LOG(LOG_DEBUG, "PS_SETSTATUS: %d", wParam);
    if (aimStatus == status)
        return 0;
    LOG(LOG_DEBUG, "Set Status to %s", (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, status, 0));
    if (status == ID_STATUS_OFFLINE) {
        aim_toc_disconnect();
        aim_util_broadcaststatus(ID_STATUS_OFFLINE);
        aim_buddy_offlineall();
    }
    else if (!aim_util_isonline()) {
        if (aimStatus == ID_STATUS_CONNECTING) {
            return 0;
        }
        aim_utils_logversion();
        status = status == ID_STATUS_ONLINE || status == ID_STATUS_FREECHAT ? ID_STATUS_ONLINE : ID_STATUS_AWAY;
        DBWriteContactSettingWord(NULL, AIM_PROTO, AIM_KEY_IS, status);
        aim_util_broadcaststatus(ID_STATUS_CONNECTING);
        pthread_create(aim_server_main, NULL);
    }
    else {
        status = status == ID_STATUS_ONLINE || status == ID_STATUS_FREECHAT ? ID_STATUS_ONLINE : ID_STATUS_AWAY;
        aim_util_broadcaststatus(status);
        aim_util_statusupdate();
    }
    return 0;
}

static int aim_recvmessage(WPARAM wParam, LPARAM lParam)
{
    DBEVENTINFO dbei;
    CCSDATA *ccs = (CCSDATA *) lParam;
    PROTORECVEVENT *pre = (PROTORECVEVENT *) ccs->lParam;

    LOG(LOG_DEBUG, "Received message");
    DBDeleteContactSetting(ccs->hContact, "CList", "Hidden");
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.szModule = AIM_PROTO;
    dbei.timestamp = pre->timestamp;
    dbei.flags = pre->flags & (PREF_CREATEREAD ? DBEF_READ : 0);
    dbei.eventType = EVENTTYPE_MESSAGE;
    dbei.cbBlob = strlen(pre->szMessage) + 1;
    dbei.pBlob = (PBYTE) pre->szMessage;
    CallService(MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) & dbei);
    return 0;
}

static int aim_sendmessage(WPARAM wParam, LPARAM lParam)
{
    CCSDATA *ccs = (CCSDATA *) lParam;

    return aim_im_sendmessage(ccs->hContact, (char *) ccs->lParam, 0);
}

static int aim_basicsearch(WPARAM wParam, LPARAM lParam)
{
    if (aimStatus != ID_STATUS_OFFLINE && hServerConn) {
        aim_search_simple(aim_util_normalize((char *) lParam));
        return 1;
    }
    return 0;
}

static int aim_searchbyname(WPARAM wParam, LPARAM lParam)
{
    PROTOSEARCHBYNAME *psbn = (PROTOSEARCHBYNAME *) lParam;

    if (psbn && aimStatus != ID_STATUS_OFFLINE && hServerConn) {
        aim_search_name(psbn);
        return 1;
    }
    return 0;
}

static int aim_searchbyemail(WPARAM wParam, LPARAM lParam)
{

    if ((char *) lParam && aimStatus != ID_STATUS_OFFLINE && hServerConn) {
        aim_search_email((char *) lParam);
        return 1;
    }
    return 0;
}

static int aim_addtolist(WPARAM wParam, LPARAM lParam)
{
    PROTOSEARCHRESULT *psr = (PROTOSEARCHRESULT *) lParam;
    if (aimStatus == ID_STATUS_OFFLINE)
        return 0;
    return (int) aim_buddy_get(psr->nick, 1, wParam & PALF_TEMPORARY ? 0 : 1, 0,NULL);
}

static DWORD lastUpdateCheck = 0;
static int aim_getinfo(WPARAM wParam, LPARAM lParam)
{
    CCSDATA *ccs = (CCSDATA *) lParam;
    DWORD now = GetTickCount();
    if (lastUpdateCheck && lastUpdateCheck + AIM_USERINFOFLOODTIMEOUT > now) {
        lastUpdateCheck = now;
        LOG(LOG_DEBUG, "Userinfo flood detected.  Stopping userinfo lookup.");
        pthread_create(aim_userinfo_infodummyshow, ccs->hContact);
        return 0;
    }
    lastUpdateCheck = now;
    pthread_create(aim_userinfo_infoshow, ccs->hContact);
    return 0;
}

static int aim_setawaymsg(WPARAM wParam, LPARAM lParam)
{
    pthread_mutex_lock(&modeMsgsMutex);
    if (wParam != ID_STATUS_ONLINE) {
        if (szModeMsg)
            free(szModeMsg);
        if ((char *) lParam)
            szModeMsg = strdup((char *) lParam);
        else
            szModeMsg = NULL;
        if (aimStatus == ID_STATUS_AWAY)
            aim_util_statusupdate();
    }
    else {
        if (szModeMsg) {
            free(szModeMsg);
            szModeMsg = NULL;
        }
    }
    pthread_mutex_unlock(&modeMsgsMutex);
    return 0;
}

static int aim_setapparentmode(WPARAM wParam, LPARAM lParam)
{
    int oldMode;
    CCSDATA *ccs = (CCSDATA *) lParam;

    if (ccs->wParam && ccs->wParam != ID_STATUS_OFFLINE)
        return 1;
    oldMode = DBGetContactSettingWord(ccs->hContact, AIM_PROTO, AIM_KEY_AM, 0);
    if ((int) ccs->wParam == oldMode)
        return 1;
    DBWriteContactSettingWord(ccs->hContact, AIM_PROTO, AIM_KEY_AM, (WORD) ccs->wParam);
    return 1;
}

static int aim_recvfile(WPARAM wParam, LPARAM lParam)
{
    DBEVENTINFO dbei;
    CCSDATA *ccs = (CCSDATA *) lParam;
    PROTORECVEVENT *pre = (PROTORECVEVENT *) ccs->lParam;
    char *szDesc, *szFile;

    DBDeleteContactSetting(ccs->hContact, "CList", "Hidden");
    szFile = pre->szMessage + sizeof(DWORD);
    szDesc = szFile + strlen(szFile) + 1;
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.szModule = AIM_PROTO;
    dbei.timestamp = pre->timestamp;
    dbei.flags = pre->flags & (PREF_CREATEREAD ? DBEF_READ : 0);
    dbei.eventType = EVENTTYPE_FILE;
    dbei.cbBlob = sizeof(DWORD) + strlen(szFile) + strlen(szDesc) + 2;
    dbei.pBlob = (PBYTE) pre->szMessage;
    CallService(MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) & dbei);
    return 0;
}

static int aim_recvfiledeny(WPARAM wParam, LPARAM lParam)
{
    CCSDATA *ccs = (CCSDATA *) lParam;
    struct aim_filerecv_request *ft = (struct aim_filerecv_request *) ccs->wParam;

    LOG(LOG_INFO, "[%s] Denying file receive request from %s", ft->cookie, ft->user);
    aim_filerecv_deny(ft->user, ft->cookie);
    aim_filerecv_free_fr(ft);
    return 0;
}

static int aim_recvfileallow(WPARAM wParam, LPARAM lParam)
{
    CCSDATA *ccs = (CCSDATA *) lParam;
    struct aim_filerecv_request *ft = (struct aim_filerecv_request *) ccs->wParam;

    LOG(LOG_INFO, "[%s] Requesting file from %s", ft->cookie, ft->user);
    ft->savepath = _strdup((char *) ccs->lParam);
    ft->state = FR_STATE_RECEIVING;
    aim_filerecv_accept(ft->user, ft->cookie);
    pthread_create(aim_filerecv_thread, (void *) ft);
    return ccs->wParam;
}

int aim_recvfilecancel(WPARAM wParam, LPARAM lParam)
{
    CCSDATA *ccs = (CCSDATA *) lParam;
    struct aim_filerecv_request *ft = (struct aim_filerecv_request *) ccs->wParam;

    LOG(LOG_INFO, "[%s] Cancelling file receive request from %s", ft->cookie, ft->user);
    aim_filerecv_deny(ft->user, ft->cookie);
    if (ft->s) {
        // should set state
        ft->state = FR_STATE_ERROR;
        Netlib_CloseHandle(ft->s);
        ft->s = NULL;
    }
    ProtoBroadcastAck(AIM_PROTO, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);
    return 0;
}

static int SyncBuddyWithServer(WPARAM wParam, LPARAM lParam)
{
	if (aim_util_isonline())
	{
		char buf[MSG_LEN * 2];
		char sn[33],group[33];
		DBVARIANT dbv;
		DBGetContactSetting((HANDLE)wParam, AIM_PROTO, AIM_KEY_UN, &dbv);
		strcpy(sn,dbv.pszVal);
		DBFreeVariant(&dbv);
		DBGetContactSetting((HANDLE)wParam, "CList", "Group", &dbv);
		if(dbv.pszVal)
		{
			strcpy(group,dbv.pszVal);
		}
		else
		{
			strcpy(group,"1");
		}
			DBFreeVariant(&dbv);
			strcpy(buf,"toc2_new_buddies {");
			strcat(buf,"g:");
			strcat(buf,group);
			strcat(buf,"\n");
			strcat(buf,"b:");
			strcat(buf,sn);
			strcat(buf,"\n");
			strcat(buf,"}");
			aim_toc_sflapsend(buf, -1, TYPE_DATA);
	}
	else
	{
		MessageBox(0,Translate("Please login before you attempt to sync your buddy list with the server-side list."),"Not Online",  MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}
void aim_services_register(HINSTANCE hInstance)
{
	CLISTMENUITEM mi;
    char szService[300];

    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_GETCAPS);
    CreateServiceFunction(szService, aim_getcaps);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_GETNAME);
    CreateServiceFunction(szService, aim_getname);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_LOADICON);
    CreateServiceFunction(szService, aim_loadicon);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_GETSTATUS);
    CreateServiceFunction(szService, aim_getstatus);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_SETSTATUS);
    CreateServiceFunction(szService, aim_setstatus);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PSR_MESSAGE);
    CreateServiceFunction(szService, aim_recvmessage);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PSS_MESSAGE);
    CreateServiceFunction(szService, aim_sendmessage);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_BASICSEARCH);
    CreateServiceFunction(szService, aim_basicsearch);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_SEARCHBYNAME);
    CreateServiceFunction(szService, aim_searchbyname);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_SEARCHBYEMAIL);
    CreateServiceFunction(szService, aim_searchbyemail);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_ADDTOLIST);
    CreateServiceFunction(szService, aim_addtolist);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PSS_GETINFO);
    CreateServiceFunction(szService, aim_getinfo);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PS_SETAWAYMSG);
    CreateServiceFunction(szService, aim_setawaymsg);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PSS_SETAPPARENTMODE);
    CreateServiceFunction(szService, aim_setapparentmode);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PSR_FILE);
    CreateServiceFunction(szService, aim_recvfile);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PSS_FILEDENY);
    CreateServiceFunction(szService, aim_recvfiledeny);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PSS_FILEALLOW);
    CreateServiceFunction(szService, aim_recvfileallow);
    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, PSS_FILECANCEL);
    CreateServiceFunction(szService, aim_recvfilecancel);
	mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, "/SyncBuddy");
	CreateServiceFunction( szService, SyncBuddyWithServer );
	memset( &mi, 0, sizeof( mi ));
	mi.pszPopupName = "Sync Buddy";
    mi.cbSize = sizeof( mi );
    mi.popupPosition = 500090000;
    mi.position = 500090000;
    mi.hIcon = LoadIcon(hInstance,MAKEINTRESOURCE( IDI_AIMXP ));
	mi.pszContactOwner = AIM_PROTO;
    mi.pszName = Translate( "Sync Buddy with Server-side list" );
    mi.pszService = szService;
	CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi );
}
