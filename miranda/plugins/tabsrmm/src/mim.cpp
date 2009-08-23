/*

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

$Id$

*/


#include "commonheaders.h"
extern PLUGININFOEX pluginInfo;

PITA 	CMimAPI::m_pfnIsThemeActive = 0;
POTD 	CMimAPI::m_pfnOpenThemeData = 0;
PDTB 	CMimAPI::m_pfnDrawThemeBackground = 0;
PCTD 	CMimAPI::m_pfnCloseThemeData = 0;
PDTT 	CMimAPI::m_pfnDrawThemeText = 0;
PDTTE	CMimAPI::m_pfnDrawThemeTextEx = 0;
PITBPT 	CMimAPI::m_pfnIsThemeBackgroundPartiallyTransparent = 0;
PDTPB  	CMimAPI::m_pfnDrawThemeParentBackground = 0;
PGTBCR 	CMimAPI::m_pfnGetThemeBackgroundContentRect = 0;
ETDT 	CMimAPI::m_pfnEnableThemeDialogTexture = 0;
PSLWA 	CMimAPI::m_pSetLayeredWindowAttributes = 0;
PULW	CMimAPI::m_pUpdateLayeredWindow = 0;
PFWEX	CMimAPI::m_MyFlashWindowEx = 0;
PAB		CMimAPI::m_MyAlphaBlend = 0;
PGF		CMimAPI::m_MyGradientFill = 0;
SMI		CMimAPI::m_fnSetMenuInfo = 0;
DEFICA	CMimAPI::m_pfnDwmExtendFrameIntoClientArea = 0;
DICE	CMimAPI::m_pfnDwmIsCompositionEnabled = 0;
MMFW	CMimAPI::m_pfnMonitorFromWindow = 0;
GMIA	CMimAPI::m_pfnGetMonitorInfoA = 0;
DRT		CMimAPI::m_pfnDwmRegisterThumbnail = 0;
BPI		CMimAPI::m_pfnBufferedPaintInit = 0;
BPU		CMimAPI::m_pfnBufferedPaintUninit = 0;
BBP		CMimAPI::m_pfnBeginBufferedPaint = 0;
EBP		CMimAPI::m_pfnEndBufferedPaint = 0;
BBW		CMimAPI::m_pfnDwmBlurBehindWindow = 0;
DGC		CMimAPI::m_pfnDwmGetColorizationColor = 0;

bool	CMimAPI::m_shutDown = 0;
TCHAR*	CMimAPI::m_userDir = 0;

/*
 * read a setting for a contact
 */

DWORD CMimAPI::GetDword(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, DWORD uDefault = 0) const
{
	return((DWORD)DBGetContactSettingDword(hContact, szModule, szSetting, uDefault));
}

/*
 * read a setting from our default module (Tab_SRMSG)
 */

DWORD CMimAPI::GetDword(const char *szSetting = 0, DWORD uDefault = 0) const
{
	return((DWORD)DBGetContactSettingDword(0, SRMSGMOD_T, szSetting, uDefault));
}

/*
 * read a contact setting with our default module name (Tab_SRMSG)
 */

DWORD CMimAPI::GetDword(const HANDLE hContact = 0, const char *szSetting = 0, DWORD uDefault = 0) const
{
	return((DWORD)DBGetContactSettingDword(hContact, SRMSGMOD_T, szSetting, uDefault));
}

/*
 * read a setting from module only
 */

DWORD CMimAPI::GetDword(const char *szModule, const char *szSetting, DWORD uDefault) const
{
	return((DWORD)DBGetContactSettingDword(0, szModule, szSetting, uDefault));
}

/*
 * same for bytes now
 */
int CMimAPI::GetByte(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, int uDefault = 0) const
{
	return(DBGetContactSettingByte(hContact, szModule, szSetting, uDefault));
}

int CMimAPI::GetByte(const char *szSetting = 0, int uDefault = 0) const
{
	return(DBGetContactSettingByte(0, SRMSGMOD_T, szSetting, uDefault));
}

