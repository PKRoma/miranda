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

int LoadSendRecvMessageModule(void);
int SplitmsgShutdown(void);
int LogErrorMessage(HWND hwndDlg, struct MessageWindowData *dat, int i, char *szMsg);
void Chat_Load(PLUGINLINK *link);
void Chat_Unload();

DWORD g_mirandaVersion = 0;

PLUGINLINK *pluginLink;
HINSTANCE g_hInst;
extern MYGLOBALS myGlobals;
struct MM_INTERFACE memoryManagerInterface;

PLUGININFO pluginInfo = {
    sizeof(PLUGININFO),
#ifdef _UNICODE
    #ifdef __GNUWIN32__
        "tabSRMsgW (MINGW32 - unicode)",
    #else    
        "tabSRMsgW (unicode)",
    #endif    
#else
    #ifdef __GNUWIN32__
        "tabSRMsg (MINGW32)",
    #else    
        "tabSRMsg",
    #endif    
#endif
    PLUGIN_MAKE_VERSION(0, 9, 9, 206),
    "Chat module for instant messaging and group chat, offering a tabbed interface and many advanced features.",
    "The Miranda developers team",
    "silvercircle@gmail.com",
    "� 2000-2006 Miranda Project",
    "http://tabsrmm.sourceforge.net",
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
    g_mirandaVersion = mirandaVersion;
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 0))
        return NULL;
    return &pluginInfo;
}

int __declspec(dllexport) Load(PLUGINLINK * link)
{
    pluginLink = link;

    memset(&memoryManagerInterface, 0, sizeof(memoryManagerInterface));
    memoryManagerInterface.cbSize = sizeof(memoryManagerInterface);
    CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM) &memoryManagerInterface);
    
	Chat_Load(pluginLink);
	return LoadSendRecvMessageModule();
}

int __declspec(dllexport) Unload(void)
{
    Chat_Unload();
	return SplitmsgShutdown();
}

#ifdef _DEBUG
#if defined(_UNICODE)
int _DebugTraceW(const wchar_t *fmt, ...)
{
    wchar_t debug[2048];
    int     ibsize = 2047;
    va_list va;
    va_start(va, fmt);

	lstrcpyW(debug, L"TABSRMM: ");

    _vsnwprintf(&debug[9], ibsize - 10, fmt, va);
    OutputDebugStringW(debug);
	return 0;
}
#endif
int _DebugTraceA(const char *fmt, ...)
{
    char    debug[2048];
    int     ibsize = 2047;
    va_list va;
    va_start(va, fmt);

	lstrcpyA(debug, "TABSRMM: ");
	_vsnprintf(&debug[9], ibsize - 10, fmt, va);
    OutputDebugStringA(debug);
	return 0;
}
#endif

/*
 * output a notification message.
 * may accept a hContact to include the contacts nickname in the notification message...
 * the actual message is using printf() rules for formatting and passing the arguments...
 *
 * can display the message either as systray notification (baloon popup) or using the
 * popup plugin.
 */

int _DebugPopup(HANDLE hContact, const char *fmt, ...)
{
    va_list va;
    char    debug[1024];
    int     ibsize = 1023;

    va_start(va, fmt);
    _vsnprintf(debug, ibsize, fmt, va);
    
    if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
        MIRANDASYSTRAYNOTIFY tn;
        char szTitle[128];
        
        tn.szProto = NULL;
        tn.cbSize = sizeof(tn);
        _snprintf(szTitle, sizeof(szTitle), Translate("tabSRMM Message (%s)"), (hContact != 0) ? (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0) : Translate("Global"));
        tn.szInfoTitle = szTitle;
        tn.szInfo = debug;
        tn.dwInfoFlags = NIIF_INFO;
        tn.uTimeout = 1000 * 4;
        CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM) & tn);
    }
    return 0;
}

