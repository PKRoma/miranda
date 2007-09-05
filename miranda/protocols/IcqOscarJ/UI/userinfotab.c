// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006,2007 Joe Kucera
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
//  Code for User details ICQ specific pages
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


extern WORD wListenPort;

extern char* calcMD5Hash(char* szFile);
extern char* MirandaVersionToString(char* szStr, int bUnicode, int v, int m);

extern char* nameXStatus[29];

static BOOL CALLBACK IcqDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void SetValue(HWND hwndDlg, int idCtrl, HANDLE hContact, char *szModule, char *szSetting, int special);

#define SVS_NORMAL        0
#define SVS_ZEROISUNSPEC  2
#define SVS_IP            3
#define SVS_SIGNED        6
#define SVS_ICQVERSION    8
#define SVS_TIMESTAMP     9
#define SVS_STATUSID      10


int OnDetailsInit(WPARAM wParam, LPARAM lParam)
{
  OPTIONSDIALOGPAGE odp = {0};

  if ((!IsICQContact((HANDLE)lParam)) && lParam)
    return 0;

  odp.cbSize = sizeof(odp);
  odp.hIcon = NULL;
  odp.hInstance = hInst;
  odp.pfnDlgProc = IcqDlgProc;
  odp.position = -1900000000;
  odp.pszTemplate = MAKEINTRESOURCE(IDD_INFO_ICQ);
  AddUserInfoPageUtf(&odp, wParam, gpszICQProtoName);

  InitChangeDetails(wParam, lParam);

  return 0;
}



static BOOL CALLBACK IcqDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {

  case WM_INITDIALOG:
    ICQTranslateDialog(hwndDlg);
    return TRUE;
    
  case WM_NOTIFY:
    {
      switch (((LPNMHDR)lParam)->idFrom)
      {
        
      case 0:
        {
          switch (((LPNMHDR)lParam)->code)
          {
            
          case PSN_INFOCHANGED:
            {
              char* szProto;
              HANDLE hContact = (HANDLE)((LPPSHNOTIFY)lParam)->lParam;

              if (hContact == NULL)
                szProto = gpszICQProtoName;
              else
                szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

              if (szProto == NULL)
                break;

              SetValue(hwndDlg, IDC_UIN, hContact, szProto, UNIQUEIDSETTING, SVS_NORMAL);
              SetValue(hwndDlg, IDC_ONLINESINCE, hContact, szProto, "LogonTS", SVS_TIMESTAMP);
              SetValue(hwndDlg, IDC_IDLETIME, hContact, szProto, "IdleTS", SVS_TIMESTAMP);
              SetValue(hwndDlg, IDC_IP, hContact, szProto, "IP", SVS_IP);
              SetValue(hwndDlg, IDC_REALIP, hContact, szProto, "RealIP", SVS_IP);

              if (hContact)
              {
                SetValue(hwndDlg, IDC_PORT, hContact, szProto, "UserPort", SVS_ZEROISUNSPEC);
                SetValue(hwndDlg, IDC_VERSION, hContact, szProto, "Version", SVS_ICQVERSION);
                SetValue(hwndDlg, IDC_MIRVER, hContact, szProto, "MirVer", SVS_ZEROISUNSPEC);
                if (ICQGetContactSettingByte(hContact, "ClientID", 0))
                  ICQWriteContactSettingDword(hContact, "TickTS", 0);
                SetValue(hwndDlg, IDC_SYSTEMUPTIME, hContact, szProto, "TickTS", SVS_TIMESTAMP);
                SetValue(hwndDlg, IDC_STATUS, hContact, szProto, "Status", SVS_STATUSID);
              }
              else
              {
                char str[MAX_PATH];

                SetValue(hwndDlg, IDC_PORT, hContact, (char*)DBVT_WORD, (char*)wListenPort, SVS_ZEROISUNSPEC);
                SetValue(hwndDlg, IDC_VERSION, hContact, (char*)DBVT_WORD, (char*)ICQ_VERSION, SVS_ICQVERSION);
                SetValue(hwndDlg, IDC_MIRVER, hContact, (char*)DBVT_ASCIIZ, MirandaVersionToString(str, gbUnicodeCore, ICQ_PLUG_VERSION, MIRANDA_VERSION), SVS_ZEROISUNSPEC);
                SetDlgItemTextUtf(hwndDlg, IDC_SUPTIME, ICQTranslateUtfStatic("Member since:", str, MAX_PATH));
                SetValue(hwndDlg, IDC_SYSTEMUPTIME, hContact, szProto, "MemberTS", SVS_TIMESTAMP);
                SetValue(hwndDlg, IDC_STATUS, hContact, (char*)DBVT_WORD, (char*)gnCurrentStatus, SVS_STATUSID);
              }
            }
            break;
          }
        }
        break;
      }
    }
    break;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDCANCEL:
      SendMessage(GetParent(hwndDlg),msg,wParam,lParam);
      break;
    }
    break;
  }

  return FALSE;  
}



