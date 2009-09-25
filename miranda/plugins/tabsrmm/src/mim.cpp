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
 * wraps some parts of Miranda API
 * Also, OS dependent stuff (visual styles api etc.)
 *
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
BPSA	CMimAPI::m_pfnBufferedPaintSetAlpha = 0;

bool	CMimAPI::m_shutDown = 0;
TCHAR	CMimAPI::m_userDir[] = _T("\0");

bool	CMimAPI::m_haveBufferedPaint = false;
DWORD	CMimAPI::m_MimVersion = 0;

void CMimAPI::timerMsg(const char *szMsg)
{
	mir_snprintf(m_timerMsg, 256, "%s: %d ticks = %f msec", szMsg, (int)(m_tStop - m_tStart), 1000 * ((double)(m_tStop - m_tStart) * m_dFreq));
	_DebugTraceA(m_timerMsg);
}
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

char *CMimAPI::utf8_encodeT(const TCHAR* src) const
{
#if defined(_UNICODE)
	return(m_utfi.utf8_encodeW(src));
#else
	return(m_utfi.utf8_encode(src));
#endif
}

TCHAR *CMimAPI::utf8_decodeT(const char* src) const
{
#if defined(_UNICODE)
	return(m_utfi.utf8_decodeW(src));
#else
	return(m_utfi.utf8_decode(const_cast<char *>(src), NULL));
#endif
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

size_t CMimAPI::pathToRelative(const TCHAR *pSrc, TCHAR *pOut, const TCHAR *szBase) const
{
	const TCHAR	*tszBase = szBase ? szBase : m_szProfilePath;

	pOut[0] = 0;
	if (!pSrc || !lstrlen(pSrc) || lstrlen(pSrc) > MAX_PATH)
		return 0;
	if (!pathIsAbsolute(pSrc)) {
		mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);
		return lstrlen(pOut);
	} else {
		TCHAR	szTmp[MAX_PATH];

		mir_sntprintf(szTmp, SIZEOF(szTmp), _T("%s"), pSrc);
		if (StriStr(szTmp, tszBase)) {
			mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc + lstrlen(tszBase) - 1);
			pOut[0]='.';
			return(lstrlen(pOut));
		} else {
			mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);
			return(lstrlen(pOut));
		}
	}
}

/**
 * Translate a relativ path to an absolute, using the current profile
 * data directory.
 *
 * @param pSrc   TCHAR *: input path + filename (relative)
 * @param pOut   TCHAR *: the result
 * @param szBase TCHAR *: (OPTIONAL) base path for the translation. Can be 0 in which case
 *               the function will use m_szProfilePath (usually \tabSRMM below %miranda_userdata%
 *
 * @return
 */
size_t CMimAPI::pathToAbsolute(const TCHAR *pSrc, TCHAR *pOut, const TCHAR *szBase) const
{
	const TCHAR	*tszBase = szBase ? szBase : m_szProfilePath;

	pOut[0] = 0;
	if (!pSrc || !lstrlen(pSrc) || lstrlen(pSrc) > MAX_PATH)
		return 0;
	if (pathIsAbsolute(pSrc) && pSrc[0]!='.')
		mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);
	else if (pSrc[0]=='.')
		mir_sntprintf(pOut, MAX_PATH, _T("%s\\%s"), tszBase, pSrc + 1);
	else
		mir_sntprintf(pOut, MAX_PATH, _T("%s\\%s"), tszBase, pSrc);

	return lstrlen(pOut);
}

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
	m_haveFolders = false;
	if (ServiceExists(MS_FOLDERS_REGISTER_PATH)) {
		m_hDataPath = (HANDLE)FoldersRegisterCustomPathT("TabSRMM", "Data path", const_cast<TCHAR *>(getDataPath()));
		m_hSkinsPath = (HANDLE)FoldersRegisterCustomPathT("TabSRMM", "Skins root", const_cast<TCHAR *>(getSkinPath()));
		m_hAvatarsPath = (HANDLE)FoldersRegisterCustomPathT("TabSRMM", "Saved Avatars", const_cast<TCHAR *>(getSavedAvatarPath()));
		m_hChatLogsPath = (HANDLE)FoldersRegisterCustomPathT("TabSRMM", "Group chat logs root", const_cast<TCHAR *>(getChatLogPath()));
		HookEvent(ME_FOLDERS_PATH_CHANGED, CMimAPI::FoldersPathChanged);
		m_haveFolders = true;
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
	CallService(MS_UTILS_CREATEDIRTREET, 0, (LPARAM)m_szChatLogsPath);

	Skin->extractSkinsAndLogo();
	Skin->setupAeroSkins();
	return 0;
}

const TCHAR* CMimAPI::getUserDir()
{
	if(m_userDir[0] == 0) {
		mir_sntprintf(m_userDir, MAX_PATH, ::Utils_ReplaceVarsT(_T("%miranda_userdata%")));
		if(m_userDir[lstrlen(m_userDir) - 1] != '\\')
		   _tcscat(m_userDir, _T("\\"));
	}
	return(m_userDir);
}

void CMimAPI::InitPaths()
{
	m_szProfilePath[0] = 0;
	m_szSkinsPath[0] = 0;
	m_szSavedAvatarsPath[0] = 0;

	const TCHAR *szUserdataDir = getUserDir();

	mir_sntprintf(m_szProfilePath, MAX_PATH, _T("%stabSRMM"), szUserdataDir);
	mir_sntprintf(m_szChatLogsPath, MAX_PATH, _T("%s"), szUserdataDir);

	_sntprintf(m_szSkinsPath, MAX_PATH, _T("%s"), szUserdataDir);
	_sntprintf(m_szSavedAvatarsPath, MAX_PATH, _T("%s\\Saved Contact Pictures"), m_szProfilePath);
	m_szSkinsPath[MAX_PATH - 1] = m_szSavedAvatarsPath[MAX_PATH - 1] = 0;
}

bool CMimAPI::getAeroState()
{
	BOOL result = FALSE;
	m_isAero = m_DwmActive = false;
#if defined(_UNICODE)
	if(IsWinVerVistaPlus()) {
		m_DwmActive = (m_pfnDwmIsCompositionEnabled && (m_pfnDwmIsCompositionEnabled(&result) == S_OK) && result) ? true : false;
		m_isAero = (CSkin::m_skinEnabled == false) && GetByte("useAero", 1) && CSkin::m_fAeroSkinsValid && m_DwmActive;

	}
#endif
	m_isVsThemed = (m_VsAPI && m_pfnIsThemeActive && m_pfnIsThemeActive());
	return(m_isAero);
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
			m_pfnBufferedPaintSetAlpha = (BPSA)GetProcAddress(m_hUxTheme, "BufferedPaintSetAlpha");
			m_haveBufferedPaint = (m_pfnBeginBufferedPaint != 0 && m_pfnEndBufferedPaint != 0) ? true : false;
			if(m_haveBufferedPaint)
				m_pfnBufferedPaintInit();
		}
    }
	else
		m_haveBufferedPaint = false;
}

CMimAPI *M = 0;
FI_INTERFACE *FIF = 0;