int CMimAPI::GetByte(const HANDLE hContact = 0, const char *szSetting = 0, int uDefault = 0) const
{
	return(DBGetContactSettingByte(hContact, SRMSGMOD_T, szSetting, uDefault));
}

int CMimAPI::GetByte(const char *szModule, const char *szSetting, int uDefault) const
{
	return(DBGetContactSettingByte(0, szModule, szSetting, uDefault));
}

INT_PTR CMimAPI::GetTString(const HANDLE hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv) const
{
	return(DBGetContactSettingTString(hContact, szModule, szSetting, dbv));
}

INT_PTR CMimAPI::GetString(const HANDLE hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv) const
{
	return(DBGetContactSettingString(hContact, szModule, szSetting, dbv));
}

/*
 * writer functions
 */

INT_PTR CMimAPI::WriteDword(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, DWORD value = 0) const
{
	return(DBWriteContactSettingDword(hContact, szModule, szSetting, value));
}

/*
 * write non-contact setting
*/

INT_PTR CMimAPI::WriteDword(const char *szModule = 0, const char *szSetting = 0, DWORD value = 0) const
{
	return(DBWriteContactSettingDword(0, szModule, szSetting, value));
}

INT_PTR CMimAPI::WriteByte(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, BYTE value = 0) const
{
	return(DBWriteContactSettingByte(hContact, szModule, szSetting, value));
}

INT_PTR CMimAPI::WriteByte(const char *szModule = 0, const char *szSetting = 0, BYTE value = 0) const
{
	return(DBWriteContactSettingByte(0, szModule, szSetting, value));
}

INT_PTR CMimAPI::WriteTString(const HANDLE hContact, const char *szModule = 0, const char *szSetting = 0, const TCHAR *str = 0) const
{
	return(DBWriteContactSettingTString(hContact, szModule, szSetting, str));
}

void CMimAPI::GetUTFI()
{
	mir_getUTFI(&m_utfi);
}
char *CMimAPI::utf8_decode(char* str, wchar_t** ucs2) const
{
	return(m_utfi.utf8_decode(str, ucs2));
}
char *CMimAPI::utf8_decodecp(char* str, int codepage, wchar_t** ucs2 ) const
{
	return(m_utfi.utf8_decodecp(str, codepage, ucs2));
}
char *CMimAPI::utf8_encode(const char* src) const
{
	return(m_utfi.utf8_encode(src));
}

char *CMimAPI::utf8_encodecp(const char* src, int codepage) const
{
	return(m_utfi.utf8_encodecp(src, codepage));
}
char *CMimAPI::utf8_encodeW(const wchar_t* src) const
{
	return(m_utfi.utf8_encodeW(src));
}
wchar_t *CMimAPI::utf8_decodeW(const char* str) const
{
	return(m_utfi.utf8_decodeW(str));
}

/**
 * Case insensitive _tcsstr
 *
 * @param szString TCHAR *: String to be searched
 * @param szSearchFor
 *                 TCHAR *: String that should be found in szString
 *
 * @return TCHAR *: found position of szSearchFor in szString. 0 if szSearchFor was not found
 */
const TCHAR* CMimAPI::StriStr(const TCHAR *szString, const TCHAR *szSearchFor)
{
	assert(szString != 0 && szSearchFor != 0);

	if(szString && *szString) {
		if (0 == szSearchFor || 0 == *szSearchFor)
			return(szString);

		for(; *szString; ++szString) {
			if(_totupper(*szString) == _totupper(*szSearchFor)) {
				const TCHAR *h, *n;
				for(h = szString, n = szSearchFor; *h && *n; ++h, ++n) {
					if(_totupper(*h) != _totupper(*n))
						break;
				}
				if(!*n)
					return(szString);
			}
		}
		return(0);
	}
	else
		return(0);
}

int CMimAPI::pathIsAbsolute(const TCHAR *path) const
{
	if (!path || !(lstrlen(path) > 2))
		return 0;
	if ((path[1] == ':' && path[2] == '\\') || (path[0] == '\\' && path[1] == '\\'))
		return 1;
	return 0;
}

