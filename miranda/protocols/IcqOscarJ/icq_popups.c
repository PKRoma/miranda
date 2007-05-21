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
//  PopUp Plugin stuff
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

static BOOL bPopUpService = FALSE;

static BOOL CALLBACK DlgProcIcqPopupOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


void InitPopUps()
{
  if (ServiceExists(MS_POPUP_ADDPOPUPEX))
  {
    bPopUpService = TRUE;
  }
}



void InitPopupOpts(WPARAM wParam)
{
  OPTIONSDIALOGPAGE odp = {0};

  if (bPopUpService)
  {
    odp.cbSize = sizeof(odp);
    odp.position = 100000000;
    odp.hInstance = hInst;
    odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_POPUPS);
    odp.groupPosition = 900000000;
    odp.pfnDlgProc = DlgProcIcqPopupOpts;
    odp.flags = ODPF_BOLDGROUPS;
    odp.nIDBottomSimpleControl = 0;
    AddOptionsPageUtf(&odp, wParam, "Popups", gpszICQProtoName);
  }
}


static const UINT icqPopupsControls[] = {IDC_POPUPS_LOG_ENABLED,IDC_POPUPS_SPAM_ENABLED,IDC_PREVIEW,IDC_USEWINCOLORS,IDC_USESYSICONS,IDC_POPUP_LOG0_TIMEOUT,IDC_POPUP_LOG1_TIMEOUT,IDC_POPUP_LOG2_TIMEOUT,IDC_POPUP_LOG3_TIMEOUT,IDC_POPUP_SPAM_TIMEOUT};
static const UINT icqPopupColorControls[] = {IDC_POPUP_LOG0_TEXTCOLOR,IDC_POPUP_LOG1_TEXTCOLOR,IDC_POPUP_LOG2_TEXTCOLOR,IDC_POPUP_LOG3_TEXTCOLOR,IDC_POPUP_SPAM_TEXTCOLOR,
                                             IDC_POPUP_LOG0_BACKCOLOR,IDC_POPUP_LOG1_BACKCOLOR,IDC_POPUP_LOG2_BACKCOLOR,IDC_POPUP_LOG3_BACKCOLOR,IDC_POPUP_SPAM_BACKCOLOR};
