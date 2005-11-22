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

static int LastModifiedItem = -1;
static int last_selcount = 0;
static int last_indizes[64];
extern int g_hottrack;
extern struct CluiData g_CluiData;
extern HWND hwndContactList, g_hwndViewModeFrame;
extern HIMAGELIST himlExtraImages;
extern HANDLE hPreBuildStatusMenuEvent, hClcWindowList;

typedef  DWORD  (__stdcall *pfnImgNewDecoder)(void ** ppDecoder); 
typedef DWORD (__stdcall *pfnImgDeleteDecoder)(void * pDecoder);
typedef  DWORD  (__stdcall *pfnImgNewDIBFromFile)(LPVOID /*in*/pDecoder, LPCSTR /*in*/pFileName, LPVOID /*out*/*pImg);
typedef DWORD (__stdcall *pfnImgDeleteDIBSection)(LPVOID /*in*/pImg);
typedef DWORD (__stdcall *pfnImgGetHandle)(LPVOID /*in*/pImg, HBITMAP /*out*/*pBitmap, LPVOID /*out*/*ppDIBBits);

static pfnImgNewDecoder ImgNewDecoder = 0;
static pfnImgDeleteDecoder ImgDeleteDecoder = 0;
static pfnImgNewDIBFromFile ImgNewDIBFromFile = 0;
static pfnImgDeleteDIBSection ImgDeleteDIBSection = 0;
static pfnImgGetHandle ImgGetHandle = 0;

static BOOL g_imgDecoderAvail = FALSE;

StatusItems_t *StatusItems = NULL;
ImageItem *g_ImageItems = NULL;
ImageItem *g_CLUIImageItem = NULL;
HBRUSH g_CLUISkinnedBkColor = 0;
COLORREF g_CLUISkinnedBkColorRGB = 0;

int ID_EXTBK_LAST = ID_EXTBK_LAST_D;

void SetTBSKinned(int mode);
void ReloadThemedOptions();

static StatusItems_t _StatusItems[] = {
    {"Offline", "EXBK_Offline", ID_STATUS_OFFLINE, 
        CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Online", "EXBK_Online", ID_STATUS_ONLINE, 
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Away", "EXBK_Away", ID_STATUS_AWAY, 
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"DND", "EXBK_Dnd", ID_STATUS_DND, 
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"NA", "EXBK_NA", ID_STATUS_NA, 
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Occupied", "EXBK_Occupied", ID_STATUS_OCCUPIED, 
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Free for chat", "EXBK_FFC", ID_STATUS_FREECHAT, 
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Invisible", "EXBK_Invisible", ID_STATUS_INVISIBLE, 
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"On the phone", "EXBK_OTP", ID_STATUS_ONTHEPHONE, 
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Out to lunch", "EXBK_OTL", ID_STATUS_OUTTOLUNCH, 
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"{-}Expanded Group", "EXBK_EXPANDEDGROUPS", ID_EXTBKEXPANDEDGROUP,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Collapsed Group", "EXBK_COLLAPSEDGROUP", ID_EXTBKCOLLAPSEDDGROUP,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"Empty Group", "EXBK_EMPTYGROUPS", ID_EXTBKEMPTYGROUPS,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"{-}First contact of a group", "EXBK_FIRSTITEM", ID_EXTBKFIRSTITEM,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"Single item in group", "EXBK_SINGLEITEM", ID_EXTBKSINGLEITEM,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"Last contact of a group", "EXBK_LASTITEM", ID_EXTBKLASTITEM,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"{-}First contact of NON-group", "EXBK_FIRSTITEM_NG", ID_EXTBKFIRSTITEM_NG,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"Single item in NON-group", "EXBK_SINGLEITEM_NG", ID_EXTBKSINGLEITEM_NG,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"Last contact of NON-group", "EXBK_LASTITEM_NG", ID_EXTBKLASTITEM_NG,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"{-}Even Contact Positions", "EXBK_EVEN_CNTC_POS", ID_EXTBKEVEN_CNTCTPOS,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"Odd Contact Positions", "EXBK_ODD_CNTC_POS", ID_EXTBKODD_CNTCTPOS,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
    }, {"{-}Selection", "EXBK_SELECTION", ID_EXTBKSELECTION,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Hottracked Item", "EXBK_HOTTRACK", ID_EXTBKHOTTRACK,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"{-}Frame titlebars", "EXBK_FRAMETITLE", ID_EXTBKFRAMETITLE,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Event area", "EXBK_EVTAREA", ID_EXTBKEVTAREA,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Status Bar", "EXBK_STATUSBAR", ID_EXTBKSTATUSBAR,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Tool bar", "EXBK_TOOLBAR", ID_EXTBKBUTTONBAR,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"{-}UI Button - pressed", "EXBK_BUTTONSPRESSED", ID_EXTBKBUTTONSPRESSED,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"UI Button - not pressed", "EXBK_BUTTONSNPRESSED", ID_EXTBKBUTTONSNPRESSED,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"UI Button - mouseover", "EXBK_BUTTONSMOUSEOVER", ID_EXTBKBUTTONSMOUSEOVER,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Toolbar button - pressed", "EXBK_TBBUTTONSPRESSED", ID_EXTBKTBBUTTONSPRESSED,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Toolbar button - not pressed", "EXBK_TBBUTTONSNPRESSED", ID_EXTBKTBBUTTONSNPRESSED,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }, {"Toolbar button - mouseover", "EXBK_TBBUTTONMOUSEOVER", ID_EXTBKTBBUTTONMOUSEOVER,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"{-}Status floater", "EXBK_TBBUTTONMOUSEOVER", ID_EXTBKSTATUSFLOATER,
        CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
        CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
        CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
    }
};

ChangedSItems_t ChangedSItems = {0};

BOOL __forceinline GetItemByStatus(int status, StatusItems_t *retitem)
{
    status = (status >= ID_STATUS_OFFLINE && status <= ID_EXTBK_LAST) ? status : ID_STATUS_OFFLINE;     // better check the index...
    *retitem = StatusItems[status - ID_STATUS_OFFLINE];
     if(g_hottrack && status != ID_EXTBKHOTTRACK)        // allow hottracking for ignored items, unless hottrack item itself should be ignored
         retitem->IGNORED = FALSE;
      return TRUE;
}

StatusItems_t *GetProtocolStatusItem(const char *szProto)
{
    int i;

    if(szProto == NULL)
        return NULL;
    
    for(i = ID_EXTBK_LAST_D - ID_STATUS_OFFLINE + 1; i <= ID_EXTBK_LAST - ID_STATUS_OFFLINE; i++) {
        if(!strcmp(StatusItems[i].szName[0] == '{' ? &StatusItems[i].szName[3] : StatusItems[i].szName, szProto))
            return &StatusItems[i];
    }
    return NULL;
}

// fills the struct with the settings in the database
void LoadExtBkSettingsFromDB()
{
    DWORD ret;
    int n;
    char buffer[255];
    int protoCount = 0, i;
    PROTOCOLDESCRIPTOR **protos = 0;

    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&protoCount, (LPARAM)&protos);

    StatusItems = (StatusItems_t *)malloc(sizeof(StatusItems_t) * ((ID_EXTBK_LAST - ID_STATUS_OFFLINE) + protoCount + 2));
    CopyMemory(StatusItems, _StatusItems, sizeof(_StatusItems));

    for(i = 0; i < protoCount; i++) {
        if(protos[i]->type != PROTOTYPE_PROTOCOL)
            continue;
        ID_EXTBK_LAST++;
        CopyMemory(&StatusItems[ID_EXTBK_LAST - ID_STATUS_OFFLINE], &StatusItems[0], sizeof(StatusItems_t));
        mir_snprintf(StatusItems[ID_EXTBK_LAST - ID_STATUS_OFFLINE].szDBname, 30, "EXBK_%s", protos[i]->szName);
        if(i == 0) {
            lstrcpynA(StatusItems[ID_EXTBK_LAST - ID_STATUS_OFFLINE].szName, "{-}", 30);
            strncat(StatusItems[ID_EXTBK_LAST - ID_STATUS_OFFLINE].szName, protos[i]->szName, 30);
        }
        else
            lstrcpynA(StatusItems[ID_EXTBK_LAST - ID_STATUS_OFFLINE].szName, protos[i]->szName, 30);
        StatusItems[ID_EXTBK_LAST - ID_STATUS_OFFLINE].statusID = ID_EXTBK_LAST;
    }
    for (n = 0; n <= ID_EXTBK_LAST - ID_STATUS_OFFLINE; n++) {
        if (StatusItems[n].statusID != ID_EXTBKSEPARATOR) {
            StatusItems[n].imageItem = 0;
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_IGNORE");
            ret = DBGetContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].IGNORED);
            StatusItems[n]. IGNORED = (BYTE) ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_GRADIENT");
            ret = DBGetContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].GRADIENT);            
            StatusItems[n]. GRADIENT = (BYTE) ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_CORNER");
            ret = DBGetContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].CORNER);
            StatusItems[n]. CORNER = (BYTE) ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR");
            ret = DBGetContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].COLOR);
            StatusItems[n]. COLOR = ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR2");
            ret = DBGetContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].COLOR2);
            StatusItems[n]. COLOR2 = ret;           

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR2_TRANSPARENT");
            ret = DBGetContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].COLOR2_TRANSPARENT);
            StatusItems[n]. COLOR2_TRANSPARENT = (BYTE) ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_TEXTCOLOR");
            ret = DBGetContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].TEXTCOLOR);
            StatusItems[n]. TEXTCOLOR = ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_ALPHA");
            ret = DBGetContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].ALPHA);
            StatusItems[n]. ALPHA = ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MRGN_LEFT");
            ret = DBGetContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].MARGIN_LEFT);
            StatusItems[n]. MARGIN_LEFT = ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MRGN_TOP");
            ret = DBGetContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].MARGIN_TOP);
            StatusItems[n]. MARGIN_TOP = ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MRGN_RIGHT");
            ret = DBGetContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].MARGIN_RIGHT);
            StatusItems[n]. MARGIN_RIGHT = ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MRGN_BOTTOM");
            ret = DBGetContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].MARGIN_BOTTOM);
            StatusItems[n]. MARGIN_BOTTOM = ret;

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_BDRSTYLE");
            ret = DBGetContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].BORDERSTYLE);
            StatusItems[n]. BORDERSTYLE = ret;
        }
    }
}

