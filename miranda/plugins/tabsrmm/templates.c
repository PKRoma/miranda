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

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"
#include "m_popup.h"
#include "../../include/m_database.h"
#include "nen.h"
#include "functions.h"
#include "msgdlgutils.h"

/*
 * hardcoded default set of templates for both LTR and RTL.
 * cannot be changed and may be used at any time to "revert" to a working layout
 */

char *TemplateNames[] = {
    "Message In",
    "Message Out",
    "Group In (Start)",
    "Group Out (Start)",
    "Group In (Inner)",
    "Group Out (Inner)",
    "Status change"
};
    
TemplateSet LTR_Default = { TRUE, 
    _T("%I%S %N, %E, %h:%m:%s: %M"),
    _T("%I%S %N, %E, %h:%m:%s: %M"),
    _T("%I%S %N, %E, %h:%m:%s: %M"),
    _T("%I%S %N, %E, %h:%m:%s: %M"),
    _T("%S %h:%m:%s: %M"),
    _T("%S %h:%m:%s: %M"),
    _T("%I%S %D, %h:%m:%s, %N %M"),
    "Default LTR"
};

TemplateSet RTL_Default = { TRUE, 
    _T("%I%S %N, %E, %h:%m:%s: %M"),
    _T("%I%S %N, %E, %h:%m:%s: %M"),
    _T("%I%S %N, %E, %h:%m:%s: %M"),
    _T("%I%S %N, %E, %h:%m:%s: %M"),
    _T("%S %h:%m:%s: %M"),
    _T("%S %h:%m:%s: %M"),
    _T("%I%S %D, %h:%m:%s, %N %M"),
    "Default RTL"
};

TemplateSet LTR_Active, RTL_Active;

extern struct CREOleCallback reOleCallback;

/*
 * loads template set overrides from hContact into the given set of already existing
 * templates
 */

void LoadTemplatesFrom(TemplateSet *tSet, HANDLE hContact, int rtl)
{
    DBVARIANT dbv = {0};
    int i;

    for(i = 0; i <= TMPL_STATUSCHG; i++) {
        if(DBGetContactSetting(hContact, rtl ? RTLTEMPLATES_MODULE : TEMPLATES_MODULE, TemplateNames[i], &dbv))
            continue;
        if(dbv.type == DBVT_ASCIIZ) {
#if defined(_UNICODE)
            wchar_t *decoded = Utf8_Decode(dbv.pszVal);
            mir_snprintfW(tSet->szTemplates[i], TEMPLATE_LENGTH, L"%s", decoded);
            free(decoded);
#else
            mir_snprintf(tSet->szTemplates[i], TEMPLATE_LENGTH, "%s", dbv.pszVal);
#endif            
        }
        DBFreeVariant(&dbv);
    }
}

void LoadDefaultTemplates() 
{
    LTR_Active = LTR_Default;
    RTL_Active = RTL_Default;

    LoadTemplatesFrom(&LTR_Active, (HANDLE)0, 0);
    LoadTemplatesFrom(&RTL_Active, (HANDLE)0, 1);
}

