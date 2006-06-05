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
struct toc_data *tdt = NULL;

static int CallProtoServiceSync(const char *proto, const char *service, WPARAM wParam, LPARAM lParam)
{
    char szProtoService[MAX_PATH];

    mir_snprintf(szProtoService, sizeof(szProtoService), "%s%s", proto, service);
    return CallServiceSync(szProtoService, wParam, lParam);
}

HANDLE aim_toc_connect()
{
    NETLIBOPENCONNECTION ncon = { 0 };
    NETLIBUSERSETTINGS nlus = { 0 };
    SOCKET s;
    HANDLE con;
    char host[256];
    DBVARIANT dbv;
    SOCKADDR_IN saddr;
    int len;

    hServerSideList = importBuddies || firstRun;
    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_TS, &dbv)) {
        mir_snprintf(host, sizeof(host), "%s", dbv.pszVal);
        DBFreeVariant(&dbv);
    }
    else
        mir_snprintf(host, sizeof(host), "%s", AIM_TOC_HOST);

    nlus.cbSize = sizeof(nlus);
    ncon.cbSize = sizeof(ncon);
    ncon.szHost = host;
    ncon.wPort = DBGetContactSettingWord(NULL, AIM_PROTO, AIM_KEY_TT, AIM_TOC_PORT);
    if (ncon.wPort == 0) {
        ncon.wPort = aim_util_randomnum(AIM_TOC_PORTLOW, AIM_TOC_PORTHIGH);
    }
    con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlib, (LPARAM) & ncon);
    if (!con) {
        mir_snprintf(host, sizeof(host), "%s", AIM_TOC_HOST);
        ncon.szHost = host;
        ncon.wPort = AIM_TOC_PORT;
        if (!con) {
            aim_util_broadcaststatus(ID_STATUS_OFFLINE);
            LOG(LOG_DEBUG, "Connection to %s:%d failed", ncon.szHost, ncon.wPort);
            return NULL;
        }
    }
    if (Miranda_Terminated() || CallProtoServiceSync(AIM_PROTO, PS_GETSTATUS, 0, 0) != ID_STATUS_CONNECTING) {
        Netlib_CloseHandle(con);
        return NULL;
    }
    if ((CallService(MS_NETLIB_GETUSERSETTINGS, (WPARAM) hNetlib, (LPARAM) & nlus) && !(nlus.useProxy && nlus.dnsThroughProxy)) &&
        (s = CallService(MS_NETLIB_GETSOCKET, (WPARAM) con, 0)) != INVALID_SOCKET) {
        if (getpeername(s, (SOCKADDR *) & saddr, &len) == 0) {
            DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_SA, inet_ntoa(saddr.sin_addr));
        }
        else {
            DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_SA, host);
        }
    }
    else {
        DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_SA, host);
    }

    importBuddies = 0;
    return con;
}

void aim_toc_disconnect()
{
    pthread_mutex_lock(&connectionHandleMutex);
    firstRun = 0;
    hServerSideList = importBuddies;
    if (szStatus) {
        free(szStatus);
        szStatus = NULL;
    }
    if (szModeMsg) {
        free(szModeMsg);
        szModeMsg = NULL;
    }
    if (tdt) {
        if (tdt->password) {
            free(tdt->password);
            tdt->password = NULL;
        }
        if (tdt->username) {
            free(tdt->username);
            tdt->username = NULL;
        }
        free(tdt);
        tdt = NULL;
    }
    if (hServerPacketRecver) {
        Netlib_CloseHandle(hServerPacketRecver);
        hServerPacketRecver = NULL;
    }
    if (hServerConn) {
        Netlib_CloseHandle(hServerConn);
        hServerConn = NULL;
    }
    pthread_mutex_unlock(&connectionHandleMutex);
}