static BOOL CALLBACK DlgProcIcqPopupOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  BYTE bEnabled;

  switch (msg)
  {
  case WM_INITDIALOG:
    ICQTranslateDialog(hwndDlg);
    CheckDlgButton(hwndDlg, IDC_POPUPS_LOG_ENABLED, ICQGetContactSettingByte(NULL,"PopupsLogEnabled",DEFAULT_LOG_POPUPS_ENABLED));
    CheckDlgButton(hwndDlg, IDC_POPUPS_SPAM_ENABLED, ICQGetContactSettingByte(NULL,"PopupsSpamEnabled",DEFAULT_SPAM_POPUPS_ENABLED));
    SendDlgItemMessage(hwndDlg, IDC_POPUP_LOG0_TEXTCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"Popups0TextColor",DEFAULT_LOG0_TEXT_COLORS));
    SendDlgItemMessage(hwndDlg, IDC_POPUP_LOG0_BACKCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"Popups0BackColor",DEFAULT_LOG0_BACK_COLORS));
    SetDlgItemInt(hwndDlg, IDC_POPUP_LOG0_TIMEOUT, ICQGetContactSettingDword(NULL,"Popups0Timeout",DEFAULT_LOG0_TIMEOUT),FALSE);
    SendDlgItemMessage(hwndDlg, IDC_POPUP_LOG1_TEXTCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"Popups1TextColor",DEFAULT_LOG1_TEXT_COLORS));
    SendDlgItemMessage(hwndDlg, IDC_POPUP_LOG1_BACKCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"Popups1BackColor",DEFAULT_LOG1_BACK_COLORS));
    SetDlgItemInt(hwndDlg, IDC_POPUP_LOG1_TIMEOUT, ICQGetContactSettingDword(NULL,"Popups1Timeout",DEFAULT_LOG1_TIMEOUT),FALSE);
    SendDlgItemMessage(hwndDlg, IDC_POPUP_LOG2_TEXTCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"Popups2TextColor",DEFAULT_LOG2_TEXT_COLORS));
    SendDlgItemMessage(hwndDlg, IDC_POPUP_LOG2_BACKCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"Popups2BackColor",DEFAULT_LOG2_BACK_COLORS));
    SetDlgItemInt(hwndDlg, IDC_POPUP_LOG2_TIMEOUT, ICQGetContactSettingDword(NULL,"Popups2Timeout",DEFAULT_LOG2_TIMEOUT),FALSE);
    SendDlgItemMessage(hwndDlg, IDC_POPUP_LOG3_TEXTCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"Popups3TextColor",DEFAULT_LOG3_TEXT_COLORS));
    SendDlgItemMessage(hwndDlg, IDC_POPUP_LOG3_BACKCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"Popups3BackColor",DEFAULT_LOG3_BACK_COLORS));
    SetDlgItemInt(hwndDlg, IDC_POPUP_LOG3_TIMEOUT, ICQGetContactSettingDword(NULL,"Popups3Timeout",DEFAULT_LOG3_TIMEOUT),FALSE);
    SendDlgItemMessage(hwndDlg, IDC_POPUP_SPAM_TEXTCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"PopupsSpamTextColor",DEFAULT_SPAM_TEXT_COLORS));
    SendDlgItemMessage(hwndDlg, IDC_POPUP_SPAM_BACKCOLOR, CPM_SETCOLOUR, 0, ICQGetContactSettingDword(NULL,"PopupsSpamBackColor",DEFAULT_SPAM_BACK_COLORS));
    SetDlgItemInt(hwndDlg, IDC_POPUP_SPAM_TIMEOUT, ICQGetContactSettingDword(NULL,"PopupsSpamTimeout",DEFAULT_SPAM_TIMEOUT),FALSE);
    bEnabled = ICQGetContactSettingByte(NULL,"PopupsWinColors",DEFAULT_POPUPS_WIN_COLORS);
    CheckDlgButton(hwndDlg, IDC_USEWINCOLORS, bEnabled);
    icq_EnableMultipleControls(hwndDlg, icqPopupColorControls, SIZEOF(icqPopupColorControls), bEnabled);
    CheckDlgButton(hwndDlg, IDC_USESYSICONS, ICQGetContactSettingByte(NULL,"PopupsSysIcons",DEFAULT_POPUPS_SYS_ICONS));
    bEnabled = ICQGetContactSettingByte(NULL,"PopupsEnabled",DEFAULT_POPUPS_ENABLED);
    CheckDlgButton(hwndDlg, IDC_POPUPS_ENABLED, bEnabled);
    icq_EnableMultipleControls(hwndDlg, icqPopupsControls, SIZEOF(icqPopupsControls), bEnabled);
    icq_EnableMultipleControls(hwndDlg, icqPopupColorControls, SIZEOF(icqPopupColorControls), bEnabled & !IsDlgButtonChecked(hwndDlg,IDC_USEWINCOLORS));

    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDC_PREVIEW:
      {
        ShowPopUpMsg(NULL, "Popup Title", "Sample Note",    LOG_NOTE);
        ShowPopUpMsg(NULL, "Popup Title", "Sample Warning", LOG_WARNING);
        ShowPopUpMsg(NULL, "Popup Title", "Sample Error",   LOG_ERROR);
        ShowPopUpMsg(NULL, "Popup Title", "Sample Fatal",   LOG_FATAL);
        ShowPopUpMsg(NULL, "Popup Title", "Sample Spambot", POPTYPE_SPAM);
      }
      return FALSE;

    case IDC_POPUPS_ENABLED:
      bEnabled = IsDlgButtonChecked(hwndDlg,IDC_POPUPS_ENABLED);
      icq_EnableMultipleControls(hwndDlg, icqPopupsControls, SIZEOF(icqPopupsControls), bEnabled);

    case IDC_USEWINCOLORS:
      bEnabled = IsDlgButtonChecked(hwndDlg,IDC_POPUPS_ENABLED);
      icq_EnableMultipleControls(hwndDlg, icqPopupColorControls, SIZEOF(icqPopupColorControls), bEnabled & !IsDlgButtonChecked(hwndDlg,IDC_USEWINCOLORS));
      break;
    }
    SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
    break;

  case WM_NOTIFY:
    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_APPLY:
      ICQWriteContactSettingByte(NULL,"PopupsEnabled",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_POPUPS_ENABLED));
      ICQWriteContactSettingByte(NULL,"PopupsLogEnabled",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_POPUPS_LOG_ENABLED));
      ICQWriteContactSettingByte(NULL,"PopupsSpamEnabled",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_POPUPS_SPAM_ENABLED));
      ICQWriteContactSettingDword(NULL,"Popups0TextColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_LOG0_TEXTCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"Popups0BackColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_LOG0_BACKCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"Popups0Timeout",GetDlgItemInt(hwndDlg, IDC_POPUP_LOG0_TIMEOUT, NULL, FALSE));
      ICQWriteContactSettingDword(NULL,"Popups1TextColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_LOG1_TEXTCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"Popups1BackColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_LOG1_BACKCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"Popups1Timeout",GetDlgItemInt(hwndDlg, IDC_POPUP_LOG1_TIMEOUT, NULL, FALSE));
      ICQWriteContactSettingDword(NULL,"Popups2TextColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_LOG2_TEXTCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"Popups2BackColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_LOG2_BACKCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"Popups2Timeout",GetDlgItemInt(hwndDlg, IDC_POPUP_LOG2_TIMEOUT, NULL, FALSE));
      ICQWriteContactSettingDword(NULL,"Popups3TextColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_LOG3_TEXTCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"Popups3BackColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_LOG3_BACKCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"Popups3Timeout",GetDlgItemInt(hwndDlg, IDC_POPUP_LOG3_TIMEOUT, NULL, FALSE));
      ICQWriteContactSettingDword(NULL,"PopupsSpamTextColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_SPAM_TEXTCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"PopupsSpamBackColor",SendDlgItemMessage(hwndDlg,IDC_POPUP_SPAM_BACKCOLOR,CPM_GETCOLOUR,0,0));
      ICQWriteContactSettingDword(NULL,"PopupsSpamTimeout",GetDlgItemInt(hwndDlg, IDC_POPUP_SPAM_TIMEOUT, NULL, FALSE));
      ICQWriteContactSettingByte(NULL,"PopupsWinColors",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_USEWINCOLORS));
      ICQWriteContactSettingByte(NULL,"PopupsSysIcons",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_USESYSICONS));

      return TRUE;
    }
    break;
  }
  return FALSE;
}



