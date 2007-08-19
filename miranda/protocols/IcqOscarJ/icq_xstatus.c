// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006,2007 Angeli-Ka, Joe Kucera
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
//  Support for Custom Statuses
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"
#include "m_cluiframes.h"


#define XSTATUS_COUNT 32

extern void setUserInfo();

extern HANDLE hxstatusiconchanged;

int bHideXStatusUI = 0;
static int bStatusMenu = 0;
static int bXStatusMenuBuilt = 0;
static HANDLE hHookExtraIconsRebuild = NULL;
static HANDLE hHookStatusBuild = NULL;
static HANDLE hHookExtraIconsApply = NULL;
static HANDLE hXStatusIcons[XSTATUS_COUNT];
static HANDLE hXStatusItems[XSTATUS_COUNT + 1];

void CListShowMenuItem(HANDLE hMenuItem, BYTE bShow);
void CListSetMenuItemIcon(HANDLE hMenuItem, HICON hIcon);


DWORD sendXStatusDetailsRequest(HANDLE hContact, int bForced)
{
  char *szNotify;
  int nNotifyLen;
  DWORD dwCookie;

  nNotifyLen = 94 + UINMAXLEN;
  szNotify = (char*)_alloca(nNotifyLen);
  nNotifyLen = null_snprintf(szNotify, nNotifyLen, "<srv><id>cAwaySrv</id><req><id>AwayStat</id><trans>1</trans><senderId>%d</senderId></req></srv>", dwLocalUIN);

  dwCookie = SendXtrazNotifyRequest(hContact, "<Q><PluginID>srvMng</PluginID></Q>", szNotify, bForced);

  return dwCookie;
}



static DWORD requestXStatusDetails(HANDLE hContact, BOOL bAllowDelay)
{
  rate_record rr = {0};

  if (!validateStatusMessageRequest(hContact, MTYPE_SCRIPT_NOTIFY))
    return 0; // apply privacy rules

  // delay is disabled only if fired from dialog
  if (!CheckContactCapabilities(hContact, CAPF_XTRAZ) && bAllowDelay)
    return 0; // Contact does not support xtraz, do not request details

  rr.hContact = hContact;
  rr.bType = RIT_XSTATUS_REQUEST;
  rr.nRequestType = 0x101; // request
  rr.nMinDelay = 1000;    // delay at least 1s
  EnterCriticalSection(&ratesMutex);
  rr.wGroup = ratesGroupFromSNAC(gRates, ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND);
  LeaveCriticalSection(&ratesMutex);

  if (!handleRateItem(&rr, bAllowDelay))
    return sendXStatusDetailsRequest(hContact, !bAllowDelay);

  return -1; // delayed
}



static HANDLE LoadXStatusIconLibrary(char* path, const char* sub)
{
  char* p = strrchr(path, '\\');
  HANDLE hLib;

  strcpy(p, sub);
  strcat(p, "\\xstatus_ICQ.dll");
  if (hLib = LoadLibrary(path)) return hLib;
  strcpy(p, sub);
  strcat(p, "\\xstatus_icons.dll");
  if (hLib = LoadLibrary(path)) return hLib;
  strcpy(p, "\\");
  return hLib;
}



static char* InitXStatusIconLibrary(char* buf)
{
	char path[2*MAX_PATH];
  HMODULE hXStatusIconsDLL;

  // get miranda's exe path
  GetModuleFileNameA(NULL, path, MAX_PATH);

  hXStatusIconsDLL = LoadXStatusIconLibrary(path, "\\Icons");
  if (!hXStatusIconsDLL) // TODO: add "Custom Folders" support
    hXStatusIconsDLL = LoadXStatusIconLibrary(path, "\\Plugins");

  if (hXStatusIconsDLL)
  {
    strcpy(buf, path);

    if (LoadStringA(hXStatusIconsDLL, IDS_IDENTIFY, path, sizeof(path)) == 0 || strcmpnull(path, "# Custom Status Icons #"))
    { // library is invalid
      *buf = '\0';
    }
    FreeLibrary(hXStatusIconsDLL);
  }
  else
    *buf = '\0';

  return buf;
}



HICON GetXStatusIcon(int bStatus, UINT flags)
{
  char szTemp[64];
  HICON icon;

  null_snprintf(szTemp, sizeof(szTemp), "xstatus%d", bStatus - 1);
  icon = IconLibGetIcon(szTemp);

  if (flags & LR_SHARED)
    return icon;
  else
    return CopyIcon(icon);
}



static void setContactExtraIcon(HANDLE hContact, HANDLE hIcon)
{
  IconExtraColumn iec;

  iec.cbSize = sizeof(iec);
  iec.hImage = hIcon;
  iec.ColumnType = EXTRA_ICON_ADV1;
  CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM)hContact, (LPARAM)&iec);

  NotifyEventHooks(hxstatusiconchanged, (WPARAM)hContact, (LPARAM)hIcon);
}