// writes whole struct to the database
void SaveCompleteStructToDB()
{
    int n;
    char buffer[255];

    for (n = 0; n <= ID_EXTBK_LAST - ID_STATUS_OFFLINE; n++) {
        if (StatusItems[n].statusID != ID_EXTBKSEPARATOR) {
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_IGNORE");
            DBWriteContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].IGNORED);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_GRADIENT");
            DBWriteContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].GRADIENT);            

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_CORNER");
            DBWriteContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].CORNER);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR");
            DBWriteContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].COLOR);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR2");
            DBWriteContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].COLOR2);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR2_TRANSPARENT");
            DBWriteContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].COLOR2_TRANSPARENT);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_TEXTCOLOR");
            DBWriteContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].TEXTCOLOR);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_ALPHA");
            DBWriteContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].ALPHA);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MRGN_LEFT");
            DBWriteContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].MARGIN_LEFT);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MRGN_TOP");
            DBWriteContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].MARGIN_TOP);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MRGN_RIGHT");
            DBWriteContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].MARGIN_RIGHT);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MRGN_BOTTOM");
            DBWriteContactSettingByte(NULL, "CLCExt", buffer, StatusItems[n].MARGIN_BOTTOM);

            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_BDRSTYLE");
            DBWriteContactSettingDword(NULL, "CLCExt", buffer, StatusItems[n].BORDERSTYLE);
        }
    }
}

// updates the struct with the changed dlg item
void UpdateStatusStructSettingsFromOptDlg(HWND hwndDlg, int index)
{
    char buf[15];
    ULONG bdrtype;
    
    if (ChangedSItems.bIGNORED)
        StatusItems[index]. IGNORED = IsDlgButtonChecked(hwndDlg, IDC_IGNORE);

    if (ChangedSItems.bGRADIENT) {
        StatusItems[index]. GRADIENT = GRADIENT_NONE;
        if (IsDlgButtonChecked(hwndDlg, IDC_GRADIENT))
            StatusItems[index].GRADIENT |= GRADIENT_ACTIVE;
        if (IsDlgButtonChecked(hwndDlg, IDC_GRADIENT_LR))
            StatusItems[index].GRADIENT |= GRADIENT_LR;
        if (IsDlgButtonChecked(hwndDlg, IDC_GRADIENT_RL))
            StatusItems[index].GRADIENT |= GRADIENT_RL;
        if (IsDlgButtonChecked(hwndDlg, IDC_GRADIENT_TB))
            StatusItems[index].GRADIENT |= GRADIENT_TB;
        if (IsDlgButtonChecked(hwndDlg, IDC_GRADIENT_BT))
            StatusItems[index].GRADIENT |= GRADIENT_BT;
    }
    if (ChangedSItems.bCORNER) {
        StatusItems[index]. CORNER = CORNER_NONE;
        if (IsDlgButtonChecked(hwndDlg, IDC_CORNER))
            StatusItems[index].CORNER |= CORNER_ACTIVE ;
        if (IsDlgButtonChecked(hwndDlg, IDC_CORNER_TL))
            StatusItems[index].CORNER |= CORNER_TL ;
        if (IsDlgButtonChecked(hwndDlg, IDC_CORNER_TR))
            StatusItems[index].CORNER |= CORNER_TR;
        if (IsDlgButtonChecked(hwndDlg, IDC_CORNER_BR))
            StatusItems[index].CORNER |= CORNER_BR;
        if (IsDlgButtonChecked(hwndDlg, IDC_CORNER_BL))
            StatusItems[index].CORNER |= CORNER_BL;
    }

    if (ChangedSItems.bCOLOR)
        StatusItems[index]. COLOR = SendDlgItemMessage(hwndDlg, IDC_BASECOLOUR, CPM_GETCOLOUR, 0, 0);

    if (ChangedSItems.bCOLOR2)
        StatusItems[index]. COLOR2 = SendDlgItemMessage(hwndDlg, IDC_BASECOLOUR2, CPM_GETCOLOUR, 0, 0);

    if (ChangedSItems.bCOLOR2_TRANSPARENT)
        StatusItems[index]. COLOR2_TRANSPARENT = IsDlgButtonChecked(hwndDlg, IDC_COLOR2_TRANSPARENT);

    if (ChangedSItems.bTEXTCOLOR)
        StatusItems[index]. TEXTCOLOR = SendDlgItemMessage(hwndDlg, IDC_TEXTCOLOUR, CPM_GETCOLOUR, 0, 0);

    if (ChangedSItems.bALPHA) {
        GetWindowTextA(GetDlgItem(hwndDlg, IDC_ALPHA), buf, 10);        // can be removed now
        if (lstrlenA(buf) > 0)
            StatusItems[index]. ALPHA = (BYTE) SendDlgItemMessage(hwndDlg, IDC_ALPHASPIN, UDM_GETPOS, 0, 0);
    }

    if (ChangedSItems.bMARGIN_LEFT) {
        GetWindowTextA(GetDlgItem(hwndDlg, IDC_MRGN_LEFT), buf, 10);        
        if (lstrlenA(buf) > 0)
            StatusItems[index]. MARGIN_LEFT = (BYTE) SendDlgItemMessage(hwndDlg, IDC_MRGN_LEFT_SPIN, UDM_GETPOS, 0, 0);
    }

    if (ChangedSItems.bMARGIN_TOP) {
        GetWindowTextA(GetDlgItem(hwndDlg, IDC_MRGN_TOP), buf, 10);     
        if (lstrlenA(buf) > 0)
            StatusItems[index]. MARGIN_TOP = (BYTE) SendDlgItemMessage(hwndDlg, IDC_MRGN_TOP_SPIN, UDM_GETPOS, 0, 0);
    }

    if (ChangedSItems.bMARGIN_RIGHT) {
        GetWindowTextA(GetDlgItem(hwndDlg, IDC_MRGN_RIGHT), buf, 10);       
        if (lstrlenA(buf) > 0)
            StatusItems[index]. MARGIN_RIGHT = (BYTE) SendDlgItemMessage(hwndDlg, IDC_MRGN_RIGHT_SPIN, UDM_GETPOS, 0, 0);
    }

    if (ChangedSItems.bMARGIN_BOTTOM) {
        GetWindowTextA(GetDlgItem(hwndDlg, IDC_MRGN_BOTTOM), buf, 10);      
        if (lstrlenA(buf) > 0)
            StatusItems[index]. MARGIN_BOTTOM = (BYTE) SendDlgItemMessage(hwndDlg, IDC_MRGN_BOTTOM_SPIN, UDM_GETPOS, 0, 0);
    }
    if (ChangedSItems.bBORDERSTYLE) {
        bdrtype = SendDlgItemMessage(hwndDlg, IDC_BORDERTYPE, CB_GETCURSEL, 0, 0);
        if(bdrtype == CB_ERR)
            StatusItems[index].BORDERSTYLE = 0;
        else {
            switch(bdrtype) {
                case 0:
                    StatusItems[index].BORDERSTYLE = 0;
                    break;
                case 1:
                    StatusItems[index].BORDERSTYLE = BDR_RAISEDOUTER;
                    break;
                case 2:
                    StatusItems[index].BORDERSTYLE = BDR_SUNKENINNER;
                    break;
                case 3:
                    StatusItems[index].BORDERSTYLE = EDGE_BUMP;
                    break;
                case 4:
                    StatusItems[index].BORDERSTYLE = EDGE_ETCHED;
                    break;
                default:
                    StatusItems[index].BORDERSTYLE = 0;
                    break;
            }
        }
    }
}

void SaveLatestChanges(HWND hwndDlg)
{
    int n, itemData;
    // process old selection
    if (last_selcount > 0) {
        for (n = 0; n < last_selcount; n++) {
            itemData = SendDlgItemMessage(hwndDlg, IDC_ITEMS, LB_GETITEMDATA, last_indizes[n], 0);
            if (itemData != ID_EXTBKSEPARATOR) {
                UpdateStatusStructSettingsFromOptDlg(hwndDlg, itemData - ID_STATUS_OFFLINE);
            }
        }
    }

    // reset bChange
    ChangedSItems.bALPHA = FALSE;
    ChangedSItems.bGRADIENT = FALSE;
    ChangedSItems.bCORNER = FALSE;
    ChangedSItems.bCOLOR = FALSE;
    ChangedSItems.bCOLOR2 = FALSE;
    ChangedSItems.bCOLOR2_TRANSPARENT = FALSE;
    ChangedSItems.bTEXTCOLOR = FALSE;
    ChangedSItems.bALPHA = FALSE;
    ChangedSItems.bMARGIN_LEFT = FALSE;
    ChangedSItems.bMARGIN_TOP = FALSE;
    ChangedSItems.bMARGIN_RIGHT = FALSE;
    ChangedSItems.bMARGIN_BOTTOM = FALSE;
    ChangedSItems.bIGNORED = FALSE;
    ChangedSItems.bBORDERSTYLE = FALSE;
}

