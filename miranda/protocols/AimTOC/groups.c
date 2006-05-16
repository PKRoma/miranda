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

// group stuff
// this is all a hack until miranda provides some group services

// only supports single level groups since aim doesn't have subgroups
void aim_group_create(char *group)
{
    int i;
    char str[50], name[256];
    DBVARIANT dbv;

    if (!group)
        return;
    for (i = 0;; i++) {
        itoa(i, str, 10);
        if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
            break;
        if (dbv.pszVal[0] != '\0' && !strcmp(dbv.pszVal + 1, group)) {
            DBFreeVariant(&dbv);
            return;
        }
        DBFreeVariant(&dbv);
    }
    name[0] = 1 | GROUPF_EXPANDED;
    strncpy(name + 1, group, sizeof(name) - 1);
    name[strlen(group) + 1] = '\0';
    DBWriteContactSettingString(NULL, "CListGroups", str, name);
    CallServiceSync(MS_CLUI_GROUPADDED, i + 1, 0);
}

void aim_group_adduser(HANDLE hContact, char *group)
{
    DBVARIANT dbv;

    if (!hContact || !group)
        return;
    if (!DBGetContactSetting(hContact, "CList", "Group", &dbv)) {
        if (!strcmp(dbv.pszVal, group)) {
            DBFreeVariant(&dbv);
            return;
        }
        DBFreeVariant(&dbv);
    }
    DBWriteContactSettingString(hContact, "CList", "Group", group);
}

// end big o hack
