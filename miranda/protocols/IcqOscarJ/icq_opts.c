// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006 Joe Kucera
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
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


static BOOL CALLBACK DlgProcIcqOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcIcqContactsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcIcqFeaturesOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcIcqPrivacyOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


static const char* szLogLevelDescr[] = {"Display all problems", "Display problems causing possible loss of data", "Display explanations for disconnection", "Display problems requiring user intervention"};


static void AddUniPageUtf(const char* szService, OPTIONSDIALOGPAGE *op, WPARAM wParam, const char *szGroup, const char *szTitle)
{
  char str[MAX_PATH];
  char *pszTitle;

  if (strstr(szTitle, "%s"))
  {
    char *lTitle = ICQTranslateUtfStatic(szTitle, str);
    int size = strlennull(lTitle) + strlennull(gpszICQProtoName);
 
    pszTitle = (char*)_alloca(size);
    null_snprintf(pszTitle, size, lTitle, gpszICQProtoName);
  }
  else
    pszTitle = (char*)ICQTranslateUtfStatic(szTitle, str);

  if (gbUnicodeCore)
  {
    wchar_t *utitle, *ugroup;

    utitle = make_unicode_string(pszTitle);
    if (szGroup)
      ugroup = make_unicode_string(ICQTranslateUtfStatic(szGroup, str));
    else
      ugroup = NULL;
    op->pszTitle = (char*)utitle; // this is union with ptszTitle
    op->pszGroup = (char*)ugroup;
    op->flags |= ODPF_UNICODE;
    CallService(szService, wParam, (LPARAM)op);
    SAFE_FREE(&utitle);
    SAFE_FREE(&ugroup);
  }
  else
  {
    char *title, *group, *tmp;
    int size;

    size = strlennull(pszTitle) + 2;
    title = (char*)_alloca(size);
    utf8_decode_static(pszTitle, title, size);
    if (szGroup)
    {
      tmp = ICQTranslateUtfStatic(szGroup, str);
      size = strlennull(tmp) + 2;
      group = (char*)_alloca(size);
      utf8_decode_static(tmp, group, size);
    }
    else
      group = NULL;
    op->pszTitle = title;
    op->pszGroup = group;
    CallService(szService, wParam, (LPARAM)op);
  }
}



void AddOptionsPageUtf(OPTIONSDIALOGPAGE *op, WPARAM wParam, const char *szGroup, const char *szTitle)
{
  AddUniPageUtf(MS_OPT_ADDPAGE, op, wParam, szGroup, szTitle);
}



void AddUserInfoPageUtf(OPTIONSDIALOGPAGE *op, WPARAM wParam, const char *szTitle)
{
  AddUniPageUtf(MS_USERINFO_ADDPAGE, op, wParam, NULL, szTitle);
}



int IcqOptInit(WPARAM wParam, LPARAM lParam)
{
  OPTIONSDIALOGPAGE odp = {0};

  odp.cbSize = sizeof(odp);
  odp.position = -800000000;
  odp.hInstance = hInst;

  // Add "icq" option
  odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_ICQ);
  odp.pfnDlgProc = DlgProcIcqOpts;
  odp.flags = ODPF_BOLDGROUPS;
  odp.nIDBottomSimpleControl = IDC_STICQGROUP;
  AddOptionsPageUtf(&odp, wParam, "Network", gpszICQProtoName);

  // Add "contacts" option
  odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_ICQCONTACTS);
  odp.pfnDlgProc = DlgProcIcqContactsOpts;
  odp.nIDBottomSimpleControl = 0;
  AddOptionsPageUtf(&odp, wParam, "Network", "%s Contacts");
  
  // Add "features" option
  odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_ICQFEATURES);
  odp.pfnDlgProc = DlgProcIcqFeaturesOpts;
  odp.flags |= ODPF_EXPERTONLY;
  odp.nIDBottomSimpleControl = 0;
  AddOptionsPageUtf(&odp, wParam, "Network", "%s Features");

  // Add "privacy" option
  odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_ICQPRIVACY);
  odp.pfnDlgProc = DlgProcIcqPrivacyOpts;
  odp.flags = ODPF_BOLDGROUPS;
  odp.nIDBottomSimpleControl = 0;
  AddOptionsPageUtf(&odp, wParam, "Network", "%s Privacy");

  InitPopupOpts(wParam);

  return 0;
}



