// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005 Joe Kucera
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
// File name      : $Source$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Support for Custom Statuses
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"
#include "m_cluiframes.h"


extern void setUserInfo();

static HANDLE hHookExtraIconsRebuild = NULL;
static HANDLE hHookExtraIconsApply = NULL;
static HANDLE hXStatusIcons[24];
static HANDLE hXStatusItems[25];


static void setContactExtraIcon(HANDLE hContact, HANDLE hIcon)
{
	IconExtraColumn iec;

	iec.cbSize = sizeof(iec);
	iec.hImage = hIcon;
	iec.ColumnType = EXTRA_ICON_ADV1;
	CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);
}


static int CListMW_ExtraIconsRebuild(WPARAM wParam, LPARAM lParam) 
{
	int i;
	HIMAGELIST CSImages;

	if(gbXStatusEnabled && ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
  { // TODO: divide into icons, change for IconLib
		CSImages = ImageList_LoadImage(hInst, MAKEINTRESOURCE(IDB_XSTATUS), 16, 24, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION);
		
    for(i = 0; i < 24; i++) 
    {
      HICON hXIcon = ImageList_ExtractIcon(NULL, CSImages, i);

			hXStatusIcons[i] = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)hXIcon, 0);
      DestroyIcon(hXIcon);
		}

    ImageList_Destroy(CSImages);
	}

	return 0;
}


static int CListMW_ExtraIconsApply(WPARAM wParam, LPARAM lParam) 
{
	if(gbXStatusEnabled && ServiceExists(MS_CLIST_EXTRA_SET_ICON)) 
  {
		DWORD bXStatus = ICQGetContactSettingByte((HANDLE)wParam, "XStatusId", 0);

		if (bXStatus > 0 && bXStatus < 24) 
    {
      setContactExtraIcon((HANDLE)wParam, hXStatusIcons[bXStatus-1]);
		} 
    else 
    {
      setContactExtraIcon((HANDLE)wParam, (HANDLE)-1);
		}
	}
	return 0;
}


void InitXStatusEvents()
{
	hHookExtraIconsRebuild = HookEvent(ME_CLIST_EXTRA_LIST_REBUILD, CListMW_ExtraIconsRebuild);
	hHookExtraIconsApply = HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY, CListMW_ExtraIconsApply);
}


void UninitXStatusEvents()
{
	if (hHookExtraIconsRebuild)
		UnhookEvent(hHookExtraIconsRebuild);

	if (hHookExtraIconsApply)
		UnhookEvent(hHookExtraIconsApply);
}


