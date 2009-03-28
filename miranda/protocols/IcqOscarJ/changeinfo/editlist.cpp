// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2001-2004 Richard Hughes, Martin �berg
// Copyright � 2004-2009 Joe Kucera, Bio
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


static ChangeInfoData *dataListEdit = NULL;
static HWND hwndListEdit = NULL;
static BOOL (WINAPI *MyAnimateWindow)(HWND,DWORD,DWORD);
static WNDPROC OldListEditProc;

static LRESULT CALLBACK ListEditSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  switch(msg) 
  {
    case WM_LBUTTONUP:
      CallWindowProc(OldListEditProc, hwnd, msg, wParam, lParam);
      {  
        POINT pt;

        pt.x = (short)LOWORD(lParam);
        pt.y = (short)HIWORD(lParam);
        ClientToScreen(hwnd, &pt);
        if (SendMessage(hwnd, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y)) == HTVSCROLL) break;
      }
      {  
        int i = SendMessage(hwnd, LB_GETCURSEL, 0, 0);

        if (dataListEdit)
          dataListEdit->EndListEdit(i != LB_ERR);
      }
      return 0;

    case WM_CHAR:
      if (wParam != '\r') break;
      {  
        int i = SendMessage(hwnd, LB_GETCURSEL, 0, 0);

        if (dataListEdit)
          dataListEdit->EndListEdit(i != LB_ERR);
      }
      return 0;
    case WM_KILLFOCUS:
      if (dataListEdit)
        dataListEdit->EndListEdit(1);
      return 0;
  }
  return CallWindowProc(OldListEditProc, hwnd, msg, wParam, lParam);
}


void ChangeInfoData::BeginListEdit(int iItem, RECT *rc, int iSetting, WORD wVKey)
{
  int j,n;
  POINT pt;
  int itemHeight;
  char str[MAX_PATH];

  if (dataListEdit)
    dataListEdit->EndListEdit(0);

  pt.x=pt.y=0;
  ClientToScreen(hwndList,&pt);
  OffsetRect(rc,pt.x,pt.y);
  InflateRect(rc,-2,-2);
  rc->left-=2;
  iEditItem = iItem;
  ListView_RedrawItems(hwndList, iEditItem, iEditItem);
  UpdateWindow(hwndList);

  dataListEdit = this;
  hwndListEdit = CreateWindowEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST, _T("LISTBOX"), _T(""), WS_POPUP|WS_BORDER|WS_VSCROLL, rc->left, rc->bottom, rc->right - rc->left, 150, NULL, NULL, hInst, NULL);
  SendMessage(hwndListEdit, WM_SETFONT, (WPARAM)hListFont, 0);
  itemHeight = SendMessage(hwndListEdit, LB_GETITEMHEIGHT, 0, 0);

  FieldNamesItem *list = (FieldNamesItem*)setting[iSetting].pList;

  if (list == countryField)
  { // some country codes were changed leaving old details uknown, convert it for the user
    if (settingData[iSetting].value == 420) settingData[iSetting].value = 42; // conversion of obsolete codes (OMG!)
    else if (settingData[iSetting].value == 421) settingData[iSetting].value = 4201;
    else if (settingData[iSetting].value == 102) settingData[iSetting].value = 1201;
  }

  n = ListBoxAddStringUtf(hwndListEdit, "Unspecified");
  for (j=0; ; j++)
    if (!list[j].text)
    {
      SendMessage(hwndListEdit, LB_SETITEMDATA, n, j);
      if ((settingData[iSetting].value == 0 && list[j].code == 0)
       || (setting[iSetting].dbType != DBVT_ASCIIZ && settingData[iSetting].value == list[j].code))
        SendMessage(hwndListEdit, LB_SETCURSEL, n, 0);
      break;
    }

  for (j=0; list[j].text; j++) 
  {
    n = ListBoxAddStringUtf(hwndListEdit, list[j].text);
    SendMessage(hwndListEdit, LB_SETITEMDATA, n, j);
    if ((setting[iSetting].dbType == DBVT_ASCIIZ && (!strcmpnull((char*)settingData[iSetting].value, list[j].text))
     || (setting[iSetting].dbType == DBVT_ASCIIZ && (!strcmpnull((char*)settingData[iSetting].value, ICQTranslateUtfStatic(list[j].text, str, MAX_PATH))))
     || ((char*)settingData[iSetting].value == NULL && list[j].code == 0))
     || (setting[iSetting].dbType != DBVT_ASCIIZ && settingData[iSetting].value == list[j].code))
      SendMessage(hwndListEdit, LB_SETCURSEL, n, 0);
  }
  SendMessage(hwndListEdit, LB_SETTOPINDEX, SendMessage(hwndListEdit, LB_GETCURSEL, 0, 0) - 3, 0);
  int listCount = SendMessage(hwndListEdit, LB_GETCOUNT, 0, 0);
  if (itemHeight * listCount < 150)
    SetWindowPos(hwndListEdit, 0, 0, 0, rc->right - rc->left, itemHeight * listCount + GetSystemMetrics(SM_CYBORDER) * 2, SWP_NOZORDER|SWP_NOMOVE);
  OldListEditProc = (WNDPROC)SetWindowLongPtr(hwndListEdit, GWLP_WNDPROC, (LONG_PTR)ListEditSubclassProc);
  if (MyAnimateWindow = (BOOL (WINAPI*)(HWND,DWORD,DWORD))GetProcAddress(GetModuleHandleA("user32"), "AnimateWindow")) 
  {
    BOOL enabled;

    SystemParametersInfo(SPI_GETCOMBOBOXANIMATION, 0, &enabled, FALSE);
    if (enabled) MyAnimateWindow(hwndListEdit, 200, AW_SLIDE|AW_ACTIVATE|AW_VER_POSITIVE);
  }
  ShowWindow(hwndListEdit, SW_SHOW);
  SetFocus(hwndListEdit);
  if (wVKey)
    PostMessage(hwndListEdit, WM_KEYDOWN, wVKey, 0);
}


