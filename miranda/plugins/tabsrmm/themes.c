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
*/

#include "commonheaders.h"
#pragma hdrstop

#include "msgs.h"

/*
 * writes the current "theme" to .ini file
 * a theme contains all the fonts, colors and message log formatting
 * options.
 */
 
void WriteThemeToINI(const char *szIniFilename)
{
    int i;
    DBVARIANT dbv;
    char szBuf[100], szTemp[100], szAppname[100];
    
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
    WritePrivateProfileStringA("Message Log", "FollowUpTS", _itoa(DBGetContactSettingByte(NULL, SRMSGMOD_T, "followupts", 0), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "LeftIndent", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 0), szBuf, 10), szIniFilename);
    WritePrivateProfileStringA("Message Log", "RightIndent", _itoa(DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 0), szBuf, 10), szIniFilename);
}

void ReadThemeFromINI(const char *szIniFilename)
{
    char szBuf[512], szTemp[100], szAppname[100];
    int i;
    
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
    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "followupts", GetPrivateProfileIntA("Message Log", "FollowUpTS", 0, szIniFilename));
    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", GetPrivateProfileIntA("Message Log", "LeftIndent", 0, szIniFilename));
    DBWriteContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", GetPrivateProfileIntA("Message Log", "RightIndent", 0, szIniFilename));
}

char *GetThemeFileName()
{
    static char szFilename[MAX_PATH];
    OPENFILENAMEA ofn={0};
    char szFilter[MAX_PATH];

    strncpy(szFilter, "tabSRMM themes_*.tabsrmm", MAX_PATH);
    szFilter[14] = '\0';
    ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner=0;
    ofn.lpstrFile = szFilename;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrInitialDir = ".";
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "tabsrmm";
    if (GetOpenFileNameA(&ofn))
        return szFilename;
    else
        return NULL;
}