const capstr capXStatus[24] = {
  {0x01, 0xD8, 0xD7, 0xEE, 0xAC, 0x3B, 0x49, 0x2A, 0xA5, 0x8D, 0xD3, 0xD8, 0x77, 0xE6, 0x6B, 0x92},
  {0x5A, 0x58, 0x1E, 0xA1, 0xE5, 0x80, 0x43, 0x0C, 0xA0, 0x6F, 0x61, 0x22, 0x98, 0xB7, 0xE4, 0xC7},
  {0x83, 0xC9, 0xB7, 0x8E, 0x77, 0xE7, 0x43, 0x78, 0xB2, 0xC5, 0xFB, 0x6C, 0xFC, 0xC3, 0x5B, 0xEC},
  {0xE6, 0x01, 0xE4, 0x1C, 0x33, 0x73, 0x4B, 0xD1, 0xBC, 0x06, 0x81, 0x1D, 0x6C, 0x32, 0x3D, 0x81},
  {0x8C, 0x50, 0xDB, 0xAE, 0x81, 0xED, 0x47, 0x86, 0xAC, 0xCA, 0x16, 0xCC, 0x32, 0x13, 0xC7, 0xB7},
  {0x3F, 0xB0, 0xBD, 0x36, 0xAF, 0x3B, 0x4A, 0x60, 0x9E, 0xEF, 0xCF, 0x19, 0x0F, 0x6A, 0x5A, 0x7F},
  {0xF8, 0xE8, 0xD7, 0xB2, 0x82, 0xC4, 0x41, 0x42, 0x90, 0xF8, 0x10, 0xC6, 0xCE, 0x0A, 0x89, 0xA6},
  {0x80, 0x53, 0x7D, 0xE2, 0xA4, 0x67, 0x4A, 0x76, 0xB3, 0x54, 0x6D, 0xFD, 0x07, 0x5F, 0x5E, 0xC6},
  {0xF1, 0x8A, 0xB5, 0x2E, 0xDC, 0x57, 0x49, 0x1D, 0x99, 0xDC, 0x64, 0x44, 0x50, 0x24, 0x57, 0xAF},
  {0x1B, 0x78, 0xAE, 0x31, 0xFA, 0x0B, 0x4D, 0x38, 0x93, 0xD1, 0x99, 0x7E, 0xEE, 0xAF, 0xB2, 0x18},
  {0x61, 0xBE, 0xE0, 0xDD, 0x8B, 0xDD, 0x47, 0x5D, 0x8D, 0xEE, 0x5F, 0x4B, 0xAA, 0xCF, 0x19, 0xA7},
  {0x48, 0x8E, 0x14, 0x89, 0x8A, 0xCA, 0x4A, 0x08, 0x82, 0xAA, 0x77, 0xCE, 0x7A, 0x16, 0x52, 0x08},
  {0x10, 0x7A, 0x9A, 0x18, 0x12, 0x32, 0x4D, 0xA4, 0xB6, 0xCD, 0x08, 0x79, 0xDB, 0x78, 0x0F, 0x09},
  {0x6F, 0x49, 0x30, 0x98, 0x4F, 0x7C, 0x4A, 0xFF, 0xA2, 0x76, 0x34, 0xA0, 0x3B, 0xCE, 0xAE, 0xA7},
  {0x12, 0x92, 0xE5, 0x50, 0x1B, 0x64, 0x4F, 0x66, 0xB2, 0x06, 0xB2, 0x9A, 0xF3, 0x78, 0xE4, 0x8D},
  {0xD4, 0xA6, 0x11, 0xD0, 0x8F, 0x01, 0x4E, 0xC0, 0x92, 0x23, 0xC5, 0xB6, 0xBE, 0xC6, 0xCC, 0xF0},
  {0x60, 0x9D, 0x52, 0xF8, 0xA2, 0x9A, 0x49, 0xA6, 0xB2, 0xA0, 0x25, 0x24, 0xC5, 0xE9, 0xD2, 0x60},
  {0x63, 0x62, 0x73, 0x37, 0xA0, 0x3F, 0x49, 0xFF, 0x80, 0xE5, 0xF7, 0x09, 0xCD, 0xE0, 0xA4, 0xEE},
  {0x1F, 0x7A, 0x40, 0x71, 0xBF, 0x3B, 0x4E, 0x60, 0xBC, 0x32, 0x4C, 0x57, 0x87, 0xB0, 0x4C, 0xF1},
  {0x78, 0x5E, 0x8C, 0x48, 0x40, 0xD3, 0x4C, 0x65, 0x88, 0x6F, 0x04, 0xCF, 0x3F, 0x3F, 0x43, 0xDF},
  {0xA6, 0xED, 0x55, 0x7E, 0x6B, 0xF7, 0x44, 0xD4, 0xA5, 0xD4, 0xD2, 0xE7, 0xD9, 0x5C, 0xE8, 0x1F},
  {0x12, 0xD0, 0x7E, 0x3E, 0xF8, 0x85, 0x48, 0x9E, 0x8E, 0x97, 0xA7, 0x2A, 0x65, 0x51, 0xE5, 0x8D},
  {0xBA, 0x74, 0xDB, 0x3E, 0x9E, 0x24, 0x43, 0x4B, 0x87, 0xB6, 0x2F, 0x6B, 0x8D, 0xFE, 0xE5, 0x0F},
  {0x63, 0x4F, 0x6B, 0xD8, 0xAD, 0xD2, 0x4A, 0xA1, 0xAA, 0xB9, 0x11, 0x5B, 0xC2, 0x6D, 0x05, 0xA1}};

