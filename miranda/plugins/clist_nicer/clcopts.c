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
#include "coolsb/coolscroll.h"
#include <uxtheme.h>

#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4

static BOOL CALLBACK DlgProcClcMainOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcClcBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK DlgProcClcTextOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcViewModesSetup(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcFloatingContacts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK OptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcGenOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcHotkeyOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL (WINAPI *MyEnableThemeDialogTexture)(HANDLE, DWORD);
extern void ReloadExtraIcons( void );
       
extern struct CluiData g_CluiData;
extern int    g_nextExtraCacheEntry;
extern struct ExtraCache *g_ExtraCache;
extern HIMAGELIST himlExtraImages;
extern DWORD g_gdiplusToken;
extern pfnDrawAlpha pDrawAlpha;

struct CheckBoxToStyleEx_t {
    int id;
    DWORD flag;
    int not;
} static const checkBoxToStyleEx[] = {
    {IDC_DISABLEDRAGDROP,CLS_EX_DISABLEDRAGDROP,0}, {IDC_NOTEDITLABELS,CLS_EX_EDITLABELS,1}, 
    {IDC_SHOWSELALWAYS,CLS_EX_SHOWSELALWAYS,0}, {IDC_TRACKSELECT,CLS_EX_TRACKSELECT,0}, 
    {IDC_DIVIDERONOFF,CLS_EX_DIVIDERONOFF,0}, {IDC_NOTNOTRANSLUCENTSEL,CLS_EX_NOTRANSLUCENTSEL,1}, 
    {IDC_NOTNOSMOOTHSCROLLING,CLS_EX_NOSMOOTHSCROLLING,1}
};

struct CheckBoxToGroupStyleEx_t {
    int id;
    DWORD flag;
    int not;
} static const checkBoxToGroupStyleEx[] = {
    {IDC_SHOWGROUPCOUNTS,CLS_EX_SHOWGROUPCOUNTS,0}, {IDC_HIDECOUNTSWHENEMPTY,CLS_EX_HIDECOUNTSWHENEMPTY,0},
    {IDC_LINEWITHGROUPS,CLS_EX_LINEWITHGROUPS,0}, {IDC_QUICKSEARCHVISONLY,CLS_EX_QUICKSEARCHVISONLY,0}, 
    {IDC_SORTGROUPSALPHA,CLS_EX_SORTGROUPSALPHA,0}
};

struct CheckBoxValues_t {
    DWORD style;
    TCHAR *szDescr;
};

static const struct CheckBoxValues_t greyoutValues[] = {
    {GREYF_UNFOCUS,_T("Not focused")}, {MODEF_OFFLINE,_T("Offline")}, {PF2_ONLINE,_T("Online")}, {PF2_SHORTAWAY,_T("Away")}, {PF2_LONGAWAY,_T("NA")}, {PF2_LIGHTDND,_T("Occupied")}, {PF2_HEAVYDND,_T("DND")}, {PF2_FREECHAT,_T("Free for chat")}, {PF2_INVISIBLE,_T("Invisible")}, {PF2_OUTTOLUNCH,_T("Out to lunch")}, {PF2_ONTHEPHONE,_T("On the phone")}
};
static const struct CheckBoxValues_t offlineValues[] = {
    {MODEF_OFFLINE,_T("Offline")}, {PF2_ONLINE,_T("Online")}, {PF2_SHORTAWAY,_T("Away")}, {PF2_LONGAWAY,_T("NA")}, {PF2_LIGHTDND,_T("Occupied")}, {PF2_HEAVYDND,_T("DND")}, {PF2_FREECHAT,_T("Free for chat")}, {PF2_INVISIBLE,_T("Invisible")}, {PF2_OUTTOLUNCH,_T("Out to lunch")}, {PF2_ONTHEPHONE,_T("On the phone")}
};

static UINT sortCtrlIDs[] = {IDC_SORTPRIMARY, IDC_SORTTHEN, IDC_SORTFINALLY, 0};

static void FillCheckBoxTree(HWND hwndTree, const struct CheckBoxValues_t *values, int nValues, DWORD style)
{
	TVINSERTSTRUCT tvis;
	int i;

	tvis.hParent = NULL;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_STATE;
	for (i = 0; i < nValues; i++) {
		tvis.item.lParam = values[i].style;
		tvis.item.pszText = TranslateTS(values[i].szDescr);
		tvis.item.stateMask = TVIS_STATEIMAGEMASK;
		tvis.item.state = INDEXTOSTATEIMAGEMASK((style & tvis.item.lParam) != 0 ? 2 : 1);
		TreeView_InsertItem(hwndTree, &tvis);
	}
}

static DWORD MakeCheckBoxTreeFlags(HWND hwndTree)
{
    DWORD flags = 0;
    TVITEM tvi;

    tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE;
    tvi.hItem = TreeView_GetRoot(hwndTree);
    while (tvi.hItem) {
        TreeView_GetItem(hwndTree, &tvi);
        if (((tvi.state & TVIS_STATEIMAGEMASK) >> 12 == 2))
            flags |= tvi.lParam;
        tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
    }
    return flags;
}

/*
 * load current values into the given profile
 */

static void DSP_LoadFromDefaults(DISPLAYPROFILE *p)
{
    p->dwExtraImageMask = g_CluiData.dwExtraImageMask;
    p->exIconScale = g_CluiData.exIconScale;
    p->bCenterStatusIcons = g_CluiData.bCenterStatusIcons;
    p->dwFlags = g_CluiData.dwFlags;
    p->bDimIdle = DBGetContactSettingByte(NULL, "CLC", "ShowIdle", CLCDEFAULT_SHOWIDLE);
    p->avatarBorder = g_CluiData.avatarBorder;
    p->avatarSize = g_CluiData.avatarSize;
    p->avatarRadius = g_CluiData.avatarRadius;
    p->dualRowMode = g_CluiData.dualRowMode;
    p->bNoOfflineAvatars = g_CluiData.bNoOfflineAvatars;
    p->bShowLocalTime = g_CluiData.bShowLocalTime;
    p->bShowLocalTimeSelective = g_CluiData.bShowLocalTimeSelective;
    p->clcExStyle = DBGetContactSettingDword(NULL, "CLC", "ExStyle", pcli->pfnGetDefaultExStyle());
    p->clcOfflineModes = DBGetContactSettingDword(NULL, "CLC", "OfflineModes", CLCDEFAULT_OFFLINEMODES);
    p->bDontSeparateOffline = g_CluiData.bDontSeparateOffline;
    p->sortOrder[0] = g_CluiData.sortOrder[0];
    p->sortOrder[1] = g_CluiData.sortOrder[1];
    p->sortOrder[2] = g_CluiData.sortOrder[2];
    p->bUseDCMirroring = g_CluiData.bUseDCMirroring;
    p->bCenterGroupNames = DBGetContactSettingByte(NULL, "CLCExt", "EXBK_CenterGroupnames", 0);
    p->bGroupAlign = g_CluiData.bGroupAlign;
    p->avatarPadding = g_CluiData.avatarPadding;

    p->bLeftMargin = DBGetContactSettingByte(NULL, "CLC", "LeftMargin", CLCDEFAULT_LEFTMARGIN);
    p->bRightMargin =  DBGetContactSettingByte(NULL, "CLC", "RightMargin", CLCDEFAULT_LEFTMARGIN);
    p->bRowSpacing = g_CluiData.bRowSpacing;
    p->bGroupIndent = DBGetContactSettingByte(NULL, "CLC", "GroupIndent", CLCDEFAULT_GROUPINDENT);
    p->bRowHeight = DBGetContactSettingByte(NULL, "CLC", "RowHeight", CLCDEFAULT_ROWHEIGHT);
    p->bGroupRowHeight = DBGetContactSettingByte(NULL, "CLC", "GRowHeight", CLCDEFAULT_ROWHEIGHT);
}

/*
 * apply a display profile
 */

void DSP_Apply(DISPLAYPROFILE *p)
{
    int   oldexIconScale = g_CluiData.exIconScale;
    DWORD oldMask = g_CluiData.dwExtraImageMask;
    int   i;
    DWORD exStyle;

    /*
     * icons page
     */
    g_CluiData.dwFlags &= ~(CLUI_FRAME_STATUSICONS | CLUI_SHOWVISI | CLUI_USEMETAICONS | CLUI_FRAME_OVERLAYICONS | CLUI_FRAME_SELECTIVEICONS);
    g_CluiData.dwExtraImageMask = p->dwExtraImageMask;
    g_CluiData.exIconScale = p->exIconScale;
    g_CluiData.bCenterStatusIcons = p->bCenterStatusIcons;

    DBWriteContactSettingDword(NULL, "CLUI", "ximgmask", g_CluiData.dwExtraImageMask);
    DBWriteContactSettingByte(NULL, "CLC", "ExIconScale", (BYTE)g_CluiData.exIconScale);
    DBWriteContactSettingByte(NULL, "CLC", "si_centered", (BYTE)g_CluiData.bCenterStatusIcons);
    DBWriteContactSettingByte(NULL, "CLC", "ShowIdle", (BYTE)p->bDimIdle);

    /*
     * advanced (avatars & 2nd row)
     */

    g_CluiData.dwFlags &= ~(CLUI_FRAME_AVATARSLEFT | CLUI_FRAME_AVATARSRIGHT | CLUI_FRAME_AVATARSRIGHTWITHNICK |
                            CLUI_FRAME_AVATARS | CLUI_FRAME_AVATARBORDER | CLUI_FRAME_ROUNDAVATAR |
                            CLUI_FRAME_ALWAYSALIGNNICK | CLUI_FRAME_SHOWSTATUSMSG | CLUI_FRAME_GDIPLUS);

    g_CluiData.avatarSize = p->avatarSize;
    g_CluiData.avatarBorder = p->avatarBorder;
    g_CluiData.avatarRadius = p->avatarRadius;
    g_CluiData.dualRowMode = p->dualRowMode;
    g_CluiData.bNoOfflineAvatars = p->bNoOfflineAvatars;
    g_CluiData.bShowLocalTime = p->bShowLocalTime;
    g_CluiData.bShowLocalTimeSelective = p->bShowLocalTimeSelective;

    if(g_CluiData.hBrushAvatarBorder)
        DeleteObject(g_CluiData.hBrushAvatarBorder);
    g_CluiData.hBrushAvatarBorder = CreateSolidBrush(g_CluiData.avatarBorder);

    /*
     * items page
     */

    g_CluiData.dwFlags &= ~CLUI_STICKYEVENTS;

    g_CluiData.sortOrder[0] = p->sortOrder[0];
    g_CluiData.sortOrder[1] = p->sortOrder[1];
    g_CluiData.sortOrder[2] = p->sortOrder[2];
    g_CluiData.bDontSeparateOffline = p->bDontSeparateOffline;
    DBWriteContactSettingByte(NULL, "CList", "DontSeparateOffline", (BYTE)g_CluiData.bDontSeparateOffline);
    DBWriteContactSettingDword(NULL, "CLC", "OfflineModes", p->clcOfflineModes);

    DBWriteContactSettingDword(NULL, "CList", "SortOrder", 
        MAKELONG(MAKEWORD(g_CluiData.sortOrder[0], g_CluiData.sortOrder[1]),
        MAKEWORD(g_CluiData.sortOrder[2], 0)));

    g_CluiData.bUseDCMirroring = p->bUseDCMirroring;
    DBWriteContactSettingByte(NULL, "CLC", "MirrorDC", g_CluiData.bUseDCMirroring);

    /*
     * groups page
     */

    g_CluiData.dwFlags &= ~CLUI_FRAME_NOGROUPICON;
    g_CluiData.bGroupAlign = p->bGroupAlign;
    DBWriteContactSettingByte(NULL, "CLC", "GroupAlign", g_CluiData.bGroupAlign);
    DBWriteContactSettingByte(NULL, "CLCExt", "EXBK_CenterGroupnames", (BYTE)p->bCenterGroupNames);

    exStyle = DBGetContactSettingDword(NULL, "CLC", "ExStyle", pcli->pfnGetDefaultExStyle());
    for (i = 0; i < sizeof(checkBoxToGroupStyleEx) / sizeof(checkBoxToGroupStyleEx[0]); i++)
        exStyle &= ~(checkBoxToGroupStyleEx[i].flag);

    exStyle |= p->clcExStyle;
    DBWriteContactSettingDword(NULL, "CLC", "ExStyle", exStyle);
    g_CluiData.avatarPadding = p->avatarPadding;
    DBWriteContactSettingByte(NULL, "CList", "AvatarPadding", g_CluiData.avatarPadding);

    g_CluiData.bRowSpacing = p->bRowSpacing;
    DBWriteContactSettingByte(NULL, "CLC", "RowGap", g_CluiData.bRowSpacing);

    DBWriteContactSettingByte(NULL, "CLC", "LeftMargin", (BYTE)p->bLeftMargin);
    DBWriteContactSettingByte(NULL, "CLC", "RightMargin", (BYTE)p->bRightMargin);
    DBWriteContactSettingByte(NULL, "CLC", "GroupIndent", (BYTE)p->bGroupIndent);
    DBWriteContactSettingByte(NULL, "CLC", "RowHeight", (BYTE)p->bRowHeight);
    DBWriteContactSettingByte(NULL, "CLC", "GRowHeight", (BYTE)p->bGroupRowHeight);

    if(g_CluiData.sortOrder[0] == SORTBY_LASTMSG || g_CluiData.sortOrder[1] == SORTBY_LASTMSG || g_CluiData.sortOrder[2] == SORTBY_LASTMSG) {
        int i;

        for(i = 0; i < g_nextExtraCacheEntry; i++)
            g_ExtraCache[i].dwLastMsgTime = INTSORT_GetLastMsgTime(g_ExtraCache[i].hContact);
    }

    DBWriteContactSettingByte(NULL, "CLC", "ShowLocalTime", (BYTE)g_CluiData.bShowLocalTime);
    DBWriteContactSettingByte(NULL, "CLC", "SelectiveLocalTime", (BYTE)g_CluiData.bShowLocalTimeSelective);
    DBWriteContactSettingDword(NULL, "CLC", "avatarborder", g_CluiData.avatarBorder);
    DBWriteContactSettingDword(NULL, "CLC", "avatarradius", g_CluiData.avatarRadius);
    DBWriteContactSettingWord(NULL, "CList", "AvatarSize", (WORD)g_CluiData.avatarSize);
    DBWriteContactSettingByte(NULL, "CLC", "DualRowMode", g_CluiData.dualRowMode);
    DBWriteContactSettingByte(NULL, "CList", "NoOfflineAV", (BYTE)g_CluiData.bNoOfflineAvatars);

    KillTimer(pcli->hwndContactTree, TIMERID_REFRESH);
    if(g_CluiData.bShowLocalTime)
        SetTimer(pcli->hwndContactTree, TIMERID_REFRESH, 65000, NULL);

    g_CluiData.dwFlags |= p->dwFlags;
    DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);

    pDrawAlpha = NULL;
    if(!pDrawAlpha)
        pDrawAlpha = (g_CluiData.dwFlags & CLUI_FRAME_GDIPLUS  && g_gdiplusToken) ? (pfnDrawAlpha)GDIp_DrawAlpha : (pfnDrawAlpha)DrawAlpha;

    for(i = 0; i < g_nextExtraCacheEntry; i++)
        g_ExtraCache[i].dwXMask = CalcXMask(g_ExtraCache[i].hContact);

    if(oldexIconScale != g_CluiData.exIconScale) {
        ImageList_RemoveAll(himlExtraImages);
        ImageList_SetIconSize(himlExtraImages, g_CluiData.exIconScale, g_CluiData.exIconScale);
        if(g_CluiData.IcoLib_Avail)
            IcoLibReloadIcons();
        else {
            CLN_LoadAllIcons(0);
            pcli->pfnReloadProtoMenus();
            //FYR: Not necessary. It is already notified in pfnReloadProtoMenus
            //NotifyEventHooks(pcli->hPreBuildStatusMenuEvent, 0, 0);
            ReloadExtraIcons();
        }
    }
    pcli->pfnClcOptionsChanged();
    pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
}