BOOL CALLBACK DlgProcTemplateEditor(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct MessageWindowData *dat = 0;
    TemplateEditorInfo *teInfo = 0;
    TemplateSet *tSet;
    int i;
    dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
    /*
     * since this dialog needs a struct MessageWindowData * but has no container, we can store
     * the extended info struct in pContainer *)
     */
    if(dat) {
        teInfo = (TemplateEditorInfo *)dat->pContainer;
        tSet = teInfo->rtl ? dat->rtl_templates : dat->ltr_templates;
    }
    
    switch (msg) {
        case WM_INITDIALOG:
        {
            TemplateEditorNew *teNew = (TemplateEditorNew *)lParam;
            dat = (struct MessageWindowData *) malloc(sizeof(struct MessageWindowData));

            TranslateDialogDefault(hwndDlg);
            
            ZeroMemory((void *) dat, sizeof(struct MessageWindowData));
            dat->pContainer = (struct ContainerWindowData *)malloc(sizeof(TemplateEditorInfo));
            teInfo = (TemplateEditorInfo *)dat->pContainer;
            ZeroMemory((void *)teInfo, sizeof(TemplateEditorInfo));
            teInfo->hContact = teNew->hContact;
            teInfo->rtl = teNew->rtl;
            teInfo->hwndParent = teNew->hwndParent;
            
            dat->ltr_templates = &LTR_Active;
            dat->rtl_templates = &RTL_Active;

            /*
             * set hContact to the first found contact so that we can use the Preview window properly
             * also, set other parameters needed by the streaming function to display events
             */
            
            SendDlgItemMessage(hwndDlg, IDC_PREVIEW, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback);
            SendDlgItemMessage(hwndDlg, IDC_PREVIEW, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK);
            SendDlgItemMessage(hwndDlg, IDC_PREVIEW, EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR);
            SendDlgItemMessage(hwndDlg, IDC_PREVIEW, EM_EXLIMITTEXT, 0, 0x80000000);

            dat->hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
            dat->dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
            dat->szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)dat->hContact, 0);
            mir_snprintf(dat->szNickname, 80, "%s", (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0));
            GetContactUIN(hwndDlg, dat);
            
            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) dat);
            ShowWindow(hwndDlg, SW_SHOW);
            SendDlgItemMessage(hwndDlg, IDC_EDITTEMPLATE, EM_LIMITTEXT, (WPARAM)TEMPLATE_LENGTH - 1, 0);
            SetWindowTextA(hwndDlg, Translate("Template Set Editor"));
            EnableWindow(GetDlgItem(hwndDlg, IDC_SAVETEMPLATE), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_REVERT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_FORGET), FALSE);
            for(i = 0; i <= TMPL_STATUSCHG; i++) {
                SendDlgItemMessageA(hwndDlg, IDC_TEMPLATELIST, LB_ADDSTRING, 0, (LPARAM)Translate(TemplateNames[i]));
                SendDlgItemMessage(hwndDlg, IDC_TEMPLATELIST, LB_SETITEMDATA, i, (LPARAM)i);
            }
            EnableWindow(GetDlgItem(teInfo->hwndParent, IDC_MODIFY), FALSE);
            EnableWindow(GetDlgItem(teInfo->hwndParent, IDC_RTLMODIFY), FALSE);

            SendDlgItemMessage(hwndDlg, IDC_COLOR1, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc1", SRMSGDEFSET_BKGCOLOUR));
            SendDlgItemMessage(hwndDlg, IDC_COLOR2, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc2", SRMSGDEFSET_BKGCOLOUR));
            SendDlgItemMessage(hwndDlg, IDC_COLOR3, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc3", SRMSGDEFSET_BKGCOLOUR));
            SendDlgItemMessage(hwndDlg, IDC_COLOR4, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc4", SRMSGDEFSET_BKGCOLOUR));
            SendDlgItemMessage(hwndDlg, IDC_COLOR5, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc5", SRMSGDEFSET_BKGCOLOUR));
            
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    DestroyWindow(hwndDlg);
                    break;
                case IDC_TEMPLATELIST:
                    switch(HIWORD(wParam)) {
                        case LBN_DBLCLK:
                        {
                            LRESULT iIndex = SendDlgItemMessage(hwndDlg, IDC_TEMPLATELIST, LB_GETCURSEL, 0, 0);
                            if(iIndex != LB_ERR) {
                                SetDlgItemText(hwndDlg, IDC_EDITTEMPLATE, tSet->szTemplates[iIndex]);
                                teInfo->inEdit = iIndex;
                                teInfo->changed = FALSE;
                                teInfo->selchanging = FALSE;
                                SetFocus(GetDlgItem(hwndDlg, IDC_EDITTEMPLATE));
                                //_DebugPopup(0, "editing: %d", teInfo->inEdit);
                            }
                            break;
                        }
                        case LBN_SELCHANGE:
                            teInfo->selchanging = TRUE;
                            break;
                    }
                    break;
                case IDC_EDITTEMPLATE:
                    if(HIWORD(wParam) == EN_CHANGE) {
                        if(!teInfo->selchanging) {
                            teInfo->changed = TRUE;
                            teInfo->updateInfo[teInfo->inEdit] = TRUE;
                            EnableWindow(GetDlgItem(hwndDlg, IDC_SAVETEMPLATE), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_FORGET), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_TEMPLATELIST), FALSE);
                            //_DebugPopup(0, "changed for: %d", teInfo->inEdit);
                        }
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_TEMPLATELIST), NULL, FALSE);
                    }
                    break;
                case IDC_SAVETEMPLATE:
                {
                    TCHAR newTemplate[TEMPLATE_LENGTH];

                    GetWindowText(GetDlgItem(hwndDlg, IDC_EDITTEMPLATE), newTemplate, TEMPLATE_LENGTH);
                    CopyMemory(tSet->szTemplates[teInfo->inEdit], newTemplate, sizeof(TCHAR) * TEMPLATE_LENGTH);
                    teInfo->changed = FALSE;
                    teInfo->updateInfo[teInfo->inEdit] = FALSE;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_TEMPLATELIST), NULL, FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SAVETEMPLATE), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_FORGET), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TEMPLATELIST), TRUE);
#if defined(_UNICODE)
                    {
                        char *encoded = Utf8_Encode(newTemplate);
                        DBWriteContactSettingString(teInfo->hContact, teInfo->rtl ? RTLTEMPLATES_MODULE : TEMPLATES_MODULE, TemplateNames[teInfo->inEdit], encoded);
                        free(encoded);
                    }
#else
                    DBWriteContactSettingString(teInfo->hContact, teInfo->rtl ? RTLTEMPLATES_MODULE : TEMPLATES_MODULE, TemplateNames[teInfo->inEdit], newTemplate);