static void LoadDBCheckState(HWND hwndDlg, int idCtrl, const char* szSetting, BYTE bDef)
{
  CheckDlgButton(hwndDlg, idCtrl, ICQGetContactSettingByte(NULL, szSetting, bDef));
}



static void StoreDBCheckState(HWND hwndDlg, int idCtrl, const char* szSetting)
{
  ICQWriteContactSettingByte(NULL, szSetting, (BYTE)IsDlgButtonChecked(hwndDlg, idCtrl));
}



static BOOL CALLBACK DlgProcIcqOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
    {
      char pszPwd[16];
      char szServer[MAX_PATH];

      ICQTranslateDialog(hwndDlg);

      SetDlgItemInt(hwndDlg, IDC_ICQNUM, ICQGetContactSettingUIN(NULL), FALSE);

      if (!ICQGetContactStaticString(NULL, "Password", pszPwd, sizeof(pszPwd)))
      {
        CallService(MS_DB_CRYPT_DECODESTRING, strlennull(pszPwd) + 1, (LPARAM)pszPwd);

        //bit of a security hole here, since it's easy to extract a password from an edit box
        SetDlgItemText(hwndDlg, IDC_PASSWORD, pszPwd);
      }
      
      if (!ICQGetContactStaticString(NULL, "OscarServer", szServer, MAX_PATH))
      {
        SetDlgItemText(hwndDlg, IDC_ICQSERVER, szServer);
      }
      else
      {
        SetDlgItemText(hwndDlg, IDC_ICQSERVER, DEFAULT_SERVER_HOST);
      }
      
      SetDlgItemInt(hwndDlg, IDC_ICQPORT, ICQGetContactSettingWord(NULL, "OscarPort", DEFAULT_SERVER_PORT), FALSE);
      LoadDBCheckState(hwndDlg, IDC_KEEPALIVE, "KeepAlive", 0);
      LoadDBCheckState(hwndDlg, IDC_USEGATEWAY, "UseGateway", 0);
      LoadDBCheckState(hwndDlg, IDC_SLOWSEND, "SlowSend", 1);
      LoadDBCheckState(hwndDlg, IDC_SECURE, "SecureLogin", DEFAULT_SECURE_LOGIN);
      SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL, TBM_SETRANGE, FALSE, MAKELONG(0, 3));
      SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL, TBM_SETPOS, TRUE, 3-ICQGetContactSettingByte(NULL, "ShowLogLevel", LOG_WARNING));
      SetDlgItemTextUtf(hwndDlg, IDC_LEVELDESCR, ICQTranslateUtfStatic(szLogLevelDescr[3-SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL, TBM_GETPOS, 0, 0)], szServer));
      ShowWindow(GetDlgItem(hwndDlg, IDC_RECONNECTREQD), SW_HIDE);
      LoadDBCheckState(hwndDlg, IDC_NOERRMULTI, "IgnoreMultiErrorBox", 0);
      
      return TRUE;
    }
    
  case WM_HSCROLL:
    {
      char str[MAX_PATH];

      SetDlgItemTextUtf(hwndDlg, IDC_LEVELDESCR, ICQTranslateUtfStatic(szLogLevelDescr[3-SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL,TBM_GETPOS, 0, 0)], str));
      SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
    }
    break;
    
  case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {

      case IDC_LOOKUPLINK:
        CallService(MS_UTILS_OPENURL, 1, (LPARAM)URL_FORGOT_PASSWORD);
        return TRUE;
        
      case IDC_NEWUINLINK:
        CallService(MS_UTILS_OPENURL, 1, (LPARAM)URL_REGISTER);
        return TRUE;
        
      case IDC_RESETSERVER:
        SetDlgItemText(hwndDlg, IDC_ICQSERVER, DEFAULT_SERVER_HOST);
        SetDlgItemInt(hwndDlg, IDC_ICQPORT, DEFAULT_SERVER_PORT, FALSE);
        SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
        return TRUE;
      }
      
      if (icqOnline && LOWORD(wParam) != IDC_SLOWSEND)
      {
        char szClass[80];
        
        
        GetClassName((HWND)lParam, szClass, sizeof(szClass));
        
        if (lstrcmpi(szClass, "EDIT") || HIWORD(wParam) == EN_CHANGE)
          ShowWindow(GetDlgItem(hwndDlg, IDC_RECONNECTREQD), SW_SHOW);
      }
      
      if ((LOWORD(wParam)==IDC_ICQNUM || LOWORD(wParam)==IDC_PASSWORD || LOWORD(wParam)==IDC_ICQSERVER || LOWORD(wParam)==IDC_ICQPORT) &&
        (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))
      {
        return 0;
      }
      
      SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
      break;
    }
    
  case WM_NOTIFY:
    {
      switch (((LPNMHDR)lParam)->code)
      {
        
      case PSN_APPLY:
        {
          char str[128];
          
          ICQWriteContactSettingDword(NULL, UNIQUEIDSETTING, (DWORD)GetDlgItemInt(hwndDlg, IDC_ICQNUM, NULL, FALSE));
          GetDlgItemText(hwndDlg, IDC_PASSWORD, str, sizeof(str));
          if (strlennull(str))
          {
            strcpy(gpszPassword, str);
            gbRememberPwd = TRUE;
          }
          else
          {
            gbRememberPwd = ICQGetContactSettingByte(NULL, "RememberPass", 0);
          }
          CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), (LPARAM)str);
          ICQWriteContactSettingString(NULL, "Password", str);
          GetDlgItemText(hwndDlg,IDC_ICQSERVER, str, sizeof(str));
          ICQWriteContactSettingString(NULL, "OscarServer", str);
          ICQWriteContactSettingWord(NULL, "OscarPort", (WORD)GetDlgItemInt(hwndDlg, IDC_ICQPORT, NULL, FALSE));
          StoreDBCheckState(hwndDlg, IDC_KEEPALIVE, "KeepAlive");
          StoreDBCheckState(hwndDlg, IDC_USEGATEWAY, "UseGateway");
          StoreDBCheckState(hwndDlg, IDC_SLOWSEND, "SlowSend");
          StoreDBCheckState(hwndDlg, IDC_SECURE, "SecureLogin");
          StoreDBCheckState(hwndDlg, IDC_NOERRMULTI, "IgnoreMultiErrorBox");
          ICQWriteContactSettingByte(NULL, "ShowLogLevel", (BYTE)(3-SendDlgItemMessage(hwndDlg, IDC_LOGLEVEL, TBM_GETPOS, 0, 0)));

          return TRUE;
        }
      }
    }
    break;
  }

  return FALSE;
}



