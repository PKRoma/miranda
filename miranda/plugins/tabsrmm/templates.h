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
GNU General Public License for more details .

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

$Id$

/*
 * templates for the message log...
 */

#define TMPL_MSGIN 0
#define TMPL_MSGOUT 1
#define TMPL_GRPSTARTIN 2
#define TMPL_GRPSTARTOUT 3
#define TMPL_GRPINNERIN 4
#define TMPL_GRPINNEROUT 5
#define TMPL_STATUSCHG 6
#define TMPL_ERRMSG 7

#define TEMPLATE_LENGTH 150

#define CUSTOM_COLORS 5

#define TEMPLATES_MODULE "tabSRMM_Templates"
#define RTLTEMPLATES_MODULE "tabSRMM_RTLTemplates"

typedef struct _tagTemplateSet {
    BOOL valid;             // all templates populated (may still contain crap.. so it's only half-assed safety :)
    TCHAR szTemplates[TMPL_ERRMSG + 1][TEMPLATE_LENGTH];      // the template strings
    char szSetName[20];     // everything in this world needs a name. so does this poor template set.
} TemplateSet;

typedef struct _tagTemplateEditorInfo {
    BOOL rtl;
    BOOL changed;           // template in edit field is changed
    BOOL selchanging;
    int  inEdit;            // template currently in editor
    BOOL updateInfo[TMPL_ERRMSG + 1];        // item states...
    HWND hwndParent;
    HANDLE hContact;
} TemplateEditorInfo;

typedef struct _tagTemplateEditorNew {
    HANDLE hContact;
    BOOL   rtl;
    HWND   hwndParent;
} TemplateEditorNew;

BOOL CALLBACK DlgProcTemplateEdit(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void LoadTemplatesFrom(TemplateSet *tSet, HANDLE hContact, int rtl);
void LoadDefaultTemplates();

#define DM_UPDATETEMPLATEPREVIEW (WM_USER + 50)