// wenn die listbox ge�ndert wurde
void OnListItemsChange(HWND hwndDlg)
{
    SaveLatestChanges(hwndDlg);

    // set new selection
    last_selcount = SendMessage(GetDlgItem(hwndDlg, IDC_ITEMS), LB_GETSELCOUNT, 0, 0);  
    if (last_selcount > 0) {
        int n, real_index, itemData, first_item;
        StatusItems_t DialogSettingForMultiSel;

    // get selected indizes
        SendMessage(GetDlgItem(hwndDlg, IDC_ITEMS), LB_GETSELITEMS, 64, (LPARAM) last_indizes);

    // initialize with first items value

        first_item = SendDlgItemMessage(hwndDlg, IDC_ITEMS, LB_GETITEMDATA, last_indizes[0], 0) - ID_STATUS_OFFLINE;
        DialogSettingForMultiSel = StatusItems[first_item];
        for (n = 0; n < last_selcount; n++) {
            itemData = SendDlgItemMessage(hwndDlg, IDC_ITEMS, LB_GETITEMDATA, last_indizes[n], 0);
            if (itemData != ID_EXTBKSEPARATOR) {
                real_index = itemData - ID_STATUS_OFFLINE;
                if (StatusItems[real_index].ALPHA != StatusItems[first_item].ALPHA)
                    DialogSettingForMultiSel.ALPHA = -1;
                if (StatusItems[real_index].COLOR != StatusItems[first_item].COLOR)
                    DialogSettingForMultiSel.COLOR = CLCDEFAULT_COLOR;
                if (StatusItems[real_index].COLOR2 != StatusItems[first_item].COLOR2)
                    DialogSettingForMultiSel.COLOR2 = CLCDEFAULT_COLOR2;
                if (StatusItems[real_index].COLOR2_TRANSPARENT != StatusItems[first_item].COLOR2_TRANSPARENT)
                    DialogSettingForMultiSel.COLOR2_TRANSPARENT = CLCDEFAULT_COLOR2_TRANSPARENT;
                if (StatusItems[real_index].TEXTCOLOR != StatusItems[first_item].TEXTCOLOR)
                    DialogSettingForMultiSel.TEXTCOLOR = CLCDEFAULT_TEXTCOLOR;
                if (StatusItems[real_index].CORNER != StatusItems[first_item].CORNER)
                    DialogSettingForMultiSel.CORNER = CLCDEFAULT_CORNER;
                if (StatusItems[real_index].GRADIENT != StatusItems[first_item].GRADIENT)
                    DialogSettingForMultiSel.GRADIENT = CLCDEFAULT_GRADIENT;
                if (StatusItems[real_index].IGNORED != StatusItems[first_item].IGNORED)
                    DialogSettingForMultiSel.IGNORED = CLCDEFAULT_IGNORE;
                if (StatusItems[real_index].MARGIN_BOTTOM != StatusItems[first_item].MARGIN_BOTTOM)
                    DialogSettingForMultiSel.MARGIN_BOTTOM = -1;
                if (StatusItems[real_index].MARGIN_LEFT != StatusItems[first_item].MARGIN_LEFT)
                    DialogSettingForMultiSel.MARGIN_LEFT = -1;
                if (StatusItems[real_index].MARGIN_RIGHT != StatusItems[first_item].MARGIN_RIGHT)
                    DialogSettingForMultiSel.MARGIN_RIGHT = -1;
                if (StatusItems[real_index].MARGIN_TOP != StatusItems[first_item].MARGIN_TOP)
                    DialogSettingForMultiSel.MARGIN_TOP = -1;
                if (StatusItems[real_index].BORDERSTYLE != StatusItems[first_item].BORDERSTYLE)
                    DialogSettingForMultiSel.BORDERSTYLE = -1;
            }
        }

        if (last_selcount == 1 && StatusItems[first_item].statusID == ID_EXTBKSEPARATOR) {
            ChangeControlItems(hwndDlg, 0, 0);
            last_selcount = 0;
        } else
            ChangeControlItems(hwndDlg, 1, 0);
        FillOptionDialogByStatusItem(hwndDlg, &DialogSettingForMultiSel);
    }
}

void SetButtonToSkinned()
{
    int bSkinned = g_CluiData.bSkinnedButtonMode = DBGetContactSettingByte(NULL, "CLCExt", "bskinned", 0);

    SendDlgItemMessage(hwndContactList, IDC_TBMENU, BM_SETSKINNED, 0, bSkinned);
    SendDlgItemMessage(hwndContactList, IDC_TBGLOBALSTATUS, BM_SETSKINNED, 0, bSkinned);
    if(bSkinned) {
        SendDlgItemMessage(hwndContactList, IDC_TBMENU, BUTTONSETASFLATBTN, 0, 0);
        SendDlgItemMessage(hwndContactList, IDC_TBGLOBALSTATUS, BUTTONSETASFLATBTN, 0, 0);
        SendDlgItemMessage(hwndContactList, IDC_TBGLOBALSTATUS, BUTTONSETASFLATBTN + 10, 0, 0);
        SendDlgItemMessage(hwndContactList, IDC_TBMENU, BUTTONSETASFLATBTN + 10, 0, 0);
    }
    else {
        SendDlgItemMessage(hwndContactList, IDC_TBMENU, BUTTONSETASFLATBTN, 0, 1);
        SendDlgItemMessage(hwndContactList, IDC_TBGLOBALSTATUS, BUTTONSETASFLATBTN, 0, 1);
        SendDlgItemMessage(hwndContactList, IDC_TBGLOBALSTATUS, BUTTONSETASFLATBTN + 10, 0, 1);
        SendDlgItemMessage(hwndContactList, IDC_TBMENU, BUTTONSETASFLATBTN + 10, 0, 1);
    }
    SendMessage(g_hwndViewModeFrame, WM_USER + 100, 0, 0);
}

void Reload3dBevelColors()
{
    if(g_CluiData.hPen3DBright)
        DeleteObject(g_CluiData.hPen3DBright);
    if(g_CluiData.hPen3DDark)
        DeleteObject(g_CluiData.hPen3DDark);

    g_CluiData.hPen3DBright = CreatePen(PS_SOLID, 1, DBGetContactSettingDword(NULL, "CLCExt", "3dbright", RGB(224,224,224)));
    g_CluiData.hPen3DDark = CreatePen(PS_SOLID, 1, DBGetContactSettingDword(NULL, "CLCExt", "3ddark", RGB(224,224,224)));

}

// Save Non-StatusItems Settings
void SaveNonStatusItemsSettings(HWND hwndDlg)
{
    BOOL translated;
    
    DBWriteContactSettingByte(NULL, "CLCExt", "EXBK_EqualSelection", IsDlgButtonChecked(hwndDlg, IDC_EQUALSELECTION));
    DBWriteContactSettingByte(NULL, "CLCExt", "EXBK_SelBlend", IsDlgButtonChecked(hwndDlg, IDC_SELBLEND));  
    DBWriteContactSettingByte(NULL, "CLCExt", "EXBK_FillWallpaper", IsDlgButtonChecked(hwndDlg, IDC_FILLWALLPAPER));

    g_CluiData.cornerRadius = GetDlgItemInt(hwndDlg, IDC_CORNERRAD, &translated, FALSE);
    g_CluiData.bApplyIndentToBg = IsDlgButtonChecked(hwndDlg, IDC_APPLYINDENTBG) ? 1 : 0;
    g_CluiData.bUsePerProto = IsDlgButtonChecked(hwndDlg, IDC_USEPERPROTO) ? 1 : 0;
    g_CluiData.bOverridePerStatusColors = IsDlgButtonChecked(hwndDlg, IDC_OVERRIDEPERSTATUSCOLOR) ? 1 : 0;
    DBWriteContactSettingByte(NULL, "CLCExt", "CornerRad", g_CluiData.cornerRadius);
    DBWriteContactSettingByte(NULL, "CLCExt", "applyindentbg", g_CluiData.bApplyIndentToBg);
    DBWriteContactSettingByte(NULL, "CLCExt", "useperproto", g_CluiData.bUsePerProto);
    DBWriteContactSettingByte(NULL, "CLCExt", "override_status", g_CluiData.bOverridePerStatusColors);
    DBWriteContactSettingDword(NULL, "CLCExt", "3dbright", SendDlgItemMessage(hwndDlg, IDC_3DLIGHTCOLOR, CPM_GETCOLOUR, 0, 0));
    DBWriteContactSettingDword(NULL, "CLCExt", "3ddark", SendDlgItemMessage(hwndDlg, IDC_3DDARKCOLOR, CPM_GETCOLOUR, 0, 0));
    DBWriteContactSettingByte(NULL, "CLCExt", "bskinned", IsDlgButtonChecked(hwndDlg, IDC_SETALLBUTTONSKINNED) ? 1 : 0);
    Reload3dBevelColors();
    SetButtonToSkinned();
}

// fills the combobox of the options dlg for the first time
void FillItemList(HWND hwndDlg)
{
    int n, iOff;
    UINT item;
    
    for (n = 0; n <= ID_EXTBK_LAST - ID_STATUS_OFFLINE; n++) {
        iOff = 0;
        if(strstr(StatusItems[n].szName, "{-}")) {
            item = SendDlgItemMessageA(hwndDlg, IDC_ITEMS, LB_ADDSTRING, 0, (LPARAM)"------------------------");
            SendDlgItemMessageA(hwndDlg, IDC_ITEMS, LB_SETITEMDATA, item, ID_EXTBKSEPARATOR);
            iOff = 3;
        }
        item = SendDlgItemMessageA(hwndDlg, IDC_ITEMS, LB_ADDSTRING, 0, (LPARAM)Translate(&StatusItems[n].szName[iOff]));
        SendDlgItemMessageA(hwndDlg, IDC_ITEMS, LB_SETITEMDATA, item, ID_STATUS_OFFLINE + n);
    }
}

