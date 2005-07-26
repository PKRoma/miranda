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
#include "m_popup.h"
#include "msgs.h"
#include "urlctrl.h"

int LoadSendRecvMessageModule(void);
int SplitmsgShutdown(void);
int LogErrorMessage(HWND hwndDlg, struct MessageWindowData *dat, int i, char *szMsg);

PLUGINLINK *pluginLink;
HINSTANCE g_hInst;
extern MYGLOBALS myGlobals;

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
    PLUGIN_MAKE_VERSION(0, 9, 9, 97),
    "Send and receive instant messages, using a split mode interface and tab containers.",
    "The Miranda developers team",
    "silvercircle@gmail.com",
    "� 2000-2005 Miranda Project",
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
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 0))
        return NULL;
    return &pluginInfo;
}

int __declspec(dllexport) Load(PLUGINLINK * link)
{
    pluginLink = link;
    return LoadSendRecvMessageModule();
}

int __declspec(dllexport) Unload(void)
{
    return SplitmsgShutdown();
}

#ifdef _DEBUG
int _DebugTRACE(const char *fmt, ...)
{
    char    debug[1024];
    int     ibsize = 1023;
    
    va_list va;
    va_start(va, fmt);
    _vsnprintf(debug, ibsize, fmt, va);
    OutputDebugString(debug);
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
    POPUPDATA ppd;
    va_list va;
    char    debug[1024];
    int     ibsize = 1023;
    BYTE want_debuginfo = DBGetContactSettingByte(NULL, "Tab_SRMsg", "debuginfo", 0);

    if(!want_debuginfo)
        return 0;
    
    va_start(va, fmt);
    _vsnprintf(debug, ibsize, fmt, va);
    
    if(want_debuginfo == 2 && CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
        ZeroMemory((void *)&ppd, sizeof(ppd));
        ppd.lchContact = hContact;
        ppd.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
        if(hContact != 0)
            strncpy(ppd.lpzContactName, (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0), MAX_CONTACTNAME);
        else
            strncpy(ppd.lpzContactName, Translate("Global"), MAX_CONTACTNAME);
        strcpy(ppd.lpzText, "tabSRMM: ");
        strncat(ppd.lpzText, debug, MAX_SECONDLINE - 20);
        ppd.colorText = RGB(0,0,0);
        ppd.colorBack = RGB(255,0,0);
        CallService(MS_POPUP_ADDPOPUP, (WPARAM)&ppd, 0);
    }
    else {
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
    }
    return 0;
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
#if defined(_UNICODE)
                mir_snprintf(str,sizeof(str),"%s %d.%d.%d.%d (Unicode)", Translate("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)));
#else
				mir_snprintf(str,sizeof(str),"%s %d.%d.%d.%d", Translate("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)));
#endif                
				SetDlgItemTextA(hwndDlg,IDC_VERSION,str);
				mir_snprintf(str,sizeof(str),Translate("Built %s %s"),__DATE__,__TIME__);
				SetDlgItemTextA(hwndDlg,IDC_BUILDTIME,str);
			}
            hIcon = LoadIcon(GetModuleHandleA("miranda32.exe"), MAKEINTRESOURCE(102));
            SendDlgItemMessage(hwndDlg, IDC_LOGO, STM_SETICON, (WPARAM)hIcon, 0);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)myGlobals.g_iconContainer);
            urlctrl_set(GetDlgItem(hwndDlg, IDC_SUPPORT), _T("http://www.miranda.or.at/forums/index.php"), &url_unvisited, &url_visited, UCF_TXT_HCENTER, RGB(255, 255, 255));
            DestroyIcon(hIcon);
			return TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
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
			return TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
                case IDOK:
                {
                    DestroyWindow(hwndDlg);
                    return TRUE;
                }
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
			}
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