void GetDefaultFontSetting(int i, LOGFONT *lf, COLORREF *colour)
{
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), lf, FALSE);
	*colour = GetSysColor(COLOR_WINDOWTEXT);
	switch (i) {
	case FONTID_GROUPS:
		lf->lfWeight = FW_BOLD;
		break;
	case FONTID_GROUPCOUNTS:
		lf->lfHeight = (int) (lf->lfHeight * .75);
		*colour = GetSysColor(COLOR_3DSHADOW);
		break;
	case FONTID_OFFINVIS:
	case FONTID_INVIS:
		lf->lfItalic = !lf->lfItalic;
		break;
	case FONTID_DIVIDERS:
		lf->lfHeight = (int) (lf->lfHeight * .75);
		break;
	case FONTID_NOTONLIST:
		*colour = GetSysColor(COLOR_3DSHADOW);
		break;
}	}

static CALLBACK DlgProcDspGroups(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
            int i = 0;
            TranslateDialogDefault(hwndDlg);
            SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always Left"));
            SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always Right"));
            SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Automatic (RTL)"));
			return TRUE;
		}
    case WM_COMMAND:
        if ((LOWORD(wParam) == IDC_ROWHEIGHT || LOWORD(wParam) == IDC_AVATARPADDING || LOWORD(wParam) == IDC_ROWGAP || LOWORD(wParam) == IDC_RIGHTMARGIN || LOWORD(wParam) == IDC_LEFTMARGIN || LOWORD(wParam) == IDC_SMOOTHTIME || LOWORD(wParam) == IDC_GROUPINDENT || LOWORD(wParam) == IDC_GROUPROWHEIGHT) 
            && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
            return 0;
        SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
        break;
    case WM_USER + 100:
        {
            DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;
            if(p) {
                DWORD exStyle = p->clcExStyle;
				int   i;
                for (i = 0; i < sizeof(checkBoxToGroupStyleEx) / sizeof(checkBoxToGroupStyleEx[0]); i++)
                    CheckDlgButton(hwndDlg, checkBoxToGroupStyleEx[i].id, (exStyle & checkBoxToGroupStyleEx[i].flag) ^ (checkBoxToGroupStyleEx[i].flag * checkBoxToGroupStyleEx[i].not) ? BST_CHECKED : BST_UNCHECKED);

                CheckDlgButton(hwndDlg, IDC_NOGROUPICON, (p->dwFlags & CLUI_FRAME_NOGROUPICON) ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_CENTERGROUPNAMES, p->bCenterGroupNames);
                SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_SETCURSEL, p->bGroupAlign, 0);
                SendDlgItemMessage(hwndDlg, IDC_AVATARPADDINGSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
                SendDlgItemMessage(hwndDlg, IDC_AVATARPADDINGSPIN, UDM_SETPOS, 0, p->avatarPadding);

                SendDlgItemMessage(hwndDlg, IDC_LEFTMARGINSPIN, UDM_SETRANGE, 0, MAKELONG(64, 0));
                SendDlgItemMessage(hwndDlg, IDC_LEFTMARGINSPIN, UDM_SETPOS, 0, p->bLeftMargin);
                SendDlgItemMessage(hwndDlg, IDC_RIGHTMARGINSPIN, UDM_SETRANGE, 0, MAKELONG(64, 0));
                SendDlgItemMessage(hwndDlg, IDC_RIGHTMARGINSPIN, UDM_SETPOS, 0, p->bRightMargin);
                SendDlgItemMessage(hwndDlg, IDC_ROWGAPSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
                SendDlgItemMessage(hwndDlg, IDC_ROWGAPSPIN, UDM_SETPOS, 0, p->bRowSpacing);
                SendDlgItemMessage(hwndDlg, IDC_GROUPINDENTSPIN, UDM_SETRANGE, 0, MAKELONG(50, 0));
                SendDlgItemMessage(hwndDlg, IDC_GROUPINDENTSPIN, UDM_SETPOS, 0, p->bGroupIndent);
                SendDlgItemMessage(hwndDlg, IDC_ROWHEIGHTSPIN, UDM_SETRANGE, 0, MAKELONG(255, 8));
                SendDlgItemMessage(hwndDlg, IDC_ROWHEIGHTSPIN, UDM_SETPOS, 0, p->bRowHeight);
                SendDlgItemMessage(hwndDlg, IDC_GROUPROWHEIGHTSPIN, UDM_SETRANGE, 0, MAKELONG(255, 8));
                SendDlgItemMessage(hwndDlg, IDC_GROUPROWHEIGHTSPIN, UDM_SETPOS, 0, p->bGroupRowHeight);
            }
            return 0;
        }
    case WM_USER + 200:
        {
            DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;
            if(p) {
                int i;
                DWORD exStyle = 0;
                LRESULT curSel;
                BOOL    translated;

                for (i = 0; i < sizeof(checkBoxToGroupStyleEx) / sizeof(checkBoxToGroupStyleEx[0]); i++) {
                    if ((IsDlgButtonChecked(hwndDlg, checkBoxToGroupStyleEx[i].id) == 0) == checkBoxToGroupStyleEx[i].not)
                        exStyle |= checkBoxToGroupStyleEx[i].flag;
                }
                p->clcExStyle = exStyle;
                p->dwFlags |= (IsDlgButtonChecked(hwndDlg, IDC_NOGROUPICON) ? CLUI_FRAME_NOGROUPICON : 0);
                p->bCenterGroupNames = IsDlgButtonChecked(hwndDlg, IDC_CENTERGROUPNAMES) ? 1 : 0;
                curSel = SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_GETCURSEL, 0, 0);
                if(curSel != CB_ERR)
                    p->bGroupAlign = (BYTE)curSel;

                p->avatarPadding = (BYTE)GetDlgItemInt(hwndDlg, IDC_AVATARPADDING, &translated, FALSE);
                p->bLeftMargin = (BYTE)SendDlgItemMessage(hwndDlg, IDC_LEFTMARGINSPIN, UDM_GETPOS, 0, 0);
                p->bRightMargin = (BYTE)SendDlgItemMessage(hwndDlg, IDC_RIGHTMARGINSPIN, UDM_GETPOS, 0, 0);
                p->bRowSpacing = (BYTE)SendDlgItemMessage(hwndDlg, IDC_ROWGAPSPIN, UDM_GETPOS, 0, 0);
                p->bGroupIndent = (BYTE)SendDlgItemMessage(hwndDlg, IDC_GROUPINDENTSPIN, UDM_GETPOS, 0, 0);
                p->bRowHeight = (BYTE)SendDlgItemMessage(hwndDlg, IDC_ROWHEIGHTSPIN, UDM_GETPOS, 0, 0);
                p->bGroupRowHeight = (BYTE)SendDlgItemMessage(hwndDlg, IDC_GROUPROWHEIGHTSPIN, UDM_GETPOS, 0, 0);
            }
            return 0;
        }
    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->idFrom) {
            case 0:
                switch (((LPNMHDR) lParam)->code) {
                    case PSN_APPLY:
                        {
                            return TRUE;
                        }
                }
                break;
        }
        break;
	}
	return FALSE;
}