size_t CMimAPI::pathToRelative(const TCHAR *pSrc, TCHAR *pOut) const
{
	pOut[0] = 0;
	if (!pSrc || !lstrlen(pSrc) || lstrlen(pSrc) > MAX_PATH)
		return 0;
	if (!pathIsAbsolute(pSrc)) {
		mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);
		return lstrlen(pOut);
	} else {
		TCHAR	szTmp[MAX_PATH];
		TCHAR 	szSTmp[MAX_PATH];
		mir_sntprintf(szTmp, SIZEOF(szTmp), _T("%s"), pSrc);
		if (m_szSkinsPath[0])
			mir_sntprintf(szSTmp, SIZEOF(szSTmp), _T("%s"), m_szSkinsPath);

		if(StriStr(szTmp, _T(".tsk")) && szSTmp && StriStr(szTmp, szSTmp)) {
			mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc + lstrlen(m_szSkinsPath));
			return(lstrlen(pOut));
		} else if (StriStr(szTmp, m_szProfilePath)) {
			mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc + lstrlen(m_szProfilePath) - 1);
			pOut[0]='.';
			return(lstrlen(pOut));
		} else {
			mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);
			return(lstrlen(pOut));
		}
	}
}

size_t CMimAPI::pathToRelative(const TCHAR *pSrc, TCHAR *pOut, TCHAR *szBase) const
{
	pOut[0] = 0;
	if (!pSrc||!lstrlen(pSrc)||lstrlen(pSrc) > MAX_PATH)
		return 0;
	if (!pathIsAbsolute(pSrc)) {
		mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);
		return lstrlen(pOut);
	}
	else {
		TCHAR szTmp[MAX_PATH];

		mir_sntprintf(szTmp, SIZEOF(szTmp), _T("%s"), pSrc);
		if (StriStr(szTmp, szBase)) {
			mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc + lstrlen(szBase));
			return(lstrlen(pOut));
		}
		else {
			mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);
			return(lstrlen(pOut));
		}
	}
}

size_t CMimAPI::pathToAbsolute(const TCHAR *pSrc, TCHAR *pOut) const
{
	pOut[0] = 0;
	if (!pSrc || !lstrlen(pSrc) || lstrlen(pSrc) > MAX_PATH)
		return 0;
	if (pathIsAbsolute(pSrc) && pSrc[0]!='.')
		mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);

	else if (m_szSkinsPath[0] && StriStr(pSrc, _T(".tsk")) && pSrc[0] != '.')
		mir_sntprintf(pOut, MAX_PATH, _T("%s%s"), m_szSkinsPath, pSrc);

	else if (pSrc[0]=='.')
		mir_sntprintf(pOut, MAX_PATH, _T("%s\\%s"), m_szProfilePath, pSrc + 1);
	else
		mir_sntprintf(pOut, MAX_PATH, _T("%s\\%s"), m_szProfilePath, pSrc);

	return lstrlen(pOut);
}

/*
 * ansi versions
 * they will go away at some point...
 */

#if defined(UNICODE)

int CMimAPI::pathIsAbsolute(const char *path) const
{
	if (!path || !(lstrlenA(path) > 2))
		return 0;
	if ((path[1] == ':' && path[2] == '\\') || (path[0] == '\\' && path[1] == '\\'))
		return 1;
	return 0;
}

size_t CMimAPI::pathToRelative(const char *pSrc, char *pOut) const
{
	pOut[0] = 0;
	if (!pSrc || !lstrlenA(pSrc) || lstrlenA(pSrc) > MAX_PATH)
		return 0;
	if (!pathIsAbsolute(pSrc)) {
		mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
		return lstrlenA(pOut);
	} else {
		char	szTmp[MAX_PATH];
		char 	szSTmp[MAX_PATH];
		mir_snprintf(szTmp, SIZEOF(szTmp), "%s", pSrc);
		strlwr(szTmp);
		if (m_szSkinsPathA[0]) {
			mir_snprintf(szSTmp, SIZEOF(szSTmp), "%s", m_szSkinsPathA);
			strlwr(szSTmp);
		}
		if (strstr(szTmp, ".tsk") && szSTmp && strstr(szTmp, szSTmp)) {
			mir_snprintf(pOut, MAX_PATH, "%s", pSrc + lstrlenA(m_szSkinsPathA));
			return lstrlenA(pOut);
		} else if (strstr(szTmp, m_szProfilePathA)) {
			mir_snprintf(pOut, MAX_PATH, "%s", pSrc + lstrlenA(m_szProfilePathA) - 1);
			pOut[0]='.';
			return lstrlenA(pOut);
		} else {
			mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
			return lstrlenA(pOut);
		}
	}
}