const char* nameXStatus[24] = {
  "Angry",
  "Duck",
  "Tired",
  "Party",
  "Beer",
  "Thinking",
  "Eating",
  "TV",
  "Friends",
  "Coffee",
  "Music",
  "Business",
  "Camera",
  "Funny",
  "Phone",
  "Games",
  "College",
  "Shopping",
  "Sick",
  "Sleeping",
  "Surfing",
  "Internet",
  "Engineering",
  "Typing"};

void handleXStatusCaps(HANDLE hContact, char* caps, int capsize)
{
  HANDLE hIcon = (HANDLE)-1;

  if (!gbXStatusEnabled) return;

  if (caps)
  {
    int i;

    for (i = 0; i<24; i++)
    {
      if (MatchCap(caps, capsize, (const capstr*)capXStatus[i], 0x10))
      {
//        if (DBGetContactSettingByte(hContact, gpszICQProtoName, "XStatusId", 0) != (i + 1))
        { // status changed - we request details
          char *szNotify;
          int nNotifyLen;

          ICQWriteContactSettingByte(hContact, "XStatusId", (BYTE)(i+1));
          ICQWriteContactSettingString(hContact, "XStatusName", (char*)nameXStatus[i]);

          nNotifyLen = 94 + UINMAXLEN;
          szNotify = (char*)malloc(nNotifyLen);
          nNotifyLen = mir_snprintf(szNotify, nNotifyLen, "<srv><id>cAwaySrv</id><req><id>AwayStat</id><trans>1</trans><senderId>%d</senderId></req></srv>", dwLocalUIN);
          // TODO: this should be postponed
          SendXtrazNotifyRequest(hContact, "<Q><PluginID>srvMng</PluginID></Q>", szNotify);

          SAFE_FREE(&szNotify);
        }
        hIcon = hXStatusIcons[i];

        break;
      }
    }
  }
  else
  {
    ICQDeleteContactSetting(hContact, "XStatusId");
    ICQDeleteContactSetting(hContact, "XStatusName");
    ICQDeleteContactSetting(hContact, "XStatusMsg");
  }

  if (gbXStatusEnabled != 10)
  {
    setContactExtraIcon(hContact, hIcon);
  }
}


static WNDPROC OldMessageEditProc;

static LRESULT CALLBACK MessageEditSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) 
  {
		case WM_CHAR:
			if(wParam=='\n' && GetKeyState(VK_CONTROL)&0x8000) 
      {
				PostMessage(GetParent(hwnd),WM_COMMAND,IDOK,0);
				return 0;
			}
      if (wParam == 1 && GetKeyState(VK_CONTROL) & 0x8000) 
      {      //ctrl-a
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return 0;
      }
      if (wParam == 23 && GetKeyState(VK_CONTROL) & 0x8000) 
      {     // ctrl-w
        SendMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
        return 0;
      }
      if (wParam == 127 && GetKeyState(VK_CONTROL) & 0x8000) 
      {    //ctrl-backspace
        DWORD start, end;
        TCHAR *text;
        int textLen;

        SendMessage(hwnd, EM_GETSEL, (WPARAM) & end, (LPARAM) (PDWORD) NULL);
        SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
        SendMessage(hwnd, EM_GETSEL, (WPARAM) & start, (LPARAM) (PDWORD) NULL);
        textLen = GetWindowTextLength(hwnd);
        text = (TCHAR *) malloc(sizeof(TCHAR) * (textLen + 1));
        GetWindowText(hwnd, text, textLen + 1);
        MoveMemory(text + start, text + end, sizeof(TCHAR) * (textLen + 1 - end));
        SetWindowText(hwnd, text);
        SAFE_FREE(&text);
        SendMessage(hwnd, EM_SETSEL, start, start);
        SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), EN_CHANGE), (LPARAM) hwnd);
        return 0;
      }
			break;
	}
	return CallWindowProc(OldMessageEditProc,hwnd,msg,wParam,lParam);
}


struct SetXStatusData {
	BYTE bXStatus;
	int countdown;
	char okButtonFormat[64];
};

