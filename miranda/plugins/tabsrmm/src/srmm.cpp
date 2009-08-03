/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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

$Id: srmm.c 10384 2009-07-20 19:02:32Z silvercircle $
*/

#include "commonheaders.h"

extern int 	LoadSendRecvMessageModule(void);
extern int 	SplitmsgShutdown(void);
extern void LogErrorMessage(HWND hwndDlg, struct _MessageWindowData *dat, int i, TCHAR *szMsg);
extern int  Chat_Load(PLUGINLINK *link), Chat_Unload();
extern void FreeLogFonts();

DWORD g_mirandaVersion = 0;

PLUGINLINK *pluginLink;
HINSTANCE g_hInst;
LOGFONT lfDefault = {0};

/*
 * miranda interfaces
 */

struct LIST_INTERFACE li;
struct MM_INTERFACE mmi;

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
#ifdef __GNUWIN32__
	"TabSRMM (MINGW32)",
#else
	"TabSRMM",
#endif
	PLUGIN_MAKE_VERSION(3, 0, 0, 2),
	"Chat module for instant messaging and group chat, offering a tabbed interface and many advanced features.",
	"The Miranda developers team and contributors",
	"silvercircle@gmail.com",
	"? 2000-2009 Miranda Project",
	"http://miranda.or.at",
	UNICODE_AWARE,
	DEFMOD_SRMESSAGE,            // replace internal version (if any)
#ifdef _UNICODE
	{0x6ca5f042, 0x7a7f, 0x47cc, { 0xa7, 0x15, 0xfc, 0x8c, 0x46, 0xfb, 0xf4, 0x34 }} //{6CA5F042-7A7F-47cc-A715-FC8C46FBF434}
#else
	{0x5889a3ef, 0x7c95, 0x4249, { 0x98, 0xbb, 0x34, 0xe9, 0x5, 0x3a, 0x6e, 0xa0 }} //{5889A3EF-7C95-4249-98BB-34E9053A6EA0}
#endif
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	g_hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX *MirandaPluginInfoEx(DWORD mirandaVersion)
{
	g_mirandaVersion = mirandaVersion;
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 9, 0, 0)) {
		MessageBox(0, _T("This version of tabSRMM requires Miranda 0.9.0 or later. The plugin cannot be loaded."), _T("tabSRMM"), MB_OK | MB_ICONERROR);
		return NULL;
	}
	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_SRMM, MIID_CHAT, MIID_LAST};

extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK * link)
{
	pluginLink = link;

	mir_getMMI(&mmi);
	mir_getLI(&li);

	M = new CMimAPI();

	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lfDefault), &lfDefault, FALSE);

	Chat_Load(pluginLink);
	return LoadSendRecvMessageModule();
}

extern "C" int __declspec(dllexport) Unload(void)
{
	FreeLogFonts();
	Chat_Unload();
	int iRet = SplitmsgShutdown();
	delete Skin;
	delete sendQueue;
	delete M;
	return iRet;
}

#if defined(_UNICODE)
int _DebugTraceW(const wchar_t *fmt, ...)
{
	wchar_t debug[2048];
	int     ibsize = 2047;
	va_list va;
	va_start(va, fmt);

	lstrcpyW(debug, L"TABSRMM: ");

	_vsnwprintf(&debug[9], ibsize - 10, fmt, va);
#ifdef _DEBUG
	OutputDebugStringW(debug);
#else
	{
		char szLogFileName[MAX_PATH], szDataPath[MAX_PATH];
		FILE *f;

		CallService(MS_DB_GETPROFILEPATH, MAX_PATH, (LPARAM)szDataPath);
		mir_snprintf(szLogFileName, MAX_PATH, "%s\\%s", szDataPath, "tabsrmm_debug.log");
		f = fopen(szLogFileName, "a+");
		if (f) {
			char *szDebug = M->utf8_encodeW(debug);
			fputs(szDebug, f);
			fputs("\n", f);
			fclose(f);
			if (szDebug)
				free(szDebug);
		}
	}
#endif
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
 #ifdef _DEBUG
 	OutputDebugStringA(debug);
 #else
	{
		char szLogFileName[MAX_PATH], szDataPath[MAX_PATH];
		FILE *f;

		CallService(MS_DB_GETPROFILEPATH, MAX_PATH, (LPARAM)szDataPath);
		mir_snprintf(szLogFileName, MAX_PATH, "%s\\%s", szDataPath, "tabsrmm_debug.log");
		f = fopen(szLogFileName, "a+");
		if (f) {
			fputs(debug, f);
			fputs("\n", f);
			fclose(f);
		}
	}
#endif
	return 0;
}

/*
 * output a notification message.
 * may accept a hContact to include the contacts nickname in the notification message...
 * the actual message is using printf() rules for formatting and passing the arguments...
 *
 * can display the message either as systray notification (baloon popup) or using the
 * popup plugin.
 */