size_t CMimAPI::pathToAbsolute(const char *pSrc, char *pOut) const
{
	pOut[0] = 0;
	if (!pSrc || !lstrlenA(pSrc) || lstrlenA(pSrc) > MAX_PATH)
		return 0;
	if (pathIsAbsolute(pSrc) && pSrc[0]!='.')
		mir_snprintf(pOut, MAX_PATH, "%s", pSrc);

	else if (m_szSkinsPathA[0] && strstr(pSrc, ".tsk") && pSrc[0] != '.')
		mir_snprintf(pOut, MAX_PATH, "%s%s", m_szSkinsPathA, pSrc);

	else if (pSrc[0]=='.')
		mir_snprintf(pOut, MAX_PATH, "%s\\%s", m_szProfilePathA, pSrc);

	return lstrlenA(pOut);
}

#endif /* UNICODE */
/*
 * window list functions
 */

void CMimAPI::BroadcastMessage(UINT msg = 0, WPARAM wParam = 0, LPARAM lParam = 0)
{
	WindowList_Broadcast(m_hMessageWindowList, msg, wParam, lParam);
}

void CMimAPI::BroadcastMessageAsync(UINT msg = 0, WPARAM wParam = 0, LPARAM lParam = 0)
{
	WindowList_BroadcastAsync(m_hMessageWindowList, msg, wParam, lParam);
}

HWND CMimAPI::FindWindow(HANDLE h = 0) const
{
	return(WindowList_Find(m_hMessageWindowList, h));
}

INT_PTR CMimAPI::AddWindow(HWND hWnd = 0, HANDLE h = 0)
{
	return(WindowList_Add(m_hMessageWindowList, hWnd, h));
}

INT_PTR CMimAPI::RemoveWindow(HWND hWnd = 0)
{
	return(WindowList_Remove(m_hMessageWindowList, hWnd));
}

int CMimAPI::FoldersPathChanged(WPARAM wParam, LPARAM lParam)
{
	return(M->foldersPathChanged());
}

void CMimAPI::configureCustomFolders()
{
	if (ServiceExists(MS_FOLDERS_REGISTER_PATH)) {
		m_hDataPath = (HANDLE)FoldersRegisterCustomPathT("TabSRMM", "Data path", const_cast<TCHAR *>(getDataPath()));
		m_hSkinsPath = (HANDLE)FoldersRegisterCustomPathT("TabSRMM", "Skins root", const_cast<TCHAR *>(getSkinPath()));
		m_hAvatarsPath = (HANDLE)FoldersRegisterCustomPathT("TabSRMM", "Saved Avatars", const_cast<TCHAR *>(getSavedAvatarPath()));
		m_hChatLogsPath = (HANDLE)FoldersRegisterCustomPathT("TabSRMM", "Group chat logs root", const_cast<TCHAR *>(getChatLogPath()));
		HookEvent(ME_FOLDERS_PATH_CHANGED, CMimAPI::FoldersPathChanged);
	}
	else
		m_hDataPath = m_hSkinsPath = m_hAvatarsPath = m_hChatLogsPath = 0;

	foldersPathChanged();
}

