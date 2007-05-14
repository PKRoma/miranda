/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project, 
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

#define DM_SETAVATARNAME (WM_USER + 10)
#define DM_REALODAVATAR (WM_USER + 11)
#define DM_AVATARCHANGED (WM_USER + 12)
#define DM_PROTOCOLCHANGED (WM_USER + 13)

extern int g_protocount;
extern int _DebugPopup(HANDLE hContact, const char *fmt, ...);
extern int SetAvatar(WPARAM wParam, LPARAM lParam);
extern struct protoPicCacheEntry *g_ProtoPictures;
extern HANDLE hEventChanged;
extern HINSTANCE g_hInst;
extern HICON g_hIcon;

extern int CreateAvatarInCache(HANDLE hContact, struct avatarCacheEntry *ace, char *szProto);
extern int ProtectAvatar(WPARAM wParam, LPARAM lParam);
extern int SetAvatarAttribute(HANDLE hContact, DWORD attrib, int mode);
extern int ChangeAvatar(HANDLE hContact, BOOL fLoad, BOOL fNotifyHist = FALSE, int pa_format = 0);
extern void DeleteAvatarFromCache(HANDLE, BOOL);
extern HBITMAP LoadPNG(struct avatarCacheEntry *ace, char *szFilename);
extern HANDLE GetContactThatHaveTheAvatar(HANDLE hContact, int locked = -1);

extern int AVS_pathToRelative(const char *sPrc, char *pOut);
extern int AVS_pathToAbsolute(const char *pSrc, char *pOut);
extern void MakePathRelative(HANDLE hContact, char *path);

extern int ProtoServiceExists(const char *szModule,const char *szService);
extern BOOL Proto_IsAvatarsEnabled(char *proto);

static BOOL dialoginit = TRUE;

struct WindowData {
    HANDLE hContact;
    HANDLE hHook;
};

static void RemoveProtoPic(const char *szProto)
{
    DBDeleteContactSetting(NULL, PPICT_MODULE, szProto);
    
    if(szProto) {
        for(int i = 0; i < g_protocount; i++) {
            if(g_ProtoPictures[i].szProtoname != NULL && !strcmp(g_ProtoPictures[i].szProtoname, szProto)) {
                if(g_ProtoPictures[i].hbmPic != 0)
                    DeleteObject(g_ProtoPictures[i].hbmPic);
                ZeroMemory((void *)&g_ProtoPictures[i], sizeof(struct avatarCacheEntry));
                NotifyEventHooks(hEventChanged, 0, (LPARAM)&g_ProtoPictures[i]);
            }
        }
    }
}

