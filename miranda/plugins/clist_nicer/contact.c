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

UNICODE done

*/
#include "commonheaders.h"

extern HANDLE hContactIconChangedEvent;
extern struct CluiData g_CluiData;

static int sortByStatus;
static int sortByProto;
struct {
    int status, order;
} statusModeOrder[] = {
    {ID_STATUS_OFFLINE,500}, {ID_STATUS_ONLINE,0}, {ID_STATUS_AWAY,200}, {ID_STATUS_DND,400}, {ID_STATUS_NA,450}, {ID_STATUS_OCCUPIED,100}, {ID_STATUS_FREECHAT,50}, {ID_STATUS_INVISIBLE,20}, {ID_STATUS_ONTHEPHONE,150}, {ID_STATUS_OUTTOLUNCH,425}
};

static int GetContactStatus(HANDLE hContact)
{
    char *szProto;

    szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto == NULL)
        return ID_STATUS_OFFLINE;
    return DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
}

void ChangeContactIcon(HANDLE hContact, int iIcon, int add)
{
    CallService(add ? MS_CLUI_CONTACTADDED : MS_CLUI_CONTACTSETICON, (WPARAM) hContact, iIcon);
    NotifyEventHooks(hContactIconChangedEvent, (WPARAM) hContact, iIcon);
}

static int GetStatusModeOrdering(int statusMode)
{
    int i;
    for (i = 0; i < sizeof(statusModeOrder) / sizeof(statusModeOrder[0]); i++) {
        if (statusModeOrder[i].status == statusMode)
            return statusModeOrder[i].order;
    }
    return 1000;
}

static BOOL mc_hgh_removed = FALSE;

void LoadContactTree(void)
{
    HANDLE hContact;
    int i, status, hideOffline;
    BOOL mc_disablehgh = ServiceExists(MS_MC_DISABLEHIDDENGROUP);
    DBVARIANT dbv = {0};
    
    CallService(MS_CLUI_LISTBEGINREBUILD, 0, 0);
    for (i = 1; ; i++) {
        if (pcli->pfnGetGroupName(i, NULL) == NULL)
            break;
        CallService(MS_CLUI_GROUPADDED, i, 0);
    }

    hideOffline = DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT);
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact != NULL) {
        status = GetContactStatus(hContact);
        if ((!hideOffline || status != ID_STATUS_OFFLINE) && !CLVM_GetContactHiddenStatus(hContact, NULL, NULL))
            ChangeContactIcon(hContact, IconFromStatusMode((char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0), status, hContact, NULL), 1);

        if(mc_disablehgh && !mc_hgh_removed) {
            if(!DBGetContactSetting(hContact, "CList", "Group", &dbv)) {
                if(!strcmp(dbv.pszVal, "MetaContacts Hidden Group"))
                   DBDeleteContactSetting(hContact, "CList", "Group");
                mir_free(dbv.pszVal);
            }
        }
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
    }
    mc_hgh_removed = TRUE;
    sortByStatus = DBGetContactSettingByte(NULL, "CList", "SortByStatus", SETTING_SORTBYSTATUS_DEFAULT);
    sortByProto = DBGetContactSettingByte(NULL, "CList", "SortByProto", SETTING_SORTBYPROTO_DEFAULT);
    CallService(MS_CLUI_SORTLIST, 0, 0);
    CallService(MS_CLUI_LISTENDREBUILD, 0, 0);
}

#define SAFESTRING(a) a?a:""

int CompareContacts(WPARAM wParam, LPARAM lParam)
{
    HANDLE a = (HANDLE) wParam,b = (HANDLE) lParam;
    TCHAR namea[128], *nameb;
    int statusa, statusb;
    char *szProto1, *szProto2;
    int rc;

    szProto1 = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) a, 0);
    szProto2 = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) b, 0);
    statusa = DBGetContactSettingWord((HANDLE) a, SAFESTRING(szProto1), "Status", ID_STATUS_OFFLINE);
    statusb = DBGetContactSettingWord((HANDLE) b, SAFESTRING(szProto2), "Status", ID_STATUS_OFFLINE);

    if (sortByProto) {
    /* deal with statuses, online contacts have to go above offline */
        if ((statusa == ID_STATUS_OFFLINE) != (statusb == ID_STATUS_OFFLINE)) {
            return 2 * (statusa == ID_STATUS_OFFLINE) - 1;
        }
    /* both are online, now check protocols */
        rc = strcmp(SAFESTRING(szProto1), SAFESTRING(szProto2)); /* strcmp() doesn't like NULL so feed in "" as needed */
        if (rc != 0 && (szProto1 != NULL && szProto2 != NULL))
            return rc;
    /* protocols are the same, order by display name */
    }

    if (sortByStatus) {
        int ordera, orderb;
        ordera = GetStatusModeOrdering(statusa);
        orderb = GetStatusModeOrdering(statusb);
        if (ordera != orderb)
            return ordera - orderb;
    } else {
    //one is offline: offline goes below online
        if ((statusa == ID_STATUS_OFFLINE) != (statusb == ID_STATUS_OFFLINE)) {
            return 2 * (statusa == ID_STATUS_OFFLINE) - 1;
        }
    }

    nameb = pcli->pfnGetContactDisplayName(a, 0);
    _tcsncpy(namea, nameb, safe_sizeof(namea));
    namea[safe_sizeof(namea) - 1] = 0;
    nameb = pcli->pfnGetContactDisplayName(b, 0);

    //otherwise just compare names
    return CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, namea, -1, nameb, -1) - 2;
    //return _tcsicmp(namea,nameb);
}

