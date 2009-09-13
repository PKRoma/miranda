/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * plugin loading functions and global exports.
 *
 */

#include "commonheaders.h"

extern int 	LoadSendRecvMessageModule(void);
extern int 	SplitmsgShutdown(void);
extern void LogErrorMessage(HWND hwndDlg, struct _MessageWindowData *dat, int i, TCHAR *szMsg);
extern int  Chat_Load(PLUGINLINK *link), Chat_Unload();
extern void FreeLogFonts();

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
#ifdef _WIN64_
	"TabSRMM (x64, Unicode)",
#else
#ifdef _UNICODE
	"TabSRMM (Unicode)",
#else
	"TabSRMM",
#endif
#endif
#endif
	PLUGIN_MAKE_VERSION(3, 0, 0, 7),
	"Chat module for instant messaging and group chat, offering a tabbed interface and many advanced features.",
	"The Miranda developers team and contributors",
	"silvercircle _at_ gmail _dot_ com",
	"2000-2009 Miranda Project and contributors. See readme.txt for more.",
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
	CMimAPI::m_MimVersion = mirandaVersion;

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

	CTranslator::preTranslateAll();

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

	_vsnwprintf(debug, ibsize - 10, fmt, va);
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
				mir_free(szDebug);
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
				char str[64];

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
					CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://miranda.or.at/TabSrmm:Development3_0_KnownIssues");
					break;
			}
			break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_BUILDTIME)
					|| (HWND)lParam == GetDlgItem(hwndDlg, IDC_COPYRIGHT)
					|| (HWND)lParam == GetDlgItem(hwndDlg, IDC_SUPPORT)) {
				SetTextColor((HDC)wParam, RGB(0, 0, 0));
				SetBkColor((HDC)wParam, RGB(255, 255, 255));
				return (INT_PTR)GetStockObject(WHITE_BRUSH);
			}
			break;
		case WM_PAINT: {
			PAINTSTRUCT 	ps;
			HDC 			hdc = BeginPaint(hwndDlg, &ps);
			RECT			rcClient;
			HDC				hdcMem = CreateCompatibleDC(hdc);
			bool			fAero = M->isAero(), fFree = false;
			GetClientRect(hwndDlg, &rcClient);
			LONG			cx = rcClient.right;
			LONG			cy = rcClient.bottom;
			HBITMAP			hbm = fAero ? CSkin::CreateAeroCompatibleBitmap(rcClient, hdc) : CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);;
			HBITMAP			hbmOld = reinterpret_cast<HBITMAP>(SelectObject(hdcMem, hbm));
			DWORD 			v = pluginInfo.version;
			TCHAR			str[80];

			char 			szVersion[512], *found = NULL, buildstr[50] = "";
			UINT 			build_nr = 0;
			SIZE 			sz;

			if(fAero) {
				MARGINS m;
				m.cxLeftWidth = m.cxRightWidth = 0;
				m.cyBottomHeight = 0;
				m.cyTopHeight = 50;
				if(CMimAPI::m_pfnDwmExtendFrameIntoClientArea)
					CMimAPI::m_pfnDwmExtendFrameIntoClientArea(hwndDlg, &m);
			}

			FillRect(hdcMem, &rcClient, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
			rcClient.bottom = 50;
			if(fAero) {
				FillRect(hdcMem, &rcClient, CSkin::m_BrushBack);
				CSkin::ApplyAeroEffect(hdcMem, &rcClient, CSkin::AERO_EFFECT_AREA_INFOPANEL);
			}
			else if(PluginConfig.m_WinVerMajor >= 5) {
				CSkinItem *item = &SkinItems[ID_EXTBKINFOPANELBG];
				DrawAlpha(hdcMem, &rcClient, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT, item->GRADIENT,
						  item->CORNER, item->BORDERSTYLE, 0);
			}
			if(PluginConfig.hbmLogo) {
				HBITMAP bmpLogo = CSkin::ResizeBitmap(PluginConfig.hbmLogo, 48, 48, fFree);

				HDC		hdcBmp = CreateCompatibleDC(hdc);
				HBITMAP hbmOldLogo = reinterpret_cast<HBITMAP>(SelectObject(hdcBmp, bmpLogo));
				CMimAPI::m_MyAlphaBlend(hdcMem, 3, 1, 48, 48, hdcBmp, 0, 0, 48, 48, CSkin::m_default_bf);
				SelectObject(hdcBmp, hbmOldLogo);
				DeleteDC(hdcBmp);
				if(fFree)
					DeleteObject(bmpLogo);
				rcClient.left = 60;
			}

			HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_COPYRIGHT, WM_GETFONT, 0, 0);
			LOGFONT lf = {0};

			GetObject(hFont, sizeof(lf), &lf);
			lf.lfHeight = (int)(lf.lfHeight * 1.3);
			lf.lfWeight = FW_BOLD;
			HFONT hFontBig = CreateFontIndirect(&lf);

			HFONT hFontOld = reinterpret_cast<HFONT>(SelectObject(hdcMem, hFontBig));

			rcClient.top = 1;
			rcClient.bottom = 48;

			GetTextExtentPoint32(hdcMem, _T("M"), 1, &sz);
			SetBkMode(hdcMem, TRANSPARENT);

			HANDLE hTheme = CMimAPI::m_pfnOpenThemeData ? CMimAPI::m_pfnOpenThemeData(hwndDlg, L"BUTTON") : 0;
			CSkin::RenderText(hdcMem, hTheme, _T("TabSRMM"), &rcClient, DT_SINGLELINE);

			SelectObject(hdcMem, hFont);
			DeleteObject(hFontBig);

			rcClient.top += (sz.cy + 5);

			CallService(MS_SYSTEM_GETVERSIONTEXT, 500, (LPARAM)szVersion);
			if ((found = strchr(szVersion, '#')) != NULL) {
				build_nr = atoi(found + 1);
				mir_snprintf(buildstr, 50, "[Build #%d]", build_nr);
			}
			TCHAR	*szBuildstr = mir_a2t(buildstr);
#if defined(_UNICODE)
			mir_sntprintf(str, safe_sizeof(str), _T("%s %d.%d.%d.%d (Unicode) %s"),
						 TranslateT("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)),
						 szBuildstr);
#else
			mir_snprintf(str, safe_sizeof(str), "%s %d.%d.%d.%d %s",
						 TranslateT("Version"), HIBYTE(HIWORD(v)), LOBYTE(HIWORD(v)), HIBYTE(LOWORD(v)), LOBYTE(LOWORD(v)),
						 szBuildstr);
#endif
			CSkin::RenderText(hdcMem, hTheme, str, &rcClient, DT_SINGLELINE);

			mir_free(szBuildstr);

			if(hTheme)
				CMimAPI::m_pfnCloseThemeData(hTheme);

			BitBlt(hdc, 0, 0, cx, cy, hdcMem, 0, 0, SRCCOPY);
			SelectObject(hdcMem, hbmOld);
			SelectObject(hdcMem, hFontOld);
			DeleteObject(hbm);
			DeleteDC(hdcMem);
			EndPaint(hwndDlg, &ps);
			return(0);
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