static const UINT icqPrivacyControls[]={IDC_DCALLOW_ANY, IDC_DCALLOW_CLIST, IDC_DCALLOW_AUTH, IDC_ADD_ANY, IDC_ADD_AUTH, IDC_WEBAWARE, IDC_PUBLISHPRIMARY, IDC_STATIC_DC1, IDC_STATIC_DC2, IDC_STATIC_CLIST};
static BOOL CALLBACK DlgProcIcqPrivacyOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    
  case WM_INITDIALOG:
    {
      int nDcType;
      int nAddAuth;

      nDcType = ICQGetContactSettingByte(NULL, "DCType", 0);
      nAddAuth = ICQGetContactSettingByte(NULL, "Auth", 1);
      
      ICQTranslateDialog(hwndDlg);
      if (!icqOnline)
      {
        icq_EnableMultipleControls(hwndDlg, icqPrivacyControls, sizeof(icqPrivacyControls)/sizeof(icqPrivacyControls[0]), FALSE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_STATIC_NOTONLINE), SW_SHOW);
      }
      else 
      {
        ShowWindow(GetDlgItem(hwndDlg, IDC_STATIC_NOTONLINE), SW_HIDE);
      }
      CheckDlgButton(hwndDlg, IDC_DCALLOW_ANY, (nDcType == 0));
      CheckDlgButton(hwndDlg, IDC_DCALLOW_CLIST, (nDcType == 1));
      CheckDlgButton(hwndDlg, IDC_DCALLOW_AUTH, (nDcType == 2));
      CheckDlgButton(hwndDlg, IDC_ADD_ANY, (nAddAuth == 0));
      CheckDlgButton(hwndDlg, IDC_ADD_AUTH, (nAddAuth == 1));
      LoadDBCheckState(hwndDlg, IDC_WEBAWARE, "WebAware", 0);
      LoadDBCheckState(hwndDlg, IDC_PUBLISHPRIMARY, "PublishPrimaryEmail", 0);
      LoadDBCheckState(hwndDlg, IDC_STATUSMSG_CLIST, "StatusMsgReplyCList", 0);
      LoadDBCheckState(hwndDlg, IDC_STATUSMSG_VISIBLE, "StatusMsgReplyVisible", 0);
      if (!ICQGetContactSettingByte(NULL, "StatusMsgReplyCList", 0))
        EnableDlgItem(hwndDlg, IDC_STATUSMSG_VISIBLE, FALSE);

      return TRUE;
    }
    
  case WM_COMMAND:
    switch (LOWORD(wParam))  
    {
    case IDC_DCALLOW_ANY:
    case IDC_DCALLOW_CLIST:
    case IDC_DCALLOW_AUTH:
    case IDC_ADD_ANY:
    case IDC_ADD_AUTH:
    case IDC_WEBAWARE:
    case IDC_PUBLISHPRIMARY:
    case IDC_STATUSMSG_VISIBLE:
      if ((HWND)lParam != GetFocus())  return 0;
      break;
    case IDC_STATUSMSG_CLIST:
      if (IsDlgButtonChecked(hwndDlg, IDC_STATUSMSG_CLIST)) 
      {
        EnableDlgItem(hwndDlg, IDC_STATUSMSG_VISIBLE, TRUE);
        LoadDBCheckState(hwndDlg, IDC_STATUSMSG_VISIBLE, "StatusMsgReplyVisible", 0);
      }
      else 
      {
        EnableDlgItem(hwndDlg, IDC_STATUSMSG_VISIBLE, FALSE);
        CheckDlgButton(hwndDlg, IDC_STATUSMSG_VISIBLE, FALSE);
      }
      break;
    default:
      return 0;
    }
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
    break;
    
  case WM_NOTIFY:
    switch (((LPNMHDR)lParam)->code) 
    {
      case PSN_APPLY:
        StoreDBCheckState(hwndDlg, IDC_WEBAWARE, "WebAware");
        StoreDBCheckState(hwndDlg, IDC_PUBLISHPRIMARY, "PublishPrimaryEmail");
        StoreDBCheckState(hwndDlg, IDC_STATUSMSG_CLIST, "StatusMsgReplyCList");
        StoreDBCheckState(hwndDlg, IDC_STATUSMSG_VISIBLE, "StatusMsgReplyVisible");
        if (IsDlgButtonChecked(hwndDlg, IDC_DCALLOW_AUTH))
          ICQWriteContactSettingByte(NULL, "DCType", 2);
        else if (IsDlgButtonChecked(hwndDlg, IDC_DCALLOW_CLIST))
          ICQWriteContactSettingByte(NULL, "DCType", 1);
        else 
          ICQWriteContactSettingByte(NULL, "DCType", 0);
        if (IsDlgButtonChecked(hwndDlg, IDC_ADD_AUTH))
          ICQWriteContactSettingByte(NULL, "Auth", 1);
        else 
          ICQWriteContactSettingByte(NULL, "Auth", 0);
        
        
        if (icqOnline)
        {
          PBYTE buf=NULL;
          int buflen=0;
          
          ppackLEWord(&buf, &buflen, 0);
          ppackLELNTSfromDB(&buf, &buflen, "Nick");
          ppackLELNTSfromDB(&buf, &buflen, "FirstName");
          ppackLELNTSfromDB(&buf, &buflen, "LastName");
          ppackLELNTSfromDB(&buf, &buflen, "e-mail");
          ppackLELNTSfromDB(&buf, &buflen, "City");
          ppackLELNTSfromDB(&buf, &buflen, "State");
          ppackLELNTSfromDB(&buf ,&buflen, "Phone");
          ppackLELNTSfromDB(&buf ,&buflen, "Fax");
          ppackLELNTSfromDB(&buf ,&buflen, "Street");
          ppackLELNTSfromDB(&buf ,&buflen, "Cellular");
          ppackLELNTSfromDB(&buf ,&buflen, "ZIP");
          ppackLEWord(&buf ,&buflen, (WORD)ICQGetContactSettingWord(NULL, "Country", 0));
          ppackByte(&buf ,&buflen, (BYTE)ICQGetContactSettingByte(NULL, "Timezone", 0));
          ppackByte(&buf ,&buflen, (BYTE)!ICQGetContactSettingByte(NULL, "PublishPrimaryEmail", 0));
          *(PWORD)buf = buflen-2;
          {
            IcqChangeInfo(ICQCHANGEINFO_MAIN, (LPARAM)buf);
          }
          
          buflen=2;
          
          if (ICQGetContactSettingByte(NULL, "Auth", 1) == 0)
            ppackByte(&buf, &buflen, (BYTE)1);
          else
            ppackByte(&buf, &buflen, (BYTE)0);
          ppackByte(&buf, &buflen, (BYTE)ICQGetContactSettingByte(NULL, "WebAware", 0));
          ppackByte(&buf, &buflen, (BYTE)ICQGetContactSettingByte(NULL, "DCType", 0));
          ppackByte(&buf, &buflen, 0); // User type?
          *(PWORD)buf = buflen-2;
          {
            IcqChangeInfo(ICQCHANGEINFO_SECURITY, (LPARAM)buf);
          }
          SAFE_FREE(&buf);
          
          
          // Send a status packet to notify the server about the webaware setting
          {
            WORD wStatus;
            
            wStatus = MirandaStatusToIcq(gnCurrentStatus);
            
            if (gnCurrentStatus == ID_STATUS_INVISIBLE) 
            {
              // Tell who is on our visible list
              icq_sendEntireVisInvisList(0);
              if (gbSsiEnabled)
                updateServVisibilityCode(3);
              icq_setstatus(wStatus);
            }
            else
            {
              // Tell who is on our invisible list
              icq_sendEntireVisInvisList(1);
              icq_setstatus(wStatus);
              if (gbSsiEnabled)
                updateServVisibilityCode(4);
            }
          }
        }
        return TRUE;
      }
      break;
  }
  
  return FALSE;  
}