static int __forceinline GetProtoIndex(char * szName)
{
    DWORD i;
    char buf[11];
    char * name;
    DWORD pc;
    
    if (!szName) 
        return -1;
    
    pc=DBGetContactSettingDword(NULL,"Protocols","ProtoCount",-1);
    for (i=0; i<pc; i++) {
        _itoa(i,buf,10);
        name=DBGetString(NULL,"Protocols",buf);
        if (name) {
            if (!strcmp(name,szName)) {
                mir_free(name);
                return i;
            }
            mir_free(name);
        }
    }
    return -1;
}

int InternalCompareContacts(WPARAM wParam, LPARAM lParam)
{
    struct ClcContact *c1 = (struct ClcContact *) wParam, *c2 = (struct ClcContact *) lParam;
    HANDLE a = c1->hContact, b = c2->hContact;
    //TCHAR namea[128], *nameb;
    TCHAR *namea, *nameb;
    int statusa, statusb;
    char *szProto1, *szProto2;
    int rc;

    if (c1 == 0 || c2 == 0)
        return 0;

    szProto1 = c1->proto;
    szProto2 = c2->proto;
    //statusa = DBGetContactSettingWord((HANDLE) a, SAFESTRING(szProto1), "Status", ID_STATUS_OFFLINE);
    //statusb = DBGetContactSettingWord((HANDLE) b, SAFESTRING(szProto2), "Status", ID_STATUS_OFFLINE);
    statusa = c1->wStatus;
    statusb = c2->wStatus;

    // make sure, sticky contacts are always at the beginning of the group/list

    if ((c1->flags & CONTACTF_STICKY) != (c2->flags & CONTACTF_STICKY))
        return 2 * (c2->flags & CONTACTF_STICKY) - 1;

    if (sortByProto) {
    /* deal with statuses, online contacts have to go above offline */
        if (!g_CluiData.bDontSeparateOffline && ((statusa == ID_STATUS_OFFLINE) != (statusb == ID_STATUS_OFFLINE))) {
            return 2 * (statusa == ID_STATUS_OFFLINE) - 1;
        }
    /* both are online, now check protocols */
        if(c1->bIsMeta)
            szProto1 = c1->metaProto ? c1->metaProto : c1->proto;
        if(c2->bIsMeta)
            szProto2 = c2->metaProto ? c2->metaProto : c2->proto;

        //rc = strcmp(SAFESTRING(szProto1), SAFESTRING(szProto2)); /* strcmp() doesn't like NULL so feed in "" as needed */
        rc = GetProtoIndex(szProto1) - GetProtoIndex(szProto2);

        if (rc != 0 && (szProto1 != NULL && szProto2 != NULL))
            return rc;
    /* protocols are the same, order by display name */
    }

    if (sortByStatus) {
        int ordera, orderb;
        ordera = GetStatusModeOrdering(statusa);
        orderb = GetStatusModeOrdering(statusb);
        if (ordera != orderb)
            return ordera - orderb;
    } else if (!g_CluiData.bDontSeparateOffline) {
    //one is offline: offline goes below online
        if ((statusa == ID_STATUS_OFFLINE) != (statusb == ID_STATUS_OFFLINE)) {
            return 2 * (statusa == ID_STATUS_OFFLINE) - 1;
        }
    }

    namea = pcli->pfnGetContactDisplayName(a, 0);
    nameb = pcli->pfnGetContactDisplayName(b, 0);
    
    //otherwise just compare names
    return CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, namea, -1, nameb, -1) - 2;
    //return _tcsicmp(namea,nameb);
}

#undef SAFESTRING

static int resortTimerId = 0;
static VOID CALLBACK SortContactsTimer(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
    KillTimer(NULL, resortTimerId);
    resortTimerId = 0;
    CallService(MS_CLUI_SORTLIST, 0, 0);
}

int SetHideOffline(WPARAM wParam, LPARAM lParam)
{
    switch ((int) wParam) {
        case 0:
            DBWriteContactSettingByte(NULL, "CList", "HideOffline", 0); break;
        case 1:
            DBWriteContactSettingByte(NULL, "CList", "HideOffline", 1); break;
        case -1:
            DBWriteContactSettingByte(NULL, "CList", "HideOffline", (BYTE) ! DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT)); break;
    }
    SetButtonStates(pcli->hwndContactList);
    LoadContactTree();
    return 0;
}