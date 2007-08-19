// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2001,2002,2003,2004 Richard Hughes, Martin Öberg
// Copyright © 2004,2005,2006,2007 Joe Kucera, Bio
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  ChangeInfo Plugin stuff
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


static int editTopIndex;
static HANDLE hAckHook = NULL;
static HFONT hMyFont = NULL;

#define DM_PROTOACK  (WM_USER+10)

static int DrawTextUtf(HDC hDC, char* text, LPRECT lpRect, UINT uFormat, LPSIZE lpSize)
{
  int res;

  if (gbUnicodeAPI)
  {
    wchar_t *tmp = make_unicode_string(text);

    res = DrawTextW(hDC, tmp, -1, lpRect, uFormat);
    if (lpSize)
      GetTextExtentPoint32W(hDC, tmp, wcslen(tmp), lpSize);
    SAFE_FREE(&tmp);
  }
  else
  {
    // caution, here we change text's contents
    utf8_decode_static(text, text, strlennull(text)+1); 
    res = DrawTextA(hDC, text, -1, lpRect, uFormat);
    if (lpSize)
      GetTextExtentPoint32A(hDC, text, strlennull(text), lpSize);
  }
  return res;
}



static void PaintItemSetting(HDC hdc,RECT *rc,int i,UINT itemState)
{
  char *text;
  int alloced=0;
  char str[MAX_PATH];

  if (setting[i].value==0 && !(setting[i].displayType&LIF_ZEROISVALID))
  {
    SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));

    if (setting[i].displayType & LIF_CHANGEONLY)
      text = ICQTranslateUtfStatic("<unremovable once applied>", str, MAX_PATH);
    else
      text = ICQTranslateUtfStatic("<empty>", str, MAX_PATH);
  }
  else 
  {
    switch (setting[i].displayType & LIM_TYPE)
    {
      case LI_STRING:
      case LI_LONGSTRING:
      {
        text = BinaryToEscapes((char*)setting[i].value);
        alloced = 1;
        break;
      }

      case LI_NUMBER:
        text = str;
        itoa(setting[i].value, text, 10);
        break;

      case LI_LIST:
        if (setting[i].dbType == DBVT_ASCIIZ) 
        {
          text = ICQTranslateUtfStatic((char*)setting[i].value, str, MAX_PATH);
        }
        else 
        {
          int j;

          text = ICQTranslateUtfStatic("Unknown value", str, MAX_PATH);

          for(j=0; j < setting[i].listCount; j++)
            if (((ListTypeDataItem*)setting[i].pList)[j].id == setting[i].value) 
            {
              text = ICQTranslateUtfStatic(((ListTypeDataItem*)setting[i].pList)[j].szValue, str, MAX_PATH);
              break;
            }
        }
        break;
    }
  }
  if (setting[i].displayType & LIF_PASSWORD) 
  {
    if (setting[i].changed) 
    {
      int i;
      for (i=0; text[i]; i++) text[i] = '*';
    }
    else 
    {
      if (alloced) 
      {
        SAFE_FREE(&text); 
        alloced=0;
      }
      text = "********";
    }
  }
  if ((setting[i].displayType & LIM_TYPE) == LI_LIST && (itemState & CDIS_SELECTED || iEditItem == i)) 
  {
    RECT rcBtn;

    rcBtn = *rc;
    rcBtn.left = rcBtn.right - rc->bottom + rc->top;
    InflateRect(&rcBtn,-2,-2);
    rc->right = rcBtn.left;
    DrawFrameControl(hdc, &rcBtn, DFC_SCROLL, iEditItem == i ? DFCS_SCROLLDOWN|DFCS_PUSHED : DFCS_SCROLLDOWN);
  }
  DrawTextUtf(hdc, text, rc, DT_END_ELLIPSIS|DT_LEFT|DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER, NULL);

  if (alloced) SAFE_FREE(&text);
}