static HWND hCpCombo;

struct CPTABLE {
  WORD cpId;
  char *cpName;
};

struct CPTABLE cpTable[] = {
  {  874,  "Thai" },
  {  932,  "Japanese" },
  {  936,  "Simplified Chinese" },
  {  949,  "Korean" },
  {  950,  "Traditional Chinese" },
  {  1250,  "Central European" },
  {  1251,  "Cyrillic" },
  {  1252,  "Latin I" },
  {  1253,  "Greek" },
  {  1254,  "Turkish" },
  {  1255,  "Hebrew" },
  {  1256,  "Arabic" },
  {  1257,  "Baltic" },
  {  1258,  "Vietnamese" },
  {  1361,  "Korean (Johab)" },
  {   -1, NULL}
};

static BOOL CALLBACK FillCpCombo(LPCSTR str)
{
  int i;
  UINT cp;

  cp = atoi(str);
  for (i=0; cpTable[i].cpName != NULL && cpTable[i].cpId!=cp; i++);
  if (cpTable[i].cpName != NULL) 
  {
    ComboBoxAddStringUtf(hCpCombo, cpTable[i].cpName, cpTable[i].cpId);
  }
  return TRUE;
}


static const UINT icqUnicodeControls[] = {IDC_UTFALL,IDC_UTFSTATIC,IDC_UTFCODEPAGE};
static const UINT icqDCMsgControls[] = {IDC_DCPASSIVE};
static const UINT icqXStatusControls[] = {IDC_XSTATUSAUTO};
static const UINT icqAimControls[] = {IDC_AIMENABLE,IDC_AIMSTATIC};
static BOOL CALLBACK DlgProcIcqFeaturesOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
    {
      BYTE bData;
      int sCodePage;
      int i;

      ICQTranslateDialog(hwndDlg);
      bData = ICQGetContactSettingByte(NULL, "UtfEnabled", DEFAULT_UTF_ENABLED);
      CheckDlgButton(hwndDlg, IDC_UTFENABLE, bData?TRUE:FALSE);
      CheckDlgButton(hwndDlg, IDC_UTFALL, bData==2?TRUE:FALSE);
      icq_EnableMultipleControls(hwndDlg, icqUnicodeControls, sizeof(icqUnicodeControls)/sizeof(icqUnicodeControls[0]), bData?TRUE:FALSE);
      LoadDBCheckState(hwndDlg, IDC_TEMPVISIBLE, "TempVisListEnabled",DEFAULT_TEMPVIS_ENABLED);
      bData = ICQGetContactSettingByte(NULL, "DirectMessaging", DEFAULT_DCMSG_ENABLED);
      CheckDlgButton(hwndDlg, IDC_DCENABLE, bData?TRUE:FALSE);
      CheckDlgButton(hwndDlg, IDC_DCPASSIVE, bData==1?TRUE:FALSE);
      icq_EnableMultipleControls(hwndDlg, icqDCMsgControls, sizeof(icqDCMsgControls)/sizeof(icqDCMsgControls[0]), bData?TRUE:FALSE);
      bData = ICQGetContactSettingByte(NULL, "XStatusEnabled", DEFAULT_XSTATUS_ENABLED);
      CheckDlgButton(hwndDlg, IDC_XSTATUSENABLE, bData);
      icq_EnableMultipleControls(hwndDlg, icqXStatusControls, sizeof(icqXStatusControls)/sizeof(icqXStatusControls[0]), bData);
      LoadDBCheckState(hwndDlg, IDC_XSTATUSAUTO, "XStatusAuto", DEFAULT_XSTATUS_AUTO);
      LoadDBCheckState(hwndDlg, IDC_KILLSPAMBOTS, "KillSpambots", DEFAULT_KILLSPAM_ENABLED);
      LoadDBCheckState(hwndDlg, IDC_AIMENABLE, "AimEnabled", DEFAULT_AIM_ENABLED);
      icq_EnableMultipleControls(hwndDlg, icqAimControls, sizeof(icqAimControls)/sizeof(icqAimControls[0]), icqOnline?FALSE:TRUE);

      hCpCombo = GetDlgItem(hwndDlg, IDC_UTFCODEPAGE);
      sCodePage = ICQGetContactSettingWord(NULL, "AnsiCodePage", CP_ACP);
      ComboBoxAddStringUtf(GetDlgItem(hwndDlg, IDC_UTFCODEPAGE), "System default codepage", 0);
      EnumSystemCodePagesA(FillCpCombo, CP_INSTALLED);
      if(sCodePage == 0)
        SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_SETCURSEL, (WPARAM)0, 0);
      else 
      {
        for (i = 0; i < SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_GETCOUNT, 0, 0); i++) 
        {
          if (SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_GETITEMDATA, (WPARAM)i, 0) == sCodePage)
          {
            SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_SETCURSEL, (WPARAM)i, 0);
            break;
          }
        }
      }

      return TRUE;
    }

  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDC_UTFENABLE:
      icq_EnableMultipleControls(hwndDlg, icqUnicodeControls, sizeof(icqUnicodeControls)/sizeof(icqUnicodeControls[0]), IsDlgButtonChecked(hwndDlg, IDC_UTFENABLE));
      break;
    case IDC_DCENABLE:
      icq_EnableMultipleControls(hwndDlg, icqDCMsgControls, sizeof(icqDCMsgControls)/sizeof(icqDCMsgControls[0]), IsDlgButtonChecked(hwndDlg, IDC_DCENABLE));
      break;
    case IDC_XSTATUSENABLE:
      icq_EnableMultipleControls(hwndDlg, icqXStatusControls, sizeof(icqXStatusControls)/sizeof(icqXStatusControls[0]), IsDlgButtonChecked(hwndDlg, IDC_XSTATUSENABLE));
      break;
    }
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
    break;

  case WM_NOTIFY:
    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_APPLY:
      if (IsDlgButtonChecked(hwndDlg, IDC_UTFENABLE))
        gbUtfEnabled = IsDlgButtonChecked(hwndDlg, IDC_UTFALL)?2:1;
      else
        gbUtfEnabled = 0;
      {
        int i = SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_GETCURSEL, 0, 0);
        gwAnsiCodepage = (WORD)SendDlgItemMessage(hwndDlg, IDC_UTFCODEPAGE, CB_GETITEMDATA, (WPARAM)i, 0);
        ICQWriteContactSettingWord(NULL, "AnsiCodePage", gwAnsiCodepage);
      }
      ICQWriteContactSettingByte(NULL, "UtfEnabled", gbUtfEnabled);
      gbTempVisListEnabled = (BYTE)IsDlgButtonChecked(hwndDlg, IDC_TEMPVISIBLE);
      ICQWriteContactSettingByte(NULL, "TempVisListEnabled", gbTempVisListEnabled);
      if (IsDlgButtonChecked(hwndDlg, IDC_DCENABLE))
        gbDCMsgEnabled = IsDlgButtonChecked(hwndDlg, IDC_DCPASSIVE)?1:2;
      else
        gbDCMsgEnabled = 0;
      ICQWriteContactSettingByte(NULL, "DirectMessaging", gbDCMsgEnabled);
      gbXStatusEnabled = (BYTE)IsDlgButtonChecked(hwndDlg, IDC_XSTATUSENABLE);
      ICQWriteContactSettingByte(NULL, "XStatusEnabled", gbXStatusEnabled);
      StoreDBCheckState(hwndDlg, IDC_XSTATUSAUTO, "XStatusAuto");
      StoreDBCheckState(hwndDlg, IDC_KILLSPAMBOTS , "KillSpambots");
      StoreDBCheckState(hwndDlg, IDC_AIMENABLE, "AimEnabled");
      return TRUE;
    }
    break;
  }
  return FALSE;
}