void ChangeInfoData::EndListEdit(int save)
{
  if (hwndListEdit == NULL || iEditItem == -1 || this != dataListEdit) return;
  if (save) 
  {
    int iItem = SendMessage(hwndListEdit, LB_GETCURSEL, 0, 0);
    int i = SendMessage(hwndListEdit, LB_GETITEMDATA, iItem, 0);

    if (setting[iEditItem].dbType == DBVT_ASCIIZ) 
    {
      char *szNewValue = (((FieldNamesItem*)setting[iEditItem].pList)[i].text);
      if (((FieldNamesItem*)setting[iEditItem].pList)[i].code || setting[iEditItem].displayType & LIF_ZEROISVALID)
      { 
        settingData[iEditItem].changed = strcmpnull(szNewValue, (char*)settingData[iEditItem].value);
        SAFE_FREE((void**)&settingData[iEditItem].value);
        settingData[iEditItem].value = (LPARAM)null_strdup(szNewValue);
      }
      else 
      {
        settingData[iEditItem].changed = (char*)settingData[iEditItem].value!=NULL;
        SAFE_FREE((void**)&settingData[iEditItem].value);
      }
    }
    else 
    {
      settingData[iEditItem].changed = ((FieldNamesItem*)setting[iEditItem].pList)[i].code != settingData[iEditItem].value;
      settingData[iEditItem].value = ((FieldNamesItem*)setting[iEditItem].pList)[i].code;
    }
    if (settingData[iEditItem].changed) EnableDlgItem(GetParent(hwndList), IDC_SAVE, TRUE);
  }
  ListView_RedrawItems(hwndList, iEditItem, iEditItem);
  iEditItem = -1;
  dataListEdit = NULL;
  DestroyWindow(hwndListEdit);
  hwndListEdit = NULL;
}


int IsListEditWindow(HWND hwnd)
{
  if (hwnd == hwndListEdit) return 1;
  return 0;
}
