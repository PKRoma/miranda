/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2004 Robert Rainwater

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

typedef struct
{
    char *szUser;
}
TTOCUserDeleted;

typedef struct
{
    char *szUser;
    int deny;
    int permit;
}
TTOCUser;

typedef struct
{
    char *szGroup;
    int cCount;
    TTOCUser *hChild;
}
TTOCGroup;

pthread_mutex_t buddyMutex;
int hServerSideList = 0;
static int hSetConfig = 0;
static TTOCGroup *hGroup = NULL;
static int hGroupCount = 0;
static int hMode = 1;
static TTOCUserDeleted *hUserDeleted = NULL;
static int hUserDeletedCount = 0;

static void aim_buddy_delaydelete(char *szUser)
{
    pthread_mutex_lock(&buddyMutex);
    LOG(LOG_DEBUG, "Added user to delayed delete (%s)", szUser);
    hUserDeleted = (TTOCUserDeleted *) realloc(hUserDeleted, sizeof(TTOCUserDeleted) * (hUserDeletedCount + 1));
    hUserDeleted[hUserDeletedCount].szUser = _strdup(szUser);
    hUserDeletedCount++;
    pthread_mutex_unlock(&buddyMutex);
}

void aim_buddy_delaydeletefree()
{
    int i;
    pthread_mutex_lock(&buddyMutex);
    if (hUserDeletedCount) {
        for (i = 0; i < hUserDeletedCount; i++) {
            free(hUserDeleted[i].szUser);
        }
        free(hUserDeleted);
        hUserDeleted = 0;
        hUserDeletedCount = 0;
    }
    pthread_mutex_unlock(&buddyMutex);
}

static int aim_buddy_delaydeletecheck(char *szUser)
{
    int i;
    pthread_mutex_lock(&buddyMutex);
    LOG(LOG_DEBUG, "Checking for user to delete (%s)", szUser);
    for (i = 0; i < hUserDeletedCount; i++) {
        if (!strcmp(szUser, hUserDeleted[i].szUser))
            return 1;
    }
    pthread_mutex_unlock(&buddyMutex);
    return 0;
}

HANDLE aim_buddy_get(char *nick, int create, int inlist, int noadd, char *group)
{
    HANDLE hContact;
    DBVARIANT dbv;
    char *szProto;
    char *sn = aim_util_normalize(nick);

    pthread_mutex_lock(&buddyMutex);
    if (!strcmp(sn, "pleaseupgrade000")) {
        pthread_mutex_unlock(&buddyMutex);
        return NULL;
    }
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
        szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
        if (szProto != NULL && !strcmp(szProto, AIM_PROTO)) {
            if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                if (!strcmp(dbv.pszVal, sn)) {
                    if (inlist) {
                        DBDeleteContactSetting(hContact, "CList", "NotOnList");
                        DBDeleteContactSetting(hContact, "CList", "Hidden");
                    }
                    DBFreeVariant(&dbv);
                    pthread_mutex_unlock(&buddyMutex);
                    return hContact;
                }
                else
                    DBFreeVariant(&dbv);
            }
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
    if (!create) {
        pthread_mutex_unlock(&buddyMutex);
        return NULL;
    }
    hContact = (HANDLE) CallService(MS_DB_CONTACT_ADD, 0, 0);
    if (!hContact) {
        LOG(LOG_ERROR, "Failed to create AIM contact %s. MS_DB_CONTACT_ADD failed.", nick);
        pthread_mutex_unlock(&buddyMutex);
        return NULL;
    }
    if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM) hContact, (LPARAM) AIM_PROTO) != 0) {
        // For some reason we failed to register the protocol for this contact
        CallService(MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
        LOG(LOG_ERROR, "Failed to register AIM contact %s.  MS_PROTO_ADDTOCONTACT failed.", nick);
        pthread_mutex_unlock(&buddyMutex);
        return NULL;
    }
    if (group && strlen(group)) {
        aim_group_create(group);
        aim_group_adduser(hContact, group);
    }
    LOG(LOG_INFO, "Added AIM Contact: %s", nick);
    if (!inlist) {
        DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
        DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
    }
    DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_UN, sn);
    DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_NK, sn);
    DBWriteContactSettingWord(hContact, AIM_PROTO, AIM_KEY_ST, ID_STATUS_OFFLINE);
    if (aim_util_isonline() && !noadd) {
        char buf[MSG_LEN];

        _snprintf(buf, sizeof(buf), "toc_add_buddy %s", sn);
        aim_toc_sflapsend(buf, -1, TYPE_DATA);
        if (hSetConfig) {
            SleepEx(500, FALSE);
            aim_buddy_setconfig();
        }
    }
    pthread_mutex_unlock(&buddyMutex);
    return hContact;
}

