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

/*
 * various Win32 API definitions which we use via function pointers
 */

#ifndef __MIM_H
#define __MIM_H

typedef BOOL 	(WINAPI *SMI)( HMENU hmenu, LPCMENUINFO lpcmi );
typedef HRESULT (WINAPI *DEFICA)(HWND hwnd, const MARGINS *margins);
typedef HRESULT (WINAPI *DICE)(BOOL *);
typedef DWORD 	(WINAPI *PSLWA)(HWND, DWORD, BYTE, DWORD);
typedef BOOL 	(WINAPI *PULW)(HWND, HDC, POINT *, SIZE *, HDC, POINT *, COLORREF, BLENDFUNCTION *, DWORD);
typedef BOOL 	(WINAPI *PFWEX)(FLASHWINFO *);
typedef BOOL 	(WINAPI *PAB)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
typedef BOOL 	(WINAPI *PGF)(HDC, PTRIVERTEX, ULONG, PVOID, ULONG, ULONG);

typedef BOOL 	(WINAPI *PITA)();
typedef HANDLE	(WINAPI *POTD)(HWND, LPCWSTR);
typedef UINT	(WINAPI *PDTB)(HANDLE, HDC, int, int, RECT *, RECT *);
typedef UINT	(WINAPI *PCTD)(HANDLE);
typedef UINT	(WINAPI *PDTT)(HANDLE, HDC, int, int, LPCWSTR, int, DWORD, DWORD, RECT *);
typedef UINT	(WINAPI *PDTTE)(HANDLE, HDC, int, int, LPCWSTR, int, DWORD, RECT *, const DTTOPTS *);
typedef BOOL 	(WINAPI *PITBPT)(HANDLE, int, int);
typedef HRESULT	(WINAPI *PDTPB)(HWND, HDC, RECT *);
typedef HRESULT	(WINAPI *PGTBCR)(HANDLE, HDC, int, int, const RECT *, const RECT *);

typedef HMONITOR(WINAPI *MMFW)(HWND, DWORD);
typedef BOOL	(WINAPI *GMIA)(HMONITOR, LPMONITORINFO);
typedef HRESULT	(WINAPI *DRT)(HWND, HWND *, PHTHUMBNAIL);
typedef BOOL	(WINAPI *ETDT)(HANDLE, DWORD);
/*
 * used to encapsulate some parts of the Miranda API
 * constructor does early time initialization - do NOT put anything
 * here, except thngs that deal with the core and database API.
 *
 * it is UNSAFE to assume that any plugin provided services are available
 * when the object is instantiated.
 */

class CMimAPI
{
public:
	CMimAPI()
	{
		GetUTFI();
		InitPaths();
		InitAPI();
		getAeroState();
	}

	~CMimAPI() {
		if (m_hUxTheme != 0)
			FreeLibrary(m_hUxTheme);
		if (m_hDwmApi != 0)
			FreeLibrary(m_hDwmApi);
	}

	/*
	 * database functions
	 */

	DWORD GetDword(const HANDLE hContact, const char *szModule, const char *szSetting, DWORD uDefault) const;
	DWORD GetDword(const char *szModule, const char *szSetting, DWORD uDefault) const;
	DWORD GetDword(const char *szSetting, DWORD uDefault) const;
	DWORD GetDword(const HANDLE hContact, const char *szSetting, DWORD uDefault) const;

	int GetByte(const HANDLE hContact, const char *szModule, const char *szSetting, int uDefault) const;
	int GetByte(const char *szModule, const char *szSetting, int uDefault) const;
	int GetByte(const char *szSetting, int uDefault) const;
	int GetByte(const HANDLE hContact, const char *szSetting, int uDefault) const;

	INT_PTR GetTString(const HANDLE hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv) const;
	INT_PTR GetString(const HANDLE hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv) const;

	INT_PTR WriteDword(const HANDLE hContact, const char *szModule, const char *szSetting, DWORD value) const;
	INT_PTR WriteDword(const char *szModule, const char *szSetting, DWORD value) const;

	INT_PTR WriteByte(const HANDLE hContact, const char *szModule, const char *szSetting, BYTE value) const;
	INT_PTR WriteByte(const char *szModule, const char *szSetting, BYTE value) const;

	INT_PTR WriteTString(const HANDLE hContact, const char *szModule, const char *szSetting, const TCHAR *st) const;