static CALLBACK DlgProcDspItems(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
            int i = 0;

            TranslateDialogDefault(hwndDlg);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_HIDEOFFLINEOPTS), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_HIDEOFFLINEOPTS), GWL_STYLE) | TVS_NOHSCROLL | TVS_CHECKBOXES);

            for(i = 0; sortCtrlIDs[i] != 0; i++) {
                SendDlgItemMessage(hwndDlg, sortCtrlIDs[i], CB_INSERTSTRING, -1, (LPARAM)TranslateT("Nothing"));
                SendDlgItemMessage(hwndDlg, sortCtrlIDs[i], CB_INSERTSTRING, -1, (LPARAM)TranslateT("Name"));
                SendDlgItemMessage(hwndDlg, sortCtrlIDs[i], CB_INSERTSTRING, -1, (LPARAM)TranslateT("Protocol"));
                SendDlgItemMessage(hwndDlg, sortCtrlIDs[i], CB_INSERTSTRING, -1, (LPARAM)TranslateT("Status"));
                SendDlgItemMessage(hwndDlg, sortCtrlIDs[i], CB_INSERTSTRING, -1, (LPARAM)TranslateT("Last Message"));
                SendDlgItemMessage(hwndDlg, sortCtrlIDs[i], CB_INSERTSTRING, -1, (LPARAM)TranslateT("Message Frequency"));
            }
            SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Never"));
            SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always"));
            SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("For RTL only"));
            SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("RTL TEXT only"));
			return TRUE;
		}
    case WM_COMMAND:
        SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
        break;
    case WM_USER + 100:
        {
            DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;
            if(p) {
                int i;
                FillCheckBoxTree(GetDlgItem(hwndDlg, IDC_HIDEOFFLINEOPTS), offlineValues, sizeof(offlineValues) / sizeof(offlineValues[0]), p->clcOfflineModes);
                CheckDlgButton(hwndDlg, IDC_EVENTSONTOP, (p->dwFlags & CLUI_STICKYEVENTS) ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_DONTSEPARATE, p->bDontSeparateOffline);
                for(i = 0; sortCtrlIDs[i] != 0; i++)
                    SendDlgItemMessage(hwndDlg, sortCtrlIDs[i], CB_SETCURSEL, p->sortOrder[i], 0);

                SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_SETCURSEL, p->bUseDCMirroring, 0);
            }
            return 0;
        }
    case WM_USER + 200:
        {
            DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;
            if(p) {
                int     i;
                LRESULT curSel;

                for(i = 0; sortCtrlIDs[i] != 0; i++) {
                    curSel = SendDlgItemMessage(hwndDlg, sortCtrlIDs[i], CB_GETCURSEL, 0, 0);
                    if(curSel == 0 || curSel == CB_ERR)
                        p->sortOrder[i] = 0;
                    else
                        p->sortOrder[i] = (BYTE)curSel;
                }
                p->bDontSeparateOffline = IsDlgButtonChecked(hwndDlg, IDC_DONTSEPARATE) ? 1 : 0;
                p->dwFlags |= IsDlgButtonChecked(hwndDlg, IDC_EVENTSONTOP) ? CLUI_STICKYEVENTS : 0;
                p->clcOfflineModes = MakeCheckBoxTreeFlags(GetDlgItem(hwndDlg, IDC_HIDEOFFLINEOPTS));
                p->bUseDCMirroring = (BYTE)SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_GETCURSEL, 0, 0);
            }
            return 0;
        }
    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->idFrom) {
            case IDC_HIDEOFFLINEOPTS:
                if (((LPNMHDR) lParam)->code == NM_CLICK) {
                    TVHITTESTINFO hti;
                    hti.pt.x = (short) LOWORD(GetMessagePos());
                    hti.pt.y = (short) HIWORD(GetMessagePos());
                    ScreenToClient(((LPNMHDR) lParam)->hwndFrom, &hti.pt);
                    if (TreeView_HitTest(((LPNMHDR) lParam)->hwndFrom, &hti))
                        if (hti.flags & TVHT_ONITEMSTATEICON) {
                            TVITEM tvi;
                            tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                            tvi.hItem = hti.hItem;
                            TreeView_GetItem(((LPNMHDR) lParam)->hwndFrom, &tvi);
                            tvi.iImage = tvi.iSelectedImage = tvi.iImage == 1 ? 2 : 1;
                            TreeView_SetItem(((LPNMHDR) lParam)->hwndFrom, &tvi);
                            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                        }
                }
                break;
            case 0:
                switch (((LPNMHDR) lParam)->code) {
                    case PSN_APPLY:
                        {
                            return TRUE;
                        }
                }
                break;
        }
        break;
	}
	return FALSE;
}