static int CListMW_ExtraIconsRebuild(WPARAM wParam, LPARAM lParam) 
{
  int i;

  if (gbXStatusEnabled && ServiceExists(MS_CLIST_EXTRA_ADD_ICON))
  {
    for (i = 0; i < XSTATUS_COUNT; i++) 
    {
      hXStatusIcons[i] = (HANDLE)CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM)GetXStatusIcon(i + 1, LR_SHARED), 0);
    }
  }

  return 0;
}



static int CListMW_ExtraIconsApply(WPARAM wParam, LPARAM lParam) 
{
  if (gbXStatusEnabled && ServiceExists(MS_CLIST_EXTRA_SET_ICON)) 
  {
    if (IsICQContact((HANDLE)wParam))
    { // only apply icons to our contacts, do not mess others
      DWORD bXStatus = ICQGetContactSettingByte((HANDLE)wParam, DBSETTING_XSTATUSID, 0);

      if (bXStatus > 0 && bXStatus <= XSTATUS_COUNT) 
      {
        setContactExtraIcon((HANDLE)wParam, hXStatusIcons[bXStatus-1]);
      } 
      else 
      {
        setContactExtraIcon((HANDLE)wParam, (HANDLE)-1);
      }
    }
  }
  return 0;
}



static int CListMW_BuildStatusItems(WPARAM wParam, LPARAM lParam)
{
  InitXStatusItems(TRUE);
  return 0;
}



void InitXStatusEvents()
{
  if (!hHookStatusBuild)
    if (bStatusMenu = ServiceExists(MS_CLIST_ADDSTATUSMENUITEM))
      hHookStatusBuild = HookEvent(ME_CLIST_PREBUILDSTATUSMENU, CListMW_BuildStatusItems);

  if (!hHookExtraIconsRebuild)
    hHookExtraIconsRebuild = HookEvent(ME_CLIST_EXTRA_LIST_REBUILD, CListMW_ExtraIconsRebuild);

  if (!hHookExtraIconsApply)
    hHookExtraIconsApply = HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY, CListMW_ExtraIconsApply);
  
  memset(gpiXStatusIconsIdx,-1,sizeof(gpiXStatusIconsIdx));
}



void UninitXStatusEvents()
{
  if (hHookStatusBuild)
    UnhookEvent(hHookStatusBuild);

  if (hHookExtraIconsRebuild)
    UnhookEvent(hHookExtraIconsRebuild);

  if (hHookExtraIconsApply)
    UnhookEvent(hHookExtraIconsApply);
}



const capstr capXStatus[XSTATUS_COUNT] = {
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
  {0x63, 0x4F, 0x6B, 0xD8, 0xAD, 0xD2, 0x4A, 0xA1, 0xAA, 0xB9, 0x11, 0x5B, 0xC2, 0x6D, 0x05, 0xA1},
  {0x2C, 0xE0, 0xE4, 0xE5, 0x7C, 0x64, 0x43, 0x70, 0x9C, 0x3A, 0x7A, 0x1C, 0xE8, 0x78, 0xA7, 0xDC},
  {0x10, 0x11, 0x17, 0xC9, 0xA3, 0xB0, 0x40, 0xF9, 0x81, 0xAC, 0x49, 0xE1, 0x59, 0xFB, 0xD5, 0xD4},
  {0x16, 0x0C, 0x60, 0xBB, 0xDD, 0x44, 0x43, 0xF3, 0x91, 0x40, 0x05, 0x0F, 0x00, 0xE6, 0xC0, 0x09},
  {0x64, 0x43, 0xC6, 0xAF, 0x22, 0x60, 0x45, 0x17, 0xB5, 0x8C, 0xD7, 0xDF, 0x8E, 0x29, 0x03, 0x52},
  {0x16, 0xF5, 0xB7, 0x6F, 0xA9, 0xD2, 0x40, 0x35, 0x8C, 0xC5, 0xC0, 0x84, 0x70, 0x3C, 0x98, 0xFA},
  {0x63, 0x14, 0x36, 0xff, 0x3f, 0x8a, 0x40, 0xd0, 0xa5, 0xcb, 0x7b, 0x66, 0xe0, 0x51, 0xb3, 0x64}, 
  {0xb7, 0x08, 0x67, 0xf5, 0x38, 0x25, 0x43, 0x27, 0xa1, 0xff, 0xcf, 0x4c, 0xc1, 0x93, 0x97, 0x97},
  {0xdd, 0xcf, 0x0e, 0xa9, 0x71, 0x95, 0x40, 0x48, 0xa9, 0xc6, 0x41, 0x32, 0x06, 0xd6, 0xf2, 0x80}};

