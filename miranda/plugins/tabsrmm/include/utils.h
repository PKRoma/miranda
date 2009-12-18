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
 * utility functions
 *
 */

#ifndef __UTILS_H
#define __UTILS_H

#define RTF_CTABLE_DEFSIZE 8

#if defined(_UNICODE)
	#define		CNT_KEYNAME "CNTW_Def"
	#define CNT_BASEKEYNAME "CNTW_"
#else
	#define		CNT_KEYNAME "CNT_Def"
	#define CNT_BASEKEYNAME "CNT_"
#endif

struct TRTFColorTable {
    TCHAR 		szName[10];
    COLORREF 	clr;
    int 		index;
    int 		menuid;
};

static TRTFColorTable _rtf_ctable[] = {
	_T("red"), RGB(255, 0, 0), 0, ID_FONT_RED,
	_T("blue"), RGB(0, 0, 255), 0, ID_FONT_BLUE,
	_T("green"), RGB(0, 255, 0), 0, ID_FONT_GREEN,
	_T("magenta"), RGB(255, 0, 255), 0, ID_FONT_MAGENTA,
	_T("yellow"), RGB(255, 255, 0), 0, ID_FONT_YELLOW,
	_T("cyan"), RGB(0, 255, 255), 0, ID_FONT_CYAN,
	_T("black"), 0, 0, ID_FONT_BLACK,
	_T("white"), RGB(255, 255, 255), 0, ID_FONT_WHITE,
	_T(""), 0, 0, 0
};

class Utils {

public:
	enum {
		CMD_CONTAINER = 1,
		CMD_MSGDIALOG = 2,
		CMD_INFOPANEL = 4,
	};
	static	int					TSAPI FindRTLLocale					(TWindowData *dat);
	static  TCHAR* 				TSAPI GetPreviewWithEllipsis		(TCHAR *szText, size_t iMaxLen);
	static	TCHAR* 				TSAPI FilterEventMarkers			(TCHAR *wszText);
	static  const TCHAR* 		TSAPI FormatRaw						(TWindowData *dat, const TCHAR *msg, int flags, BOOL isSent);
	static	const TCHAR* 		TSAPI FormatTitleBar				(const TWindowData *dat, const TCHAR *szFormat);
#if defined(_UNICODE)
	static	char* 				TSAPI FilterEventMarkers			(char *szText);
#endif
	static	const TCHAR* 		TSAPI DoubleAmpersands				(TCHAR *pszText);
	static	void 				TSAPI RTF_CTableInit				();
	static	void 				TSAPI RTF_ColorAdd					(const TCHAR *tszColname, size_t length);
	static	void 				TSAPI CreateColorMap				(TCHAR *Text);
	static	int 				TSAPI RTFColorToIndex				(int iCol);
	static	int					TSAPI ReadContainerSettingsFromDB	(const HANDLE hContact, TContainerSettings *cs, const char *szKey = 0);
	static	int 				TSAPI WriteContainerSettingsToDB	(const HANDLE hContact, TContainerSettings *cs, const char *szKey = 0);
	static  void 				TSAPI SettingsToContainer			(TContainerData *pContainer);
	static	void 				TSAPI ContainerToSettings			(TContainerData *pContainer);
	static	void 				TSAPI ReadPrivateContainerSettings	(TContainerData *pContainer, bool fForce = false);
	static	void				TSAPI SaveContainerSettings			(TContainerData *pContainer, const char *szSetting);
	static	DWORD 		CALLBACK 	  StreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb);
	static	LRESULT				TSAPI CmdDispatcher					(UINT uType, HWND hwndDlg, UINT cmd, WPARAM wParam, LPARAM lParam, TWindowData *dat = 0,
																	 TContainerData *pContainer = 0);
	static	void 				TSAPI addMenuItem					(const HMENU& m, MENUITEMINFO& mii, HICON hIcon,
																	 const TCHAR *szText, UINT uID, UINT pos);
	static	void				TSAPI enableDlgControl				(const HWND hwnd, UINT id, BOOL fEnable = 1);
	static 	void 				TSAPI showDlgControl				(const HWND hwnd, UINT id, int showCmd);
	static	int					TSAPI mustPlaySound					(const TWindowData *dat);
	static 	HICON				TSAPI iconFromAvatar				(const TWindowData *dat);
	static  void 				TSAPI extractResource				(const HMODULE h, const UINT uID, const TCHAR *tszName, const TCHAR *tszPath,
																	 const TCHAR *tszFilename, bool fForceOverwrite);

	template<typename T> static size_t TSAPI CopyToClipBoard(T* _t, const HWND hwndOwner)
	{
		HGLOBAL	hData;

		if (!OpenClipboard(hwndOwner) || _t == 0)
			return(0);

		size_t _s = sizeof(T);
		size_t  s = _s * ((_s == 1 ? lstrlenA(reinterpret_cast<char *>(_t)) : lstrlenW(reinterpret_cast<wchar_t*>(_t))) + 1);
		EmptyClipboard();
		hData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, s);

		CopyMemory((void *)GlobalLock(hData), (void *)_t, s);
		GlobalUnlock(hData);
		SetClipboardData(_s == 1 ? CF_TEXT : CF_UNICODETEXT, hData);
		CloseClipboard();
		return(s);
	}

	template<typename T> static void AddToFileList(T ***pppFiles, int *totalCount, const TCHAR* szFilename)
	{
		size_t _s = sizeof(T);

		*pppFiles = (T**)mir_realloc(*pppFiles, (++*totalCount + 1) * sizeof(T*));
		(*pppFiles)[*totalCount] = NULL;

		if(_s == 1)
			(*pppFiles)[*totalCount-1] = reinterpret_cast<T *>(mir_t2a(szFilename));
		else
			(*pppFiles)[*totalCount-1] = reinterpret_cast<T *>(mir_tstrdup(szFilename));

		if (GetFileAttributes(szFilename) & FILE_ATTRIBUTE_DIRECTORY) {
			WIN32_FIND_DATA fd;
			HANDLE hFind;
			TCHAR szPath[MAX_PATH];

			lstrcpy(szPath, szFilename);
			lstrcat(szPath, _T("\\*"));
			if ((hFind = FindFirstFile(szPath, &fd)) != INVALID_HANDLE_VALUE) {
				do {
					if (!lstrcmp(fd.cFileName, _T(".")) || !lstrcmp(fd.cFileName, _T("..")))
						continue;
					lstrcpy(szPath, szFilename);
					lstrcat(szPath, _T("\\"));
					lstrcat(szPath, fd.cFileName);
					AddToFileList(pppFiles, totalCount, szPath);
				} while (FindNextFile(hFind, &fd));
				FindClose(hFind);
			}
		}
	}

public:
	static	TRTFColorTable*		rtf_ctable;
	static	int					rtf_ctable_size;
};

LRESULT TSAPI		_dlgReturn(HWND hWnd, LRESULT result);

#endif /* __UTILS_H */