void aim_buddy_updatemode(HANDLE hContact)
{
    char *szProto;
    DBVARIANT dbv;

    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto != NULL && !strcmp(szProto, AIM_PROTO)) {
        if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)
            && !DBGetContactSettingByte(hContact, AIM_PROTO, AIM_KEY_DU, 0)     // User is being deleted, don't add
            && !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0)) {
            char buf[MSG_LEN * 2];
            if (ID_STATUS_OFFLINE == DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, 0)) {
                _snprintf(buf, sizeof(buf), "toc_add_deny %s", dbv.pszVal);
            }
            else {
                _snprintf(buf, sizeof(buf), "toc_add_permit %s", dbv.pszVal);
            }
            aim_toc_sflapsend(buf, -1, TYPE_DATA);
            if (hSetConfig)
                aim_buddy_setconfig();
            DBFreeVariant(&dbv);
        }
    }
}

void aim_buddy_offlineall()
{
    HANDLE hContact;
    char *szProto;

    LOG(LOG_DEBUG, "aim_buddy_offlineall(): Setting buddies offline");
    DBWriteContactSettingWord(NULL, AIM_PROTO, AIM_KEY_ST, ID_STATUS_OFFLINE);
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
        szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
        if (szProto != NULL && !strcmp(szProto, AIM_PROTO)) {
            DBWriteContactSettingWord(hContact, AIM_PROTO, AIM_KEY_ST, ID_STATUS_OFFLINE);
            DBWriteContactSettingDword(hContact, AIM_PROTO, AIM_KEY_OT, 0);     // reset online time
            DBWriteContactSettingDword(hContact, AIM_PROTO, AIM_KEY_IT, 0);     // reset idle time
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
    aimStatus = ID_STATUS_OFFLINE;
    hSetConfig = 0;
}

void aim_buddy_delete(HANDLE hContact)
{
    char *szProto;

    if (!hContact)
        return;
    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto && !strcmp(szProto, AIM_PROTO)) {
        DBWriteContactSettingByte(hContact, AIM_PROTO, AIM_KEY_DU, 1);
        if (!hServerSideList)
            return;
        if (DBGetContactSettingByte(hContact, "CList", "Delete", 0)) {
            DBVARIANT dbv;

            if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                // add user to delete queue
                aim_buddy_delaydelete(dbv.pszVal);
                DBFreeVariant(&dbv);
            }
        }
        else if (hSetConfig) {
            aim_buddy_setconfig();
        }
    }
}