const char* nameXStatus[XSTATUS_COUNT] = {
  LPGEN("Angry"),
  LPGEN("Taking a bath"),
  LPGEN("Tired"),
  LPGEN("Party"),
  LPGEN("Drinking beer"),
  LPGEN("Thinking"),
  LPGEN("Eating"),
  LPGEN("Watching TV"),
  LPGEN("Meeting"),
  LPGEN("Coffee"),
  LPGEN("Listening to music"),
  LPGEN("Business"),
  LPGEN("Shooting"),
  LPGEN("Having fun"),
  LPGEN("On the phone"),
  LPGEN("Gaming"),
  LPGEN("Studying"),
  LPGEN("Shopping"),
  LPGEN("Feeling sick"),
  LPGEN("Sleeping"),
  LPGEN("Surfing"),
  LPGEN("Browsing"),
  LPGEN("Working"),
  LPGEN("Typing"),
  LPGEN("Picnic"),
  LPGEN("Cooking"),
  LPGEN("Smoking"),
  LPGEN("I'm high"),
  LPGEN("On WC"),
  LPGEN("To be or not to be"),
  LPGEN("Watching pro7 on TV"),
  LPGEN("Love")};


void handleXStatusCaps(HANDLE hContact, char* caps, int capsize)
{
  HANDLE hIcon = (HANDLE)-1;

  if (!gbXStatusEnabled) return;

  if (caps)
  {
    int i;

    for (i = 0; i < XSTATUS_COUNT; i++)
    {
      if (MatchCap(caps, capsize, (const capstr*)capXStatus[i], 0x10))
      {
        BYTE bXStatusId = (BYTE)(i+1);
        char str[MAX_PATH];

        if (ICQGetContactSettingByte(hContact, DBSETTING_XSTATUSID, -1) != bXStatusId)
        { // only write default name when it is really needed, i.e. on Custom Status change
          ICQWriteContactSettingByte(hContact, DBSETTING_XSTATUSID, bXStatusId);
          ICQWriteContactSettingUtf(hContact, DBSETTING_XSTATUSNAME, ICQTranslateUtfStatic(nameXStatus[i], str, MAX_PATH));
        }

        if (ICQGetContactSettingByte(NULL, "XStatusAuto", DEFAULT_XSTATUS_AUTO))
          requestXStatusDetails(hContact, TRUE);

        hIcon = hXStatusIcons[i];

        break;
      }
    }
  }
  if (hIcon == (HANDLE)-1)
  {
    ICQDeleteContactSetting(hContact, DBSETTING_XSTATUSID);
    ICQDeleteContactSetting(hContact, DBSETTING_XSTATUSNAME);
    ICQDeleteContactSetting(hContact, DBSETTING_XSTATUSMSG);
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
      { // ctrl-a
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return 0;
      }
      if (wParam == 23 && GetKeyState(VK_CONTROL) & 0x8000) 
      { // ctrl-w
        SendMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
        return 0;
      }
      if (wParam == 127 && GetKeyState(VK_CONTROL) & 0x8000) 
      { // ctrl-backspace
        DWORD start, end;
        wchar_t *text;

        SendMessage(hwnd, EM_GETSEL, (WPARAM) & end, (LPARAM) (PDWORD) NULL);
        SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
        SendMessage(hwnd, EM_GETSEL, (WPARAM) & start, (LPARAM) (PDWORD) NULL);
        text = GetWindowTextUcs(hwnd);
        MoveMemory(text + start, text + end, sizeof(wchar_t) * (wcslen(text) + 1 - end));
        SetWindowTextUcs(hwnd, text);
        SAFE_FREE(&text);
        SendMessage(hwnd, EM_SETSEL, start, start);
        SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), EN_CHANGE), (LPARAM) hwnd);
        return 0;
      }
      break;
  }
  return CallWindowProcUtf(OldMessageEditProc,hwnd,msg,wParam,lParam);
}


typedef struct SetXStatusData_s {
  BYTE bAction;
  BYTE bXStatus;
  HANDLE hContact;
  HANDLE hEvent;
  DWORD iEvent;
  int countdown;
  char *okButtonFormat;
} SetXStatusData;

typedef struct InitXStatusData_s {
  BYTE bAction;
  BYTE bXStatus;
  char *szXStatusName;
  char *szXStatusMsg;
  HANDLE hContact;
} InitXStatusData;