void FillOptionDialogByStatusItem(HWND hwndDlg, StatusItems_t *item)
{
    char itoabuf[15];
    DWORD ret;
    int index;
    
    CheckDlgButton(hwndDlg, IDC_IGNORE, (item->IGNORED) ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(hwndDlg, IDC_GRADIENT, (item->GRADIENT & GRADIENT_ACTIVE) ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_LR), item->GRADIENT & GRADIENT_ACTIVE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_RL), item->GRADIENT & GRADIENT_ACTIVE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_TB), item->GRADIENT & GRADIENT_ACTIVE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_BT), item->GRADIENT & GRADIENT_ACTIVE);
    CheckDlgButton(hwndDlg, IDC_GRADIENT_LR, (item->GRADIENT & GRADIENT_LR) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_GRADIENT_RL, (item->GRADIENT & GRADIENT_RL) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_GRADIENT_TB, (item->GRADIENT & GRADIENT_TB) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_GRADIENT_BT, (item->GRADIENT & GRADIENT_BT) ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(hwndDlg, IDC_CORNER, (item->CORNER & CORNER_ACTIVE) ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_TL), item->CORNER & CORNER_ACTIVE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_TR), item->CORNER & CORNER_ACTIVE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_BR), item->CORNER & CORNER_ACTIVE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_BL), item->CORNER & CORNER_ACTIVE);

    CheckDlgButton(hwndDlg, IDC_CORNER_TL, (item->CORNER & CORNER_TL) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_CORNER_TR, (item->CORNER & CORNER_TR) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_CORNER_BR, (item->CORNER & CORNER_BR) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_CORNER_BL, (item->CORNER & CORNER_BL) ? BST_CHECKED : BST_UNCHECKED);

    ret = item->COLOR;
    SendDlgItemMessage(hwndDlg, IDC_BASECOLOUR, CPM_SETDEFAULTCOLOUR, 0, CLCDEFAULT_COLOR);
    SendDlgItemMessage(hwndDlg, IDC_BASECOLOUR, CPM_SETCOLOUR, 0, ret);

    ret = item->COLOR2;
    SendDlgItemMessage(hwndDlg, IDC_BASECOLOUR2, CPM_SETDEFAULTCOLOUR, 0, CLCDEFAULT_COLOR2);
    SendDlgItemMessage(hwndDlg, IDC_BASECOLOUR2, CPM_SETCOLOUR, 0, ret);

    CheckDlgButton(hwndDlg, IDC_COLOR2_TRANSPARENT, (item->COLOR2_TRANSPARENT) ? BST_CHECKED : BST_UNCHECKED);

    ret = item->TEXTCOLOR;
    SendDlgItemMessage(hwndDlg, IDC_TEXTCOLOUR, CPM_SETDEFAULTCOLOUR, 0, CLCDEFAULT_TEXTCOLOR);
    SendDlgItemMessage(hwndDlg, IDC_TEXTCOLOUR, CPM_SETCOLOUR, 0, ret);


    //  TODO: I suppose we don't need to use _itoa here. 
    //  we could probably just set the integer value of the buddy spinner control:

    if (item->ALPHA == -1) {
        SetDlgItemTextA(hwndDlg, IDC_ALPHA, "");
    } else {
        ret = item->ALPHA;
        _itoa(ret, itoabuf, 10);    
        SetDlgItemTextA(hwndDlg, IDC_ALPHA, itoabuf);
    }

    if (item->MARGIN_LEFT == -1)
        SetDlgItemTextA(hwndDlg, IDC_MRGN_LEFT, "");
    else {
        ret = item->MARGIN_LEFT;
        _itoa(ret, itoabuf, 10);
        SetDlgItemTextA(hwndDlg, IDC_MRGN_LEFT, itoabuf);
    }

    if (item->MARGIN_TOP == -1)
        SetDlgItemTextA(hwndDlg, IDC_MRGN_TOP, "");
    else {
        ret = item->MARGIN_TOP;
        _itoa(ret, itoabuf, 10);
        SetDlgItemTextA(hwndDlg, IDC_MRGN_TOP, itoabuf);
    }

    if (item->MARGIN_RIGHT == -1)
        SetDlgItemTextA(hwndDlg, IDC_MRGN_RIGHT, "");
    else {
        ret = item->MARGIN_RIGHT;
        _itoa(ret, itoabuf, 10);
        SetDlgItemTextA(hwndDlg, IDC_MRGN_RIGHT, itoabuf);
    }

    if (item->MARGIN_BOTTOM == -1)
        SetDlgItemTextA(hwndDlg, IDC_MRGN_BOTTOM, "");
    else {
        ret = item->MARGIN_BOTTOM;
        _itoa(ret, itoabuf, 10);
        SetDlgItemTextA(hwndDlg, IDC_MRGN_BOTTOM, itoabuf);
    }
    if(item->BORDERSTYLE == -1)
        SendDlgItemMessage(hwndDlg, IDC_BORDERTYPE, CB_SETCURSEL, 0, 0);
    else {
        switch(item->BORDERSTYLE) {
            case 0:
            case -1:
                index = 0;
                break;
            case BDR_RAISEDOUTER:
                index = 1;
                break;
            case BDR_SUNKENINNER:
                index = 2;
                break;
            case EDGE_BUMP:
                index = 3;
                break;
            case EDGE_ETCHED:
                index = 4;
                break;
        }
        SendDlgItemMessage(hwndDlg, IDC_BORDERTYPE, CB_SETCURSEL, (WPARAM)index, 0);
    }
    ReActiveCombo(hwndDlg);
}
// update dlg with selected item
void FillOptionDialogByCurrentSel(HWND hwndDlg)
{
    int index = SendDlgItemMessage(hwndDlg, IDC_ITEMS, LB_GETCURSEL, 0, 0);
    int itemData = SendDlgItemMessage(hwndDlg, IDC_ITEMS, LB_GETITEMDATA, index, 0);
    if(itemData != ID_EXTBKSEPARATOR) {
        LastModifiedItem = itemData - ID_STATUS_OFFLINE;

        if (CheckItem(itemData - ID_STATUS_OFFLINE, hwndDlg)) {
            FillOptionDialogByStatusItem(hwndDlg, &StatusItems[itemData - ID_STATUS_OFFLINE]);
        }
    }
}

void ReActiveCombo(HWND hwndDlg)
{
    if (IsDlgButtonChecked(hwndDlg, IDC_IGNORE)) {
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_LR), IsDlgButtonChecked(hwndDlg, IDC_GRADIENT));
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_RL), IsDlgButtonChecked(hwndDlg, IDC_GRADIENT));
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_TB), IsDlgButtonChecked(hwndDlg, IDC_GRADIENT));
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_BT), IsDlgButtonChecked(hwndDlg, IDC_GRADIENT));

        EnableWindow(GetDlgItem(hwndDlg, IDC_BASECOLOUR2), !IsDlgButtonChecked(hwndDlg, IDC_COLOR2_TRANSPARENT));
        EnableWindow(GetDlgItem(hwndDlg, IDC_COLOR2LABLE), !IsDlgButtonChecked(hwndDlg, IDC_COLOR2_TRANSPARENT));

        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_TL), IsDlgButtonChecked(hwndDlg, IDC_CORNER));
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_TR), IsDlgButtonChecked(hwndDlg, IDC_CORNER));
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_BR), IsDlgButtonChecked(hwndDlg, IDC_CORNER));
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_BL), IsDlgButtonChecked(hwndDlg, IDC_CORNER));      
        ChangeControlItems(hwndDlg, !IsDlgButtonChecked(hwndDlg, IDC_IGNORE), IDC_IGNORE);
    } else {
        ChangeControlItems(hwndDlg, !IsDlgButtonChecked(hwndDlg, IDC_IGNORE), IDC_IGNORE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_LR), IsDlgButtonChecked(hwndDlg, IDC_GRADIENT));
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_RL), IsDlgButtonChecked(hwndDlg, IDC_GRADIENT));
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_TB), IsDlgButtonChecked(hwndDlg, IDC_GRADIENT));
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_BT), IsDlgButtonChecked(hwndDlg, IDC_GRADIENT));

        EnableWindow(GetDlgItem(hwndDlg, IDC_BASECOLOUR2), !IsDlgButtonChecked(hwndDlg, IDC_COLOR2_TRANSPARENT));
        EnableWindow(GetDlgItem(hwndDlg, IDC_COLOR2LABLE), !IsDlgButtonChecked(hwndDlg, IDC_COLOR2_TRANSPARENT));

        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_TL), IsDlgButtonChecked(hwndDlg, IDC_CORNER));
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_TR), IsDlgButtonChecked(hwndDlg, IDC_CORNER));
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_BR), IsDlgButtonChecked(hwndDlg, IDC_CORNER));
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_BL), IsDlgButtonChecked(hwndDlg, IDC_CORNER));
    }
}

