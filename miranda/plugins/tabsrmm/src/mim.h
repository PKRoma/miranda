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

$Id:$

*/

/*
 * various Win32 API definitions which we use via function pointers
 */

#ifndef __MIM_H
#define __MIM_H

typedef BOOL 	(WINAPI *pfnSetMenuInfo )( HMENU hmenu, LPCMENUINFO lpcmi );
typedef HRESULT (STDAPICALLTYPE *DEFICA)(HWND hwnd, const MARGINS *margins);
typedef HRESULT (STDAPICALLTYPE *DICE)(BOOL *);
typedef DWORD (WINAPI *PSLWA)(HWND, DWORD, BYTE, DWORD);
typedef BOOL (WINAPI *PULW)(HWND, HDC, POINT *, SIZE *, HDC, POINT *, COLORREF, BLENDFUNCTION *, DWORD);
typedef BOOL (WINAPI *PFWEX)(FLASHWINFO *);
typedef BOOL (WINAPI *PAB)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
typedef BOOL (WINAPI *PGF)(HDC, PTRIVERTEX, ULONG, PVOID, ULONG, ULONG);

typedef BOOL (WINAPI *PITA)();
typedef HANDLE(WINAPI *POTD)(HWND, LPCWSTR);
typedef UINT(WINAPI *PDTB)(HANDLE, HDC, int, int, RECT *, RECT *);
typedef UINT(WINAPI *PCTD)(HANDLE);
typedef UINT(WINAPI *PDTT)(HANDLE, HDC, int, int, LPCWSTR, int, DWORD, DWORD, RECT *);
typedef BOOL (WINAPI *PITBPT)(HANDLE, int, int);
typedef HRESULT(WINAPI *PDTPB)(HWND, HDC, RECT *);
typedef HRESULT(WINAPI *PGTBCR)(HANDLE, HDC, int, int, const RECT *, const RECT *);


/*
 * used to encapsulate some parts of the Miranda API
 * constructor does early time initialization - do NOT put anything
 * here, except thngs that deal with the core and database API.
 *
 * it is UNSAFE to assume that any plugin provided services are available
 * when the object is instantiated.
 */

class _Mim
{
public:
	_Mim()
	{
		GetUTFI();
		InitPaths();
		InitAPI();
	}

	~_Mim() {
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

	void InitPaths();
	int pathIsAbsolute(const TCHAR *path) const;
	size_t pathToRelative(const TCHAR *pSrc, TCHAR *pOut);
	size_t pathToAbsolute(const TCHAR *pSrc, TCHAR *pOut);

	/*
	 * for backwards compatiblity still needed (not everything path-related is unicode
	*/

	int pathIsAbsolute(const char *path) const;
	size_t pathToRelative(const char *pSrc, char *pOut);
	size_t pathToAbsolute(const char *pSrc, char *pOut);

	const TCHAR  *getDataPath() const { return(m_szProfilePath); }
	const TCHAR  *getSkinPath() const { return(m_szSkinsPath); }
	const TCHAR  *getSavedAvatarPath() const { return(m_szSavedAvatarsPath); }

	const char  *getDataPathA() const { return(m_szProfilePathA); }
	const char  *getSkinPathA() const { return(m_szSkinsPathA); }
	const char  *getSavedAvatarPathA() const { return(m_szSavedAvatarsPathA); }

	const bool  isVSAPIState() const { return m_VsAPI; }
	const bool  isAero() const { BOOL result; return m_dwmIsCompositionEnabled && (m_dwmIsCompositionEnabled(&result) == S_OK) && result; }

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
	PITA 	m_pfnIsThemeActive;
	POTD 	m_pfnOpenThemeData;
	PDTB 	m_pfnDrawThemeBackground;
	PCTD 	m_pfnCloseThemeData;
	PDTT 	m_pfnDrawThemeText;
	PITBPT 	m_pfnIsThemeBackgroundPartiallyTransparent;
	PDTPB  	m_pfnDrawThemeParentBackground;
	PGTBCR 	m_pfnGetThemeBackgroundContentRect;
	BOOL (WINAPI *m_pfnEnableThemeDialogTexture)(HANDLE, DWORD);
	PSLWA 	m_pSetLayeredWindowAttributes;
	PULW	m_pUpdateLayeredWindow;
	PFWEX	m_MyFlashWindowEx;
	PAB		m_MyAlphaBlend;
	PGF		m_MyGradientFill;
	pfnSetMenuInfo m_fnSetMenuInfo;
	DEFICA	m_dwmExtendFrameIntoClientArea;
	DICE	m_dwmIsCompositionEnabled;

private:
	UTF8_INTERFACE 	m_utfi;
	TCHAR 		m_szProfilePath[MAX_PATH], m_szSkinsPath[MAX_PATH], m_szSavedAvatarsPath[MAX_PATH];
	char		m_szProfilePathA[MAX_PATH], m_szSkinsPathA[MAX_PATH], m_szSavedAvatarsPathA[MAX_PATH];
	HMODULE		m_hUxTheme, m_hDwmApi;
	bool		m_VsAPI;

	void	InitAPI();
	void	GetUTFI();
};

extern  _Mim		*M;

#endif /* __MIM_H */