static void aim_buddy_updatedetails(HANDLE hContact, char *nick, char *sn, int online, int type, int idle, int evil, time_t signon, time_t idle_time)
{
    DBVARIANT dbv;
    long currentTime;
    int current = DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_ST, ID_STATUS_OFFLINE);
    int newstatus = online ? type & USER_UNAVAILABLE ? ID_STATUS_AWAY : ID_STATUS_ONLINE : ID_STATUS_OFFLINE;

	if ( type & USER_WIRELESS ) newstatus = ID_STATUS_ONTHEPHONE;

    if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_NK, &dbv)) {
        if (strcmp(dbv.pszVal, nick)) {
            LOG(LOG_DEBUG, "Updating Nickname (%s=>%s)", dbv.pszVal, nick);
            DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_NK, nick);
        }
        DBFreeVariant(&dbv);
    }
    else {
        LOG(LOG_DEBUG, "Setting Nickname (%s)", nick);
        DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_NK, nick);
    }
    if (current != newstatus) {
        char *name = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0);
        if (name)
            LOG(LOG_INFO, "%s (%s) changed status to %s.", name, sn, (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, newstatus, 0));
        DBWriteContactSettingWord(hContact, AIM_PROTO, AIM_KEY_ST, newstatus);
    }
    current = DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_EV, 0);
    if (current != evil) {
        LOG(LOG_DEBUG, "Set Evil Mode (%s, %d)", sn, evil);
        DBWriteContactSettingWord(hContact, AIM_PROTO, AIM_KEY_EV, evil);
    }
    current = DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_TP, 0);
    if (current != type) {
        LOG(LOG_DEBUG, "Set User Type (%s, %d)", sn, type);
        DBWriteContactSettingWord(hContact, AIM_PROTO, AIM_KEY_TP, type);
    }
    currentTime = DBGetContactSettingDword(hContact, AIM_PROTO, AIM_KEY_OT, 0);
    if (currentTime != signon) {
        char buf[256];
        strftime(buf, sizeof(buf), "%b %d %H:%M:%S GMT", gmtime(&signon));
        LOG(LOG_DEBUG, "Set Online Time (%s, %s)", sn, signon > 0 ? buf : "N/A");
        DBWriteContactSettingDword(hContact, AIM_PROTO, AIM_KEY_OT, signon);
    }
    currentTime = DBGetContactSettingDword(hContact, AIM_PROTO, AIM_KEY_IT, 0);
    if (currentTime != idle_time) {
        char buf[256];
        strftime(buf, sizeof(buf), "%b %d %H:%M:%S GMT", gmtime(&idle_time));
        LOG(LOG_DEBUG, "Set Idle Time (%s, %s)", sn, idle_time > 0 ? buf : "N/A");
        DBWriteContactSettingDword(hContact, AIM_PROTO, AIM_KEY_IT, idle_time);
    }
}

void aim_buddy_update(char *nick, int online, int type, int idle, int evil, time_t signon, time_t idle_time)
{
    HANDLE hContact;
    char *szProto;
    DBVARIANT dbv;
    char *sn = aim_util_normalize(nick);

    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
        szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
        if (szProto != NULL && !strcmp(szProto, AIM_PROTO)) {
            if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                if (!strcmp(dbv.pszVal, sn)) {
                    aim_buddy_updatedetails(hContact, nick, dbv.pszVal, online, type, idle, evil, signon, idle_time);
                    DBFreeVariant(&dbv);
                    return;
                }
                else
                    DBFreeVariant(&dbv);
            }
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
}