// enabled or disabled the whole status controlitems group (with exceptional control)
void ChangeControlItems(HWND hwndDlg, int status, int except)
{
    if (except != IDC_GRADIENT)
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT), status);
    if (except != IDC_GRADIENT_LR)
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_LR), status);
    if (except != IDC_GRADIENT_RL)
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_RL), status);
    if (except != IDC_GRADIENT_TB)
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_TB), status);
    if (except != IDC_GRADIENT_BT)
        EnableWindow(GetDlgItem(hwndDlg, IDC_GRADIENT_BT), status);
    if (except != IDC_CORNER)
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER), status);
    if (except != IDC_CORNER_TL)
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_TL), status);
    if (except != IDC_CORNER_TR)
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_TR), status);
    if (except != IDC_CORNER_BR)
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_BR), status);
    if (except != IDC_CORNER_BL)
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_BL), status);
    if (except != IDC_CORNER_TL)
        EnableWindow(GetDlgItem(hwndDlg, IDC_CORNER_TL), status);
    if (except != IDC_MARGINLABLE)
        EnableWindow(GetDlgItem(hwndDlg, IDC_MARGINLABLE), status);
    if (except != IDC_MRGN_TOP)
        EnableWindow(GetDlgItem(hwndDlg, IDC_MRGN_TOP), status);
    if (except != IDC_MRGN_RIGHT)
        EnableWindow(GetDlgItem(hwndDlg, IDC_MRGN_RIGHT), status);
    if (except != IDC_MRGN_BOTTOM)
        EnableWindow(GetDlgItem(hwndDlg, IDC_MRGN_BOTTOM), status);
    if (except != IDC_MRGN_LEFT)
        EnableWindow(GetDlgItem(hwndDlg, IDC_MRGN_LEFT), status);
    if (except != IDC_MRGN_TOP_SPIN)
        EnableWindow(GetDlgItem(hwndDlg, IDC_MRGN_TOP_SPIN), status);
    if (except != IDC_MRGN_RIGHT_SPIN)
        EnableWindow(GetDlgItem(hwndDlg, IDC_MRGN_RIGHT_SPIN), status);
    if (except != IDC_MRGN_BOTTOM_SPIN)
        EnableWindow(GetDlgItem(hwndDlg, IDC_MRGN_BOTTOM_SPIN), status);
    if (except != IDC_MRGN_LEFT_SPIN)
        EnableWindow(GetDlgItem(hwndDlg, IDC_MRGN_LEFT_SPIN), status);
    if (except != IDC_BASECOLOUR)
        EnableWindow(GetDlgItem(hwndDlg, IDC_BASECOLOUR), status);
    if (except != IDC_COLORLABLE)
        EnableWindow(GetDlgItem(hwndDlg, IDC_COLORLABLE), status);
    if (except != IDC_BASECOLOUR2)
        EnableWindow(GetDlgItem(hwndDlg, IDC_BASECOLOUR2), status);
    if (except != IDC_COLOR2LABLE)
        EnableWindow(GetDlgItem(hwndDlg, IDC_COLOR2LABLE), status);
    if (except != IDC_COLOR2_TRANSPARENT)
        EnableWindow(GetDlgItem(hwndDlg, IDC_COLOR2_TRANSPARENT), status);
    if (except != IDC_TEXTCOLOUR)
        EnableWindow(GetDlgItem(hwndDlg, IDC_TEXTCOLOUR), status);
    if (except != IDC_TEXTCOLOURLABLE)
        EnableWindow(GetDlgItem(hwndDlg, IDC_TEXTCOLOURLABLE), status);

    if (except != IDC_ALPHA)
        EnableWindow(GetDlgItem(hwndDlg, IDC_ALPHA), status);
    if (except != IDC_ALPHASPIN)
        EnableWindow(GetDlgItem(hwndDlg, IDC_ALPHASPIN), status);
    if (except != IDC_ALPHALABLE)
        EnableWindow(GetDlgItem(hwndDlg, IDC_ALPHALABLE), status);
    if (except != IDC_IGNORE)
        EnableWindow(GetDlgItem(hwndDlg, IDC_IGNORE), status);

    if (except != IDC_BORDERTYPE)
        EnableWindow(GetDlgItem(hwndDlg, IDC_BORDERTYPE), status);
    
}

// enabled all status controls if the selected item is a separator
BOOL CheckItem(int item, HWND hwndDlg)
{
    if (StatusItems[item].statusID == ID_EXTBKSEPARATOR) {
        ChangeControlItems(hwndDlg, 0, 0);
        return FALSE;
    } else {
        ChangeControlItems(hwndDlg, 1, 0);
        return TRUE;
    }
}

void SetChangedStatusItemFlag(WPARAM wParam, HWND hwndDlg)
{
    if (
        // not non status item controls
       LOWORD(wParam) != IDC_EXPORT && LOWORD(wParam) != IDC_IMPORT && LOWORD(wParam) != IDC_EQUALSELECTION && LOWORD(wParam) != IDC_SELBLEND && LOWORD(wParam) != IDC_FILLWALLPAPER && LOWORD(wParam) != IDC_CENTERGROUPNAMES && LOWORD(wParam) != IDC_ITEMS
        // focussed
       && (GetDlgItem(hwndDlg, LOWORD(wParam)) == GetFocus() || HIWORD(wParam) == CPN_COLOURCHANGED)
        // change message
       && (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CPN_COLOURCHANGED)) {
        switch (LOWORD(wParam)) {
            case IDC_IGNORE:
                ChangedSItems.bIGNORED = TRUE; break;
            case IDC_GRADIENT:
                ChangedSItems.bGRADIENT = TRUE; break;
            case IDC_GRADIENT_LR:
                ChangedSItems.bGRADIENT = TRUE;break;
            case IDC_GRADIENT_RL:
                ChangedSItems.bGRADIENT = TRUE; break;
            case IDC_GRADIENT_BT:
                ChangedSItems.bGRADIENT = TRUE; break;
            case IDC_GRADIENT_TB:
                ChangedSItems.bGRADIENT = TRUE; break;

            case IDC_CORNER:
                ChangedSItems.bCORNER = TRUE; break;
            case IDC_CORNER_TL:
                ChangedSItems.bCORNER = TRUE; break;
            case IDC_CORNER_TR:
                ChangedSItems.bCORNER = TRUE; break;
            case IDC_CORNER_BR:
                ChangedSItems.bCORNER = TRUE; break;
            case IDC_CORNER_BL:
                ChangedSItems.bCORNER = TRUE; break;

            case IDC_BASECOLOUR:
                ChangedSItems.bCOLOR = TRUE; break;     
            case IDC_BASECOLOUR2:
                ChangedSItems.bCOLOR2 = TRUE; break;
            case IDC_COLOR2_TRANSPARENT:
                ChangedSItems.bCOLOR2_TRANSPARENT = TRUE; break;
            case IDC_TEXTCOLOUR:
                ChangedSItems.bTEXTCOLOR = TRUE; break;

            case IDC_ALPHA:
                ChangedSItems.bALPHA = TRUE; break;
            case IDC_ALPHASPIN:
                ChangedSItems.bALPHA = TRUE; break;

            case IDC_MRGN_LEFT:
                ChangedSItems.bMARGIN_LEFT = TRUE; break;
            case IDC_MRGN_LEFT_SPIN:
                ChangedSItems.bMARGIN_LEFT = TRUE; break;

            case IDC_MRGN_TOP:
                ChangedSItems.bMARGIN_TOP = TRUE; break;
            case IDC_MRGN_TOP_SPIN:
                ChangedSItems.bMARGIN_TOP = TRUE; break;

            case IDC_MRGN_RIGHT:
                ChangedSItems.bMARGIN_RIGHT = TRUE; break;
            case IDC_MRGN_RIGHT_SPIN:
                ChangedSItems.bMARGIN_RIGHT = TRUE; break;

            case IDC_MRGN_BOTTOM:
                ChangedSItems.bMARGIN_BOTTOM = TRUE; break;
            case IDC_MRGN_BOTTOM_SPIN:
                ChangedSItems.bMARGIN_BOTTOM = TRUE; break;

            case IDC_BORDERTYPE:
                ChangedSItems.bBORDERSTYLE = TRUE; break;
        }
    }
}

BOOL isValidItem(void)
{
    if (StatusItems[LastModifiedItem].statusID == ID_EXTBKSEPARATOR)
        return FALSE;

    return TRUE;
}

/*
 * skin/theme related settings which are exported to/imported from the .ini style .clist file
 */

struct {char *szModule; char *szSetting; unsigned int size; int defaultval;} _tagSettings[] = {
    "CLCExt", "3dbright", 4, RGB(224, 225, 225),
    "CLCExt", "3ddark", 4, RGB(224, 225, 225),
    "CLCExt", "bskinned", 1, 0,
    "CLCExt", "CornerRad", 1, 0,
    "CLCExt", "applyindentbg", 1, 0,
    "CLCExt", "override_status", 1, 0,
    "CLCExt", "useperproto", 1, 0,
    "CLUI", "tb_skinned", 1, 0,
    "CLUI", "sb_skinned", 1, 0,
    "CLC", "RowGap", 1, 0,
    "CLC", "ExIconScale", 1, 0,
    "CLUI", "UseBkSkin", 1, 0,
    "CLUI", "clipborder", 1, 0,
    "CLUIFrames", "GapBetweenFrames", 4, 0,
    "CLC", "BkColour", 4, RGB(224, 224, 224),
    "CLCExt", "EXBK_CenterGroupnames", 1, 0,
    "CLUI", "TBSize", 1, 19,
    "CLC", "BkBmpUse", 2, 0,
    "CLUI", "clmargins", 4, 0,
    // frame stuff

