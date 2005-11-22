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

UNICODE - done.

*/
#include "commonheaders.h"
#include "resource.h"
#include "m_userinfo.h"

static LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int DefaultImageListColorDepth=ILC_COLOR32;

extern struct CluiData g_CluiData;
extern struct ClcData *g_clcData;

extern HWND hwndContactList, hwndContactTree;
//extern void RemoveFromImgCache(HANDLE hContact, struct avatarCacheEntry *ace), FreeImgCache(), ShutdownGdiPlus();
extern pfnDrawAlpha pDrawAlpha;
extern HWND hwndTip;
extern TOOLINFOA ti;
extern HANDLE hTTProcess;
extern BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);
extern int during_sizing, g_isConnecting;
extern StatusItems_t *StatusItems;
extern int g_shutDown;
extern int g_nextExtraCacheEntry, g_maxExtraCacheEntry;
extern struct ExtraCache *g_ExtraCache;
extern int disableTrayFlash, disableIconFlash;
extern COLORREF g_CLUISkinnedBkColorRGB;

extern void InvalidateDisplayNameCacheEntry(HANDLE hContact);

HIMAGELIST himlCListClc;
extern HIMAGELIST himlExtraImages;

HANDLE hClcWindowList;
static HANDLE hShowInfoTipEvent;
HANDLE hHideInfoTipEvent;
HANDLE hSoundHook = 0, hIcoLibChanged = 0;

static HANDLE hAckHook;
static HANDLE hClcSettingsChanged;

static HRESULT  (WINAPI *MyCloseThemeData)(HANDLE);

LONG g_cxsmIcon, g_cysmIcon;

int hClcProtoCount = 0;

ClcProtoStatus *clcProto = NULL;

int GetProtocolVisibility(char *ProtoName)
{
    int i;
    int res=0;
    DBVARIANT dbv;
    char buf2[10];
    int count;

    if(ProtoName == NULL)
        return 0;
    
    count = (int)DBGetContactSettingDword(0, "Protocols", "ProtoCount", -1);
    if (count == -1) 
        return 1;
    for (i = 0; i < count; i++) {
        _itoa(i, buf2, 10);
        if (!DBGetContactSetting(NULL, "Protocols", buf2, &dbv)) {
            if (strcmp(ProtoName, dbv.pszVal) == 0) {
                mir_free(dbv.pszVal);
                _itoa(i + 400, buf2, 10);
                res= DBGetContactSettingDword(NULL, "Protocols", buf2, 0);
                return res;
            }
            mir_free(dbv.pszVal);
        }
    }
    return 0;
}

void ClcOptionsChanged(void)
{
    WindowList_Broadcast(hClcWindowList, INTM_RELOADOPTIONS, 0, 0);
}

int AvatarChanged(WPARAM wParam, LPARAM lParam)
{
    struct avatarCacheEntry *cEntry = (struct avatarCacheEntry *)lParam;

    WindowList_Broadcast(hClcWindowList, INTM_AVATARCHANGED, wParam, lParam);
    return 0;
}

static int __forceinline __strcmp(const char * src, const char * dst)
{
    int ret = 0 ;
    
    while( ! (ret = *(unsigned char *)src - *(unsigned char *)dst) && *dst)
            ++src, ++dst;
    
    /*
    if ( ret < 0 )
            ret = -1 ;
    else if ( ret > 0 )
            ret = 1 ;
    */
    return( ret );
}

static int ClcSettingChanged(WPARAM wParam, LPARAM lParam)
{
    char *szProto = NULL;
    DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;  

    if (wParam) {
        if(!__strcmp(cws->szModule, "CList")) {
            if (!__strcmp(cws->szSetting, "MyHandle")) {
                InvalidateDisplayNameCacheEntry((HANDLE) wParam);
                WindowList_Broadcast(hClcWindowList, INTM_NAMECHANGED, wParam, lParam);
            }
            else if (!__strcmp(cws->szSetting, "Group"))
                WindowList_Broadcast(hClcWindowList, INTM_GROUPCHANGED, wParam, lParam);
            else if (!__strcmp(cws->szSetting, "Hidden")) {
                //if(cws->value.bVal != 0 && cws->value.type != DBVT_DELETED)
                //    _DebugPopup((HANDLE)wParam, "%s / %s changed to %d", cws->szModule, cws->szSetting, cws->value.bVal);
                if (cws->value.type == DBVT_DELETED || cws->value.bVal == 0) {
                    char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
                    ChangeContactIcon((HANDLE) wParam, IconFromStatusMode(szProto, szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE) wParam, szProto, "Status", ID_STATUS_OFFLINE), (HANDLE) wParam, NULL), 1);
                } else
                    CallService(MS_CLUI_CONTACTDELETED, wParam, 0);
                WindowList_Broadcast(hClcWindowList, INTM_HIDDENCHANGED, wParam, lParam);
            }
            else if (!__strcmp(cws->szSetting, "NotOnList"))
                WindowList_Broadcast(hClcWindowList, INTM_NOTONLISTCHANGED, wParam, lParam);
            else if (!__strcmp(cws->szSetting, "Status"))
                WindowList_Broadcast(hClcWindowList, INTM_INVALIDATE, 0, 0);
            else if (!__strcmp(cws->szSetting, "NameOrder"))
                WindowList_Broadcast(hClcWindowList, INTM_NAMEORDERCHANGED, 0, 0);
            else if (!__strcmp(cws->szSetting, "StatusMsg"))
                WindowList_Broadcast(hClcWindowList, INTM_STATUSMSGCHANGED, wParam, lParam);
        } else if (!__strcmp(cws->szModule, "UserInfo")) {
            if (!__strcmp(cws->szSetting, "ANSIcodepage"))
                WindowList_Broadcast(hClcWindowList, INTM_CODEPAGECHANGED, wParam, lParam);
            else if(!__strcmp(cws->szSetting, "Timezone"))
                ReloadExtraInfo((HANDLE)wParam);
        } else if(wParam != 0 && (szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0)) != NULL) {
            char *id = NULL;
            if (!__strcmp(cws->szModule, "Protocol") && !__strcmp(cws->szSetting, "p")) {
                char *szProto_s;
                WindowList_Broadcast(hClcWindowList, INTM_PROTOCHANGED, wParam, lParam);
                if (cws->value.type == DBVT_DELETED)
                    szProto_s = NULL;
                else
                    szProto_s = cws->value.pszVal;
                ChangeContactIcon((HANDLE) wParam, IconFromStatusMode(szProto_s, szProto_s == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE) wParam, szProto_s, "Status", ID_STATUS_OFFLINE), (HANDLE) wParam, NULL), 0);
            }
    // something is being written to a protocol module
            if (!__strcmp(szProto, cws->szModule)) {
    // was a unique setting key written?
                id = (char*) CallProtoService(szProto, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
                InvalidateDisplayNameCacheEntry((HANDLE) wParam);
                if ((int) id != CALLSERVICE_NOTFOUND && id != NULL && !__strcmp(id, cws->szSetting)) {
                    WindowList_Broadcast(hClcWindowList, INTM_PROTOCHANGED, wParam, lParam);
                } else if (!__strcmp(cws->szSetting, "Status")) {
                    if (!DBGetContactSettingByte((HANDLE) wParam, "CList", "Hidden", 0)) {
                        if (DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT)) {
        // User's state is changing, and we are hideOffline-ing
                            if (cws->value.wVal == ID_STATUS_OFFLINE) {
                                ChangeContactIcon((HANDLE) wParam, IconFromStatusMode(cws->szModule, cws->value.wVal, (HANDLE) wParam, NULL), 0);
                                CallService(MS_CLUI_CONTACTDELETED, wParam, 0);
                                return 0;
                            }
                            ChangeContactIcon((HANDLE) wParam, IconFromStatusMode(cws->szModule, cws->value.wVal, (HANDLE) wParam, NULL), 1);
                        }
                        ChangeContactIcon((HANDLE) wParam, IconFromStatusMode(cws->szModule, cws->value.wVal, (HANDLE) wParam, NULL), 0);
                    }
                    SendMessage(hwndContactTree, INTM_STATUSCHANGED, wParam, lParam);
                    return 0;
                }
                else if (g_CluiData.bMetaAvail  && !(g_CluiData.dwFlags & CLUI_USEMETAICONS) && !__strcmp(szProto, "MetaContacts")) {
                    if ((lstrlenA(cws->szSetting) > 6 && !strncmp(cws->szSetting, "Status", 6)) || strstr("Default,ForceSend,Nick", cws->szSetting))
                        WindowList_Broadcast(hClcWindowList, INTM_NAMEORDERCHANGED, wParam, lParam);
                }
                else if(strstr("YMsg|StatusDescr|XStatusMsg", cws->szSetting))
                    WindowList_Broadcast(hClcWindowList, INTM_STATUSMSGCHANGED, wParam, lParam);
                else if (strstr(cws->szSetting, "XStatus"))
                    WindowList_Broadcast(hClcWindowList, INTM_XSTATUSCHANGED, wParam, lParam);
                else if(!__strcmp(cws->szSetting, "Timezone"))
                    ReloadExtraInfo((HANDLE)wParam);
            }
            if(strstr("Nick|FirstName|e-mail|LastName|UIN", cws->szSetting)) {
            //if (!__strcmp(cws->szSetting, "Nick") || !__strcmp(cws->szSetting, "FirstName") || !__strcmp(cws->szSetting, "e-mail") || !__strcmp(cws->szSetting, "LastName") || !__strcmp(cws->szSetting, "UIN")) {
                CallService(MS_CLUI_CONTACTRENAMED, wParam, 0);
                WindowList_Broadcast(hClcWindowList, INTM_NAMECHANGED, wParam, lParam);
            }
            else if (!__strcmp(cws->szSetting, "ApparentMode"))
                WindowList_Broadcast(hClcWindowList, INTM_APPARENTMODECHANGED, wParam, lParam);
            else if (!__strcmp(cws->szSetting, "IdleTS"))
                WindowList_Broadcast(hClcWindowList, INTM_IDLECHANGED, wParam, lParam);
            else if (!__strcmp(cws->szSetting, "MirVer"))
                WindowList_Broadcast(hClcWindowList, INTM_CLIENTCHANGED, wParam, lParam);
        }
    }
    else if (wParam == 0 && !__strcmp(cws->szModule, "MetaContacts")) {
        BYTE bMetaEnabled = DBGetContactSettingByte(NULL, "MetaContacts", "Enabled", 1);
        if(bMetaEnabled != (BYTE)g_CluiData.bMetaEnabled) {
            g_CluiData.bMetaEnabled = bMetaEnabled;
            WindowList_Broadcast(hClcWindowList, CLM_AUTOREBUILD, 0, 0);
        }
    }
    else if (wParam == 0 && !__strcmp(cws->szModule, "CList")) {
        if (!__strcmp(cws->szSetting, "DisableTrayFlash"))
            disableTrayFlash = (int) cws->value.bVal;
        else if (!__strcmp(cws->szSetting, "NoIconBlink"))
            disableIconFlash = (int) cws->value.bVal;
    }
    else if(szProto == NULL && wParam == 0) {
        if(!__strcmp(cws->szSetting, "XStatusId"))
            CluiProtocolStatusChanged(0, 0);
        else if (!__strcmp(cws->szModule, "CListGroups"))
            WindowList_Broadcast(hClcWindowList, INTM_GROUPSCHANGED, wParam, lParam);
        return 0;
    }
    return 0;
}

