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

pthread_mutex_t buddyMutex;
int hServerSideList = 0;
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
    if (!strncmp(sn, "pleaseupgrade", strlen("pleaseupgrade"))) {
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
            DBWriteContactSettingByte(hContact, AIM_PROTO, AIM_KEY_LL, 0);
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
    aimStatus = ID_STATUS_OFFLINE;
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
    if (!config)
        return;
    if (hServerSideList) {
        char *c, group[256];
        HANDLE hContact;
        TList *buddies = NULL;
        
        LOG(LOG_DEBUG, "Parsing configuation from server");
        group[0] = '\0';
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
                            DBWriteContactSettingByte(hContact, AIM_PROTO, AIM_KEY_LL, 1);
                            buddies = tlist_append(buddies, _strdup(nm));
                        }
                    }
                }
                else if (*c == 'd') {
                    char nm[80];
                    _snprintf(nm, sizeof(nm), c + 2);
                    LOG(LOG_DEBUG, "Parsed deny mode from server config (%s)", nm);
                    hContact = aim_buddy_get(nm, 1, 0, 0, group);
                    if (hContact) {
                        LOG(LOG_DEBUG, "Updating deny mode for contact (%s)", nm);
                        DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, ID_STATUS_OFFLINE);
                    }

                }
                else if (*c == 'p') {
                    char nm[80];
                    _snprintf(nm, sizeof(nm), c + 2);
                    LOG(LOG_DEBUG, "Parsed permit mode from server config (%s)", nm);
                    hContact = aim_buddy_get(nm, 1, 0, 0, group);
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
        {   // Update new contacts on the server
            if (buddies) {
                TList *n = buddies;
                char *un;

                char mbuf[MSG_LEN * 2];
                int buflen = 0;

                strcpy(mbuf, "toc_add_buddy ");
                buflen = strlen(mbuf);
                while(n) {
                    un = (char*)n->data;
                    if (un&&strlen(un)&&(buflen+strlen(un)+1<MSG_LEN*2)) {
                        strcat(mbuf, aim_util_normalize(un));
                        strcat(mbuf, " ");
                        buflen = strlen(mbuf);
                    }
                    if (un) free((char*)n->data);
                    n = n->next;
                }
                if (strlen(mbuf)>strlen("toc_add_buddy ")) {
                    LOG(LOG_DEBUG, "Updating new contacts on the server");
                    aim_toc_sflapsend(mbuf, -1, TYPE_DATA);
                }
                tlist_free(buddies);
            }
        }
        aim_buddy_updateconfig(1);
    }
}

void aim_buddy_updateconfig(int ssilist)
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
                if (!ssilist||!DBGetContactSettingByte(hContact, AIM_PROTO, AIM_KEY_LL, 0)) {
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