    "WorldTime", "BgColour", 4, 0,
    "WorldTime", "FontCol", 4, 0,
    NULL, NULL, 0, 0
};
void export(char *file)
{
    int n, i;
    char buffer[255];   
    char szSection[255];
    char szKey[255];
    DBVARIANT dbv = {0};
    DWORD data;
    
    data = 3;
    
    WritePrivateProfileStructA("Global", "Version", &data, 4, file);
    for (n = 0; n <= ID_EXTBK_LAST - ID_STATUS_OFFLINE; n++) {
        if (StatusItems[n].statusID != ID_EXTBKSEPARATOR) {
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_ALPHA");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].ALPHA), sizeof(StatusItems[n].ALPHA), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].COLOR), sizeof(StatusItems[n].COLOR), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR2");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].COLOR2), sizeof(StatusItems[n].COLOR2), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR2_TRANSPARENT");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].COLOR2_TRANSPARENT), sizeof(StatusItems[n].COLOR2_TRANSPARENT), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_TEXTCOLOR");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].TEXTCOLOR), sizeof(StatusItems[n].TEXTCOLOR), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_CORNER");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].CORNER), sizeof(StatusItems[n].CORNER), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_GRADIENT");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].GRADIENT), sizeof(StatusItems[n].GRADIENT), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_IGNORED");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].IGNORED), sizeof(StatusItems[n].IGNORED), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MARGIN_BOTTOM");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].MARGIN_BOTTOM), sizeof(StatusItems[n].MARGIN_BOTTOM), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MARGIN_LEFT");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].MARGIN_LEFT), sizeof(StatusItems[n].MARGIN_LEFT), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MARGIN_RIGHT");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].MARGIN_RIGHT), sizeof(StatusItems[n].MARGIN_RIGHT), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MARGIN_TOP");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].MARGIN_TOP), sizeof(StatusItems[n].MARGIN_TOP), file);
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_BORDERSTYLE");
            WritePrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].BORDERSTYLE), sizeof(StatusItems[n].BORDERSTYLE), file);
        }
    }
    for(n = 0; n <= FONTID_MAX; n++) {
        mir_snprintf(szSection, 255, "Font%d", n);
        
        mir_snprintf(szKey, 255, "Font%dName", n);
        if(!DBGetContactSetting(NULL, "CLC", szKey, &dbv)) {
            WritePrivateProfileStringA(szSection, "Name", dbv.pszVal, file);
            mir_free(dbv.pszVal);
        }
        mir_snprintf(szKey, 255, "Font%dSize", n);
        data = (DWORD)DBGetContactSettingByte(NULL, "CLC", szKey, 8);
        WritePrivateProfileStructA(szSection, "Size", &data, 1, file);

        mir_snprintf(szKey, 255, "Font%dSty", n);
        data = (DWORD)DBGetContactSettingByte(NULL, "CLC", szKey, 8);
        WritePrivateProfileStructA(szSection, "Style", &data, 1, file);

        mir_snprintf(szKey, 255, "Font%dSet", n);
        data = (DWORD)DBGetContactSettingByte(NULL, "CLC", szKey, 8);
        WritePrivateProfileStructA(szSection, "Set", &data, 1, file);

        mir_snprintf(szKey, 255, "Font%dCol", n);
        data = DBGetContactSettingDword(NULL, "CLC", szKey, 8);
        WritePrivateProfileStructA(szSection, "Color", &data, 4, file);

        mir_snprintf(szKey, 255, "Font%dFlags", n);
        data = (DWORD)DBGetContactSettingWord(NULL, "CLC", szKey, 8);
        WritePrivateProfileStructA(szSection, "Flags", &data, 2, file);

        mir_snprintf(szKey, 255, "Font%dAs", n);
        data = (DWORD)DBGetContactSettingWord(NULL, "CLC", szKey, 8);
        WritePrivateProfileStructA(szSection, "SameAs", &data, 2, file);
    }
    i = 0;
    while(_tagSettings[i].szModule != NULL) {
        data = 0;
        switch(_tagSettings[i].size) {
            case 1:
                data = (DWORD)DBGetContactSettingByte(NULL, _tagSettings[i].szModule, _tagSettings[i].szSetting, (BYTE)_tagSettings[i].defaultval);
                break;
            case 2:
                data = (DWORD)DBGetContactSettingWord(NULL, _tagSettings[i].szModule, _tagSettings[i].szSetting, (DWORD)_tagSettings[i].defaultval);
                break;
            case 4:
                data = (DWORD)DBGetContactSettingDword(NULL, _tagSettings[i].szModule, _tagSettings[i].szSetting, (DWORD)_tagSettings[i].defaultval);
                break;
        }
        WritePrivateProfileStructA("Global", _tagSettings[i].szSetting, &data, _tagSettings[i].size, file);
        i++;
    }
    if(!DBGetContactSetting(NULL, "CLC", "BkBitmap", &dbv)) {
        WritePrivateProfileStringA("Global", "BkBitmap", dbv.pszVal, file);
        DBFreeVariant(&dbv);
    }
}

DWORD __fastcall HexStringToLong(const char *szSource)
{
    char *stopped;
    COLORREF clr = strtol(szSource, &stopped, 16);
    if(clr == -1)
        return clr;
    return(RGB(GetBValue(clr), GetGValue(clr), GetRValue(clr)));
}

static StatusItems_t default_item = 
    {"{--Contact--}", "", 0, 
     CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
     CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, -1, 
     CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, 
     CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE};


static void ReadItem(StatusItems_t *this_item, char *szItem, char *file)
{
    char buffer[512], def_color[20];
    COLORREF clr;
    
    StatusItems_t *defaults = &default_item;
    GetPrivateProfileStringA(szItem, "BasedOn", "None", buffer, 400, file);

    
    if(strcmp(buffer, "None")) {
        int i;
        
        for(i = 0; i <= ID_EXTBK_LAST - ID_STATUS_OFFLINE; i++) {
            if(!_stricmp(StatusItems[i].szName[0] == '{' ? &StatusItems[i].szName[3] : StatusItems[i].szName, buffer)) {
                defaults = &StatusItems[i];
                break;
            }
        }
    }
    this_item->ALPHA = (int)GetPrivateProfileIntA(szItem, "Alpha", defaults->ALPHA, file);
    this_item->ALPHA = min(this_item->ALPHA, 100);
    
    clr = RGB(GetBValue(defaults->COLOR), GetGValue(defaults->COLOR), GetRValue(defaults->COLOR));
    _snprintf(def_color, 15, "%6.6x", clr);
    GetPrivateProfileStringA(szItem, "Color1", def_color, buffer, 400, file);
    this_item->COLOR = HexStringToLong(buffer);

    clr = RGB(GetBValue(defaults->COLOR2), GetGValue(defaults->COLOR2), GetRValue(defaults->COLOR2));
    _snprintf(def_color, 15, "%6.6x", clr);
    GetPrivateProfileStringA(szItem, "Color2", def_color, buffer, 400, file);
    this_item->COLOR2 = HexStringToLong(buffer);
    
    this_item->COLOR2_TRANSPARENT = (BYTE)GetPrivateProfileIntA(szItem, "COLOR2_TRANSPARENT", defaults->COLOR2_TRANSPARENT, file);

    this_item->CORNER = defaults->CORNER & CORNER_ACTIVE ? defaults->CORNER : 0;
    GetPrivateProfileStringA(szItem, "Corner", "None", buffer, 400, file);
    if(strstr(buffer, "tl"))
        this_item->CORNER |= CORNER_TL;
    if(strstr(buffer, "tr"))
        this_item->CORNER |= CORNER_TR;
    if(strstr(buffer, "bl"))
        this_item->CORNER |= CORNER_BL;
    if(strstr(buffer, "br"))
        this_item->CORNER |= CORNER_BR;
    if(this_item->CORNER)
        this_item->CORNER |= CORNER_ACTIVE;

    this_item->GRADIENT = defaults->GRADIENT & GRADIENT_ACTIVE ?  defaults->GRADIENT : 0;
    GetPrivateProfileStringA(szItem, "Gradient", "None", buffer, 400, file);
    if(strstr(buffer, "left"))
        this_item->GRADIENT = GRADIENT_RL;
    else if(strstr(buffer, "right"))
        this_item->GRADIENT = GRADIENT_LR;
    else if(strstr(buffer, "up"))
        this_item->GRADIENT = GRADIENT_BT;
    else if(strstr(buffer, "down"))
        this_item->GRADIENT = GRADIENT_TB;
    if(this_item->GRADIENT)
        this_item->GRADIENT |= GRADIENT_ACTIVE;

    this_item->MARGIN_LEFT = GetPrivateProfileIntA(szItem, "Left", defaults->MARGIN_LEFT, file);
    this_item->MARGIN_RIGHT = GetPrivateProfileIntA(szItem, "Right", defaults->MARGIN_RIGHT, file);
    this_item->MARGIN_TOP = GetPrivateProfileIntA(szItem, "Top", defaults->MARGIN_TOP, file);
    this_item->MARGIN_BOTTOM = GetPrivateProfileIntA(szItem, "Bottom", defaults->MARGIN_BOTTOM, file);
    this_item->BORDERSTYLE = GetPrivateProfileIntA(szItem, "Borderstyle", defaults->BORDERSTYLE, file);

    GetPrivateProfileStringA(szItem, "Textcolor", "ffffffff", buffer, 400, file);
    this_item->TEXTCOLOR = HexStringToLong(buffer);
}

