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
 
#define CURRENT_THEME_VERSION 3

extern char *TemplateNames[];
extern TemplateSet LTR_Active, RTL_Active;

void WriteThemeToINI(const char *szIniFilename)
{
    int i;
    DBVARIANT dbv;
    char szBuf[100], szTemp[100], szAppname[100];
    
    WritePrivateProfileStringA("TabSRMM Theme", "Version", _itoa(CURRENT_THEME_VERSION, szBuf, 10), szIniFilename);
    
    for(i = 0; i < MSGDLGFONTCOUNT; i++) {
        sprintf(szTemp, "Font%d", i);
        strcpy(szAppname, szTemp);
        if(!DBGetContactSetting(NULL, SRMSGMOD_T, szTemp, &dbv)) {
            WritePrivateProfileStringA(szAppname, "Face", dbv.pszVal, szIniFilename);
            DBFreeVariant(&dbv);
        }
        sprintf(szTemp, "Font%dCol", i);
        WritePrivateProfileStringA(szAppname, "Color", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, szTemp, 0), szBuf, 10), szIniFilename);
        sprintf(szTemp, "Font%dSty", i);
        WritePrivateProfileStringA(szAppname, "Style", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, szTemp, 0), szBuf, 10), szIniFilename);
        sprintf(szTemp, "Font%dSize", i);
        WritePrivateProfileStringA(szAppname, "Size", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, szTemp, 0), szBuf, 10), szIniFilename);
        sprintf(szTemp, "Font%dSet", i);
        WritePrivateProfileStringA(szAppname, "Set", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, szTemp, 0), szBuf, 10), szIniFilename);
        sprintf(szTemp, "Font%dAs", i);
        WritePrivateProfileStringA(szAppname, "Inherit", _itoa(DBGetContactSettingWord(NULL, SRMSGMOD_T, szTemp, 0), szBuf, 10), szIniFilename);
    }
    WritePrivateProfileStringA("Message Log", "BackgroundColor", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "IncomingBG", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "inbg", RGB(224, 224, 224)), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "OutgoingBG", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "outbg", RGB(224, 224, 224)), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "InputBG", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "inputbg", RGB(224, 224, 224)), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "HgridColor", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "hgrid", RGB(224, 224, 224)), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "DWFlags", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "VGrid", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", 0), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "ExtraMicroLF", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", 0), szBuf, 10), szIniFilename);

    WritePrivateProfileStringA("Message Log", "ArrowIcons", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, "in_out_icons", 0), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "LeftIndent", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 0), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "RightIndent", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 0), szBuf, 10), szIniFilename);

    for(i = 0; i <= TMPL_STATUSCHG; i++) {
#if defined(_UNICODE)
        char *encoded = Utf8_Encode(LTR_Active.szTemplates[i]);
        WritePrivateProfileStringA("Templates", TemplateNames[i], encoded, szIniFilename);
        free(encoded);
        encoded = Utf8_Encode(RTL_Active.szTemplates[i]);
        WritePrivateProfileStringA("RTLTemplates", TemplateNames[i], encoded, szIniFilename);
        free(encoded);
#else
        WritePrivateProfileStringA("Templates", TemplateNames[i], LTR_Active.szTemplates[i], szIniFilename);
        WritePrivateProfileStringA("RTLTemplates", TemplateNames[i], RTL_Active.szTemplates[i], szIniFilename);
#endif        
    }

    for(i = 0; i < CUSTOM_COLORS; i++) {
        sprintf(szTemp, "cc%d", i + 1);
        WritePrivateProfileStringA("Custom Colors", szTemp, _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, szTemp, 0), szBuf, 10), szIniFilename);
    }
}

