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

extern PLUGININFO pluginInfo;

int aim_util_isonline()
{
    if (aimStatus == ID_STATUS_ONLINE || aimStatus == ID_STATUS_AWAY)
        return 1;
    return 0;
}

int aim_util_netsend(char *data, int datalen)
{
    int rlen;

    if (!hServerConn || !tdt || tdt->state == STATE_PAUSE) {
        LOG(LOG_ERROR, "Server disconnected.  Packet not sent.");
        return 1;
    }
    rlen = Netlib_Send(hServerConn, data, datalen, 0);
    if (rlen == SOCKET_ERROR) {
        LOG(LOG_ERROR, "SEND Error -> %d [WSAGetLastError]", WSAGetLastError());
        return 1;
    }
    return 0;
}

void aim_util_broadcaststatus(int s)
{
    int oldStatus = aimStatus;
    if (oldStatus == s)
        return;
    aimStatus = s;

    if (aimStatus == ID_STATUS_AWAY) {
        DBWriteContactSettingDword(NULL, AIM_PROTO, AIM_KEY_LA, (DWORD) time(NULL));
    }
    ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) oldStatus, aimStatus);
    LOG(LOG_DEBUG, "Broadcasted new status (%s)", (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, aimStatus, 0));
    aim_gchat_updatestatus();
    aim_password_updatemenu();
}

int aim_util_userdeleted(WPARAM wParam, LPARAM lParam)
{
    if ((HANDLE) wParam == NULL)
        return 0;
    if (!aim_util_isonline())
        return 0;
    if (DBGetContactSettingByte((HANDLE) wParam, AIM_PROTO, AIM_CHAT, 0) == 0) {
        aim_buddy_delete((HANDLE) wParam);
    }
    else {
        aim_gchat_delete_by_contact((HANDLE) wParam);
    }
    return 0;
}

int aim_util_dbsettingchanged(WPARAM wParam, LPARAM lParam)
{
    DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;

    if ((HANDLE) wParam == NULL)
        return 0;
    if (!aim_util_isonline())
        return 0;
    if (!strcmp(cws->szModule, "CList")) {
        if (!strcmp(cws->szSetting, "NotOnList")) {
            if (DBGetContactSettingByte((HANDLE) wParam, "CList", "Hidden", 0))
                return 0;
            if (cws->value.type == DBVT_DELETED || (cws->value.type == DBVT_BYTE && cws->value.bVal == 0)) {
                char *szProto;

                LOG(LOG_DEBUG, "DB Setting changed.  AIM user was deleted.");
                szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) wParam, 0);
                if (szProto && !strcmp(szProto, AIM_PROTO)) {
                    DBDeleteContactSetting((HANDLE) wParam, AIM_PROTO, AIM_KEY_DU);
                }
            }
        }
    }
    else if (!strcmp(cws->szModule, AIM_PROTO) && !strcmp(cws->szSetting, AIM_KEY_AM)) {
        LOG(LOG_DEBUG, "DB Setting changed.  AIM user's visible setting changed.");
        aim_buddy_updatemode((HANDLE) wParam);
    }
    return 0;
}