static void SetProtoPic(char *szProto)
{
    char FileName[MAX_PATH];
    OPENFILENAMEA ofn={0};
	char filter[256];

	filter[0] = '\0';
	CallService(MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof(filter), (LPARAM) filter);

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofn.lpstrFilter = filter;
    ofn.hwndOwner=0;
    ofn.lpstrFile = FileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.Flags=OFN_HIDEREADONLY;
    ofn.lpstrInitialDir = ".";
    *FileName = '\0';
    ofn.lpstrDefExt="";
    if (GetOpenFileNameA(&ofn)) {
        HANDLE hFile;
        
        if((hFile = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
            return;
        else {
            char szNewPath[MAX_PATH];
            int i;
            
            CloseHandle(hFile);
            AVS_pathToRelative(FileName, szNewPath);
            DBWriteContactSettingString(NULL, PPICT_MODULE, szProto, szNewPath);
            for(i = 0; i < g_protocount; i++) {
                if(lstrlenA(g_ProtoPictures[i].szProtoname) == 0)
                    break;
                if(!strcmp(g_ProtoPictures[i].szProtoname, szProto) && lstrlenA(g_ProtoPictures[i].szProtoname) == lstrlenA(szProto)) {
                    if(g_ProtoPictures[i].hbmPic != 0) 
                        DeleteObject(g_ProtoPictures[i].hbmPic);
                    ZeroMemory((void *)&g_ProtoPictures[i], sizeof(struct protoPicCacheEntry));
                    CreateAvatarInCache(0, (struct avatarCacheEntry *)&g_ProtoPictures[i], (char *)szProto);
                    strncpy(g_ProtoPictures[i].szProtoname, szProto, 100);
                    g_ProtoPictures[i].szProtoname[99] = 0;
                    NotifyEventHooks(hEventChanged, 0, (LPARAM)&g_ProtoPictures[i]);
                    break;
                }
            }
        }
    }
}

static char g_selectedProto[100];

BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndList = GetDlgItem(hwndDlg, IDC_PROTOCOLS);
    HWND hwndChoosePic = GetDlgItem(hwndDlg, IDC_SETPROTOPIC);
    HWND hwndRemovePic = GetDlgItem(hwndDlg, IDC_REMOVEPROTOPIC);
    
    switch (msg) {
        case WM_INITDIALOG: {
            LVITEMA item = {0};
            LVCOLUMN lvc = {0};
            int i = 0;
			UINT64 newItem = 0;
            
            dialoginit = TRUE;
            TranslateDialogDefault(hwndDlg);
            ListView_SetExtendedListViewStyle(hwndList, LVS_EX_CHECKBOXES);
            lvc.mask = LVCF_FMT;
            lvc.fmt = LVCFMT_IMAGE | LVCFMT_LEFT;
            ListView_InsertColumn(hwndList, 0, &lvc);
            
            item.mask = LVIF_TEXT;
            item.iItem = 1000;
            for(i = 0;;i++) {
                if(lstrlenA(g_ProtoPictures[i].szProtoname) == 0)
                    break;
                item.pszText = g_ProtoPictures[i].szProtoname;
				newItem = SendMessageA(hwndList, LVM_INSERTITEMA, 0, (LPARAM)&item);
                if(newItem >= 0)
                    ListView_SetCheckState(hwndList, newItem, DBGetContactSettingByte(NULL, AVS_MODULE, item.pszText, 1) ? TRUE : FALSE);
            }
            ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);
            ListView_Arrange(hwndList, LVA_ALIGNLEFT | LVA_ALIGNTOP);
            EnableWindow(hwndChoosePic, FALSE);
            EnableWindow(hwndRemovePic, FALSE);

            SendDlgItemMessage(hwndDlg, IDC_SIZELIMITSPIN, UDM_SETRANGE, 0, MAKELONG(1024, 0));
            SendDlgItemMessage(hwndDlg, IDC_SIZELIMITSPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingDword(0, AVS_MODULE, "SizeLimit", 70));

            CheckDlgButton(hwndDlg, IDC_SHOWWARNINGS, DBGetContactSettingByte(0, AVS_MODULE, "warnings", 0));
            CheckDlgButton(hwndDlg, IDC_MAKE_GRAYSCALE, DBGetContactSettingByte(0, AVS_MODULE, "MakeGrayscale", 0));
            CheckDlgButton(hwndDlg, IDC_MAKE_TRANSPARENT_BKG, DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0));
			CheckDlgButton(hwndDlg, IDC_MAKE_TRANSP_PROPORTIONAL, DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparencyProportionalToColorDiff", 0));
			CheckDlgButton(hwndDlg, IDC_MAKE_MY_AVATARS_TRANSP, DBGetContactSettingByte(0, AVS_MODULE, "MakeMyAvatarsTransparent", 0));
            CheckDlgButton(hwndDlg, IDC_SET_MAKE_SQUARE, DBGetContactSettingByte(0, AVS_MODULE, "SetAllwaysMakeSquare", 0));

			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETRANGE, 0, MAKELONG(8, 2));
			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgNumPoints", 5));

			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETRANGE, 0, MAKELONG(100, 0));
			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgColorDiff", 10));

			BOOL enabled = IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_L), enabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_SPIN), enabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS), enabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_L), enabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN), enabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE), enabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_MAKE_TRANSP_PROPORTIONAL), enabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_MAKE_MY_AVATARS_TRANSP), enabled);

            dialoginit = FALSE;
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_SETPROTOPIC:
                case IDC_REMOVEPROTOPIC:
                {
                    LVITEMA item = {0};
                    int iItem = ListView_GetSelectionMark(hwndList);
                    char szTemp[100];
                    
                    szTemp[0] = 0;
                    
                    item.mask = LVIF_TEXT;
                    item.pszText = szTemp;
                    item.cchTextMax = 100;
                    item.iItem = iItem;
					SendMessageA(hwndList, LVM_GETITEMA, 0, (LPARAM)&item);
                    if(lstrlenA(szTemp) > 1) {
                        if(LOWORD(wParam) == IDC_SETPROTOPIC)
                            SetProtoPic(szTemp);
                        else
                            RemoveProtoPic(szTemp);
                        SendMessage(hwndList, WM_LBUTTONDOWN, 0, 0);
                    }
                    break;
                }
                case IDC_SIZELIMIT:
                    if(HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus())
                        return 0;
                    else {
                        SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                        break;
                    }
                default:
					if ((LOWORD(wParam) == IDC_BKG_NUM_POINTS || LOWORD(wParam) == IDC_BKG_COLOR_DIFFERENCE || LOWORD(wParam) == IDC_SIZELIMIT)						&& (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
						return 0;
                    SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            }
            break;
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;

            if(dis->CtlType == ODT_BUTTON && dis->CtlID == IDC_PROTOPIC) {
                AVATARDRAWREQUEST avdrq = {0};
                avdrq.cbSize = sizeof(avdrq);
                avdrq.hTargetDC = dis->hDC;
                avdrq.dwFlags |= AVDRQ_PROTOPICT;
                avdrq.szProto = g_selectedProto;
                GetClientRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), &avdrq.rcDraw);
                CallService(MS_AV_DRAWAVATAR, 0, (LPARAM)&avdrq);
            }
            return TRUE;
        }
        case WM_NOTIFY:
            if(dialoginit)
                break;
            switch (((LPNMHDR) lParam)->idFrom) {
                case IDC_PROTOCOLS:
                    switch (((LPNMHDR) lParam)->code) {
                        case LVN_ITEMCHANGED:
                            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                            //break;
                            //LVN_
                        case NM_CLICK:
                        {
                            int iItem = ListView_GetSelectionMark(hwndList);
                            LVITEMA item = {0};
                            DBVARIANT dbv = {0};
                            g_selectedProto[0] = 0;

                            item.mask = LVIF_TEXT;
                            item.pszText = g_selectedProto;
                            item.cchTextMax = 100;
                            item.iItem = iItem;
							SendMessageA(hwndList, LVM_GETITEMA, 0, (LPARAM)&item);
                            g_selectedProto[99] = 0;
                            EnableWindow(hwndChoosePic, TRUE);
                            EnableWindow(hwndRemovePic, TRUE);
                            if(!DBGetContactSetting(NULL, PPICT_MODULE, g_selectedProto, &dbv)) {
                                char szFinalPath[MAX_PATH];
                                
                                AVS_pathToAbsolute(dbv.pszVal, szFinalPath);
                                SetWindowTextA(GetDlgItem(hwndDlg, IDC_PROTOAVATARNAME), szFinalPath);
                                InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
                                DBFreeVariant(&dbv);
                            }
                            else {
                                SetWindowTextA(GetDlgItem(hwndDlg, IDC_PROTOAVATARNAME), "");
                                InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
                            }
                            break;
                        }
                    }
                    break;
				case IDC_MAKE_TRANSPARENT_BKG:
				{
					BOOL transp_enabled = IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_L), transp_enabled);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_SPIN), transp_enabled);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS), transp_enabled);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_L), transp_enabled);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN), transp_enabled);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE), transp_enabled);
					EnableWindow(GetDlgItem(hwndDlg, IDC_MAKE_TRANSP_PROPORTIONAL), transp_enabled);
					EnableWindow(GetDlgItem(hwndDlg, IDC_MAKE_MY_AVATARS_TRANSP), transp_enabled);
					break;
				}
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                        {
                            LVITEMA item = {0};
                            int i;
                            char szTemp[110];
                            
                            item.mask = LVIF_TEXT;
                            item.pszText = szTemp;
                            item.cchTextMax = 100;
                            for(i = 0; i < ListView_GetItemCount(hwndList); i++) {
                                item.iItem = i;
								SendMessageA(hwndList, LVM_GETITEMA, 0, (LPARAM)&item);
                                if(ListView_GetCheckState(hwndList, i))
                                    DBWriteContactSettingByte(NULL, AVS_MODULE, item.pszText, 1);
                                else
                                    DBWriteContactSettingByte(NULL, AVS_MODULE, item.pszText, 0);
                            }
                            DBWriteContactSettingDword(NULL, AVS_MODULE, "SizeLimit", (DWORD)SendDlgItemMessage(hwndDlg, IDC_SIZELIMITSPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingByte(NULL, AVS_MODULE, "warnings", IsDlgButtonChecked(hwndDlg, IDC_SHOWWARNINGS) ? 1 : 0);
							DBWriteContactSettingByte(NULL, AVS_MODULE, "MakeGrayscale", IsDlgButtonChecked(hwndDlg, IDC_MAKE_GRAYSCALE) ? 1 : 0);
                            DBWriteContactSettingByte(NULL, AVS_MODULE, "MakeTransparentBkg", IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSPARENT_BKG) ? 1 : 0);
							DBWriteContactSettingByte(NULL, AVS_MODULE, "MakeTransparencyProportionalToColorDiff", IsDlgButtonChecked(hwndDlg, IDC_MAKE_TRANSP_PROPORTIONAL) ? 1 : 0);
							DBWriteContactSettingByte(NULL, AVS_MODULE, "MakeMyAvatarsTransparent", IsDlgButtonChecked(hwndDlg, IDC_MAKE_MY_AVATARS_TRANSP) ? 1 : 0);
							DBWriteContactSettingWord(NULL, AVS_MODULE, "TranspBkgNumPoints", (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_GETPOS, 0, 0));
							DBWriteContactSettingWord(NULL, AVS_MODULE, "TranspBkgColorDiff", (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_GETPOS, 0, 0));
							DBWriteContactSettingByte(NULL, AVS_MODULE, "SetAllwaysMakeSquare", IsDlgButtonChecked(hwndDlg, IDC_SET_MAKE_SQUARE) ? 1 : 0);
                        }
                    }
            }
            break;
        case WM_DESTROY:
            break;
    }
    return FALSE;
}