static void SetValue(HWND hwndDlg, int idCtrl, HANDLE hContact, char* szModule, char* szSetting, int special)
{
  DBVARIANT dbv = {0};
  char str[MAX_PATH];
  char* pstr = NULL;
  int unspecified = 0;
  int bUtf = 0, bDbv = 0, bAlloc = 0;

  dbv.type = DBVT_DELETED;

  if ((hContact == NULL) && ((int)szModule<0x100))
  {
    dbv.type = (BYTE)szModule;

    switch((int)szModule)
    {
    case DBVT_BYTE:
      dbv.cVal = (BYTE)szSetting;
      break;
    case DBVT_WORD:
      dbv.wVal = (WORD)szSetting;
      break;
    case DBVT_DWORD:
      dbv.dVal = (DWORD)szSetting;
      break;
    case DBVT_ASCIIZ:
      dbv.pszVal = pstr = szSetting;
      break;
    default:
      unspecified = 1;
      dbv.type = DBVT_DELETED;
    }
  }
  else
  {
    if (szModule == NULL)
      unspecified = 1;
    else
    {
      unspecified = DBGetContactSetting(hContact, szModule, szSetting, &dbv);
      bDbv = 1;
    }
  }

  if (!unspecified)
  {
    switch (dbv.type)
    {

    case DBVT_BYTE:
      unspecified = (special == SVS_ZEROISUNSPEC && dbv.bVal == 0);
      pstr = itoa(special == SVS_SIGNED ? dbv.cVal:dbv.bVal, str, 10);
      break;
      
    case DBVT_WORD:
      if (special == SVS_ICQVERSION)
      {
        if (dbv.wVal != 0)
        {
          char szExtra[80];

          null_snprintf(str, 250, "%d", dbv.wVal);
          pstr = str;

          if (hContact && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 1))
          {
            ICQTranslateUtfStatic(" (DC Established)", szExtra, 80);
            strcat(str, szExtra);
            bUtf = 1;
          }
        }
        else
          unspecified = 1;
      }
      else if (special == SVS_STATUSID)
      {
        char* pXName;
        char* pszStatus;
        BYTE bXStatus = ICQGetContactSettingByte(hContact, DBSETTING_XSTATUSID, 0);

        pszStatus = MirandaStatusToStringUtf(dbv.wVal);
        if (bXStatus)
        {
          pXName = ICQGetContactSettingUtf(hContact, DBSETTING_XSTATUSNAME, "");
          if (!strlennull(pXName))
          { // give default name
            pXName = ICQTranslateUtf(nameXStatus[bXStatus-1]);
          }
          null_snprintf(str, sizeof(str), "%s (%s)", pszStatus, pXName);
          SAFE_FREE(&pXName);
        }
        else
          null_snprintf(str, sizeof(str), pszStatus);

        bUtf = 1;
        SAFE_FREE(&pszStatus);
        pstr = str;
        unspecified = 0;
      }
      else
      {
        unspecified = (special == SVS_ZEROISUNSPEC && dbv.wVal == 0);
        pstr = itoa(special == SVS_SIGNED ? dbv.sVal:dbv.wVal, str, 10);
      }
      break;
      
    case DBVT_DWORD:
      unspecified = (special == SVS_ZEROISUNSPEC && dbv.dVal == 0);
      if (special == SVS_IP)
      {
        struct in_addr ia;
        ia.S_un.S_addr = htonl(dbv.dVal);
        pstr = inet_ntoa(ia);
        if (dbv.dVal == 0)
          unspecified=1;
      }
      else if (special == SVS_TIMESTAMP)
      {
        if (dbv.dVal == 0)
          unspecified = 1;
        else
        {
          pstr = asctime(localtime(&dbv.dVal));
          pstr[24] = '\0'; // Remove newline
        }
      }
      else
        pstr = itoa(special == SVS_SIGNED ? dbv.lVal:dbv.dVal, str, 10);
      break;
      
    case DBVT_ASCIIZ:
    case DBVT_WCHAR:
      unspecified = (special == SVS_ZEROISUNSPEC && dbv.pszVal[0] == '\0');
      if (!unspecified && pstr != szSetting)
      {
        pstr = UniGetContactSettingUtf(hContact, szModule, szSetting, NULL);
        bUtf = 1;
        bAlloc = 1;
      }
      if (idCtrl == IDC_UIN)
        SetDlgItemTextUtf(hwndDlg, IDC_UINSTATIC, ICQTranslateUtfStatic("ScreenName:", str, MAX_PATH));
      break;
      
    default:
      pstr = str;
      strcpy(str,"???");
      break;
    }
  }
  
  EnableDlgItem(hwndDlg, idCtrl, !unspecified);
  if (unspecified)
    SetDlgItemTextUtf(hwndDlg, idCtrl, ICQTranslateUtfStatic("<not specified>", str, MAX_PATH));
  else if (bUtf)
    SetDlgItemTextUtf(hwndDlg, idCtrl, pstr);
  else
    SetDlgItemText(hwndDlg, idCtrl, pstr);
  
  if (bDbv)
    ICQFreeVariant(&dbv);

  if (bAlloc)
    SAFE_FREE(&pstr);
}