static int ClcModulesLoaded(WPARAM wParam, LPARAM lParam)
{
    PROTOCOLDESCRIPTOR **proto;
    int protoCount, i;
    DBVARIANT dbv = {
        0
    };

    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &protoCount, (LPARAM) &proto);
    for (i = 0; i < protoCount; i++) {
        if (proto[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        clcProto = (ClcProtoStatus *) mir_realloc(clcProto, sizeof(ClcProtoStatus) * (hClcProtoCount + 1));
        clcProto[hClcProtoCount]. szProto = proto[i]->szName;
        clcProto[hClcProtoCount]. dwStatus = ID_STATUS_OFFLINE;
        hClcProtoCount++;
    }

    // Load the settings for the extended background features from the DB
    LoadExtBkSettingsFromDB();
    //IMG_LoadItems();
    return 0;
}

static int ClcProtoAck(WPARAM wParam, LPARAM lParam)
{
    ACKDATA *ack = (ACKDATA *) lParam;
    int i;

    if (ack->type == ACKTYPE_STATUS) {
        if (ack->result == ACKRESULT_SUCCESS) {
            for (i = 0; i < hClcProtoCount; i++) {
                if (!lstrcmpA(clcProto[i].szProto, ack->szModule)) {
                    clcProto[i]. dwStatus = (WORD) ack->lParam;
                    break;
                }
            }
        }
    }
    return 0;
}

static int ClcContactAdded(WPARAM wParam, LPARAM lParam)
{
    WindowList_Broadcast(hClcWindowList, INTM_CONTACTADDED, wParam, lParam);
    return 0;
}
                                           
static int ClcContactDeleted(WPARAM wParam, LPARAM lParam)
{
    WindowList_Broadcast(hClcWindowList, INTM_CONTACTDELETED, wParam, lParam);
    return 0;
}

static int ClcContactIconChanged(WPARAM wParam, LPARAM lParam)
{
    WindowList_Broadcast(hClcWindowList, INTM_ICONCHANGED, wParam, lParam);
    return 0;
}

static int ClcIconsChanged(WPARAM wParam, LPARAM lParam)
{
    SendMessage(GetDlgItem(hwndContactList, IDC_TBMENU), BM_SETIMAGE, IMAGE_ICON, (LPARAM) LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
    WindowList_Broadcast(hClcWindowList, INTM_INVALIDATE, 0, 0);
    return 0;
}

static int SetInfoTipHoverTime(WPARAM wParam, LPARAM lParam)
{
    DBWriteContactSettingWord(NULL, "CLC", "InfoTipHoverTime", (WORD) wParam);
    WindowList_Broadcast(hClcWindowList, INTM_SETINFOTIPHOVERTIME, wParam, 0);
    return 0;
}

static int GetInfoTipHoverTime(WPARAM wParam, LPARAM lParam)
{
    return DBGetContactSettingWord(NULL, "CLC", "InfoTipHoverTime", 750);
}

static int ClcShutdown(WPARAM wParam, LPARAM lParam)
{
	SFL_Destroy();
	g_shutDown = TRUE;
    UnhookEvent(hAckHook);
    UnhookEvent(hClcSettingsChanged);
    if (hIcoLibChanged)
        UnhookEvent(hIcoLibChanged);
    mir_free(clcProto);
    FreeFileDropping();
    ImageList_RemoveAll(himlExtraImages);
    ImageList_Destroy(himlExtraImages);
    if (g_CluiData.hIconInvisible)
        DestroyIcon(g_CluiData.hIconInvisible);
    if (g_CluiData.hIconVisible)
        DestroyIcon(g_CluiData.hIconVisible);
	if (g_CluiData.hIconChatactive)
		DestroyIcon(g_CluiData.hIconChatactive);

	DeleteObject(g_CluiData.hBrushColorKey);
	DeleteObject(g_CluiData.hBrushCLCBk);
    DeleteObject(g_CluiData.hBrushAvatarBorder);
    ClearIcons(1);
    ShutdownGdiPlus();
    pDrawAlpha = 0;
    free(StatusItems);
    return 0;
}

// simulate tweakUI

void Tweak_It(COLORREF clr)
{
    SetWindowLong(hwndContactList, GWL_EXSTYLE, GetWindowLong(hwndContactList, GWL_EXSTYLE) | WS_EX_LAYERED);
    MySetLayeredWindowAttributes(hwndContactList, clr, 0, LWA_COLORKEY);
    g_CluiData.colorkey = clr;
}

int LoadCLCModule(void)
{
    WNDCLASS wndclass;

    g_cxsmIcon = GetSystemMetrics(SM_CXSMICON);
    g_cysmIcon = GetSystemMetrics(SM_CYSMICON);

    himlCListClc = (HIMAGELIST) CallService(MS_CLIST_GETICONSIMAGELIST, 0, 0);
    hClcWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
    hShowInfoTipEvent = CreateHookableEvent(ME_CLC_SHOWINFOTIP);
    hHideInfoTipEvent = CreateHookableEvent(ME_CLC_HIDEINFOTIP);
    CreateServiceFunction(MS_CLC_SETINFOTIPHOVERTIME, SetInfoTipHoverTime);
    CreateServiceFunction(MS_CLC_GETINFOTIPHOVERTIME, GetInfoTipHoverTime);

    wndclass.style = /*CS_HREDRAW | CS_VREDRAW | */CS_DBLCLKS | CS_GLOBALCLASS | CS_PARENTDC;
    wndclass.lpfnWndProc = ContactListControlWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = sizeof(void*);
    wndclass.hInstance = g_hInst;
    wndclass.hIcon = NULL;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = CLISTCONTROL_CLASS;
    RegisterClass(&wndclass);
    LoadCLCButtonModule();
    InitFileDropping();

    HookEvent(ME_SYSTEM_MODULESLOADED, ClcModulesLoaded);
    hClcSettingsChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ClcSettingChanged);
    HookEvent(ME_DB_CONTACT_ADDED, ClcContactAdded);
    HookEvent(ME_DB_CONTACT_DELETED, ClcContactDeleted);
    HookEvent(ME_CLIST_CONTACTICONCHANGED, ClcContactIconChanged);
    HookEvent(ME_SKIN_ICONSCHANGED, ClcIconsChanged);
    HookEvent(ME_OPT_INITIALISE, ClcOptInit);
    hAckHook = (HANDLE) HookEvent(ME_PROTO_ACK, ClcProtoAck);
    HookEvent(ME_SYSTEM_SHUTDOWN, ClcShutdown);

    return 0;
}

static LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct ClcData *dat;

    dat = (struct ClcData *) GetWindowLong(hwnd, 0);
    if (msg >= CLM_FIRST && msg < CLM_LAST)
        return ProcessExternalMessages(hwnd, dat, msg, wParam, lParam);

    switch (msg) {
        case WM_CREATE:
            WindowList_Add(hClcWindowList, hwnd, NULL);
            RegisterFileDropping(hwnd);
            dat = (struct ClcData *) mir_alloc(sizeof(struct ClcData));
            SetWindowLong(hwnd, 0, (LONG) dat); {
                int i;
                for (i = 0; i <= FONTID_MAX; i++) {
                    dat->fontInfo[i].changed = 1;
                }
            }

			RowHeights_Initialize(dat);
            dat->forcePaint = dat->forceScroll = 0;
            dat->lastRepaint = 0;
            dat->yScroll = 0;
            dat->selection = -1;
            dat->iconXSpace = 20;
            dat->checkboxSize = 13;
            dat->dragAutoScrollHeight = 30;
            dat->himlHighlight = NULL;
            dat->hBmpBackground = NULL;
            dat->hwndRenameEdit = NULL;
            dat->iDragItem = -1;
            dat->iInsertionMark = -1;
            dat->insertionMarkHitHeight = 5;
            dat->szQuickSearch[0] = 0;
            dat->bkChanged = 0;
            dat->iHotTrack = -1;
            dat->infoTipTimeout = DBGetContactSettingWord(NULL, "CLC", "InfoTipHoverTime", 750);
            dat->hInfoTipItem = NULL;
            dat->himlExtraColumns = himlExtraImages;
            dat->extraColumnsCount = 0;
            dat->extraColumnSpacing = 20;
            dat->showSelAlways = 0;
            dat->list.contactCount = 0;
            dat->list.allocedCount = 0;
            dat->list.contact = NULL;
            dat->list.parent = NULL;
            dat->list.hideOffline = 0;
            dat->isMultiSelect = 0;
            dat->hwndParent = GetParent(hwnd);
            dat->lastSort = GetTickCount();
            dat->bNeedSort = FALSE;
            {
                CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
                if(cs->lpCreateParams == (LPVOID)0xff00ff00) {
                    dat->bisEmbedded = FALSE;
                    if(g_CluiData.bShowLocalTime)
                        SetTimer(hwnd, TIMERID_REFRESH, 65000, NULL);
                }
                else
                    dat->bisEmbedded = TRUE;
            }
            LoadClcOptions(hwnd, dat);
            RebuildEntireList(hwnd, dat); {
                NMCLISTCONTROL nm;
                nm.hdr.code = CLN_LISTREBUILT;
                nm.hdr.hwndFrom = hwnd;
                nm.hdr.idFrom = GetDlgCtrlID(hwnd);
                SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
            }
            if(GetParent(hwnd) == hwndContactList && MySetLayeredWindowAttributes != 0 && g_CluiData.bFullTransparent) {
                if(g_CLUISkinnedBkColorRGB)
                    Tweak_It(g_CLUISkinnedBkColorRGB);
                else if(g_CluiData.bClipBorder || g_CluiData.dwFlags & CLUI_FRAME_ROUNDEDFRAME)
                    Tweak_It(RGB(255, 0, 255));
                else
                    Tweak_It(dat->bkColour);
            }
            break;
        case INTM_SCROLLBARCHANGED:
            {
                if (GetWindowLong(hwnd, GWL_STYLE) & CLS_CONTACTLIST) {
                    if (dat->noVScrollbar)
                        ShowScrollBar(hwnd, SB_VERT, FALSE);
                    else
                        RecalcScrollBar(hwnd, dat);
                }
                break;
            }
        case INTM_RELOADOPTIONS:
            LoadClcOptions(hwnd, dat);
            SaveStateAndRebuildList(hwnd, dat);
            break;
        case WM_THEMECHANGED:
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case WM_SIZE:
            EndRename(hwnd, dat, 1);
            KillTimer(hwnd, TIMERID_INFOTIP);
            KillTimer(hwnd, TIMERID_RENAME);
            RecalcScrollBar(hwnd, dat);
            break;

        case WM_SYSCOLORCHANGE:
            SendMessage(hwnd, WM_SIZE, 0, 0);
            break;

        case WM_GETDLGCODE:
            if (lParam) {
                MSG *msg = (MSG *) lParam;
                if (msg->message == WM_KEYDOWN) {
                    if (msg->wParam == VK_TAB)
                        return 0;
                    if (msg->wParam == VK_ESCAPE && dat->hwndRenameEdit == NULL && dat->szQuickSearch[0] == 0)
                        return 0;
                }
                if (msg->message == WM_CHAR) {
                    if (msg->wParam == '\t')
                        return 0;
                    if (msg->wParam == 27 && dat->hwndRenameEdit == NULL && dat->szQuickSearch[0] == 0)
                        return 0;
                }
            }
            return DLGC_WANTMESSAGE;

        case WM_KILLFOCUS:
            KillTimer(hwnd, TIMERID_INFOTIP);
            KillTimer(hwnd, TIMERID_RENAME);
        case WM_SETFOCUS:
        case WM_ENABLE:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_GETFONT:
            return(LRESULT) dat->fontInfo[FONTID_CONTACTS].hFont;

        case INTM_GROUPSCHANGED:
            {
                DBCONTACTWRITESETTING *dbcws = (DBCONTACTWRITESETTING *) lParam;
                if (dbcws->value.type == DBVT_ASCIIZ || dbcws->value.type == DBVT_UTF8) {
                    int groupId = atoi(dbcws->szSetting) + 1;
                    struct ClcContact *contact;
                    struct ClcGroup *group;
                    TCHAR szFullName[512];
                    int i, nameLen, eq;
        //check name of group and ignore message if just being expanded/collapsed
                    if (FindItem(hwnd, dat, (HANDLE) (groupId | HCONTACT_ISGROUP), &contact, &group, NULL)) {
                        lstrcpy(szFullName, contact->szText);
                        while (group->parent) {
                            for (i = 0; i < group->parent->contactCount; i++) {
                                if (group->parent->contact[i].group == group)
                                    break;
                            }
                            if (i == group->parent->contactCount) {
                                szFullName[0] = '\0';
                                break;
                            }
                            group = group->parent;
                            nameLen = lstrlen(group->contact[i].szText);
                            if (lstrlen(szFullName) + 1 + nameLen > safe_sizeof(szFullName)) {
                                szFullName[0] = '\0';
                                break;
                            }
                            memmove(szFullName + 1 + nameLen, szFullName, sizeof(TCHAR) * (lstrlen(szFullName) + 1));
                            memcpy(szFullName, group->contact[i].szText, sizeof(TCHAR) * nameLen);
                            szFullName[nameLen] = '\\';
                        }

                        if ( dbcws->value.type == DBVT_ASCIIZ )
                        #if defined( UNICODE )
                        {	WCHAR* wszGrpName = a2u(dbcws->value.pszVal+1);
                            eq = !lstrcmp( szFullName, wszGrpName );
                            mir_free( wszGrpName );
                        }
                        #else
                            eq = !lstrcmp( szFullName, dbcws->value.pszVal+1 );
                        #endif
                        else {
                            char* szGrpName = NEWSTR_ALLOCA(dbcws->value.pszVal+1);
                            #if defined( UNICODE )
                                WCHAR* wszGrpName = NULL;
                                Utf8Decode(szGrpName, &wszGrpName );
                                eq = !lstrcmp( szFullName, wszGrpName );
                                /*mir_*/free( wszGrpName );                 // XXX mir_free() is wrong here, Utf8Decode() 
                                                                            // doesn't mir_alloc

                            #else
                                Utf8Decode(szGrpName, NULL);
                                eq = !lstrcmp( szFullName, szGrpName );
                            #endif
                        }

                        if (eq && (contact->group->hideOffline != 0) == ((dbcws->value.pszVal[0] & GROUPF_HIDEOFFLINE) != 0))
                            break;  //only expanded has changed: no action reqd
                    }
                }
                SaveStateAndRebuildList(hwnd, dat);
                break;
            }

        case INTM_NAMEORDERCHANGED:
            PostMessage(hwnd, CLM_AUTOREBUILD, 0, 0);
            break;

        case INTM_CONTACTADDED:
            AddContactToTree(hwnd, dat, (HANDLE) wParam, 1, 1);          
            NotifyNewContact(hwnd, (HANDLE) wParam);
            RecalcScrollBar(hwnd, dat);
            dat->bNeedSort = TRUE;
            PostMessage(hwnd, INTM_SORTCLC, 0, 0);
            // XXX SortCLC(hwnd, dat, 1);
            break;

        case INTM_CONTACTDELETED:
            DeleteItemFromTree(hwnd, (HANDLE) wParam);
            SortCLC(hwnd, dat, 1);
            RecalcScrollBar(hwnd, dat);
            break;

        case INTM_HIDDENCHANGED:
            {
                DBCONTACTWRITESETTING *dbcws = (DBCONTACTWRITESETTING *) lParam;
                if (GetWindowLong(hwnd, GWL_STYLE) & CLS_SHOWHIDDEN)
                    break;
                if (dbcws->value.type == DBVT_DELETED || dbcws->value.bVal == 0) {
                    if (FindItem(hwnd, dat, (HANDLE) wParam, NULL, NULL, NULL))
                        break;
                    AddContactToTree(hwnd, dat, (HANDLE) wParam, 1, 1);
                    NotifyNewContact(hwnd, (HANDLE) wParam);
                } else
                    DeleteItemFromTree(hwnd, (HANDLE) wParam);
                SortCLC(hwnd, dat, 1);
                RecalcScrollBar(hwnd, dat);
                break;
            }

        case INTM_GROUPCHANGED:
            {
                struct ClcContact *contact;
                BYTE iExtraImage[MAXEXTRACOLUMNS];
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    memset(iExtraImage, 0xFF, sizeof(iExtraImage));
                else
                    CopyMemory(iExtraImage, contact->iExtraImage, sizeof(iExtraImage));
                DeleteItemFromTree(hwnd, (HANDLE) wParam);
                if (GetWindowLong(hwnd, GWL_STYLE) & CLS_SHOWHIDDEN || !CLVM_GetContactHiddenStatus((HANDLE)wParam, NULL, dat)) {
                    NMCLISTCONTROL nm;
                    AddContactToTree(hwnd, dat, (HANDLE) wParam, 1, 1);
                    if (FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                        CopyMemory(contact->iExtraImage, iExtraImage, sizeof(iExtraImage));
                    nm.hdr.code = CLN_CONTACTMOVED;
                    nm.hdr.hwndFrom = hwnd;
                    nm.hdr.idFrom = GetDlgCtrlID(hwnd);
                    nm.flags = 0;
                    nm.hItem = (HANDLE) wParam;
                    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
                }
                dat->bNeedSort = TRUE;
                PostMessage(hwnd, INTM_SORTCLC, 0, 1);
                //SortCLC(hwnd, dat, 1);
                //RecalcScrollBar(hwnd, dat);
                break;
            }

        case INTM_ICONCHANGED:
            {
                struct ClcContact *contact = NULL;
                struct ClcGroup *group = NULL;
                int recalcScrollBar = 0,shouldShow;
                WORD status = ID_STATUS_OFFLINE;
                char *szProto;
                
                szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
                if (szProto == NULL)
                    status = ID_STATUS_OFFLINE;
                else
                    status = DBGetContactSettingWord((HANDLE) wParam, szProto, "Status", ID_STATUS_OFFLINE);

                shouldShow = (GetWindowLong(hwnd, GWL_STYLE) & CLS_SHOWHIDDEN || !CLVM_GetContactHiddenStatus((HANDLE)wParam, szProto, dat)) && ((g_CluiData.bFilterEffective ? TRUE : !IsHiddenMode(dat, status)) || CallService(MS_CLIST_GETCONTACTICON, wParam, 0) != lParam);  // XXX CLVM changed - this means an offline msg is flashing, so the contact should be shown
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, &group, NULL)) {
                    if (shouldShow) {
                        AddContactToTree(hwnd, dat, (HANDLE) wParam, 0, 0);
                        recalcScrollBar = 1;                  
                        FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL);
                        if (contact) {
                            contact->iImage = (WORD) lParam;
                            NotifyNewContact(hwnd, (HANDLE) wParam);
                        }
                    }
                } else {
        //item in list already
                    DWORD style = GetWindowLong(hwnd, GWL_STYLE);              
                    if (contact->iImage == (WORD) lParam)
                        break;
                    if (!shouldShow && !(style & CLS_NOHIDEOFFLINE) && (style & CLS_HIDEOFFLINE || group->hideOffline || g_CluiData.bFilterEffective)) {        // CLVM changed
                        HANDLE hSelItem;
                        struct ClcContact *selcontact;
                        struct ClcGroup *selgroup;
                        if (GetRowByIndex(dat, dat->selection, &selcontact, NULL) == -1)
                            hSelItem = NULL;
                        else
                            hSelItem = ContactToHItem(selcontact);
                        RemoveItemFromGroup(hwnd, group, contact, 0);
                        if (hSelItem)
                            if (FindItem(hwnd, dat, hSelItem, &selcontact, &selgroup, NULL))
                                dat->selection = GetRowsPriorTo(&dat->list, selgroup, selcontact - selgroup->contact);
                        recalcScrollBar = 1;
                    } else {
                        contact->iImage = (WORD) lParam;
                        if (!IsHiddenMode(dat, status))
                            contact->flags |= CONTACTF_ONLINE;
                        else
                            contact->flags &= ~CONTACTF_ONLINE;
                    }
                }
                dat->bNeedSort = TRUE;
                PostMessage(hwnd, INTM_SORTCLC, 0, recalcScrollBar);
                PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
                if (recalcScrollBar)
                    RecalcScrollBar(hwnd, dat);
                break;
            }

        case INTM_METACHANGED:
            {
                struct ClcContact *contact;
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                if(contact->bIsMeta && g_CluiData.bMetaAvail && !(g_CluiData.dwFlags & CLUI_USEMETAICONS)) {
                    contact->hSubContact = (HANDLE) CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM) contact->hContact, 0);
                    contact->metaProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) contact->hSubContact, 0);
                    contact->iImage = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) contact->hSubContact, 0);
                    if(contact->extraCacheEntry >= 0 && contact->extraCacheEntry < g_nextExtraCacheEntry) {
                        int subIndex = GetExtraCache(contact->hSubContact, contact->metaProto);
                        g_ExtraCache[contact->extraCacheEntry].proto_status_item = GetProtocolStatusItem(contact->metaProto);
                        if(subIndex >= 0 && subIndex <= g_nextExtraCacheEntry)
                            g_ExtraCache[contact->extraCacheEntry].status_item = g_ExtraCache[subIndex].status_item;
                    }
                }
                SendMessage(hwnd, INTM_NAMEORDERCHANGED, wParam, lParam);
                break;
            }
        case INTM_METACHANGEDEVENT:
            {
                struct ClcContact *contact;
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                if(lParam == 0)
                    SendMessage(hwnd, CLM_AUTOREBUILD, 0, 0);
                break;
            }
        case INTM_NAMECHANGED:
            {
                struct ClcContact *contact;
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                lstrcpyn(contact->szText, GetContactDisplayNameW((HANDLE) wParam, 0), safe_sizeof(contact->szText));
#if defined(_UNICODE)
                RTL_DetectAndSet(contact, 0);
#endif                
                dat->bNeedSort = TRUE;
                PostMessage(hwnd, INTM_SORTCLC, 0, 0);
                // XXX SortCLC(hwnd, dat, 1);
                break;
            }

        case INTM_CODEPAGECHANGED:
            {
                struct ClcContact *contact;
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                contact->codePage = DBGetContactSettingDword((HANDLE) wParam, "Tab_SRMsg", "ANSIcodepage", DBGetContactSettingDword((HANDLE)wParam, "UserInfo", "ANSIcodepage", CP_ACP));
                // XXX InvalidateRect(hwnd, NULL, FALSE);
                PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
                break;
            }
        case INTM_AVATARCHANGED:
            {
                struct avatarCacheEntry *cEntry = (struct avatarCacheEntry *)lParam;
                struct ClcContact *contact = NULL;

                if(wParam == 0) {
                    //RemoveFromImgCache(0, cEntry);
                    g_CluiData.bForceRefetchOnPaint = TRUE;
                    InvalidateRect(hwnd, NULL, FALSE);
                    UpdateWindow(hwnd);
                    g_CluiData.bForceRefetchOnPaint = FALSE;
                    break;
                }
                
                if(!FindItem(hwnd, dat, (HANDLE)wParam, &contact, NULL, NULL))
                    break;
                /*
                if(contact->ace) {
                    if(!(contact->ace->dwFlags & AVS_PROTOPIC))
                        RemoveFromImgCache(contact->hContact, contact->ace);
                }*/
                //contact->gdipObject = NULL;
                contact->ace = cEntry;
                // XXX InvalidateRect(hwnd, NULL, FALSE);
                PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
                break;
            }
        case INTM_STATUSMSGCHANGED:
            {
                struct ClcContact *contact = NULL;
                int index = -1;
                char *szProto = NULL;
                
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    index = GetExtraCache((HANDLE)wParam, NULL);
                else {
                    index = contact->extraCacheEntry;
                    szProto = contact->proto;
                }
                GetCachedStatusMsg(index, szProto);
                PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
                break;
            }
        case INTM_STATUSCHANGED:
            {
                struct ClcContact *contact = NULL;
                WORD wStatus;
                
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                
                wStatus = DBGetContactSettingWord((HANDLE)wParam, contact->proto, "Status", ID_STATUS_OFFLINE);
                if(g_CluiData.bNoOfflineAvatars && wStatus != ID_STATUS_OFFLINE && contact->wStatus == ID_STATUS_OFFLINE) {
                    if(g_CluiData.bAvatarServiceAvail && contact->ace == NULL) {
                        contact->ace = (struct avatarCacheEntry *)CallService(MS_AV_GETAVATARBITMAP, wParam, 0);
                        if (contact->ace != NULL && contact->ace->cbSize != sizeof(struct avatarCacheEntry))
                            contact->ace = NULL;
                        if (contact->ace != NULL)
                            contact->ace->t_lastAccess = time(NULL);
                    }
                }
                contact->wStatus = wStatus;
                //dat->bNeedSort = TRUE;
                //PostMessage(hwnd, INTM_SORTCLC, 0, 0);
                break;
            }
        case INTM_PROTOCHANGED:
            {
                DBCONTACTWRITESETTING *dbcws = (DBCONTACTWRITESETTING *) lParam;
                struct ClcContact *contact = NULL;

                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                contact->proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
                CallService(MS_CLIST_INVALIDATEDISPLAYNAME, wParam, 0);
                lstrcpyn(contact->szText, GetContactDisplayNameW((HANDLE) wParam, 0), safe_sizeof(contact->szText));
#if defined(_UNICODE)
                RTL_DetectAndSet(contact, 0);
#endif                
                dat->bNeedSort = TRUE;
                PostMessage(hwnd, INTM_SORTCLC, 0, 0);
                // XXX SortCLC(hwnd, dat, 1);
                break;
            }

        case INTM_NOTONLISTCHANGED:
            {
                DBCONTACTWRITESETTING *dbcws = (DBCONTACTWRITESETTING *) lParam;
                struct ClcContact *contact;
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                if (contact->type != CLCIT_CONTACT)
                    break;
                if (dbcws->value.type == DBVT_DELETED || dbcws->value.bVal == 0)
                    contact->flags &= ~CONTACTF_NOTONLIST;
                else
                    contact->flags |= CONTACTF_NOTONLIST;
                // XXX InvalidateRect(hwnd, NULL, FALSE);
                PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
                break;
            }

        case INTM_INVALIDATE:
            if(!dat->bNeedPaint) {
                //InvalidateRect(hwnd, NULL, FALSE);
                KillTimer(hwnd, TIMERID_PAINT);
                SetTimer(hwnd, TIMERID_PAINT, 100, NULL);
                dat->bNeedPaint = TRUE;
            }
            break;

        case INTM_SORTCLC:
            //KillTimer(hwnd, TIMERID_SORT);
            //SetTimer(hwnd, TIMERID_SORT, 80, NULL);
            if(dat->bNeedSort) {
                SortCLC(hwnd, dat, TRUE);
                dat->bNeedSort = FALSE;
            }
            if(lParam)
                RecalcScrollBar(hwnd, dat);
            break;
            
        case INTM_APPARENTMODECHANGED:
            {
                DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;  
                WORD apparentMode;
                char *szProto;
                struct ClcContact *contact;
                
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                szProto = (char*)cws->szModule;
                if (szProto == NULL)
                    break;
                apparentMode = DBGetContactSettingWord((HANDLE) wParam, szProto, "ApparentMode", 0);
                contact->flags &= ~(CONTACTF_INVISTO | CONTACTF_VISTO);
                if (apparentMode == ID_STATUS_OFFLINE)
                    contact->flags |= CONTACTF_INVISTO;
                else if (apparentMode == ID_STATUS_ONLINE)
                    contact->flags |= CONTACTF_VISTO;
                else if (apparentMode)
                    contact->flags |= CONTACTF_VISTO | CONTACTF_INVISTO;
                // XXX InvalidateRect(hwnd, NULL, FALSE);
                PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
                break;
            }

        case INTM_SETINFOTIPHOVERTIME:
            dat->infoTipTimeout = wParam;
            break;

        case INTM_IDLECHANGED:
            {
                DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;  
                char *szProto;
                struct ClcContact *contact;
                
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                szProto = (char*)cws->szModule;
                if (szProto == NULL)
                    break;
                contact->flags &= ~CONTACTF_IDLE;
                if (DBGetContactSettingDword((HANDLE) wParam, szProto, "IdleTS", 0)) {
                    contact->flags |= CONTACTF_IDLE;
                }
                // XXX InvalidateRect(hwnd, NULL, FALSE);
                PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
                break;
            }
        case INTM_XSTATUSCHANGED:
            {
                DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;  
                char *szProto;
                struct ClcContact *contact;
                int index;
                
                if (!wParam)
                    CluiProtocolStatusChanged(0, 0);

                szProto = (char *)cws->szModule;
                
                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    index = GetExtraCache((HANDLE)wParam, szProto);
                else {
                    contact->xStatus = DBGetContactSettingByte((HANDLE) wParam, szProto, "XStatusId", 0);
                    index = contact->extraCacheEntry;
                }
                if (szProto == NULL)
                    break;
                GetCachedStatusMsg(index, szProto);
                PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
                break;
            }
        case INTM_CLIENTCHANGED:
            {
                DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;  
                char *szProto;
                struct ClcContact *contact;
                DBVARIANT dbv = {0};

                if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
                    break;
                szProto = (char*)cws->szModule;
                if (szProto == NULL)
                    break;
                if (!DBGetContactSetting((HANDLE) wParam, szProto, "MirVer", &dbv)) {
                    if (dbv.type == DBVT_ASCIIZ && dbv.pszVal != NULL && lstrlenA(dbv.pszVal) > 1)
                        GetClientID(contact, dbv.pszVal);
                    DBFreeVariant(&dbv);
                }
                // XXX InvalidateRect(hwnd, NULL, FALSE);
                PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
                break;
            }
        case WM_PRINTCLIENT:
            PaintClc(hwnd, dat, (HDC) wParam, NULL);
            break;

        case WM_NCPAINT:
            if (wParam == 1)
                break; {
                POINT ptTopLeft = {
                    0, 0
                };
                HRGN hClientRgn;
                ClientToScreen(hwnd, &ptTopLeft);
                hClientRgn = CreateRectRgn(0, 0, 1, 1);
                CombineRgn(hClientRgn, (HRGN) wParam, NULL, RGN_COPY);
                OffsetRgn(hClientRgn, -ptTopLeft.x, -ptTopLeft.y);
                InvalidateRgn(hwnd, hClientRgn, FALSE);
                DeleteObject(hClientRgn);
                UpdateWindow(hwnd);
            }
            break;

        case WM_PAINT:
            {
                HDC hdc;
                PAINTSTRUCT ps;
                hdc = BeginPaint(hwnd, &ps);
                if (IsWindowVisible(hwnd) && !during_sizing) {
                    if((g_isConnecting && GetTickCount() - dat->lastRepaint > 500) || !g_isConnecting || dat->forcePaint) {
                        PaintClc(hwnd, dat, hdc, &ps.rcPaint);
                        dat->bNeedPaint = FALSE;
                        dat->lastRepaint = GetTickCount();
                        dat->forcePaint = FALSE;
                    }
                }
                EndPaint(hwnd, &ps);
                break;
            }

		case WM_VSCROLL:
		{	int desty;
			RECT clRect;
			int noSmooth=0;
			int item;

			EndRename(hwnd,dat,1);
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			desty=dat->yScroll;
			GetClientRect(hwnd,&clRect);
			item = RowHeights_HitTest(dat, dat->yScroll);
			switch(LOWORD(wParam)) {
				case SB_LINEUP: 
                    desty -= ( item > 0 && item <= dat->row_heights_size ? dat->row_heights[item-1] : dat->max_row_height ); 
                    break;
				case SB_LINEDOWN: 
                    desty += ( item >= 0 && item < dat->row_heights_size >= 0 ? dat->row_heights[item] : dat->max_row_height ); 
                    break;
				case SB_PAGEUP: 
                    desty-=clRect.bottom-dat->max_row_height; 
                    break;
				case SB_PAGEDOWN: 
                    desty+=clRect.bottom -dat->max_row_height; 
                    break;
				case SB_BOTTOM: 
                    desty=0x7FFFFFFF; 
                    break;
				case SB_TOP: 
                    desty=0; 
                    break;
				case SB_THUMBTRACK: 
                    desty = HIWORD(wParam); 
                    noSmooth=1; 
                    break;    //noone has more than 4000 contacts, right?
				default: 
                    return 0;
			}
            dat->forceScroll = TRUE;
			ScrollTo(hwnd,dat,desty,noSmooth);
			break;
		}
        
		case WM_MOUSEWHEEL:
		{	UINT scrollLines;
			EndRename(hwnd,dat,1);
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			if(!SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0,&scrollLines,FALSE))
				scrollLines=3;
            dat->forceScroll = TRUE;
			ScrollTo(hwnd,dat,dat->yScroll-(short)HIWORD(wParam)*dat->max_row_height*(signed)scrollLines/WHEEL_DELTA,0);
			return 0;
		}

        case WM_KEYDOWN:
            {
                int selMoved = 0;
                int changeGroupExpand = 0;
                int pageSize;
                HideInfoTip(hwnd, dat);
                KillTimer(hwnd, TIMERID_INFOTIP);
                KillTimer(hwnd, TIMERID_RENAME);
                if (CallService(MS_CLIST_MENUPROCESSHOTKEY, wParam, MPCF_CONTACTMENU))
                    break; {
                    RECT clRect;
                    GetClientRect(hwnd, &clRect);
       				pageSize=clRect.bottom/dat->max_row_height;
                    //pageSize = clRect.bottom / dat->rowHeight;
                }
                switch (wParam) {
                    case VK_DOWN:
                        dat->selection++; selMoved = 1; break;
                    case VK_UP:
                        dat->selection--; selMoved = 1; break;
                    case VK_PRIOR:
                        dat->selection -= pageSize; selMoved = 1; break;
                    case VK_NEXT:
                        dat->selection += pageSize; selMoved = 1; break;
                    case VK_HOME:
                        dat->selection = 0; selMoved = 1; break;
                    case VK_END:
                        dat->selection = GetGroupContentsCount(&dat->list, 1) - 1; selMoved = 1; break;
                    case VK_LEFT:
                        changeGroupExpand = 1; break;
                    case VK_RIGHT:
                        changeGroupExpand = 2; break;
                    case VK_RETURN:
                        DoSelectionDefaultAction(hwnd, dat); return 0;
                    case VK_F2:
                        BeginRenameSelection(hwnd, dat); return 0;
                    case VK_DELETE:
                        DeleteFromContactList(hwnd, dat); return 0;
                    default:
                        {
                            NMKEY nmkey;
                            nmkey.hdr.hwndFrom = hwnd;
                            nmkey.hdr.idFrom = GetDlgCtrlID(hwnd);
                            nmkey.hdr.code = NM_KEYDOWN;
                            nmkey.nVKey = wParam;
                            nmkey.uFlags = HIWORD(lParam);
                            if (SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nmkey))
                                return 0;
                        }
                }
                if (changeGroupExpand) {
                    int hit;
                    struct ClcContact *contact;
                    struct ClcGroup *group;
                    dat->szQuickSearch[0] = 0;
                    hit = GetRowByIndex(dat, dat->selection, &contact, &group);
                    if (hit != -1) {
                        if (changeGroupExpand == 1 && contact->type == CLCIT_CONTACT) {
                            if (group == &dat->list)
                                return 0;
                            dat->selection = GetRowsPriorTo(&dat->list, group, -1);
                            selMoved = 1;
                        } else {
                            if (contact->type == CLCIT_GROUP)
                                SetGroupExpand(hwnd, dat, contact->group, changeGroupExpand == 2);
                            return 0;
                        }
                    } else
                        return 0;
                }
                if (selMoved) {
                    dat->szQuickSearch[0] = 0;
                    if (dat->selection >= GetGroupContentsCount(&dat->list, 1))
                        dat->selection = GetGroupContentsCount(&dat->list, 1) - 1;
                    if (dat->selection < 0)
                        dat->selection = 0;
                    InvalidateRect(hwnd, NULL, FALSE);
                    EnsureVisible(hwnd, dat, dat->selection, 0);
                    UpdateWindow(hwnd);
                    return 0;
                }
                break;
            }

        case WM_CHAR:
            HideInfoTip(hwnd, dat);
            KillTimer(hwnd, TIMERID_INFOTIP);
            KillTimer(hwnd, TIMERID_RENAME);
            if (wParam == 27)   //escape
                dat->szQuickSearch[0] = 0;
            else if (wParam == '\b' && dat->szQuickSearch[0])
                dat->szQuickSearch[lstrlen(dat->szQuickSearch) - 1] = '\0';
            else if (wParam < ' ')
                break;
            else if (wParam == ' ' && dat->szQuickSearch[0] == '\0' && GetWindowLong(hwnd, GWL_STYLE) & CLS_CHECKBOXES) {
                struct ClcContact *contact;
                NMCLISTCONTROL nm;
                if (GetRowByIndex(dat, dat->selection, &contact, NULL) == -1)
                    break;
                if (contact->type != CLCIT_CONTACT)
                    break;
                contact->flags ^= CONTACTF_CHECKED;
                if (contact->type == CLCIT_GROUP)
                    SetGroupChildCheckboxes(contact->group, contact->flags & CONTACTF_CHECKED);
                RecalculateGroupCheckboxes(hwnd, dat);
                InvalidateRect(hwnd, NULL, FALSE);
                nm.hdr.code = CLN_CHECKCHANGED;
                nm.hdr.hwndFrom = hwnd;
                nm.hdr.idFrom = GetDlgCtrlID(hwnd);
                nm.flags = 0;
                nm.hItem = ContactToItemHandle(contact, &nm.flags);
                SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
            } else {
                TCHAR szNew[2];
                szNew[0] = (TCHAR) wParam; 
                szNew[1] = '\0';
                if (lstrlen(dat->szQuickSearch) >= safe_sizeof(dat->szQuickSearch) - 1) {
                    MessageBeep(MB_OK);
                    break;
                }
                _tcscat(dat->szQuickSearch, szNew);
            }
            if (dat->szQuickSearch[0]) {
                int index;
                index = FindRowByText(hwnd, dat, dat->szQuickSearch, 1);
                if (index != -1)
                    dat->selection = index;
                else {
                    MessageBeep(MB_OK);
                    dat->szQuickSearch[lstrlen(dat->szQuickSearch) - 1] = '\0';
                }
                InvalidateRect(hwnd, NULL, FALSE);
                EnsureVisible(hwnd, dat, dat->selection, 0);
            } else
                InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_SYSKEYDOWN:
            EndRename(hwnd, dat, 1);
            HideInfoTip(hwnd, dat);
            KillTimer(hwnd, TIMERID_INFOTIP);
            KillTimer(hwnd, TIMERID_RENAME);
            dat->iHotTrack = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            ReleaseCapture();
            if (wParam == VK_F10 && GetKeyState(VK_SHIFT) & 0x8000)
                break;
            SendMessage(GetParent(hwnd), msg, wParam, lParam);
            return 0;

        case WM_TIMER:
            if (wParam == TIMERID_RENAME)
                BeginRenameSelection(hwnd, dat);
            else if (wParam == TIMERID_PAINT) {
                KillTimer(hwnd, TIMERID_PAINT);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (wParam == TIMERID_SORT) {
                KillTimer(hwnd, TIMERID_SORT);
                dat->bNeedSort = FALSE;
                SortCLC(hwnd, dat, 1);
            }
            else if (wParam == TIMERID_DRAGAUTOSCROLL)
                ScrollTo(hwnd,dat,dat->yScroll+dat->dragAutoScrolling*dat->max_row_height*2,0);
            else if (wParam == TIMERID_INFOTIP) {
                CLCINFOTIP it;
                struct ClcContact *contact;
                int hit;
                RECT clRect;
                POINT ptClientOffset = {0};

                KillTimer(hwnd, wParam);
                GetCursorPos(&it.ptCursor);
                ScreenToClient(hwnd, &it.ptCursor);
                if (it.ptCursor.x != dat->ptInfoTip.x || it.ptCursor.y != dat->ptInfoTip.y)
                    break;
                GetClientRect(hwnd, &clRect);
                it.rcItem.left = 0; it.rcItem.right = clRect.right;
                hit = HitTest(hwnd, dat, it.ptCursor.x, it.ptCursor.y, &contact, NULL, NULL);
                if (hit == -1)
                    break;
                if (contact->type != CLCIT_GROUP && contact->type != CLCIT_CONTACT)
                    break;
                ClientToScreen(hwnd, &it.ptCursor);
                ClientToScreen(hwnd, &ptClientOffset);
                it.isTreeFocused = GetFocus() == hwnd;
                //it.rcItem.top = hit * dat->rowHeight - dat->yScroll;
                //it.rcItem.bottom = it.rcItem.top + dat->rowHeight;

				it.rcItem.top = RowHeights_GetItemTopY(dat,hit) - dat->yScroll;
				it.rcItem.bottom = it.rcItem.top + dat->row_heights[hit];

                OffsetRect(&it.rcItem, ptClientOffset.x, ptClientOffset.y);
                it.isGroup = contact->type == CLCIT_GROUP;
                it.hItem = contact->type == CLCIT_GROUP ? (HANDLE) contact->groupId : contact->hContact;
                it.cbSize = sizeof(it);
                dat->hInfoTipItem = ContactToHItem(contact);
                NotifyEventHooks(hShowInfoTipEvent, 0, (LPARAM) &it);
            }
            else if(wParam == TIMERID_REFRESH)
                InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_MBUTTONDOWN:
        case WM_LBUTTONDOWN:
            {
                struct ClcContact *contact;
                struct ClcGroup *group;
                int hit;
                DWORD hitFlags;

                dat->forcePaint = TRUE;
                
                if ((GetKeyState(VK_SHIFT) & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000) && !dat->bisEmbedded) {
                    dat->isMultiSelect = !dat->isMultiSelect;
                    SetWindowLong(hwnd, GWL_STYLE, dat->isMultiSelect ? GetWindowLong(hwnd, GWL_STYLE) | (CLS_CHECKBOXES | CLS_GROUPCHECKBOXES) : GetWindowLong(hwnd, GWL_STYLE) & ~(CLS_CHECKBOXES | CLS_GROUPCHECKBOXES));
                }
                if (GetFocus() != hwnd)
                    SetFocus(hwnd);
                HideInfoTip(hwnd, dat);
                KillTimer(hwnd, TIMERID_INFOTIP);
                KillTimer(hwnd, TIMERID_RENAME);
                EndRename(hwnd, dat, 1);
                dat->ptDragStart.x = (short) LOWORD(lParam);
                dat->ptDragStart.y = (short) HIWORD(lParam);
                dat->szQuickSearch[0] = 0;
                hit = HitTest(hwnd, dat, (short) LOWORD(lParam), (short) HIWORD(lParam), &contact, &group, &hitFlags);
                if (hit != -1) {
                    if (hit == dat->selection && hitFlags & CLCHT_ONITEMLABEL && dat->exStyle & CLS_EX_EDITLABELS) {
                        SetCapture(hwnd);
                        dat->iDragItem = dat->selection;
                        dat->dragStage = DRAGSTAGE_NOTMOVED | DRAGSTAGEF_MAYBERENAME;
                        dat->dragAutoScrolling = 0;
                        break;
                    }
                }
                if (hit != -1 && contact->type == CLCIT_GROUP)
                    if (hitFlags & CLCHT_ONITEMICON) {
                        struct ClcGroup *selgroup;
                        struct ClcContact *selcontact;
                        dat->selection = GetRowByIndex(dat, dat->selection, &selcontact, &selgroup);
                        SetGroupExpand(hwnd, dat, contact->group, -1);
                        if (dat->selection != -1) {
                            dat->selection = GetRowsPriorTo(&dat->list, selgroup, ((unsigned) selcontact - (unsigned) selgroup->contact) / sizeof(struct ClcContact));
                            if (dat->selection == -1)
                                dat->selection = GetRowsPriorTo(&dat->list, contact->group, -1);
                        }
                        InvalidateRect(hwnd, NULL, FALSE);
                        UpdateWindow(hwnd);
                        break;
                    }
                if (hit != -1 && hitFlags & CLCHT_ONITEMCHECK) {
                    NMCLISTCONTROL nm;
                    contact->flags ^= CONTACTF_CHECKED;
                    if (contact->type == CLCIT_GROUP)
                        SetGroupChildCheckboxes(contact->group, contact->flags & CONTACTF_CHECKED);
                    RecalculateGroupCheckboxes(hwnd, dat);
                    InvalidateRect(hwnd, NULL, FALSE);
                    nm.hdr.code = CLN_CHECKCHANGED;
                    nm.hdr.hwndFrom = hwnd;
                    nm.hdr.idFrom = GetDlgCtrlID(hwnd);
                    nm.flags = 0;
                    nm.hItem = ContactToItemHandle(contact, &nm.flags);
                    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
                }
                if (!(hitFlags & (CLCHT_ONITEMICON | CLCHT_ONITEMLABEL | CLCHT_ONITEMCHECK | CLCHT_ONITEMSPACE))) {
                    NMCLISTCONTROL nm;
                    nm.hdr.code = NM_CLICK;
                    nm.hdr.hwndFrom = hwnd;
                    nm.hdr.idFrom = GetDlgCtrlID(hwnd);
                    nm.flags = 0;
                    if (hit == -1)
                        nm.hItem = NULL;
                    else
                        nm.hItem = ContactToItemHandle(contact, &nm.flags);
                    nm.iColumn = hitFlags & CLCHT_ONITEMEXTRA ? HIBYTE(HIWORD(hitFlags)) : -1;
                    nm.pt = dat->ptDragStart;
                    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
                }
                if (hitFlags & (CLCHT_ONITEMCHECK | CLCHT_ONITEMEXTRA) && !(hitFlags & CLCHT_ONITEMSPACE))
                    break;
                dat->selection = hit;
                InvalidateRect(hwnd, NULL, FALSE);
                if (dat->selection != -1)
                    EnsureVisible(hwnd, dat, hit, 0);
                UpdateWindow(hwnd);
                if (dat->selection != -1 && (contact->type == CLCIT_CONTACT || contact->type == CLCIT_GROUP) && !(hitFlags & (CLCHT_ONITEMEXTRA | CLCHT_ONITEMCHECK))) {
                    SetCapture(hwnd);
                    dat->iDragItem = dat->selection;
                    dat->dragStage = DRAGSTAGE_NOTMOVED;
                    dat->dragAutoScrolling = 0;
                }
                break;
            }

        case WM_MOUSEMOVE:
            if (dat->iDragItem == -1) {
                int iOldHotTrack = dat->iHotTrack;
                if (dat->hwndRenameEdit != NULL)
                    break;
                if (GetKeyState(VK_MENU) & 0x8000 || GetKeyState(VK_F10) & 0x8000)
                    break;
                dat->iHotTrack = HitTest(hwnd, dat, (short) LOWORD(lParam), (short) HIWORD(lParam), NULL, NULL, NULL);
                if (iOldHotTrack != dat->iHotTrack) {
                    if (iOldHotTrack == -1)
                        SetCapture(hwnd);
                    else if (dat->iHotTrack == -1)
                        ReleaseCapture();
                    if (dat->exStyle & CLS_EX_TRACKSELECT) {
                        InvalidateItem(hwnd, dat, iOldHotTrack);
                        InvalidateItem(hwnd, dat, dat->iHotTrack);
                        dat->forcePaint = TRUE;
                        if(g_isConnecting)
                            UpdateWindow(hwnd);
                    }
                    HideInfoTip(hwnd, dat);
                }
                KillTimer(hwnd, TIMERID_INFOTIP);
                if (wParam == 0 && dat->hInfoTipItem == NULL) {
                    dat->ptInfoTip.x = (short) LOWORD(lParam);
                    dat->ptInfoTip.y = (short) HIWORD(lParam);
                    SetTimer(hwnd, TIMERID_INFOTIP, dat->infoTipTimeout, NULL);
                }
                break;
            }
            if ((dat->dragStage & DRAGSTAGEM_STAGE) == DRAGSTAGE_NOTMOVED && !(dat->exStyle & CLS_EX_DISABLEDRAGDROP)) {
                if (abs((short) LOWORD(lParam) - dat->ptDragStart.x) >= GetSystemMetrics(SM_CXDRAG) || abs((short) HIWORD(lParam) - dat->ptDragStart.y) >= GetSystemMetrics(SM_CYDRAG))
                    dat->dragStage = (dat->dragStage & ~DRAGSTAGEM_STAGE) | DRAGSTAGE_ACTIVE;
            }
            if ((dat->dragStage & DRAGSTAGEM_STAGE) == DRAGSTAGE_ACTIVE) {
                HCURSOR hNewCursor;
                RECT clRect;
                POINT pt;
                int target;

                GetClientRect(hwnd, &clRect);
                pt.x = (short) LOWORD(lParam); pt.y = (short) HIWORD(lParam);
                hNewCursor = LoadCursor(NULL, IDC_NO);
                InvalidateRect(hwnd, NULL, FALSE);
                if (dat->dragAutoScrolling) {
                    KillTimer(hwnd, TIMERID_DRAGAUTOSCROLL); dat->dragAutoScrolling = 0;
                }
                target = GetDropTargetInformation(hwnd, dat, pt);
                if (dat->dragStage & DRAGSTAGEF_OUTSIDE && target != DROPTARGET_OUTSIDE) {
                    NMCLISTCONTROL nm;
                    struct ClcContact *contact;
                    GetRowByIndex(dat, dat->iDragItem, &contact, NULL);
                    nm.hdr.code = CLN_DRAGSTOP;
                    nm.hdr.hwndFrom = hwnd;
                    nm.hdr.idFrom = GetDlgCtrlID(hwnd);
                    nm.flags = 0;
                    nm.hItem = ContactToItemHandle(contact, &nm.flags);
                    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
                    dat->dragStage &= ~DRAGSTAGEF_OUTSIDE;
                }
                switch (target) {
                    case DROPTARGET_ONSELF:
                    case DROPTARGET_ONCONTACT:
                        break;
                    case DROPTARGET_ONGROUP:
                        hNewCursor = LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER));
                        break;
                    case DROPTARGET_INSERTION:
                        hNewCursor = LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER));
                        break;
                    case DROPTARGET_OUTSIDE:
                        {
                            NMCLISTCONTROL nm;
                            struct ClcContact *contact;

                            if (pt.x >= 0 && pt.x < clRect.right && ((pt.y< 0 && pt.y> - dat->dragAutoScrollHeight) || (pt.y >= clRect.bottom && pt.y< clRect.bottom + dat->dragAutoScrollHeight))) {
                                if (!dat->dragAutoScrolling) {
                                    if (pt.y < 0)
                                        dat->dragAutoScrolling = -1;
                                    else
                                        dat->dragAutoScrolling = 1;
                                    SetTimer(hwnd, TIMERID_DRAGAUTOSCROLL, dat->scrollTime, NULL);
                                }
                                SendMessage(hwnd, WM_TIMER, TIMERID_DRAGAUTOSCROLL, 0);
                            }

                            dat->dragStage |= DRAGSTAGEF_OUTSIDE;
                            GetRowByIndex(dat, dat->iDragItem, &contact, NULL);
                            nm.hdr.code = CLN_DRAGGING;
                            nm.hdr.hwndFrom = hwnd;
                            nm.hdr.idFrom = GetDlgCtrlID(hwnd);
                            nm.flags = 0;
                            nm.hItem = ContactToItemHandle(contact, &nm.flags);
                            nm.pt = pt;
                            if (SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm))
                                return 0;
                            break;
                        }
                    default:
                        {
                            struct ClcGroup *group;
                            GetRowByIndex(dat, dat->iDragItem, NULL, &group);
                            if (group->parent)
                                hNewCursor = LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER));
                            break;
                        }
                }
                SetCursor(hNewCursor);
            }
            break;

        case WM_LBUTTONUP:
            dat->forcePaint = TRUE;
            if (dat->iDragItem == -1)
                break;
            SetCursor((HCURSOR) GetClassLong(hwnd, GCL_HCURSOR));
            if (dat->exStyle & CLS_EX_TRACKSELECT) {
                dat->iHotTrack = HitTest(hwnd, dat, (short) LOWORD(lParam), (short) HIWORD(lParam), NULL, NULL, NULL);
                if (dat->iHotTrack == -1)
                    ReleaseCapture();
            } else
                ReleaseCapture();
            KillTimer(hwnd, TIMERID_DRAGAUTOSCROLL);
            if (dat->dragStage == (DRAGSTAGE_NOTMOVED | DRAGSTAGEF_MAYBERENAME))
                SetTimer(hwnd, TIMERID_RENAME, GetDoubleClickTime(), NULL);
            else if ((dat->dragStage & DRAGSTAGEM_STAGE) == DRAGSTAGE_ACTIVE) {
                POINT pt;
                int target;

                pt.x = (short) LOWORD(lParam); pt.y = (short) HIWORD(lParam);
                target = GetDropTargetInformation(hwnd, dat, pt);
                switch (target) {
                    case DROPTARGET_ONSELF:
                        break;
                    case DROPTARGET_ONCONTACT:
                        break;
                    case DROPTARGET_ONGROUP:
                        {
                            struct ClcContact *contact;
                            TCHAR *szGroup;
                            GetRowByIndex(dat, dat->selection, &contact, NULL);
                            szGroup = GetGroupNameT(contact->groupId, NULL);
                            GetRowByIndex(dat, dat->iDragItem, &contact, NULL);
                            if (contact->type == CLCIT_CONTACT)     //dropee is a contact
                                DBWriteContactSettingTString(contact->hContact, "CList", "Group", szGroup);
                            else if (contact->type == CLCIT_GROUP) {
            //dropee is a group
                                TCHAR szNewName[120];
                                _sntprintf(szNewName, safe_sizeof(szNewName), _T("%s\\%s"), szGroup, contact->szText);
                                szNewName[safe_sizeof(szNewName) - 1] = 0;
                                RenameGroupT(contact->groupId, szNewName);
                            }
                            break;
                        }
                    case DROPTARGET_INSERTION:
                        {
                            struct ClcContact *contact, *destcontact;
                            struct ClcGroup *destgroup;
                            GetRowByIndex(dat, dat->iDragItem, &contact, NULL);
                            if (GetRowByIndex(dat, dat->iInsertionMark, &destcontact, &destgroup) == -1 || destgroup != contact->group->parent)
                                CallService(MS_CLIST_GROUPMOVEBEFORE, contact->groupId, 0);
                            else {
                                if (destcontact->type == CLCIT_GROUP)
                                    destgroup = destcontact->group;
                                else
                                    destgroup = destgroup;
                                CallService(MS_CLIST_GROUPMOVEBEFORE, contact->groupId, destgroup->groupId);
                            }
                            WindowList_Broadcast(hClcWindowList, CLM_AUTOREBUILD, 0, 0);
                            break;
                        }
                    case DROPTARGET_OUTSIDE:
                        {
                            NMCLISTCONTROL nm;
                            struct ClcContact *contact;
                            GetRowByIndex(dat, dat->iDragItem, &contact, NULL);
                            nm.hdr.code = CLN_DROPPED;
                            nm.hdr.hwndFrom = hwnd;
                            nm.hdr.idFrom = GetDlgCtrlID(hwnd);
                            nm.flags = 0;
                            nm.hItem = ContactToItemHandle(contact, &nm.flags);
                            nm.pt = pt;
                            SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
                            break;
                        }
                    default:
                        {
                            struct ClcGroup *group;
                            struct ClcContact *contact;
                            GetRowByIndex(dat, dat->iDragItem, &contact, &group);
                            if (group->parent) {
            //move to root
                                if (contact->type == CLCIT_CONTACT)     //dropee is a contact
                                    DBDeleteContactSetting(contact->hContact, "CList", "Group");
                                else if (contact->type == CLCIT_GROUP) {
            //dropee is a group
                                    TCHAR szNewName[120];
                                    lstrcpyn(szNewName, contact->szText, safe_sizeof(szNewName));
                                    RenameGroupT(contact->groupId, szNewName);
                                }
                            }
                            break;
                        }
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
            dat->iDragItem = -1;
            dat->iInsertionMark = -1;
            break;

        case WM_LBUTTONDBLCLK:
            {
                struct ClcContact *contact;
                DWORD hitFlags;
                ReleaseCapture();
                dat->iHotTrack = -1;
                HideInfoTip(hwnd, dat);
                KillTimer(hwnd, TIMERID_RENAME);
                KillTimer(hwnd, TIMERID_INFOTIP);
                dat->forcePaint = TRUE;
                dat->szQuickSearch[0] = 0;
                dat->selection = HitTest(hwnd, dat, (short) LOWORD(lParam), (short) HIWORD(lParam), &contact, NULL, &hitFlags);
                if(hitFlags & CLCHT_ONITEMEXTRAEX && hwnd == hwndContactTree && contact != 0) {
                    int column = hitFlags >> 24;
                    if(column-- > 0) {
                        if (contact->type == CLCIT_CONTACT) {
                            if(column == 0) {
                                char buf[4096];
                                DBVARIANT dbv = {0};
                                char *szEmail = NULL;
                                if(!DBGetContactSetting(contact->hContact, "UserInfo", "Mye-mail0", &dbv))
                                    szEmail = dbv.pszVal;
                                else if(!DBGetContactSetting(contact->hContact, contact->proto, "e-mail", &dbv))
                                    szEmail = dbv.pszVal;

                                if (szEmail) {
                                    mir_snprintf(buf, sizeof(buf), "mailto:%s", szEmail);
                                    mir_free(szEmail);
                                    ShellExecuteA(hwnd,"open",buf,NULL,NULL,SW_SHOW);
                                }
                                break;
                            }
                            if(column == 1) {
                                char *homepage = NULL;
                                DBVARIANT dbv = {0};

                                if(!DBGetContactSetting(contact->hContact, "UserInfo", "Homepage", &dbv))
                                    homepage = dbv.pszVal;
                                else if(!DBGetContactSetting(contact->hContact, contact->proto, "Homepage", &dbv))
                                    homepage = dbv.pszVal;
                                if (homepage) {											
                                    ShellExecuteA(hwnd,"open",homepage,NULL,NULL,SW_SHOW);
                                    mir_free(homepage);
                                }
                                break;
                            }
                        }
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
                if (dat->selection != -1)
                    EnsureVisible(hwnd, dat, dat->selection, 0);
                if(hitFlags & CLCHT_ONAVATAR && g_CluiData.bDblClkAvatars) {
                    CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)contact->hContact, 0);
                    break;
                }
                if (!(hitFlags & (CLCHT_ONITEMICON | CLCHT_ONITEMLABEL | CLCHT_ONITEMSPACE)))
                    break;
                UpdateWindow(hwnd);
                DoSelectionDefaultAction(hwnd, dat);
                break;
            }

        case WM_CONTEXTMENU:
            {
                struct ClcContact *contact;
                HMENU hMenu = NULL;
                POINT pt;
                DWORD hitFlags;

                EndRename(hwnd, dat, 1);
                HideInfoTip(hwnd, dat);
                KillTimer(hwnd, TIMERID_RENAME);
                KillTimer(hwnd, TIMERID_INFOTIP);
                if (GetFocus() != hwnd)
                    SetFocus(hwnd);
                dat->iHotTrack = -1;
                dat->szQuickSearch[0] = 0;
                pt.x = (short) LOWORD(lParam);
                pt.y = (short) HIWORD(lParam);
                if (pt.x == -1 && pt.y == -1) {
                    dat->selection = GetRowByIndex(dat, dat->selection, &contact, NULL);
                    if (dat->selection != -1)
                        EnsureVisible(hwnd, dat, dat->selection, 0);
                    pt.x = dat->iconXSpace + 15;
    				pt.y = RowHeights_GetItemTopY(dat, dat->selection) - dat->yScroll + (int)(dat->row_heights[dat->selection]*.7);
                    //pt.y = dat->selection * dat->rowHeight - dat->yScroll + (int) (dat->rowHeight * .7);
                    hitFlags = dat->selection == -1 ? CLCHT_NOWHERE : CLCHT_ONITEMLABEL;
                } else {
                    ScreenToClient(hwnd, &pt);
                    dat->selection = HitTest(hwnd, dat, pt.x, pt.y, &contact, NULL, &hitFlags);
                }
                InvalidateRect(hwnd, NULL, FALSE);
                if (dat->selection != -1)
                    EnsureVisible(hwnd, dat, dat->selection, 0);
                UpdateWindow(hwnd);

                if (dat->selection != -1 && hitFlags & (CLCHT_ONITEMICON | CLCHT_ONITEMCHECK | CLCHT_ONITEMLABEL)) {
                    if (contact->type == CLCIT_GROUP) {
                        //hMenu = GetSubMenu(LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT)), 2);  XXX old code
                        hMenu = (HMENU)CallService(MS_CLIST_MENUBUILDSUBGROUP, (WPARAM)contact->group, 0);
                        ClientToScreen(hwnd,&pt);
                        TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,(HWND)CallService(MS_CLUI_GETHWND,0,0),NULL);
                        return 0;
                        //CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hMenu, LPTDF_UNICODE);    XXX old code
                        CheckMenuItem(hMenu, POPUP_GROUPHIDEOFFLINE, contact->group->hideOffline ? MF_CHECKED : MF_UNCHECKED);
                    } else if (contact->type == CLCIT_CONTACT)
                        hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) contact->hContact, 0);
                } else {
                //call parent for new group/hide offline menu
                    PostMessage(GetParent(hwnd),WM_CONTEXTMENU,wParam,lParam);
                    return 0;
                }
                if (hMenu != NULL) {
                    ClientToScreen(hwnd, &pt);
                    TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hMenu);
                }
                return 0;
            }

        case WM_MEASUREITEM:
            return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
        case WM_DRAWITEM:
            return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);

        case WM_COMMAND:
            {
                int hit;
                struct ClcContact *contact;

                if(LOWORD(wParam) == POPUP_NEWGROUP) {
                    SendMessage(GetParent(hwnd), msg, wParam, lParam);
                    break;
                }
                hit = GetRowByIndex(dat, dat->selection, &contact, NULL);
                if (hit == -1)
                    break;
                if (contact->type == CLCIT_CONTACT)
                    if (CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_CONTACTMENU), (LPARAM) contact->hContact))
                        break;
                switch (LOWORD(wParam)) {
                    case POPUP_NEWSUBGROUP:
                        if (contact->type != CLCIT_GROUP)
                            break;
                        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~CLS_HIDEEMPTYGROUPS);
                        CallService(MS_CLIST_GROUPCREATE, contact->groupId, 0);
                        break;
                    case POPUP_RENAMEGROUP:
                        BeginRenameSelection(hwnd, dat);
                        break;
                    case POPUP_DELETEGROUP:
                        if (contact->type != CLCIT_GROUP)
                            break;
                        CallService(MS_CLIST_GROUPDELETE, contact->groupId, 0);
                        break;
                    case POPUP_GROUPHIDEOFFLINE:
                        if (contact->type != CLCIT_GROUP)
                            break;
                        CallService(MS_CLIST_GROUPSETFLAGS, contact->groupId, MAKELPARAM(contact->group->hideOffline ? 0 : GROUPF_HIDEOFFLINE, GROUPF_HIDEOFFLINE));
                        break;
                }
                break;
            }
        case WM_NCHITTEST:
            {
                LRESULT lr = SendMessage(GetParent(hwnd), WM_NCHITTEST, wParam, lParam);
                if(lr == HTLEFT || lr == HTRIGHT || lr == HTBOTTOM || lr == HTTOP || lr == HTTOPLEFT || lr == HTTOPRIGHT
                   || lr == HTBOTTOMLEFT || lr == HTBOTTOMRIGHT)
                    return HTTRANSPARENT;
                break;
            }
        case WM_DESTROY:
            HideInfoTip(hwnd, dat); 
            {
                int i;
                for (i = 0; i <= FONTID_MAX; i++) {
                    if (!dat->fontInfo[i].changed)
                        DeleteObject(dat->fontInfo[i].hFont);
                }
            }
            if (dat->himlHighlight)
                ImageList_Destroy(dat->himlHighlight);
            if (dat->hwndRenameEdit)
                DestroyWindow(dat->hwndRenameEdit);
            if (!dat->bkChanged && dat->hBmpBackground)
                DeleteObject(dat->hBmpBackground);
            FreeGroup(&dat->list);
   			RowHeights_Free(dat);
            mir_free(dat);
            UnregisterFileDropping(hwnd);
            WindowList_Remove(hClcWindowList, hwnd);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}