void LoadTransparentData(HWND hwndDlg, HANDLE hContact)
{
	CheckDlgButton(hwndDlg, IDC_MAKETRANSPBKG, DBGetContactSettingByte(hContact, "ContactPhoto", "MakeTransparentBkg", DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0)));
	SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgNumPoints", DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgNumPoints", 5)));
	SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETPOS, 0, (LPARAM)DBGetContactSettingWord(hContact, "ContactPhoto", "TranspBkgColorDiff", DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgColorDiff", 10)));

	BOOL transp_enabled = IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_L), transp_enabled);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_SPIN), transp_enabled);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS), transp_enabled);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_L), transp_enabled);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN), transp_enabled);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE), transp_enabled);
}

void SaveTransparentData(HWND hwndDlg, HANDLE hContact)
{
	BOOL transp = IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG);
	if (DBGetContactSettingByte(0, AVS_MODULE, "MakeTransparentBkg", 0) == transp)
		DBDeleteContactSetting(hContact, "ContactPhoto", "MakeTransparentBkg");
	else
		DBWriteContactSettingByte(hContact, "ContactPhoto", "MakeTransparentBkg", transp);

	WORD tmp = (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_GETPOS, 0, 0);
	if (DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgNumPoints", 5) == tmp)
		DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgNumPoints");
	else
		DBWriteContactSettingWord(hContact, "ContactPhoto", "TranspBkgNumPoints", tmp);

	tmp = (WORD) SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_GETPOS, 0, 0);
	if (DBGetContactSettingWord(0, AVS_MODULE, "TranspBkgColorDiff", 10) == tmp)
		DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgColorDiff");
	else
		DBWriteContactSettingWord(hContact, "ContactPhoto", "TranspBkgColorDiff", tmp);
}