static BOOL CALLBACK SetXStatusDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
  struct SetXStatusData *dat = (struct SetXStatusData*)GetWindowLong(hwndDlg,GWL_USERDATA);

  switch(message) 
  {
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
      dat=(struct SetXStatusData*)malloc(sizeof(struct SetXStatusData));
			SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)dat);
      dat->bXStatus = (BYTE)lParam;
      SendDlgItemMessage(hwndDlg, IDC_XTITLE, EM_LIMITTEXT, 256, 0);
			SendDlgItemMessage(hwndDlg, IDC_XMSG, EM_LIMITTEXT, 1024, 0);
			OldMessageEditProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_XTITLE),GWL_WNDPROC,(LONG)MessageEditSubclassProc);
			OldMessageEditProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_XMSG),GWL_WNDPROC,(LONG)MessageEditSubclassProc);
			{	
        char str[256], format[128];
				GetWindowText(hwndDlg, format, sizeof(format));
				mir_snprintf(str, sizeof(str), format, Translate(nameXStatus[dat->bXStatus-1]));
				SetWindowText(hwndDlg, str);
			}
      GetDlgItemText(hwndDlg,IDOK,dat->okButtonFormat,sizeof(dat->okButtonFormat));
			{	
        DBVARIANT dbv;
        char szSetting[64];

        sprintf(szSetting, "XStatus%dName", dat->bXStatus);
        if (!ICQGetContactSetting(NULL, szSetting, &dbv))
        {
          SetDlgItemText(hwndDlg,IDC_XTITLE,dbv.pszVal);
          DBFreeVariant(&dbv);
        }
        sprintf(szSetting, "XStatus%dMsg", dat->bXStatus);
        if (!ICQGetContactSetting(NULL, szSetting, &dbv))
        {
          SetDlgItemText(hwndDlg,IDC_XMSG,dbv.pszVal);
          DBFreeVariant(&dbv);
        }
			}
			dat->countdown=5;
			SendMessage(hwndDlg, WM_TIMER, 0, 0);
			SetTimer(hwndDlg,1,1000,0);

			return TRUE;
		}
		case WM_TIMER:
			if(dat->countdown==-1) 
      {
        DestroyWindow(hwndDlg); 
        break;
      }
			{	
        char str[64];
				mir_snprintf(str,sizeof(str),dat->okButtonFormat,dat->countdown);
				SetDlgItemText(hwndDlg,IDOK,str);
			}
			dat->countdown--;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam)) 
      {
				case IDOK:
					DestroyWindow(hwndDlg);
					break;
        case IDC_XTITLE:
				case IDC_XMSG:
					KillTimer(hwndDlg,1);
					SetDlgItemText(hwndDlg,IDOK,Translate("OK"));
					break;
			}
			break;

		case WM_DESTROY:
			{
        char str[1025];
        char szSetting[64];

				GetDlgItemText(hwndDlg,IDC_XMSG,str,sizeof(str));
        sprintf(szSetting, "XStatus%dMsg", dat->bXStatus);
        ICQWriteContactSettingString(NULL, szSetting, str);
        ICQWriteContactSettingString(NULL, "XStatusMsg", str);
				GetDlgItemText(hwndDlg,IDC_XTITLE,str,sizeof(str));
        sprintf(szSetting, "XStatus%dName", dat->bXStatus);
        ICQWriteContactSettingString(NULL, szSetting, str);
        ICQWriteContactSettingString(NULL, "XStatusName", str);

        setUserInfo();
			}
			SetWindowLong(GetDlgItem(hwndDlg,IDC_XMSG),GWL_WNDPROC,(LONG)OldMessageEditProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_XTITLE),GWL_WNDPROC,(LONG)OldMessageEditProc);
			SAFE_FREE(&dat);
			break;
	}
	return FALSE;
}


