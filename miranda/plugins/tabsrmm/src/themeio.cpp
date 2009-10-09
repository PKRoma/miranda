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
 * Import and export theme settings between files and the database
 *
 */

#include "commonheaders.h"
#pragma hdrstop

#define CURRENT_THEME_VERSION 4
#define THEME_COOKIE 25099837

extern char 		*TemplateNames[];
extern TemplateSet 	LTR_Active, RTL_Active;


/*
 * this loads a font definition from an INI file.
 * i = font number
 * szKey = ini section (e.g. [Font10])
 * *lf = pointer to a LOGFONT structure which will receive the font definition
 * *colour = pointer to a COLORREF which will receive the color of the font definition
*/
static void TSAPI LoadLogfontFromINI(int i, char *szKey, LOGFONTA *lf, COLORREF *colour, const char *szIniFilename)
{
	int style;
	char bSize;

	if (colour)
		*colour = GetPrivateProfileIntA(szKey, "Color", 0, szIniFilename);

	if (lf) {
		HDC hdc = GetDC(NULL);
		if (i == H_MSGFONTID_DIVIDERS)
			lf->lfHeight = 5;
		else {
			bSize = (char)GetPrivateProfileIntA(szKey, "Size", -12, szIniFilename);
			if (bSize > 0)
				lf->lfHeight = -MulDiv(bSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			else
				lf->lfHeight = bSize;
		}

		ReleaseDC(NULL, hdc);

		lf->lfWidth = 0;
		lf->lfEscapement = 0;
		lf->lfOrientation = 0;
		style = (int)GetPrivateProfileIntA(szKey, "Style", 0, szIniFilename);
		lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
		lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
		lf->lfUnderline = style & FONTF_UNDERLINE ? 1 : 0;
		lf->lfStrikeOut = 0;
		if (i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
			lf->lfCharSet = SYMBOL_CHARSET;
		else
			lf->lfCharSet = (BYTE)GetPrivateProfileIntA(szKey, "Set", DEFAULT_CHARSET, szIniFilename);
		lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf->lfQuality = DEFAULT_QUALITY;
		lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		if (i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT) {
			lstrcpynA(lf->lfFaceName, "Webdings", LF_FACESIZE);
			lf->lfCharSet = SYMBOL_CHARSET;
		} else
			GetPrivateProfileStringA(szKey, "Face", "Tahoma", lf->lfFaceName, LF_FACESIZE - 1, szIniFilename);
	}
}

static struct _tagFontBlocks {
	char *szModule;
	int iFirst;
	int iCount;
	char *szIniTemp;
	char *szBLockname;
} fontBlocks[] = {
	FONTMODULE, 0, MSGDLGFONTCOUNT, "Font%d", "StdFonts",
	FONTMODULE, 100, IPFONTCOUNT, "IPFont%d", "MiscFonts",
	CHAT_FONTMODULE, 0, CHATFONTCOUNT, "ChatFont%d", "ChatFonts",
	NULL, 0, 0, NULL
};

int TSAPI CheckThemeVersion(const TCHAR *szIniFilename)
{
	int cookie = GetPrivateProfileInt(_T("TabSRMM Theme"), _T("Cookie"), 0, szIniFilename);
	int version = GetPrivateProfileInt(_T("TabSRMM Theme"), _T("Version"), 0, szIniFilename);

	if (version >= CURRENT_THEME_VERSION && cookie == THEME_COOKIE)
		return 1;
	return 0;
}

void TSAPI WriteThemeToINI(const TCHAR *szIniFilenameT, struct _MessageWindowData *dat)
{
	int i, n = 0;
	DBVARIANT dbv;
	char szBuf[100], szTemp[100], szAppname[100];
	COLORREF def;

#if defined(_UNICODE)
	char *szIniFilename = mir_u2a(szIniFilenameT);
#else
	const char *szIniFilename = szIniFilenameT;
#endif

	WritePrivateProfileStringA("TabSRMM Theme", "Version", _itoa(CURRENT_THEME_VERSION, szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("TabSRMM Theme", "Cookie", _itoa(THEME_COOKIE, szBuf, 10), szIniFilename);

	while (fontBlocks[n].szModule) {
		int firstIndex = fontBlocks[n].iFirst;
		char *szModule = fontBlocks[n].szModule;
		WritePrivateProfileStringA(fontBlocks[n].szBLockname, "Valid", "1", szIniFilename);
		for (i = 0; i < fontBlocks[n].iCount; i++) {
			sprintf(szTemp, "Font%d", firstIndex + i);
			sprintf(szAppname, fontBlocks[n].szIniTemp, firstIndex + i);
			if (!DBGetContactSettingString(NULL, szModule, szTemp, &dbv)) {
				WritePrivateProfileStringA(szAppname, "Face", dbv.pszVal, szIniFilename);
				DBFreeVariant(&dbv);
			}
			sprintf(szTemp, "Font%dCol", firstIndex + i);
			WritePrivateProfileStringA(szAppname, "Color", _itoa(M->GetDword(szModule, szTemp, 0), szBuf, 10), szIniFilename);
			sprintf(szTemp, "Font%dSty", firstIndex + i);
			WritePrivateProfileStringA(szAppname, "Style", _itoa(M->GetByte(szModule, szTemp, 0), szBuf, 10), szIniFilename);
			sprintf(szTemp, "Font%dSize", firstIndex + i);
			WritePrivateProfileStringA(szAppname, "Size", _itoa(M->GetByte(szModule, szTemp, 0), szBuf, 10), szIniFilename);
			sprintf(szTemp, "Font%dSet", firstIndex + i);
			WritePrivateProfileStringA(szAppname, "Set", _itoa(M->GetByte(szModule, szTemp, 0), szBuf, 10), szIniFilename);
		}
		n++;
	}
	def = SRMSGDEFSET_BKGCOLOUR;

	WritePrivateProfileStringA("Message Log", "BackgroundColor", _itoa(M->GetDword(FONTMODULE, SRMSGSET_BKGCOLOUR, def), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "IncomingBG", _itoa(M->GetDword(FONTMODULE, "inbg", SRMSGDEFSET_BKGINCOLOUR), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "OutgoingBG", _itoa(M->GetDword(FONTMODULE, "outbg", SRMSGDEFSET_BKGOUTCOLOUR), szBuf, 10), szIniFilename);

	WritePrivateProfileStringA("Message Log", "OldIncomingBG", _itoa(M->GetDword(FONTMODULE, "oldinbg", SRMSGDEFSET_BKGINCOLOUR), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "OldOutgoingBG", _itoa(M->GetDword(FONTMODULE, "oldoutbg", SRMSGDEFSET_BKGOUTCOLOUR), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "StatusBG", _itoa(M->GetDword(FONTMODULE, "statbg", def), szBuf, 10), szIniFilename);

	WritePrivateProfileStringA("Message Log", "InputBG", _itoa(M->GetDword(FONTMODULE, "inputbg", def), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "HgridColor", _itoa(M->GetDword(FONTMODULE, "hgrid", def), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "DWFlags", _itoa(M->GetDword("mwflags", MWF_LOG_DEFAULT), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "VGrid", _itoa(M->GetByte("wantvgrid", 0), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "ExtraMicroLF", _itoa(M->GetByte("extramicrolf", 0), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Chat", "UserListBG", _itoa(M->GetDword("Chat", "ColorNicklistBG", def), szBuf, 10), szIniFilename);

	WritePrivateProfileStringA("Message Log", "LeftIndent", _itoa(M->GetDword("IndentAmount", 20), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "RightIndent", _itoa(M->GetDword("RightIndent", 20), szBuf, 10), szIniFilename);

	WritePrivateProfileStringA("Custom Colors", "InfopanelBG", _itoa(M->GetDword(FONTMODULE, "ipfieldsbg", GetSysColor(COLOR_3DFACE)), szBuf, 10), szIniFilename);

	for (i = 0; i <= TMPL_ERRMSG; i++) {
#if defined(_UNICODE)
		char *encoded;
		if (dat == 0)
			encoded = M->utf8_encodeW(LTR_Active.szTemplates[i]);
		else
			encoded = M->utf8_encodeW(dat->pContainer->ltr_templates->szTemplates[i]);
		WritePrivateProfileStringA("Templates", TemplateNames[i], encoded, szIniFilename);
		mir_free(encoded);
		if (dat == 0)
			encoded = M->utf8_encodeW(RTL_Active.szTemplates[i]);
		else
			encoded = M->utf8_encodeW(dat->pContainer->rtl_templates->szTemplates[i]);
		WritePrivateProfileStringA("RTLTemplates", TemplateNames[i], encoded, szIniFilename);
		mir_free(encoded);
#else
		if (dat == 0) {
			WritePrivateProfileStringA("Templates", TemplateNames[i], LTR_Active.szTemplates[i], szIniFilename);
			WritePrivateProfileStringA("RTLTemplates", TemplateNames[i], RTL_Active.szTemplates[i], szIniFilename);
		} else {
			WritePrivateProfileStringA("Templates", TemplateNames[i], dat->ltr_templates->szTemplates[i], szIniFilename);
			WritePrivateProfileStringA("RTLTemplates", TemplateNames[i], dat->rtl_templates->szTemplates[i], szIniFilename);
		}
#endif
	}
	for (i = 0; i < CUSTOM_COLORS; i++) {
		sprintf(szTemp, "cc%d", i + 1);
		if (dat == 0)
			WritePrivateProfileStringA("Custom Colors", szTemp, _itoa(M->GetDword(szTemp, 0), szBuf, 10), szIniFilename);
		else
			WritePrivateProfileStringA("Custom Colors", szTemp, _itoa(dat->pContainer->theme.custom_colors[i], szBuf, 10), szIniFilename);
	}
	for (i = 0; i <= 7; i++)
		WritePrivateProfileStringA("Nick Colors", _itoa(i, szBuf, 10), _itoa(g_Settings.nickColors[i], szTemp, 10), szIniFilename);

#if defined(_UNICODE)
	mir_free(szIniFilename);
#endif
}

void TSAPI ReadThemeFromINI(const TCHAR *szIniFilenameT, ContainerWindowData *dat, int noAdvanced, DWORD dwFlags)
{
	char szBuf[512], szTemp[100], szAppname[100];
	int i, n = 0;
	int version;
	COLORREF def;

#if defined(_UNICODE)
	char *szIniFilename = mir_u2a(szIniFilenameT);
	char szTemplateBuffer[TEMPLATE_LENGTH * 3 + 2];
#else
	const char *szIniFilename = szIniFilenameT;
#endif

	char bSize = 0;
	HDC hdc;
	int charset;

	if ((version = GetPrivateProfileIntA("TabSRMM Theme", "Version", 0, szIniFilename)) == 0)        // no version number.. assume 1
		version = 1;

	if (dat == 0) {
		hdc = GetDC(NULL);

		while (fontBlocks[n].szModule && (dwFlags & THEME_READ_FONTS)) {
			char *szModule = fontBlocks[n].szModule;
			int firstIndex = fontBlocks[n].iFirst;

			if (n != 1 && !(dwFlags & THEME_READ_FONTS)) {
				n++;
				continue;
			}
			if (GetPrivateProfileIntA(fontBlocks[n].szBLockname, "Valid", 0, szIniFilename) == 0 && n != 0) {
				n++;
				continue;
			}
			for (i = 0; i < fontBlocks[n].iCount; i++) {
				sprintf(szTemp, "Font%d", firstIndex + i);
				sprintf(szAppname, fontBlocks[n].szIniTemp, firstIndex + i);
				if (GetPrivateProfileStringA(szAppname, "Face", "Verdana", szBuf, sizeof(szBuf), szIniFilename) != 0) {
					if (i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
						lstrcpynA(szBuf, "Arial", sizeof(szBuf));
					DBWriteContactSettingString(NULL, szModule, szTemp, szBuf);
				}

				sprintf(szTemp, "Font%dCol", firstIndex + i);
				M->WriteDword(szModule, szTemp, GetPrivateProfileIntA(szAppname, "Color", GetSysColor(COLOR_WINDOWTEXT), szIniFilename));

				sprintf(szTemp, "Font%dSty", firstIndex + i);
				M->WriteByte(szModule, szTemp, (BYTE)(GetPrivateProfileIntA(szAppname, "Style", 0, szIniFilename)));

				sprintf(szTemp, "Font%dSize", firstIndex + i);
				bSize = (char)GetPrivateProfileIntA(szAppname, "Size", -10, szIniFilename);
				if (bSize > 0)
					bSize = -MulDiv(bSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
				M->WriteByte(szModule, szTemp, bSize);

				sprintf(szTemp, "Font%dSet", firstIndex + i);
				charset = GetPrivateProfileIntA(szAppname, "Set", 0, szIniFilename);
				if (i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
					charset = 0;
				M->WriteByte(szModule, szTemp, (BYTE)charset);
			}
			n++;
		}
		def = SRMSGDEFSET_BKGCOLOUR;
		ReleaseDC(NULL, hdc);
		M->WriteDword(FONTMODULE, "ipfieldsbg",
								   GetPrivateProfileIntA("Custom Colors", "InfopanelBG", GetSysColor(COLOR_3DFACE), szIniFilename));
		if (dwFlags & THEME_READ_FONTS) {
			COLORREF defclr;

			M->WriteDword(FONTMODULE, SRMSGSET_BKGCOLOUR, GetPrivateProfileIntA("Message Log", "BackgroundColor", def, szIniFilename));
			M->WriteDword("Chat", "ColorLogBG", GetPrivateProfileIntA("Message Log", "BackgroundColor", def, szIniFilename));
			M->WriteDword(FONTMODULE, "inbg", GetPrivateProfileIntA("Message Log", "IncomingBG", def, szIniFilename));
			M->WriteDword(FONTMODULE, "outbg", GetPrivateProfileIntA("Message Log", "OutgoingBG", def, szIniFilename));
			M->WriteDword(FONTMODULE, "inputbg", GetPrivateProfileIntA("Message Log", "InputBG", def, szIniFilename));

			M->WriteDword(FONTMODULE, "oldinbg", GetPrivateProfileIntA("Message Log", "OldIncomingBG", def, szIniFilename));
			M->WriteDword(FONTMODULE, "oldoutbg", GetPrivateProfileIntA("Message Log", "OldOutgoingBG", def, szIniFilename));
			M->WriteDword(FONTMODULE, "statbg", GetPrivateProfileIntA("Message Log", "StatusBG", def, szIniFilename));

			M->WriteDword(FONTMODULE, "hgrid", GetPrivateProfileIntA("Message Log", "HgridColor", def, szIniFilename));
			M->WriteDword(SRMSGMOD_T, "mwflags", GetPrivateProfileIntA("Message Log", "DWFlags", MWF_LOG_DEFAULT, szIniFilename));
			M->WriteByte(SRMSGMOD_T, "wantvgrid", (BYTE)(GetPrivateProfileIntA("Message Log", "VGrid", 0, szIniFilename)));
			M->WriteByte(SRMSGMOD_T, "extramicrolf", (BYTE)(GetPrivateProfileIntA("Message Log", "ExtraMicroLF", 0, szIniFilename)));

			M->WriteDword(SRMSGMOD_T, "IndentAmount", GetPrivateProfileIntA("Message Log", "LeftIndent", 0, szIniFilename));
			M->WriteDword(SRMSGMOD_T, "RightIndent", GetPrivateProfileIntA("Message Log", "RightIndent", 0, szIniFilename));

			M->WriteDword("Chat", "ColorNicklistBG", GetPrivateProfileIntA("Chat", "UserListBG", def, szIniFilename));

			for (i = 0; i < CUSTOM_COLORS; i++) {
				sprintf(szTemp, "cc%d", i + 1);
				if (dat == 0)
					M->WriteDword(SRMSGMOD_T, szTemp, GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename));
				else
					dat->theme.custom_colors[i] = GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename);
			}
			for (i = 0; i <= 7; i++) {
				if (i == 5)
					defclr = GetSysColor(COLOR_HIGHLIGHT);
				else if (i == 6)
					defclr = GetSysColor(COLOR_HIGHLIGHTTEXT);
				else
					defclr = g_Settings.crUserListColor;
				g_Settings.nickColors[i] = GetPrivateProfileIntA("Nick Colors", _itoa(i, szTemp, 10), defclr, szIniFilename);
				sprintf(szTemp, "NickColor%d", i);
				M->WriteDword("Chat", szTemp, g_Settings.nickColors[i]);
			}
		}
	} else {
		HDC hdc = GetDC(NULL);
		int SY = GetDeviceCaps(hdc, LOGPIXELSY);
		ReleaseDC(NULL, hdc);
		if (!noAdvanced) {
			for (i = 0; i < MSGDLGFONTCOUNT; i++) {
				_snprintf(szTemp, 20, "Font%d", i);
				LoadLogfontFromINI(i, szTemp, &dat->theme.logFonts[i], &dat->theme.fontColors[i], szIniFilename);
				wsprintfA(dat->theme.rtfFonts + (i * RTFCACHELINESIZE), "\\f%u\\cf%u\\b%d\\i%d\\ul%d\\fs%u", i, i, dat->theme.logFonts[i].lfWeight >= FW_BOLD ? 1 : 0, dat->theme.logFonts[i].lfItalic, dat->theme.logFonts[i].lfUnderline, 2 * abs(dat->theme.logFonts[i].lfHeight) * 74 / SY);
			}
			wsprintfA(dat->theme.rtfFonts + (MSGDLGFONTCOUNT * RTFCACHELINESIZE), "\\f%u\\cf%u\\b%d\\i%d\\ul%d\\fs%u", MSGDLGFONTCOUNT, MSGDLGFONTCOUNT, 0, 0, 0, 0);
		}
		dat->theme.bg = GetPrivateProfileIntA("Message Log", "BackgroundColor", RGB(224, 224, 224), szIniFilename);
		dat->theme.inbg = GetPrivateProfileIntA("Message Log", "IncomingBG", RGB(224, 224, 224), szIniFilename);
		dat->theme.outbg = GetPrivateProfileIntA("Message Log", "OutgoingBG", RGB(224, 224, 224), szIniFilename);

		dat->theme.oldinbg = GetPrivateProfileIntA("Message Log", "OldIncomingBG", RGB(224, 224, 224), szIniFilename);
		dat->theme.oldoutbg = GetPrivateProfileIntA("Message Log", "OldOutgoingBG", RGB(224, 224, 224), szIniFilename);
		dat->theme.statbg = GetPrivateProfileIntA("Message Log", "StatusBG", RGB(224, 224, 224), szIniFilename);

		dat->theme.inputbg = GetPrivateProfileIntA("Message Log", "InputBG", RGB(224, 224, 224), szIniFilename);
		dat->theme.hgrid = GetPrivateProfileIntA("Message Log", "HgridColor", RGB(224, 224, 224), szIniFilename);
		dat->theme.dwFlags = GetPrivateProfileIntA("Message Log", "DWFlags", MWF_LOG_DEFAULT, szIniFilename);
		M->WriteByte(SRMSGMOD_T, "wantvgrid", (BYTE)(GetPrivateProfileIntA("Message Log", "VGrid", 0, szIniFilename)));
		M->WriteByte(SRMSGMOD_T, "extramicrolf", (BYTE)(GetPrivateProfileIntA("Message Log", "ExtraMicroLF", 0, szIniFilename)));

		dat->theme.left_indent = GetPrivateProfileIntA("Message Log", "LeftIndent", 0, szIniFilename);
		dat->theme.right_indent = GetPrivateProfileIntA("Message Log", "RightIndent", 0, szIniFilename);

		for (i = 0; i < CUSTOM_COLORS; i++) {
			sprintf(szTemp, "cc%d", i + 1);
			if (dat == 0)
				M->WriteDword(SRMSGMOD_T, szTemp, GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename));
			else
				dat->theme.custom_colors[i] = GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename);
		}
	}

	if (version >= 3) {
		if (!noAdvanced && dwFlags & THEME_READ_TEMPLATES) {
			for (i = 0; i <= TMPL_ERRMSG; i++) {
#if defined(_UNICODE)
				wchar_t *decoded = 0;

				GetPrivateProfileStringA("Templates", TemplateNames[i], "[undef]", szTemplateBuffer, TEMPLATE_LENGTH * 3, szIniFilename);

				if (strcmp(szTemplateBuffer, "[undef]")) {
					if (dat == 0)
						DBWriteContactSettingStringUtf(NULL, TEMPLATES_MODULE, TemplateNames[i], szTemplateBuffer);
					decoded = M->utf8_decodeW(szTemplateBuffer);
					if (dat == 0)
						mir_snprintfW(LTR_Active.szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
					else
						mir_snprintfW(dat->ltr_templates->szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
					mir_free(decoded);
				}

				GetPrivateProfileStringA("RTLTemplates", TemplateNames[i], "[undef]", szTemplateBuffer, TEMPLATE_LENGTH * 3, szIniFilename);

				if (strcmp(szTemplateBuffer, "[undef]")) {
					if (dat == 0)
						DBWriteContactSettingStringUtf(NULL, RTLTEMPLATES_MODULE, TemplateNames[i], szTemplateBuffer);
					decoded = M->utf8_decodeW(szTemplateBuffer);
					if (dat == 0)
						mir_snprintfW(RTL_Active.szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
					else
						mir_snprintfW(dat->rtl_templates->szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
					mir_free(decoded);
				}
#else
				if (dat == 0) {
					GetPrivateProfileStringA("Templates", TemplateNames[i], "[undef]", LTR_Active.szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
					DBWriteContactSettingString(NULL, TEMPLATES_MODULE, TemplateNames[i], LTR_Active.szTemplates[i]);
					GetPrivateProfileStringA("RTLTemplates", TemplateNames[i], "", RTL_Active.szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
					DBWriteContactSettingString(NULL, RTLTEMPLATES_MODULE, TemplateNames[i], RTL_Active.szTemplates[i]);
				} else {
					GetPrivateProfileStringA("Templates", TemplateNames[i], "", dat->ltr_templates->szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
					GetPrivateProfileStringA("RTLTemplates", TemplateNames[i], "", dat->rtl_templates->szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
				}
#endif
			}
		}
	}
	if (PluginConfig.m_chat_enabled)
		LoadGlobalSettings();

#if defined(_UNICODE)
	mir_free(szIniFilename);
#endif
}

/*
 * iMode = 0 - GetOpenFilename, otherwise, GetSaveAs...
 */

const TCHAR* TSAPI GetThemeFileName(int iMode)
{
	static TCHAR szFilename[MAX_PATH];
	OPENFILENAME ofn = {0};
	TCHAR szInitialDir[MAX_PATH];

	mir_sntprintf(szInitialDir, MAX_PATH, _T("%s"), M->getSkinPath());

	szFilename[0] = 0;

	ofn.lpstrFilter = _T("tabSRMM themes\0*.tabsrmm\0\0");
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = 0;
	ofn.lpstrFile = szFilename;
	ofn.lpstrInitialDir = szInitialDir;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_DONTADDTORECENT;
	ofn.lpstrDefExt = _T("tabsrmm");
	if (!iMode) {
		if (GetOpenFileName(&ofn))
			return szFilename;
		else
			return NULL;
	} else {
		if (GetSaveFileName(&ofn))
			return szFilename;
		else
			return NULL;
	}
}