void IMG_ReadItem(const char *itemname, const char *szFileName)
{
    ImageItem tmpItem = {0}, *newItem = NULL;
    char buffer[512], szItemNr[30];
    char szFinalName[MAX_PATH];
    HDC hdc = GetDC(hwndContactList);
    int i, n;
    BOOL alloced = FALSE;
    char szDrive[MAX_PATH], szPath[MAX_PATH];
    
    GetPrivateProfileStringA(itemname, "Image", "None", buffer, 500, szFileName);
    if(strcmp(buffer, "None")) {
        strncpy(tmpItem.szName, &itemname[1], sizeof(tmpItem.szName));
        tmpItem.szName[sizeof(tmpItem.szName) - 1] = 0;
        _splitpath(szFileName, szDrive, szPath, NULL, NULL);
        mir_snprintf(szFinalName, MAX_PATH, "%s\\%s\\%s", szDrive, szPath, buffer);
        tmpItem.alpha = GetPrivateProfileIntA(itemname, "Alpha", 100, szFileName);
        tmpItem.alpha = min(tmpItem.alpha, 100);
        tmpItem.alpha = (BYTE)((FLOAT)(((FLOAT) tmpItem.alpha) / 100) * 255);
        tmpItem.bf.SourceConstantAlpha = tmpItem.alpha;
        tmpItem.bLeft = GetPrivateProfileIntA(itemname, "Left", 0, szFileName);
        tmpItem.bRight = GetPrivateProfileIntA(itemname, "Right", 0, szFileName);
        tmpItem.bTop = GetPrivateProfileIntA(itemname, "Top", 0, szFileName);
        tmpItem.bBottom = GetPrivateProfileIntA(itemname, "Bottom", 0, szFileName);

        GetPrivateProfileStringA(itemname, "Stretch", "None", buffer, 500, szFileName);
        if(buffer[0] == 'B' || buffer[0] == 'b')
            tmpItem.bStretch = IMAGE_STRETCH_B;
        else if(buffer[0] == 'h' || buffer[0] == 'H')
            tmpItem.bStretch = IMAGE_STRETCH_V;
        else if(buffer[0] == 'w' || buffer[0] == 'W')
            tmpItem.bStretch = IMAGE_STRETCH_H;
        tmpItem.hbm = 0;
        for(n = 0;;n++) {
            mir_snprintf(szItemNr, 30, "Item%d", n);
            GetPrivateProfileStringA(itemname, szItemNr, "None", buffer, 500, szFileName);
            if(!strcmp(buffer, "None"))
                break;
            if(!strcmp(buffer, "CLUI")) {
                IMG_CreateItem(&tmpItem, szFinalName, hdc);
                if(tmpItem.hbm) {
                    COLORREF clr;
                    
                    newItem = malloc(sizeof(ImageItem));
                    ZeroMemory(newItem, sizeof(ImageItem));
                    *newItem = tmpItem;
                    g_CLUIImageItem = newItem;
                    GetPrivateProfileStringA(itemname, "Colorkey", "e5e5e5", buffer, 500, szFileName);
                    clr = HexStringToLong(buffer);
                    g_CluiData.colorkey = clr;
                    DBWriteContactSettingDword(NULL, "CLUI", "ColorKey", clr);
                    if(g_CLUISkinnedBkColor)
                        DeleteObject(g_CLUISkinnedBkColor);
                    g_CLUISkinnedBkColor = CreateSolidBrush(clr);
                    g_CLUISkinnedBkColorRGB = clr;
                }
                continue;
            }
            for(i = 0; i <= ID_EXTBK_LAST - ID_STATUS_OFFLINE; i++) {
                if(!_stricmp(StatusItems[i].szName[0] == '{' ? &StatusItems[i].szName[3] : StatusItems[i].szName, buffer)) {
                    if(!alloced) {
                        IMG_CreateItem(&tmpItem, szFinalName, hdc);
                        if(tmpItem.hbm) {
                            newItem = malloc(sizeof(ImageItem));
                            ZeroMemory(newItem, sizeof(ImageItem));
                            *newItem = tmpItem;
                            StatusItems[i].imageItem = newItem;
                            if(g_ImageItems == NULL)
                                g_ImageItems = newItem;
                            else {
                                ImageItem *pItem = g_ImageItems;

                                while(pItem->nextItem != 0)
                                    pItem = pItem->nextItem;
                                pItem->nextItem = newItem;
                            }
                            alloced = TRUE;
                            //_DebugPopup(0, "successfully assigned %s to %s with %s (handle: %d), p = %d", itemname, buffer, szFinalName, newItem->hbm, newItem);
                        }
                    }
                    else if(newItem != NULL)
                        StatusItems[i].imageItem = newItem;
                }
            }
        }
    }
    ReleaseDC(hwndContactList, hdc);
}

static void PreMultiply(HBITMAP hBitmap, int mode)
{
	BYTE *p = NULL;
	DWORD dwLen;
    int width, height, x, y;
    BITMAP bmp;
    BYTE alpha;
    
	GetObject(hBitmap, sizeof(bmp), &bmp);
    width = bmp.bmWidth;
	height = bmp.bmHeight;
	dwLen = width * height * 4;
	p = (BYTE *)malloc(dwLen);
    if(p) {
        GetBitmapBits(hBitmap, dwLen, p);
        for (y = 0; y < height; ++y) {
            BYTE *px = p + width * 4 * y;

            for (x = 0; x < width; ++x) {
                if(mode) {
                    alpha = px[3];
                    px[0] = px[0] * alpha/255;
                    px[1] = px[1] * alpha/255;
                    px[2] = px[2] * alpha/255;
                }
                else
                    px[3] = 255;
                px += 4;
            }
        }
        dwLen = SetBitmapBits(hBitmap, dwLen, p);
        free(p);
    }
}

static HBITMAP LoadPNG(const char *szFilename, ImageItem *item)
{
    LPVOID imgDecoder = NULL;
    LPVOID pImg = NULL;
    HBITMAP hBitmap = 0;
    LPVOID pBitmapBits = NULL;
    LPVOID m_pImgDecoder = NULL;

    if(!g_imgDecoderAvail)
        return 0;
    
    ImgNewDecoder(&m_pImgDecoder);
    if (!ImgNewDIBFromFile(m_pImgDecoder, (char *)szFilename, &pImg))
        ImgGetHandle(pImg, &hBitmap, (LPVOID *)&pBitmapBits);
    ImgDeleteDecoder(m_pImgDecoder);
    if(hBitmap == 0)
        return 0;
    item->lpDIBSection = pImg;
    return hBitmap;
}

static void IMG_CreateItem(ImageItem *item, const char *fileName, HDC hdc)
{
    HBITMAP hbm = LoadPNG(fileName, item);
    BITMAP bm;

    if(hbm) {
        item->hbm = hbm;
        item->bf.BlendFlags = 0;
        item->bf.BlendOp = AC_SRC_OVER;
        item->bf.AlphaFormat = 0;
        
        GetObject(hbm, sizeof(bm), &bm);
        if(bm.bmBitsPixel == 32) {
            PreMultiply(hbm, 1);
            item->dwFlags |= IMAGE_PERPIXEL_ALPHA;
            item->bf.AlphaFormat = AC_SRC_ALPHA;
        }
        item->width = bm.bmWidth;
        item->height = bm.bmHeight;
        item->inner_height = item->height - item->bTop - item->bBottom;
        item->inner_width = item->width - item->bLeft - item->bRight;
        if(item->bTop && item->bBottom && item->bLeft && item->bRight) {
            item->dwFlags |= IMAGE_FLAG_DIVIDED;
            if(item->inner_height <= 0 || item->inner_width <= 0) {
                DeleteObject(hbm);
                item->hbm = 0;
                return;
            }
        }
        item->hdc = CreateCompatibleDC(hdc);
        item->hbmOld = SelectObject(item->hdc, item->hbm);
    }
}

static void IMG_DeleteItem(ImageItem *item)
{
    SelectObject(item->hdc, item->hbmOld);
    DeleteObject(item->hbm);
    DeleteDC(item->hdc);
    if(item->lpDIBSection && ImgDeleteDIBSection)
        ImgDeleteDIBSection(item->lpDIBSection);
}

