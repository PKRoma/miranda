/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

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

$Id$

Themes and skinning for tabSRMM

*/

#include "commonheaders.h"
#pragma hdrstop

/*
 * writes the current "theme" to .ini file
 * a theme contains all the fonts, colors and message log formatting
 * options.
 */

#define CURRENT_THEME_VERSION 4
#define THEME_COOKIE 25099837

extern PSLWA pSetLayeredWindowAttributes;
extern PGF MyGradientFill;
extern PAB MyAlphaBlend;

extern char *TemplateNames[];
extern TemplateSet LTR_Active, RTL_Active;
extern MYGLOBALS myGlobals;
extern struct ContainerWindowData *pFirstContainer;
extern int          g_chat_integration_enabled;

static void __inline gradientVertical(UCHAR *ubRedFinal, UCHAR *ubGreenFinal, UCHAR *ubBlueFinal,
									  ULONG ulBitmapHeight, UCHAR ubRed, UCHAR ubGreen, UCHAR ubBlue, UCHAR ubRed2,
									  UCHAR ubGreen2, UCHAR ubBlue2, DWORD FLG_GRADIENT, BOOL transparent, UINT32 y, UCHAR *ubAlpha);

static void __inline gradientHorizontal(UCHAR *ubRedFinal, UCHAR *ubGreenFinal, UCHAR *ubBlueFinal,
										ULONG ulBitmapWidth, UCHAR ubRed, UCHAR ubGreen, UCHAR ubBlue,  UCHAR ubRed2,
										UCHAR ubGreen2, UCHAR ubBlue2, DWORD FLG_GRADIENT, BOOL transparent, UINT32 x, UCHAR *ubAlpha);


static ImageItem *g_ImageItems = NULL;
ImageItem *g_glyphItem = NULL;

HBRUSH g_ContainerColorKeyBrush = 0, g_MenuBGBrush = 0;
COLORREF g_ContainerColorKey = 0;
SIZE g_titleBarButtonSize = {0};
int g_titleBarLeftOff = 0, g_titleButtonTopOff = 0, g_captionOffset = 0, g_captionPadding = 0, g_titleBarRightOff = 0, g_sidebarTopOffset = 0, g_sidebarBottomOffset;

extern BOOL g_skinnedContainers;
extern BOOL g_framelessSkinmode;

int  SIDEBARWIDTH = DEFAULT_SIDEBARWIDTH;

UINT nextButtonID;
ButtonSet g_ButtonSet = {0};

#define   NR_MAXSKINICONS 100

ICONDESC *g_skinIcons = NULL;
int       g_nrSkinIcons = 0;

StatusItems_t StatusItems[] = {
	{"Container", "TSKIN_Container", ID_EXTBKCONTAINER,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Toolbar", "TSKIN_Container", ID_EXTBKBUTTONBAR,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"{-}Buttonpressed", "TSKIN_BUTTONSPRESSED", ID_EXTBKBUTTONSPRESSED,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Buttonnotpressed", "TSKIN_BUTTONSNPRESSED", ID_EXTBKBUTTONSNPRESSED,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Buttonmouseover", "TSKIN_BUTTONSMOUSEOVER", ID_EXTBKBUTTONSMOUSEOVER,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Infopanelfield", "TSKIN_INFOPANELFIELD", ID_EXTBKINFOPANEL,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Titlebutton", "TSKIN_TITLEBUTTON", ID_EXTBKTITLEBUTTON,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Titlebuttonmouseover", "TSKIN_TITLEBUTTONHOVER", ID_EXTBKTITLEBUTTONMOUSEOVER,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Titlebuttonpressed", "TSKIN_TITLEBUTTONPRESSED", ID_EXTBKTITLEBUTTONPRESSED,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Tabpage", "TSKIN_TABPAGE", ID_EXTBKTABPAGE,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Tabitem", "TSKIN_TABITEM", ID_EXTBKTABITEM,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Tabitem_active", "TSKIN_TABITEMACTIVE", ID_EXTBKTABITEMACTIVE,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Tabitem_bottom", "TSKIN_TABITEMBOTTOM", ID_EXTBKTABITEMBOTTOM,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Tabitem_active_bottom", "TSKIN_TABITEMACTIVEBOTTOM", ID_EXTBKTABITEMACTIVEBOTTOM,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Frame", "TSKIN_FRAME", ID_EXTBKFRAME,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"MessageLog", "TSKIN_MLOG", ID_EXTBKHISTORY,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"InputArea", "TSKIN_INPUT", ID_EXTBKINPUTAREA,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"FrameInactive", "TSKIN_FRAMEINACTIVE", ID_EXTBKFRAMEINACTIVE,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Tabitem_hottrack", "TSKIN_TABITEMHOTTRACK", ID_EXTBKTABITEMHOTTRACK,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Tabitem_hottrack_bottom", "TSKIN_TABITEMHOTTRACKBOTTOM", ID_EXTBKTABITEMHOTTRACKBOTTOM,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Statusbarpanel", "TSKIN_STATUSBARPANEL", ID_EXTBKSTATUSBARPANEL,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Statusbar", "TSKIN_STATUSBAR", ID_EXTBKSTATUSBAR,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}, {"Userlist", "TSKIN_USERLIST", ID_EXTBKUSERLIST,
		CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
		CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
		CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}
};

static void LoadLogfontFromINI(int i, char *szKey, LOGFONTA *lf, COLORREF *colour, const char *szIniFilename)
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

int CheckThemeVersion(const char *szIniFilename)
{
	int cookie = GetPrivateProfileIntA("TabSRMM Theme", "Cookie", 0, szIniFilename);
	int version = GetPrivateProfileIntA("TabSRMM Theme", "Version", 0, szIniFilename);

	if (version >= CURRENT_THEME_VERSION && cookie == THEME_COOKIE)
		return 1;
	return 0;
}

void WriteThemeToINI(const char *szIniFilename, struct MessageWindowData *dat)
{
	int i, n = 0;
	DBVARIANT dbv;
	char szBuf[100], szTemp[100], szAppname[100];
	COLORREF def;

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
			WritePrivateProfileStringA(szAppname, "Color", _itoa(DBGetContactSettingDword(NULL, szModule, szTemp, 0), szBuf, 10), szIniFilename);
			sprintf(szTemp, "Font%dSty", firstIndex + i);
			WritePrivateProfileStringA(szAppname, "Style", _itoa(DBGetContactSettingByte(NULL, szModule, szTemp, 0), szBuf, 10), szIniFilename);
			sprintf(szTemp, "Font%dSize", firstIndex + i);
			WritePrivateProfileStringA(szAppname, "Size", _itoa(DBGetContactSettingByte(NULL, szModule, szTemp, 0), szBuf, 10), szIniFilename);
			sprintf(szTemp, "Font%dSet", firstIndex + i);
			WritePrivateProfileStringA(szAppname, "Set", _itoa(DBGetContactSettingByte(NULL, szModule, szTemp, 0), szBuf, 10), szIniFilename);
		}
		n++;
	}
	def = GetSysColor(COLOR_WINDOW);

	WritePrivateProfileStringA("Message Log", "BackgroundColor", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, def), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "IncomingBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "inbg", def), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "OutgoingBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "outbg", def), szBuf, 10), szIniFilename);

	WritePrivateProfileStringA("Message Log", "OldIncomingBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "oldinbg", def), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "OldOutgoingBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "oldoutbg", def), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "StatusBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "statbg", def), szBuf, 10), szIniFilename);

	WritePrivateProfileStringA("Message Log", "InputBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", def), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "HgridColor", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "hgrid", def), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "DWFlags", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "VGrid", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", 0), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "ExtraMicroLF", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", 0), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Chat", "UserListBG", _itoa(DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", def), szBuf, 10), szIniFilename);

	WritePrivateProfileStringA("Message Log", "LeftIndent", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 0), szBuf, 10), szIniFilename);
	WritePrivateProfileStringA("Message Log", "RightIndent", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 0), szBuf, 10), szIniFilename);

	WritePrivateProfileStringA("Custom Colors", "InfopanelBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "ipfieldsbg", GetSysColor(COLOR_3DFACE)), szBuf, 10), szIniFilename);

	for (i = 0; i <= TMPL_ERRMSG; i++) {
#if defined(_UNICODE)
		char *encoded;
		if (dat == 0)
			encoded = Utf8_Encode(LTR_Active.szTemplates[i]);
		else
			encoded = Utf8_Encode(dat->ltr_templates->szTemplates[i]);
		WritePrivateProfileStringA("Templates", TemplateNames[i], encoded, szIniFilename);
		free(encoded);
		if (dat == 0)
			encoded = Utf8_Encode(RTL_Active.szTemplates[i]);
		else
			encoded = Utf8_Encode(dat->rtl_templates->szTemplates[i]);
		WritePrivateProfileStringA("RTLTemplates", TemplateNames[i], encoded, szIniFilename);
		free(encoded);
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
			WritePrivateProfileStringA("Custom Colors", szTemp, _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, szTemp, 0), szBuf, 10), szIniFilename);
		else
			WritePrivateProfileStringA("Custom Colors", szTemp, _itoa(dat->theme.custom_colors[i], szBuf, 10), szIniFilename);
	}
	for (i = 0; i <= 7; i++)
		WritePrivateProfileStringA("Nick Colors", _itoa(i, szBuf, 10), _itoa(g_Settings.nickColors[i], szTemp, 10), szIniFilename);
}