void aim_util_striphtml(char *dest, const char *src, size_t destsize)
{
    char *ptr;
    char *ptrl;
    char *rptr;

    mir_snprintf(dest, destsize, "%s", src);
    while ((ptr = strstr(dest, "<P>")) != NULL || (ptr = strstr(dest, "<p>")) != NULL) {
        memmove(ptr + 4, ptr + 3, strlen(ptr + 3) + 1);
        *ptr = '\r';
        *(ptr + 1) = '\n';
        *(ptr + 2) = '\r';
        *(ptr + 3) = '\n';
    }
    while ((ptr = strstr(dest, "</P>")) != NULL || (ptr = strstr(dest, "</p>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        *(ptr + 2) = '\r';
        *(ptr + 3) = '\n';
    }
    while ((ptr = strstr(dest, "<BR>")) != NULL || (ptr = strstr(dest, "<br>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        memmove(ptr + 2, ptr + 4, strlen(ptr + 4) + 1);
    }
    while ((ptr = strstr(dest, "<HR>")) != NULL || (ptr = strstr(dest, "<hr>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        memmove(ptr + 2, ptr + 4, strlen(ptr + 4) + 1);
    }
    rptr = dest;
	while ((ptr = strstr(rptr, "<A HREF=\"")) || (ptr = strstr(rptr, "<a href=\""))) {
        ptrl = ptr + 8;
		memmove(ptr, ptrl + 1, strlen(ptrl + 1) + 1);
        if ((ptr = strstr(ptrl, "\">"))) {
			if ((ptrl = strstr(ptrl, "</A")) || (ptrl = strstr(ptrl, "</a"))) {
				memmove(ptr, ptrl + 4, strlen(ptrl + 4) + 1);
			}
        }
        else
            rptr++;
	}
   while ((ptr = strstr(rptr, "<"))) {
        ptrl = ptr + 1;
        if ((ptrl = strstr(ptrl, ">"))) {
            memmove(ptr, ptrl + 1, strlen(ptrl + 1) + 1);
        }
        else
            rptr++;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&quot;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '"';
        memmove(ptr + 1, ptr + 6, strlen(ptr + 6) + 1);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&lt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '<';
        memmove(ptr + 1, ptr + 4, strlen(ptr + 4) + 1);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&gt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '>';
        memmove(ptr + 1, ptr + 4, strlen(ptr + 4) + 1);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&amp;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '&';
        memmove(ptr + 1, ptr + 5, strlen(ptr + 5) + 1);
        ptrl = ptr;
    }
}

int aim_util_escape(char *msg)
{
    char *c, *cpy;
    int cnt = 0;

    if (strlen(msg) > MSG_LEN) {
        LOG(LOG_WARN, "Truncating message to %d bytes.  The world is coming to an end!", MSG_LEN);
        msg[MSG_LEN - 1] = '\0';
    }
    cpy = _strdup(msg);
    c = cpy;
    while (*c) {
        switch (*c) {
            case '\r':
                // turn carriage return into and html <BR> tag
                // aim clients only understand html
                if (*(c + 1) == '\n') {
                    msg[cnt++] = '<';
                    msg[cnt++] = 'B';
                    msg[cnt++] = 'R';
                    msg[cnt++] = '>';
                    c++;
                }
                else {
                    msg[cnt++] = *c;
                }
                break;
            case '\n':
                // same as above except there wasn't a \r in front of it
                msg[cnt++] = '<';
                msg[cnt++] = 'B';
                msg[cnt++] = 'R';
                msg[cnt++] = '>';
                break;
            case '&':
                msg[cnt++] = '&';
                msg[cnt++] = 'a';
                msg[cnt++] = 'm';
                msg[cnt++] = 'p';
                msg[cnt++] = ';';
                break;
            case '<':
                msg[cnt++] = '&';
                msg[cnt++] = 'l';
                msg[cnt++] = 't';
                msg[cnt++] = ';';
                break;
            case '>':
                msg[cnt++] = '&';
                msg[cnt++] = 'g';
                msg[cnt++] = 't';
                msg[cnt++] = ';';
                break;
            case '"':
            case '$':
            case '[':
            case ']':
            case '(':
            case ')':
            case '\\':
                msg[cnt++] = '\\';      // fall-through
            default:
                msg[cnt++] = *c;
        }
        c++;
    }
    msg[cnt] = '\0';
    free(cpy);
    return cnt;
}
int aim_util_profile_escape(char *msg)
{
    char *c, *cpy;
    int cnt = 0;

    if (strlen(msg) > MSG_LEN) {
        LOG(LOG_WARN, "Truncating message to %d bytes.  The world is coming to an end!", MSG_LEN);
        msg[MSG_LEN - 1] = '\0';
    }
    cpy = _strdup(msg);
    c = cpy;
    while (*c) {
        switch (*c) {
            case '\r':
                // turn carriage return into and html <BR> tag
                // aim clients only understand html
                if (*(c + 1) == '\n') {
                    msg[cnt++] = '<';
                    msg[cnt++] = 'B';
                    msg[cnt++] = 'R';
                    msg[cnt++] = '>';
                    c++;
                }
                else {
                    msg[cnt++] = *c;
                }
                break;
            case '\n':
                // same as above except there wasn't a \r in front of it
                msg[cnt++] = '<';
                msg[cnt++] = 'B';
                msg[cnt++] = 'R';
                msg[cnt++] = '>';
                break;
            case '&':
                msg[cnt++] = '&';
                msg[cnt++] = 'a';
                msg[cnt++] = 'm';
                msg[cnt++] = 'p';
                msg[cnt++] = ';';
                break;
            case '"':
            case '$':
            case '[':
            case ']':
            case '(':
            case ')':
            case '\\':
                msg[cnt++] = '\\';      // fall-through
            default:
                msg[cnt++] = *c;
        }
        c++;
    }
    msg[cnt] = '\0';
    free(cpy);
    return cnt;
}
void aim_util_browselostpwd()
{
    CallService(MS_UTILS_OPENURL, (WPARAM) 1, (LPARAM) AIM_LOSTPW);
}

int aim_util_randomnum(int lowrange, int highrange)
{
    int res;

    res = (int) ((highrange + 1) * rand() / (RAND_MAX + 1.0)) + lowrange;
    return res;
}

// The next two functions were adopted from GAIM's TOC implementation
char *aim_util_normalize(const char *s)
{
    static char buf[MSG_LEN];
    int i, j;

    if (s == NULL)
        return NULL;
    strncpy(buf, s, MSG_LEN);
    for (i = 0, j = 0; buf[j]; i++, j++) {
        while (buf[j] == ' ')
            j++;
        buf[i] = tolower(buf[j]);
    }
    buf[i] = '\0';
    return buf;
}

unsigned char *aim_util_roastpwd(char *pass)
{
    static unsigned char rp[256];
    static char *roast = ROAST;
    int pos;
    int x;

    strcpy(rp, "0x");
    pos = strlen(rp);
    for (x = 0; (x < 150) && pass[x]; x++)
        pos += sprintf(&rp[pos], "%02x", pass[x] ^ roast[x % strlen(roast)]);
    rp[pos] = '\0';
    return rp;
}

static DWORD hLastStatusUpdate = 0;

void aim_util_statusupdate()
{
    char snd[MSG_LEN * 2];
    if (!aim_util_isonline())
        return;
    if (aimStatus == ID_STATUS_ONLINE) {
        if (szStatus) {
            free(szStatus);
            szStatus = NULL;
        }
        mir_snprintf(snd, MSG_LEN, "toc_set_away");
        hLastStatusUpdate = 0;
    }
    else {
        char *szTmp;

        if (szStatus && (char *) szModeMsg && !strcmp(szStatus, szModeMsg)) {
            return;
        }
        if (szStatus)
            free(szStatus);
        szStatus = szModeMsg ? strdup(szModeMsg) : strdup((char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM) ID_STATUS_AWAY, 0));
        szTmp = malloc(strlen(szStatus) + MSG_LEN);
        strcpy(szTmp, szStatus);
        aim_util_escape(szTmp);
        mir_snprintf(snd, MSG_LEN, "toc_set_away \"%s\"", szTmp);
        free(szTmp);
        if ((GetTickCount() - hLastStatusUpdate) < 1000) {
            LOG(LOG_WARN, "Sending status message updates too fast.  Ignoring request.");
            return;
        }
        hLastStatusUpdate = GetTickCount();
    }
    aim_toc_sflapsend(snd, -1, TYPE_DATA);
}

/*static void __cdecl aim_util_parseurlthread(void *url)
{
    // This thread will process a GOTO URL sent by the TOC server
    DBVARIANT dbv;

    if (!aim_util_isonline()) {
        free(url);
        return;
    }
    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_SA, &dbv)) {
		char host[256];
		int port;
        NETLIBHTTPREQUEST nlhr, *nlreply;
        NETLIBHTTPHEADER headers[4];
        char szURL[256];
		mir_snprintf(host, 256, dbv.pszVal);
		DBFreeVariant(&dbv);
		port=DBGetContactSettingWord(NULL, AIM_PROTO, AIM_KEY_TT,AIM_TOC_PORT);
        ZeroMemory(&nlhr, sizeof(nlhr));
		mir_snprintf(host, 256, "%s:%d",host,port);
		mir_snprintf(szURL, 256, "http://%s/%s",host,(char *) url);
		CallService(MS_UTILS_OPENURL , 1, (LPARAM)(const char*)szURL);
        nlhr.cbSize = sizeof(nlhr);
        nlhr.requestType = REQUEST_GET;
        nlhr.flags = NLHRF_DUMPASTEXT;
        nlhr.szUrl = szURL;
        nlhr.headersCount = 3;
        nlhr.headers = headers;
        headers[0].szName = "User-Agent";
        headers[0].szValue = "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)";
        headers[1].szName = "Host";
        headers[1].szValue = host;
        headers[2].szName = "Accept";
        headers[2].szValue = "**";//if uncommenting insert '/' between the two stars.
        nlreply = (NETLIBHTTPREQUEST *) CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM) hNetlib, (LPARAM) & nlhr);
        if (nlreply) {
            if (nlreply->resultCode >= 200 && nlreply->resultCode < 300 && nlreply->dataLength) {
                // Dir Search Reply
                if (strstr(nlreply->pData, "<TITLE>Dir Information</TITLE>")) {
                    PROTOSEARCHRESULT psr;

                    if (strstr(nlreply->pData, "Not found.</BODY></HTML>")) {
                        ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
                    }
                    else {
                        char *current = nlreply->pData, *strend, *str, *sstr, *sstrend, nick[256], fname[256], lname[256], data[2048];

                        while (str = strstr(current, "<TABLE>")) {
                            str += 45;
                            if (strend = strstr(str, "</TABLE>")) {
                                *strend = '\0';
                                nick[0] = 0;
                                fname[0] = 0;
                                lname[0] = 0;
                                mir_snprintf(data, sizeof(data), "%s", str);
                                if (sstr = strstr(data, "<TR><TD>Screen Name:</TD><TD><B>")) {
                                    sstr += 32;
                                    if (sstrend = strstr(sstr, "</B></TD></TR>")) {
                                        *sstrend = '\0';
                                        mir_snprintf(nick, sizeof(nick), "%s", sstr);
                                    }
                                }
                                mir_snprintf(data, sizeof(data), "%s", str);
                                if (sstr = strstr(data, "<TR><TD>First Name:</TD><TD><B>")) {
                                    sstr += 31;
                                    if (sstrend = strstr(sstr, "</B></TD></TR>")) {
                                        *sstrend = '\0';
                                        mir_snprintf(fname, sizeof(fname), "%s", sstr);
                                    }
                                }
                                mir_snprintf(data, sizeof(data), "%s", str);
                                if (sstr = strstr(data, "<TR><TD>Last Name:</TD><TD><B>")) {
                                    sstr += 30;
                                    if (sstrend = strstr(sstr, "</B></TD></TR>")) {
                                        *sstrend = '\0';
                                        mir_snprintf(lname, sizeof(lname), "%s", sstr);
                                    }
                                }
                                ZeroMemory(&psr, sizeof(psr));
                                psr.cbSize = sizeof(psr);
                                psr.nick = nick;
                                psr.firstName = fname;
                                psr.lastName = lname;
                                ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
                                current = strend + 1;
                            }
                            else
                                break;
                        }
                        ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
                    }

                }
                // User Info Request
                if (strstr(nlreply->pData, "<TITLE>User Information for")) {
                    char *str, *strend, *nick;
                    char info[2048];
                    HANDLE hContact;
                    int wFound = 0;


                    if (str = strstr(nlreply->pData, "Username : <B>")) {
                        str += 14;
                        if (strend = strstr(str, "</B>")) {
                            *strend = 0;
                            nick = aim_util_normalize(str);
                            hContact = aim_buddy_get(nick, 0, 0, 0, NULL);
                            if (hContact)
                                DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_UU, szURL);
                            strend++;
                            if (str = strstr(strend, "<hr><br>")) {
                                str += 9;
                                if (strend = strstr(str, "<br><hr>")) {
                                    *strend = 0;
                                    aim_util_striphtml(info, str, sizeof(info));
                                    if (hContact && strlen(info)) {
                                        wFound = 1;
                                        DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_PR, info);
                                    }
                                }
                            }
                            if (!wFound)
                                DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_PR, Translate("No User information Provided"));
                            ProtoBroadcastAck(AIM_PROTO, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
                        }
                    }
                }
            }
            CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM) nlreply);
        }
        DBFreeVariant(&dbv);
    }
    free(url);
}

void aim_util_parseurl(char *url)
{
    // url must be freeable and will be freed when we are done
    pthread_create(aim_util_parseurlthread, (void *) url);
}*/

void aim_util_formatnick(char *nick)
{
    char buf[MSG_LEN * 2];

    mir_snprintf(buf, sizeof(buf), "toc_format_nickname \"%s\"", nick);
    aim_toc_sflapsend(buf, -1, TYPE_DATA);
}

int aim_util_isme(char *nick)
{
    DBVARIANT dbv;
    char *sn = aim_util_normalize(nick);

    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_UN, &dbv)) {
        if (!strcmp(dbv.pszVal, sn)) {
            DBFreeVariant(&dbv);
            return 1;
        }
        DBFreeVariant(&dbv);
    }
    return 0;
}

static SIMPLEWINDOWLISTENTRY *windowList = NULL;
static int windowListCount = 0;
static int nextWindowListId = 1;

HANDLE aim_util_winlistalloc()
{
    return (HANDLE) nextWindowListId++;
}

void aim_util_winlistadd(HANDLE hList, HWND hwnd)
{
    SIMPLEWINDOWLISTENTRY wl;

    ZeroMemory(&wl, sizeof(wl));
    wl.hList = hList;
    wl.hwnd = hwnd;
    windowList = (SIMPLEWINDOWLISTENTRY *) realloc(windowList, sizeof(SIMPLEWINDOWLISTENTRY) * (windowListCount + 1));
    windowList[windowListCount++] = wl;
}

void aim_util_winlistdel(HANDLE hList, HWND hwnd)
{
    int i;
    for (i = 0; i < windowListCount; i++)
        if (windowList[i].hwnd == hwnd && windowList[i].hList == hList) {
            MoveMemory(&windowList[i], &windowList[i + 1], sizeof(WINDOWLISTENTRY) * (windowListCount - i - 1));
            windowListCount--;
        }
}

void aim_util_winlistbroadcast(HANDLE hList, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i;
    MSG msg;

    msg.message = message;
    msg.wParam = wParam;
    msg.lParam = lParam;
    for (i = 0; i < windowListCount; i++)
        if (windowList[i].hList == hList)
            SendMessage(windowList[i].hwnd, msg.message, msg.wParam, msg.lParam);
}

int aim_util_isicquserbyhandle(HANDLE hContact)
{
    DBVARIANT dbv;

    if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)) {
        int ret = 0;

        ret = aim_util_isicquser(dbv.pszVal);
        DBFreeVariant(&dbv);
        return ret;
    }
    else
        return 0;
}

int aim_util_isicquser(char *sn)
{
    char *tmp = sn;

    if (!strlen(tmp))
        return 0;
    while (*tmp) {
        if (!isdigit(*tmp))
            return 0;
        tmp++;
    }
    return 1;
}

int aim_util_shownotification(char *title, char *info, DWORD flags)
{
    if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
        MIRANDASYSTRAYNOTIFY err;
        err.szProto = AIM_PROTO;
        err.cbSize = sizeof(err);
        err.szInfoTitle = title;
        err.szInfo = info;
        err.dwInfoFlags = flags;
        err.uTimeout = 1000 * 5;
        CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM) & err);
        return 1;
    }
    return 0;
}