int aim_toc_login(HANDLE hConn)
{
    DBVARIANT dbv;

    if (!hConn)
        return 0;
    if (tdt) {
        if (tdt->password)
            free(tdt->password);
        if (tdt->username)
            free(tdt->username);
        free(tdt);
    }
    tdt = malloc(sizeof(struct toc_data));
    tdt->password = NULL;
    tdt->username = NULL;
    tdt->state = STATE_OFFLINE;
    hServerConn = hConn;
    hServerPacketRecver = NULL;
    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_PW, &dbv)) {
        CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
        tdt->password = _strdup(dbv.pszVal);
        DBFreeVariant(&dbv);
    }
    else {
        LOG(LOG_ERROR, "Error retrieving password");
        return 0;
    }
    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_UN, &dbv)) {
        tdt->username = _strdup(dbv.pszVal);
        DBFreeVariant(&dbv);
    }
    else {
        LOG(LOG_ERROR, "Error retrieving screenname");
        return 0;
    }
    if (aim_util_netsend(FLAPON, strlen(FLAPON))) {
        aim_util_broadcaststatus(ID_STATUS_OFFLINE);
        LOG(LOG_ERROR, "Problem sending FLAPON");
        return 0;
    }
    tdt->state = STATE_FLAPON;
    return 1;
}

static void toc_peformerror(int e)
{
    char buf[256];

    buf[0] = '\0';
    switch (e) {
            // Misc Errors
		case 901:
			mir_snprintf(buf, sizeof(buf), Translate("Buddy not avaliable."));
			break;
		case 903:
            mir_snprintf(buf, sizeof(buf), Translate("A message has been dropped. You are exceeding the server speed limit."));
			break;
		case 931:
            mir_snprintf(buf, sizeof(buf),Translate("Unable to add buddy or group. You may have the max allowed buddies or groups or are trying to add a buddy to a group that doesn't exist and cannot be created."));
            MessageBox(0, Translate("Unable to add buddy or group. You may have the max allowed buddies or groups or are trying to add a buddy to a group that doesn't exist and cannot be created."), "Oops",MB_OK);
			break;
		case 960:
            mir_snprintf(buf, sizeof(buf), Translate("You are sending messages too fast.  Some messages may have been dropped."));
            break;
        case 961:
            mir_snprintf(buf, sizeof(buf), Translate("You missed a message because it was too big."));
            break;
        case 962:
            mir_snprintf(buf, sizeof(buf), Translate("You missed a message because it was sent too fast."));
            break;
            // Login Errors
        case 980:
            mir_snprintf(buf, sizeof(buf), Translate("Incorrect nickname or password.  Please change your login details and try again."));
            break;
        case 981:
            mir_snprintf(buf, sizeof(buf), Translate("The service is temporarily unavailable.  Please try again later."));
            break;
        case 982:
            mir_snprintf(buf, sizeof(buf), Translate("Your warning level is currently too high to sign on.  Please try again later."));
            break;
        case 983:
            mir_snprintf(buf, sizeof(buf),
                         Translate
                         ("You have been connecting and disconnecting too frequently.  Wait 10 minutes and try again.  If you continue to try, you will need to wait even longer."));
            MessageBox(0, Translate("You have been connecting and disconnecting too frequently.  Wait 10 minutes and try again.  If you continue to try, you will need to wait even longer."), "Error 983",MB_ICONERROR | MB_OK);
			break;
        case 989:
            mir_snprintf(buf, sizeof(buf), Translate("An unknown signon error has occurred.  Please try again later."));
            break;
    }
    if (buf[0]) {
        LOG(LOG_ERROR, "Error %d: %s", e, buf);
        if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_SE, AIM_KEY_SE_DEF)) {
            if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
                aim_util_shownotification(Translate("AIM Network Error"), buf, NIIF_ERROR);
            }
            else
                MessageBox(0, buf, Translate("AIM Network Error"), MB_OK | MB_ICONERROR);
        }
    }
    else {
        LOG(LOG_ERROR, "Unknown Error: %d", e);
    }
}