#define HM_PROTOACK (WM_USER+10)
static BOOL CALLBACK SetXStatusDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
  SetXStatusData *dat = (SetXStatusData*)GetWindowLong(hwndDlg,GWL_USERDATA);
  char str[MAX_PATH];

  switch(message) 
  {
    case HM_PROTOACK:
    {
      ACKDATA *ack = (ACKDATA*)lParam;
      char *szText;

      if (ack->type != ICQACKTYPE_XSTATUS_RESPONSE) break;	
      if (ack->hContact != dat->hContact) break;
      if ((DWORD)ack->hProcess != dat->iEvent) break;

      ShowWindow(GetDlgItem(hwndDlg, IDC_RETRXSTATUS), SW_HIDE);
      ShowWindow(GetDlgItem(hwndDlg, IDC_XMSG), SW_SHOW);
      ShowWindow(GetDlgItem(hwndDlg, IDC_XTITLE), SW_SHOW);
      SetDlgItemTextUtf(hwndDlg,IDOK,ICQTranslateUtfStatic("Close", str, MAX_PATH));
      UnhookEvent(dat->hEvent); dat->hEvent = NULL;
      szText = ICQGetContactSettingUtf(dat->hContact, DBSETTING_XSTATUSNAME, "");
      SetDlgItemTextUtf(hwndDlg, IDC_XTITLE, szText);
      SAFE_FREE(&szText);
      szText = ICQGetContactSettingUtf(dat->hContact, DBSETTING_XSTATUSMSG, "");
      SetDlgItemTextUtf(hwndDlg, IDC_XMSG, szText);
      SAFE_FREE(&szText);

      break;
    }
    case WM_INITDIALOG:
    {
      InitXStatusData *init = (InitXStatusData*)lParam;

      ICQTranslateDialog(hwndDlg);
      dat = (SetXStatusData*)SAFE_MALLOC(sizeof(SetXStatusData));
      SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)dat);
      dat->bAction = init->bAction;

      if (!init->bAction)
      { // set our xStatus
        dat->bXStatus = init->bXStatus;
        SendDlgItemMessage(hwndDlg, IDC_XTITLE, EM_LIMITTEXT, 256, 0);
        SendDlgItemMessage(hwndDlg, IDC_XMSG, EM_LIMITTEXT, 1024, 0);
        OldMessageEditProc = (WNDPROC)SetWindowLongUtf(GetDlgItem(hwndDlg,IDC_XTITLE),GWL_WNDPROC,(LONG)MessageEditSubclassProc);
        OldMessageEditProc = (WNDPROC)SetWindowLongUtf(GetDlgItem(hwndDlg,IDC_XMSG),GWL_WNDPROC,(LONG)MessageEditSubclassProc);
        dat->okButtonFormat = GetDlgItemTextUtf(hwndDlg,IDOK);

        SetDlgItemTextUtf(hwndDlg, IDC_XTITLE, init->szXStatusName);
        SetDlgItemTextUtf(hwndDlg, IDC_XMSG, init->szXStatusMsg);

        dat->countdown=5;
        SendMessage(hwndDlg, WM_TIMER, 0, 0);
        SetTimer(hwndDlg,1,1000,0);
      }
      else
      { // retrieve contact's xStatus
        dat->hContact = init->hContact;
        dat->bXStatus = ICQGetContactSettingByte(dat->hContact, DBSETTING_XSTATUSID, 0);
        dat->okButtonFormat = NULL;
        SendMessage(GetDlgItem(hwndDlg, IDC_XTITLE), EM_SETREADONLY, 1, 0);
        SendMessage(GetDlgItem(hwndDlg, IDC_XMSG), EM_SETREADONLY, 1, 0);
        if (!ICQGetContactSettingByte(NULL, "XStatusAuto", DEFAULT_XSTATUS_AUTO))
        {
          SetDlgItemTextUtf(hwndDlg,IDOK,ICQTranslateUtfStatic("Cancel", str, MAX_PATH));
          dat->hEvent = HookEventMessage(ME_PROTO_ACK, hwndDlg, HM_PROTOACK);
          ShowWindow(GetDlgItem(hwndDlg, IDC_RETRXSTATUS), SW_SHOW);
          ShowWindow(GetDlgItem(hwndDlg, IDC_XMSG), SW_HIDE);
          ShowWindow(GetDlgItem(hwndDlg, IDC_XTITLE), SW_HIDE);
          dat->iEvent = requestXStatusDetails(dat->hContact, FALSE);
        }
        else
        {
          char *szText;

          SetDlgItemTextUtf(hwndDlg,IDOK,ICQTranslateUtfStatic("Close", str, MAX_PATH));
          dat->hEvent = NULL;
          szText = ICQGetContactSettingUtf(dat->hContact, DBSETTING_XSTATUSNAME, "");
          SetDlgItemTextUtf(hwndDlg, IDC_XTITLE, szText);
          SAFE_FREE(&szText);
          szText = ICQGetContactSettingUtf(dat->hContact, DBSETTING_XSTATUSMSG, "");
          SetDlgItemTextUtf(hwndDlg, IDC_XMSG, szText);
          SAFE_FREE(&szText);
        }
      }

      if (dat->bXStatus)
      {
        SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)GetXStatusIcon(dat->bXStatus, LR_SHARED));
      }

      {  
        char *format;
        char buf[MAX_PATH];

        format = GetWindowTextUtf(hwndDlg);
        null_snprintf(str, sizeof(str), format, dat->bXStatus?ICQTranslateUtfStatic(nameXStatus[dat->bXStatus-1], buf, MAX_PATH):"");
        SetWindowTextUtf(hwndDlg, str);
        SAFE_FREE(&format);
      }
      return TRUE;
    }
    case WM_TIMER:
      if(dat->countdown==-1) 
      {
        DestroyWindow(hwndDlg); 
        break;
      }
      {  
        null_snprintf(str,sizeof(str),dat->okButtonFormat,dat->countdown);
        SetDlgItemTextUtf(hwndDlg,IDOK,str);
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
          if (!dat->bAction)
          { // set our xStatus
            KillTimer(hwndDlg,1);
            SetDlgItemTextUtf(hwndDlg,IDOK,ICQTranslateUtfStatic("OK", str, MAX_PATH));
          }
          break;
      }
      break;

    case WM_DESTROY:
      if (!dat->bAction)
      { // set our xStatus
        char szSetting[64];
        char* szValue;

        ICQWriteContactSettingByte(NULL, DBSETTING_XSTATUSID, dat->bXStatus);
        szValue = GetDlgItemTextUtf(hwndDlg,IDC_XMSG);
        sprintf(szSetting, "XStatus%dMsg", dat->bXStatus);
        ICQWriteContactSettingUtf(NULL, szSetting, szValue);
        ICQWriteContactSettingUtf(NULL, DBSETTING_XSTATUSMSG, szValue);
        SAFE_FREE(&szValue);
        szValue = GetDlgItemTextUtf(hwndDlg,IDC_XTITLE);
        sprintf(szSetting, "XStatus%dName", dat->bXStatus);
        ICQWriteContactSettingUtf(NULL, szSetting, szValue);
        ICQWriteContactSettingUtf(NULL, DBSETTING_XSTATUSNAME, szValue);
        SAFE_FREE(&szValue);

        setUserInfo();

        SetWindowLongUtf(GetDlgItem(hwndDlg,IDC_XMSG),GWL_WNDPROC,(LONG)OldMessageEditProc);
        SetWindowLongUtf(GetDlgItem(hwndDlg,IDC_XTITLE),GWL_WNDPROC,(LONG)OldMessageEditProc);
      }
      if (dat->hEvent) UnhookEvent(dat->hEvent);
      SAFE_FREE(&dat->okButtonFormat);
      SAFE_FREE(&dat);
      break;

    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      break;
  }
  return FALSE;
}



