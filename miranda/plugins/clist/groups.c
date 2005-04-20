/*

Miranda IM: the free IM client for Microsoft* Windows*

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

static int RenameGroup(WPARAM wParam, LPARAM lParam);
static int MoveGroupBefore(WPARAM wParam, LPARAM lParam);

static int CountGroups(void)
{
    DBVARIANT dbv;
    int i;
    char str[33];

    for (i = 0;; i++) {
        itoa(i, str, 10);
        if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
            break;
        DBFreeVariant(&dbv);
    }
    return i;
}

static int GroupNameExists(const char *name, int skipGroup)
{
    char idstr[33];
    DBVARIANT dbv;
    int i;

    for (i = 0;; i++) {
        if (i == skipGroup)
            continue;
        itoa(i, idstr, 10);
        if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv))
            break;
        if (!strcmp(dbv.pszVal + 1, name)) {
            DBFreeVariant(&dbv);
            return 1;
        }
        DBFreeVariant(&dbv);
    }
    return 0;
}

static int CreateGroup(WPARAM wParam, LPARAM lParam)
{
    int newId = CountGroups();
    char str[33], newBaseName[127], newName[128];
    int i;
    DBVARIANT dbv;

    if (wParam) {
        itoa(wParam - 1, str, 10);
        if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
            return 0;
        mir_snprintf(newBaseName, sizeof(newBaseName), "%s\\%s", dbv.pszVal + 1, Translate("New Group"));
        mir_free(dbv.pszVal);
    }
    else
        lstrcpyn(newBaseName, Translate("New Group"), sizeof(newBaseName));
    itoa(newId, str, 10);
    i = 1;
    lstrcpyn(newName + 1, newBaseName, sizeof(newName) - 1);
    while (GroupNameExists(newName + 1, -1))
        mir_snprintf(newName + 1, sizeof(newName) - 1, "%s (%d)", newBaseName, ++i);
    newName[0] = 1 | GROUPF_EXPANDED;   //1 is required so we never get '\0'
    DBWriteContactSettingString(NULL, "CListGroups", str, newName);
    CallService(MS_CLUI_GROUPADDED, newId + 1, 1);
    return newId + 1;
}

static int GetGroupName2(WPARAM wParam, LPARAM lParam)
{
    char idstr[33];
    DBVARIANT dbv;
    static char name[128];

    itoa(wParam - 1, idstr, 10);
    if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv))
        return (int) (char *) NULL;
    lstrcpyn(name, dbv.pszVal + 1, sizeof(name));
    if ((DWORD *) lParam != NULL)
        *(DWORD *) lParam = dbv.pszVal[0];
    DBFreeVariant(&dbv);
    return (int) name;
}

static int GetGroupName(WPARAM wParam, LPARAM lParam)
{
    int ret;
    ret = GetGroupName2(wParam, lParam);
    if ((int *) lParam)
        *(int *) lParam = 0 != (*(int *) lParam & GROUPF_EXPANDED);
    return ret;
}

static int DeleteGroup(WPARAM wParam, LPARAM lParam)
{
    int i;
    char str[33], name[256], szNewParent[256], *pszLastBackslash;
    DBVARIANT dbv;
    HANDLE hContact;

    //get the name
    itoa(wParam - 1, str, 10);
    if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
        return 1;
    lstrcpyn(name, dbv.pszVal + 1, sizeof(name));
    DBFreeVariant(&dbv);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    //must remove setting from all child contacts too
    //children are demoted to the next group up, not deleted.
    lstrcpy(szNewParent, name);
    pszLastBackslash = strrchr(szNewParent, '\\');
    if (pszLastBackslash)
        pszLastBackslash[0] = '\0';
    else
        szNewParent[0] = '\0';
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    do {
        if (DBGetContactSetting(hContact, "CList", "Group", &dbv))
            continue;
        if (strcmp(dbv.pszVal, name)) {
            DBFreeVariant(&dbv);
            continue;
        }
        DBFreeVariant(&dbv);
        if (szNewParent[0])
            DBWriteContactSettingString(hContact, "CList", "Group", szNewParent);
        else
            DBDeleteContactSetting(hContact, "CList", "Group");
    } while ((hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0)) != NULL);
    //shuffle list of groups up to fill gap
    for (i = wParam - 1;; i++) {
        itoa(i + 1, str, 10);
        if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
            break;
        itoa(i, str, 10);
        DBWriteContactSettingString(NULL, "CListGroups", str, dbv.pszVal);
        DBFreeVariant(&dbv);
    }
    itoa(i, str, 10);
    DBDeleteContactSetting(NULL, "CListGroups", str);
    //rename subgroups
    {
        char szNewName[256];
        int len;

        len = lstrlen(name);
        for (i = 0;; i++) {
            itoa(i, str, 10);
            if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
                break;
            if (!strncmp(dbv.pszVal + 1, name, len) && dbv.pszVal[len + 1] == '\\' && strchr(dbv.pszVal + len + 2, '\\') == NULL) {
                if (szNewParent[0])
                    mir_snprintf(szNewName, sizeof(szNewName), "%s\\%s", szNewParent, dbv.pszVal + len + 2);
                else
                    lstrcpyn(szNewName, dbv.pszVal + len + 2, sizeof(szNewName));
                RenameGroup(i + 1, (LPARAM) szNewName);
            }
            DBFreeVariant(&dbv);
        }
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    LoadContactTree();
    return 0;
}

static int RenameGroupWithMove(int groupId, const char *szName, int move)
{
    char str[256], idstr[33], oldName[256];
    DBVARIANT dbv;
    HANDLE hContact;

    if (GroupNameExists(szName, groupId)) {
        MessageBox(NULL, Translate("You already have a group with that name. Please enter a unique name for the group."), Translate("Rename Group"),
                   MB_OK);
        return 1;
    }

    //do the change
    itoa(groupId, idstr, 10);
    if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv))
        return 1;
    str[0] = dbv.pszVal[0];
    lstrcpyn(oldName, dbv.pszVal + 1, sizeof(oldName));
    DBFreeVariant(&dbv);
    lstrcpyn(str + 1, szName, sizeof(str) - 1);
    DBWriteContactSettingString(NULL, "CListGroups", idstr, str);

    //must rename setting in all child contacts too
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    do {
        if (DBGetContactSetting(hContact, "CList", "Group", &dbv))
            continue;
        if (strcmp(dbv.pszVal, oldName))
            continue;
        DBWriteContactSettingString(hContact, "CList", "Group", szName);
    } while ((hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0)) != NULL);

    //rename subgroups
    {
        char szNewName[256];
        int len, i;

        len = lstrlen(oldName);
        for (i = 0;; i++) {
            if (i == groupId)
                continue;
            itoa(i, idstr, 10);
            if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv))
                break;
            if (!strncmp(dbv.pszVal + 1, oldName, len) && dbv.pszVal[len + 1] == '\\' && strchr(dbv.pszVal + len + 2, '\\') == NULL) {
                mir_snprintf(szNewName, sizeof(szNewName), "%s\\%s", szName, dbv.pszVal + len + 2);
                RenameGroupWithMove(i, szNewName, 0);   //luckily, child groups will never need reordering
            }
            DBFreeVariant(&dbv);
        }
    }

    //finally must make sure it's after any parent items
    if (move) {
        char *pszLastBackslash;
        int i;

        lstrcpyn(str, szName, sizeof(str));
        pszLastBackslash = strrchr(str, '\\');
        if (pszLastBackslash == NULL)
            return 0;
        *pszLastBackslash = '\0';
        for (i = 0;; i++) {
            itoa(i, idstr, 10);
            if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv))
                break;
            if (!lstrcmp(dbv.pszVal + 1, str)) {
                if (i < groupId)
                    break;      //is OK
                MoveGroupBefore(groupId + 1, i + 2);
                break;
            }
            DBFreeVariant(&dbv);
        }
    }
    return 0;
}

static int RenameGroup(WPARAM wParam, LPARAM lParam)
{
    return -1 != RenameGroupWithMove(wParam - 1, (char *) lParam, 1);
}

static int SetGroupExpandedState(WPARAM wParam, LPARAM lParam)
{
    char idstr[33];
    DBVARIANT dbv;

    itoa(wParam - 1, idstr, 10);
    if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv))
        return 1;
    if (lParam)
        dbv.pszVal[0] |= GROUPF_EXPANDED;
    else
        dbv.pszVal[0] = dbv.pszVal[0] & ~GROUPF_EXPANDED;
    DBWriteContactSettingString(NULL, "CListGroups", idstr, dbv.pszVal);
    DBFreeVariant(&dbv);
    return 0;
}

static int SetGroupFlags(WPARAM wParam, LPARAM lParam)
{
    char idstr[33];
    DBVARIANT dbv;
    int flags, oldval, newval;

    itoa(wParam - 1, idstr, 10);
    if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv))
        return 1;
    flags = LOWORD(lParam) & HIWORD(lParam);
    oldval = dbv.pszVal[0];
    newval = dbv.pszVal[0] = (dbv.pszVal[0] & ~HIWORD(lParam)) | flags;
    DBWriteContactSettingString(NULL, "CListGroups", idstr, dbv.pszVal);
    DBFreeVariant(&dbv);
    if ((oldval & GROUPF_HIDEOFFLINE) != (newval & GROUPF_HIDEOFFLINE))
        LoadContactTree();
    return 0;
}

static int MoveGroupBefore(WPARAM wParam, LPARAM lParam)
{
    int i, shuffleFrom, shuffleTo, shuffleDir;
    char str[33];
    char *szMoveName;
    DBVARIANT dbv;

    if (wParam == 0 || (LPARAM) wParam == lParam)
        return 0;
    itoa(wParam - 1, str, 10);
    if (DBGetContactSetting(NULL, "CListGroups", str, &dbv))
        return 0;
    szMoveName = dbv.pszVal;
    //shuffle list of groups up to fill gap
    if (lParam == 0) {
        shuffleFrom = wParam - 1;
        shuffleTo = -1;
        shuffleDir = -1;
    }
    else {
        if ((LPARAM) wParam < lParam) {
            shuffleFrom = wParam - 1;
            shuffleTo = lParam - 2;
            shuffleDir = -1;
        }
        else {
            shuffleFrom = wParam - 1;
            shuffleTo = lParam - 1;
            shuffleDir = 1;
        }
    }
    if (shuffleDir == -1) {
        for (i = shuffleFrom; i != shuffleTo; i++) {
            itoa(i + 1, str, 10);
            if (DBGetContactSetting(NULL, "CListGroups", str, &dbv)) {
                shuffleTo = i;
                break;
            }
            itoa(i, str, 10);
            DBWriteContactSettingString(NULL, "CListGroups", str, dbv.pszVal);
            DBFreeVariant(&dbv);
        }
    }
    else {
        for (i = shuffleFrom; i != shuffleTo; i--) {
            itoa(i - 1, str, 10);
            if (DBGetContactSetting(NULL, "CListGroups", str, &dbv)) {
                mir_free(szMoveName);
                return 1;
            }                   //never happens
            itoa(i, str, 10);
            DBWriteContactSettingString(NULL, "CListGroups", str, dbv.pszVal);
            DBFreeVariant(&dbv);
        }
    }
    itoa(shuffleTo, str, 10);
    DBWriteContactSettingString(NULL, "CListGroups", str, szMoveName);
    mir_free(szMoveName);
    return shuffleTo + 1;
}

static int BuildGroupMenu(WPARAM wParam, LPARAM lParam)
{
    char idstr[33];
    DBVARIANT dbv;
    int groupId;
    HMENU hRootMenu, hThisMenu;
    int nextMenuId = 100;
    char *pBackslash, *pNextField, szThisField[128], szThisMenuItem[128];
    int menuId, compareResult, menuItemCount;
    MENUITEMINFO mii = { 0 };

    if (DBGetContactSetting(NULL, "CListGroups", "0", &dbv))
        return (int) (HMENU) NULL;
    DBFreeVariant(&dbv);
    hRootMenu = CreateMenu();
    for (groupId = 0;; groupId++) {
        itoa(groupId, idstr, 10);
        if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv))
            break;

        pNextField = dbv.pszVal + 1;
        hThisMenu = hRootMenu;
        mii.cbSize = MENUITEMINFO_V4_SIZE;
        do {
            pBackslash = strchr(pNextField, '\\');
            if (pBackslash == NULL) {
                lstrcpyn(szThisField, pNextField, sizeof(szThisField));
                pNextField = NULL;
            }
            else {
                lstrcpyn(szThisField, pNextField, min(sizeof(szThisField), pBackslash - pNextField + 1));
                pNextField = pBackslash + 1;
            }
            compareResult = 1;
            menuItemCount = GetMenuItemCount(hThisMenu);
            for (menuId = 0; menuId < menuItemCount; menuId++) {
                mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_DATA;
                mii.cch = sizeof(szThisMenuItem);
                mii.dwTypeData = szThisMenuItem;
                GetMenuItemInfo(hThisMenu, menuId, TRUE, &mii);
                compareResult = lstrcmp(szThisField, szThisMenuItem);
                if (compareResult == 0) {
                    if (pNextField == NULL) {
                        mii.fMask = MIIM_DATA;
                        mii.dwItemData = groupId + 1;
                        SetMenuItemInfo(hThisMenu, menuId, TRUE, &mii);
                    }
                    else {
                        if (mii.hSubMenu == NULL) {
                            mii.fMask = MIIM_SUBMENU;
                            mii.hSubMenu = CreateMenu();
                            SetMenuItemInfo(hThisMenu, menuId, TRUE, &mii);
                            mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID;
                            //dwItemData doesn't change
                            mii.fType = MFT_STRING;
                            mii.dwTypeData = Translate("This group");
                            mii.wID = nextMenuId++;
                            InsertMenuItem(mii.hSubMenu, 0, TRUE, &mii);
                            mii.fMask = MIIM_TYPE;
                            mii.fType = MFT_SEPARATOR;
                            InsertMenuItem(mii.hSubMenu, 1, TRUE, &mii);
                        }
                        hThisMenu = mii.hSubMenu;
                    }
                    break;
                }
                if ((int) mii.dwItemData - 1 > groupId)
                    break;
            }
            if (compareResult) {
                mii.fMask = MIIM_TYPE | MIIM_ID;
                mii.wID = nextMenuId++;
                mii.dwTypeData = szThisField;
                mii.fType = MFT_STRING;
                if (pNextField) {
                    mii.fMask |= MIIM_SUBMENU;
                    mii.hSubMenu = CreateMenu();
                }
                else {
                    mii.fMask |= MIIM_DATA;
                    mii.dwItemData = groupId + 1;
                }
                InsertMenuItem(hThisMenu, menuId, TRUE, &mii);
                if (pNextField) {
                    hThisMenu = mii.hSubMenu;
                }
            }
        } while (pNextField);

        DBFreeVariant(&dbv);
    }
    return (int) hRootMenu;
}

int InitGroupServices(void)
{
    CreateServiceFunction(MS_CLIST_GROUPCREATE, CreateGroup);
    CreateServiceFunction(MS_CLIST_GROUPDELETE, DeleteGroup);
    CreateServiceFunction(MS_CLIST_GROUPRENAME, RenameGroup);
    CreateServiceFunction(MS_CLIST_GROUPGETNAME, GetGroupName);
    CreateServiceFunction(MS_CLIST_GROUPGETNAME2, GetGroupName2);
    CreateServiceFunction(MS_CLIST_GROUPSETEXPANDED, SetGroupExpandedState);
    CreateServiceFunction(MS_CLIST_GROUPSETFLAGS, SetGroupFlags);
    CreateServiceFunction(MS_CLIST_GROUPMOVEBEFORE, MoveGroupBefore);
    CreateServiceFunction(MS_CLIST_GROUPBUILDMENU, BuildGroupMenu);
    return 0;
}