INT_PTR CMimAPI::foldersPathChanged()
{
	TCHAR szTemp[MAX_PATH] = {'\0'};

	if(m_hDataPath) {
		FoldersGetCustomPathT(m_hDataPath, szTemp, MAX_PATH, const_cast<TCHAR *>(getDataPath()));
		mir_sntprintf(m_szProfilePath, MAX_PATH, _T("%s"), szTemp);

		FoldersGetCustomPathT(m_hSkinsPath, szTemp, MAX_PATH, const_cast<TCHAR *>(getSkinPath()));
		mir_sntprintf(m_szSkinsPath, MAX_PATH, _T("%s"), szTemp);

		FoldersGetCustomPathT(m_hAvatarsPath, szTemp, MAX_PATH, const_cast<TCHAR *>(getSavedAvatarPath()));
		mir_sntprintf(m_szSavedAvatarsPath, MAX_PATH, _T("%s"), szTemp);

		FoldersGetCustomPathT(m_hChatLogsPath, szTemp, MAX_PATH, const_cast<TCHAR *>(getChatLogPath()));
		mir_sntprintf(m_szChatLogsPath, MAX_PATH, _T("%s"), szTemp);
	}
	CallService(MS_UTILS_CREATEDIRTREET, 0, (LPARAM)m_szProfilePath);
	CallService(MS_UTILS_CREATEDIRTREET, 0, (LPARAM)m_szSkinsPath);
	CallService(MS_UTILS_CREATEDIRTREET, 0, (LPARAM)m_szSavedAvatarsPath);

	return 0;
}

void CMimAPI::InitPaths()
{
	m_szProfilePath[0] = 0;
	m_szSkinsPath[0] = 0;
	m_szSavedAvatarsPath[0] = 0;

	const TCHAR *szUserdataDir = getUserDir();

	mir_sntprintf(m_szProfilePath, MAX_PATH, _T("%stabSRMM"), szUserdataDir);
	mir_sntprintf(m_szChatLogsPath, MAX_PATH, _T("%s"), szUserdataDir);

	_sntprintf(m_szSkinsPath, MAX_PATH, _T("%s\\skins"), m_szProfilePath);
	_sntprintf(m_szSavedAvatarsPath, MAX_PATH, _T("%s\\Saved Contact Pictures"), m_szProfilePath);
	m_szSkinsPath[MAX_PATH - 1] = m_szSavedAvatarsPath[MAX_PATH - 1] = 0;

#if defined(_UNICODE)
	WideCharToMultiByte(CP_ACP, 0, m_szProfilePath, MAX_PATH, m_szProfilePathA, MAX_PATH, 0, 0);
	WideCharToMultiByte(CP_ACP, 0, m_szSkinsPath, MAX_PATH, m_szSkinsPathA, MAX_PATH, 0, 0);
	WideCharToMultiByte(CP_ACP, 0, m_szSavedAvatarsPath, MAX_PATH, m_szSavedAvatarsPathA, MAX_PATH, 0, 0);
#else
	strncpy(m_szProfilePathA, m_szProfilePath, MAX_PATH);
	strncpy(m_szSkinsPathA, m_szSkinsPath, MAX_PATH);
	strncpy(m_szSavedAvatarsPathA, m_szSavedAvatarsPath, MAX_PATH);
	m_szSavedAvatarsPathA[MAX_PATH - 1] = m_szSkinsPathA[MAX_PATH - 1] = m_szProfilePathA[MAX_PATH - 1] = 0;
#endif

	strlwr(m_szProfilePathA);
	strlwr(m_szSavedAvatarsPathA);
	strlwr(m_szSkinsPathA);
}

/**
 * Initialize various Win32 API functions which are not common to all versions of Windows.
 * We have to work with functions pointers here.
 */