void ReadThemeFromINI(const char *szIniFilename, struct MessageWindowData *dat, int noAdvanced, DWORD dwFlags)
{
	char szBuf[512], szTemp[100], szAppname[100];
	int i, n = 0;
	int version;
	COLORREF def;
#if defined(_UNICODE)
	char szTemplateBuffer[TEMPLATE_LENGTH * 3 + 2];
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
				DBWriteContactSettingDword(NULL, szModule, szTemp, GetPrivateProfileIntA(szAppname, "Color", GetSysColor(COLOR_WINDOWTEXT), szIniFilename));

				sprintf(szTemp, "Font%dSty", firstIndex + i);
				DBWriteContactSettingByte(NULL, szModule, szTemp, (BYTE)(GetPrivateProfileIntA(szAppname, "Style", 0, szIniFilename)));

				sprintf(szTemp, "Font%dSize", firstIndex + i);
				bSize = (char)GetPrivateProfileIntA(szAppname, "Size", -10, szIniFilename);
				if (bSize > 0)
					bSize = -MulDiv(bSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
				DBWriteContactSettingByte(NULL, szModule, szTemp, bSize);

				sprintf(szTemp, "Font%dSet", firstIndex + i);
				charset = GetPrivateProfileIntA(szAppname, "Set", 0, szIniFilename);
				if (i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
					charset = 0;
				DBWriteContactSettingByte(NULL, szModule, szTemp, (BYTE)charset);
			}
			n++;
		}
		def = GetSysColor(COLOR_WINDOW);
		ReleaseDC(NULL, hdc);
		DBWriteContactSettingDword(NULL, FONTMODULE, "ipfieldsbg",
								   GetPrivateProfileIntA("Custom Colors", "InfopanelBG", GetSysColor(COLOR_3DFACE), szIniFilename));
		if (dwFlags & THEME_READ_FONTS) {
			COLORREF defclr;

			DBWriteContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, GetPrivateProfileIntA("Message Log", "BackgroundColor", def, szIniFilename));
			DBWriteContactSettingDword(NULL, "Chat", "ColorLogBG", GetPrivateProfileIntA("Message Log", "BackgroundColor", def, szIniFilename));
			DBWriteContactSettingDword(NULL, FONTMODULE, "inbg", GetPrivateProfileIntA("Message Log", "IncomingBG", def, szIniFilename));
			DBWriteContactSettingDword(NULL, FONTMODULE, "outbg", GetPrivateProfileIntA("Message Log", "OutgoingBG", def, szIniFilename));
			DBWriteContactSettingDword(NULL, FONTMODULE, "inputbg", GetPrivateProfileIntA("Message Log", "InputBG", def, szIniFilename));
			
			DBWriteContactSettingDword(NULL, FONTMODULE, "oldinbg", GetPrivateProfileIntA("Message Log", "OldIncomingBG", def, szIniFilename));
			DBWriteContactSettingDword(NULL, FONTMODULE, "oldoutbg", GetPrivateProfileIntA("Message Log", "OldOutgoingBG", def, szIniFilename));
			DBWriteContactSettingDword(NULL, FONTMODULE, "statbg", GetPrivateProfileIntA("Message Log", "StatusBG", def, szIniFilename));

			DBWriteContactSettingDword(NULL, FONTMODULE, "hgrid", GetPrivateProfileIntA("Message Log", "HgridColor", def, szIniFilename));
			DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", GetPrivateProfileIntA("Message Log", "DWFlags", MWF_LOG_DEFAULT, szIniFilename));
			DBWriteContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", (BYTE)(GetPrivateProfileIntA("Message Log", "VGrid", 0, szIniFilename)));
			DBWriteContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", (BYTE)(GetPrivateProfileIntA("Message Log", "ExtraMicroLF", 0, szIniFilename)));

			DBWriteContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", GetPrivateProfileIntA("Message Log", "LeftIndent", 0, szIniFilename));
			DBWriteContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", GetPrivateProfileIntA("Message Log", "RightIndent", 0, szIniFilename));

			DBWriteContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetPrivateProfileIntA("Chat", "UserListBG", def, szIniFilename));

			for (i = 0; i < CUSTOM_COLORS; i++) {
				sprintf(szTemp, "cc%d", i + 1);
				if (dat == 0)
					DBWriteContactSettingDword(NULL, SRMSGMOD_T, szTemp, GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename));
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
				DBWriteContactSettingDword(NULL, "Chat", szTemp, g_Settings.nickColors[i]);
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
		DBWriteContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", (BYTE)(GetPrivateProfileIntA("Message Log", "VGrid", 0, szIniFilename)));
		DBWriteContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", (BYTE)(GetPrivateProfileIntA("Message Log", "ExtraMicroLF", 0, szIniFilename)));

		dat->theme.left_indent = GetPrivateProfileIntA("Message Log", "LeftIndent", 0, szIniFilename);
		dat->theme.right_indent = GetPrivateProfileIntA("Message Log", "RightIndent", 0, szIniFilename);

		for (i = 0; i < CUSTOM_COLORS; i++) {
			sprintf(szTemp, "cc%d", i + 1);
			if (dat == 0)
				DBWriteContactSettingDword(NULL, SRMSGMOD_T, szTemp, GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename));
			else
				dat->theme.custom_colors[i] = GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename);
		}
	}

	if (version >= 3) {
		if (!noAdvanced && dwFlags & THEME_READ_TEMPLATES) {
			for (i = 0; i <= TMPL_ERRMSG; i++) {
#if defined(_UNICODE)
				wchar_t *decoded;

				GetPrivateProfileStringA("Templates", TemplateNames[i], "[undef]", szTemplateBuffer, TEMPLATE_LENGTH * 3, szIniFilename);

				if (strcmp(szTemplateBuffer, "[undef]")) {
					if (dat == 0)
						DBWriteContactSettingStringUtf(NULL, TEMPLATES_MODULE, TemplateNames[i], szTemplateBuffer);
					decoded = Utf8_Decode(szTemplateBuffer);
					if (dat == 0)
						mir_snprintfW(LTR_Active.szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
					else
						mir_snprintfW(dat->ltr_templates->szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
					free(decoded);
				}

				GetPrivateProfileStringA("RTLTemplates", TemplateNames[i], "[undef]", szTemplateBuffer, TEMPLATE_LENGTH * 3, szIniFilename);

				if (strcmp(szTemplateBuffer, "[undef]")) {
					if (dat == 0)
						DBWriteContactSettingStringUtf(NULL, RTLTEMPLATES_MODULE, TemplateNames[i], szTemplateBuffer);
					decoded = Utf8_Decode(szTemplateBuffer);
					if (dat == 0)
						mir_snprintfW(RTL_Active.szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
					else
						mir_snprintfW(dat->rtl_templates->szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
					free(decoded);
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
	if (g_chat_integration_enabled)
		LoadGlobalSettings();
}

/*
 * iMode = 0 - GetOpenFilename, otherwise, GetSaveAs...
 */

char *GetThemeFileName(int iMode)
{
	static char szFilename[MAX_PATH];
	OPENFILENAMEA ofn = {0};
	char szInitialDir[MAX_PATH];

	if(!myGlobals.szSkinsPath&&myGlobals.szDataPath)
		mir_snprintf(szInitialDir, MAX_PATH, "%sskins\\", myGlobals.szDataPath);
	else mir_snprintf(szInitialDir, MAX_PATH,"%s",myGlobals.szSkinsPath);


	szFilename[0] = 0;

	ofn.lpstrFilter = "tabSRMM themes\0*.tabsrmm\0\0";
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = 0;
	ofn.lpstrFile = szFilename;
	ofn.lpstrInitialDir = szInitialDir;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_DONTADDTORECENT;
	ofn.lpstrDefExt = "tabsrmm";
	if (!iMode) {
		if (GetOpenFileNameA(&ofn))
			return szFilename;
		else
			return NULL;
	} else {
		if (GetSaveFileNameA(&ofn))
			return szFilename;
		else
			return NULL;
	}
}

BYTE __forceinline percent_to_byte(UINT32 percent)
{
	return(BYTE)((FLOAT)(((FLOAT) percent) / 100) * 255);
}

COLORREF __forceinline revcolref(COLORREF colref)
{
	return RGB(GetBValue(colref), GetGValue(colref), GetRValue(colref));
}

DWORD __forceinline argb_from_cola(COLORREF col, UINT32 alpha)
{
	return((BYTE) percent_to_byte(alpha) << 24 | col);
}


void DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, int alpha, DWORD basecolor2, BYTE transparent, BYTE FLG_GRADIENT, BYTE FLG_CORNER, DWORD BORDERSTYLE, ImageItem *imageItem)
{
	HBRUSH BrMask;
	HBRUSH holdbrush;
	HDC hdc;
	BLENDFUNCTION bf;
	HBITMAP hbitmap;
	HBITMAP holdbitmap;
	BITMAPINFO bmi;
	VOID *pvBits;
	UINT32 x, y;
	ULONG ulBitmapWidth, ulBitmapHeight;
	UCHAR ubAlpha = 0xFF;
	UCHAR ubRedFinal = 0xFF;
	UCHAR ubGreenFinal = 0xFF;
	UCHAR ubBlueFinal = 0xFF;
	UCHAR ubRed;
	UCHAR ubGreen;
	UCHAR ubBlue;
	UCHAR ubRed2;
	UCHAR ubGreen2;
	UCHAR ubBlue2;

	int realx;

	FLOAT fAlphaFactor;
	LONG realHeight = (rc->bottom - rc->top);
	LONG realWidth = (rc->right - rc->left);
	LONG realHeightHalf = realHeight >> 1;

	if (rc == NULL || MyGradientFill == 0 || MyAlphaBlend == 0)
		return;

	if (imageItem) {
		IMG_RenderImageItem(hdcwnd, imageItem, rc);
		return;
	}

	if (rc->right < rc->left || rc->bottom < rc->top || (realHeight <= 0) || (realWidth <= 0))
		return;

	if (FLG_CORNER == 0) {
		GRADIENT_RECT grect;
		TRIVERTEX tvtx[2];
		int orig = 1, dest = 0;

		if (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_TB) {
			orig = 0;
			dest = 1;
		}

		tvtx[0].x = rc->left;
		tvtx[0].y = rc->top;
		tvtx[1].x = rc->right;
		tvtx[1].y = rc->bottom;

		tvtx[orig].Red = (COLOR16)GetRValue(basecolor) << 8;
		tvtx[orig].Blue = (COLOR16)GetBValue(basecolor) << 8;
		tvtx[orig].Green = (COLOR16)GetGValue(basecolor) << 8;
		tvtx[orig].Alpha = (COLOR16) alpha << 8;

		tvtx[dest].Red = (COLOR16)GetRValue(basecolor2) << 8;
		tvtx[dest].Blue = (COLOR16)GetBValue(basecolor2) << 8;
		tvtx[dest].Green = (COLOR16)GetGValue(basecolor2) << 8;
		tvtx[dest].Alpha = (COLOR16) alpha << 8;

		grect.UpperLeft = 0;
		grect.LowerRight = 1;

		MyGradientFill(hdcwnd, tvtx, 2, &grect, 1, (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT) ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
		return;
	}

	hdc = CreateCompatibleDC(hdcwnd);
	if (!hdc)
		return;

	ZeroMemory(&bmi, sizeof(BITMAPINFO));

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	if (FLG_GRADIENT & GRADIENT_ACTIVE && (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_RL)) {
		bmi.bmiHeader.biWidth = ulBitmapWidth = realWidth;
		bmi.bmiHeader.biHeight = ulBitmapHeight = 1;
	} else if (FLG_GRADIENT & GRADIENT_ACTIVE && (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT)) {
		bmi.bmiHeader.biWidth = ulBitmapWidth = 1;
		bmi.bmiHeader.biHeight = ulBitmapHeight = realHeight;
	} else {
		bmi.bmiHeader.biWidth = ulBitmapWidth = 1;
		bmi.bmiHeader.biHeight = ulBitmapHeight = 1;
	}

	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = ulBitmapWidth * ulBitmapHeight * 4;

	if (ulBitmapWidth <= 0 || ulBitmapHeight <= 0)
		return;

	hbitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	if (hbitmap == NULL || pvBits == NULL) {
		DeleteDC(hdc);
		return;
	}

	holdbitmap = SelectObject(hdc, hbitmap);

	// convert basecolor to RGB and then merge alpha so its ARGB
	basecolor = argb_from_cola(revcolref(basecolor), alpha);
	basecolor2 = argb_from_cola(revcolref(basecolor2), alpha);

	ubRed = (UCHAR)(basecolor >> 16);
	ubGreen = (UCHAR)(basecolor >> 8);
	ubBlue = (UCHAR) basecolor;

	ubRed2 = (UCHAR)(basecolor2 >> 16);
	ubGreen2 = (UCHAR)(basecolor2 >> 8);
	ubBlue2 = (UCHAR) basecolor2;

	//DRAW BASE - make corner space 100% transparent
	for (y = 0; y < ulBitmapHeight; y++) {
		for (x = 0 ; x < ulBitmapWidth ; x++) {
			if (FLG_GRADIENT & GRADIENT_ACTIVE) {
				if (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_RL) {
					realx = x + realHeightHalf;
					realx = (ULONG) realx > ulBitmapWidth ? ulBitmapWidth : realx;
					gradientHorizontal(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, ulBitmapWidth, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, realx, &ubAlpha);
				} else if (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT)
					gradientVertical(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, ulBitmapHeight, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, y, &ubAlpha);

				fAlphaFactor = (float) ubAlpha / (float) 0xff;
				((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
			} else {
				ubAlpha = percent_to_byte(alpha);
				ubRedFinal = ubRed;
				ubGreenFinal = ubGreen;
				ubBlueFinal = ubBlue;
				fAlphaFactor = (float) ubAlpha / (float) 0xff;

				((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
			}
		}
	}
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = (UCHAR)(basecolor >> 24);
	bf.AlphaFormat = AC_SRC_ALPHA; // so it will use our specified alpha value

	MyAlphaBlend(hdcwnd, rc->left + realHeightHalf, rc->top, (realWidth - realHeightHalf * 2), realHeight, hdc, 0, 0, ulBitmapWidth, ulBitmapHeight, bf);

	SelectObject(hdc, holdbitmap);
	DeleteObject(hbitmap);

	// corners
	BrMask = CreateSolidBrush(RGB(0xFF, 0x00, 0xFF));
	{
		bmi.bmiHeader.biWidth = ulBitmapWidth = realHeightHalf;
		bmi.bmiHeader.biHeight = ulBitmapHeight = realHeight;
		bmi.bmiHeader.biSizeImage = ulBitmapWidth * ulBitmapHeight * 4;

		if (ulBitmapWidth <= 0 || ulBitmapHeight <= 0) {
			DeleteDC(hdc);
			DeleteObject(BrMask);
			return;
		}

		// TL+BL CORNER
		hbitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);

		if (hbitmap == 0 || pvBits == NULL)  {
			DeleteObject(BrMask);
			DeleteDC(hdc);
			return;
		}

		holdbrush = SelectObject(hdc, BrMask);
		holdbitmap = SelectObject(hdc, hbitmap);
		RoundRect(hdc, -1, -1, ulBitmapWidth * 2 + 1, (realHeight + 1), BORDERSTYLE, BORDERSTYLE);

		for (y = 0; y < ulBitmapHeight; y++) {
			for (x = 0; x < ulBitmapWidth; x++) {
				if (((((UINT32 *) pvBits)[x + y * ulBitmapWidth]) << 8) == 0xFF00FF00 || (y < ulBitmapHeight >> 1 && !(FLG_CORNER & CORNER_BL && FLG_CORNER & CORNER_ACTIVE)) || (y > ulBitmapHeight >> 2 && !(FLG_CORNER & CORNER_TL && FLG_CORNER & CORNER_ACTIVE))) {
					if (FLG_GRADIENT & GRADIENT_ACTIVE) {
						if (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_RL)
							gradientHorizontal(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, realWidth, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, x, &ubAlpha);
						else if (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT)
							gradientVertical(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, ulBitmapHeight, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, y, &ubAlpha);

						fAlphaFactor = (float) ubAlpha / (float) 0xff;
						((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
					} else {
						ubAlpha = percent_to_byte(alpha);
						ubRedFinal = ubRed;
						ubGreenFinal = ubGreen;
						ubBlueFinal = ubBlue;
						fAlphaFactor = (float) ubAlpha / (float) 0xff;

						((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
					}
				}
			}
		}
		MyAlphaBlend(hdcwnd, rc->left, rc->top, ulBitmapWidth, ulBitmapHeight, hdc, 0, 0, ulBitmapWidth, ulBitmapHeight, bf);
		SelectObject(hdc, holdbitmap);
		DeleteObject(hbitmap);

		// TR+BR CORNER
		hbitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);

		//SelectObject(hdc, BrMask); // already BrMask?
		holdbitmap = SelectObject(hdc, hbitmap);
		RoundRect(hdc, -1 - ulBitmapWidth, -1, ulBitmapWidth + 1, (realHeight + 1), BORDERSTYLE, BORDERSTYLE);

		for (y = 0; y < ulBitmapHeight; y++) {
			for (x = 0; x < ulBitmapWidth; x++) {
				if (((((UINT32 *) pvBits)[x + y * ulBitmapWidth]) << 8) == 0xFF00FF00 || (y < ulBitmapHeight >> 1 && !(FLG_CORNER & CORNER_BR && FLG_CORNER & CORNER_ACTIVE)) || (y > ulBitmapHeight >> 1 && !(FLG_CORNER & CORNER_TR && FLG_CORNER & CORNER_ACTIVE))) {
					if (FLG_GRADIENT & GRADIENT_ACTIVE) {
						if (FLG_GRADIENT & GRADIENT_LR || FLG_GRADIENT & GRADIENT_RL) {
							realx = x + realWidth;
							realx = realx > realWidth ? realWidth : realx;
							gradientHorizontal(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, realWidth, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, realx, &ubAlpha);
						} else if (FLG_GRADIENT & GRADIENT_TB || FLG_GRADIENT & GRADIENT_BT)
							gradientVertical(&ubRedFinal, &ubGreenFinal, &ubBlueFinal, ulBitmapHeight, ubRed, ubGreen, ubBlue, ubRed2, ubGreen2, ubBlue2, FLG_GRADIENT, transparent, y, &ubAlpha);

						fAlphaFactor = (float) ubAlpha / (float) 0xff;
						((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
					} else {
						ubAlpha = percent_to_byte(alpha);
						ubRedFinal = ubRed;
						ubGreenFinal = ubGreen;
						ubBlueFinal = ubBlue;
						fAlphaFactor = (float) ubAlpha / (float) 0xff;

						((UINT32 *) pvBits)[x + y * ulBitmapWidth] = (ubAlpha << 24) | ((UCHAR)(ubRedFinal * fAlphaFactor) << 16) | ((UCHAR)(ubGreenFinal * fAlphaFactor) << 8) | ((UCHAR)(ubBlueFinal * fAlphaFactor));
					}
				}
			}
		}
		MyAlphaBlend(hdcwnd, rc->right - realHeightHalf, rc->top, ulBitmapWidth, ulBitmapHeight, hdc, 0, 0, ulBitmapWidth, ulBitmapHeight, bf);
	}
	SelectObject(hdc, holdbitmap);
	DeleteObject(hbitmap);
	SelectObject(hdc, holdbrush);
	DeleteObject(BrMask);
	DeleteDC(hdc);
}

void __forceinline gradientHorizontal(UCHAR *ubRedFinal, UCHAR *ubGreenFinal, UCHAR *ubBlueFinal, ULONG ulBitmapWidth, UCHAR ubRed, UCHAR ubGreen, UCHAR ubBlue, UCHAR ubRed2, UCHAR ubGreen2, UCHAR ubBlue2, DWORD FLG_GRADIENT, BOOL transparent, UINT32 x, UCHAR *ubAlpha)
{
	FLOAT fSolidMulti, fInvSolidMulti;

	// solid to transparent
	if (transparent) {
		*ubAlpha = (UCHAR)((float) x / (float) ulBitmapWidth * 255);
		*ubAlpha = FLG_GRADIENT & GRADIENT_LR ? 0xFF - (*ubAlpha) : (*ubAlpha);
		*ubRedFinal = ubRed;
		*ubGreenFinal = ubGreen;
		*ubBlueFinal = ubBlue;
	} else { // solid to solid2
		if (FLG_GRADIENT & GRADIENT_LR) {
			fSolidMulti = ((float) x / (float) ulBitmapWidth);
			fInvSolidMulti = 1 - fSolidMulti;
		} else {
			fInvSolidMulti = ((float) x / (float) ulBitmapWidth);
			fSolidMulti = 1 - fInvSolidMulti;
		}

		*ubRedFinal = (UCHAR)((((float) ubRed * (float) fInvSolidMulti)) + (((float) ubRed2 * (float) fSolidMulti)));
		*ubGreenFinal = (UCHAR)((UCHAR)(((float) ubGreen * (float) fInvSolidMulti)) + (((float) ubGreen2 * (float) fSolidMulti)));
		*ubBlueFinal = (UCHAR)((((float) ubBlue * (float) fInvSolidMulti)) + (UCHAR)(((float) ubBlue2 * (float) fSolidMulti)));

		*ubAlpha = 0xFF;
	}
}

void __forceinline gradientVertical(UCHAR *ubRedFinal, UCHAR *ubGreenFinal, UCHAR *ubBlueFinal, ULONG ulBitmapHeight, UCHAR ubRed, UCHAR ubGreen, UCHAR ubBlue, UCHAR ubRed2, UCHAR ubGreen2, UCHAR ubBlue2, DWORD FLG_GRADIENT, BOOL transparent, UINT32 y, UCHAR *ubAlpha)
{
	FLOAT fSolidMulti, fInvSolidMulti;

	// solid to transparent
	if (transparent) {
		*ubAlpha = (UCHAR)((float) y / (float) ulBitmapHeight * 255);
		*ubAlpha = FLG_GRADIENT & GRADIENT_BT ? 0xFF - *ubAlpha : *ubAlpha;
		*ubRedFinal = ubRed;
		*ubGreenFinal = ubGreen;
		*ubBlueFinal = ubBlue;
	} else { // solid to solid2
		if (FLG_GRADIENT & GRADIENT_BT) {
			fSolidMulti = ((float) y / (float) ulBitmapHeight);
			fInvSolidMulti = 1 - fSolidMulti;
		} else {
			fInvSolidMulti = ((float) y / (float) ulBitmapHeight);
			fSolidMulti = 1 - fInvSolidMulti;
		}

		*ubRedFinal = (UCHAR)((((float) ubRed * (float) fInvSolidMulti)) + (((float) ubRed2 * (float) fSolidMulti)));
		*ubGreenFinal = (UCHAR)((((float) ubGreen * (float) fInvSolidMulti)) + (((float) ubGreen2 * (float) fSolidMulti)));
		*ubBlueFinal = (UCHAR)(((float) ubBlue * (float) fInvSolidMulti)) + (UCHAR)(((float) ubBlue2 * (float) fSolidMulti));

		*ubAlpha = 0xFF;
	}
}

/*
 * render a skin image to the given rect.
 * all parameters are in ImageItem already pre-configured
 */

// TODO: add support for more stretching options (stretch/tile divided image parts etc.

void __fastcall IMG_RenderImageItem(HDC hdc, ImageItem *item, RECT *rc)
{
	BYTE l = item->bLeft, r = item->bRight, t = item->bTop, b = item->bBottom;
	LONG width = rc->right - rc->left;
	LONG height = rc->bottom - rc->top;
	BOOL isGlyph = (item->dwFlags & IMAGE_GLYPH) && g_glyphItem;
	BOOL fCleanUp = TRUE;
	HDC hdcSrc = 0;
	HBITMAP hbmOld;
	LONG srcOrigX = isGlyph ? item->glyphMetrics[0] : 0;
	LONG srcOrigY = isGlyph ? item->glyphMetrics[1] : 0;


	if (item->hdc == 0) {
		hdcSrc = CreateCompatibleDC(hdc);
		hbmOld = SelectObject(hdcSrc, isGlyph ? g_glyphItem->hbm : item->hbm);
	} else {
		hdcSrc = isGlyph ? g_glyphItem->hdc : item->hdc;
		fCleanUp = FALSE;
	}

	if (MyAlphaBlend == 0)
		goto img_render_done;

	if (item->dwFlags & IMAGE_FLAG_DIVIDED) {
		// top 3 items

		MyAlphaBlend(hdc, rc->left, rc->top, l, t, hdcSrc, srcOrigX, srcOrigY, l, t, item->bf);
		MyAlphaBlend(hdc, rc->left + l, rc->top, width - l - r, t, hdcSrc, srcOrigX + l, srcOrigY, item->inner_width, t, item->bf);
		MyAlphaBlend(hdc, rc->right - r, rc->top, r, t, hdcSrc, srcOrigX + (item->width - r), srcOrigY, r, t, item->bf);

		// middle 3 items

		MyAlphaBlend(hdc, rc->left, rc->top + t, l, height - t - b, hdcSrc, srcOrigX, srcOrigY + t, l, item->inner_height, item->bf);

		if (item->dwFlags & IMAGE_FILLSOLID && item->fillBrush) {
			RECT rcFill;
			rcFill.left = rc->left + l;
			rcFill.top = rc->top + t;
			rcFill.right = rc->right - r;
			rcFill.bottom = rc->bottom - b;
			FillRect(hdc, &rcFill, item->fillBrush);
		} else
			MyAlphaBlend(hdc, rc->left + l, rc->top + t, width - l - r, height - t - b, hdcSrc, srcOrigX + l, srcOrigY + t, item->inner_width, item->inner_height, item->bf);

		MyAlphaBlend(hdc, rc->right - r, rc->top + t, r, height - t - b, hdcSrc, srcOrigX + (item->width - r), srcOrigY + t, r, item->inner_height, item->bf);

		// bottom 3 items

		MyAlphaBlend(hdc, rc->left, rc->bottom - b, l, b, hdcSrc, srcOrigX, srcOrigY + (item->height - b), l, b, item->bf);
		MyAlphaBlend(hdc, rc->left + l, rc->bottom - b, width - l - r, b, hdcSrc, srcOrigX + l, srcOrigY + (item->height - b), item->inner_width, b, item->bf);
		MyAlphaBlend(hdc, rc->right - r, rc->bottom - b, r, b, hdcSrc, srcOrigX + (item->width - r), srcOrigY + (item->height - b), r, b, item->bf);
	} else {
		switch (item->bStretch) {
			case IMAGE_STRETCH_H:
				// tile image vertically, stretch to width
			{
				LONG top = rc->top;

				do {
					if (top + item->height <= rc->bottom) {
						MyAlphaBlend(hdc, rc->left, top, width, item->height, hdcSrc, srcOrigX, srcOrigY, item->width, item->height, item->bf);
						top += item->height;
					} else {
						MyAlphaBlend(hdc, rc->left, top, width, rc->bottom - top, hdcSrc, srcOrigX, srcOrigY, item->width, rc->bottom - top, item->bf);
						break;
					}
				} while (TRUE);
				break;
			}
			case IMAGE_STRETCH_V:
				// tile horizontally, stretch to height
			{
				LONG left = rc->left;

				do {
					if (left + item->width <= rc->right) {
						MyAlphaBlend(hdc, left, rc->top, item->width, height, hdcSrc, srcOrigX, srcOrigY, item->width, item->height, item->bf);
						left += item->width;
					} else {
						MyAlphaBlend(hdc, left, rc->top, rc->right - left, height, hdcSrc, srcOrigX, srcOrigY, rc->right - left, item->height, item->bf);
						break;
					}
				} while (TRUE);
				break;
			}
			case IMAGE_STRETCH_B:
				// stretch the image in both directions...
				MyAlphaBlend(hdc, rc->left, rc->top, width, height, hdcSrc, srcOrigX, srcOrigY, item->width, item->height, item->bf);
				break;
				/*
				case IMAGE_STRETCH_V:
				    // stretch vertically, draw 3 horizontal tiles...
				    AlphaBlend(hdc, rc->left, rc->top, l, height, item->hdc, 0, 0, l, item->height, item->bf);
				    AlphaBlend(hdc, rc->left + l, rc->top, width - l - r, height, item->hdc, l, 0, item->inner_width, item->height, item->bf);
				    AlphaBlend(hdc, rc->right - r, rc->top, r, height, item->hdc, item->width - r, 0, r, item->height, item->bf);
				    break;
				case IMAGE_STRETCH_H:
				    // stretch horizontally, draw 3 vertical tiles...
				    AlphaBlend(hdc, rc->left, rc->top, width, t, item->hdc, 0, 0, item->width, t, item->bf);
				    AlphaBlend(hdc, rc->left, rc->top + t, width, height - t - b, item->hdc, 0, t, item->width, item->inner_height, item->bf);
				    AlphaBlend(hdc, rc->left, rc->bottom - b, width, b, item->hdc, 0, item->height - b, item->width, b, item->bf);
				    break;
				*/
			default:
				break;
		}
	}
img_render_done:
	if (fCleanUp) {
		SelectObject(hdcSrc, hbmOld);
		DeleteDC(hdcSrc);
	}
}

static DWORD __fastcall HexStringToLong(const char *szSource)
{
	char *stopped;
	COLORREF clr = strtol(szSource, &stopped, 16);
	if (clr == -1)
		return clr;
	return(RGB(GetBValue(clr), GetGValue(clr), GetRValue(clr)));
}

static StatusItems_t StatusItem_Default = {
	"Container", "EXBK_Offline", ID_EXTBKCONTAINER,
	CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT,
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
};

static void BTN_ReadItem(char *itemName, char *file)
{
	ButtonItem tmpItem, *newItem;
	char szBuffer[1024];
	ImageItem *imgItem = g_ImageItems;
	HICON *phIcon;

	ZeroMemory(&tmpItem, sizeof(tmpItem));
	mir_snprintf(tmpItem.szName, sizeof(tmpItem.szName), "%s", &itemName[1]);
	tmpItem.width = GetPrivateProfileIntA(itemName, "Width", 16, file);
	tmpItem.height = GetPrivateProfileIntA(itemName, "Height", 16, file);
	tmpItem.xOff = GetPrivateProfileIntA(itemName, "xoff", 0, file);
	tmpItem.yOff = GetPrivateProfileIntA(itemName, "yoff", 0, file);

	tmpItem.dwFlags |= GetPrivateProfileIntA(itemName, "toggle", 0, file) ? BUTTON_ISTOGGLE : 0;

	GetPrivateProfileStringA(itemName, "Pressed", "None", szBuffer, 1000, file);
	if (!_stricmp(szBuffer, "default"))
		tmpItem.imgPressed = StatusItems[ID_EXTBKBUTTONSPRESSED].imageItem;
	else {
		while (imgItem) {
			if (!_stricmp(imgItem->szName, szBuffer)) {
				tmpItem.imgPressed = imgItem;
				break;
			}
			imgItem = imgItem->nextItem;
		}
	}

	imgItem = g_ImageItems;
	GetPrivateProfileStringA(itemName, "Normal", "None", szBuffer, 1000, file);
	if (!_stricmp(szBuffer, "default"))
		tmpItem.imgNormal = StatusItems[ID_EXTBKBUTTONSNPRESSED].imageItem;
	else {
		while (imgItem) {
			if (!_stricmp(imgItem->szName, szBuffer)) {
				tmpItem.imgNormal = imgItem;
				break;
			}
			imgItem = imgItem->nextItem;
		}
	}

	imgItem = g_ImageItems;
	GetPrivateProfileStringA(itemName, "Hover", "None", szBuffer, 1000, file);
	if (!_stricmp(szBuffer, "default"))
		tmpItem.imgHover = StatusItems[ID_EXTBKBUTTONSMOUSEOVER].imageItem;
	else {
		while (imgItem) {
			if (!_stricmp(imgItem->szName, szBuffer)) {
				tmpItem.imgHover = imgItem;
				break;
			}
			imgItem = imgItem->nextItem;
		}
	}

	tmpItem.uId = IDC_TBFIRSTUID - 1;
	tmpItem.pfnAction = tmpItem.pfnCallback = NULL;

	if (GetPrivateProfileIntA(itemName, "Sidebar", 0, file)) {
		tmpItem.dwFlags |= BUTTON_ISSIDEBAR;
		myGlobals.m_SideBarEnabled = TRUE;
		SIDEBARWIDTH = max(tmpItem.width + 2, SIDEBARWIDTH);
	}

	GetPrivateProfileStringA(itemName, "Action", "Custom", szBuffer, 1000, file);
	if (!_stricmp(szBuffer, "service")) {
		tmpItem.szService[0] = 0;
		GetPrivateProfileStringA(itemName, "Service", "None", szBuffer, 1000, file);
		if (_stricmp(szBuffer, "None")) {
			mir_snprintf(tmpItem.szService, 256, "%s", szBuffer);
			tmpItem.dwFlags |= BUTTON_ISSERVICE;
			tmpItem.uId = nextButtonID++;
		}
	} else if (!_stricmp(szBuffer, "protoservice")) {
		tmpItem.szService[0] = 0;
		GetPrivateProfileStringA(itemName, "Service", "None", szBuffer, 1000, file);
		if (_stricmp(szBuffer, "None")) {
			mir_snprintf(tmpItem.szService, 256, "%s", szBuffer);
			tmpItem.dwFlags |= BUTTON_ISPROTOSERVICE;
			tmpItem.uId = nextButtonID++;
		}
	} else if (!_stricmp(szBuffer, "database")) {
		int n;

		GetPrivateProfileStringA(itemName, "Module", "None", szBuffer, 1000, file);
		if (_stricmp(szBuffer, "None"))
			mir_snprintf(tmpItem.szModule, 256, "%s", szBuffer);
		GetPrivateProfileStringA(itemName, "Setting", "None", szBuffer, 1000, file);
		if (_stricmp(szBuffer, "None"))
			mir_snprintf(tmpItem.szSetting, 256, "%s", szBuffer);
		if (GetPrivateProfileIntA(itemName, "contact", 0, file) != 0)
			tmpItem.dwFlags |= BUTTON_DBACTIONONCONTACT;

		for (n = 0; n <= 1; n++) {
			char szKey[20];
			BYTE *pValue;

			strcpy(szKey, n == 0 ? "dbonpush" : "dbonrelease");
			pValue = (n == 0 ? tmpItem.bValuePush : tmpItem.bValueRelease);

			GetPrivateProfileStringA(itemName, szKey, "None", szBuffer, 1000, file);
			switch (szBuffer[0]) {
				case 'b': {
					BYTE value = (BYTE)atol(&szBuffer[1]);
					pValue[0] = value;
					tmpItem.type = DBVT_BYTE;
					break;
				}
				case 'w': {
					WORD value = (WORD)atol(&szBuffer[1]);
					*((WORD *)&pValue[0]) = value;
					tmpItem.type = DBVT_WORD;
					break;
				}
				case 'd': {
					DWORD value = (DWORD)atol(&szBuffer[1]);
					*((DWORD *)&pValue[0]) = value;
					tmpItem.type = DBVT_DWORD;
					break;
				}
				case 's': {
					mir_snprintf((char *)pValue, 256, &szBuffer[1]);
					tmpItem.type = DBVT_ASCIIZ;
					break;
				}
			}
		}
		if (tmpItem.szModule[0] && tmpItem.szSetting[0]) {
			tmpItem.dwFlags |= BUTTON_ISDBACTION;
			if (tmpItem.szModule[0] == '$' && (tmpItem.szModule[1] == 'c' || tmpItem.szModule[1] == 'C'))
				tmpItem.dwFlags |= BUTTON_ISCONTACTDBACTION;
			tmpItem.uId = nextButtonID++;
		}
	} else if (_stricmp(szBuffer, "Custom")) {
		if (BTN_GetStockItem(&tmpItem, szBuffer))
			goto create_it;
	}
	GetPrivateProfileStringA(itemName, "PassContact", "None", szBuffer, 1000, file);
	if (_stricmp(szBuffer, "None")) {
		if (szBuffer[0] == 'w' || szBuffer[0] == 'W')
			tmpItem.dwFlags |= BUTTON_PASSHCONTACTW;
		else if (szBuffer[0] == 'l' || szBuffer[0] == 'L')
			tmpItem.dwFlags |= BUTTON_PASSHCONTACTL;
	}

	GetPrivateProfileStringA(itemName, "Tip", "None", szBuffer, 1000, file);
	if (strcmp(szBuffer, "None")) {
#if defined(_UNICODE)
		MultiByteToWideChar(myGlobals.m_LangPackCP, 0, szBuffer, -1, tmpItem.szTip, 256);
		tmpItem.szTip[255] = 0;
#else
		mir_snprintf(tmpItem.szTip, 256, "%s", szBuffer);
#endif
	} else
		tmpItem.szTip[0] = 0;

create_it:

	GetPrivateProfileStringA(itemName, "Label", "None", szBuffer, 40, file);
	if (strcmp(szBuffer, "None")) {
#if defined(_UNICODE)
		MultiByteToWideChar(myGlobals.m_LangPackCP, 0, szBuffer, -1, tmpItem.tszLabel, 40);
		tmpItem.tszLabel[39] = 0;
#else
		mir_snprintf(tmpItem.tszLabel, 40, "%s", szBuffer);
#endif
		tmpItem.dwFlags |= BUTTON_HASLABEL;
	} else
		tmpItem.tszLabel[0] = 0;

	GetPrivateProfileStringA(itemName, "NormalGlyph", "0, 0, 0, 0", szBuffer, 1000, file);
	if (_stricmp(szBuffer, "default")) {
		tmpItem.dwFlags &= ~BUTTON_NORMALGLYPHISICON;
		if ((phIcon = BTN_GetIcon(szBuffer)) != 0) {
			tmpItem.dwFlags |= BUTTON_NORMALGLYPHISICON;
			tmpItem.normalGlyphMetrics[0] = (LONG)phIcon;
		} else {
			sscanf(szBuffer, "%d,%d,%d,%d", &tmpItem.normalGlyphMetrics[0], &tmpItem.normalGlyphMetrics[1],
				   &tmpItem.normalGlyphMetrics[2], &tmpItem.normalGlyphMetrics[3]);
			tmpItem.normalGlyphMetrics[2] = (tmpItem.normalGlyphMetrics[2] - tmpItem.normalGlyphMetrics[0]) + 1;
			tmpItem.normalGlyphMetrics[3] = (tmpItem.normalGlyphMetrics[3] - tmpItem.normalGlyphMetrics[1]) + 1;
		}
	}

	GetPrivateProfileStringA(itemName, "PressedGlyph", "0, 0, 0, 0", szBuffer, 1000, file);
	if (_stricmp(szBuffer, "default")) {
		tmpItem.dwFlags &= ~BUTTON_PRESSEDGLYPHISICON;
		if ((phIcon = BTN_GetIcon(szBuffer)) != 0) {
			tmpItem.pressedGlyphMetrics[0] = (LONG)phIcon;
			tmpItem.dwFlags |= BUTTON_PRESSEDGLYPHISICON;
		} else {
			sscanf(szBuffer, "%d,%d,%d,%d", &tmpItem.pressedGlyphMetrics[0], &tmpItem.pressedGlyphMetrics[1],
				   &tmpItem.pressedGlyphMetrics[2], &tmpItem.pressedGlyphMetrics[3]);
			tmpItem.pressedGlyphMetrics[2] = (tmpItem.pressedGlyphMetrics[2] - tmpItem.pressedGlyphMetrics[0]) + 1;
			tmpItem.pressedGlyphMetrics[3] = (tmpItem.pressedGlyphMetrics[3] - tmpItem.pressedGlyphMetrics[1]) + 1;
		}
	}

	GetPrivateProfileStringA(itemName, "HoverGlyph", "0, 0, 0, 0", szBuffer, 1000, file);
	if (_stricmp(szBuffer, "default")) {
		tmpItem.dwFlags &= ~BUTTON_HOVERGLYPHISICON;
		if ((phIcon = BTN_GetIcon(szBuffer)) != 0) {
			tmpItem.hoverGlyphMetrics[0] = (LONG)phIcon;
			tmpItem.dwFlags |= BUTTON_HOVERGLYPHISICON;
		} else {
			sscanf(szBuffer, "%d,%d,%d,%d", &tmpItem.hoverGlyphMetrics[0], &tmpItem.hoverGlyphMetrics[1],
				   &tmpItem.hoverGlyphMetrics[2], &tmpItem.hoverGlyphMetrics[3]);
			tmpItem.hoverGlyphMetrics[2] = (tmpItem.hoverGlyphMetrics[2] - tmpItem.hoverGlyphMetrics[0]) + 1;
			tmpItem.hoverGlyphMetrics[3] = (tmpItem.hoverGlyphMetrics[3] - tmpItem.hoverGlyphMetrics[1]) + 1;
		}
	}

	/*
	 * calculate client margins...
	 */

	newItem = (ButtonItem *)malloc(sizeof(ButtonItem));
	ZeroMemory(newItem, sizeof(ButtonItem));
	if (g_ButtonSet.items == NULL) {
		g_ButtonSet.items = newItem;
		*newItem = tmpItem;
		newItem->nextItem = 0;
	} else {
		ButtonItem *curItem = g_ButtonSet.items;
		while (curItem->nextItem)
			curItem = curItem->nextItem;
		*newItem = tmpItem;
		newItem->nextItem = 0;
		curItem->nextItem = newItem;
	}
	return;
}

static void ReadItem(StatusItems_t *this_item, char *szItem, char *file)
{
	char buffer[512], def_color[20];
	COLORREF clr;
	StatusItems_t *defaults = &StatusItem_Default;

	this_item->ALPHA = (int)GetPrivateProfileIntA(szItem, "Alpha", defaults->ALPHA, file);
	this_item->ALPHA = min(this_item->ALPHA, 100);

	clr = RGB(GetBValue(defaults->COLOR), GetGValue(defaults->COLOR), GetRValue(defaults->COLOR));
	_snprintf(def_color, 15, "%6.6x", clr);
	GetPrivateProfileStringA(szItem, "Color1", def_color, buffer, 400, file);
	this_item->COLOR = HexStringToLong(buffer);

	clr = RGB(GetBValue(defaults->COLOR2), GetGValue(defaults->COLOR2), GetRValue(defaults->COLOR2));
	_snprintf(def_color, 15, "%6.6x", clr);
	GetPrivateProfileStringA(szItem, "Color2", def_color, buffer, 400, file);
	this_item->COLOR2 = HexStringToLong(buffer);

	this_item->COLOR2_TRANSPARENT = (BYTE)GetPrivateProfileIntA(szItem, "COLOR2_TRANSPARENT", defaults->COLOR2_TRANSPARENT, file);

	this_item->CORNER = defaults->CORNER & CORNER_ACTIVE ? defaults->CORNER : 0;
	GetPrivateProfileStringA(szItem, "Corner", "None", buffer, 400, file);
	if (strstr(buffer, "tl"))
		this_item->CORNER |= CORNER_TL;
	if (strstr(buffer, "tr"))
		this_item->CORNER |= CORNER_TR;
	if (strstr(buffer, "bl"))
		this_item->CORNER |= CORNER_BL;
	if (strstr(buffer, "br"))
		this_item->CORNER |= CORNER_BR;
	if (this_item->CORNER)
		this_item->CORNER |= CORNER_ACTIVE;

	this_item->GRADIENT = defaults->GRADIENT & GRADIENT_ACTIVE ?  defaults->GRADIENT : 0;
	GetPrivateProfileStringA(szItem, "Gradient", "None", buffer, 400, file);
	if (strstr(buffer, "left"))
		this_item->GRADIENT = GRADIENT_RL;
	else if (strstr(buffer, "right"))
		this_item->GRADIENT = GRADIENT_LR;
	else if (strstr(buffer, "up"))
		this_item->GRADIENT = GRADIENT_BT;
	else if (strstr(buffer, "down"))
		this_item->GRADIENT = GRADIENT_TB;
	if (this_item->GRADIENT)
		this_item->GRADIENT |= GRADIENT_ACTIVE;

	this_item->MARGIN_LEFT = GetPrivateProfileIntA(szItem, "Left", defaults->MARGIN_LEFT, file);
	this_item->MARGIN_RIGHT = GetPrivateProfileIntA(szItem, "Right", defaults->MARGIN_RIGHT, file);
	this_item->MARGIN_TOP = GetPrivateProfileIntA(szItem, "Top", defaults->MARGIN_TOP, file);
	this_item->MARGIN_BOTTOM = GetPrivateProfileIntA(szItem, "Bottom", defaults->MARGIN_BOTTOM, file);
	this_item->BORDERSTYLE = GetPrivateProfileIntA(szItem, "Radius", defaults->BORDERSTYLE, file);

	GetPrivateProfileStringA(szItem, "Textcolor", "ffffffff", buffer, 400, file);
	this_item->TEXTCOLOR = HexStringToLong(buffer);
	this_item->IGNORED = 0;
}

static void IMG_ReadItem(const char *itemname, const char *szFileName)
{
	ImageItem tmpItem = {0}, *newItem = NULL;
	char buffer[512], szItemNr[30];
	char szFinalName[MAX_PATH];
	HDC hdc = GetDC(0);
	int i, n;
	BOOL alloced = FALSE;
	char szDrive[MAX_PATH], szPath[MAX_PATH];

	ZeroMemory(&tmpItem, sizeof(tmpItem));
	GetPrivateProfileStringA(itemname, "Glyph", "None", buffer, 500, szFileName);
	if (strcmp(buffer, "None")) {
		sscanf(buffer, "%d,%d,%d,%d", &tmpItem.glyphMetrics[0], &tmpItem.glyphMetrics[1],
			   &tmpItem.glyphMetrics[2], &tmpItem.glyphMetrics[3]);
		if (tmpItem.glyphMetrics[2] > tmpItem.glyphMetrics[0] && tmpItem.glyphMetrics[3] > tmpItem.glyphMetrics[1]) {
			tmpItem.dwFlags |= IMAGE_GLYPH;
			tmpItem.glyphMetrics[2] = (tmpItem.glyphMetrics[2] - tmpItem.glyphMetrics[0]) + 1;
			tmpItem.glyphMetrics[3] = (tmpItem.glyphMetrics[3] - tmpItem.glyphMetrics[1]) + 1;
			goto done_with_glyph;
		}
	}
	GetPrivateProfileStringA(itemname, "Image", "None", buffer, 500, szFileName);
	if (strcmp(buffer, "None")) {

done_with_glyph:

		strncpy(tmpItem.szName, &itemname[1], sizeof(tmpItem.szName));
		tmpItem.szName[sizeof(tmpItem.szName) - 1] = 0;
		_splitpath(szFileName, szDrive, szPath, NULL, NULL);
		mir_snprintf(szFinalName, MAX_PATH, "%s\\%s\\%s", szDrive, szPath, buffer);
		tmpItem.alpha = GetPrivateProfileIntA(itemname, "Alpha", 100, szFileName);
		tmpItem.alpha = min(tmpItem.alpha, 100);
		tmpItem.alpha = (BYTE)((FLOAT)(((FLOAT) tmpItem.alpha) / 100) * 255);
		tmpItem.bf.SourceConstantAlpha = tmpItem.alpha;
		tmpItem.bLeft = GetPrivateProfileIntA(itemname, "Left", 0, szFileName);
		tmpItem.bRight = GetPrivateProfileIntA(itemname, "Right", 0, szFileName);
		tmpItem.bTop = GetPrivateProfileIntA(itemname, "Top", 0, szFileName);
		tmpItem.bBottom = GetPrivateProfileIntA(itemname, "Bottom", 0, szFileName);
		if (tmpItem.dwFlags & IMAGE_GLYPH) {
			tmpItem.width = tmpItem.glyphMetrics[2];
			tmpItem.height = tmpItem.glyphMetrics[3];
			tmpItem.inner_height = tmpItem.glyphMetrics[3] - tmpItem.bTop - tmpItem.bBottom;
			tmpItem.inner_width = tmpItem.glyphMetrics[2] - tmpItem.bRight - tmpItem.bLeft;

			if (tmpItem.bTop && tmpItem.bBottom && tmpItem.bLeft && tmpItem.bRight)
				tmpItem.dwFlags |= IMAGE_FLAG_DIVIDED;
			tmpItem.bf.BlendFlags = 0;
			tmpItem.bf.BlendOp = AC_SRC_OVER;
			tmpItem.bf.AlphaFormat = 0;
			tmpItem.dwFlags |= IMAGE_PERPIXEL_ALPHA;
			tmpItem.bf.AlphaFormat = AC_SRC_ALPHA;
			if (tmpItem.inner_height <= 0 || tmpItem.inner_width <= 0)
				return;
		}
		GetPrivateProfileStringA(itemname, "Fillcolor", "None", buffer, 500, szFileName);
		if (strcmp(buffer, "None")) {
			COLORREF fillColor = HexStringToLong(buffer);
			tmpItem.fillBrush = CreateSolidBrush(fillColor);
			tmpItem.dwFlags |= IMAGE_FILLSOLID;
		} else
			tmpItem.fillBrush = 0;
		GetPrivateProfileStringA(itemname, "Colorkey", "None", buffer, 500, szFileName);
		if (strcmp(buffer, "None")) {
			g_ContainerColorKey = HexStringToLong(buffer);
			if (g_ContainerColorKeyBrush)
				DeleteObject(g_ContainerColorKeyBrush);
			g_ContainerColorKeyBrush = CreateSolidBrush(g_ContainerColorKey);
		}
		GetPrivateProfileStringA(itemname, "Stretch", "None", buffer, 500, szFileName);
		if (buffer[0] == 'B' || buffer[0] == 'b')
			tmpItem.bStretch = IMAGE_STRETCH_B;
		else if (buffer[0] == 'h' || buffer[0] == 'H')
			tmpItem.bStretch = IMAGE_STRETCH_V;
		else if (buffer[0] == 'w' || buffer[0] == 'W')
			tmpItem.bStretch = IMAGE_STRETCH_H;
		tmpItem.hbm = 0;
		if (GetPrivateProfileIntA(itemname, "Perpixel", 0, szFileName))
			tmpItem.dwFlags |= IMAGE_PERPIXEL_ALPHA;

		if (!_stricmp(itemname, "$glyphs")) {
			IMG_CreateItem(&tmpItem, szFinalName, hdc);
			if (tmpItem.hbm) {
				newItem = malloc(sizeof(ImageItem));
				ZeroMemory(newItem, sizeof(ImageItem));
				*newItem = tmpItem;
				g_glyphItem = newItem;
			}
			goto imgread_done;
		}

		for (n = 0;;n++) {
			mir_snprintf(szItemNr, 30, "Item%d", n);
			GetPrivateProfileStringA(itemname, szItemNr, "None", buffer, 500, szFileName);
			if (!strcmp(buffer, "None"))
				break;
			for (i = 0; i <= ID_EXTBK_LAST; i++) {
				if (!_stricmp(StatusItems[i].szName[0] == '{' ? &StatusItems[i].szName[3] : StatusItems[i].szName, buffer)) {
					if (!alloced) {
						if (!(tmpItem.dwFlags & IMAGE_GLYPH))
							IMG_CreateItem(&tmpItem, szFinalName, hdc);
						if (tmpItem.hbm || tmpItem.dwFlags & IMAGE_GLYPH) {
							newItem = malloc(sizeof(ImageItem));
							ZeroMemory(newItem, sizeof(ImageItem));
							*newItem = tmpItem;
							StatusItems[i].imageItem = newItem;
							if (g_ImageItems == NULL)
								g_ImageItems = newItem;
							else {
								ImageItem *pItem = g_ImageItems;

								while (pItem->nextItem != 0)
									pItem = pItem->nextItem;
								pItem->nextItem = newItem;
							}
							alloced = TRUE;
						}
					} else if (newItem != NULL)
						StatusItems[i].imageItem = newItem;
				}
			}
		}
	}
imgread_done:
	ReleaseDC(0, hdc);
}

static void CorrectBitmap32Alpha(HBITMAP hBitmap)
{
	BITMAP bmp;
	DWORD dwLen;
	BYTE *p;
	int x, y;
	BOOL fixIt = TRUE;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	if (bmp.bmBitsPixel != 32)
		return;

	dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		return;
	memset(p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, p);

	for (y = 0; y < bmp.bmHeight; ++y) {
		BYTE *px = p + bmp.bmWidth * 4 * y;

		for (x = 0; x < bmp.bmWidth; ++x) {
			if (px[3] != 0)
				fixIt = FALSE;
			else
				px[3] = 255;
			px += 4;
		}
	}

	if (fixIt)
		SetBitmapBits(hBitmap, bmp.bmWidth * bmp.bmHeight * 4, p);

	free(p);
}

static void PreMultiply(HBITMAP hBitmap, int mode)
{
	BYTE *p = NULL;
	DWORD dwLen;
	int width, height, x, y;
	BITMAP bmp;
	BYTE alpha;

	GetObject(hBitmap, sizeof(bmp), &bmp);
	width = bmp.bmWidth;
	height = bmp.bmHeight;
	dwLen = width * height * 4;
	p = (BYTE *)malloc(dwLen);
	if (p) {
		GetBitmapBits(hBitmap, dwLen, p);
		for (y = 0; y < height; ++y) {
			BYTE *px = p + width * 4 * y;

			for (x = 0; x < width; ++x) {
				if (mode) {
					alpha = px[3];
					px[0] = px[0] * alpha / 255;
					px[1] = px[1] * alpha / 255;
					px[2] = px[2] * alpha / 255;
				} else
					px[3] = 255;
				px += 4;
			}
		}
		dwLen = SetBitmapBits(hBitmap, dwLen, p);
		free(p);
	}
}

static HBITMAP LoadPNG(const char *szFilename, ImageItem *item)
{
	LPVOID imgDecoder = NULL;
	LPVOID pImg = NULL;
	HBITMAP hBitmap = 0;
	LPVOID pBitmapBits = NULL;
	LPVOID m_pImgDecoder = NULL;

	hBitmap = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (LPARAM)szFilename);
	if (hBitmap != 0)
		CorrectBitmap32Alpha(hBitmap);

	return hBitmap;
}

static void IMG_CreateItem(ImageItem *item, const char *fileName, HDC hdc)
{
	HBITMAP hbm = LoadPNG(fileName, item);
	BITMAP bm;

	if (hbm) {
		item->hbm = hbm;
		item->bf.BlendFlags = 0;
		item->bf.BlendOp = AC_SRC_OVER;
		item->bf.AlphaFormat = 0;

		GetObject(hbm, sizeof(bm), &bm);
		if (bm.bmBitsPixel == 32 && item->dwFlags & IMAGE_PERPIXEL_ALPHA) {
			PreMultiply(hbm, 1);
			item->bf.AlphaFormat = AC_SRC_ALPHA;
		}
		item->width = bm.bmWidth;
		item->height = bm.bmHeight;
		item->inner_height = item->height - item->bTop - item->bBottom;
		item->inner_width = item->width - item->bLeft - item->bRight;
		if (item->bTop && item->bBottom && item->bLeft && item->bRight) {
			item->dwFlags |= IMAGE_FLAG_DIVIDED;
			if (item->inner_height <= 0 || item->inner_width <= 0) {
				DeleteObject(hbm);
				item->hbm = 0;
				return;
			}
		}
		item->hdc = 0;
		item->hbmOld = 0;
	}
}

static void IMG_DeleteItem(ImageItem *item)
{
	if (!(item->dwFlags & IMAGE_GLYPH)) {
		//SelectObject(item->hdc, item->hbmOld);
		DeleteObject(item->hbm);
		//DeleteDC(item->hdc);
	}
	if (item->fillBrush)
		DeleteObject(item->fillBrush);
}

void IMG_DeleteItems()
{
	ImageItem *pItem = g_ImageItems, *pNextItem;
	ButtonItem *pbItem = g_ButtonSet.items, *pbNextItem;
	int i;

	if (g_skinIcons) {
		for (i = 0; i < g_nrSkinIcons; i++) {
			if (g_skinIcons[i].szName)
				free(g_skinIcons[i].szName);
			DestroyIcon((HICON)g_skinIcons[i].uId);
		}
		g_nrSkinIcons = 0;
	}

	while (pItem) {
		IMG_DeleteItem(pItem);
		pNextItem = pItem->nextItem;
		free(pItem);
		pItem = pNextItem;
	}
	g_ImageItems = NULL;

	if (g_glyphItem) {
		IMG_DeleteItem(g_glyphItem);
		free(g_glyphItem);
	}
	g_glyphItem = NULL;

	while (pbItem) {
		//DestroyWindow(pbItem->hWnd);
		pbNextItem = pbItem->nextItem;
		free(pbItem);
		pbItem = pbNextItem;
	}
	ZeroMemory(&g_ButtonSet, sizeof(ButtonSet));
	myGlobals.m_SideBarEnabled = FALSE;

	if (myGlobals.g_closeGlyph)
		DestroyIcon(myGlobals.g_closeGlyph);
	if (myGlobals.g_minGlyph)
		DestroyIcon(myGlobals.g_minGlyph);
	if (myGlobals.g_maxGlyph)
		DestroyIcon(myGlobals.g_maxGlyph);
	if (g_ContainerColorKeyBrush)
		DeleteObject(g_ContainerColorKeyBrush);
	if (myGlobals.g_pulldownGlyph)
		DeleteObject(myGlobals.g_pulldownGlyph);

	myGlobals.g_minGlyph = myGlobals.g_maxGlyph = myGlobals.g_closeGlyph = myGlobals.g_pulldownGlyph = 0;
	g_ContainerColorKeyBrush = 0;
	myGlobals.g_DisableScrollbars = FALSE;
}

static void SkinLoadIcon(char *file, char *szSection, char *name, HICON *hIcon)
{
	char buffer[512];
	if (*hIcon != 0)
		DestroyIcon(*hIcon);
	GetPrivateProfileStringA(szSection, name, "none", buffer, 500, file);
	buffer[500] = 0;

	if (strcmp(buffer, "none")) {
		char szDrive[MAX_PATH], szDir[MAX_PATH], szImagePath[MAX_PATH];

		_splitpath(file, szDrive, szDir, NULL, NULL);
		mir_snprintf(szImagePath, MAX_PATH, "%s\\%s\\%s", szDrive, szDir, buffer);
		if (hIcon)
			*hIcon = LoadImageA(0, szImagePath, IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
	}
}

static void IMG_LoadItems(char *szFileName)
{
	char *szSections = NULL;
	char *p, *p1;
	ImageItem *pItem = g_ImageItems, *pNextItem;
	int i;
	ICONDESC tmpIconDesc = {0};

	if (!PathFileExistsA(szFileName))
		return;

	while (pItem) {
		IMG_DeleteItem(pItem);
		pNextItem = pItem->nextItem;
		free(pItem);
		pItem = pNextItem;
	}
	g_ImageItems = NULL;

	for (i = 0; i <= ID_EXTBK_LAST; i++)
		StatusItems[i].imageItem = NULL;

	if (g_skinIcons == NULL)
		g_skinIcons = malloc(sizeof(ICONDESC) * NR_MAXSKINICONS);

	ZeroMemory(g_skinIcons, sizeof(ICONDESC) * NR_MAXSKINICONS);

	szSections = malloc(5002);
	ZeroMemory(szSections, 5002);

	GetPrivateProfileSectionA("Icons", szSections, 5000, szFileName);
	p = szSections;
	while (lstrlenA(p) > 1) {
		p1 = strchr(p, (int)'=');
		if (p1)
			*p1 = 0;
		if (g_nrSkinIcons < NR_MAXSKINICONS && p1) {
			SkinLoadIcon(szFileName, "Icons", p, (HICON *)&tmpIconDesc.uId);
			if (tmpIconDesc.uId) {
				ZeroMemory(&g_skinIcons[g_nrSkinIcons], sizeof(ICONDESC));
				g_skinIcons[g_nrSkinIcons].uId = tmpIconDesc.uId;
				g_skinIcons[g_nrSkinIcons].phIcon = (HICON *)(&g_skinIcons[g_nrSkinIcons].uId);
				g_skinIcons[g_nrSkinIcons].szName = malloc(lstrlenA(p) + 1);
				lstrcpyA(g_skinIcons[g_nrSkinIcons].szName, p);
				g_nrSkinIcons++;
			}
		}
		if (p1)
			*p1 = '=';
		p += (lstrlenA(p) + 1);
	}

	ZeroMemory(szSections, 5002);
	p = szSections;
	GetPrivateProfileSectionNamesA(szSections, 5000, szFileName);

	szSections[5001] = szSections[5000] = 0;
	p = szSections;
	while (lstrlenA(p) > 1) {
		if (p[0] == '$')
			IMG_ReadItem(p, szFileName);
		p += (lstrlenA(p) + 1);
	}
	nextButtonID = IDC_TBFIRSTUID;
	SIDEBARWIDTH = DEFAULT_SIDEBARWIDTH;

	p = szSections;
	while (lstrlenA(p) > 1) {
		if (p[0] == '!')
			BTN_ReadItem(p, szFileName);
		p += (lstrlenA(p) + 1);
	}
	free(szSections);
	g_ButtonSet.top = GetPrivateProfileIntA("ButtonArea", "top", 0, szFileName);
	g_ButtonSet.bottom = GetPrivateProfileIntA("ButtonArea", "bottom", 0, szFileName);
	g_ButtonSet.left = GetPrivateProfileIntA("ButtonArea", "left", 0, szFileName);
	g_ButtonSet.right = GetPrivateProfileIntA("ButtonArea", "right", 0, szFileName);
}

static void SkinCalcFrameWidth()
{
	int xBorder, yBorder, yCaption;

	xBorder = GetSystemMetrics(SM_CXSIZEFRAME);
	yBorder = GetSystemMetrics(SM_CYSIZEFRAME);
	yCaption = GetSystemMetrics(SM_CYCAPTION);

#ifdef _DEBUG
	_DebugTraceA("SPI: %d, %d, %d", xBorder, yBorder, yCaption);
#endif
	myGlobals.g_realSkinnedFrame_left = myGlobals.g_SkinnedFrame_left - xBorder;
	myGlobals.g_realSkinnedFrame_right = myGlobals.g_SkinnedFrame_right - xBorder;
	myGlobals.g_realSkinnedFrame_bottom = myGlobals.g_SkinnedFrame_bottom - yBorder;
	myGlobals.g_realSkinnedFrame_caption = myGlobals.g_SkinnedFrame_caption - yCaption;
#ifdef _DEBUG
	_DebugTraceA("Real frame metrics: %d, %d, %d, %d", myGlobals.g_realSkinnedFrame_left, myGlobals.g_realSkinnedFrame_right,
				 myGlobals.g_realSkinnedFrame_caption, myGlobals.g_realSkinnedFrame_bottom);
#endif
}


static struct {
	char *szIniKey, *szIniName;
	char *szSetting;
	unsigned int size;
	int defaultval;
} _tagSettings[] = {
	"Global", "SbarHeight", "S_sbarheight", 1, 22,
	"ClientArea", "Left", "S_tborder_outer_left", 1, 0,
	"ClientArea", "Right", "S_tborder_outer_right", 1, 0,
	"ClientArea", "Top", "S_tborder_outer_top", 1, 0,
	"ClientArea", "Bottom", "S_tborder_outer_bottom", 1, 0,
	"ClientArea", "Inner", "S_tborder", 1, 0,
	"Global", "TabTextNormal", "S_tab_txt_normal", 5, 0,
	"Global", "TabTextActive", "S_tab_txt_active", 5, 0,
	"Global", "TabTextUnread", "S_tab_txt_unread", 5, 0,
	"Global", "TabTextHottrack", "S_tab_txt_hottrack", 5, 0,
	NULL, NULL, NULL, 0, 0
};

static void LoadSkinItems(char *file, int onStartup)
{
	char *p;
	char *szSections = malloc(3002);
	int i = 1, j = 0;
	UINT data;
	char buffer[500];

	i = 0;
	while (_tagSettings[i].szIniKey != NULL) {
		data = 0;
		data = GetPrivateProfileIntA(_tagSettings[i].szIniKey, _tagSettings[i].szIniName, _tagSettings[i].defaultval, file);
		switch (_tagSettings[i].size) {
			case 1:
				DBWriteContactSettingByte(NULL, SRMSGMOD_T, _tagSettings[i].szSetting, (BYTE)data);
				break;
			case 4:
				DBWriteContactSettingDword(NULL, SRMSGMOD_T, _tagSettings[i].szSetting, data);
				break;
			case 2:
				DBWriteContactSettingWord(NULL, SRMSGMOD_T, _tagSettings[i].szSetting, (WORD)data);
				break;
			case 5:
				GetPrivateProfileStringA(_tagSettings[i].szIniKey, _tagSettings[i].szIniName, "000000", buffer, 10, file);
				DBWriteContactSettingDword(NULL, SRMSGMOD_T, _tagSettings[i].szSetting, HexStringToLong(buffer));
				break;
		}
		i++;
	}

	if (!(GetPrivateProfileIntA("Global", "Version", 0, file) >= 1 && GetPrivateProfileIntA("Global", "Signature", 0, file) == 101))
		return;

	myGlobals.g_DisableScrollbars = FALSE;

	ZeroMemory(szSections, 3000);
	p = szSections;
	GetPrivateProfileSectionNamesA(szSections, 3000, file);
	szSections[3001] = szSections[3000] = 0;
	p = szSections;
	while (lstrlenA(p) > 1) {
		if (p[0] != '%') {
			p += (lstrlenA(p) + 1);
			continue;
		}
		for (i = 0; i <= ID_EXTBK_LAST; i++) {
			if (!_stricmp(&p[1], StatusItems[i].szName[0] == '{' ? &StatusItems[i].szName[3] : StatusItems[i].szName)) {
				ReadItem(&StatusItems[i], p, file);
				break;
			}
		}
		p += (lstrlenA(p) + 1);
		j++;
	}

	if (j > 0)
		g_skinnedContainers = TRUE;

	myGlobals.bAvatarBoderType = GetPrivateProfileIntA("Avatars", "BorderType", 0, file);
	GetPrivateProfileStringA("Avatars", "BorderColor", "000000", buffer, 20, file);
	DBWriteContactSettingDword(NULL, SRMSGMOD_T, "avborderclr", HexStringToLong(buffer));

	SkinLoadIcon(file, "Global", "CloseGlyph", &myGlobals.g_closeGlyph);
	SkinLoadIcon(file, "Global", "MaximizeGlyph", &myGlobals.g_maxGlyph);
	SkinLoadIcon(file, "Global", "MinimizeGlyph", &myGlobals.g_minGlyph);

	g_framelessSkinmode = GetPrivateProfileIntA("Global", "framelessmode", 0, file);
	myGlobals.g_DisableScrollbars = GetPrivateProfileIntA("Global", "NoScrollbars", 0, file);

	data = GetPrivateProfileIntA("Global", "SkinnedTabs", 1, file);
	myGlobals.m_TabAppearance = data ? myGlobals.m_TabAppearance | TCF_NOSKINNING : myGlobals.m_TabAppearance & ~TCF_NOSKINNING;
	DBWriteContactSettingDword(NULL, SRMSGMOD_T, "tabconfig", myGlobals.m_TabAppearance);

	myGlobals.g_SkinnedFrame_left = GetPrivateProfileIntA("WindowFrame", "left", 4, file);
	myGlobals.g_SkinnedFrame_right = GetPrivateProfileIntA("WindowFrame", "right", 4, file);
	myGlobals.g_SkinnedFrame_caption = GetPrivateProfileIntA("WindowFrame", "Caption", 24, file);
	myGlobals.g_SkinnedFrame_bottom = GetPrivateProfileIntA("WindowFrame", "bottom", 4, file);

	g_titleBarButtonSize.cx = GetPrivateProfileIntA("WindowFrame", "TitleButtonWidth", 24, file);
	g_titleBarButtonSize.cy = GetPrivateProfileIntA("WindowFrame", "TitleButtonHeight", 12, file);
	g_titleButtonTopOff = GetPrivateProfileIntA("WindowFrame", "TitleButtonTopOffset", 0, file);

	g_titleBarRightOff = GetPrivateProfileIntA("WindowFrame", "TitleBarRightOffset", 0, file);
	g_titleBarLeftOff = GetPrivateProfileIntA("WindowFrame", "TitleBarLeftOffset", 0, file);

	g_captionOffset = GetPrivateProfileIntA("WindowFrame", "CaptionOffset", 3, file);
	g_captionPadding = GetPrivateProfileIntA("WindowFrame", "CaptionPadding", 0, file);
	g_sidebarTopOffset = GetPrivateProfileIntA("ClientArea", "SidebarTop", -1, file);
	g_sidebarBottomOffset = GetPrivateProfileIntA("ClientArea", "SidebarBottom", -1, file);

	myGlobals.bClipBorder = GetPrivateProfileIntA("WindowFrame", "ClipFrame", 0, file);
	{
		BYTE radius_tl, radius_tr, radius_bl, radius_br;
		char szFinalName[MAX_PATH];
		char szDrive[MAX_PATH], szPath[MAX_PATH];

		radius_tl = GetPrivateProfileIntA("WindowFrame", "RadiusTL", 0, file);
		radius_tr = GetPrivateProfileIntA("WindowFrame", "RadiusTR", 0, file);
		radius_bl = GetPrivateProfileIntA("WindowFrame", "RadiusBL", 0, file);
		radius_br = GetPrivateProfileIntA("WindowFrame", "RadiusBR", 0, file);

		myGlobals.bRoundedCorner = radius_tl;

		GetPrivateProfileStringA("Theme", "File", "None", buffer, MAX_PATH, file);

		_splitpath(file, szDrive, szPath, NULL, NULL);
		mir_snprintf(szFinalName, MAX_PATH, "%s\\%s\\%s", szDrive, szPath, buffer);
		if (PathFileExistsA(szFinalName)) {
			ReadThemeFromINI(szFinalName, 0, FALSE, onStartup ? 0 : DBGetContactSettingByte(NULL, SRMSGMOD_T, "skin_loadmode", 0));
			CacheLogFonts();
			CacheMsgLogIcons();
		}
	}
	GetPrivateProfileStringA("Global", "MenuBarBG", "None", buffer, 20, file);
	data = HexStringToLong(buffer);
	if (g_MenuBGBrush) {
		DeleteObject(g_MenuBGBrush);
		g_MenuBGBrush = 0;
	}
	if (strcmp(buffer, "None"))
		g_MenuBGBrush = CreateSolidBrush(data);

	GetPrivateProfileStringA("Global", "LightShadow", "000000", buffer, 20, file);
	data = HexStringToLong(buffer);
	myGlobals.g_SkinLightShadowPen = CreatePen(PS_SOLID, 1, RGB(GetRValue(data), GetGValue(data), GetBValue(data)));
	GetPrivateProfileStringA("Global", "DarkShadow", "000000", buffer, 20, file);
	data = HexStringToLong(buffer);
	myGlobals.g_SkinDarkShadowPen = CreatePen(PS_SOLID, 1, RGB(GetRValue(data), GetGValue(data), GetBValue(data)));

	SkinCalcFrameWidth();

	GetPrivateProfileStringA("Global", "FontColor", "None", buffer, 20, file);
	if (strcmp(buffer, "None"))
		myGlobals.skinDefaultFontColor = HexStringToLong(buffer);
	else
		myGlobals.skinDefaultFontColor = GetSysColor(COLOR_BTNTEXT);
	buffer[499] = 0;
	FreeTabConfig();
	ReloadTabConfig();
	free(szSections);
}

void SkinDrawBG(HWND hwndClient, HWND hwnd, struct ContainerWindowData *pContainer, RECT *rcClient, HDC hdcTarget)
{
	RECT rcWindow;
	POINT pt;
	LONG width = rcClient->right - rcClient->left;
	LONG height = rcClient->bottom - rcClient->top;
	HDC dc;

	GetWindowRect(hwndClient, &rcWindow);
	pt.x = rcWindow.left + rcClient->left;
	pt.y = rcWindow.top + rcClient->top;
	ScreenToClient(hwnd, &pt);
	if (pContainer)
		dc = pContainer->cachedDC;
	else
		dc = GetDC(hwnd);
	BitBlt(hdcTarget, rcClient->left, rcClient->top, width, height, dc, pt.x, pt.y, SRCCOPY);
	if (!pContainer)
		ReleaseDC(hwnd, dc);
}

void ReloadContainerSkin(int doLoad, int onStartup)
{
	DBVARIANT dbv = {0};
	char szFinalPath[MAX_PATH];
	int i;

	g_skinnedContainers = g_framelessSkinmode = FALSE;
	myGlobals.bClipBorder = 0;
	myGlobals.bRoundedCorner = 0;
	if (g_MenuBGBrush) {
		DeleteObject(g_MenuBGBrush);
		g_MenuBGBrush = 0;
	}
	if (pFirstContainer) {
		if (MessageBox(0, TranslateT("All message containers need to close before the skin can be changed\nProceed?"), TranslateT("Change skin"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
			struct ContainerWindowData *pContainer = pFirstContainer;
			while (pFirstContainer)
				SendMessage(pFirstContainer->hwnd, WM_CLOSE, 0, 1);
		} else
			return;
	}

	for (i = 0; i <= ID_EXTBK_LAST; i++)
		StatusItems[i].IGNORED = 1;

	if (myGlobals.g_SkinLightShadowPen)
		DeleteObject(myGlobals.g_SkinLightShadowPen);
	if (myGlobals.g_SkinDarkShadowPen)
		DeleteObject(myGlobals.g_SkinDarkShadowPen);

	IMG_DeleteItems();

	if (doLoad) {
		if (pSetLayeredWindowAttributes == 0 || MyAlphaBlend == 0)
			return;

		if (!DBGetContactSettingString(NULL, SRMSGMOD_T, "ContainerSkin", &dbv)) {
			MY_pathToAbsolute(dbv.pszVal, szFinalPath);
			LoadSkinItems(szFinalPath, onStartup);
			IMG_LoadItems(szFinalPath);
			DBFreeVariant(&dbv);
		}
	}
}

static BLENDFUNCTION bf_t = {0};

void DrawDimmedIcon(HDC hdc, LONG left, LONG top, LONG dx, LONG dy, HICON hIcon, BYTE alpha)
{
	HDC dcMem = CreateCompatibleDC(hdc);
	HBITMAP hbm = CreateCompatibleBitmap(hdc, dx, dy), hbmOld = 0;

	hbmOld = SelectObject(dcMem, hbm);
	BitBlt(dcMem, 0, 0, dx, dx, hdc, left, top, SRCCOPY);
	DrawIconEx(dcMem, 0, 0, hIcon, dx, dy, 0, 0, DI_NORMAL);
	bf_t.SourceConstantAlpha = alpha;
	if (MyAlphaBlend)
		MyAlphaBlend(hdc, left, top, dx, dy, dcMem, 0, 0, dx, dy, bf_t);
	else {
		SetStretchBltMode(hdc, HALFTONE);
		StretchBlt(hdc, left, top, dx, dy, dcMem, 0, 0, dx, dy, SRCCOPY);
	}
	SelectObject(dcMem, hbmOld);
	DeleteObject(hbm);
	DeleteDC(dcMem);
}