void aim_buddy_parseconfig(char *config)
{
    LOG(LOG_DEBUG, "aim_buddy_parseconfig: Parsing configuation from server if using server-side lists");
    if (!config)
        return;
    if (hServerSideList && (aim_buddy_cfglen() > MSG_LEN || strlen(config) > (MSG_LEN - strlen("CONFIG:")))) {
        hServerSideList = 0;
        LOG(LOG_WARN, "List too big, turning off server-side list support");
    }
    if (hServerSideList || !DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_IL, 0)) {
        char *c, group[256];
        HANDLE hContact;

        group[0] = '\0';
        DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_IL, 1);
        if (config != NULL) {
            c = strtok(config, "\n");
            do {
                if (c == NULL)
                    break;
                if (*c == 'g') {
                    _snprintf(group, sizeof(group), c + 2);
                    LOG(LOG_DEBUG, "Parsed group from server config: (%s)", group);
                }
                else if (*c == 'b') {
                    char nm[80], *a;
                    if ((a = strchr(c + 2, ':')) != NULL)
                        *a++ = '\0';
                    _snprintf(nm, sizeof(nm), c + 2);
                    if (!aim_buddy_delaydeletecheck(nm)) {
                        LOG(LOG_DEBUG, "Parsed buddy from server config (%s) in %s", nm, group);
                        hContact = aim_buddy_get(nm, 1, 1, 1, group);
                        if (hContact) {
                            // reset permission for user, gets set back on d and p items if present
                            DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, 0);
                            LOG(LOG_DEBUG, "Setting server-side list group for %s (%s)", nm, group);
                            DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_SG, group);
                        }
                    }
                }
                else if (*c == 'd') {
                    char nm[80];
                    _snprintf(nm, sizeof(nm), c + 2);
                    LOG(LOG_DEBUG, "Parsed deny mode from server config (%s)", nm);
                    hContact = aim_buddy_get(nm, 1, 1, 1, group);
                    if (hContact) {
                        LOG(LOG_DEBUG, "Updating deny mode for contact (%s)", nm);
                        DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, ID_STATUS_OFFLINE);
                    }

                }
                else if (*c == 'p') {
                    char nm[80];
                    _snprintf(nm, sizeof(nm), c + 2);
                    LOG(LOG_DEBUG, "Parsed permit mode from server config (%s)", nm);
                    hContact = aim_buddy_get(nm, 1, 1, 1, group);
                    if (hContact) {
                        LOG(LOG_DEBUG, "Updating permit mode for contact (%s)", nm);
                        DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, ID_STATUS_ONLINE);
                    }
                }
                else if (*c == 'm') {
                    int m;

                    sscanf(c + 2, "%d", &m);
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_SM, m);
                }
            } while ((c = strtok(NULL, "\n")));
        }
    }
    aim_buddy_setconfig();
    aim_buddy_updateconfig();
}

void aim_buddy_updateconfig()
{
    HANDLE hContact;
    char *szProto;
    DBVARIANT dbv;
    int n;
    int m;
    char buf[MSG_LEN * 2];
    char dbuf[MSG_LEN * 2];
    if (!aim_util_isonline())
        return;

	n = _snprintf(buf, sizeof(buf), "toc_add_buddy");
    m = _snprintf(dbuf, sizeof(dbuf), "toc_add_deny");

    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
        szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
        if (szProto != NULL && !strcmp(szProto, AIM_PROTO)) {
            ZeroMemory(&dbv, sizeof(dbv));
            if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)
                && !DBGetContactSettingByte(hContact, AIM_PROTO, AIM_KEY_DU, 0) // User is being deleted, don't add
                && !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0)
                && !aim_buddy_delaydeletecheck(dbv.pszVal)) {
                if (strlen(dbv.pszVal) + n + 32 > MSG_LEN) {
                    aim_toc_sflapsend(buf, -1, TYPE_DATA);
                    n = _snprintf(buf, sizeof(buf), "toc_add_buddy");
                }
                if (ID_STATUS_OFFLINE == DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, 0)) {
                    LOG(LOG_DEBUG, "Setting deny mode for %s", dbv.pszVal);
                    if (strlen(dbv.pszVal) + m + 32 > MSG_LEN) {
                        aim_toc_sflapsend(dbuf, -1, TYPE_DATA);
                        m = _snprintf(dbuf, sizeof(dbuf), "toc_add_deny");
                    }
                    m += _snprintf(dbuf + m, sizeof(dbuf) - m, " %s", dbv.pszVal);
                }
                n += _snprintf(buf + n, sizeof(buf) - n, " %s", dbv.pszVal);
            }
            if (dbv.pszVal)
                DBFreeVariant(&dbv);
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
    if (n > lstrlen("toc_add_buddy"))
        aim_toc_sflapsend(buf, -1, TYPE_DATA);
    if (m > lstrlen("toc_add_deny"))
        aim_toc_sflapsend(dbuf, -1, TYPE_DATA);
}