static UINT avatar_controls[] = { IDC_ALIGNMENT, IDC_AVATARSBORDER, IDC_AVATARSROUNDED, IDC_AVATARBORDERCLR, IDC_ALWAYSALIGNNICK, IDC_AVATARHEIGHT, IDC_AVATARSIZESPIN, 0 };
static CALLBACK DlgProcDspAdvanced(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
            int i = 0;

            TranslateDialogDefault(hwndDlg);
            SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Never"));
            SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always"));
            SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("When space allows it"));
            SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("When needed"));

            SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_INSERTSTRING, -1, (LPARAM)TranslateT("With Nickname - left"));
            SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Far left"));
            SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Far right"));
            SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_INSERTSTRING, -1, (LPARAM)TranslateT("With Nickname - right"));

            if(g_CluiData.bAvatarServiceAvail) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_CLISTAVATARS), TRUE);
                while(avatar_controls[i] != 0)
                    EnableWindow(GetDlgItem(hwndDlg, avatar_controls[i++]), TRUE);
            }
            else {
                EnableWindow(GetDlgItem(hwndDlg, IDC_CLISTAVATARS), FALSE);
                while(avatar_controls[i] != 0)
                    EnableWindow(GetDlgItem(hwndDlg, avatar_controls[i++]), FALSE);
            }
			return TRUE;
		}
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
            case IDC_CLISTAVATARS:
                if((HWND)lParam != GetFocus())
                    return 0;
                break;
            case IDC_SHOWLOCALTIME:
                EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWLOCALTIMEONLYWHENDIFFERENT), IsDlgButtonChecked(hwndDlg, IDC_SHOWLOCALTIME));
                break;
            case IDC_AVATARSROUNDED:
                EnableWindow(GetDlgItem(hwndDlg, IDC_RADIUS), IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED) ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_RADIUSSPIN), IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED) ? TRUE : FALSE);
                break;
            case IDC_AVATARSBORDER:
                EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARBORDERCLR), IsDlgButtonChecked(hwndDlg, IDC_AVATARSBORDER) ? TRUE : FALSE);
                break;
        }
        if ((LOWORD(wParam) == IDC_RADIUS || LOWORD(wParam) == IDC_AVATARHEIGHT) && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
            return 0;
        SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
        break;
    case WM_USER + 100:
        {
            DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;
            if(p) {
                CheckDlgButton(hwndDlg, IDC_NOAVATARSOFFLINE, p->bNoOfflineAvatars);
                SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_SETCURSEL, (WPARAM)p->dualRowMode, 0);
                CheckDlgButton(hwndDlg, IDC_CLISTAVATARS, (p->dwFlags & CLUI_FRAME_AVATARS) ? BST_CHECKED : BST_UNCHECKED);

                CheckDlgButton(hwndDlg, IDC_AVATARSBORDER, (p->dwFlags & CLUI_FRAME_AVATARBORDER) ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_AVATARSROUNDED, (p->dwFlags & CLUI_FRAME_ROUNDAVATAR) ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_ALWAYSALIGNNICK, (p->dwFlags & CLUI_FRAME_ALWAYSALIGNNICK) ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_SHOWSTATUSMSG, (p->dwFlags & CLUI_FRAME_SHOWSTATUSMSG) ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_RENDERGDIP, (p->dwFlags & CLUI_FRAME_GDIPLUS) ? BST_CHECKED : BST_UNCHECKED);

                SendDlgItemMessage(hwndDlg, IDC_AVATARBORDERCLR, CPM_SETCOLOUR, 0, p->avatarBorder);

                SendDlgItemMessage(hwndDlg, IDC_RADIUSSPIN, UDM_SETRANGE, 0, MAKELONG(10, 2));
                SendDlgItemMessage(hwndDlg, IDC_RADIUSSPIN, UDM_SETPOS, 0, p->avatarRadius);

                SendDlgItemMessage(hwndDlg, IDC_AVATARSIZESPIN, UDM_SETRANGE, 0, MAKELONG(100, 16));
                SendDlgItemMessage(hwndDlg, IDC_AVATARSIZESPIN, UDM_SETPOS, 0, p->avatarSize);

                EnableWindow(GetDlgItem(hwndDlg, IDC_RADIUS), IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED) ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_RADIUSSPIN), IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED) ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARBORDERCLR), IsDlgButtonChecked(hwndDlg, IDC_AVATARSBORDER) ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_RENDERGDIP), g_gdiplusToken != 0 ? TRUE : FALSE);

                CheckDlgButton(hwndDlg, IDC_SHOWLOCALTIME, p->bShowLocalTime ? 1 : 0);
                CheckDlgButton(hwndDlg, IDC_SHOWLOCALTIMEONLYWHENDIFFERENT, p->bShowLocalTimeSelective ? 1 : 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWLOCALTIMEONLYWHENDIFFERENT), IsDlgButtonChecked(hwndDlg, IDC_SHOWLOCALTIME));

                if(p->dwFlags & CLUI_FRAME_AVATARSLEFT)
                    SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_SETCURSEL, 1, 0);
                else if(p->dwFlags & CLUI_FRAME_AVATARSRIGHT)
                    SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_SETCURSEL, 2, 0);
                else if(p->dwFlags & CLUI_FRAME_AVATARSRIGHTWITHNICK)
                    SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_SETCURSEL, 3, 0);
                else
                    SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_SETCURSEL, 0, 0);
            }
            return 0;
        }
    case WM_USER + 200:
        {
            DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;
            if(p) {
                LRESULT sel = SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_GETCURSEL, 0, 0);
				BOOL    translated;

                if(sel != CB_ERR) {
                    if(sel == 1)
                        p->dwFlags |= CLUI_FRAME_AVATARSLEFT;
                    else if(sel == 2)
                        p->dwFlags |= CLUI_FRAME_AVATARSRIGHT;
                    else if(sel == 3)
                        p->dwFlags |= CLUI_FRAME_AVATARSRIGHTWITHNICK;
                }

                p->dwFlags |= ((IsDlgButtonChecked(hwndDlg, IDC_CLISTAVATARS) ? CLUI_FRAME_AVATARS : 0) |
                               (IsDlgButtonChecked(hwndDlg, IDC_AVATARSBORDER) ? CLUI_FRAME_AVATARBORDER : 0) |
                               (IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED) ? CLUI_FRAME_ROUNDAVATAR : 0) |
                               (IsDlgButtonChecked(hwndDlg, IDC_ALWAYSALIGNNICK) ? CLUI_FRAME_ALWAYSALIGNNICK : 0) |
                               (IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUSMSG) ? CLUI_FRAME_SHOWSTATUSMSG : 0) |
                               (IsDlgButtonChecked(hwndDlg, IDC_RENDERGDIP) ? CLUI_FRAME_GDIPLUS : 0));

                p->avatarBorder = SendDlgItemMessage(hwndDlg, IDC_AVATARBORDERCLR, CPM_GETCOLOUR, 0, 0);
                p->avatarRadius = GetDlgItemInt(hwndDlg, IDC_RADIUS, &translated, FALSE);
                p->avatarSize = GetDlgItemInt(hwndDlg, IDC_AVATARHEIGHT, &translated, FALSE);
                p->bNoOfflineAvatars = IsDlgButtonChecked(hwndDlg, IDC_NOAVATARSOFFLINE) ? TRUE : FALSE;
                p->bShowLocalTime = IsDlgButtonChecked(hwndDlg, IDC_SHOWLOCALTIME) ? 1 : 0;
                p->bShowLocalTimeSelective = IsDlgButtonChecked(hwndDlg, IDC_SHOWLOCALTIMEONLYWHENDIFFERENT) ? 1 : 0;

                p->dualRowMode = (BYTE)SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_GETCURSEL, 0, 0);
                if(p->dualRowMode == CB_ERR)
                    p->dualRowMode = 0;
            }
            return 0;
        }
    case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
		case PSN_APPLY:
			{
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcXIcons(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			return TRUE;
		}
	case WM_COMMAND:
		if ((LOWORD(wParam) == IDC_EXICONSCALE) && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
			return 0;
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;

    case WM_USER + 100:
        {
            DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;
            if(p) {
                CheckDlgButton(hwndDlg, IDC_SHOWCLIENTICONS, p->dwExtraImageMask & EIMG_SHOW_CLIENT);
                CheckDlgButton(hwndDlg, IDC_SHOWEXTENDEDSTATUS, p->dwExtraImageMask & EIMG_SHOW_EXTRA);
                CheckDlgButton(hwndDlg, IDC_EXTRAMAIL, p->dwExtraImageMask & EIMG_SHOW_MAIL);
                CheckDlgButton(hwndDlg, IDC_EXTRAWEB, p->dwExtraImageMask & EIMG_SHOW_URL);
                CheckDlgButton(hwndDlg, IDC_EXTRAPHONE, p->dwExtraImageMask & EIMG_SHOW_SMS);
                CheckDlgButton(hwndDlg, IDC_EXTRARESERVED, p->dwExtraImageMask & EIMG_SHOW_RESERVED);
                CheckDlgButton(hwndDlg, IDC_EXTRARESERVED2, p->dwExtraImageMask & EIMG_SHOW_RESERVED2);
                CheckDlgButton(hwndDlg, IDC_EXTRARESERVED3, p->dwExtraImageMask & EIMG_SHOW_RESERVED3);
                CheckDlgButton(hwndDlg, IDC_EXTRARESERVED4, p->dwExtraImageMask & EIMG_SHOW_RESERVED4);
                CheckDlgButton(hwndDlg, IDC_EXTRARESERVED5, p->dwExtraImageMask & EIMG_SHOW_RESERVED5);

                CheckDlgButton(hwndDlg, IDC_SHOWSTATUSICONS, (p->dwFlags & CLUI_FRAME_STATUSICONS) ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_SHOWVISIBILITY, (p->dwFlags & CLUI_SHOWVISI) ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_SHOWMETA, (p->dwFlags & CLUI_USEMETAICONS) ? BST_CHECKED : BST_UNCHECKED);

                CheckDlgButton(hwndDlg, IDC_OVERLAYICONS, (p->dwFlags & CLUI_FRAME_OVERLAYICONS) ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_SELECTIVEICONS, (p->dwFlags & CLUI_FRAME_SELECTIVEICONS) ? BST_CHECKED : BST_UNCHECKED);

                CheckDlgButton(hwndDlg, IDC_STATUSICONSCENTERED, p->bCenterStatusIcons ? 1 : 0);
                CheckDlgButton(hwndDlg, IDC_IDLE, p->bDimIdle ? BST_CHECKED : BST_UNCHECKED);

                SendDlgItemMessage(hwndDlg, IDC_EXICONSCALESPIN, UDM_SETRANGE, 0, MAKELONG(20, 8));
                SendDlgItemMessage(hwndDlg, IDC_EXICONSCALESPIN, UDM_SETPOS, 0, (LPARAM)p->exIconScale);
            }
            return 0;
        }
    case WM_USER + 200:
        {
            DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;
            if(p) {
                p->dwExtraImageMask = (IsDlgButtonChecked(hwndDlg, IDC_EXTRAMAIL) ? EIMG_SHOW_MAIL : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRAWEB) ? EIMG_SHOW_URL : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_SHOWEXTENDEDSTATUS) ? EIMG_SHOW_EXTRA : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRAPHONE) ? EIMG_SHOW_SMS : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED) ? EIMG_SHOW_RESERVED : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_SHOWCLIENTICONS) ? EIMG_SHOW_CLIENT : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED2) ? EIMG_SHOW_RESERVED2 : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED3) ? EIMG_SHOW_RESERVED3 : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED4) ? EIMG_SHOW_RESERVED4 : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED5) ? EIMG_SHOW_RESERVED5 : 0);

                    p->exIconScale = SendDlgItemMessage(hwndDlg, IDC_EXICONSCALESPIN, UDM_GETPOS, 0, 0);
                    p->exIconScale = (p->exIconScale < 8 || p->exIconScale > 20) ? 16 : p->exIconScale;

                    p->dwFlags |= ((IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUSICONS) ? CLUI_FRAME_STATUSICONS : 0) |
                                   (IsDlgButtonChecked(hwndDlg, IDC_SHOWVISIBILITY) ? CLUI_SHOWVISI : 0) |
                                   (IsDlgButtonChecked(hwndDlg, IDC_SHOWMETA) ? CLUI_USEMETAICONS : 0) |
                                   (IsDlgButtonChecked(hwndDlg, IDC_OVERLAYICONS) ? CLUI_FRAME_OVERLAYICONS : 0) |
                                   (IsDlgButtonChecked(hwndDlg, IDC_SELECTIVEICONS) ? CLUI_FRAME_SELECTIVEICONS : 0));
                                    
                    p->bDimIdle = IsDlgButtonChecked(hwndDlg, IDC_IDLE) ? 1 : 0;
                    p->bCenterStatusIcons = IsDlgButtonChecked(hwndDlg, IDC_STATUSICONSCENTERED) ? 1 : 0;

            }
            return 0;
        }
    case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
		case PSN_APPLY:
			{
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcDspProfiles(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static int iInit = TRUE;
   static HWND hwndTab;
   static int iTabCount;

   switch(msg)
   {
      case WM_INITDIALOG:
      {
         TCITEM tci;
         RECT rcClient;
         int oPage = DBGetContactSettingByte(NULL, "CLUI", "opage_d", 0);
         HWND hwndAdd;
         DISPLAYPROFILE dsp_default;

         hwndAdd = GetDlgItem(hwnd, IDC_DSP_ADD);
         SendMessage(hwndAdd, BUTTONSETASFLATBTN, 0, 1);
         SendMessage(hwndAdd, BUTTONSETASFLATBTN + 10, 0, 1);
         SendMessage(hwndAdd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ADDCONTACT), IMAGE_ICON, 16, 16, LR_SHARED));
         SetWindowText(hwndAdd, TranslateT("Add New..."));

         hwndAdd = GetDlgItem(hwnd, IDC_DSP_DELETE);
         SendMessage(hwndAdd, BUTTONSETASFLATBTN, 0, 1);
         SendMessage(hwndAdd, BUTTONSETASFLATBTN + 10, 0, 1);
         SendMessage(hwndAdd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DELETE), IMAGE_ICON, 16, 16, LR_SHARED));
         SetWindowText(hwndAdd, TranslateT("Delete"));

         hwndAdd = GetDlgItem(hwnd, IDC_DSP_RENAME);
         SendMessage(hwndAdd, BUTTONSETASFLATBTN, 0, 1);
         SendMessage(hwndAdd, BUTTONSETASFLATBTN + 10, 0, 1);
         SendMessage(hwndAdd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_RENAME), IMAGE_ICON, 16, 16, LR_SHARED));
         SetWindowText(hwndAdd, TranslateT("Rename..."));

         hwndAdd = GetDlgItem(hwnd, IDC_DSP_APPLY);
         SendMessage(hwndAdd, BUTTONSETASFLATBTN, 0, 1);
         SendMessage(hwndAdd, BUTTONSETASFLATBTN + 10, 0, 1);
         SendMessage(hwndAdd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_OPTIONS), IMAGE_ICON, 16, 16, LR_SHARED));
         SetWindowText(hwndAdd, TranslateT("Apply this profile"));

         GetClientRect(hwnd, &rcClient);
         hwndTab = GetDlgItem(hwnd, IDC_OPTIONSTAB);
         iInit = TRUE;
         tci.mask = TCIF_PARAM|TCIF_TEXT;

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_DSPITEMS), hwnd, DlgProcDspItems);
         tci.pszText = TranslateT("Contacts");
         TabCtrl_InsertItem(hwndTab, 0, &tci);
         MoveWindow((HWND)tci.lParam,124,25,rcClient.right-128,rcClient.bottom-67,1);
         ShowWindow((HWND)tci.lParam, oPage == 0 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_DSPGROUPS), hwnd, DlgProcDspGroups);
         tci.pszText = TranslateT("Groups and layout");
         TabCtrl_InsertItem(hwndTab, 1, &tci);
         MoveWindow((HWND)tci.lParam,124,25,rcClient.right-128,rcClient.bottom-67,1);
         ShowWindow((HWND)tci.lParam, oPage == 1 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_XICONS), hwnd, DlgProcXIcons);
         tci.pszText = TranslateT("Icons");
         TabCtrl_InsertItem(hwndTab, 2, &tci);
         MoveWindow((HWND)tci.lParam,124,25,rcClient.right-128,rcClient.bottom-67,1);
         ShowWindow((HWND)tci.lParam, oPage == 2 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_DSPADVANCED), hwnd, DlgProcDspAdvanced);
         tci.pszText = TranslateT("Advanced");
         TabCtrl_InsertItem(hwndTab, 3, &tci);
         MoveWindow((HWND)tci.lParam,124,25,rcClient.right-128,rcClient.bottom-67,1);
         ShowWindow((HWND)tci.lParam, oPage == 3 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB), oPage);

         DSP_LoadFromDefaults(&dsp_default);

         iTabCount =  TabCtrl_GetItemCount(hwndTab);

         SendMessage(hwnd, WM_USER + 100, 0, (LPARAM)&dsp_default);
         iInit = FALSE;
         return FALSE;
      }

      /*
       * distribute a WM_USER message to all child windows so they can update their pages from the
       * display profile structure
       * LPARAM = DISPLAYPROFILE *
       */

      case WM_USER + 100:
      {
          DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;

          if(p) {
              int i;
              TCITEM item = {0};
              item.mask = TCIF_PARAM;

              for(i = 0; i < iTabCount; i++) {
                  TabCtrl_GetItem(hwndTab, i, &item);
                  if(item.lParam && IsWindow((HWND)item.lParam))
                     SendMessage((HWND)item.lParam, WM_USER + 100, 0, (LPARAM)p);
              }
          }
          return 0;
      }

      /*
       * collect the settings from the pages into a DISPLAYPROFILE struct
       */
      case WM_USER + 200:
      {
          DISPLAYPROFILE *p = (DISPLAYPROFILE *)lParam;
          int i;
          TCITEM item = {0};
          item.mask = TCIF_PARAM;

          for(i = 0; i < iTabCount; i++) {
              TabCtrl_GetItem(hwndTab, i, &item);
              if(item.lParam && IsWindow((HWND)item.lParam))
                 SendMessage((HWND)item.lParam, WM_USER + 200, 0, (LPARAM)p);
          }
          return 0;
      }

      case PSM_CHANGED: // used so tabs dont have to call SendMessage(GetParent(GetParent(hwnd)), PSM_CHANGED, 0, 0);
         if(!iInit)
             SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
         break;
      case WM_NOTIFY:
         switch(((LPNMHDR)lParam)->idFrom) {
            case 0:
               switch (((LPNMHDR)lParam)->code)
               {
                  case PSN_APPLY:
                     {
                         DISPLAYPROFILE p;

                         ZeroMemory(&p, sizeof(DISPLAYPROFILE));
                         SendMessage(hwnd, WM_USER + 200, 0, (LPARAM)&p);
                         DSP_Apply(&p);
                     }
                  break;
               }
            break;
            case IDC_OPTIONSTAB:
               switch (((LPNMHDR)lParam)->code)
               {
                  case TCN_SELCHANGING:
                     {
                        TCITEM tci;
                        tci.mask = TCIF_PARAM;
                        TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
                        ShowWindow((HWND)tci.lParam,SW_HIDE);                     
                     }
                  break;
                  case TCN_SELCHANGE:
                     {
                        TCITEM tci;
                        tci.mask = TCIF_PARAM;
                        TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
                        ShowWindow((HWND)tci.lParam,SW_SHOW);
                        DBWriteContactSettingByte(NULL, "CLUI", "opage_d", (BYTE)TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB)));
                     }
                  break;
               }
            break;

         }
      break;
   }
   return FALSE;
}

