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
void aim_remove_all_buddies()
{
	HANDLE hContact;
    char *szProto;
	if(IDOK==MessageBox(0,Translate("Continuing with this operation will disconnect you from aim, remove all of your miranda buddies, their histories, and any settings you have for them.\r\n(Note: When reconnecting to miranda-aim any buddy on your server-side list will reappear.)\r\nClick \"Ok\" to continue"),Translate("Warning"), MB_OKCANCEL | MB_ICONEXCLAMATION))
	{
        aim_toc_disconnect();
        aim_util_broadcaststatus(ID_STATUS_OFFLINE);
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact)
		{
			szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
			if (szProto != NULL && !strcmp(szProto, AIM_PROTO))
			{
				if (DBGetContactSettingByte(hContact, AIM_PROTO, AIM_CHAT, 0) == 0)
				{
					HANDLE tempContact=hContact;
					hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
					CallService(MS_DB_CONTACT_DELETE, (WPARAM) tempContact, 0);
				}
				else
				{
					hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
				}
			}
			else
			{
				hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
			}
		}
	}
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
    char *sn;
    pthread_mutex_lock(&buddyMutex);
	sn = aim_util_normalize(nick);
    if (!strncmp(sn, "pleaseupgrade", strlen("pleaseupgrade"))) {
        pthread_mutex_unlock(&buddyMutex);
        return NULL;
    }
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
        szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
        if (szProto != NULL && !strcmp(szProto, AIM_PROTO)) {
            if (DBGetContactSettingByte(hContact, AIM_PROTO, AIM_CHAT, 0) == 0) {
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
	DBWriteContactSettingString(hContact,AIM_PROTO,"Group","Miranda Merged");
    LOG(LOG_INFO, "Added AIM Contact: %s", nick);
    if (!inlist) {
        DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
        DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
    }
    DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_UN, sn);
    DBWriteContactSettingString(hContact, AIM_PROTO, AIM_KEY_NK, sn);
    DBWriteContactSettingWord(hContact, AIM_PROTO, AIM_KEY_ST, ID_STATUS_OFFLINE);
    if (aim_util_isonline() && !noadd) {
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
                mir_snprintf(buf, sizeof(buf), "toc_add_deny %s", dbv.pszVal);
            }
            else {
                mir_snprintf(buf, sizeof(buf), "toc_add_permit %s", dbv.pszVal);
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
            if (DBGetContactSettingByte(hContact, AIM_PROTO, AIM_CHAT, 0) == 0) {
                DBWriteContactSettingWord(hContact, AIM_PROTO, AIM_KEY_ST, ID_STATUS_OFFLINE);
                DBWriteContactSettingDword(hContact, AIM_PROTO, AIM_KEY_OT, 0); // reset online time
                DBWriteContactSettingDword(hContact, AIM_PROTO, AIM_KEY_IT, 0); // reset idle time
                DBWriteContactSettingByte(hContact, AIM_PROTO, AIM_KEY_LL, 0);
            }
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
    aimStatus = ID_STATUS_OFFLINE;
}

void aim_buddy_delete(HANDLE hContact)
{
    char *szProto;
	DBVARIANT dbv;
	char mbuf[MSG_LEN * 2];
    if (!hContact)
        return;

    szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto && !strcmp(szProto, AIM_PROTO)) {
	DBGetContactSetting(hContact,AIM_PROTO, AIM_KEY_UN, &dbv);
	if(dbv.pszVal&&_strcmpi(dbv.pszVal,"temp"))
	{
		char sn[32];
		mir_snprintf(sn, sizeof(sn),dbv.pszVal);
		DBFreeVariant(&dbv);
		DBGetContactSetting(hContact,AIM_PROTO, "Group", &dbv);
		if(dbv.pszVal)
		{
			mir_snprintf(mbuf, sizeof(mbuf),"toc2_remove_buddy %s \"%s\"",sn,dbv.pszVal);
			aim_toc_sflapsend(mbuf, -1, TYPE_DATA);
			DBFreeVariant(&dbv);
		}
		else
		{
			char buf[256];
			mir_snprintf(buf, sizeof(buf), Translate("Buddy does not exist on server!"));
			if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_SE, AIM_KEY_SE_DEF)) 
			{
				if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) 
				{
					aim_util_shownotification(Translate("Oops!"), buf, NIIF_ERROR);
				}
				else
				{
					MessageBox(0, buf, Translate("Oops!"), MB_OK | MB_ICONERROR);
				}
			}
		}
	}
    DBWriteContactSettingByte(hContact, AIM_PROTO, AIM_KEY_DU, 1);
        if (!hServerSideList)
            return;
        if (
			DBGetContactSettingByte(hContact, "CList", "Delete", 0)) {
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

    if (type & USER_WIRELESS)
        newstatus = ID_STATUS_ONTHEPHONE;

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
            if (DBGetContactSettingByte(hContact, AIM_PROTO, AIM_CHAT, 0) == 0) {
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
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
}

void aim_buddy_parseconfig(char *config)
{
	char *c="\0", group[256];
	wchar_t* wgroup;
    HANDLE hContact;
	HANDLE tempContact;
	tempContact = aim_buddy_get("temp", 1, 1, 1, 0);
    if (!config)
        return;
        LOG(LOG_DEBUG, "Parsing configuation from server");
        group[0] = '\0';
		c = strtok(config, "\n");
		while(c[0]!='g' || c[1]!=':')
		{
			c = strtok(NULL, "\n");
			if(c==NULL)
				break;
		}
		if (config != NULL) {
			do {
				
                if (c == NULL)
                    break;
				if(c[0]=='d'&&c[1]=='o'&&c[2]=='n'&&c[3]=='e'&&c[4]==':')
				{
					break;
				}
				else if (*c == 'g') {
					c[strlen(c)]='\0';
                    mir_snprintf(group, sizeof(group), c + 2);
					wgroup =make_unicode_string(group);
                    LOG(LOG_DEBUG, "Parsed group from server config: (%s)", wgroup);
					
                }
                else if (*c == 'b') {
                    char nm[80], *a;
                    if ((a = strchr(c + 2, ':')) != NULL)
                        *a++ = '\0';
                    mir_snprintf(nm, sizeof(nm), c + 2);

                    if (!aim_buddy_delaydeletecheck(nm)) {
                        LOG(LOG_DEBUG, "Parsed buddy from server config (%s) in %s", nm, wgroup);
						hContact = aim_buddy_get(nm, 0, 0, 0, 0);
						if(!hContact)
						{
							hContact = aim_buddy_get(nm, 1, 1, 1, group);
							if (hContact)
							{
								int i=0;
								BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
								DBWriteContactSettingByte(hContact, AIM_PROTO, AIM_KEY_LL, 1);
								if(bUtfReadyDB==1)
									DBWriteContactSettingStringUtf(hContact,AIM_PROTO,"Group",group);
								else
									DBWriteContactSettingString(hContact,AIM_PROTO,"Group",group);
							}
                        }
						else
						{	
							DBVARIANT dbv;
							if(DBGetContactSetting(tempContact, AIM_PROTO, nm, &dbv))
							{
								BOOL bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
								if(bUtfReadyDB==1)
									DBWriteContactSettingStringUtf(tempContact,AIM_PROTO,nm,group);
								else
									DBWriteContactSettingString(tempContact,AIM_PROTO,nm,group);
							}
							else if(_strcmpi(dbv.pszVal,group)!=0)
							{
								char mbuf[MSG_LEN];
								mir_snprintf(mbuf, sizeof(mbuf),"toc2_remove_buddy %s \"%s\"",nm,group);
								aim_toc_sflapsend(mbuf, -1, TYPE_DATA);
							}
							if(dbv.pszVal)
								DBFreeVariant(&dbv);
						}
                    }
                }
                else if (*c == 'd') {
                    char nm[80];
                    mir_snprintf(nm, sizeof(nm), c + 2);
                    LOG(LOG_DEBUG, "Parsed deny mode from server config (%s)", nm);
                    hContact = aim_buddy_get(nm, 1, 0, 0, group);
                    if (hContact) {
                        LOG(LOG_DEBUG, "Updating deny mode for contact (%s)", nm);
                        DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, ID_STATUS_OFFLINE);
                    }

                }
                else if (*c == 'p') {
                    char nm[80];
                    mir_snprintf(nm, sizeof(nm), c + 2);
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
			} while ((c=strtok(NULL, "\n")));
        }
        aim_buddy_updateconfig(1);
		CallService(MS_DB_CONTACT_DELETE, (WPARAM) tempContact, 0);
}

void aim_buddy_updateconfig(int ssilist)
{
    HANDLE hContact;
    char *szProto;
    DBVARIANT dbv;
    int n=0;
    int m;
    char buf[MSG_LEN * 2];
    char dbuf[MSG_LEN * 2];
    if (!aim_util_isonline())
        return;
//	n = mir_snprintf(buf, sizeof(buf), "toc2_add_buddy");
    m = mir_snprintf(dbuf, sizeof(dbuf), "toc_add_deny");

    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
        szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
        if (szProto != NULL && !strcmp(szProto, AIM_PROTO)) {
            if (DBGetContactSettingByte(hContact, AIM_PROTO, AIM_CHAT, 0) == 0) {
                ZeroMemory(&dbv, sizeof(dbv));
                if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)
                    && !DBGetContactSettingByte(hContact, AIM_PROTO, AIM_KEY_DU, 0)     // User is being deleted, don't add
                    && !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0)
                    && !aim_buddy_delaydeletecheck(dbv.pszVal)) {
                    if (!ssilist || !DBGetContactSettingByte(hContact, AIM_PROTO, AIM_KEY_LL, 0)) {
                        if (strlen(dbv.pszVal) + n + 32 > MSG_LEN) {
                            aim_toc_sflapsend(buf, -1, TYPE_DATA);
                        }
                        if (ID_STATUS_OFFLINE == DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, 0)) {
                            LOG(LOG_DEBUG, "Setting deny mode for %s", dbv.pszVal);
                            if (strlen(dbv.pszVal) + m + 32 > MSG_LEN) {
                                aim_toc_sflapsend(dbuf, -1, TYPE_DATA);
                                m = mir_snprintf(dbuf, sizeof(dbuf), "toc_add_deny");
                            }
                            m += mir_snprintf(dbuf + m, sizeof(dbuf) - m, " %s", dbv.pszVal);
                        }
						//n += mir_snprintf(buf + n, sizeof(buf) - n, "toc_get_status %s",dbv.pszVal);
						//aim_toc_sflapsend(buf, -1, TYPE_DATA);
                    }
                }
                if (dbv.pszVal)
                    DBFreeVariant(&dbv);
            }
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
	//if (n > lstrlen("toc2_add_buddy"))
      //  aim_toc_sflapsend(buf, -1, TYPE_DATA);
    if (m > lstrlen("toc_add_deny"))
        aim_toc_sflapsend(dbuf, -1, TYPE_DATA);
}