int _DebugPopup(HANDLE hContact, const TCHAR *fmt, ...)
{
	va_list	va;
	TCHAR		debug[1024];
	int			ibsize = 1023;

	va_start(va, fmt);
	_vsntprintf(debug, ibsize, fmt, va);

	if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
		MIRANDASYSTRAYNOTIFY tn;
		TCHAR	szTitle[128];

		tn.szProto = NULL;
		tn.cbSize = sizeof(tn);
		mir_sntprintf(szTitle, safe_sizeof(szTitle), TranslateT("tabSRMM Message (%s)"), (hContact != 0) ? (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR) : TranslateT("Global"));
#if defined(_UNICODE)
		tn.tszInfoTitle = szTitle;
		tn.tszInfo = debug;
#else
		tn.szInfoTitle = szTitle;
		tn.szInfo = debug;
#endif
		tn.dwInfoFlags = NIIF_INFO;
#if defined(_UNICODE)
		tn.dwInfoFlags |= NIIF_INTERN_UNICODE;
#endif
		tn.uTimeout = 1000 * 4;
		CallService(MS_CLIST_SYSTRAY_NOTIFY, 0, (LPARAM) & tn);
	}
	return 0;
}

INT_PTR CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	COLORREF url_visited = RGB(128, 0, 128);
	COLORREF url_unvisited = RGB(0, 0, 255);

	switch (msg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			{
				int h;
				HFONT hFont;
				LOGFONT lf;

				hFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_TABSRMM, WM_GETFONT, 0, 0);
				GetObject(hFont, sizeof(lf), &lf);
				h = lf.lfHeight;
				lf.lfHeight = (int)(lf.lfHeight * 1.5);
				lf.lfWeight = FW_BOLD;
				hFont = CreateFontIndirect(&lf);
				SendDlgItemMessage(hwndDlg, IDC_TABSRMM, WM_SETFONT, (WPARAM)hFont, 0);
				lf.lfHeight = h;
				hFont = CreateFontIndirect(&lf);
				SendDlgItemMessage(hwndDlg, IDC_VERSION, WM_SETFONT, (WPARAM)hFont, 0);
			}
			{
				char str[64];
				DWORD v = pluginInfo.version;
				char szVersion[512], *found = NULL, buildstr[50] = "";
				UINT build_nr = 0;

				CallService(MS_SYSTEM_GETVERSIONTEXT, 500, (LPARAM)szVersion);
				if ((found = strchr(szVersion, '#')) != NULL) {
					build_nr = atoi(found + 1);
					mir_snprintf(buildstr, 50, "[Build #%d]", build_nr);
				}
#if defined(_UNICODE)
				mir_snprintf(str, sizeof(str), "%s %d.%d.%d.%d (Unicode) %s", Translate("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)), buildstr);
#else
				mir_snprintf(str, sizeof(str), "%s %d.%d.%d.%d %s", Translate("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)), buildstr);
#endif
				SetDlgItemTextA(hwndDlg, IDC_VERSION, str);
				mir_snprintf(str, sizeof(str), Translate("Built %s %s"), __DATE__, __TIME__);
				SetDlgItemTextA(hwndDlg, IDC_BUILDTIME, str);
			}
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PluginConfig.g_iconContainer);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDC_SUPPORT:
					CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://wiki.miranda.or.at/");
					break;
			}
			break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_WHITERECT)
					|| (HWND)lParam == GetDlgItem(hwndDlg, IDC_TABSRMM)
					|| (HWND)lParam == GetDlgItem(hwndDlg, IDC_VERSION)
					|| (HWND)lParam == GetDlgItem(hwndDlg, IDC_BUILDTIME)
					|| (HWND)lParam == GetDlgItem(hwndDlg, IDC_COPYRIGHT)
					|| (HWND)lParam == GetDlgItem(hwndDlg, IDC_SUPPORT)
					|| (HWND)lParam == GetDlgItem(hwndDlg, IDC_LOGO)) {
				if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TABSRMM))
					SetTextColor((HDC)wParam, RGB(180, 10, 10));
				else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_VERSION))
					SetTextColor((HDC)wParam, RGB(70, 70, 70));
				else
					SetTextColor((HDC)wParam, RGB(0, 0, 0));
				SetBkColor((HDC)wParam, RGB(255, 255, 255));
				return (INT_PTR)GetStockObject(WHITE_BRUSH);
			}
			break;
		case WM_DESTROY: {
			HFONT hFont;

			hFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_TABSRMM, WM_GETFONT, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_TABSRMM, WM_SETFONT, SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT, 0, 0), 0);
			DeleteObject(hFont);
			hFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_VERSION, WM_GETFONT, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_VERSION, WM_SETFONT, SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT, 0, 0), 0);
			DeleteObject(hFont);
		}
		break;
	}
	return FALSE;
}

#define _DBG_STR_LENGTH_MAX 2048

int _DebugMessage(HWND hwndDlg, struct _MessageWindowData *dat, const char *fmt, ...)
{
	va_list va;
	char            szDebug[_DBG_STR_LENGTH_MAX];

	va_start(va, fmt);
	_vsnprintf(szDebug, _DBG_STR_LENGTH_MAX, fmt, va);
	szDebug[_DBG_STR_LENGTH_MAX - 1] = '\0';


	// LogErrorMessage(hwndDlg, dat, -1, szDebug);
	return 0;
}