static void setXStatus(BYTE bXStatus)
{
  CLISTMENUITEM mi;
  BYTE bOldXStatus = ICQGetContactSettingByte(NULL, "XStatusId", 0);

  mi.cbSize = sizeof(mi);

  if (bOldXStatus <= 24)
  {
    mi.flags = CMIM_FLAGS;
    CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hXStatusItems[bOldXStatus], (LPARAM)&mi);
  }

  ICQWriteContactSettingByte(NULL, "XStatusId", bXStatus);
  mi.flags = CMIM_FLAGS | (bXStatus?CMIF_CHECKED:0);
  CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hXStatusItems[bXStatus], (LPARAM)&mi);

  if (bXStatus)
  {
    DBVARIANT dbv;
    char szSetting[64];

    sprintf(szSetting, "XStatus%dName", bXStatus);
    if (!ICQGetContactSetting(NULL, szSetting, &dbv))
    {
      ICQWriteContactSettingString(NULL, "XStatusName", dbv.pszVal);
      DBFreeVariant(&dbv);
    }
    else 
      ICQDeleteContactSetting(NULL, "XStatusName");
    sprintf(szSetting, "XStatus%dMsg", bXStatus);
    if (!ICQGetContactSetting(NULL, szSetting, &dbv))
    {
      ICQWriteContactSettingString(NULL, "XStatusMsg", dbv.pszVal);
      DBFreeVariant(&dbv);
    }
    else 
      ICQDeleteContactSetting(NULL, "XStatusMsg");
    sprintf(szSetting, "XStatus%dStat", bXStatus);
    if (!ICQGetContactSettingByte(NULL, szSetting, 0))
      CreateDialogParam(hInst,MAKEINTRESOURCE(IDD_SETXSTATUS),NULL,SetXStatusDlgProc,(LPARAM)bXStatus);
    else
      setUserInfo();
  }
  else
  {
    ICQWriteContactSettingString(NULL, "XStatusName", Translate(nameXStatus[bXStatus-1]));
    ICQDeleteContactSetting(NULL, "XStatusMsg");

    setUserInfo();
  }
}


static int menuXStatus0(WPARAM wParam,LPARAM lParam)
{
  setXStatus(0); return 0;
}

static int menuXStatus1(WPARAM wParam,LPARAM lParam)
{
  setXStatus(1); return 0;
}

static int menuXStatus2(WPARAM wParam,LPARAM lParam)
{
  setXStatus(2); return 0;
}

static int menuXStatus3(WPARAM wParam,LPARAM lParam)
{
  setXStatus(3); return 0;
}

static int menuXStatus4(WPARAM wParam,LPARAM lParam)
{
  setXStatus(4); return 0;
}

static int menuXStatus5(WPARAM wParam,LPARAM lParam)
{
  setXStatus(5); return 0;
}

static int menuXStatus6(WPARAM wParam,LPARAM lParam)
{
  setXStatus(6); return 0;
}

static int menuXStatus7(WPARAM wParam,LPARAM lParam)
{
  setXStatus(7); return 0;
}

static int menuXStatus8(WPARAM wParam,LPARAM lParam)
{
  setXStatus(8); return 0;
}

static int menuXStatus9(WPARAM wParam,LPARAM lParam)
{
  setXStatus(9); return 0;
}

static int menuXStatus10(WPARAM wParam,LPARAM lParam)
{
  setXStatus(10); return 0;
}

static int menuXStatus11(WPARAM wParam,LPARAM lParam)
{
  setXStatus(11); return 0;
}

static int menuXStatus12(WPARAM wParam,LPARAM lParam)
{
  setXStatus(12); return 0;
}

static int menuXStatus13(WPARAM wParam,LPARAM lParam)
{
  setXStatus(13); return 0;
}

static int menuXStatus14(WPARAM wParam,LPARAM lParam)
{
  setXStatus(14); return 0;
}

static int menuXStatus15(WPARAM wParam,LPARAM lParam)
{
  setXStatus(15); return 0;
}

static int menuXStatus16(WPARAM wParam,LPARAM lParam)
{
  setXStatus(16); return 0;
}

static int menuXStatus17(WPARAM wParam,LPARAM lParam)
{
  setXStatus(17); return 0;
}

static int menuXStatus18(WPARAM wParam,LPARAM lParam)
{
  setXStatus(18); return 0;
}