void SaveTransparentData(HWND hwndDlg, HANDLE hContact, BOOL locked)
{
	SaveTransparentData(hwndDlg, hContact);

	HANDLE tmp = GetContactThatHaveTheAvatar(hContact, locked);
	if (tmp != hContact)
		SaveTransparentData(hwndDlg, tmp);
}

BOOL CALLBACK DlgProcAvatarOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact;
    struct WindowData *dat = (struct WindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);

    if(dat)
        hContact = dat->hContact;

    switch(msg) {
        case WM_INITDIALOG:
        {
            TCHAR szTitle[512];
            TCHAR *szNick = NULL;
            struct WindowData *dat = (struct WindowData *)malloc(sizeof(struct WindowData));

            if(dat)
                dat->hContact = (HANDLE)lParam;

            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
            hContact = (HANDLE)lParam;
            TranslateDialogDefault(hwndDlg);
            if(hContact) {
                szNick = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR);
				mir_sntprintf(szTitle, 500, TranslateT("Set avatar options for %s"), szNick);
                SetWindowText(hwndDlg, szTitle);
            }
            SendMessage(hwndDlg, DM_SETAVATARNAME, 0, 0);
            ShowWindow(hwndDlg, SW_SHOWNORMAL);
            InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, FALSE);
            CheckDlgButton(hwndDlg, IDC_PROTECTAVATAR, DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0) ? TRUE : FALSE);
            CheckDlgButton(hwndDlg, IDC_HIDEAVATAR, DBGetContactSettingByte(hContact, "ContactPhoto", "Hidden", 0) ? TRUE : FALSE);

			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS), 0);
			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETRANGE, 0, MAKELONG(8, 2));

			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE), 0);
			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETRANGE, 0, MAKELONG(100, 0));

			LoadTransparentData(hwndDlg, GetContactThatHaveTheAvatar(hContact));
            dat->hHook = HookEventMessage(ME_AV_AVATARCHANGED, hwndDlg, DM_AVATARCHANGED);
            SendMessage(hwndDlg, WM_SETICON, IMAGE_ICON, (LPARAM)g_hIcon);
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
				case ID_USE_DEFAULTS:
				{
					hContact = GetContactThatHaveTheAvatar(hContact);

					DBDeleteContactSetting(hContact, "ContactPhoto", "MakeTransparentBkg");
					DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgNumPoints");
					DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgColorDiff");

					LoadTransparentData(hwndDlg, hContact);

					SendMessage(hwndDlg, DM_REALODAVATAR, 0, 0);
					break;
				}
                case IDOK:
                {
                    BOOL locked = IsDlgButtonChecked(hwndDlg, IDC_PROTECTAVATAR);
					int hidden = IsDlgButtonChecked(hwndDlg, IDC_HIDEAVATAR) ? 1 : 0;
					SetAvatarAttribute(hContact, AVS_HIDEONCLIST, hidden);
                    if(hidden != DBGetContactSettingByte(hContact, "ContactPhoto", "Hidden", 0))
						DBWriteContactSettingByte(hContact, "ContactPhoto", "Hidden", hidden);

					if (!locked && DBGetContactSettingByte(hContact, "ContactPhoto", "NeedUpdate", 0))
						QueueAdd(requestQueue, hContact);

					// Continue to the cancel handle
                }
                case IDCANCEL:
                    DestroyWindow(hwndDlg);
                    break;
                case IDC_PROTECTAVATAR:
                {
                    BOOL locked = IsDlgButtonChecked(hwndDlg, IDC_PROTECTAVATAR);
                    ProtectAvatar((WPARAM)hContact, locked ? 1 : 0);
                    break;
                }
                case IDC_CHANGE:
				{
                    SetAvatar((WPARAM)hContact, 0);
                    SendMessage(hwndDlg, DM_SETAVATARNAME, 0, 0);
                    CheckDlgButton(hwndDlg, IDC_PROTECTAVATAR, DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0) ? TRUE : FALSE);
                    break;
				}
				case IDC_BKG_NUM_POINTS:
				case IDC_BKG_COLOR_DIFFERENCE:
					if (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())
						break;
				case IDC_MAKETRANSPBKG:
				{
					BOOL enable = IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_L), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_SPIN), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_L), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE), enable);

                    SendMessage(hwndDlg, DM_REALODAVATAR, 0, 0);
                    break;
				}
                case IDC_RESET:
                {
                    char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					DBVARIANT dbv = {0};

                    ProtectAvatar((WPARAM)hContact, 0);
					if(MessageBox(0, TranslateT("Delete picture file from disk (may be necessary to force a reload, but will delete local pictures)?"), TranslateT("Reset contact picture"), MB_YESNO) == IDYES) {
						if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
							DeleteFileA(dbv.pszVal);
							DBFreeVariant(&dbv);
						}
					}
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Locked");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Backup");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "RFile");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "File");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Format");
                    DBDeleteContactSetting(hContact, szProto, "AvatarHash");
                    DBDeleteContactSetting(hContact, szProto, "AvatarSaved");
					DeleteAvatarFromCache(hContact, FALSE);

					QueueAdd(requestQueue, hContact);

                    DestroyWindow(hwndDlg);
                    break;
                }
                case IDC_DELETE:
				{
					DBVARIANT dbv = {0};
                    ProtectAvatar((WPARAM)hContact, 0);
					if(MessageBox(0, TranslateT("Delete picture file from disk (may be necessary to force a reload, but will delete local pictures)?"), TranslateT("Reset contact picture"), MB_YESNO) == IDYES) {
						if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
							DeleteFileA(dbv.pszVal);
							DBFreeVariant(&dbv);
						}
					}
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Locked");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Backup");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "RFile");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "File");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Format");
					DeleteAvatarFromCache(hContact, FALSE);
                    SendMessage(hwndDlg, DM_SETAVATARNAME, 0, 0);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
                    break;
				}
            }
            break;
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;

            if(dis->CtlType == ODT_BUTTON && dis->CtlID == IDC_PROTOPIC) {
                AVATARDRAWREQUEST avdrq = {0};
				GetClientRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), &avdrq.rcDraw);

				FillRect(dis->hDC, &avdrq.rcDraw, GetSysColorBrush(COLOR_BTNFACE));

                avdrq.hContact = hContact;
                avdrq.cbSize = sizeof(avdrq);
                avdrq.hTargetDC = dis->hDC;
                avdrq.dwFlags |= AVDRQ_DRAWBORDER;
                avdrq.clrBorder = GetSysColor(COLOR_BTNTEXT);
                avdrq.radius = 6;
				if (!CallService(MS_AV_DRAWAVATAR, 0, (LPARAM)&avdrq)) 
				{
					// Get text rectangle
					RECT rc = avdrq.rcDraw;
					rc.top += 10;
					rc.bottom -= 10;
					rc.left += 10;
					rc.right -= 10;

					// Calc text size
					RECT rc_ret = rc;
					DrawText(dis->hDC, TranslateT("Contact has no avatar"), -1, &rc_ret, 
							DT_WORDBREAK | DT_NOPREFIX | DT_CENTER | DT_CALCRECT);
					
					// Calc needed size
					rc.top += ((rc.bottom - rc.top) - (rc_ret.bottom - rc_ret.top)) / 2;
					rc.bottom = rc.top + (rc_ret.bottom - rc_ret.top);
					DrawText(dis->hDC, TranslateT("Contact has no avatar"), -1, &rc, 
							DT_WORDBREAK | DT_NOPREFIX | DT_CENTER);
				}

				FrameRect(dis->hDC, &avdrq.rcDraw, GetSysColorBrush(COLOR_BTNSHADOW));
            }
            return TRUE;
        }
        case DM_SETAVATARNAME:
        {
            char szFinalName[MAX_PATH];
            DBVARIANT dbv = {0};
            BYTE is_locked = DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0);

            szFinalName[0] = 0;
            
            if(is_locked && !DBGetContactSetting(hContact, "ContactPhoto", "Backup", &dbv)) {
                AVS_pathToAbsolute(dbv.pszVal, szFinalName);
                DBFreeVariant(&dbv);
            }
            else if(!DBGetContactSetting(hContact, "ContactPhoto", "RFile", &dbv)) {
                AVS_pathToAbsolute(dbv.pszVal, szFinalName);
                DBFreeVariant(&dbv);
            }
            else if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
                AVS_pathToAbsolute(dbv.pszVal, szFinalName);
                DBFreeVariant(&dbv);
            }
            szFinalName[MAX_PATH - 1] = 0;
            SetDlgItemTextA(hwndDlg, IDC_AVATARNAME, szFinalName);
            break;
        }
        case DM_REALODAVATAR:
        {
			SaveTransparentData(hwndDlg, hContact, IsDlgButtonChecked(hwndDlg, IDC_PROTECTAVATAR));
            ChangeAvatar(hContact, TRUE);
			InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
            break;
        }
        case DM_AVATARCHANGED:
            InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOPIC), NULL, TRUE);
            break;
        case WM_NCDESTROY:
        {
            if(dat) {
                UnhookEvent(dat->hHook);
                free(dat);
            }
            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            break;
        }
    }
    return FALSE;
}