static void setXStatusEx(BYTE bXStatus, BYTE bQuiet)
{
  CLISTMENUITEM mi = {0};
  BYTE bOldXStatus = ICQGetContactSettingByte(NULL, DBSETTING_XSTATUSID, 0);

  mi.cbSize = sizeof(mi);

  if (bOldXStatus <= XSTATUS_COUNT)
  {
    mi.flags = CMIM_FLAGS;
    CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hXStatusItems[bOldXStatus], (LPARAM)&mi);
  }

  mi.flags = CMIM_FLAGS | CMIF_CHECKED;
  CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hXStatusItems[bXStatus], (LPARAM)&mi);

  if (bXStatus)
  {
    char szSetting[64];
    char str[MAX_PATH];
    char *szName, *szMsg;

    sprintf(szSetting, "XStatus%dName", bXStatus);
    szName = ICQGetContactSettingUtf(NULL, szSetting, ICQTranslateUtfStatic(nameXStatus[bXStatus-1], str, MAX_PATH));
    sprintf(szSetting, "XStatus%dMsg", bXStatus);
    szMsg = ICQGetContactSettingUtf(NULL, szSetting, "");

    sprintf(szSetting, "XStatus%dStat", bXStatus);
    if (!bQuiet && !ICQGetContactSettingByte(NULL, szSetting, 0))
    {
      InitXStatusData init;

      init.bAction = 0; // set
      init.bXStatus = bXStatus;
      init.szXStatusName = szName;
      init.szXStatusMsg = szMsg;
      DialogBoxUtf(FALSE, hInst, MAKEINTRESOURCEA(IDD_SETXSTATUS),NULL,SetXStatusDlgProc,(LPARAM)&init);
    }
    else
    {
      ICQWriteContactSettingByte(NULL, DBSETTING_XSTATUSID, bXStatus);
      ICQWriteContactSettingUtf(NULL, DBSETTING_XSTATUSNAME, szName);
      ICQWriteContactSettingUtf(NULL, DBSETTING_XSTATUSMSG, szMsg);

      setUserInfo();
    }
    SAFE_FREE(&szName);
    SAFE_FREE(&szMsg);
  }
  else
  {
    ICQWriteContactSettingByte(NULL, DBSETTING_XSTATUSID, bXStatus);
    ICQDeleteContactSetting(NULL, DBSETTING_XSTATUSNAME);
    ICQDeleteContactSetting(NULL, DBSETTING_XSTATUSMSG);

    setUserInfo();
  }
}



