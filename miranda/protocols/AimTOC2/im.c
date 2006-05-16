/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003 Robert Rainwater

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

static void __cdecl aim_im_sendacksuccess(HANDLE hContact)
{
    ProtoBroadcastAck(AIM_PROTO, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

static void __cdecl aim_im_sendackfail(HANDLE hContact)
{
    char buf[256], *contactName;

    contactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0);
    mir_snprintf(buf, sizeof(buf), Translate("%s is currently offline.  Please try again later when the user is online."),
                 contactName ? contactName : Translate("User"));
    SleepEx(1000, TRUE);
    ProtoBroadcastAck(AIM_PROTO, hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, (int) buf);
}

static DWORD lastIMTime = 0;
int aim_im_sendmessage(HANDLE hContact, char *msg, int automsg)
{
    DBVARIANT dbv;
    DWORD timenow = GetTickCount();

    // Wait 0.25 seconds between sending messages
    if ((timenow - lastIMTime) < 250) {
        LOG(LOG_DEBUG, "Sending IM's too fast.  Waiting to send next message.");
        SleepEx(500, TRUE);
    }
    if (!msg || strlen(msg) == 0) {
        pthread_create(aim_im_sendackfail, hContact);
        return (int) (HANDLE) 1;
    }
    lastIMTime = timenow;
 //   if (DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_ST, ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE && !automsg) {
   //     if (!automsg) {
	//		pthread_create(aim_im_sendackfail, hContact);
     //   return (int) (HANDLE) 1;
   // }
    if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)) {
        char buf[MSG_LEN * 2];
        char *tmp = malloc(strlen(msg) * 4 + 1);

        strcpy(tmp, msg);
        aim_util_escape(tmp);
        if (strlen(tmp) + 52 > MSG_LEN) {
            if (!automsg) {
                pthread_create(aim_im_sendacksuccess, hContact);
            }
            free(tmp);
            DBFreeVariant(&dbv);
            return (int) (HANDLE) 1;
        }
        mir_snprintf(buf, MSG_LEN - 8, "toc2_send_im %s \"%s\"%s", dbv.pszVal, tmp, automsg ? " auto" : "");
        aim_toc_sflapsend(buf, -1, TYPE_DATA);
        free(tmp);
        DBFreeVariant(&dbv);
        if (!automsg) {
            pthread_create(aim_im_sendacksuccess, hContact);
        }
        else {
            DBEVENTINFO dbei;
            char *t = (char *) malloc(strlen(Translate(AIM_STR_AR)) + 2 + strlen(msg) + 1);

            lstrcpy(t, Translate(AIM_STR_AR));
            lstrcat(t, ": ");
            lstrcat(t, msg);
            ZeroMemory(&dbei, sizeof(dbei));
            dbei.cbSize = sizeof(dbei);
            dbei.szModule = AIM_PROTO;
            dbei.timestamp = time(NULL);
            dbei.flags = DBEF_SENT;
            dbei.eventType = EVENTTYPE_MESSAGE;
            dbei.cbBlob = strlen(t) + 1;
            dbei.pBlob = (PBYTE) t;
            CallService(MS_DB_EVENT_ADD, (WPARAM) hContact, (LPARAM) & dbei);
            if (t)
                free(t);
        }
        return (int) (HANDLE) 1;
    }
    return 0;
}

void aim_im_sendautoresponse(char *nick)
{
    HANDLE hContact;
    time_t ctime = time(NULL);

    if (!DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AR, AIM_KEY_AR_DEF))
        return;
    hContact = aim_buddy_get(nick, 1, 0, 0, NULL);
    if (!hContact || (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AC, AIM_KEY_AC_DEF)
                      && (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0)
                          || ID_STATUS_OFFLINE == DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_AM, 0))))
        return;
    if (DBGetContactSettingDword(NULL, AIM_PROTO, AIM_KEY_LA, (DWORD) ctime) > DBGetContactSettingDword(hContact, AIM_PROTO, AIM_KEY_LM, ctime)) {
        if (szStatus) {
            LOG(LOG_DEBUG, "Sent autoresponse to %s: %s", nick, szStatus);
            aim_im_sendmessage(hContact, szStatus, 1);
        }
    }
}
