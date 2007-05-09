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

#include "m_flash.h"

// icqj magic id
#define FA_MAGIC_ID       0x4943516A 


extern WORD wListenPort;

extern char* calcMD5Hash(char* szFile);
extern char* MirandaVersionToString(char* szStr, int bUnicode, int v, int m);

extern char* nameXStatus[29];

static BOOL CALLBACK IcqDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK AvatarDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
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

  if (((lParam != 0) && gbAvatarsEnabled) || (gbSsiEnabled && gbAvatarsEnabled && !ServiceExists(MS_AV_GETMYAVATAR)))
  {
    DWORD dwUin;
    uid_str dwUid;

    if (!ICQGetContactSettingUID((HANDLE)lParam, &dwUin, &dwUid))
    { // Avatar page only for valid contacts
      char *szAvtTitle;

      odp.pfnDlgProc = AvatarDlgProc;
      odp.position = -1899999998;
      odp.pszTemplate = MAKEINTRESOURCE(IDD_INFO_AVATAR);
      if (lParam)
        szAvtTitle = "Avatar";
      else
        szAvtTitle = "%s Avatar";

      AddUserInfoPageUtf(&odp, wParam, szAvtTitle);
    }
  }

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
                SetDlgItemTextUtf(hwndDlg, IDC_SUPTIME, ICQTranslateUtfStatic("Member since:", str));
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

typedef struct AvtDlgProcData_t
{
  HANDLE hContact;
  HANDLE hEventHook;
  HWND hFlashAvatar;
  HBITMAP hImageAvatar;
} AvtDlgProcData;

#define HM_REBIND_AVATAR  (WM_USER + 1024)

static char* ChooseAvatarFileName()
{
  char* szDest = (char*)SAFE_MALLOC(MAX_PATH+0x10);
  char str[MAX_PATH];
  char szFilter[512];
  OPENFILENAME ofn = {0};

  str[0] = 0;
  CallService(MS_UTILS_GETBITMAPFILTERSTRINGS,sizeof(szFilter),(LPARAM)szFilter);
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.lpstrFilter = szFilter;
  ofn.lpstrFile = szDest;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.nMaxFile = MAX_PATH;
  ofn.nMaxFileTitle = MAX_PATH;
  ofn.lpstrDefExt = "jpg";
  if (!GetOpenFileName(&ofn))
  {
    SAFE_FREE(&szDest);
    return NULL;
  }

  return szDest;
}



static void PrepareFlashAvatar(HWND hwndDlg, AvtDlgProcData* pData)
{
  FLASHAVATAR fa = {0};

  fa.hContact = pData->hContact;
  fa.cProto = gpszICQProtoName;
  fa.id = FA_MAGIC_ID;
  CallService(MS_FAVATAR_GETINFO, (WPARAM)&fa, 0);
  if (fa.hWindow)
  {
    pData->hFlashAvatar = fa.hWindow;
    SetParent(fa.hWindow, GetDlgItem(hwndDlg, IDC_AVATAR));
  }
  else
  {
    fa.hParentWindow = GetDlgItem(hwndDlg, IDC_AVATAR);
    CallService(MS_FAVATAR_MAKE, (WPARAM)&fa, 0);
    if (fa.hWindow)
      pData->hFlashAvatar = fa.hWindow;
  }
}



static void ReleaseFlashAvatar(AvtDlgProcData* pData)
{
  if (pData->hFlashAvatar)
  { // release expired flash avatar object
    FLASHAVATAR fa;

    fa.hContact = pData->hContact;
    fa.cProto = gpszICQProtoName;
    fa.id = FA_MAGIC_ID; // icqj magic id
    CallService(MS_FAVATAR_DESTROY, (WPARAM)&fa, 0);
    pData->hFlashAvatar = NULL;
  }
}



static void PrepareImageAvatar(HWND hwndDlg, AvtDlgProcData* pData, char* szFile)
{
  pData->hImageAvatar = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (WPARAM)szFile);

  SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (WPARAM)pData->hImageAvatar);
}



static void ReleaseImageAvatar(HWND hwndDlg, AvtDlgProcData* pData)
{
  if (pData->hImageAvatar)
  {
    HBITMAP avt = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);

    // force re-draw avatar window
    InvalidateRect(GetDlgItem(hwndDlg, IDC_AVATAR), NULL, FALSE);

    // in XP you can get different image, and it is leaked if not Destroy()ed
    if (avt != pData->hImageAvatar)
      DeleteObject(avt);

    DeleteObject(pData->hImageAvatar);
    pData->hImageAvatar = NULL;
  }
}