static int menuXStatus(WPARAM wParam,LPARAM lParam,LPARAM fParam)
{
  setXStatusEx((BYTE)fParam, 0);

  return 0;
}



void InitXStatusItems(BOOL bAllowStatus)
{
  CLISTMENUITEM mi;
  int i = 0;
  char srvFce[MAX_PATH + 64];
  char szItem[MAX_PATH + 64];
  HANDLE hXStatusRoot;

  BYTE bXStatus = ICQGetContactSettingByte(NULL, DBSETTING_XSTATUSID, 0);

  if (!gbXStatusEnabled) return;

  if (bStatusMenu && !bAllowStatus) return;

  null_snprintf(szItem, sizeof(szItem), ICQTranslate("%s Custom Status"), gpszICQProtoName);
  mi.cbSize = sizeof(mi);
  mi.pszPopupName = szItem;
  mi.popupPosition= 500084000;
  mi.position = 2000040000;

  for(i = 0; i <= XSTATUS_COUNT; i++) 
  {
    null_snprintf(srvFce, sizeof(srvFce), "%s/menuXStatus%d", gpszICQProtoName, i);

    mi.hIcon = i ? GetXStatusIcon(i, LR_SHARED) : NULL;
    mi.position++;

    if (!bXStatusMenuBuilt)
      CreateServiceFunctionParam(srvFce, menuXStatus, i);

    mi.flags = bXStatus == i?CMIF_CHECKED:0;
    mi.pszName = i?(char*)nameXStatus[i-1]:LPGEN("None");
    mi.pszService = srvFce;
    mi.pszContactOwner = gpszICQProtoName;

    if (bStatusMenu)
      hXStatusItems[i] = (HANDLE)CallService(MS_CLIST_ADDSTATUSMENUITEM, (WPARAM)&hXStatusRoot, (LPARAM)&mi);
    else
      hXStatusItems[i] = (HANDLE)CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi);
  }

  bXStatusMenuBuilt = 1;
}



void InitXStatusIcons()
{
  char szSection[MAX_PATH + 64];
  char str[MAX_PATH], prt[MAX_PATH];
  char lib[2*MAX_PATH] = {0};
  char* icon_lib;
  int i;
  
  if (!gbXStatusEnabled) return;

  icon_lib = InitXStatusIconLibrary(lib);

  null_snprintf(szSection, sizeof(szSection), ICQTranslateUtfStatic("%s/Custom Status", str, MAX_PATH), ICQTranslateUtfStatic(gpszICQProtoName, prt, MAX_PATH));

  for (i = 0; i < XSTATUS_COUNT; i++) 
  {
    char szTemp[64];

    null_snprintf(szTemp, sizeof(szTemp), "xstatus%d", i);
    IconLibDefine(ICQTranslateUtfStatic(nameXStatus[i], str, MAX_PATH), szSection, szTemp, NULL, icon_lib, -(IDI_XSTATUS1+i));
  }

  if (bXStatusMenuBuilt)
    ChangedIconsXStatus();
}



void ChangedIconsXStatus()
{ // reload icons
  int i;

  if (!gbXStatusEnabled) return;

  for (i = 1; i <= XSTATUS_COUNT; i++)
  {
    CListSetMenuItemIcon(hXStatusItems[i], GetXStatusIcon(i, LR_SHARED));
  }
  memset(gpbXStatusIconsValid,0,sizeof(gpbXStatusIconsValid));
}



int IcqShowXStatusDetails(WPARAM wParam, LPARAM lParam)
{
  InitXStatusData init;

  init.bAction = 1; // retrieve
  init.hContact = (HANDLE)wParam;
  DialogBoxUtf(FALSE, hInst, MAKEINTRESOURCEA(IDD_SETXSTATUS), NULL, SetXStatusDlgProc, (LPARAM)&init);

  return 0;
}