#endif
                    break;
                }
                case IDC_FORGET:
                {
                    teInfo->changed = FALSE;
                    teInfo->updateInfo[teInfo->inEdit] = FALSE;
                    teInfo->selchanging = TRUE;
                    SetDlgItemText(hwndDlg, IDC_EDITTEMPLATE, tSet->szTemplates[teInfo->inEdit]);
                    SetFocus(GetDlgItem(hwndDlg, IDC_EDITTEMPLATE));
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_TEMPLATELIST), NULL, FALSE);
                    teInfo->selchanging = FALSE;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SAVETEMPLATE), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_FORGET), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TEMPLATELIST), TRUE);
                    break;
                }
                case IDC_REVERT:
                {
                    teInfo->changed = FALSE;
                    teInfo->updateInfo[teInfo->inEdit] = FALSE;
                    teInfo->selchanging = TRUE;
                    CopyMemory(tSet->szTemplates[teInfo->inEdit], LTR_Default.szTemplates[teInfo->inEdit], sizeof(TCHAR) * TEMPLATE_LENGTH);
                    SetDlgItemText(hwndDlg, IDC_EDITTEMPLATE, tSet->szTemplates[teInfo->inEdit]);
                    DBDeleteContactSetting(teInfo->hContact, teInfo->rtl ? RTLTEMPLATES_MODULE : TEMPLATES_MODULE, TemplateNames[teInfo->inEdit]);
                    SetFocus(GetDlgItem(hwndDlg, IDC_EDITTEMPLATE));
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_TEMPLATELIST), NULL, FALSE);
                    teInfo->selchanging = FALSE;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SAVETEMPLATE), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_REVERT), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_FORGET), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TEMPLATELIST), TRUE);
                    break;
                }
                case IDC_UPDATEPREVIEW:
                    SendMessage(hwndDlg, DM_UPDATETEMPLATEPREVIEW, 0, 0);
                    break;
            }
            break;
        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *) lParam;
            int iItem = dis->itemData;
            HBRUSH bkg, oldBkg;
            SetBkMode(dis->hDC, TRANSPARENT);
            FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_WINDOW));
            if (dis->itemState & ODS_SELECTED) {
                if(teInfo->updateInfo[iItem] == TRUE) {
                    bkg = CreateSolidBrush(RGB(255, 0, 0));
                    oldBkg = SelectObject(dis->hDC, bkg);
                    FillRect(dis->hDC, &dis->rcItem, bkg);
                    SelectObject(dis->hDC, oldBkg);
                    DeleteObject(bkg);
                }
                else
                    FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
                
                SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            }
            else {
                if(teInfo->updateInfo[iItem] == TRUE)
                    SetTextColor(dis->hDC, RGB(255, 0, 0));
                else
                    SetTextColor(dis->hDC, GetSysColor(COLOR_WINDOWTEXT));
            }
            TextOutA(dis->hDC, dis->rcItem.left, dis->rcItem.top, Translate(TemplateNames[iItem]), lstrlenA(Translate(TemplateNames[iItem])));
            return TRUE;
        }
        case DM_UPDATETEMPLATEPREVIEW:
        {
            DBEVENTINFO dbei = {0};

            dbei.szModule = dat->szProto;
            dbei.timestamp = time(NULL);
            dbei.eventType = EVENTTYPE_MESSAGE;
            dbei.cbSize = sizeof(dbei);
            dbei.pBlob = (BYTE *)"The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog.";
            dbei.cbBlob = lstrlenA(dbei.pBlob) + 1;
            dbei.flags = 0;
            SetWindowText(GetDlgItem(hwndDlg, IDC_PREVIEW), _T(""));
            StreamInEvents(hwndDlg, 0, 1, 1, &dbei);
            
            break;
        }
        case WM_DESTROY:
            if(dat->pContainer)
                free(dat->pContainer);
            if(dat)
                free(dat);

            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "cc1", SendDlgItemMessage(hwndDlg, IDC_COLOR1, CPM_GETCOLOUR, 0, 0));
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "cc2", SendDlgItemMessage(hwndDlg, IDC_COLOR2, CPM_GETCOLOUR, 0, 0));
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "cc3", SendDlgItemMessage(hwndDlg, IDC_COLOR3, CPM_GETCOLOUR, 0, 0));
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "cc4", SendDlgItemMessage(hwndDlg, IDC_COLOR4, CPM_GETCOLOUR, 0, 0));
            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "cc5", SendDlgItemMessage(hwndDlg, IDC_COLOR5, CPM_GETCOLOUR, 0, 0));

            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            EnableWindow(GetDlgItem(teInfo->hwndParent, IDC_MODIFY), TRUE);
            EnableWindow(GetDlgItem(teInfo->hwndParent, IDC_RTLMODIFY), TRUE);
            break;
    }
    return FALSE;
}