void aim_utils_logversion()
{
    char str[256];

#ifdef AIM_CVSBUILD
    mir_snprintf(str, sizeof(str), "AimTOC v%d.%d.%d.%da (%s %s)", (pluginInfo.version >> 24) & 0xFF, (pluginInfo.version >> 16) & 0xFF,
                 (pluginInfo.version >> 8) & 0xFF, pluginInfo.version & 0xFF, __DATE__, __TIME__);
#else
    mir_snprintf(str, sizeof(str), "AimTOC v%d.%d.%d.%d", (pluginInfo.version >> 24) & 0xFF, (pluginInfo.version >> 16) & 0xFF,
                 (pluginInfo.version >> 8) & 0xFF, pluginInfo.version & 0xFF);
#endif
    LOG(LOG_INFO, str);
#ifdef AIM_CVSBUILD
    LOG(LOG_INFO, "You are using a development version of AIM.  Please make sure you are using the latest version before posting bug reports.");
#endif
}

void aim_util_base64decode(char *in, char **out)
{
    NETLIBBASE64 b64;
    char *data;

    if (in == NULL || strlen(in) == 0) {
        *out = NULL;
        return;
    }
    data = (char *) malloc(Netlib_GetBase64DecodedBufferSize(strlen(in)) + 1);
    b64.pszEncoded = in;
    b64.cchEncoded = strlen(in);
    b64.pbDecoded = data;
    b64.cbDecoded = Netlib_GetBase64DecodedBufferSize(b64.cchEncoded);
    CallService(MS_NETLIB_BASE64DECODE, 0, (LPARAM) & b64);
    data[b64.cbDecoded] = 0;
    *out = data;
}
unsigned char *make_utf8_string(const wchar_t *unicode)
{

    int size = 0;
    int index = 0;
     int out_index = 0;
    unsigned char* out;
    unsigned short c;


    /* first calculate the size of the target string */
    c = unicode[index++];
    while(c) {
        if(c < 0x0080) {
            size += 1;
        } else if(c < 0x0800) {
            size += 2;
        } else {
            size += 3;
        }
        c = unicode[index++];
    }

    out = malloc(size + 1);
    if (out == NULL)
        return NULL;
    index = 0;

    c = unicode[index++];
    while(c)
    {
        if(c < 0x080) {
            out[out_index++] = (unsigned char)c;
        } else if(c < 0x800) {
            out[out_index++] = 0xc0 | (c >> 6);
            out[out_index++] = 0x80 | (c & 0x3f);
        } else {
            out[out_index++] = 0xe0 | (c >> 12);
            out[out_index++] = 0x80 | ((c >> 6) & 0x3f);
            out[out_index++] = 0x80 | (c & 0x3f);
        }
        c = unicode[index++];
    }
    out[out_index] = 0x00;

    return out;
}

wchar_t *make_unicode_string(const unsigned char *utf8)
{

    int size = 0, index = 0, out_index = 0;
    wchar_t *out;
    unsigned char c;


    /* first calculate the size of the target string */
    c = utf8[index++];
    while(c) {
        if((c & 0x80) == 0) {
            index += 0;
        } else if((c & 0xe0) == 0xe0) {
            index += 2;
        } else {
            index += 1;
        }
        size += 1;
        c = utf8[index++];
    }

    out = malloc((size + 1) * sizeof(wchar_t));
    if (out == NULL)
        return NULL;
    index = 0;

    c = utf8[index++];
    while(c)
    {
        if((c & 0x80) == 0) {
            out[out_index++] = c;
        } else if((c & 0xe0) == 0xe0) {
            out[out_index] = (c & 0x1F) << 12;
          c = utf8[index++];
            out[out_index] |= (c & 0x3F) << 6;
          c = utf8[index++];
            out[out_index++] |= (c & 0x3F);
        } else {
            out[out_index] = (c & 0x3F) << 6;
          c = utf8[index++];
            out[out_index++] |= (c & 0x3F);
        }
        c = utf8[index++];
    }
    out[out_index] = 0;

    return out;
}