static BOOL CALLBACK TabOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static int iInit = TRUE;
   
   switch(msg)
   {
      case WM_INITDIALOG:
      {
         TCITEM tci;
         RECT rcClient;
         int oPage = DBGetContactSettingByte(NULL, "CLUI", "opage_m", 0);

         GetClientRect(hwnd, &rcClient);
         iInit = TRUE;
         tci.mask = TCIF_PARAM|TCIF_TEXT;
         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CLIST), hwnd, DlgProcGenOpts);
         tci.pszText = TranslateT("General");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 0, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 0 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CLC), hwnd, DlgProcClcMainOpts);
         tci.pszText = TranslateT("List layout");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 1, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 1 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CLUI), hwnd, DlgProcCluiOpts);
         tci.pszText = TranslateT("Window");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 2, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 2 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CLCBKG), hwnd, DlgProcClcBkgOpts);
         tci.pszText = TranslateT("Background");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 3, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 3 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_SBAR), hwnd, DlgProcSBarOpts);
         tci.pszText = TranslateT("Status Bar");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 4, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 4 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB), oPage);
         iInit = FALSE;
         return FALSE;
      }
      
       case PSM_CHANGED: // used so tabs dont have to call SendMessage(GetParent(GetParent(hwnd)), PSM_CHANGED, 0, 0);
         if(!iInit)
             SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
         break;
      case WM_NOTIFY:
         switch(((LPNMHDR)lParam)->idFrom) {
            case 0:
               switch (((LPNMHDR)lParam)->code)
               {
                  case PSN_APPLY:
                     {
                        TCITEM tci;
                        int i,count;
                        tci.mask = TCIF_PARAM;
                        count = TabCtrl_GetItemCount(GetDlgItem(hwnd,IDC_OPTIONSTAB));
                        for (i=0;i<count;i++)
                        {
                           TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),i,&tci);
                           SendMessage((HWND)tci.lParam,WM_NOTIFY,0,lParam);
                        }
                     }
                  break;
               }
            break;
            case IDC_OPTIONSTAB:
               switch (((LPNMHDR)lParam)->code)
               {
                  case TCN_SELCHANGING:
                     {
                        TCITEM tci;
                        tci.mask = TCIF_PARAM;
                        TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
                        ShowWindow((HWND)tci.lParam,SW_HIDE);                     
                     }
                  break;
                  case TCN_SELCHANGE:
                     {
                        TCITEM tci;
                        tci.mask = TCIF_PARAM;
                        TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
                        ShowWindow((HWND)tci.lParam,SW_SHOW);
                        DBWriteContactSettingByte(NULL, "CLUI", "opage_m", (BYTE)TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB)));
                     }
                  break;
               }
            break;

         }
      break;
   }
   return FALSE;
}