BOOL CALLBACK DlgProcAvatarUserInfo(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact;
    struct WindowData *dat = (struct WindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);

    if(dat)
        hContact = dat->hContact;

    switch(msg) {
        case WM_INITDIALOG:
        {
            dat = (struct WindowData *) malloc(sizeof(struct WindowData));
            if (dat == NULL)
				return FALSE;
            dat->hContact = (HANDLE)lParam;

			HWND protopic = GetDlgItem(hwndDlg, IDC_PROTOPIC);
            SendMessage(protopic, AVATAR_SETCONTACT, 0, (LPARAM) dat->hContact);
			SendMessage(protopic, AVATAR_SETBKGCOLOR, 0, (LPARAM) GetSysColor(COLOR_BTNFACE));
			SendMessage(protopic, AVATAR_SETBORDERCOLOR, 0, (LPARAM) GetSysColor(COLOR_BTNSHADOW));
			SendMessage(protopic, AVATAR_SETAVATARBORDERCOLOR, 0, (LPARAM) GetSysColor(COLOR_BTNSHADOW));
			SendMessage(protopic, AVATAR_SETNOAVATARTEXT, 0, (LPARAM) "Contact has no avatar");
			SendMessage(protopic, AVATAR_RESPECTHIDDEN, 0, (LPARAM) FALSE);

            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
            hContact = (HANDLE)lParam;
            TranslateDialogDefault(hwndDlg);
            SendMessage(hwndDlg, DM_SETAVATARNAME, 0, 0);
            CheckDlgButton(hwndDlg, IDC_PROTECTAVATAR, DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0) ? TRUE : FALSE);
            CheckDlgButton(hwndDlg, IDC_HIDEAVATAR, DBGetContactSettingByte(hContact, "ContactPhoto", "Hidden", 0) ? TRUE : FALSE);

			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS), 0);
			SendDlgItemMessage(hwndDlg, IDC_BKG_NUM_POINTS_SPIN, UDM_SETRANGE, 0, MAKELONG(8, 2));

			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE), 0);
			SendDlgItemMessage(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN, UDM_SETRANGE, 0, MAKELONG(100, 0));

			LoadTransparentData(hwndDlg, GetContactThatHaveTheAvatar(hContact));
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
				case ID_USE_DEFAULTS:
				{
					hContact = GetContactThatHaveTheAvatar(hContact);

					DBDeleteContactSetting(hContact, "ContactPhoto", "MakeTransparentBkg");
					DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgNumPoints");
					DBDeleteContactSetting(hContact, "ContactPhoto", "TranspBkgColorDiff");

					LoadTransparentData(hwndDlg, hContact);

					SendMessage(hwndDlg, DM_REALODAVATAR, 0, 0);
					break;
				}
                case IDC_CHANGE:
				{
                    SetAvatar((WPARAM)hContact, 0);
                    SendMessage(hwndDlg, DM_SETAVATARNAME, 0, 0);
                    CheckDlgButton(hwndDlg, IDC_PROTECTAVATAR, DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0) ? TRUE : FALSE);
                    break;
				}
				case IDC_HIDEAVATAR:
				{
                    int hidden = IsDlgButtonChecked(hwndDlg, IDC_HIDEAVATAR) ? 1 : 0;
                    SetAvatarAttribute(hContact, AVS_HIDEONCLIST, hidden);
                    if(hidden != DBGetContactSettingByte(hContact, "ContactPhoto", "Hidden", 0))
						DBWriteContactSettingByte(hContact, "ContactPhoto", "Hidden", hidden);
					break;
				}
				case IDC_PROTECTAVATAR:
				{
					BOOL locked = IsDlgButtonChecked(hwndDlg, IDC_PROTECTAVATAR);
					SaveTransparentData(hwndDlg, hContact, locked);
                    ProtectAvatar((WPARAM)hContact, locked ? 1 : 0);

					break;
				}
				case IDC_BKG_NUM_POINTS:
				case IDC_BKG_COLOR_DIFFERENCE:
					if (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())
						break;
				case IDC_MAKETRANSPBKG:
				{
					BOOL enable = IsDlgButtonChecked(hwndDlg, IDC_MAKETRANSPBKG);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_L), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS_SPIN), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_NUM_POINTS), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_L), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE_SPIN), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG_COLOR_DIFFERENCE), enable);

                    SendMessage(hwndDlg, DM_REALODAVATAR, 0, 0);
                    break;
				}
                case IDC_RESET:
                {
                    char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					DBVARIANT dbv = {0};

                    ProtectAvatar((WPARAM)hContact, 0);
					if(MessageBox(0, TranslateT("Delete picture file from disk (may be necessary to force a reload, but will delete local pictures)?"), TranslateT("Reset contact picture"), MB_YESNO) == IDYES) {
						if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
							DeleteFileA(dbv.pszVal);
							DBFreeVariant(&dbv);
						}
					}
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Locked");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Backup");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "RFile");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "File");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Format");
                    DBDeleteContactSetting(hContact, szProto, "AvatarHash");
                    DBDeleteContactSetting(hContact, szProto, "AvatarSaved");
					DeleteAvatarFromCache(hContact, FALSE);

					QueueAdd(requestQueue, hContact);
                    break;
                }
                case IDC_DELETE:
				{
					DBVARIANT dbv = {0};
					
                    ProtectAvatar((WPARAM)hContact, 0);
					if(MessageBox(0, TranslateT("Delete picture file from disk (may be necessary to force a reload, but will delete local pictures)?"), TranslateT("Reset contact picture"), MB_YESNO) == IDYES) {
						if(!DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv)) {
							DeleteFileA(dbv.pszVal);
							DBFreeVariant(&dbv);
						}
					}
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Locked");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Backup");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "RFile");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "File");
                    DBDeleteContactSetting(hContact, "ContactPhoto", "Format");
                    DeleteAvatarFromCache(hContact, FALSE);
                    SendMessage(hwndDlg, DM_SETAVATARNAME, 0, 0);
                    break;
				}
            }
            break;
        case DM_REALODAVATAR:
        {
			SaveTransparentData(hwndDlg, hContact, IsDlgButtonChecked(hwndDlg, IDC_PROTECTAVATAR));
            ChangeAvatar(hContact, TRUE);
            break;
        }
        case WM_NCDESTROY:
        {
            if(dat)
                free(dat);
            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            break;
        }
    }
    return FALSE;
}