static int aim_buddy_cfgaddgroup(char *szGroup)
{
    int i;
    for (i = 0; i < hGroupCount; i++) {
        if (!strcmp(hGroup[i].szGroup, szGroup))
            return i;
    }
    hGroup = (TTOCGroup *) realloc(hGroup, sizeof(TTOCGroup) * (hGroupCount + 1));
    hGroup[i].szGroup = _strdup(szGroup);
    hGroup[i].cCount = 0;
    hGroup[i].hChild = NULL;
    hGroupCount++;
    LOG(LOG_DEBUG, "CONFIG: Added group (%s)", szGroup);
    return hGroupCount - 1;
}

static void aim_buddy_cfgadduser(char *szUser, char *szGroup, int deny, int permit)
{
    int i;

    i = aim_buddy_cfgaddgroup(szGroup);
    hGroup[i].hChild = (TTOCUser *) realloc(hGroup[i].hChild, sizeof(TTOCUser) * (hGroup[i].cCount + 1));
    hGroup[i].hChild[hGroup[i].cCount].szUser = _strdup(szUser);
    hGroup[i].hChild[hGroup[i].cCount].deny = deny;
    hGroup[i].hChild[hGroup[i].cCount].permit = permit;
    hGroup[i].cCount++;
    if (permit)
        hMode = 3;
    else if (deny)
        hMode = 4;
    LOG(LOG_DEBUG, "CONFIG: Added user (%s)", szUser);
}

static void toc_buddy_cfgfree()
{
    int i, j;

    for (i = 0; i < hGroupCount; i++) {
        LOG(LOG_DEBUG, "CONFIG: Freeing group (%s)", hGroup[i].szGroup);
        free(hGroup[i].szGroup);
        for (j = 0; j < hGroup[i].cCount; j++) {
            LOG(LOG_DEBUG, "CONFIG: Freeing user (%s)", hGroup[i].hChild[j].szUser);
            free(hGroup[i].hChild[j].szUser);
        }
    }
    free(hGroup);
    hGroup = NULL;
    hGroupCount = 0;
    hMode = 1;
}

static void aim_buddy_cfgbuild(char *buf, int len)
{
    int pos = 0, i, j;
    int smode = DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_SM, 1);

    LOG(LOG_DEBUG, "aim_buddy_cfgbuild: Loading config data into toc_set_config");
    pos += _snprintf(&buf[pos], len - pos, "m %d\n", smode > 1 ? smode : hMode);
    for (i = 0; i < hGroupCount && len > pos; i++) {
        pos += _snprintf(&buf[pos], len - pos, "g %s\n", hGroup[i].szGroup);
        for (j = 0; j < hGroup[i].cCount && len > pos; j++) {
            pos += _snprintf(&buf[pos], len - pos, "b %s\n", hGroup[i].hChild[j].szUser);
        }
    }
    for (i = 0; i < hGroupCount && len > pos; i++) {
        for (j = 0; j < hGroup[i].cCount && len > pos; j++) {
            if (hGroup[i].hChild[j].deny)
                pos += _snprintf(&buf[pos], len - pos, "d %s\n", hGroup[i].hChild[j].szUser);
        }
    }
    for (i = 0; i < hGroupCount && len > pos; i++) {
        for (j = 0; j < hGroup[i].cCount && len > pos; j++) {
            if (hGroup[i].hChild[j].permit)
                pos += _snprintf(&buf[pos], len - pos, "p %s\n", hGroup[i].hChild[j].szUser);
        }
    }
}