void ReadThemeFromINI(const char *szIniFilename)
{
    char szBuf[512], szTemp[100], szAppname[100];
    int i;
    int version;
    char szTemplateBuffer[TEMPLATE_LENGTH * 3 + 2];
    
    if((version = GetPrivateProfileIntA("TabSRMM Theme", "Version", 0, szIniFilename)) == 0)         // no version number.. assume 1
        version = 1;
    
    for(i = 0; i < MSGDLGFONTCOUNT; i++) {
        sprintf(szTemp, "Font%d", i);
        strcpy(szAppname, szTemp);
        if(GetPrivateProfileStringA(szAppname, "Face", "Arial", szBuf, sizeof(szBuf), szIniFilename) != 0)
            DBWriteContactSettingString(NULL, SRMSGMOD_T, szTemp, szBuf);
        
        sprintf(szTemp, "Font%dCol", i);
        DBWriteContactSettingDword(NULL, SRMSGMOD_T, szTemp, GetPrivateProfileIntA(szAppname, "Color", RGB(224, 224, 224), szIniFilename));
        
        sprintf(szTemp, "Font%dSty", i);
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, szTemp, GetPrivateProfileIntA(szAppname, "Style", 0, szIniFilename));

        sprintf(szTemp, "Font%dSize", i);
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, szTemp, GetPrivateProfileIntA(szAppname, "Size", 0, szIniFilename));

        sprintf(szTemp, "Font%dSet", i);
        DBWriteContactSettingByte(NULL, SRMSGMOD_T, szTemp, GetPrivateProfileIntA(szAppname, "Set", 0, szIniFilename));
        
        sprintf(szTemp, "Font%dAs", i);
        DBWriteContactSettingWord(NULL, SRMSGMOD_T, szTemp, GetPrivateProfileIntA(szAppname, "Inherit", 0, szIniFilename));
    }
    DBWriteContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, GetPrivateProfileIntA("Message Log", "BackgroundColor", RGB(224, 224, 224), szIniFilename));
    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "inbg", GetPrivateProfileIntA("Message Log", "IncomingBG", RGB(224, 224, 224), szIniFilename));
    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "outbg", GetPrivateProfileIntA("Message Log", "OutgoingBG", RGB(224, 224, 224), szIniFilename));
    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "inputbg", GetPrivateProfileIntA("Message Log", "InputBG", RGB(224, 224, 224), szIniFilename));
    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "hgrid", GetPrivateProfileIntA("Message Log", "HgridColor", RGB(224, 224, 224), szIniFilename));
    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", GetPrivateProfileIntA("Message Log", "DWFlags", MWF_LOG_DEFAULT, szIniFilename));
    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", GetPrivateProfileIntA("Message Log", "VGrid", 0, szIniFilename));
    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", GetPrivateProfileIntA("Message Log", "ExtraMicroLF", 0, szIniFilename));

    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "in_out_icons", GetPrivateProfileIntA("Message Log", "ArrowIcons", 0, szIniFilename));
    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", GetPrivateProfileIntA("Message Log", "LeftIndent", 0, szIniFilename));
    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", GetPrivateProfileIntA("Message Log", "RightIndent", 0, szIniFilename));

    if(version >= 3) {
        for(i = 0; i <= TMPL_STATUSCHG; i++) {
#if defined(_UNICODE)
            wchar_t *decoded;
            GetPrivateProfileStringA("Templates", TemplateNames[i], "", szTemplateBuffer, TEMPLATE_LENGTH * 3, szIniFilename);
            DBWriteContactSettingString(NULL, TEMPLATES_MODULE, TemplateNames[i], szTemplateBuffer);
            decoded = Utf8_Decode(szTemplateBuffer);
            mir_snprintfW(LTR_Active.szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
            free(decoded);
            GetPrivateProfileStringA("RTLTemplates", TemplateNames[i], "", szTemplateBuffer, TEMPLATE_LENGTH * 3, szIniFilename);
            DBWriteContactSettingString(NULL, RTLTEMPLATES_MODULE, TemplateNames[i], szTemplateBuffer);
            decoded = Utf8_Decode(szTemplateBuffer);
            mir_snprintfW(RTL_Active.szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
            free(decoded);
#else
            GetPrivateProfileStringA("Templates", TemplateNames[i], "", LTR_Active.szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
            GetPrivateProfileStringA("RTLTemplates", TemplateNames[i], "", RTL_Active.szTemplates[i], TEMPLATE_LENGTH - 1, szIniFilename);
#endif        
        }
        for(i = 0; i < CUSTOM_COLORS; i++) {
            sprintf(szTemp, "cc%d", i + 1);
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, szTemp, GetPrivateProfileIntA("Custom Colors", szTemp, RGB(224, 224, 224), szIniFilename));
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

    strncpy(szFilter, "tabSRMM themes_*.tabsrmm", MAX_PATH);
    szFilter[14] = '\0';
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