int ShowPopUpMsg(HANDLE hContact, const char* szTitle, const char* szMsg, BYTE bType)
{
  if (bPopUpService && ICQGetContactSettingByte(NULL, "PopupsEnabled", DEFAULT_POPUPS_ENABLED))
  {
    POPUPDATAEX ppd = {0};
    POPUPDATAW ppdw = {0};
    LPCTSTR rsIcon;
    HINSTANCE hIcons = NULL;
    char szPrefix[32], szSetting[32];

    strcpy(szPrefix, "Popups");
    ppd.iSeconds = 0;

    switch(bType) 
    {
      case LOG_NOTE:
        rsIcon = MAKEINTRESOURCE(IDI_INFORMATION);
        ppd.colorBack = DEFAULT_LOG0_BACK_COLORS;
        ppd.colorText = DEFAULT_LOG0_TEXT_COLORS;
        strcat(szPrefix, "0");
        break;

      case LOG_WARNING:
        rsIcon = MAKEINTRESOURCE(IDI_WARNING);
        ppd.colorBack = DEFAULT_LOG1_BACK_COLORS;
        ppd.colorText = DEFAULT_LOG1_TEXT_COLORS;
        strcat(szPrefix, "1");
        break;

      case LOG_ERROR:
        rsIcon = MAKEINTRESOURCE(IDI_ERROR);
        ppd.colorBack = DEFAULT_LOG2_BACK_COLORS;
        ppd.colorText = DEFAULT_LOG2_TEXT_COLORS;
        strcat(szPrefix, "2");
        break;

      case LOG_FATAL:
        rsIcon = MAKEINTRESOURCE(IDI_ERROR);
        ppd.colorBack = DEFAULT_LOG3_BACK_COLORS;
        ppd.colorText = DEFAULT_LOG3_TEXT_COLORS;
        strcat(szPrefix, "3");
        break;

      case POPTYPE_SPAM:
        rsIcon = MAKEINTRESOURCE(IDI_WARNING);
        ppd.colorBack = DEFAULT_SPAM_BACK_COLORS;
        ppd.colorText = DEFAULT_SPAM_TEXT_COLORS;
        strcat(szPrefix, "Spam");
        break;
      default:
        return -1;
    }
    if (!ICQGetContactSettingByte(NULL, "PopupsSysIcons", DEFAULT_POPUPS_SYS_ICONS))
    {
      hIcons = hInst;
      rsIcon = MAKEINTRESOURCE(IDI_ICQ);
    }
    ppd.lchIcon = (HICON)LoadImage(hIcons, rsIcon, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    if (ICQGetContactSettingByte(NULL, "PopupsWinColors", DEFAULT_POPUPS_WIN_COLORS))
    {
      ppd.colorText = GetSysColor(COLOR_WINDOWTEXT);
      ppd.colorBack = GetSysColor(COLOR_WINDOW);
    }
    else
    {
      strcpy(szSetting, szPrefix);
      strcat(szSetting, "TextColor");
      ppd.colorText = ICQGetContactSettingDword(NULL, szSetting, ppd.colorText);
      strcpy(szSetting, szPrefix);
      strcat(szSetting, "BackColor");
      ppd.colorBack = ICQGetContactSettingDword(NULL, szSetting, ppd.colorBack);
    }
    strcpy(szSetting, szPrefix);
    strcat(szSetting, "Timeout");
    ppd.iSeconds = ICQGetContactSettingDword(NULL, szSetting, ppd.iSeconds);

    if (gbUnicodeAPI && ServiceExists(MS_POPUP_ADDPOPUPW))
    { // call unicode popup module - only on unicode OS otherwise it will not work properly :(
      // due to Popup Plug bug in ADDPOPUPW implementation
      wchar_t *tmp;
      char str[MAX_SECONDLINE];

      tmp = make_unicode_string(ICQTranslateUtfStatic(szTitle, str));
      memmove(ppdw.lpwzContactName, tmp, wcslen(tmp)*sizeof(wchar_t));
      SAFE_FREE(&tmp);
      tmp = make_unicode_string(ICQTranslateUtfStatic(szMsg, str));
      memmove(ppdw.lpwzText, tmp, wcslen(tmp)*sizeof(wchar_t));
      SAFE_FREE(&tmp);
      ppdw.lchContact = hContact;
      ppdw.lchIcon = ppd.lchIcon;
      ppdw.colorBack = ppd.colorBack;
      ppdw.colorText = ppd.colorText;
      ppdw.PluginWindowProc = NULL;
      ppdw.PluginData = NULL;
      ppdw.iSeconds = ppd.iSeconds;

      return CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&ppdw, 0);
    }
    else
    {
      char str[MAX_SECONDLINE];

      utf8_decode_static(ICQTranslateUtfStatic(szTitle, str), ppd.lpzContactName, MAX_CONTACTNAME);
      utf8_decode_static(ICQTranslateUtfStatic(szMsg, str), ppd.lpzText, MAX_SECONDLINE);
      ppd.lchContact = hContact;
      ppd.PluginWindowProc = NULL;
      ppd.PluginData = NULL;

      return CallService(MS_POPUP_ADDPOPUPEX, (WPARAM)&ppd, 0);
    }
  }
  return -1; // Failure
}