int aim_buddy_cfglen()
{
    int len = 0, i, j;

    {
        HANDLE hContact;
        char *szProto;
        DBVARIANT dbv;
        DBVARIANT dbvg;
        int deny;
        int permit;

        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
        while (hContact) {
            szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
            if (szProto != NULL && !strcmp(szProto, AIM_PROTO)) {
                ZeroMemory(&dbv, sizeof(dbv));
                if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)
                    && !DBGetContactSettingByte(hContact, AIM_PROTO, AIM_KEY_DU, 0)     // User is being deleted, don't add
                    && !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0)
                    && !DBGetContactSettingByte(hContact, "CList", "Hidden", 0)
                    && !aim_buddy_delaydeletecheck(dbv.pszVal)) {
                    deny = 0;
                    permit = 0;
                    if (ID_STATUS_OFFLINE == DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, 0))
                        deny = 1;
                    if (ID_STATUS_ONLINE == DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, 0))
                        permit = 1;
                    if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_SG, &dbvg)) {
                        aim_buddy_cfgadduser(dbv.pszVal, dbvg.pszVal, deny, permit);
                        DBFreeVariant(&dbvg);
                    }
                    else {
                        // Add the user to the default group
                        aim_buddy_cfgadduser(dbv.pszVal, AIM_SERVERSIDE_GROUP, deny, permit);
                    }
                }
                if (dbv.pszVal)
                    DBFreeVariant(&dbv);
            }
            hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
        }
    }
    len += 4;                   // "m %d\n"
    for (i = 0; i < hGroupCount; i++) {
        len += 3 + strlen(hGroup[i].szGroup);   // "g %s\n"
        for (j = 0; j < hGroup[i].cCount; j++) {
            len += 3 + strlen(hGroup[i].hChild[j].szUser);      // "b %s\n"
        }
    }
    for (i = 0; i < hGroupCount; i++) {
        for (j = 0; j < hGroup[i].cCount; j++) {
            if (hGroup[i].hChild[j].deny)
                len += 3 + strlen(hGroup[i].hChild[j].szUser);  // "d %s\n"
        }
    }
    for (i = 0; i < hGroupCount; i++) {
        for (j = 0; j < hGroup[i].cCount; j++) {
            if (hGroup[i].hChild[j].permit)
                len += 3 + strlen(hGroup[i].hChild[j].szUser);  // "p %s\n"
        }
    }
    toc_buddy_cfgfree();
    return len + strlen("toc_set_config \\{\\}");
}

void aim_buddy_setconfig()
{
    // Check to see if we are using ss lists (if not just skip update)
    if (hServerSideList && aim_buddy_cfglen() > MSG_LEN) {
        hServerSideList = 0;
        LOG(LOG_WARN, "List too big, turning off server-side list support");
    }
    if (!hServerSideList)
        return;
    hSetConfig = 1;
    LOG(LOG_DEBUG, "aim_buddy_setconfig: Building list configuration");
    // Retrieve contacts/groups
    {
        HANDLE hContact;
        char *szProto;
        DBVARIANT dbv;
        DBVARIANT dbvg;
        int deny;
        int permit;

        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
        while (hContact) {
            szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
            if (szProto != NULL && !strcmp(szProto, AIM_PROTO)) {
                ZeroMemory(&dbv, sizeof(dbv));
                if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)
                    && !DBGetContactSettingByte(hContact, AIM_PROTO, AIM_KEY_DU, 0)     // User is being deleted, don't add
                    && !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0)
                    && !DBGetContactSettingByte(hContact, "CList", "Hidden", 0)
                    && !aim_buddy_delaydeletecheck(dbv.pszVal)) {
                    deny = 0;
                    permit = 0;
                    if (ID_STATUS_OFFLINE == DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, 0))
                        deny = 1;
                    if (ID_STATUS_ONLINE == DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, 0))
                        permit = 1;
                    if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_SG, &dbvg)) {
                        aim_buddy_cfgadduser(dbv.pszVal, dbvg.pszVal, deny, permit);
                        DBFreeVariant(&dbvg);
                    }
                    else {
                        // Add the user to the default group
                        aim_buddy_cfgadduser(dbv.pszVal, AIM_SERVERSIDE_GROUP, deny, permit);
                    }
                }
                if (dbv.pszVal)
                    DBFreeVariant(&dbv);
            }
            hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
        }
    }

    // build list
    {
        char buf[MSG_LEN], snd[MSG_LEN * 2];

        aim_buddy_cfgbuild(buf, sizeof(buf) - strlen("toc_set_config \\{\\}"));
        _snprintf(snd, MSG_LEN, "toc_set_config {%s}", buf);
        aim_toc_sflapsend(snd, -1, TYPE_DATA);
    }
    toc_buddy_cfgfree();
    aim_buddy_delaydeletefree();
}