int ClcOptInit(WPARAM wParam, LPARAM lParam)
{
    OPTIONSDIALOGPAGE odp;

    ZeroMemory(&odp, sizeof(odp));
    odp.cbSize = sizeof(odp);
    odp.position = 0;
    odp.hInstance = g_hInst;
    odp.pszGroup = Translate("Contact List");

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_DSPPROFILES);
    odp.pszTitle = Translate("Display Profiles");
    odp.pfnDlgProc = DlgProcDspProfiles;
    odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_FLOATING);
    odp.pszTitle = Translate("Floating contacts");
    odp.pfnDlgProc = DlgProcFloatingContacts;
    odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT);
    odp.pszGroup = Translate("Customize");
    odp.pszTitle = Translate("Contact list skin");
    odp.flags = ODPF_BOLDGROUPS;
    odp.pfnDlgProc = OptionsDlgProc;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.position = -1000000000;
    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONSDIALOG);
    odp.pszGroup = NULL;
    odp.pszTitle = Translate("Contact List");
    odp.pfnDlgProc = TabOptionsDlgProc;
    odp.flags = ODPF_BOLDGROUPS;
    odp.nIDBottomSimpleControl = 0;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.position = -900000000;
    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_HOTKEY);
    odp.pszTitle = Translate("Hotkeys");
    odp.pszGroup = Translate("Events");
    odp.pfnDlgProc = DlgProcHotkeyOpts;
    odp.nIDBottomSimpleControl = 0;
    odp.nExpertOnlyControls = 0;
    odp.expertOnlyControls = NULL;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);
    return 0;
}