static const UINT icqContactsControls[] = {IDC_ADDSERVER,IDC_LOADFROMSERVER,IDC_SAVETOSERVER,IDC_UPLOADNOW};
static const UINT icqAvatarControls[] = {IDC_AUTOLOADAVATARS,IDC_LINKAVATARS};
static BOOL CALLBACK DlgProcIcqContactsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
    ICQTranslateDialog(hwndDlg);
    LoadDBCheckState(hwndDlg, IDC_ENABLE, "UseServerCList", DEFAULT_SS_ENABLED);
    LoadDBCheckState(hwndDlg, IDC_ADDSERVER, "ServerAddRemove", DEFAULT_SS_ADDSERVER);
    LoadDBCheckState(hwndDlg, IDC_LOADFROMSERVER, "LoadServerDetails", DEFAULT_SS_LOAD);
    LoadDBCheckState(hwndDlg, IDC_SAVETOSERVER, "StoreServerDetails", DEFAULT_SS_STORE);
    LoadDBCheckState(hwndDlg, IDC_ENABLEAVATARS, "AvatarsEnabled", DEFAULT_AVATARS_ENABLED);
    LoadDBCheckState(hwndDlg, IDC_AUTOLOADAVATARS, "AvatarsAutoLoad", DEFAULT_LOAD_AVATARS);
    LoadDBCheckState(hwndDlg, IDC_LINKAVATARS, "AvatarsAutoLink", DEFAULT_LINK_AVATARS);

    icq_EnableMultipleControls(hwndDlg, icqContactsControls, sizeof(icqContactsControls)/sizeof(icqContactsControls[0]), 
      ICQGetContactSettingByte(NULL, "UseServerCList", DEFAULT_SS_ENABLED)?TRUE:FALSE);
    icq_EnableMultipleControls(hwndDlg, icqAvatarControls, sizeof(icqAvatarControls)/sizeof(icqAvatarControls[0]), 
      ICQGetContactSettingByte(NULL, "AvatarsEnabled", DEFAULT_AVATARS_ENABLED)?TRUE:FALSE);

    if (icqOnline)
    {
      ShowWindow(GetDlgItem(hwndDlg, IDC_OFFLINETOENABLE), SW_SHOW);
      EnableDlgItem(hwndDlg, IDC_ENABLE, FALSE);
      EnableDlgItem(hwndDlg, IDC_ENABLEAVATARS, FALSE);
    }
    else
    {
      EnableDlgItem(hwndDlg, IDC_UPLOADNOW, FALSE);
    }
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDC_UPLOADNOW:
      ShowUploadContactsDialog();
      return TRUE;
    case IDC_ENABLE:
      icq_EnableMultipleControls(hwndDlg, icqContactsControls, sizeof(icqContactsControls)/sizeof(icqContactsControls[0]), IsDlgButtonChecked(hwndDlg, IDC_ENABLE));
      if (icqOnline) 
        ShowWindow(GetDlgItem(hwndDlg, IDC_RECONNECTREQD), SW_SHOW);
      else 
        EnableDlgItem(hwndDlg, IDC_UPLOADNOW, FALSE);
      break;
    case IDC_ENABLEAVATARS:
      icq_EnableMultipleControls(hwndDlg, icqAvatarControls, sizeof(icqAvatarControls)/sizeof(icqAvatarControls[0]), IsDlgButtonChecked(hwndDlg, IDC_ENABLEAVATARS));
      break;
    }
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
    break;

  case WM_NOTIFY:
    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_APPLY:
      StoreDBCheckState(hwndDlg, IDC_ENABLE, "UseServerCList");
      StoreDBCheckState(hwndDlg, IDC_ADDSERVER, "ServerAddRemove");
      StoreDBCheckState(hwndDlg, IDC_LOADFROMSERVER, "LoadServerDetails");
      StoreDBCheckState(hwndDlg, IDC_SAVETOSERVER, "StoreServerDetails");
      StoreDBCheckState(hwndDlg, IDC_ENABLEAVATARS, "AvatarsEnabled");
      StoreDBCheckState(hwndDlg, IDC_AUTOLOADAVATARS, "AvatarsAutoLoad");
      StoreDBCheckState(hwndDlg, IDC_LINKAVATARS, "AvatarsAutoLink");

      return TRUE;
    }
    break;
  }
  return FALSE;
}