static BOOL CALLBACK AvatarDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
    ICQTranslateDialog(hwndDlg);
    {
      DBVARIANT dbvHash;
      AvtDlgProcData* pData = (AvtDlgProcData*)SAFE_MALLOC(sizeof(AvtDlgProcData));
      DWORD dwUIN;
      uid_str szUID;
      char szAvatar[MAX_PATH];
      DWORD dwPaFormat;
      int bValid = 0;

      pData->hContact = (HANDLE)lParam;

      if (pData->hContact)
        pData->hEventHook = HookEventMessage(ME_PROTO_ACK, hwndDlg, HM_REBIND_AVATAR);
      else
      { 
        ShowWindow(GetDlgItem(hwndDlg, IDC_SETAVATAR), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_DELETEAVATAR), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, -1), SW_SHOW);
        if (!icqOnline)
        {
          EnableDlgItem(hwndDlg, IDC_SETAVATAR, FALSE);
          EnableDlgItem(hwndDlg, IDC_DELETEAVATAR, FALSE);
        }
      }
      SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)pData);

      if (!ICQGetContactSettingUID((HANDLE)lParam, &dwUIN, &szUID))
      {
        if (!ICQGetContactSetting((HANDLE)lParam, "AvatarHash", &dbvHash) && dbvHash.type == DBVT_BLOB)
        {
          dwPaFormat = ICQGetContactSettingByte((HANDLE)lParam, "AvatarType", PA_FORMAT_UNKNOWN);
          if (!pData->hContact || (dwPaFormat != PA_FORMAT_UNKNOWN))
          { // we do not know avatar format, so neither filename is known, not valid
            if (pData->hContact)
              GetFullAvatarFileName(dwUIN, szUID, dwPaFormat, szAvatar, 255);
            else
            { // owner's avatar
              char* file = loadMyAvatarFileName();

              if (file)
              {
                strcpy(szAvatar, file);
                SAFE_FREE(&file);
                if (dbvHash.pbVal[1] == AVATAR_HASH_FLASH) // we do this by hand, as owner's format is not saved
                  dwPaFormat = PA_FORMAT_XML;
              }
              else
                szAvatar[0] = '\0';
            }

            if (!pData->hContact || !IsAvatarSaved((HANDLE)lParam, dbvHash.pbVal, dbvHash.cpbVal))
            { // if the file exists, we know we have the current avatar
              if (!access(szAvatar, 0)) bValid = 1;
            }
          }
        }
        else
          return TRUE;

        if (bValid)
        { //load avatar
          if ((dwPaFormat == PA_FORMAT_XML || dwPaFormat == PA_FORMAT_SWF) && ServiceExists(MS_FAVATAR_GETINFO))
          { // flash avatar and we have FlashAvatars installed, init flash avatar
            PrepareFlashAvatar(hwndDlg, pData);
          }
          else
          { // static avatar processing
            PrepareImageAvatar(hwndDlg, pData, szAvatar);
          }
        }
        else if (pData->hContact) // only retrieve users avatars
        {
          GetAvatarFileName(dwUIN, szUID, szAvatar, 255);
          GetAvatarData((HANDLE)lParam, dwUIN, szUID, dbvHash.pbVal, dbvHash.cpbVal, szAvatar);
        }
      }

      ICQFreeVariant(&dbvHash);
    }
    return TRUE;
  
  case HM_REBIND_AVATAR:
    {  
      AvtDlgProcData* pData = (AvtDlgProcData*)GetWindowLong(hwndDlg, GWL_USERDATA);
      ACKDATA* ack = (ACKDATA*)lParam;

      if (!pData->hContact) break; // we do not use this for us

      if (ack->type == ACKTYPE_AVATAR && ack->hContact == pData->hContact)
      {
        if (ack->result == ACKRESULT_SUCCESS)
        { // load avatar
          PROTO_AVATAR_INFORMATION* AI = (PROTO_AVATAR_INFORMATION*)ack->hProcess;

          ReleaseFlashAvatar(pData);
          ReleaseImageAvatar(hwndDlg, pData);

          if ((AI->format == PA_FORMAT_XML || AI->format == PA_FORMAT_SWF) && ServiceExists(MS_FAVATAR_GETINFO))
          { // flash avatar and we have FlashAvatars installed, init flash avatar
            PrepareFlashAvatar(hwndDlg, pData);
          }
          else
          { // process image avatar
            PrepareImageAvatar(hwndDlg, pData, AI->filename);
          }
        }
        else if (ack->result == ACKRESULT_STATUS)
        { // contact has changed avatar
          DWORD dwUIN;
          uid_str szUID;
          DBVARIANT dbvHash;
          
          if (!ICQGetContactSettingUID(pData->hContact, &dwUIN, &szUID))
          {
            if (!ICQGetContactSetting(pData->hContact, "AvatarHash", &dbvHash))
            {
              char szAvatar[MAX_PATH];

              GetAvatarFileName(dwUIN, szUID, szAvatar, 255);
              GetAvatarData(pData->hContact, dwUIN, szUID, dbvHash.pbVal, 0x14, szAvatar);
            }
            ICQFreeVariant(&dbvHash);
          }
        }
      }  
    }
    break;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDC_SETAVATAR:
      {
        char* szFile;
        AvtDlgProcData* pData = (AvtDlgProcData*)GetWindowLong(hwndDlg, GWL_USERDATA);

        if (szFile = ChooseAvatarFileName())
        { // user selected file for his avatar
          DWORD dwPaFormat = DetectAvatarFormat(szFile);

          if (dwPaFormat == PA_FORMAT_XML || dwPaFormat == PA_FORMAT_JPEG || dwPaFormat == PA_FORMAT_GIF || dwPaFormat == PA_FORMAT_BMP)
          { // a valid file
            if (!IcqSetMyAvatar(0, (LPARAM)szFile)) // set avatar
            {
              ReleaseFlashAvatar(pData);
              ReleaseImageAvatar(hwndDlg, pData);

              if (dwPaFormat != PA_FORMAT_XML || !ServiceExists(MS_FAVATAR_GETINFO))
              { // it is not flash
                PrepareImageAvatar(hwndDlg, pData, szFile);
              }
              else
              { // is is flash load it
                PrepareFlashAvatar(hwndDlg, pData);
              }
            }
          }
          SAFE_FREE(&szFile);
        }
      }
      break;
    case IDC_DELETEAVATAR:
      {
        AvtDlgProcData* pData = (AvtDlgProcData*)GetWindowLong(hwndDlg, GWL_USERDATA);

        ReleaseFlashAvatar(pData);
        ReleaseImageAvatar(hwndDlg, pData);

        IcqSetMyAvatar(0, 0); // clear hash on server
      }
      break;
    }
    break;

  case WM_DESTROY:
    {
      AvtDlgProcData* pData = (AvtDlgProcData*)GetWindowLong(hwndDlg, GWL_USERDATA);
      if (pData->hContact)
        UnhookEvent(pData->hEventHook);
        
      ReleaseFlashAvatar(pData);
      ReleaseImageAvatar(hwndDlg, pData);
      SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)0);
      SAFE_FREE(&pData);
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
            ICQTranslateUtfStatic(" (DC Established)", szExtra);
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
      unspecified = (special == SVS_ZEROISUNSPEC && dbv.pszVal[0] == '\0');
      if (!unspecified && pstr != szSetting)
      {
        pstr = UniGetContactSettingUtf(hContact, szModule, szSetting, NULL);
        bUtf = 1;
        bAlloc = 1;
      }
      if (idCtrl == IDC_UIN)
        SetDlgItemTextUtf(hwndDlg, IDC_UINSTATIC, ICQTranslateUtfStatic("ScreenName:", str));
      break;
      
    default:
      pstr = str;
      lstrcpy(str,"???");
      break;
    }
  }
  
  EnableDlgItem(hwndDlg, idCtrl, !unspecified);
  if (unspecified)
    SetDlgItemTextUtf(hwndDlg, idCtrl, ICQTranslateUtfStatic("<not specified>", str));
  else if (bUtf)
    SetDlgItemTextUtf(hwndDlg, idCtrl, pstr);
  else
    SetDlgItemText(hwndDlg, idCtrl, pstr);
  
  if (bDbv)
    ICQFreeVariant(&dbv);

  if (bAlloc)
    SAFE_FREE(&pstr);
}