void IMG_LoadItems()
{
    char *szSections = NULL;
    char *p;
    DBVARIANT dbv;
    char szFileName[MAX_PATH];
    HANDLE hFile;
    ImageItem *pItem = g_ImageItems, *pNextItem;
    int i;
    
    if(DBGetContactSetting(NULL, "CLC", "ContactSkins", &dbv))
        return;

    CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szFileName);
    DBFreeVariant(&dbv);
    
    if((hFile = CreateFileA(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        return;

    CloseHandle(hFile);
    
    while(pItem) {
        IMG_DeleteItem(pItem);
        pNextItem = pItem->nextItem;
        free(pItem);
        pItem = pNextItem;
    }
    g_ImageItems = NULL;

    if(g_CLUIImageItem) {
        IMG_DeleteItem(g_CLUIImageItem);
        free(g_CLUIImageItem);
    }
    g_CLUIImageItem = NULL;
    
    for(i = 0; i <= ID_EXTBK_LAST - ID_STATUS_OFFLINE; i++)
        StatusItems[i].imageItem = NULL;
    
    szSections = malloc(3002);
    ZeroMemory(szSections, 3002);
    p = szSections;
    GetPrivateProfileSectionNamesA(szSections, 3000, szFileName);
    
    szSections[3001] = szSections[3000] = 0;
    p = szSections;
    while(lstrlenA(p) > 1) {
        if(p[0] == '$')
            IMG_ReadItem(p, szFileName);
        p += (lstrlenA(p) + 1);
    }
    free(szSections);
}

void LoadPerContactSkins(char *file)
{
    char *p, *szProto, *uid, szItem[100];
    char *szSections = malloc(3002);
    StatusItems_t *items = NULL, *this_item;
    HANDLE hContact;
    int i = 1;
    
    ReadItem(&default_item, "%Default", file);
    ZeroMemory(szSections, 3000);
    p = szSections;
    GetPrivateProfileSectionNamesA(szSections, 3000, file);
    szSections[3001] = szSections[3000] = 0;
    p = szSections;
    while(lstrlenA(p) > 1) {
        if(p[0] == '%') {
            p += (lstrlenA(p) + 1);
            continue;
        }
        items = realloc(items, i * sizeof(StatusItems_t));
        ZeroMemory(&items[i - 1], sizeof(StatusItems_t));
        this_item = &items[i - 1];
        GetPrivateProfileStringA(p, "Proto", "", this_item->szName, 40, file);
        this_item->szName[39] = 0;
        GetPrivateProfileStringA(p, "UIN", "", this_item->szDBname, 40, file);
        this_item->szDBname[39] = 0;
        this_item->IGNORED = 0;
        GetPrivateProfileStringA(p, "Item", "", szItem, 100, file);
        szItem[99] = 0;
        //_DebugPopup(0, "Section: %s -> %s", p, szItem);
        ReadItem(this_item, szItem, file);
        p += (lstrlenA(p) + 1);
        i++;
    }

    if(items) {
        hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
        while(hContact) {
            char UIN[40];
            int j;

            szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
            if(szProto) {
                uid = (char *)CallProtoService(szProto, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
                if ((int) uid != CALLSERVICE_NOTFOUND && uid != NULL) {
                    DBVARIANT dbv = {0};

                    DBGetContactSetting(hContact, szProto, uid, &dbv);
                    switch(dbv.type) {
                        case DBVT_DWORD:
                            mir_snprintf(UIN, 40, "%d", dbv.dVal);
                            break;
                        case DBVT_ASCIIZ:
                            mir_snprintf(UIN, 40, "%s", dbv.pszVal);
                            DBFreeVariant(&dbv);
                            break;
                        default:
                            UIN[0] = 0;
                            break;
                    }
                    for(j = 0; j < i - 1; j++) {
                        if(!strcmp(szProto, items[j].szName) && !strcmp(UIN, items[j].szDBname) 
                           && lstrlenA(szProto) == lstrlenA(items[j].szName) && lstrlenA(UIN) == lstrlenA(items[j].szDBname)) {
                            
                            //_DebugPopup(hContact, "Found: %s, %s", szProto, UIN);
                            DBWriteContactSettingDword(hContact, "EXTBK", "TEXT", items[j].TEXTCOLOR);
                            DBWriteContactSettingDword(hContact, "EXTBK", "COLOR1", items[j].COLOR);
                            DBWriteContactSettingDword(hContact, "EXTBK", "COLOR2", items[j].COLOR2);
                            DBWriteContactSettingByte(hContact, "EXTBK", "ALPHA", items[j].ALPHA);

                            DBWriteContactSettingByte(hContact, "EXTBK", "LEFT", (BYTE)items[j].MARGIN_LEFT);
                            DBWriteContactSettingByte(hContact, "EXTBK", "RIGHT", (BYTE)items[j].MARGIN_RIGHT);
                            DBWriteContactSettingByte(hContact, "EXTBK", "TOP", (BYTE)items[j].MARGIN_TOP);
                            DBWriteContactSettingByte(hContact, "EXTBK", "BOTTOM", (BYTE)items[j].MARGIN_BOTTOM);

                            DBWriteContactSettingByte(hContact, "EXTBK", "TRANS", items[j].COLOR2_TRANSPARENT);
                            DBWriteContactSettingDword(hContact, "EXTBK", "BDR", items[j].BORDERSTYLE);
                            
                            DBWriteContactSettingByte(hContact, "EXTBK", "CORNER", items[j].CORNER);
                            DBWriteContactSettingByte(hContact, "EXTBK", "GRAD", items[j].GRADIENT);
                            DBWriteContactSettingByte(hContact, "EXTBK", "TRANS", items[j].COLOR2_TRANSPARENT);

                            DBWriteContactSettingByte(hContact, "EXTBK", "VALID", 1);
                            break;
                        }
                    }
                    if(j == i - 1) {            // disable the db copy if it has been disabled in the skin .ini file
                        if(DBGetContactSettingByte(hContact, "EXTBK", "VALID", 0))
                            DBWriteContactSettingByte(hContact, "EXTBK", "VALID", 0);
                    }
                }
            }
            hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
        }
    }
    free(szSections);
    free(items);
}

void import(char *file, HWND hwndDlg)
{
    int n, i;
    char buffer[255];
    char szKey[255], szSection[255];
    DWORD data, version = 0;
    int oldexIconScale = g_CluiData.exIconScale;

    for (n = 0; n <= ID_EXTBK_LAST - ID_STATUS_OFFLINE; n++) {
        if (StatusItems[n].statusID != ID_EXTBKSEPARATOR) {
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_ALPHA");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].ALPHA), sizeof(StatusItems[n].ALPHA), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].COLOR), sizeof(StatusItems[n].COLOR), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR2");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].COLOR2), sizeof(StatusItems[n].COLOR2), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_COLOR2_TRANSPARENT");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].COLOR2_TRANSPARENT), sizeof(StatusItems[n].COLOR2_TRANSPARENT), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_TEXTCOLOR");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].TEXTCOLOR), sizeof(StatusItems[n].TEXTCOLOR), file);
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_CORNER");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].CORNER), sizeof(StatusItems[n].CORNER), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_GRADIENT");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].GRADIENT), sizeof(StatusItems[n].GRADIENT), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_IGNORED");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].IGNORED), sizeof(StatusItems[n].IGNORED), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MARGIN_BOTTOM");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].MARGIN_BOTTOM), sizeof(StatusItems[n].MARGIN_BOTTOM), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MARGIN_LEFT");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].MARGIN_LEFT), sizeof(StatusItems[n].MARGIN_LEFT), file); 
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MARGIN_RIGHT");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].MARGIN_RIGHT), sizeof(StatusItems[n].MARGIN_RIGHT), file);   
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_MARGIN_TOP");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].MARGIN_TOP), sizeof(StatusItems[n].MARGIN_TOP), file);
            lstrcpyA(buffer, StatusItems[n].szDBname); lstrcatA(buffer, "_BORDERSTYLE");
            GetPrivateProfileStructA("ExtBKSettings", buffer, &(StatusItems[n].BORDERSTYLE), sizeof(StatusItems[n].BORDERSTYLE), file);
        }
    }

    data = 0;
    GetPrivateProfileStructA("Global", "Version", &version, 4, file);
    if(version >= 2) {
        for(n = 0; n <= FONTID_MAX; n++) {
            mir_snprintf(szSection, 255, "Font%d", n);

            mir_snprintf(szKey, 255, "Font%dName", n);
            GetPrivateProfileStringA(szSection, "Name", "Arial", buffer, sizeof(buffer), file);
            DBWriteContactSettingString(NULL, "CLC", szKey, buffer);

            mir_snprintf(szKey, 255, "Font%dSize", n);
            data = 0;
            GetPrivateProfileStructA(szSection, "Size", &data, 1, file);
            DBWriteContactSettingByte(NULL, "CLC", szKey, (BYTE)data);

            mir_snprintf(szKey, 255, "Font%dSty", n);
            data = 0;
            GetPrivateProfileStructA(szSection, "Style", &data, 1, file);
            DBWriteContactSettingByte(NULL, "CLC", szKey, (BYTE)data);

            mir_snprintf(szKey, 255, "Font%dSet", n);
            data = 0;
            GetPrivateProfileStructA(szSection, "Set", &data, 1, file);
            DBWriteContactSettingByte(NULL, "CLC", szKey, (BYTE)data);

            mir_snprintf(szKey, 255, "Font%dCol", n);
            data = 0;
            GetPrivateProfileStructA(szSection, "Color", &data, 4, file);
            DBWriteContactSettingDword(NULL, "CLC", szKey, data);

            mir_snprintf(szKey, 255, "Font%dFlags", n);
            data = 0;
            GetPrivateProfileStructA(szSection, "Flags", &data, 2, file);
            DBWriteContactSettingWord(NULL, "CLC", szKey, (WORD)data);

            mir_snprintf(szKey, 255, "Font%dAs", n);
            data = 0;
            GetPrivateProfileStructA(szSection, "SameAs", &data, 2, file);
            DBWriteContactSettingWord(NULL, "CLC", szKey, (WORD)data);
        }
    }
    i = 0;
    if(version >= 3) {
        char szString[MAX_PATH];
        szString[0] = 0;
        
        while(_tagSettings[i].szModule != NULL) {
            data = 0;
            GetPrivateProfileStructA("Global", _tagSettings[i].szSetting, &data, _tagSettings[i].size, file);
            switch(_tagSettings[i].size) {
                case 1:
                    DBWriteContactSettingByte(NULL, _tagSettings[i].szModule, _tagSettings[i].szSetting, (BYTE)data);
                    break;
                case 4:
                    DBWriteContactSettingDword(NULL, _tagSettings[i].szModule, _tagSettings[i].szSetting, data);
                    break;
                case 2:
                    DBWriteContactSettingWord(NULL, _tagSettings[i].szModule, _tagSettings[i].szSetting, (WORD)data);
                    break;
            }
            i++;
        }
        GetPrivateProfileStringA("Global", "BkBitmap", "", szString, MAX_PATH, file);
        if(lstrlenA(szString) > 0)
            DBWriteContactSettingString(NULL, "CLC", "BkBitmap", szString);
        GetPrivateProfileStringA("Advanced Skin", "File", "", szString, MAX_PATH, file);
        if(lstrlenA(szString) > 0) {
            char szDrive[MAX_PATH], szDir[MAX_PATH], szAdvancedSkinFile[MAX_PATH];
            HANDLE hFile;
            
            _splitpath(file, szDrive, szDir, NULL, NULL);
            mir_snprintf(szAdvancedSkinFile, MAX_PATH, "%s\\%s\\%s", szDrive, szDir, szString);
            if((hFile = CreateFileA(szAdvancedSkinFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
            }
        }
    }
    Reload3dBevelColors();
    ReloadThemedOptions();
    SetTBSKinned(g_CluiData.bSkinnedToolbar);
    // refresh
    FillOptionDialogByCurrentSel(hwndDlg);
    ClcOptionsChanged();
    ConfigureCLUIGeometry();
    SendMessage(hwndContactList, WM_SIZE, 0, 0);
    RedrawWindow(hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);   
    if(oldexIconScale != g_CluiData.exIconScale) {
        ImageList_SetIconSize(himlExtraImages, g_CluiData.exIconScale, g_CluiData.exIconScale);
        if(g_CluiData.IcoLib_Avail)
            IcoLibReloadIcons();
        else {
            CLN_LoadAllIcons(0);
            NotifyEventHooks(hPreBuildStatusMenuEvent, 0, 0);
            ReloadExtraIcons();
        }
        WindowList_Broadcast(hClcWindowList, CLM_AUTOREBUILD, 0, 0);
    }
}

void IMG_InitDecoder()
{
    HMODULE hModule;
    
    if((hModule = LoadLibraryA("imgdecoder.dll")) == 0) {
        if((hModule = LoadLibraryA("plugins\\imgdecoder.dll")) != 0)
            g_imgDecoderAvail = TRUE;
    }
    else
        g_imgDecoderAvail = TRUE;

    if(hModule) {
        ImgNewDecoder = (pfnImgNewDecoder )GetProcAddress(hModule, "ImgNewDecoder");
        ImgDeleteDecoder=(pfnImgDeleteDecoder )GetProcAddress(hModule, "ImgDeleteDecoder");
        ImgNewDIBFromFile=(pfnImgNewDIBFromFile)GetProcAddress(hModule, "ImgNewDIBFromFile");
        ImgDeleteDIBSection=(pfnImgDeleteDIBSection)GetProcAddress(hModule, "ImgDeleteDIBSection");
        ImgGetHandle=(pfnImgGetHandle)GetProcAddress(hModule, "ImgGetHandle");
    }
}