static int opt_clc_main_changed = 0;

static BOOL CALLBACK DlgProcClcMainOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            TranslateDialogDefault(hwndDlg);
            opt_clc_main_changed = 0;
            SetWindowLong(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), GWL_STYLE) | TVS_NOHSCROLL | TVS_CHECKBOXES);

            {
				int i;
                DWORD exStyle = DBGetContactSettingDword(NULL, "CLC", "ExStyle", pcli->pfnGetDefaultExStyle());
                UDACCEL accel[2] = {
                    {0,10}, {2,50}
                };
                SendDlgItemMessage(hwndDlg, IDC_SMOOTHTIMESPIN, UDM_SETRANGE, 0, MAKELONG(999, 0));
                SendDlgItemMessage(hwndDlg, IDC_SMOOTHTIMESPIN, UDM_SETACCEL, sizeof(accel) / sizeof(accel[0]), (LPARAM) &accel);
                SendDlgItemMessage(hwndDlg, IDC_SMOOTHTIMESPIN, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingWord(NULL, "CLC", "ScrollTime", CLCDEFAULT_SCROLLTIME), 0));

                for (i = 0; i < sizeof(checkBoxToStyleEx) / sizeof(checkBoxToStyleEx[0]); i++)
                    CheckDlgButton(hwndDlg, checkBoxToStyleEx[i].id, (exStyle & checkBoxToStyleEx[i].flag) ^ (checkBoxToStyleEx[i].flag * checkBoxToStyleEx[i].not) ? BST_CHECKED : BST_UNCHECKED);
            }
            CheckDlgButton(hwndDlg, IDC_FULLROWSELECT, (g_CluiData.dwFlags & CLUI_FULLROWSELECT) ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(hwndDlg, IDC_DBLCLKAVATARS, g_CluiData.bDblClkAvatars);
            CheckDlgButton(hwndDlg, IDC_GREYOUT, DBGetContactSettingDword(NULL, "CLC", "GreyoutFlags", CLCDEFAULT_GREYOUTFLAGS) ? BST_CHECKED : BST_UNCHECKED);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SMOOTHTIME), IsDlgButtonChecked(hwndDlg, IDC_NOTNOSMOOTHSCROLLING));
            EnableWindow(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), IsDlgButtonChecked(hwndDlg, IDC_GREYOUT));
            FillCheckBoxTree(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), greyoutValues, sizeof(greyoutValues) / sizeof(greyoutValues[0]), DBGetContactSettingDword(NULL, "CLC", "FullGreyoutFlags", CLCDEFAULT_FULLGREYOUTFLAGS));
            CheckDlgButton(hwndDlg, IDC_NOSCROLLBAR, DBGetContactSettingByte(NULL, "CLC", "NoVScrollBar", 0) ? BST_CHECKED : BST_UNCHECKED);

            return TRUE;
        case WM_VSCROLL:
            opt_clc_main_changed = 1;
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_NOTNOSMOOTHSCROLLING)
                EnableWindow(GetDlgItem(hwndDlg, IDC_SMOOTHTIME), IsDlgButtonChecked(hwndDlg, IDC_NOTNOSMOOTHSCROLLING));
            if (LOWORD(wParam) == IDC_GREYOUT)
                EnableWindow(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), IsDlgButtonChecked(hwndDlg, IDC_GREYOUT));
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            opt_clc_main_changed = 1;
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case IDC_GREYOUTOPTS:
                    if (((LPNMHDR) lParam)->code == NM_CLICK) {
                        TVHITTESTINFO hti;
                        hti.pt.x = (short) LOWORD(GetMessagePos());
                        hti.pt.y = (short) HIWORD(GetMessagePos());
                        ScreenToClient(((LPNMHDR) lParam)->hwndFrom, &hti.pt);
                        if (TreeView_HitTest(((LPNMHDR) lParam)->hwndFrom, &hti))
                            if (hti.flags & TVHT_ONITEMSTATEICON) {
                                TVITEM tvi;
                                tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                                tvi.hItem = hti.hItem;
                                TreeView_GetItem(((LPNMHDR) lParam)->hwndFrom, &tvi);
                                tvi.iImage = tvi.iSelectedImage = tvi.iImage == 1 ? 2 : 1;
                                TreeView_SetItem(((LPNMHDR) lParam)->hwndFrom, &tvi);
                                opt_clc_main_changed = 1;
                                SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                            }
                    }
                    break;
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                            {
                                int i;
                                DWORD exStyle = DBGetContactSettingDword(NULL, "CLC", "ExStyle", CLCDEFAULT_EXSTYLE);

                                if(!opt_clc_main_changed)
                                    return TRUE;

                                for (i = 0; i < sizeof(checkBoxToStyleEx) / sizeof(checkBoxToStyleEx[0]); i++)
                                    exStyle &= ~(checkBoxToStyleEx[i].flag);

                                for (i = 0; i < sizeof(checkBoxToStyleEx) / sizeof(checkBoxToStyleEx[0]); i++) {
                                    if ((IsDlgButtonChecked(hwndDlg, checkBoxToStyleEx[i].id) == 0) == checkBoxToStyleEx[i].not)
                                        exStyle |= checkBoxToStyleEx[i].flag;
                                }
                                DBWriteContactSettingDword(NULL, "CLC", "ExStyle", exStyle);
                            } {
                                DWORD fullGreyoutFlags = MakeCheckBoxTreeFlags(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS));
                                DBWriteContactSettingDword(NULL, "CLC", "FullGreyoutFlags", fullGreyoutFlags);
                                if (IsDlgButtonChecked(hwndDlg, IDC_GREYOUT))
                                    DBWriteContactSettingDword(NULL, "CLC", "GreyoutFlags", fullGreyoutFlags);
                                else
                                    DBWriteContactSettingDword(NULL, "CLC", "GreyoutFlags", 0);
                            }
                            DBWriteContactSettingWord(NULL, "CLC", "ScrollTime", (WORD) SendDlgItemMessage(hwndDlg, IDC_SMOOTHTIMESPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingByte(NULL, "CLC", "NoVScrollBar", (BYTE) (IsDlgButtonChecked(hwndDlg, IDC_NOSCROLLBAR) ? 1 : 0));
                            g_CluiData.dwFlags = IsDlgButtonChecked(hwndDlg, IDC_FULLROWSELECT) ? g_CluiData.dwFlags | CLUI_FULLROWSELECT : g_CluiData.dwFlags & ~CLUI_FULLROWSELECT;
                            g_CluiData.bDblClkAvatars = IsDlgButtonChecked(hwndDlg, IDC_DBLCLKAVATARS) ? TRUE : FALSE;
                            DBWriteContactSettingByte(NULL, "CLC", "dblclkav", (BYTE)g_CluiData.bDblClkAvatars);
                            DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);

                            pcli->pfnClcOptionsChanged();
                            CoolSB_SetupScrollBar();
                            PostMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
                            opt_clc_main_changed = 0;
                            return TRUE;
                    }
                    break;
            }
            break;
        case WM_DESTROY:
            ImageList_Destroy(TreeView_GetImageList(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), TVSIL_NORMAL));
            break;
    }
    return FALSE;
}

