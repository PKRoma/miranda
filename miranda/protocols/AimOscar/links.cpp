/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003 Robert Rainwater

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

#include "aim.h"
#include "links.h"

#define AIMWATCHERCLASS "__AIMWatcherClass__"

static void aim_links_normalize(char *szMsg)
{
	char *s = szMsg;
	while (*s) {
		if (*s == '+')
			*s = ' ';
		s++;
	}
}

static LRESULT CALLBACK aim_links_watcherwndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_COPYDATA:
		{
			char *szData=NULL, *s;
			COPYDATASTRUCT *cds = (COPYDATASTRUCT *) lParam;
			CAimProto* ppro = (CAimProto*)GetWindowLong( hwnd, GWL_USERDATA );

			//LOG(LOG_DEBUG, "Links: WM_COPYDATA");
			// Check to see if link support is enabled
			// We shouldn't need this check since the class instance shouldn't be running
			// but lets be safe
			if ( !ppro->getByte( AIM_KEY_AL, 0))
				break;
			if (!(char *) cds->lpData)
				break;
			s = szData = strldup((char *) cds->lpData,lstrlen((char*)cds->lpData));
			aim_links_normalize(szData);
			s += 4;
			if (!_strnicmp(s, "addbuddy?", 9)) { // group is current ignored
				char *tok, *sn = NULL, *group = NULL;
				ADDCONTACTSTRUCT acs;
				PROTOSEARCHRESULT psr;

				//LOG(LOG_DEBUG, "Links: WM_COPYDATA - addbuddy?");
				s += 9;
				tok = strtok(s, "&");
				while (tok != NULL)
				{
					if (!_strnicmp(tok, "screenname=", 11))
					{
						sn = tok + 11;
					}
					if (!_strnicmp(tok, "groupname=", 10))
					{
						group = tok + 10;
					}
					tok = strtok(NULL, "&");
				}
				if ( sn && lstrlen(sn) && !ppro->find_contact(sn))
				{
					acs.handleType = HANDLE_SEARCHRESULT;
					acs.szProto = ppro->m_szModuleName;
					acs.psr = &psr;
					memset( &psr, 0, sizeof( PROTOSEARCHRESULT ));
					psr.cbSize = sizeof( PROTOSEARCHRESULT );
					psr.nick = sn;
					CallService(MS_ADDCONTACT_SHOW,(WPARAM)NULL,(LPARAM)&acs);
				}
			}
			else if (!_strnicmp(s, "goim?", 5))
			{
				char *tok, *sn = NULL, *msg = NULL;

				//LOG(LOG_DEBUG, "Links: WM_COPYDATA - goim");
				s += 5;
				if (!(*s))
				{
					delete[] szData;
					break;
				}
				tok = strtok(s, "&");
				while (tok != NULL)
				{
					if (!_strnicmp(tok, "screenname=", 11))
					{
						sn = tok + 11;
					}
					if (!_strnicmp(tok, "message=", 8))
					{
						msg = tok + 8;
					}
					tok = strtok(NULL, "&");
				}
				if (sn && ServiceExists(MS_MSG_SENDMESSAGE))
				{
					HANDLE hContact = ppro->find_contact(sn);
					if(!hContact)
					{
						hContact = ppro->add_contact(sn);
						DBWriteContactSettingByte(hContact,MOD_KEY_CL,AIM_KEY_NL,1);
					}
					if (hContact)
						CallService(MS_MSG_SENDMESSAGE, (WPARAM) hContact, (LPARAM) msg);
				}
			}
			delete[] szData;
			break;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CAimProto::aim_links_regwatcher()
{
	WNDCLASS wc;

	//LOG(LOG_DEBUG, "Links: regwatcher");
	if (hWatcher || aWatcherClass) {
		return;
	}
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = aim_links_watcherwndproc;
	wc.hInstance = hInstance;
	wc.lpszClassName = AIMWATCHERCLASS;
	aWatcherClass = RegisterClass(&wc);
	hWatcher = CreateWindowEx(0, AIMWATCHERCLASS, "", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	SetWindowLong( hWatcher, GWL_USERDATA, LPARAM(this));
	return;
}

void CAimProto::aim_links_unregwatcher()
{
	//LOG(LOG_DEBUG, "Links: unregwatcher");
	if (hWatcher) {
		DestroyWindow(hWatcher);
		hWatcher = NULL;
	}
	if (aWatcherClass) {
		UnregisterClass(AIMWATCHERCLASS, hInstance);
		aWatcherClass = 0;
	}
	return;
}

static BOOL CALLBACK aim_linsk_enumthreadwindowsproc(HWND hwnd, LPARAM lParam)
{
	char szBuf[64];

	if (GetClassName(hwnd, szBuf, sizeof(szBuf))) {
		if (lstrcmp(szBuf, AIMWATCHERCLASS)) {
			COPYDATASTRUCT cds;

			//LOG(LOG_DEBUG, "Links: enumthreadwindowsproc - found AIMWATCHERCLASS");
			cds.dwData = 1;
			cds.cbData = lstrlen((char *) lParam) + 1;
			cds.lpData = (char *) lParam;
			SendMessageTimeout(hwnd, WM_COPYDATA, (WPARAM) hwnd, (LPARAM) & cds, SMTO_ABORTIFHUNG | SMTO_BLOCK, 150, NULL);
		}
	};
	return TRUE;
}

static BOOL CALLBACK aim_links_enumwindowsproc(HWND hwnd, LPARAM lParam)
{
	char szBuf[32];

	//LOG(LOG_DEBUG, "Links: enumwindowsproc");
	if (GetClassName(hwnd, szBuf, sizeof(szBuf)))
	{
		if (!lstrcmp(szBuf, MIRANDACLASS)) {
			//LOG(LOG_DEBUG, "Links: enumwindowsproc - found Miranda window");
			EnumThreadWindows(GetWindowThreadProcessId(hwnd, NULL), aim_linsk_enumthreadwindowsproc, lParam);
		}
	}
	return TRUE;
}

static void aim_links_register()
{
	HKEY hkey;
	char szBuf[MAX_PATH], szExe[MAX_PATH * 2], szShort[MAX_PATH];

	GetModuleFileName(hInstance, szBuf, sizeof(szBuf));
	GetShortPathName(szBuf, szShort, sizeof(szShort));
	//LOG(LOG_DEBUG, "Links: register");
	if (RegCreateKey(HKEY_CLASSES_ROOT, "aim", &hkey) == ERROR_SUCCESS) {
		RegSetValue(hkey, NULL, REG_SZ, "URL:AIM Protocol", lstrlen("URL:AIM Protocol"));
		RegSetValueEx(hkey, "URL Protocol", 0, REG_SZ, (PBYTE) "", 1);
		RegCloseKey(hkey);
	}
	else {
		//LOG(LOG_ERROR, "Links: register - unable to create registry key (root)");
		return;
	}
	if (RegCreateKey(HKEY_CLASSES_ROOT, "aim\\DefaultIcon", &hkey) == ERROR_SUCCESS) {
		char szIcon[MAX_PATH];

		mir_snprintf(szIcon, sizeof(szIcon), "%s,0", szShort);
		RegSetValue(hkey, NULL, REG_SZ, szIcon, lstrlen(szIcon));
		RegCloseKey(hkey);
	}
	else {
		//LOG(LOG_ERROR, "Links: register - unable to create registry key (DefaultIcon)");
		return;
	}
	if (RegCreateKey(HKEY_CLASSES_ROOT, "aim\\shell\\open\\command", &hkey) == ERROR_SUCCESS) {
		// MSVC exports differently than gcc/mingw
#ifdef _MSC_VER
		mir_snprintf(szExe, sizeof(szExe), "RUNDLL32.EXE %s,_aim_links_exec@16 %%1", szShort);
		//LOG(LOG_INFO, "Links: registering (%s)", szExe);
#else
		mir_snprintf(szExe, sizeof(szExe), "RUNDLL32.EXE %s,aim_links_exec@16 %%1", szShort);
		//LOG(LOG_INFO, "Links: registering (%s)", szExe);
#endif
		RegSetValue(hkey, NULL, REG_SZ, szExe, lstrlen(szExe));
		RegCloseKey(hkey);
	}
	else {
		//LOG(LOG_ERROR, "Links: register - unable to create registry key (command)");
		return;
	}
}

void CAimProto::aim_links_unregister()
{
	//LOG(LOG_DEBUG, "Links: unregister");
	RegDeleteKey(HKEY_CLASSES_ROOT, "aim\\shell\\open\\command");
	RegDeleteKey(HKEY_CLASSES_ROOT, "aim\\shell\\open");
	RegDeleteKey(HKEY_CLASSES_ROOT, "aim\\shell");
	RegDeleteKey(HKEY_CLASSES_ROOT, "aim\\DefaultIcon");
	RegDeleteKey(HKEY_CLASSES_ROOT, "aim");
}

void CAimProto::aim_links_init()
{
	//LOG(LOG_DEBUG, "Links: init");
	if (getByte( AIM_KEY_AL, 0)) {
		//LOG(LOG_DEBUG, "Links: init - links support is on");
		aim_links_register();
		aim_links_regwatcher();
	}
}

void CAimProto::aim_links_destroy()
{
	// LOG(LOG_DEBUG, "Links: destroy");
	aim_links_unregwatcher();
}

extern "C" void __declspec(dllexport)
CALLBACK aim_links_exec(HWND /*hwnd*/, HINSTANCE /*hInst*/, char *lpszCmdLine, int /*nCmdShow*/)
{
	EnumWindows(aim_links_enumwindowsproc, (LPARAM) lpszCmdLine);
}