	char *utf8_decode(char* str, wchar_t** ucs2) const;
	char *utf8_decodecp(char* str, int codepage, wchar_t** ucs2) const;
	char *utf8_encode(const char* src) const;
	char *utf8_encodecp(const char* src, int codepage) const;
	char *utf8_encodeW(const wchar_t* src) const;
	wchar_t *utf8_decodeW(const char* str) const;

	/*
	 * path utilities
	 */

	int pathIsAbsolute(const TCHAR *path) const;
	size_t pathToRelative(const TCHAR *pSrc, TCHAR *pOut);
	size_t pathToAbsolute(const TCHAR *pSrc, TCHAR *pOut);

	/*
	 * for backwards compatiblity still needed (not everything path-related is unicode
	*/

#if defined(UNICODE)
	int pathIsAbsolute(const char *path) const;
	size_t pathToRelative(const char *pSrc, char *pOut);
	size_t pathToAbsolute(const char *pSrc, char *pOut);
#endif

	const TCHAR  *getDataPath() const { return(m_szProfilePath); }
	const TCHAR  *getSkinPath() const { return(m_szSkinsPath); }
	const TCHAR  *getSavedAvatarPath() const { return(m_szSavedAvatarsPath); }

	const char  *getDataPathA() const { return(m_szProfilePathA); }
	const char  *getSkinPathA() const { return(m_szSkinsPathA); }
	const char  *getSavedAvatarPathA() const { return(m_szSavedAvatarsPathA); }

	const bool  isVSAPIState() const { return m_VsAPI; }
	/**
	 * return status of Vista Aero
	 *
	 * @return bool: true if aero active, false otherwise
	 */
	const bool  isAero() const { return(m_isAero); }

	/**
	 * Refresh Aero status.
	 * Called on:
	 * * plugin init
	 * * WM_DWMCOMPOSITIONCHANGED message received
	 *
	 * @return
	 */
	bool		getAeroState()
	{
		BOOL result = FALSE;
		m_isAero = false;
#if defined(_UNICODE)
		if(IsWinVerVistaPlus()) {
			m_isAero = (CSkin::m_skinEnabled == false) && GetByte("useAero", 0) &&
				((m_pfnDwmIsCompositionEnabled && (m_pfnDwmIsCompositionEnabled(&result) == S_OK) && result) ? true : false);
		}
#endif
		return(m_isAero);
	}
	/*
	 * window lists
	 */

	void		BroadcastMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void		BroadcastMessageAsync(UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR		AddWindow(HWND hWnd, HANDLE h);
	INT_PTR		RemoveWindow(HWND hWnd);
	HWND		FindWindow(HANDLE h) const;


public:
	HANDLE 		m_hMessageWindowList;
	/*
	 various function pointers
	*/
	static PITA 	m_pfnIsThemeActive;
	static POTD 	m_pfnOpenThemeData;
	static PDTB 	m_pfnDrawThemeBackground;
	static PCTD 	m_pfnCloseThemeData;
	static PDTT 	m_pfnDrawThemeText;
	static PDTTE	m_pfnDrawThemeTextEx;
	static PITBPT 	m_pfnIsThemeBackgroundPartiallyTransparent;
	static PDTPB  	m_pfnDrawThemeParentBackground;
	static PGTBCR 	m_pfnGetThemeBackgroundContentRect;
	static ETDT 	m_pfnEnableThemeDialogTexture;
	static PSLWA 	m_pSetLayeredWindowAttributes;
	static PULW		m_pUpdateLayeredWindow;
	static PFWEX	m_MyFlashWindowEx;
	static PAB		m_MyAlphaBlend;
	static PGF		m_MyGradientFill;
	static SMI		m_fnSetMenuInfo;
	static DEFICA	m_pfnDwmExtendFrameIntoClientArea;
	static DICE		m_pfnDwmIsCompositionEnabled;
	static MMFW		m_pfnMonitorFromWindow;
	static GMIA		m_pfnGetMonitorInfoA;
	static DRT		m_pfnDwmRegisterThumbnail;
private:
	UTF8_INTERFACE 	m_utfi;
	TCHAR 		m_szProfilePath[MAX_PATH], m_szSkinsPath[MAX_PATH], m_szSavedAvatarsPath[MAX_PATH];
	char		m_szProfilePathA[MAX_PATH], m_szSkinsPathA[MAX_PATH], m_szSavedAvatarsPathA[MAX_PATH];
	HMODULE		m_hUxTheme, m_hDwmApi;
	bool		m_VsAPI;
	bool		m_isAero;

	void	InitAPI();
	void	GetUTFI();
	void 	InitPaths();
};

extern  CMimAPI		*M;

#endif /* __MIM_H */