BOOL CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HICON hIcon;
    COLORREF url_visited = RGB(128, 0, 128);
    COLORREF url_unvisited = RGB(0, 0, 255);
    
    switch (msg)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			{	int h;
                HFONT hFont;
				LOGFONT lf;
                
				hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_TABSRMM,WM_GETFONT,0,0);
				GetObject(hFont,sizeof(lf),&lf);
                h=lf.lfHeight;
				lf.lfHeight=(int)(lf.lfHeight*1.5);
				lf.lfWeight=FW_BOLD;
				hFont=CreateFontIndirect(&lf);
				SendDlgItemMessage(hwndDlg,IDC_TABSRMM,WM_SETFONT,(WPARAM)hFont,0);
				lf.lfHeight=h;
				hFont=CreateFontIndirect(&lf);
				SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_SETFONT,(WPARAM)hFont,0);
			}
			{	char str[64];
                DWORD v = pluginInfo.version;
				char szVersion[512], *found = NULL, buildstr[50] = "";
				UINT build_nr = 0;

				CallService(MS_SYSTEM_GETVERSIONTEXT, 500, (LPARAM)szVersion);
				if((found = strchr(szVersion, '#')) != NULL) {
					build_nr = atoi(found + 1);
					mir_snprintf(buildstr, 50, "[Build #%d]", build_nr);
				}
#if defined(_UNICODE)
				mir_snprintf(str,sizeof(str),"%s %d.%d.%d.%d (Unicode) %s", Translate("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)), buildstr);
#else
				mir_snprintf(str,sizeof(str),"%s %d.%d.%d.%d %s", Translate("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)), buildstr);
#endif                
				SetDlgItemTextA(hwndDlg,IDC_VERSION,str);
				mir_snprintf(str,sizeof(str),Translate("Built %s %s"),__DATE__,__TIME__);
				SetDlgItemTextA(hwndDlg,IDC_BUILDTIME,str);
			}
            hIcon = LoadIcon(GetModuleHandleA("miranda32.exe"), MAKEINTRESOURCE(102));
            SendDlgItemMessage(hwndDlg, IDC_LOGO, STM_SETICON, (WPARAM)hIcon, 0);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)myGlobals.g_iconContainer);
            DestroyIcon(hIcon);
			return TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDC_SUPPORT:
					CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://miranda.or.at/");
					break;
			}
			break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_TABSRMM)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_VERSION)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_BUILDTIME)
                    || (HWND)lParam==GetDlgItem(hwndDlg,IDC_COPYRIGHT)
                    || (HWND)lParam==GetDlgItem(hwndDlg,IDC_SUPPORT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO)) {
                if((HWND)lParam==GetDlgItem(hwndDlg,IDC_TABSRMM))
				    SetTextColor((HDC)wParam,RGB(180,10,10));
                else if((HWND)lParam==GetDlgItem(hwndDlg,IDC_VERSION))
				    SetTextColor((HDC)wParam,RGB(70,70,70));
                else
				    SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
			break;
		case WM_DESTROY:
			{	HFONT hFont;
            
				hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_TABSRMM,WM_GETFONT,0,0);
				SendDlgItemMessage(hwndDlg,IDC_TABSRMM,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0),0);
				DeleteObject(hFont);				
                hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_GETFONT,0,0);
                SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0),0);
                DeleteObject(hFont);				
			}
			break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcFirsttime(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)myGlobals.g_iconContainer);
            ShowWindow(hwndDlg, SW_SHOWNORMAL);
            EnableWindow(GetDlgItem(hwndDlg, IDC_IEVEIW), ServiceExists(MS_IEVIEW_EVENT) ? TRUE : FALSE);
            CheckDlgButton(hwndDlg, IDC_COMPACTTHEME, TRUE);
            CheckDlgButton(hwndDlg, IDC_SHOWAVATARS, TRUE);
            CheckDlgButton(hwndDlg, IDC_SHOWINFOPANEL, TRUE);
			return TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
                case IDOK:
                {
                    char szFilename[MAX_PATH];
                    HANDLE hFile;
                    
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", IsDlgButtonChecked(hwndDlg, IDC_SHOWAVATARS) ? 3 : 4);
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "want_ieview", IsDlgButtonChecked(hwndDlg, IDC_IEVIEWMODE) ? 1 : 0);
                    if(IsDlgButtonChecked(hwndDlg, IDC_COMPACTTHEME))
                        CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)"plugins\\tabsrmm\\themes\\compact.tabsrmm", (LPARAM)szFilename);
                    if(IsDlgButtonChecked(hwndDlg, IDC_FANCYTHEME))
                        CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)"plugins\\tabsrmm\\themes\\fancy.tabsrmm", (LPARAM)szFilename);
                    
                    if((hFile = CreateFileA(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE ) {
                        CloseHandle(hFile);
                        ReadThemeFromINI(szFilename, 0, 0);
                        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "firstrun", 1);
                    }
                    else
                        MessageBoxA(0, "The selected theme was not installed, because the file couldn't be found.\nYou may want to reinstall tabSRMM", "tabSRMM", MB_OK | MB_ICONEXCLAMATION);
                    
                    DestroyWindow(hwndDlg);
                    return TRUE;
                }
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
			}
			break;
		case WM_DESTROY:
            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "firstrun", 1);
			break;
	}
	return FALSE;
}

void FirstTimeConfig()
{
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "firstrun", 0))
        return;
    CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_FIRSTTIME), 0, DlgProcFirsttime, 0);
}

int _DebugMessage(HWND hwndDlg, struct MessageWindowData *dat, const char *fmt, ...)
{
    va_list va;
    char    debug[1024];
    int     ibsize = 1023;

    va_start(va, fmt);
    _vsnprintf(debug, ibsize, fmt, va);

    LogErrorMessage(hwndDlg, dat, -1, debug);
    return 0;
}
