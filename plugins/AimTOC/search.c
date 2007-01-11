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

static void __cdecl aim_search_simplethread(void *snsearch)
{
    char *sn = aim_util_normalize((char *) snsearch);   // normalize it
    PROTOSEARCHRESULT psr;

    if (aim_util_isme(sn)) {
        ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
        return;
    }
    if (strlen(sn) > MAX_SN_LEN) {
        ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
        return;
    }
    ZeroMemory(&psr, sizeof(psr));
    psr.cbSize = sizeof(psr);
    psr.nick = sn;

    ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
    ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

void aim_search_simple(char *nick)
{
    pthread_create(aim_search_simplethread, (void *) nick);
}


static void __cdecl aim_search_genericthread(void *s)
{
    char *snd = (char *) s;

    aim_toc_sflapsend(snd, -1, TYPE_DATA);
    free(snd);
}

void aim_search_name(PROTOSEARCHBYNAME * psbn)
{
    char snd[MSG_LEN * 2], *ret;
    char nick[MSG_LEN], fname[MSG_LEN], lname[MSG_LEN];

    mir_snprintf(nick, sizeof(nick), "%s", psbn->pszNick ? psbn->pszNick : "");
    mir_snprintf(fname, sizeof(fname), "%s", psbn->pszFirstName ? psbn->pszFirstName : "");
    mir_snprintf(lname, sizeof(lname), "%s", psbn->pszLastName ? psbn->pszLastName : "");
    aim_util_escape(nick);
    aim_util_escape(fname);
    aim_util_escape(lname);
    mir_snprintf(snd, sizeof(snd), "toc_dir_search %s::%s:::::", strlen(nick) ? nick : fname, lname);
    ret = _strdup(snd);
    pthread_create(aim_search_genericthread, (void *) ret);
}

void aim_search_email(char *email)
{
    char snd[MSG_LEN * 2], *ret;

    aim_util_escape(email);
    mir_snprintf(snd, sizeof(snd), "toc_dir_search :::::::%s", email);
    ret = _strdup(snd);
    pthread_create(aim_search_genericthread, (void *) ret);
}
