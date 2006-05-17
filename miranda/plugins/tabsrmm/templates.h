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
static void LoadTemplatesFrom(TemplateSet *tSet, HANDLE hContact, int rtl);
void LoadDefaultTemplates();

#define DM_UPDATETEMPLATEPREVIEW (WM_USER + 50)