static int opt_clc_bkg_changed = 0;

static BOOL CALLBACK DlgProcClcBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            opt_clc_bkg_changed = 0;
            TranslateDialogDefault(hwndDlg);
            CheckDlgButton(hwndDlg, IDC_BITMAP, DBGetContactSettingByte(NULL, "CLC", "UseBitmap", CLCDEFAULT_USEBITMAP) ? BST_CHECKED : BST_UNCHECKED);
            SendMessage(hwndDlg, WM_USER + 10, 0, 0);           
            SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, CLCDEFAULT_BKCOLOUR);
            SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, "CLC", "BkColour", CLCDEFAULT_BKCOLOUR));   
            CheckDlgButton(hwndDlg, IDC_WINCOLOUR, DBGetContactSettingByte(NULL, "CLC", "UseWinColours", 0));
            CheckDlgButton(hwndDlg, IDC_SKINMODE, g_CluiData.bWallpaperMode);
            SendMessage(hwndDlg, WM_USER + 11, 0, 0); {
                DBVARIANT dbv;
                
                if (!DBGetContactSetting(NULL, "CLC", "BkBitmap", &dbv)) {
                    if (ServiceExists(MS_UTILS_PATHTOABSOLUTE)) {
                        char szPath[MAX_PATH];

                        if (CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM) dbv.pszVal, (LPARAM) szPath))
                            SetDlgItemTextA(hwndDlg, IDC_FILENAME, szPath);
                    }
                    DBFreeVariant(&dbv);
                }
            } {
                WORD bmpUse = DBGetContactSettingWord(NULL, "CLC", "BkBmpUse", CLCDEFAULT_BKBMPUSE);
                CheckDlgButton(hwndDlg, IDC_STRETCHH, bmpUse & CLB_STRETCHH ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_STRETCHV, bmpUse & CLB_STRETCHV ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_TILEH, bmpUse & CLBF_TILEH ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_TILEV, bmpUse & CLBF_TILEV ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_SCROLL, bmpUse & CLBF_SCROLL ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROPORTIONAL, bmpUse & CLBF_PROPORTIONAL ? BST_CHECKED : BST_UNCHECKED);
            } {
                HRESULT (STDAPICALLTYPE *MySHAutoComplete)(HWND, DWORD);
                MySHAutoComplete = (HRESULT(STDAPICALLTYPE *)(HWND, DWORD))GetProcAddress(GetModuleHandleA("shlwapi"), "SHAutoComplete");
                if (MySHAutoComplete)
                    MySHAutoComplete(GetDlgItem(hwndDlg, IDC_FILENAME), 1);
            }
            return TRUE;
        case WM_USER+10:
            EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_STRETCHH), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_STRETCHV), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_TILEH), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_TILEV), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_SCROLL), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_PROPORTIONAL), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            break;
        case WM_USER+11:
            {
                BOOL b = IsDlgButtonChecked(hwndDlg, IDC_WINCOLOUR);
                EnableWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOUR), !b);           
                break;
            }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BROWSE) {
                char str[MAX_PATH];
                OPENFILENAMEA ofn = {
                    0
                };
                char filter[512];

                GetDlgItemTextA(hwndDlg, IDC_FILENAME, str, sizeof(str));
                ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
                ofn.hwndOwner = hwndDlg;
                ofn.hInstance = NULL;
                CallService(MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof(filter), (LPARAM) filter);
                ofn.lpstrFilter = filter;
                ofn.lpstrFile = str;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                ofn.nMaxFile = sizeof(str);
                ofn.nMaxFileTitle = MAX_PATH;
                ofn.lpstrDefExt = "bmp";
                if (!GetOpenFileNameA(&ofn))
                    break;
                SetDlgItemTextA(hwndDlg, IDC_FILENAME, str);
            } else if (LOWORD(wParam) == IDC_FILENAME && HIWORD(wParam) != EN_CHANGE)
                break;
            if (LOWORD(wParam) == IDC_BITMAP)
                SendMessage(hwndDlg, WM_USER + 10, 0, 0);
            if (LOWORD(wParam) == IDC_WINCOLOUR)
                SendMessage(hwndDlg, WM_USER + 11, 0, 0);
            if (LOWORD(wParam) == IDC_FILENAME && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
                return 0;
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            opt_clc_bkg_changed = 1;
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                                if(!opt_clc_bkg_changed)
                                    return TRUE;

                                DBWriteContactSettingByte(NULL, "CLC", "UseBitmap", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_BITMAP)); {
                                COLORREF col;
                                col = SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0);
                                if (col == CLCDEFAULT_BKCOLOUR)
                                    DBDeleteContactSetting(NULL, "CLC", "BkColour");
                                else
                                    DBWriteContactSettingDword(NULL, "CLC", "BkColour", col);
                                DBWriteContactSettingByte(NULL, "CLC", "UseWinColours", (BYTE)IsDlgButtonChecked(hwndDlg, IDC_WINCOLOUR));
                            } {
                                char str[MAX_PATH], strrel[MAX_PATH];
                                
                                GetDlgItemTextA(hwndDlg, IDC_FILENAME, str, sizeof(str));
                                if (CallService(MS_UTILS_PATHTORELATIVE, (WPARAM) str, (LPARAM) strrel))
                                    DBWriteContactSettingString(NULL, "CLC", "BkBitmap", strrel);
                                else
                                    DBWriteContactSettingString(NULL, "CLC", "BkBitmap", str);
                            } {
                                WORD flags = 0;
                                if (IsDlgButtonChecked(hwndDlg, IDC_STRETCHH))
                                    flags |= CLB_STRETCHH;
                                if (IsDlgButtonChecked(hwndDlg, IDC_STRETCHV))
                                    flags |= CLB_STRETCHV;
                                if (IsDlgButtonChecked(hwndDlg, IDC_TILEH))
                                    flags |= CLBF_TILEH;
                                if (IsDlgButtonChecked(hwndDlg, IDC_TILEV))
                                    flags |= CLBF_TILEV;
                                if (IsDlgButtonChecked(hwndDlg, IDC_SCROLL))
                                    flags |= CLBF_SCROLL;
                                if (IsDlgButtonChecked(hwndDlg, IDC_PROPORTIONAL))
                                    flags |= CLBF_PROPORTIONAL;
                                DBWriteContactSettingWord(NULL, "CLC", "BkBmpUse", flags);
                                g_CluiData.bWallpaperMode = IsDlgButtonChecked(hwndDlg, IDC_SKINMODE) ? 1 : 0;
                                DBWriteContactSettingByte(NULL, "CLUI", "UseBkSkin", (BYTE)g_CluiData.bWallpaperMode);
                            }
                            pcli->pfnClcOptionsChanged();
                            PostMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
                            opt_clc_bkg_changed = 0;
                            return TRUE;
                    }
                    break;
            }
            break;
    }
    return FALSE;
}