int IcqSetXStatus(WPARAM wParam, LPARAM lParam)
{ // obsolete (TODO: remove in next version)
  if (!gbXStatusEnabled) return 0;

  if (wParam >= 0 && wParam <= XSTATUS_COUNT)
  {
    setXStatusEx((BYTE)wParam, 1);

    return wParam;
  }
  return 0;
}



int IcqGetXStatus(WPARAM wParam, LPARAM lParam)
{ // obsolete (TODO: remove in next version)
  BYTE status = ICQGetContactSettingByte(NULL, DBSETTING_XSTATUSID, 0);

  if (!gbXStatusEnabled) return 0;

  if (!icqOnline) return 0;

  if (status < 1 || status > XSTATUS_COUNT) status = 0;

  if (wParam) *((char**)wParam) = DBSETTING_XSTATUSNAME;
  if (lParam) *((char**)lParam) = DBSETTING_XSTATUSMSG;

  return status;
}



int IcqSetXStatusEx(WPARAM wParam, LPARAM lParam)
{
  ICQ_CUSTOM_STATUS *pData = (ICQ_CUSTOM_STATUS*)lParam;

  if (!gbXStatusEnabled) return 1;

  if (pData->cbSize < sizeof(ICQ_CUSTOM_STATUS)) return 1; // Failure
  
  if (pData->flags & CSSF_MASK_STATUS)
  { // set custom status
    int status = *pData->status;

    if (status >= 0 && status <= XSTATUS_COUNT)
    {
      setXStatusEx((BYTE)status, 1);
    }
    else
      return 1; // Failure
  }

  if (pData->flags & (CSSF_MASK_NAME | CSSF_MASK_MESSAGE))
  {
    BYTE status = ICQGetContactSettingByte(NULL, DBSETTING_XSTATUSID, 0);

    if (status < 1 || status > XSTATUS_COUNT) return 1; // Failure

    if (pData->flags & CSSF_MASK_NAME)
    { // set custom status name
      if (pData->flags & CSSF_UNICODE)
      {
        char* utf = make_utf8_string(pData->pwszName);

        ICQWriteContactSettingUtf(NULL, DBSETTING_XSTATUSNAME, utf);
        SAFE_FREE(&utf);
      }
      else
        ICQWriteContactSettingString(NULL, DBSETTING_XSTATUSNAME, pData->pszName);
    }
    if (pData->flags & CSSF_MASK_MESSAGE)
    { // set custom status message
      if (pData->flags & CSSF_UNICODE)
      {
        char* utf = make_utf8_string(pData->pwszMessage);

        ICQWriteContactSettingUtf(NULL, DBSETTING_XSTATUSMSG, utf);
        SAFE_FREE(&utf);
      }
      else
        ICQWriteContactSettingString(NULL, DBSETTING_XSTATUSMSG, pData->pszMessage);
    }
  }

  if (pData->flags & CSSF_DISABLE_UI)
  { // hide menu items
    int n;

    bHideXStatusUI = (*pData->wParam) ? 0 : 1;

    for (n = 0; n <= XSTATUS_COUNT; n++)
      CListShowMenuItem(hXStatusItems[n], (BYTE)!bHideXStatusUI);
  }

  return 0; // Success
}