static char * GetSelectedProtocol(HWND hwndDlg)
{
	HWND hwndList = GetDlgItem(hwndDlg, IDC_PROTOCOLS);

	// Get selection
    int iItem = ListView_GetSelectionMark(hwndList);
	if (iItem < 0)
		return NULL;

	// Get protocol name
    LVITEMA item = {0};
    item.mask = LVIF_PARAM;
    item.iItem = iItem;
	SendMessageA(hwndList, LVM_GETITEMA, 0, (LPARAM)&item);
	return (char *) item.lParam;
}

static void EnableDisableControls(HWND hwndDlg)
{
	char *proto = GetSelectedProtocol(hwndDlg);
	if (proto == NULL)
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHANGE), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHANGE), TRUE);

		int width, height;
		SendMessage(GetDlgItem(hwndDlg, IDC_PROTOPIC), AVATAR_GETUSEDSPACE, (WPARAM) &width, (LPARAM) &height);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), (LPARAM) width != 0 || height != 0);
	}
}

BOOL CALLBACK DlgProcAvatarProtoInfo(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) 
	{
        case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);

			HWND protopic = GetDlgItem(hwndDlg, IDC_PROTOPIC);
			SendMessage(protopic, AVATAR_SETAVATARBORDERCOLOR, 0, (LPARAM) GetSysColor(COLOR_BTNSHADOW));
			SendMessage(protopic, AVATAR_SETNOAVATARTEXT, 0, (LPARAM) "Protocol has no avatar");
			SendMessage(protopic, AVATAR_SETRESIZEIFSMALLER, 0, (LPARAM) FALSE);

		    HWND hwndList = GetDlgItem(hwndDlg, IDC_PROTOCOLS);
			ListView_SetExtendedListViewStyleEx(hwndList, 0, LVS_EX_SUBITEMIMAGES);
            
			HIMAGELIST hIml = ImageList_Create(16, 16, ILC_MASK | (IsWinVerXPPlus()? ILC_COLOR32 : ILC_COLOR16), 4, 0);
			ListView_SetImageList(hwndList, hIml, LVSIL_SMALL);

            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_FMT;
            lvc.fmt = LVCFMT_IMAGE | LVCFMT_LEFT;
            ListView_InsertColumn(hwndList, 0, &lvc);

            LVITEMA item = {0};
            item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
            item.iItem = 1000;

			// List protocols
			PROTOCOLDESCRIPTOR **protos;
			int i, count, num = 0;
			char description[256];

			CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&count, (LPARAM)&protos);
			for (i = 0; i < count; i++)
			{
				if (protos[i]->type != PROTOTYPE_PROTOCOL)
					continue;

				if (protos[i]->szName == NULL || protos[i]->szName[0] == '\0')
					continue;

				if (!ProtoServiceExists(protos[i]->szName, PS_SETMYAVATAR))
					continue;

				if (!Proto_IsAvatarsEnabled(protos[i]->szName))
					continue;
				
				CallProtoService(protos[i]->szName, PS_GETNAME, sizeof(description),(LPARAM) description);

				ImageList_AddIcon(hIml, LoadSkinnedProtoIcon(protos[i]->szName, ID_STATUS_ONLINE));
				item.pszText = description;
				item.iImage = num;
				item.lParam = (LPARAM) protos[i]->szName;

				SendMessageA(hwndList, LVM_INSERTITEMA, 0, (LPARAM)&item);
				num++;
			}

            ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);
            ListView_Arrange(hwndList, LVA_ALIGNLEFT | LVA_ALIGNTOP);
			ListView_SetItemState(hwndList, 0, LVIS_FOCUSED | LVIS_SELECTED, 0x0F);
            return TRUE;
        }
        case WM_NOTIFY:
		{
			LPNMHDR nm = (LPNMHDR) lParam;
            switch(nm->idFrom)
			{
				case IDC_PROTOCOLS:
				{
					switch (nm->code) 
					{
						case LVN_ITEMCHANGED:
						{
							LPNMLISTVIEW li = (LPNMLISTVIEW) nm;
							if (li->uNewState & LVIS_SELECTED)
							{
								SendMessage(GetDlgItem(hwndDlg, IDC_PROTOPIC), AVATAR_SETPROTOCOL, 0, li->lParam);
								EnableDisableControls(hwndDlg);
							}
							break;
						}
					}
					break;
				}
				case IDC_PROTOPIC:
				{
					if (nm->code) 
					{
						case NM_AVATAR_CHANGED:
						{
							EnableDisableControls(hwndDlg);
							break;
						}
					}
					break;
				}
			}
			break;
		}
        case WM_COMMAND:
		{
            switch(LOWORD(wParam)) 
			{
                case IDC_CHANGE:
				{
					char *proto = GetSelectedProtocol(hwndDlg);
					if (proto == NULL)
						break;

					CallService(MS_AV_SETMYAVATAR, (WPARAM) proto, 0);
                    break;
				}
                case IDC_DELETE:
				{
					char *proto = GetSelectedProtocol(hwndDlg);
					if (proto == NULL)
						break;

					char description[256];
					CallProtoService(proto, PS_GETNAME, sizeof(description),(LPARAM) description);
					if (MessageBoxA(hwndDlg, Translate("Are you sure you want to remove your avatar?"), description, MB_YESNO) == IDYES)
						CallService(MS_AV_SETMYAVATAR, (WPARAM) proto, (LPARAM) "");
                    break;
				}
            }
            break;
		}
    }
    return FALSE;
}