void CMimAPI::InitAPI()
{
	m_hUxTheme = 0;
	m_VsAPI = false;

	HMODULE hDLL = GetModuleHandleA("user32");
	m_pSetLayeredWindowAttributes = (PSLWA) GetProcAddress(hDLL, "SetLayeredWindowAttributes");
	m_pUpdateLayeredWindow = (PULW) GetProcAddress(hDLL, "UpdateLayeredWindow");
	m_MyFlashWindowEx = (PFWEX) GetProcAddress(hDLL, "FlashWindowEx");
	m_MyAlphaBlend = (PAB) GetProcAddress(GetModuleHandleA("msimg32"), "AlphaBlend");
	m_MyGradientFill = (PGF) GetProcAddress(GetModuleHandleA("msimg32"), "GradientFill");
	m_fnSetMenuInfo = (SMI)GetProcAddress(GetModuleHandleA("USER32.DLL"), "SetMenuInfo");

	m_pfnMonitorFromWindow = (MMFW)GetProcAddress(GetModuleHandleA("USER32"), "MonitorFromWindow");
	m_pfnGetMonitorInfoA = (GMIA)GetProcAddress(GetModuleHandleA("USER32"), "GetMonitorInfoA");

	if (IsWinVerXPPlus()) {
		if ((m_hUxTheme = LoadLibraryA("uxtheme.dll")) != 0) {
			m_pfnIsThemeActive = (PITA)GetProcAddress(m_hUxTheme, "IsThemeActive");
			m_pfnOpenThemeData = (POTD)GetProcAddress(m_hUxTheme, "OpenThemeData");
			m_pfnDrawThemeBackground = (PDTB)GetProcAddress(m_hUxTheme, "DrawThemeBackground");
			m_pfnCloseThemeData = (PCTD)GetProcAddress(m_hUxTheme, "CloseThemeData");
			m_pfnDrawThemeText = (PDTT)GetProcAddress(m_hUxTheme, "DrawThemeText");
			m_pfnIsThemeBackgroundPartiallyTransparent = (PITBPT)GetProcAddress(m_hUxTheme, "IsThemeBackgroundPartiallyTransparent");
			m_pfnDrawThemeParentBackground = (PDTPB)GetProcAddress(m_hUxTheme, "DrawThemeParentBackground");
			m_pfnGetThemeBackgroundContentRect = (PGTBCR)GetProcAddress(m_hUxTheme, "GetThemeBackgroundContentRect");
			m_pfnEnableThemeDialogTexture = (ETDT)GetProcAddress(m_hUxTheme, "EnableThemeDialogTexture");

			if (m_pfnIsThemeActive != 0 && m_pfnOpenThemeData != 0 && m_pfnDrawThemeBackground != 0 && m_pfnCloseThemeData != 0
				&& m_pfnDrawThemeText != 0 && m_pfnIsThemeBackgroundPartiallyTransparent != 0 && m_pfnDrawThemeParentBackground != 0
				&& m_pfnGetThemeBackgroundContentRect != 0) {
				m_VsAPI = true;
			}
		}
	}

	/*
	 * vista+ DWM API
	 */

	m_hDwmApi = 0;
	if (IsWinVerVistaPlus())  {
	    m_hDwmApi = LoadLibraryA("dwmapi.dll");
	    if (m_hDwmApi)  {
            m_pfnDwmExtendFrameIntoClientArea = (DEFICA)GetProcAddress(m_hDwmApi,"DwmExtendFrameIntoClientArea");
            m_pfnDwmIsCompositionEnabled = (DICE)GetProcAddress(m_hDwmApi,"DwmIsCompositionEnabled");
			m_pfnDwmRegisterThumbnail = (DRT)GetProcAddress(m_hDwmApi, "DwmRegisterThumbnail");
			m_pfnDwmBlurBehindWindow = (BBW)GetProcAddress(m_hDwmApi, "DwmEnableBlurBehindWindow");
			m_pfnDwmGetColorizationColor = (DGC)GetProcAddress(m_hDwmApi, "DwmGetColorizationColor");
	    }
		/*
		 * additional uxtheme APIs (Vista+)
		 */
		if(m_hUxTheme) {
			m_pfnDrawThemeTextEx = (PDTTE)GetProcAddress(m_hUxTheme, "DrawThemeTextEx");
			m_pfnBeginBufferedPaint = (BBP)GetProcAddress(m_hUxTheme, "BeginBufferedPaint");
			m_pfnEndBufferedPaint = (EBP)GetProcAddress(m_hUxTheme, "EndBufferedPaint");
			m_pfnBufferedPaintInit = (BPI)GetProcAddress(m_hUxTheme, "BufferedPaintInit");
			m_pfnBufferedPaintUninit = (BPU)GetProcAddress(m_hUxTheme, "BufferedPaintUnInit");
		}
    }
}

CMimAPI *M = 0;
FI_INTERFACE *FIF = 0;