int IcqGetXStatusEx(WPARAM wParam, LPARAM lParam)
{
  ICQ_CUSTOM_STATUS *pData = (ICQ_CUSTOM_STATUS*)lParam;
  HANDLE hContact = (HANDLE)wParam;

  if (!gbXStatusEnabled) return 1;

  if (pData->cbSize < sizeof(ICQ_CUSTOM_STATUS)) return 1; // Failure

  if (pData->flags & CSSF_MASK_STATUS)
  { // fill status member
    *pData->status = ICQGetContactSettingByte(hContact, DBSETTING_XSTATUSID, 0);
  }

  if (pData->flags & CSSF_MASK_NAME)
  { // fill status name member
    if (pData->flags & CSSF_DEFAULT_NAME)
    {
      int status = *pData->wParam;

      if (status < 1 || status > XSTATUS_COUNT) return 1; // Failure

      if (pData->flags & CSSF_UNICODE)
      {
        char *text = (char*)nameXStatus[status -1];

        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, text, -1, pData->pwszName, MAX_PATH);
      }
      else
        strcpy(pData->pszName, nameXStatus[status - 1]);
    }
    else
    {
      if (pData->flags & CSSF_UNICODE)
      {
        char* str = ICQGetContactSettingUtf(hContact, DBSETTING_XSTATUSNAME, "");
        wchar_t* wstr = make_unicode_string(str);

        wcscpy(pData->pwszName, wstr);
        SAFE_FREE(&str);
        SAFE_FREE(&wstr);
      }
      else
      {
        DBVARIANT dbv = {0};
      
        if (!ICQGetContactSetting(hContact, DBSETTING_XSTATUSNAME, &dbv) && dbv.pszVal)
          strcpy(pData->pszName, dbv.pszVal);
        else 
          strcpy(pData->pszName, "");

        ICQFreeVariant(&dbv);
      }
    }
  }

  if (pData->flags & CSSF_MASK_MESSAGE)
  { // fill status message member
    if (pData->flags & CSSF_UNICODE)
    {
      char* str = ICQGetContactSettingUtf(hContact, DBSETTING_XSTATUSMSG, "");
      wchar_t* wstr = make_unicode_string(str);

      wcscpy(pData->pwszMessage, wstr);
      SAFE_FREE(&str);
      SAFE_FREE(&wstr);
    }
    else
    {
      DBVARIANT dbv = {0};
      
      if (!ICQGetContactSetting(hContact, DBSETTING_XSTATUSMSG, &dbv) && dbv.pszVal)
        strcpy(pData->pszMessage, dbv.pszVal);
      else
        strcpy(pData->pszMessage, "");

      ICQFreeVariant(&dbv);
    }
  }

  if (pData->flags & CSSF_DISABLE_UI)
  {
    if (pData->wParam) *pData->wParam = !bHideXStatusUI;
  }

  if (pData->flags & CSSF_STATUSES_COUNT)
  {
    if (pData->wParam) *pData->wParam = XSTATUS_COUNT;
  }

  if (pData->flags & CSSF_STR_SIZES)
  {
    DBVARIANT dbv = {0};

    if (pData->wParam)
    {
      if (!ICQGetContactSetting(hContact, DBSETTING_XSTATUSNAME, &dbv))
      {
        *pData->wParam = strlennull(dbv.pszVal);
        ICQFreeVariant(&dbv);
      }
      else
        *pData->wParam = 0;
    }
    if (pData->lParam)
    {
      if (!ICQGetContactSetting(hContact, DBSETTING_XSTATUSMSG, &dbv))
      {
        *pData->lParam = strlennull(dbv.pszVal);
        ICQFreeVariant(&dbv);
      }
      else
        *pData->lParam = 0;
    }
  }

  return 0; // Success
}



int IcqGetXStatusIcon(WPARAM wParam, LPARAM lParam)
{
  if (!gbXStatusEnabled) return 0;

  if (!wParam)
    wParam = ICQGetContactSettingByte(NULL, DBSETTING_XSTATUSID, 0);

  if (wParam >= 1 && wParam <= XSTATUS_COUNT)
  {
    if (lParam == LR_SHARED)
      return (int)GetXStatusIcon((BYTE)wParam, lParam);
    else
      return (int)GetXStatusIcon((BYTE)wParam, 0);
  }
  return 0;
}



int IcqRequestXStatusDetails(WPARAM wParam, LPARAM lParam)
{
  HANDLE hContact = (HANDLE)wParam;

  if (!gbXStatusEnabled) return 0;

  if (hContact && !ICQGetContactSettingByte(NULL, "XStatusAuto", DEFAULT_XSTATUS_AUTO) &&
    ICQGetContactSettingByte(hContact, DBSETTING_XSTATUSID, 0))
  { // user has xstatus, no auto-retrieve details, valid contact, request details
    return requestXStatusDetails(hContact, TRUE);
  }
  return 0;
}



int IcqRequestAdvStatusIconIdx(WPARAM wParam, LPARAM lParam)
{
  BYTE bXStatus;

  if (!gbXStatusEnabled) return -1;

  bXStatus = ICQGetContactSettingByte((HANDLE)wParam, DBSETTING_XSTATUSID, 0);

  if (bXStatus > 0 && bXStatus <= XSTATUS_COUNT)
  {
    int idx=-1;

    if (!gpbXStatusIconsValid[bXStatus-1])
    {	// adding icon
			int idx = gpiXStatusIconsIdx[bXStatus-1] ? gpiXStatusIconsIdx[bXStatus-1] : 0; 
			HIMAGELIST hCListImageList = (HIMAGELIST)CallService(MS_CLIST_GETICONSIMAGELIST,0,0);

			if (hCListImageList)
			{
				if (idx > 0)
					ImageList_ReplaceIcon(hCListImageList, idx, GetXStatusIcon(bXStatus, LR_SHARED));
				else
					gpiXStatusIconsIdx[bXStatus-1] = ImageList_AddIcon(hCListImageList, GetXStatusIcon(bXStatus, LR_SHARED));
				gpbXStatusIconsValid[bXStatus-1] = TRUE;
			}
				
		}
		idx = gpbXStatusIconsValid[bXStatus-1] ? gpiXStatusIconsIdx[bXStatus-1] : -1;
		
    if (idx > 0) 
			return (idx & 0xFFFF) << 16;
	}
	return -1;
}