int aim_toc_parse(char *buf, int len)
{
    struct toc_sflap_hdr *hdr;
    char snd[MSG_LEN * 2];
    char *c;
	int d,e,f,g,pw,sn;
	char code[15];
    hdr = (struct toc_sflap_hdr *) buf;

    if (tdt->state == STATE_FLAPON) {
        struct signon so;
        char host[256];
        int port;
        DBVARIANT dbv;

        if (hdr->type != TYPE_SIGNON) {
            LOG(LOG_ERROR, "Received SFLAP Header != TYPE_SIGNON");
            return -1;
        }
        else
            LOG(LOG_DEBUG, "Received SFLAP SIGNON");
        if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_AS, &dbv)) {
            mir_snprintf(host, sizeof(host), "%s", dbv.pszVal);
            DBFreeVariant(&dbv);
        }
        else
            mir_snprintf(host, sizeof(host), "%s", AIM_AUTH_HOST);
        port = DBGetContactSettingWord(NULL, AIM_PROTO, AIM_KEY_TT, AIM_AUTH_PORT);
        if (port == 0) {
            port = aim_util_randomnum(AIM_AUTH_PORTLOW, AIM_AUTH_PORTHIGH);
            LOG(LOG_DEBUG, "Using random auth port for %s (Port %d)", host, port);
        }
        tdt->seqno = ntohs(hdr->seqno);
        tdt->state = STATE_SIGNON;
        mir_snprintf(so.username, sizeof(so.username), "%s", tdt->username);
        so.ver = htonl(1);
        so.tag = htons(1);
        so.namelen = htons(strlen(so.username));
		

        if (aim_toc_sflapsend((char *) &so, ntohs(so.namelen) + 8, TYPE_SIGNON)) {
            return -1;
        }
		sn = tdt->username[0]-96;
		pw = tdt->password[0]-96;
		d = sn * 7696 + 738816;
		e = sn * 746512;
		f = pw * d;
		g = f - d + e + 71665152;
		itoa(g,code,10);
		//toc2_login login.oscar.aol.com 29999 screenname 0x3900005d3b01 English "TIC:\Revision: 1.61 " 160 US "" "" 3 0 30303 -kentucky -utf8 94791632'
		mir_snprintf(snd, sizeof(snd), "toc2_login %s %d %s %s %s %s %s", host, 29999, aim_util_normalize(tdt->username),
		aim_util_roastpwd(tdt->password), LANGUAGE, REVISION, code);
		if (aim_toc_sflapsend(snd, -1, TYPE_DATA)) {
			return -1;
		}
		return len;
    }
    if (tdt->state == STATE_SIGNON) {
        if (_strnicmp(buf + sizeof(struct toc_sflap_hdr), "SIGN_ON", strlen("SIGN_ON"))) {
            if (!_strnicmp(buf + sizeof(struct toc_sflap_hdr), "ERROR", strlen("ERROR"))) {
                int error;
                c = strtok(buf + sizeof(struct toc_sflap_hdr), ":");
                error = atoi(strtok(NULL, ":"));
                toc_peformerror(error);
            }
            return -1;
        }
        tdt->state = STATE_ONLINE;
        LOG(LOG_DEBUG, "TOC Version: %s", buf + sizeof(struct toc_sflap_hdr) + 8);
        // force online if not
        if (!aim_util_isonline()) {
            int status = DBGetContactSettingWord(NULL, AIM_PROTO, AIM_KEY_IS, ID_STATUS_OFFLINE);
            aim_util_broadcaststatus(status);
            if (status == ID_STATUS_AWAY)
                aim_util_statusupdate();
        }
        aim_userinfo_send();
        aim_buddy_updateconfig(0);
        mir_snprintf(snd, sizeof(snd), "toc_init_done");
        aim_toc_sflapsend(snd, -1, TYPE_DATA);
        mir_snprintf(snd, sizeof(snd), "toc_set_caps %s %s %s", UID_ICQ_SUPPORT, UID_AIM_CHAT, UID_AIM_FILE_RECV);
        aim_toc_sflapsend(snd, -1, TYPE_DATA);
        return len;
    }
    c = strtok(buf + sizeof(struct toc_sflap_hdr), ":");
    if (!c)
        return len;
    LOG(LOG_DEBUG, "Packet type: %s", c);
    // ERROR:<Error Code>:Var args
    if (!_strcmpi(c, "ERROR")) {
        // ignore arguments for now
        int error = atoi(strtok(NULL, ":"));
        LOG(LOG_DEBUG, "Parsing ERROR");
        toc_peformerror(error);
        return len;
    }
	if (!_strcmpi(c, "NEW_BUDDY_REPLY2")) {
        char* ch=strtok(NULL, ":");
		MessageBox(0, Translate("Buddy Added Successfully!"),ch, MB_OK | MB_ICONINFORMATION);
    }
    // SIGN_ON:<Client Version Supported>
    if (!_strcmpi(c, "SIGN_ON")) {
        LOG(LOG_DEBUG, "Parsing SIGN_ON");
        if (tdt->state != STATE_PAUSE) {
            LOG(LOG_WARN, "SIGN_ON request without being in PAUSE state");
        }
        else {
            char host[256];
            int port;
            DBVARIANT dbv;

            if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_AS, &dbv)) {
                mir_snprintf(host, sizeof(host), "%s", dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else
                mir_snprintf(host, sizeof(host), "%s", AIM_AUTH_HOST);
            port = DBGetContactSettingWord(NULL, AIM_PROTO, AIM_KEY_TT, AIM_AUTH_PORT);
            tdt->state = STATE_ONLINE;
            mir_snprintf(snd, sizeof(snd), "toc2_signon %s %d %s %s %s %s", host, port, aim_util_normalize(tdt->username),
                         aim_util_roastpwd(tdt->password), LANGUAGE, REVISION);
            if (aim_toc_sflapsend(snd, -1, TYPE_DATA)) {
                return -1;
            }
            mir_snprintf(snd, sizeof(snd), "toc_init_done");
            aim_toc_sflapsend(snd, -1, TYPE_DATA);
        }
        return len;
    }
    // PAUSE
    else if (!_strcmpi(c, "PAUSE")) {
        tdt->state = STATE_PAUSE;
        LOG(LOG_DEBUG, "PAUSE state requested");
        return len;
    }
    // CONFIG:<config>
    else if (!_strcmpi(c, "CONFIG2")) {
        LOG(LOG_DEBUG, "Parsing CONFIG");
        c = strtok(NULL, "");
        aim_buddy_parseconfig(c);
        return len;
    }
    // NICK:<Nickname>
    else if (!_strcmpi(c, "NICK")) {
        DBVARIANT dbv;

        LOG(LOG_DEBUG, "Parsing NICK");
        c = strtok(NULL, ":");
        if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_NO, &dbv)) {
            if (strcmp(dbv.pszVal, c)) {
                LOG(LOG_INFO, "Updating Owner Nickname (%s)", c);
                DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_NO, c);
            }
            DBFreeVariant(&dbv);
        }
        else {
            LOG(LOG_INFO, "Setting Owner Nickname (%s)", c);
            DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_NO, c);
        }
        return len;
    }
    // UPDATE_BUDDY:<Buddy User>:<Online? T/F>:<Evil Amount>:<Signon Time>:<IdleTime>:<UC>
    else if (!_strcmpi(c, "UPDATE_BUDDY2")) {
        char *l, *uc;
        int logged, evil, idle, type = 0;
        time_t signon, time_idle;

        LOG(LOG_DEBUG, "Parsing UPDATE_BUDDY");
        c = strtok(NULL, ":");
        l = strtok(NULL, ":");
        sscanf(strtok(NULL, ":"), "%d", &evil);
        sscanf(strtok(NULL, ":"), "%ld", &signon);
        sscanf(strtok(NULL, ":"), "%d", &idle);
        uc = strtok(NULL, ":");
        logged = (l && (*l == 'T')) ? 1 : 0;
        if (uc) {
            if (uc[0] == 'A')
                type |= USER_AOL;
            switch (uc[1]) {
                case 'A':
                    type |= USER_ADMIN;
                    break;
                case 'U':
                    type |= USER_UNCONFIRMED;
                    break;
                case 'O':
                    type |= USER_NORMAL;
                    break;
                case 'C':
                    type |= USER_WIRELESS;
                    break;
            }
            if (uc[2] == 'U')
                type |= USER_UNAVAILABLE;
        }
        if (idle) {
            time(&time_idle);
            time_idle -= idle * 60;
        }
        else
            time_idle = 0;
        aim_buddy_update(c, logged, type, idle, evil, signon, time_idle);
        return len;
    }
    // EVILED:<new evil>:<name of eviler, blank if anonymous>
    else if (!_strcmpi(c, "EVILED")) {
        int level;
        char *sn;

        LOG(LOG_DEBUG, "Parsing EVILED");
        sscanf(strtok(NULL, ":"), "%d", &level);
        sn = strtok(NULL, ":");
        LOG(LOG_DEBUG, "You have been eviled (%d%%)", level);
        if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_WD, AIM_KEY_WD_DEF)) {
            aim_evil_update(sn, level);
        }
        DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_EV, level);
        return len;
    }
    // IM_IN:<Source User>:<Auto Response T/F?>:<Message>
    else if (!_strcmpi(c, "IM_IN_ENC2")) {
        char *message, *msg;
        int autoresponse = 0;
        CCSDATA ccs;
        PROTORECVEVENT pre;
        HANDLE hContact;
        LOG(LOG_DEBUG, "Parsing IM_IN");
        c = strtok(NULL, ":");
        message = strtok(NULL, ":");
		message = strtok(NULL, ":");
		message = strtok(NULL, ":");
		message = strtok(NULL, ":");
		message = strtok(NULL, ":");
		message = strtok(NULL, ":");
		message = strtok(NULL, ":");
        if (message && (*message == 'T'))
            autoresponse = 1;
        while (*message && (*message != ':'))
            message++;
        message++;

        if (autoresponse) {
            int len = strlen(Translate(AIM_STR_AR)) + 2 + strlen(message) + 1;
            char *m = malloc(len);
            msg = malloc(len);
            mir_snprintf(m, len, "%s: %s", Translate(AIM_STR_AR), message);
            aim_util_striphtml(msg, m, len);
            free(m);
        }
        else {
            msg = malloc(strlen(message) + 1);
            aim_util_striphtml(msg, message, strlen(message) + 1);
        }
        if (!msg || !strlen(msg)) {
            if (msg)
                free(msg);
            LOG(LOG_WARN, "Ignoring empty message from %s", c);
            return len;
        }
        ccs.szProtoService = PSR_MESSAGE;
        ccs.hContact = hContact = aim_buddy_get(c, 1, 0, 0, NULL);
        ccs.wParam = 0;
        ccs.lParam = (LPARAM) & pre;
        pre.flags = 0;
        pre.timestamp = time(NULL);
        pre.szMessage = msg;
        pre.lParam = 0;
        CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
        if (aimStatus == ID_STATUS_AWAY) {
            LOG(LOG_DEBUG, "Checking for possible auto-response message to send");
            aim_im_sendautoresponse(c);
        }
        DBWriteContactSettingDword(ccs.hContact, AIM_PROTO, AIM_KEY_LM, (DWORD) time(NULL));
        if (msg)
            free(msg);
        return len;
    }
    // GOTO_URL:<Window Name>:<Url>
    else if (!_strcmpi(c, "GOTO_URL")) {
        char *url;

        strtok(NULL, ":");
        url = _strdup(strtok(NULL, ":"));
        LOG(LOG_DEBUG, "Parsing GOTO_URL: %s", url);
        aim_util_parseurl(url);
        return len;
    }
    // ADMIN_PASSWD_STATUS:<Return Code>:<Optional args>
    else if (!_strcmpi(c, "ADMIN_PASSWD_STATUS")) {
        int ret = -1;

        LOG(LOG_DEBUG, "Parsing ADMIN_PASSWD_STATUS");
        sscanf(strtok(NULL, ":"), "%d", &ret);
        if (ret == 0) {
            LOG(LOG_DEBUG, "Password was successfully changed");
            aim_password_update(1);
        }
        return len;
    }
    // CHAT_JOIN:<Chat Room Id>:<Chat Room Name>
    else if (!_strcmpi(c, "CHAT_JOIN")) {
        int groupid;
        char *szRoom;

        LOG(LOG_DEBUG, "Parsing CHAT_JOIN");
        sscanf(strtok(NULL, ":"), "%d", &groupid);
        szRoom = strtok(NULL, ":");
        aim_gchat_create(groupid, szRoom);
        return len;
    }
    // CHAT_UPDATE_BUDDY:<Chat Room Id>:<Inside? T/F>:<User 1>:<User 2>...
    else if (!_strcmpi(c, "CHAT_UPDATE_BUDDY")) {
        int groupid, join;
        char *tmp;

        LOG(LOG_DEBUG, "Parsing CHAT_UPDATE_BUDDY");
        sscanf(strtok(NULL, ":"), "%d", &groupid);
        tmp = strtok(NULL, ":");
        join = *tmp == 'T' ? 1 : 0;
        while (tmp = strtok(NULL, ":")) {
            LOG(LOG_DEBUG, "Got chat update for %s in group %d", tmp, groupid);
            aim_gchat_updatebuddy(groupid, tmp, join);
        }
        return len;
    }
    // CHAT_IN:<Chat Room Id>:<Source User>:<Whisper? T/F>:<Message>
    else if (!_strcmpi(c, "CHAT_IN")) {
        int groupid, whisper;
        char *szUser, *tmp;

        LOG(LOG_DEBUG, "Parsing CHAT_IN");
        sscanf(strtok(NULL, ":"), "%d", &groupid);
        szUser = strtok(NULL, ":");
        tmp = strtok(NULL, ":");
        whisper = *tmp == 'T' ? 1 : 0;
        while (*tmp && (*tmp != ':'))
            tmp++;
        tmp++;
        aim_gchat_sendmessage(groupid, szUser, tmp, whisper);
        return len;
    }
    // CHAT_INVITE:<Chat Room Name>:<Chat Room Id>:<Invite Sender>:<Message>
    else if (!_strcmpi(c, "CHAT_INVITE")) {
        char *szRoom, *msg, *szUser, *chatid;

        LOG(LOG_DEBUG, "Parsing CHAT_INVITE");
        szRoom = strtok(NULL, ":");
        chatid = strtok(NULL, ":");
        msg = szUser = strtok(NULL, ":");
        while (*msg)
            msg++;
        msg++;
        LOG(LOG_DEBUG, "Received chat invitation from %s to room %s", szUser, szRoom, msg);
        aim_gchat_chatinvite(szRoom, szUser, chatid, msg);
        return len;
    }
    else if (!_strcmpi(c, "RVOUS_PROPOSE")) {
        char *user, *uuid, *cookie;
        int seq;
        char *rip, *pip, *vip, *trillian = NULL;
        int port;

        user = strtok(NULL, ":");
        uuid = strtok(NULL, ":");
        cookie = strtok(NULL, ":");
        sscanf(strtok(NULL, ":"), "%d", &seq);
        rip = strtok(NULL, ":");
        pip = strtok(NULL, ":");
        vip = strtok(NULL, ":");
        sscanf(strtok(NULL, ":"), "%d", &port);

        if (!strcmp(uuid, UID_AIM_FILE_RECV)) {
            HANDLE hContact;
            int unk[4], i;
            char *messages[4], *tmp, *name;
            int subtype, files, totalsize = 0;
            struct aim_filerecv_request *ft;
            CCSDATA ccs;
            PROTORECVEVENT pre;
            char *szBlob;

            hContact = aim_buddy_get(user, 0, 0, 0, NULL);
            if (!hContact) {
                LOG(LOG_WARN, "Received file receive request from contact not in list (ignoring)");
                aim_filerecv_deny(user, cookie);
                return len;
            }
            messages[0] = NULL;
            messages[1] = NULL;
            messages[2] = NULL;
            messages[3] = NULL;
            for (i = 0; i < 4; i++) {
                trillian = strtok(NULL, ":");
                sscanf(trillian, "%d", &unk[i]);
                if (unk[i] == 10001)
                    break;
                if (*(trillian + strlen(trillian) + 1) != ':') {
                    aim_util_base64decode(strtok(NULL, ":"), &messages[i]);
                }
            }
            aim_util_base64decode(strtok(NULL, ":"), &tmp);
            subtype = tmp[1];
            files = tmp[3];
            totalsize |= (tmp[4] << 24) & 0xff000000;
            totalsize |= (tmp[5] << 16) & 0x00ff0000;
            totalsize |= (tmp[6] << 8) & 0x0000ff00;
            totalsize |= (tmp[7] << 0) & 0x000000ff;
            if (!totalsize) {
                free(tmp);
                for (i--; i >= 0; i--)
                    free(messages[i]);
                return len;
            }
            name = tmp + 8;
            ft = (struct aim_filerecv_request *) malloc(sizeof(struct aim_filerecv_request));
            ZeroMemory(ft, sizeof(struct aim_filerecv_request));
            ft->current = 0;
            ft->cookie = _strdup(cookie);
            ft->ip = _strdup(pip);
            ft->vip = _strdup(vip);
            ft->port = port;
            if (i && messages[0]) {
                ft->message = (char *) malloc(strlen(messages[0]) + 1);
                // AIM sends descriptions as HTML (boo for them) :(
                aim_util_striphtml(ft->message, messages[0], strlen(messages[0]) + 1);
            }
            else
                ft->message = _strdup(Translate("No description given"));
            ft->filename = _strdup(name);
            ft->user = _strdup(aim_util_normalize(user));
            ft->size = totalsize;
            ft->files = files;
            mir_snprintf(ft->UID, sizeof(ft->UID), "%s", UID_AIM_FILE_RECV);
            ft->hContact = hContact;
            free(tmp);
            for (i--; i >= 0; i--)
                free(messages[i]);
            LOG(LOG_INFO, "[%s] Receieved file receive request from %s", ft->cookie, ft->user);
            // blob is DWORD(*ft), ASCIIZ(filenames), ASCIIZ(description)
            szBlob = (char *) malloc(sizeof(DWORD) + strlen(ft->filename) + strlen(ft->message) + 2);
            *((PDWORD) szBlob) = (DWORD) ft;
            strcpy(szBlob + sizeof(DWORD), ft->filename);
            strcpy(szBlob + sizeof(DWORD) + strlen(ft->filename) + 1, ft->message);
            pre.flags = 0;
            pre.timestamp = time(NULL);
            pre.szMessage = szBlob;
            pre.lParam = 0;
            ccs.szProtoService = PSR_FILE;
            ccs.hContact = ft->hContact;
            ccs.wParam = 0;
            ccs.lParam = (LPARAM) & pre;
            CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
            free(szBlob);
            return len;
        }
        else {
            LOG(LOG_INFO, "Received Unknown RVOUS_PROPOSE");
            return len;
        }
    }
    return len;
}

int aim_toc_sflapsend(char *buf, int olen, int type)
{
    int len;
    int slen = 0;
    struct toc_sflap_hdr hdr;
    char obuf[MSG_LEN];

    if (!hServerConn || !tdt || tdt->state == STATE_PAUSE)
        return 0;
    if (olen < 0)
        len = strlen(buf);
    else
        len = olen;
    if (len > MSG_LEN) {
        LOG(LOG_WARN, "Message too long, truncating");
        buf[MSG_LEN - 1] = '\0';
        len = MSG_LEN - 1;
    }
    hdr.ast = '*';
    hdr.type = type;
    hdr.seqno = htons(tdt->seqno++ & 0xffff);
    hdr.len = htons(len + (type == TYPE_SIGNON ? 0 : 1));
    LOG(LOG_DEBUG, "SFLAP: datalen=%d, type=%d", len, hdr.type);
    memcpy(obuf, &hdr, sizeof(hdr));
    slen += sizeof(hdr);
    memcpy(&obuf[slen], buf, len);
    slen += len;
    if (type != TYPE_SIGNON) {
        obuf[slen] = '\0';
        slen += 1;
    }
    return aim_util_netsend(obuf, slen);
}
