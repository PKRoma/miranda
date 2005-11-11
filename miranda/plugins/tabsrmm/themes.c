/*

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

*/

#include "commonheaders.h"
#pragma hdrstop

#include "msgs.h"
#include "msgdlgutils.h"

/*
 * writes the current "theme" to .ini file
 * a theme contains all the fonts, colors and message log formatting
 * options.
 */
 
#define CURRENT_THEME_VERSION 4
#define THEME_COOKIE 25099837

extern char *TemplateNames[];
extern TemplateSet LTR_Active, RTL_Active;

static void LoadLogfontFromINI(int i, char *szKey, LOGFONTA *lf, COLORREF *colour, const char *szIniFilename)
{
    int style;
    char bSize;

    if (colour)
        *colour = GetPrivateProfileIntA(szKey, "Color", 0, szIniFilename);

    if (lf) {
        HDC hdc = GetDC(NULL);
        if(i == H_MSGFONTID_DIVIDERS)
            lf->lfHeight = 5;
        else {
            bSize = (char)GetPrivateProfileIntA(szKey, "Size", -12, szIniFilename);
            if(bSize > 0)
                lf->lfHeight = -MulDiv(bSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            else
                lf->lfHeight = bSize;
        }

        ReleaseDC(NULL,hdc);				
        
        lf->lfWidth = 0;
        lf->lfEscapement = 0;
        lf->lfOrientation = 0;
        style = (int)GetPrivateProfileIntA(szKey, "Style", 0, szIniFilename);
        lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
        lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
        lf->lfUnderline = style & FONTF_UNDERLINE ? 1 : 0;
        lf->lfStrikeOut = 0;
        if(i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
            lf->lfCharSet = SYMBOL_CHARSET;
        else
            lf->lfCharSet = (BYTE)GetPrivateProfileIntA(szKey, "Set", DEFAULT_CHARSET, szIniFilename);
        lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf->lfQuality = DEFAULT_QUALITY;
        lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        if(i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT) {
            lstrcpynA(lf->lfFaceName, "Webdings", LF_FACESIZE);
            lf->lfCharSet = SYMBOL_CHARSET;
        }
        else
            GetPrivateProfileStringA(szKey, "Face", "Tahoma", lf->lfFaceName, LF_FACESIZE - 1, szIniFilename);
    }
}

int CheckThemeVersion(const char *szIniFilename)
{
    int cookie = GetPrivateProfileIntA("TabSRMM Theme", "Cookie", 0, szIniFilename);
    int version = GetPrivateProfileIntA("TabSRMM Theme", "Version", 0, szIniFilename);

    if(version >= CURRENT_THEME_VERSION && cookie == THEME_COOKIE)
        return 1;
    return 0;
}

void WriteThemeToINI(const char *szIniFilename, struct MessageWindowData *dat)
{
    int i;
    DBVARIANT dbv;
    char szBuf[100], szTemp[100], szAppname[100];
    
    WritePrivateProfileStringA("TabSRMM Theme", "Version", _itoa(CURRENT_THEME_VERSION, szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("TabSRMM Theme", "Cookie", _itoa(THEME_COOKIE, szBuf, 10), szIniFilename);
    
    if(dat == 0) {
        for(i = 0; i < MSGDLGFONTCOUNT; i++) {
            sprintf(szTemp, "Font%d", i);
            strcpy(szAppname, szTemp);
            if(!DBGetContactSetting(NULL, FONTMODULE, szTemp, &dbv)) {
                WritePrivateProfileStringA(szAppname, "Face", dbv.pszVal, szIniFilename);
                DBFreeVariant(&dbv);
            }
            sprintf(szTemp, "Font%dCol", i);
            WritePrivateProfileStringA(szAppname, "Color", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, szTemp, 0), szBuf, 10), szIniFilename);
            sprintf(szTemp, "Font%dSty", i);
            WritePrivateProfileStringA(szAppname, "Style", _itoa(DBGetContactSettingByte(NULL, FONTMODULE, szTemp, 0), szBuf, 10), szIniFilename);
            sprintf(szTemp, "Font%dSize", i);
            WritePrivateProfileStringA(szAppname, "Size", _itoa(DBGetContactSettingByte(NULL, FONTMODULE, szTemp, 0), szBuf, 10), szIniFilename);
            sprintf(szTemp, "Font%dSet", i);
            WritePrivateProfileStringA(szAppname, "Set", _itoa(DBGetContactSettingByte(NULL, FONTMODULE, szTemp, 0), szBuf, 10), szIniFilename);
            sprintf(szTemp, "Font%dAs", i);
            WritePrivateProfileStringA(szAppname, "Inherit", _itoa(DBGetContactSettingWord(NULL, FONTMODULE, szTemp, 0), szBuf, 10), szIniFilename);
        }
        WritePrivateProfileStringA("Message Log", "BackgroundColor", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR), szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "IncomingBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "inbg", RGB(224, 224, 224)), szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "OutgoingBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "outbg", RGB(224, 224, 224)), szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "InputBG", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", RGB(224, 224, 224)), szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "HgridColor", _itoa(DBGetContactSettingDword(NULL, FONTMODULE, "hgrid", RGB(224, 224, 224)), szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "DWFlags", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT), szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "VGrid", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", 0), szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "ExtraMicroLF", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", 0), szBuf, 10), szIniFilename);

        WritePrivateProfileStringA("Message Log", "LeftIndent", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 0), szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "RightIndent", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 0), szBuf, 10), szIniFilename);
    }
    else {
        DWORD style;
        char bSize;
        DWORD dwSize;
        HDC hDC = GetDC(NULL);
        for(i = 0; i < MSGDLGFONTCOUNT; i++) {
            sprintf(szTemp, "Font%d", i);
            strcpy(szAppname, szTemp);
            WritePrivateProfileStringA(szAppname, "Face", dat->theme.logFonts[i].lfFaceName, szIniFilename);
            WritePrivateProfileStringA(szAppname, "Color", _itoa(dat->theme.fontColors[i], szBuf, 10), szIniFilename);
            style = (dat->theme.logFonts[i].lfUnderline ? FONTF_UNDERLINE : 0) | (dat->theme.logFonts[i].lfWeight == FW_BOLD ? FONTF_BOLD : 0) | (dat->theme.logFonts[i].lfItalic ? FONTF_ITALIC : 0);
            bSize = (char)dat->theme.logFonts[i].lfHeight;
            WritePrivateProfileStringA(szAppname, "Style", _itoa(style, szBuf, 10), szIniFilename);
            if(bSize > 0)
                bSize = -bSize;
            dwSize = (unsigned char)bSize;
            WritePrivateProfileStringA(szAppname, "Size", _ultoa(dwSize, szBuf, 10), szIniFilename);
            WritePrivateProfileStringA(szAppname, "Set", _itoa(dat->theme.logFonts[i].lfCharSet, szBuf, 10), szIniFilename);
            WritePrivateProfileStringA(szAppname, "Inherit", _itoa(DBGetContactSettingWord(NULL, FONTMODULE, szTemp, 0), szBuf, 10), szIniFilename);
        }
        ReleaseDC(NULL, hDC);
        WritePrivateProfileStringA("Message Log", "BackgroundColor", _itoa(dat->theme.bg, szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "IncomingBG", _itoa(dat->theme.inbg, szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "OutgoingBG", _itoa(dat->theme.outbg, szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "InputBG", _itoa(dat->theme.inputbg, szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "HgridColor", _itoa(dat->theme.hgrid, szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "DWFlags", _itoa(dat->dwFlags & MWF_LOG_ALL, szBuf, 10), szIniFilename);

        WritePrivateProfileStringA("Message Log", "LeftIndent", _itoa(dat->theme.left_indent / 15, szBuf, 10), szIniFilename);
        WritePrivateProfileStringA("Message Log", "RightIndent", _itoa(dat->theme.right_indent / 15, szBuf, 10), szIniFilename);
    }

    for(i = 0; i <= TMPL_ERRMSG; i++) {
#if defined(_UNICODE)
        char *encoded;
        if(dat == 0)
            encoded = Utf8_Encode(LTR_Active.szTemplates[i]);
        else
            encoded = Utf8_Encode(dat->ltr_templates->szTemplates[i]);
        WritePrivateProfileStringA("Templates", TemplateNames[i], encoded, szIniFilename);
        free(encoded);
        if(dat == 0)
            encoded = Utf8_Encode(RTL_Active.szTemplates[i]);
        else
            encoded = Utf8_Encode(dat->rtl_templates->szTemplates[i]);
        WritePrivateProfileStringA("RTLTemplates", TemplateNames[i], encoded, szIniFilename);
        free(encoded);
#else
        if(dat == 0) {
            WritePrivateProfileStringA("Templates", TemplateNames[i], LTR_Active.szTemplates[i], szIniFilename);
            WritePrivateProfileStringA("RTLTemplates", TemplateNames[i], RTL_Active.szTemplates[i], szIniFilename);
        }
        else {
            WritePrivateProfileStringA("Templates", TemplateNames[i], dat->ltr_templates->szTemplates[i], szIniFilename);
            WritePrivateProfileStringA("RTLTemplates", TemplateNames[i], dat->rtl_templates->szTemplates[i], szIniFilename);
        }
#endif        
    }

    for(i = 0; i < CUSTOM_COLORS; i++) {
        sprintf(szTemp, "cc%d", i + 1);
        if(dat == 0)
            WritePrivateProfileStringA("Custom Colors", szTemp, _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, szTemp, 0), szBuf, 10), szIniFilename);
        else
            WritePrivateProfileStringA("Custom Colors", szTemp, _itoa(dat->theme.custom_colors[i], szBuf, 10), szIniFilename);
    }
}

void ReadThemeFromINI(const char *szIniFilename, struct MessageWindowData *dat, int noAdvanced)
{
    char szBuf[512], szTemp[100], szAppname[100];
    int i;
    int version;
    char szTemplateBuffer[TEMPLATE_LENGTH * 3 + 2];
    char bSize = 0;
    HDC hdc;
    int charset;
    
    if((version = GetPrivateProfileIntA("TabSRMM Theme", "Version", 0, szIniFilename)) == 0)         // no version number.. assume 1
        version = 1;
    
    if(dat == 0) {
        hdc = GetDC(NULL);

        for(i = 0; i < MSGDLGFONTCOUNT; i++) {
            sprintf(szTemp, "Font%d", i);
            strcpy(szAppname, szTemp);
            if(GetPrivateProfileStringA(szAppname, "Face", "Arial", szBuf, sizeof(szBuf), szIniFilename) != 0) {
                if(i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
                    lstrcpynA(szBuf, "Arial", sizeof(szBuf));
                DBWriteContactSettingString(NULL, FONTMODULE, szTemp, szBuf);
            }

            sprintf(szTemp, "Font%dCol", i);
            DBWriteContactSettingDword(NULL, FONTMODULE, szTemp, GetPrivateProfileIntA(szAppname, "Color", RGB(224, 224, 224), szIniFilename));

            sprintf(szTemp, "Font%dSty", i);
            DBWriteContactSettingByte(NULL, FONTMODULE, szTemp, GetPrivateProfileIntA(szAppname, "Style", 0, szIniFilename));

            sprintf(szTemp, "Font%dSize", i);
            bSize = (char)GetPrivateProfileIntA(szAppname, "Size", 0, szIniFilename);
            if(bSize > 0)
                bSize = -MulDiv(bSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            DBWriteContactSettingByte(NULL, FONTMODULE, szTemp, bSize);

            sprintf(szTemp, "Font%dSet", i);
            charset = GetPrivateProfileIntA(szAppname, "Set", 0, szIniFilename);
            if(i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
                charset = 0;
            DBWriteContactSettingByte(NULL, FONTMODULE, szTemp, (BYTE)charset);

            sprintf(szTemp, "Font%dAs", i);
            DBWriteContactSettingWord(NULL, FONTMODULE, szTemp, GetPrivateProfileIntA(szAppname, "Inherit", 0, szIniFilename));
        }
        ReleaseDC(NULL, hdc);
        DBWriteContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, GetPrivateProfileIntA("Message Log", "BackgroundColor", RGB(224, 224, 224), szIniFilename));
        DBWriteContactSettingDword(NULL, FONTMODULE, "inbg", GetPrivateProfileIntA("Message Log", "IncomingBG", RGB(224, 224, 224), szIniFilename));
        DBWriteContactSettingDword(NULL, FONTMODULE, "outbg", GetPrivateProfileIntA("Message Log", "OutgoingBG", RGB(224, 224, 224), szIniFilename));
        DBWriteContactSettingDword(NULL, FONTMODULE, "inputbg", GetPrivateProfileIntA("Message Log", "InputBG", RGB(224, 224, 224), szIniFilename));
        DBWriteContactSettingDword(NULL, FONTMODULE, "hgrid", GetPrivateProfileIntA("Message Log", "HgridColor", RGB(224, 224, 224), szIniFilename));
        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", GetPrivateProfileIntA("Message Log", "DWFlags", MWF_LOG_DEFAULT, szIniFilename));
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", GetPrivateProfileIntA("Message Log", "VGrid", 0, szIniFilename));
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", GetPrivateProfileIntA("Message Log", "ExtraMicroLF", 0, szIniFilename));

        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", GetPrivateProfileIntA("Message Log", "LeftIndent", 0, szIniFilename));
        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", GetPrivateProfileIntA("Message Log", "RightIndent", 0, szIniFilename));
    }
    else {
        HDC hdc = GetDC(NULL);
        int SY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
        if(!noAdvanced) {
            for(i = 0; i < MSGDLGFONTCOUNT; i++) {
                _snprintf(szTemp, 20, "Font%d", i);
                LoadLogfontFromINI(i, szTemp, &dat->theme.logFonts[i], &dat->theme.fontColors[i], szIniFilename);
                wsprintfA(dat->theme.rtfFonts + (i * RTFCACHELINESIZE), "\\f%u\\cf%u\\b%d\\i%d\\ul%d\\fs%u", i, i, dat->theme.logFonts[i].lfWeight >= FW_BOLD ? 1 : 0, dat->theme.logFonts[i].lfItalic, dat->theme.logFonts[i].lfUnderline, 2 * abs(dat->theme.logFonts[i].lfHeight) * 74 / SY);
            }
            wsprintfA(dat->theme.rtfFonts + (MSGDLGFONTCOUNT * RTFCACHELINESIZE), "\\f%u\\cf%u\\b%d\\i%d\\ul%d\\fs%u", MSGDLGFONTCOUNT, MSGDLGFONTCOUNT, 0, 0, 0, 0);
        }
        dat->theme.bg = GetPrivateProfileIntA("Message Log", "BackgroundColor", RGB(224, 224, 224), szIniFilename);
        dat->theme.inbg = GetPrivateProfileIntA("Message Log", "IncomingBG", RGB(224, 224, 224), szIniFilename);
        dat->theme.outbg = GetPrivateProfileIntA("Message Log", "OutgoingBG", RGB(224, 224, 224), szIniFilename);
        dat->theme.inputbg = GetPrivateProfileIntA("Message Log", "InputBG", RGB(224, 224, 224), szIniFilename);
        dat->theme.hgrid = GetPrivateProfileIntA("Message Log", "HgridColor", RGB(224, 224, 224), szIniFilename);
        dat->theme.dwFlags = GetPrivateProfileIntA("Message Log", "DWFlags", MWF_LOG_DEFAULT, szIniFilename);
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", GetPrivateProfileIntA("Message Log", "VGrid", 0, szIniFilename));
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", GetPrivateProfileIntA("Message Log", "ExtraMicroLF", 0, szIniFilename));

        dat->theme.left_indent = GetPrivateProfileIntA("Message Log", "LeftIndent", 0, szIniFilename);
        dat->theme.right_indent = GetPrivateProfileIntA("Message Log", "RightIndent", 0, szIniFilename);
    }

    if(version >= 3) {
        if(!noAdvanced) {
            for(i = 0; i <= TMPL_ERRMSG; i++) {
    #if defined(_UNICODE)
                wchar_t *decoded;
                GetPrivateProfileStringA("Templates", TemplateNames[i], "", szTemplateBuffer, TEMPLATE_LENGTH * 3, szIniFilename);
                if(dat == 0)
                    DBWriteContactSettingStringUtf(NULL, TEMPLATES_MODULE, TemplateNames[i], szTemplateBuffer);
                decoded = Utf8_Decode(szTemplateBuffer);
                if(dat == 0)
                    mir_snprintfW(LTR_Active.szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
                else
                    mir_snprintfW(dat->ltr_templates->szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
                free(decoded);
                GetPrivateProfileStringA("RTLTemplates", TemplateNames[i], "", szTemplateBuffer, TEMPLATE_LENGTH * 3, szIniFilename);
                if(dat == 0)
                    DBWriteContactSettingStringUtf(NULL, RTLTEMPLATES_MODULE, TemplateNames[i], szTemplateBuffer);
                decoded = Utf8_Decode(szTemplateBuffer);
                if(dat == 0)
                    mir_snprintfW(RTL_Active.szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
                else
                    mir_snprintfW(dat->rtl_templates->szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
                free(decoded);
    #else
                if(dat == 0) {
                    GetPrivateProfileStringA("Templates", TemplateNames[i], "", LTR_Active.szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
                    GetPrivateProfileStringA("RTLTemplates", TemplateNames[i], "", RTL_Active.szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
                }
                else {
                    GetPrivateProfileStringA("Templates", TemplateNames[i], "", dat->ltr_templates->szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
                    GetPrivateProfileStringA("RTLTemplates", TemplateNames[i], "", dat->rtl_templates->szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
                }
    #endif        
            }
        }
        for(i = 0; i < CUSTOM_COLORS; i++) {
            sprintf(szTemp, "cc%d", i + 1);
            if(dat == 0)
                DBWriteContactSettingDword(NULL, SRMSGMOD_T, szTemp, GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename));
            else
                dat->theme.custom_colors[i] = GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename);
        }
    }
}

/*
 * iMode = 0 - GetOpenFilename, otherwise, GetSaveAs...
 */

char *GetThemeFileName(int iMode)
{
    static char szFilename[MAX_PATH];
    OPENFILENAMEA ofn={0};
    char szFilter[MAX_PATH];

    strncpy(szFilter, "tabSRMM themes\0*.tabsrmm\0\0", MAX_PATH);
    ofn.lStructSize= OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner=0;
    ofn.lpstrFile = szFilename;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrInitialDir = ".";
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.Flags = OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "tabsrmm";
    if(!iMode) {
        if (GetOpenFileNameA(&ofn))
            return szFilename;
        else
            return NULL;
    }
    else {
        if (GetSaveFileNameA(&ofn))
            return szFilename;
        else
            return NULL;
    }
}