static int InfoDlg_Resize(HWND hwndDlg,LPARAM lParam,UTILRESIZECONTROL *urc)
{
  switch (urc->wId)
  {
    case IDC_LIST:
      return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;
      break;
    case IDC_SAVE:
      return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;      
      break;
    case IDC_UPLOADING:
      return RD_ANCHORX_WIDTH | RD_ANCHORY_BOTTOM;      
      break;
  }
  return RD_ANCHORX_LEFT | RD_ANCHORY_TOP; // default
}



BOOL CALLBACK ChangeInfoDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg) 
  {
    case WM_INITDIALOG:
      ICQTranslateDialog(hwndDlg);
      hwndList=GetDlgItem(hwndDlg,IDC_LIST);
      LoadSettingsFromDb(0);
      ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT);
      iEditItem=-1;
      {
        LOGFONT lf;

        hListFont=(HFONT)SendMessage(hwndList,WM_GETFONT,0,0);
        GetObject(hListFont,sizeof(lf),&lf);
        lf.lfHeight-=5;
        hMyFont=CreateFontIndirect(&lf);
        SendMessage(hwndList,WM_SETFONT,(WPARAM)hMyFont,0);
      }
      {  
        LV_COLUMN lvc={0};
        RECT rc;

        GetClientRect(hwndList,&rc);
        rc.right-=GetSystemMetrics(SM_CXVSCROLL);
        lvc.mask = LVCF_WIDTH;
        lvc.cx = rc.right/3;
        ListView_InsertColumn(hwndList, 0, &lvc);
        lvc.cx = rc.right-lvc.cx;
        ListView_InsertColumn(hwndList, 1, &lvc);
      }
      {  
        LV_ITEM lvi={0};
        lvi.mask = LVIF_PARAM;

        for (lvi.iItem=0;lvi.iItem<settingCount;lvi.iItem++) 
        {
          lvi.lParam=lvi.iItem;
          ListView_InsertItem(hwndList, &lvi);
        }
      }
      {
        char *pwd = GetUserPassword(TRUE);
       
        if (pwd) 
          strcpy(Password, pwd);
        else
          strcpy(Password, "");
      }

      SendMessage(GetParent(hwndDlg),PSM_CHANGED,0,0);
      return TRUE;

    case WM_NOTIFY:
      switch (((LPNMHDR)lParam)->idFrom) 
      {
        case 0:
          switch (((LPNMHDR)lParam)->code) 
          {
            case PSN_INFOCHANGED:
              LoadSettingsFromDb(1);
              break;

            case PSN_KILLACTIVE:
              EndStringEdit(1);
              EndListEdit(1);
              break;

            case PSN_APPLY:
              if(ChangesMade()) 
              {
                if (IDYES!=MessageBoxUtf(hwndDlg, LPGEN("You've made some changes to your ICQ details but it has not been saved to the server. Are you sure you want to close this dialog?"), LPGEN("Change ICQ Details"), MB_YESNOCANCEL))
                {
                  SetWindowLongUtf(hwndDlg,DWL_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
                  return TRUE;
                }
              }
              PostMessage(hwndList,WM_SETFONT,(WPARAM)hListFont,0);
              break;
          }
          break;

        case IDC_LIST:
          switch (((LPNMHDR)lParam)->code) 
          {
            case LVN_GETDISPINFOW:
            case LVN_GETDISPINFO:
              if (iEditItem != -1) 
              {
                if (editTopIndex != ListView_GetTopIndex(hwndList)) 
                {
                  EndStringEdit(1);
                  EndListEdit(1);
                }
              }
              break;

            case NM_CUSTOMDRAW:
            {
              LPNMLVCUSTOMDRAW cd=(LPNMLVCUSTOMDRAW)lParam;

              switch(cd->nmcd.dwDrawStage)
              {
                case CDDS_PREPAINT:
                  SetWindowLongUtf(hwndDlg,DWL_MSGRESULT,CDRF_NOTIFYSUBITEMDRAW);
                  return TRUE;

                case CDDS_ITEMPREPAINT:
                {
                  RECT rc;

                  ListView_GetItemRect(hwndList, cd->nmcd.dwItemSpec, &rc, LVIR_BOUNDS);

                  if (GetWindowLong(hwndList, GWL_STYLE) & WS_DISABLED)
                  {  // Disabled List
                    SetTextColor(cd->nmcd.hdc, cd->clrText);
                    FillRect(cd->nmcd.hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
                  }
                  else if ((cd->nmcd.uItemState & CDIS_SELECTED || iEditItem == (int)cd->nmcd.dwItemSpec)
                    && setting[cd->nmcd.lItemlParam].displayType != LI_DIVIDER)
                  {  // Selected item
                    SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    FillRect(cd->nmcd.hdc, &rc, GetSysColorBrush(COLOR_HIGHLIGHT));
                  }
                  else
                  { // Unselected item
                    SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_WINDOWTEXT));
                    FillRect(cd->nmcd.hdc, &rc, GetSysColorBrush(COLOR_WINDOW));
                  }

                  if (setting[cd->nmcd.lItemlParam].displayType == LI_DIVIDER)
                  {
                    RECT rcLine;
                    SIZE textSize;
                    char str[MAX_PATH];
                    char *szText = ICQTranslateUtfStatic(setting[cd->nmcd.lItemlParam].szDescription, str, MAX_PATH);
                    HFONT hoFont;

                    hoFont = (HFONT)SelectObject(cd->nmcd.hdc, hListFont);
                    SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_3DSHADOW));
                    ListView_GetItemRect(hwndList, cd->nmcd.dwItemSpec, &rc, LVIR_BOUNDS);
                    DrawTextUtf(cd->nmcd.hdc, szText, &rc, DT_CENTER|DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER, &textSize);
                    rcLine.top = (rc.top + rc.bottom)/2-1;
                    rcLine.bottom = rcLine.top+2;
                    rcLine.left = rc.left + 3;
                    rcLine.right = (rc.left+rc.right-textSize.cx)/2-3;
                    DrawEdge(cd->nmcd.hdc, &rcLine, BDR_SUNKENOUTER, BF_RECT);
                    rcLine.left = (rc.left + rc.right + textSize.cx)/2 + 3;
                    rcLine.right = rc.right-3;
                    DrawEdge(cd->nmcd.hdc, &rcLine, BDR_SUNKENOUTER, BF_RECT);
                    SetWindowLongUtf(hwndDlg, DWL_MSGRESULT, CDRF_SKIPDEFAULT);
                  }
                  else
                  {
                    SetWindowLongUtf(hwndDlg, DWL_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW|CDRF_NOTIFYPOSTPAINT);
                  }

                  return TRUE;
                }

                case CDDS_SUBITEM|CDDS_ITEMPREPAINT:
                {  
                  RECT rc;
                  HFONT hoFont;

                  hoFont=(HFONT)SelectObject(cd->nmcd.hdc,hListFont);
                  ListView_GetSubItemRect(hwndList,cd->nmcd.dwItemSpec,cd->iSubItem,LVIR_BOUNDS,&rc);

                  if (cd->iSubItem==0)  
                  {
                    RECT rc2;
                    char str[MAX_PATH];

                    ListView_GetSubItemRect(hwndList,cd->nmcd.dwItemSpec,1,LVIR_BOUNDS,&rc2);
                    rc.right=rc2.left;
                    rc.left+=2;
                    DrawTextUtf(cd->nmcd.hdc, ICQTranslateUtfStatic(setting[cd->nmcd.lItemlParam].szDescription, str, MAX_PATH), &rc, DT_END_ELLIPSIS|DT_LEFT|DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER, NULL);
                  }
                  else 
                    PaintItemSetting(cd->nmcd.hdc, &rc, cd->nmcd.lItemlParam, cd->nmcd.uItemState);
                  SetWindowLongUtf(hwndDlg,DWL_MSGRESULT,CDRF_SKIPDEFAULT);

                  return TRUE;
                }
/*
                case CDDS_ITEMPOSTPAINT:
                {  RECT rc;
                  ListView_GetItemRect(hwndList,cd->nmcd.dwItemSpec,&rc,LVIR_BOUNDS);
                  if(cd->nmcd.uItemState&CDIS_FOCUS) {
                    HDC hdc2
                    hdc2=GetDC(hwndList);  //I don't know what the listview's done to its DC, but I can't figure out how to undo it
                    DrawFocusRect(hdc2,&rc);
                    ReleaseDC(hwndList,hdc2);
                  }
                  break;
                }
*/

              }
              break;
            }
            case NM_CLICK:
            {  
              LPNMLISTVIEW nm=(LPNMLISTVIEW)lParam;
              LV_ITEM lvi;
              RECT rc;

              EndStringEdit(1);
              EndListEdit(1);
              if (nm->iSubItem != 1) break;
              lvi.mask = LVIF_PARAM|LVIF_STATE;
              lvi.stateMask = 0xFFFFFFFF;
              lvi.iItem = nm->iItem; lvi.iSubItem = nm->iSubItem;
              ListView_GetItem(hwndList, &lvi);
              if (!(lvi.state & LVIS_SELECTED)) break;
              ListView_EnsureVisible(hwndList, lvi.iItem, FALSE);
              ListView_GetSubItemRect(hwndList, lvi.iItem, lvi.iSubItem, LVIR_BOUNDS, &rc);
              editTopIndex = ListView_GetTopIndex(hwndList);
              switch (setting[lvi.lParam].displayType & LIM_TYPE) 
              {
                case LI_STRING:
                case LI_LONGSTRING:
                case LI_NUMBER:
                  BeginStringEdit(nm->iItem, &rc, lvi. lParam, 0);
                  break;
                case LI_LIST:
                  BeginListEdit(nm->iItem, &rc, lvi. lParam, 0);
                  break;
              }
              break;
            }
            case LVN_KEYDOWN:
            {  
              LPNMLVKEYDOWN nm=(LPNMLVKEYDOWN)lParam;
              LV_ITEM lvi;
              RECT rc;

              EndStringEdit(1);
              EndListEdit(1);
              if(nm->wVKey==VK_SPACE || nm->wVKey==VK_RETURN || nm->wVKey==VK_F2) nm->wVKey=0;
              if(nm->wVKey && (nm->wVKey<'0' || (nm->wVKey>'9' && nm->wVKey<'A') || (nm->wVKey>'Z' && nm->wVKey<VK_NUMPAD0) || nm->wVKey>=VK_F1))
                break;
              lvi.mask=LVIF_PARAM|LVIF_STATE;
              lvi.stateMask=0xFFFFFFFF;
              lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_ALL|LVNI_SELECTED);
              if(lvi.iItem==-1) break;
              lvi.iSubItem=1;
              ListView_GetItem(hwndList,&lvi);
              ListView_EnsureVisible(hwndList,lvi.iItem,FALSE);
              ListView_GetSubItemRect(hwndList,lvi.iItem,lvi.iSubItem,LVIR_BOUNDS,&rc);
              editTopIndex=ListView_GetTopIndex(hwndList);
              switch(setting[lvi.lParam].displayType&LIM_TYPE) 
              {
                case LI_STRING:
                case LI_LONGSTRING:
                case LI_NUMBER:
                  BeginStringEdit(lvi.iItem,&rc,lvi.lParam,nm->wVKey);
                  break;
                case LI_LIST:
                  BeginListEdit(lvi.iItem,&rc,lvi.lParam,nm->wVKey);
                  break;
              }
              SetWindowLongUtf(hwndDlg,DWL_MSGRESULT,TRUE);
              return TRUE;
            }
            case NM_KILLFOCUS:
              if(!IsStringEditWindow(GetFocus())) EndStringEdit(1);
              if(!IsListEditWindow(GetFocus())) EndListEdit(1);
              break;
          }
          break;
      }
      break;
    case WM_KILLFOCUS:
      EndStringEdit(1);
      EndListEdit(1);
      break;
    case WM_COMMAND:
      switch(LOWORD(wParam)) 
      {
        case IDCANCEL:
          SendMessage(GetParent(hwndDlg), msg, wParam, lParam);
          break;

        case IDC_SAVE:
          if (!SaveSettingsToDb(hwndDlg)) break;
          EnableDlgItem(hwndDlg, IDC_SAVE, FALSE);
          EnableDlgItem(hwndDlg, IDC_LIST, FALSE);
          {
            char str[MAX_PATH];

            SetDlgItemTextUtf(hwndDlg, IDC_UPLOADING, ICQTranslateUtfStatic("Upload in progress...", str, MAX_PATH));
          }
          EnableDlgItem(hwndDlg, IDC_UPLOADING, TRUE);
          ShowWindow(GetDlgItem(hwndDlg, IDC_UPLOADING), SW_SHOW);
          hAckHook = HookEventMessage(ME_PROTO_ACK, hwndDlg, DM_PROTOACK);

          if (!UploadSettings(hwndDlg)) 
          {
            EnableDlgItem(hwndDlg, IDC_SAVE, TRUE);
            EnableDlgItem(hwndDlg, IDC_LIST, TRUE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_UPLOADING), SW_HIDE);
            UnhookEvent(hAckHook); 
            hAckHook = NULL;
          }
          break;
      }
      break;

    case WM_SIZE:
    { // make the dlg resizeable
      UTILRESIZEDIALOG urd = {0};

      if (IsIconic(hwndDlg)) break;
      urd.cbSize = sizeof(urd);
      urd.hInstance = hInst;
      urd.hwndDlg = hwndDlg;
      urd.lParam = 0; // user-defined
      urd.lpTemplate = MAKEINTRESOURCEA(IDD_INFO_CHANGEINFO);
      urd.pfnResizer = InfoDlg_Resize;
      CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM) &urd);

      { // update listview column widths
        RECT rc;

        GetClientRect(hwndList,&rc);
        rc.right-=GetSystemMetrics(SM_CXVSCROLL);
        ListView_SetColumnWidth(hwndList, 0, rc.right/3);
        ListView_SetColumnWidth(hwndList, 1, rc.right - rc.right/3);
      }
      break;
    }

    case DM_PROTOACK:
    {
      ACKDATA *ack=(ACKDATA*)lParam;
      int i,done;
      char str[MAX_PATH];

      if (ack->type != ACKTYPE_SETINFO) break;
      if (ack->result == ACKRESULT_SUCCESS)
      {
        for (i=0; i < SIZEOF(hUpload); i++)
          if (hUpload[i] && ack->hProcess == hUpload[i]) break;

        if (i == SIZEOF(hUpload)) break;
        hUpload[i] = NULL;
        for (done = 0, i = 0; i < SIZEOF(hUpload); i++)
          done += hUpload[i] == NULL;
        wsprintf(str,"%s%d%%", ICQTranslateUtfStatic("Upload in progress...", str, MAX_PATH), 100*done/(SIZEOF(hUpload)));
        SetDlgItemTextUtf(hwndDlg, IDC_UPLOADING, str);
        if (done < SIZEOF(hUpload)) break;

        ClearChangeFlags();
        UnhookEvent(hAckHook); hAckHook = NULL;
        EnableDlgItem(hwndDlg, IDC_LIST, TRUE);
        EnableDlgItem(hwndDlg, IDC_UPLOADING, FALSE);
        SetDlgItemTextUtf(hwndDlg, IDC_UPLOADING, ICQTranslateUtfStatic("Upload complete", str, MAX_PATH));
        SendMessage(GetParent(hwndDlg), PSM_FORCECHANGED, 0, 0);
      }
      else if (ack->result==ACKRESULT_FAILED)
      {
        UnhookEvent(hAckHook); 
        hAckHook = NULL;
        EnableDlgItem(hwndDlg, IDC_LIST, TRUE);
        EnableDlgItem(hwndDlg, IDC_UPLOADING, FALSE);
        SetDlgItemTextUtf(hwndDlg, IDC_UPLOADING, ICQTranslateUtfStatic("Upload FAILED", str, MAX_PATH));
        SendMessage(GetParent(hwndDlg), PSM_FORCECHANGED, 0, 0);
      }
      break;
    }
    case WM_DESTROY:
      if(hAckHook) 
      {
        UnhookEvent(hAckHook);
        hAckHook=NULL;
      }
      //SendMessage(hwndList,WM_GETFONT,0,0);
      DeleteObject(hMyFont);
      FreeStoredDbSettings();
      break;
  }
  return FALSE;
}
