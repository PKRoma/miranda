/*
SRMM

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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

#include "commonheaders.h"

int LoadSendRecvMessageModule(void);
int SplitmsgShutdown(void);

PLUGINLINK *pluginLink;
HINSTANCE g_hInst;

PLUGININFO pluginInfo = {
    sizeof(PLUGININFO),
    "Send/Receive Messages",
    PLUGIN_MAKE_VERSION(2, 0, 0, 0),
    "Send and receive instant messages",
    "Miranda IM Development Team",
    "info@miranda-im.org",
    "Copyright © 2000-2005 Miranda ICQ/IM Project",
    "http://www.miranda-im.org",
    0,
    DEFMOD_SRMESSAGE            // replace internal version (if any)
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    g_hInst = hinstDLL;
    return TRUE;
}

__declspec(dllexport)
     PLUGININFO *MirandaPluginInfo(DWORD mirandaVersion)
{
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 3, 3, 0))
        return NULL;
    return &pluginInfo;
}

#define UPDATE_SETTINGS_FROM_OLD_SRMM_MODULE
#ifdef UPDATE_SETTINGS_FROM_OLD_SRMM_MODULE
#define SRMMMOD_OLD "SRMsg"
#define SRMMMOD_SETTINGS_NAM "Settings"
#define SRMMMOD_SETTINGS_VER 4
static int old_db_setting_exists(char *s, int type) {
	DBVARIANT dbv;
	DBCONTACTGETSETTING cgs;

	cgs.szModule=SRMMMOD_OLD;
	cgs.szSetting=s;
	cgs.pValue=&dbv;
    dbv.type=type;
    if(CallService(MS_DB_CONTACT_GETSETTINGSTATIC,(WPARAM)NULL,(LPARAM)&cgs))
        return 0;
    return 1;
}
static void convert_old_byte_setting(char *s) {
    if (!s) return;
    if (old_db_setting_exists(s, DBVT_BYTE)) {
        db_byte_set(NULL, SRMMMOD, s, db_byte_get(NULL, SRMMMOD_OLD, s, 0));
        db_unset(NULL, SRMMMOD_OLD, s);

    }
}
static void convert_old_word_setting(char *s) {
    if (!s) return;
    if (old_db_setting_exists(s, DBVT_WORD)) {
        db_word_set(NULL, SRMMMOD, s, db_word_get(NULL, SRMMMOD_OLD, s, 0));
        db_unset(NULL, SRMMMOD_OLD, s);

    }
}
#endif
int __declspec(dllexport) Load(PLUGINLINK * link)
{
    pluginLink = link;
    #ifdef UPDATE_SETTINGS_FROM_OLD_SRMM_MODULE
    if (db_word_get(NULL, SRMMMOD, SRMMMOD_SETTINGS_NAM, 0)<SRMMMOD_SETTINGS_VER) {
        convert_old_byte_setting(SRMSGSET_SHOWBUTTONLINE);
        convert_old_byte_setting(SRMSGSET_SHOWINFOLINE);
        convert_old_byte_setting(SRMSGSET_AUTOPOPUP);
        convert_old_byte_setting(SRMSGSET_AUTOMIN);
        convert_old_byte_setting(SRMSGSET_AUTOCLOSE);
        convert_old_byte_setting(SRMSGSET_SAVEPERCONTACT);
        convert_old_byte_setting(SRMSGSET_CASCADE);
        convert_old_byte_setting(SRMSGSET_SENDONENTER);
        convert_old_byte_setting(SRMSGSET_SENDONDBLENTER);
        convert_old_byte_setting(SRMSGSET_STATUSICON);
        convert_old_byte_setting(SRMSGSET_SENDBUTTON);
        convert_old_byte_setting(SRMSGSET_CHARCOUNT);
        convert_old_byte_setting(SRMSGSET_LOADHISTORY);
        convert_old_byte_setting(SRMSGSET_HIDENAMES);
        convert_old_byte_setting(SRMSGSET_SHOWTIME);
        convert_old_byte_setting(SRMSGSET_SHOWDATE);
        convert_old_byte_setting(SRMSGSET_TYPING);
        convert_old_byte_setting(SRMSGSET_TYPINGNEW);
        convert_old_byte_setting(SRMSGSET_TYPINGUNKNOWN);
        convert_old_byte_setting(SRMSGSET_SHOWTYPING);
        convert_old_byte_setting(SRMSGSET_SHOWTYPINGWIN);
        convert_old_byte_setting(SRMSGSET_SHOWTYPINGNOWIN);
        convert_old_byte_setting(SRMSGSET_SHOWTYPINGCLIST);
        convert_old_word_setting(SRMSGSET_LOADCOUNT);
        convert_old_word_setting(SRMSGSET_LOADTIME);
        db_unset(NULL, SRMMMOD_OLD, "splitheight");
        db_unset(NULL, SRMMMOD_OLD, "splitwidth");
        db_unset(NULL, SRMMMOD_OLD, "splitx");
        db_unset(NULL, SRMMMOD_OLD, "splity");
        db_unset(NULL, SRMMMOD_OLD, "splitsplity");
        db_unset(NULL, SRMMMOD_OLD, "ShowFiles");
        db_unset(NULL, SRMMMOD_OLD, "ShowURLs");
        db_unset(NULL, SRMMMOD_OLD, "Split");
        db_unset(NULL, SRMMMOD_OLD, "multisplit");
        db_unset(NULL, SRMMMOD_OLD, "CloseOnReply");
        db_unset(NULL, SRMMMOD_OLD, "ShowLogIcons");
        db_unset(NULL, SRMMMOD_OLD, "MessageTimeout");
        db_dword_set(NULL, SRMMMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);
        db_word_set(NULL, SRMMMOD, SRMMMOD_SETTINGS_NAM, SRMMMOD_SETTINGS_VER);
    }
    #endif
    return LoadSendRecvMessageModule();
}

int __declspec(dllexport) Unload(void)
{
    return SplitmsgShutdown();
}