static int menuXStatus19(WPARAM wParam,LPARAM lParam)
{
  setXStatus(19); return 0;
}

static int menuXStatus20(WPARAM wParam,LPARAM lParam)
{
  setXStatus(20); return 0;
}

static int menuXStatus21(WPARAM wParam,LPARAM lParam)
{
  setXStatus(21); return 0;
}

static int menuXStatus22(WPARAM wParam,LPARAM lParam)
{
  setXStatus(22); return 0;
}

static int menuXStatus23(WPARAM wParam,LPARAM lParam)
{
  setXStatus(23); return 0;
}

static int menuXStatus24(WPARAM wParam,LPARAM lParam)
{
  setXStatus(24); return 0;
}


void InitXStatusItems()
{
  CLISTMENUITEM mi;
	int i = 0;
	char srvFce[MAX_PATH + 64];
  char szItem[MAX_PATH + 14];

	BYTE bXStatus = ICQGetContactSettingByte(NULL, "XStatusId", 0);
	HIMAGELIST CSImages = ImageList_LoadImage(hInst, MAKEINTRESOURCE(IDB_XSTATUS), 16, 24, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION);

  if (!gbXStatusEnabled) return;

  sprintf(szItem, Translate("%s Custom Status"), gpszICQProtoName);
  mi.cbSize = sizeof(mi);
	mi.pszPopupName = szItem;
	mi.popupPosition= 500084000;
	mi.position = 2000040000;

	for(i = 0; i < 25; i++) 
  {
		sprintf(srvFce, "%s\\SetXStatus%d", gpszICQProtoName, i);
    mi.hIcon = (i > 0) ? ImageList_ExtractIcon(NULL, CSImages, i-1) : NULL;
		mi.position++;

		switch(i) 
    {
      case 0: CreateServiceFunction(srvFce, menuXStatus0); break;
			case 1: CreateServiceFunction(srvFce, menuXStatus1); break;
			case 2: CreateServiceFunction(srvFce, menuXStatus2); break;
			case 3: CreateServiceFunction(srvFce, menuXStatus3); break;
			case 4: CreateServiceFunction(srvFce, menuXStatus4); break;
			case 5: CreateServiceFunction(srvFce, menuXStatus5); break;
			case 6: CreateServiceFunction(srvFce, menuXStatus6); break;
			case 7: CreateServiceFunction(srvFce, menuXStatus7); break;
			case 8: CreateServiceFunction(srvFce, menuXStatus8); break;
			case 9: CreateServiceFunction(srvFce, menuXStatus9); break;
			case 10: CreateServiceFunction(srvFce, menuXStatus10); break;
			case 11: CreateServiceFunction(srvFce, menuXStatus11); break;
			case 12: CreateServiceFunction(srvFce, menuXStatus12); break;
			case 13: CreateServiceFunction(srvFce, menuXStatus13); break;
			case 14: CreateServiceFunction(srvFce, menuXStatus14); break;
			case 15: CreateServiceFunction(srvFce, menuXStatus15); break;
			case 16: CreateServiceFunction(srvFce, menuXStatus16); break;
			case 17: CreateServiceFunction(srvFce, menuXStatus17); break;
			case 18: CreateServiceFunction(srvFce, menuXStatus18); break;
			case 19: CreateServiceFunction(srvFce, menuXStatus19); break;
			case 20: CreateServiceFunction(srvFce, menuXStatus20); break;
			case 21: CreateServiceFunction(srvFce, menuXStatus21); break;
			case 22: CreateServiceFunction(srvFce, menuXStatus22); break;
			case 23: CreateServiceFunction(srvFce, menuXStatus23); break;
			case 24: CreateServiceFunction(srvFce, menuXStatus24); break;
		}

    mi.flags = bXStatus == i?CMIF_CHECKED:0;
    mi.pszName = Translate(i?nameXStatus[i-1]:"None");
		mi.pszService = srvFce;
		hXStatusItems[i] = (HANDLE)CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi);
    if (i) DestroyIcon(mi.hIcon);
	}
  ImageList_Destroy(CSImages);
}

