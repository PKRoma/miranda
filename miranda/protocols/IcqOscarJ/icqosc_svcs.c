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
//  High-level code for exported API services
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



int gbIdleAllow;
int icqGoingOnlineStatus;

extern WORD wListenPort;

extern char* calcMD5Hash(char* szFile);
extern filetransfer *CreateFileTransfer(HANDLE hContact, DWORD dwUin, int nVersion);


int IcqGetCaps(WPARAM wParam, LPARAM lParam)
{
  int nReturn = 0;


  switch (wParam)
  {

  case PFLAGNUM_1:
    nReturn = PF1_IM | PF1_URL | PF1_AUTHREQ | PF1_BASICSEARCH | PF1_ADDSEARCHRES |
      PF1_VISLIST | PF1_INVISLIST | PF1_MODEMSG | PF1_FILE | PF1_EXTSEARCH |
      PF1_EXTSEARCHUI | PF1_SEARCHBYEMAIL | PF1_SEARCHBYNAME |
      PF1_ADDED | PF1_CONTACT;
    if (!gbAimEnabled)
      nReturn |= PF1_NUMERICUSERID;
    if (gbSsiEnabled && ICQGetContactSettingByte(NULL, "ServerAddRemove", DEFAULT_SS_ADDSERVER))
      nReturn |= PF1_SERVERCLIST;
    break;

  case PFLAGNUM_2:
    nReturn = PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND |
      PF2_FREECHAT | PF2_INVISIBLE;
    break;

  case PFLAGNUM_3:
    nReturn = PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND |
      PF2_FREECHAT;
    break;

  case PFLAGNUM_4:
    nReturn = PF4_SUPPORTIDLE;
    if (gbAvatarsEnabled)
      nReturn |= PF4_AVATARS;
#ifdef DBG_CAPMTN
    nReturn |= PF4_SUPPORTTYPING;
#endif
    break;

  case PFLAG_UNIQUEIDTEXT:
    nReturn = (int)ICQTranslate("User ID");
    break;

  case PFLAG_UNIQUEIDSETTING:
    nReturn = (int)UNIQUEIDSETTING;
    break;

  case PFLAG_MAXCONTACTSPERPACKET:
    nReturn = MAX_CONTACTSSEND;
    break;

  case PFLAG_MAXLENOFMESSAGE:
    nReturn = MAX_MESSAGESNACSIZE-102;
  }

  return nReturn;
}



int IcqGetName(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  {
    strncpy((char *)lParam, ICQTranslate(gpszICQProtoName), wParam);

    return 0; // Success
  }

  return 1; // Failure
}



int IcqLoadIcon(WPARAM wParam, LPARAM lParam)
{
  UINT id;


  switch (wParam & 0xFFFF)
  {
    case PLI_PROTOCOL:
      id = IDI_ICQ;
      break;

    default:
      return 0; // Failure
  }

  return (int)LoadImage(hInst, MAKEINTRESOURCE(id), IMAGE_ICON, GetSystemMetrics(wParam&PLIF_SMALL?SM_CXSMICON:SM_CXICON), GetSystemMetrics(wParam&PLIF_SMALL?SM_CYSMICON:SM_CYICON), 0);
}



int IcqIdleChanged(WPARAM wParam, LPARAM lParam)
{
  int bIdle = (lParam&IDF_ISIDLE);
  int bPrivacy = (lParam&IDF_PRIVACY);

  if (bPrivacy) return 0;

  ICQWriteContactSettingDword(NULL, "IdleTS", bIdle ? time(0) : 0);

  if (gbTempVisListEnabled) // remove temporary visible users
    clearTemporaryVisibleList();

  icq_setidle(bIdle ? 1 : 0);

  return 0;
}



int IcqGetAvatarInfo(WPARAM wParam, LPARAM lParam)
{
  PROTO_AVATAR_INFORMATION* pai = (PROTO_AVATAR_INFORMATION*)lParam;
  DWORD dwUIN;
  uid_str szUID;
  DBVARIANT dbv;
  int dwPaFormat;

  if (!gbAvatarsEnabled) return GAIR_NOAVATAR;

  if (ICQGetContactSetting(pai->hContact, "AvatarHash", &dbv) || dbv.cpbVal != 0x14)
    return GAIR_NOAVATAR; // we did not found avatar hash or hash invalid - no avatar available

  if (ICQGetContactSettingUID(pai->hContact, &dwUIN, &szUID))
  {
    ICQFreeVariant(&dbv);

    return GAIR_NOAVATAR; // we do not support avatars for invalid contacts
  }

  dwPaFormat = ICQGetContactSettingByte(pai->hContact, "AvatarType", PA_FORMAT_UNKNOWN);
  if (dwPaFormat != PA_FORMAT_UNKNOWN)
  { // we know the format, test file
    GetFullAvatarFileName(dwUIN, szUID, dwPaFormat, pai->filename, MAX_PATH);

    pai->format = dwPaFormat; 

    if (!IsAvatarSaved(pai->hContact, dbv.pbVal))
    { // hashes are the same
      if (access(pai->filename, 0) == 0)
      {
        ICQFreeVariant(&dbv);

        return GAIR_SUCCESS; // we have found the avatar file, whoala
      }
    }
  }

  if (IsAvatarSaved(pai->hContact, dbv.pbVal))
  { // we didn't received the avatar before - this ensures we will not request avatar again and again
    if ((wParam & GAIF_FORCE) != 0 && pai->hContact != 0)
    { // request avatar data
      GetAvatarFileName(dwUIN, szUID, pai->filename, MAX_PATH);
      GetAvatarData(pai->hContact, dwUIN, szUID, dbv.pbVal, dbv.cpbVal, pai->filename);
      ICQFreeVariant(&dbv);

      return GAIR_WAITFOR;
    }
  }
  ICQFreeVariant(&dbv);

  return GAIR_NOAVATAR;
}



int IcqGetMaxAvatarSize(WPARAM wParam, LPARAM lParam)
{
  if (wParam) *((int*)wParam) = 64;
  if (lParam) *((int*)lParam) = 64;

  return 0;
}



int IcqAvatarFormatSupported(WPARAM wParam, LPARAM lParam)
{
  if (lParam == PA_FORMAT_JPEG || lParam == PA_FORMAT_GIF)
    return -2;
  else
    return 0;
}



int IcqGetMyAvatar(WPARAM wParam, LPARAM lParam)
{
  if (!gbAvatarsEnabled) return -2;

  if (!wParam) return -3;

  if (!ICQGetContactStaticString(NULL, "AvatarFile", (char*)wParam, (int)lParam))
  {
    if (!access((char*)wParam, 0)) return 0;
  }
  return -1;
}



int IcqSetMyAvatar(WPARAM wParam, LPARAM lParam)
{
  char* szFile = (char*)lParam;
  int iRet = -1;

  if (!gbAvatarsEnabled || !gbSsiEnabled) return -2;

  if (szFile)
  { // set file for avatar
    char szMyFile[MAX_PATH+1];
    int dwPaFormat = DetectAvatarFormat(szFile);
    char* hash;
    HBITMAP avt;

    avt = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (WPARAM)szFile);
    if (!avt) return iRet;
    DeleteObject(avt);

    GetFullAvatarFileName(0, NULL, dwPaFormat, szMyFile, MAX_PATH);
    if (!CopyFile(szFile, szMyFile, FALSE))
    {
      NetLog_Server("Failed to copy our avatar to local storage.");
      return iRet;
    }

    hash = calcMD5Hash(szMyFile);
    if (hash)
    {
      char* ihash = (char*)_alloca(0x14);
      // upload hash to server
      ihash[0] = 0;    //unknown
      ihash[1] = 1;    //hash type
      ihash[2] = 1;    //hash status
      ihash[3] = 0x10; //hash len
      memcpy(ihash+4, hash, 0x10);
      updateServAvatarHash(ihash+2, 0x12);

      if (ICQWriteContactSettingBlob(NULL, "AvatarHash", ihash, 0x14))
      {
        NetLog_Server("Failed to save avatar hash.");
      }

      ICQWriteContactSettingString(NULL, "AvatarFile", szMyFile);
      iRet = 0;

      SAFE_FREE(&hash);
    }
  }
  else
  { // delete user avatar
    BYTE bEmptyAvatar[7] = {0x00,0x05,0x02,0x01,0xD2,0x04,0x72};

    ICQDeleteContactSetting(NULL, "AvatarFile");
    ICQDeleteContactSetting(NULL, "AvatarHash");
    updateServAvatarHash(bEmptyAvatar, 7); // clear hash on server
    iRet = 0;
  }

  return iRet;
}



void updateAimAwayMsg()
{
  char** szMsg = MirandaStatusToAwayMsg(gnCurrentStatus);

  EnterCriticalSection(&modeMsgsMutex);
  if (szMsg)
    icq_sendSetAimAwayMsgServ(*szMsg);
  LeaveCriticalSection(&modeMsgsMutex);
}



int IcqSetStatus(WPARAM wParam, LPARAM lParam)
{
  int nNewStatus = MirandaStatusToSupported(wParam);

  if (gbTempVisListEnabled) // remove temporary visible users
    clearTemporaryVisibleList();

  if (nNewStatus != gnCurrentStatus)
  {
    if (ICQGetContactSettingByte(NULL, "XStatusReset", DEFAULT_XSTATUS_RESET))
    { // clear custom status on status change
      IcqSetXStatus(0, 0);
    }

    // New status is OFFLINE
    if (nNewStatus == ID_STATUS_OFFLINE)
    {
      // for quick logoff
      icqGoingOnlineStatus = nNewStatus;

      // Send disconnect packet
      icq_sendCloseConnection();

      icq_serverDisconnect(FALSE);

      SetCurrentStatus(ID_STATUS_OFFLINE);

      NetLog_Server("Logged off.");
    }
    else
    {
      switch (gnCurrentStatus)
      {

      // We are offline and need to connect
      case ID_STATUS_OFFLINE:
        {
          char *pszPwd;

          // Update user connection settings
          UpdateGlobalSettings();

          // Read UIN from database
          dwLocalUIN = ICQGetContactSettingUIN(NULL);
          if (dwLocalUIN == 0)
          {
            SetCurrentStatus(ID_STATUS_OFFLINE);

            icq_LogMessage(LOG_FATAL, "You have not entered a ICQ number.\nConfigure this in Options->Network->ICQ and try again.");
            return 0;
          }

          // Set status to 'Connecting'
          icqGoingOnlineStatus = nNewStatus;
          SetCurrentStatus(ID_STATUS_CONNECTING);

          // Read password from database
          pszPwd = GetUserPassword(FALSE);

          if (pszPwd)
            icq_login(pszPwd);
          else 
            RequestPassword();

          break;
        }

      // We are connecting... We only need to change the going online status
      case ID_STATUS_CONNECTING:
        {
          icqGoingOnlineStatus = nNewStatus;
          break;
        }

      // We are already connected so we should just change status
      default:
        {
          SetCurrentStatus(nNewStatus);

          if (gnCurrentStatus == ID_STATUS_INVISIBLE)
          {
            // Tell whos on our visible list
            icq_sendEntireVisInvisList(0);
            if (gbSsiEnabled)
              updateServVisibilityCode(3);
            icq_setstatus(MirandaStatusToIcq(gnCurrentStatus));
            if (gbAimEnabled)
              updateAimAwayMsg();
          }
          else
          {
            // Tell whos on our invisible list
            icq_sendEntireVisInvisList(1);
            icq_setstatus(MirandaStatusToIcq(gnCurrentStatus));
            if (gbSsiEnabled)
              updateServVisibilityCode(4);
            if (gbAimEnabled)
              updateAimAwayMsg();
          }
        }
      }
    }
  }

  return 0;
}



int IcqGetStatus(WPARAM wParam, LPARAM lParam)
{
  return gnCurrentStatus;
}



int IcqSetAwayMsg(WPARAM wParam, LPARAM lParam)
{
  char** ppszMsg = NULL;
  char* szNewUtf = NULL;


  EnterCriticalSection(&modeMsgsMutex);

  ppszMsg = MirandaStatusToAwayMsg(wParam);
  if (!ppszMsg)
  {
    LeaveCriticalSection(&modeMsgsMutex);
    return 1; // Failure
  }

  // Prepare UTF-8 status message
  szNewUtf = ansi_to_utf8((char*)lParam);

  if (strcmpnull(szNewUtf, *ppszMsg))
  {
    // Free old message
    SAFE_FREE(ppszMsg);

    // Set new message
    *ppszMsg = szNewUtf;
    szNewUtf = NULL;

    if (gbAimEnabled && (gnCurrentStatus == (int)wParam))
      icq_sendSetAimAwayMsgServ(*ppszMsg);
  }
  SAFE_FREE(&szNewUtf);

  LeaveCriticalSection(&modeMsgsMutex);

  return 0; // Success
}



static HANDLE HContactFromAuthEvent(WPARAM hEvent)
{
  DBEVENTINFO dbei;
  DWORD body[2];

  ZeroMemory(&dbei, sizeof(dbei));
  dbei.cbSize = sizeof(dbei);
  dbei.cbBlob = sizeof(DWORD)*2;
  dbei.pBlob = (PBYTE)&body;

  if (CallService(MS_DB_EVENT_GET, hEvent, (LPARAM)&dbei))
    return INVALID_HANDLE_VALUE;

  if (dbei.eventType != EVENTTYPE_AUTHREQUEST)
    return INVALID_HANDLE_VALUE;

  if (strcmpnull(dbei.szModule, gpszICQProtoName))
    return INVALID_HANDLE_VALUE;

  return (HANDLE)body[1]; // this is bad - needs new auth system
}



int IcqAuthAllow(WPARAM wParam, LPARAM lParam)
{
  if (icqOnline && wParam)
  {
    DWORD uin;
    uid_str uid;
    HANDLE hContact;

    hContact = HContactFromAuthEvent(wParam);
    if (hContact == INVALID_HANDLE_VALUE)
      return 1;

    if (ICQGetContactSettingUID(hContact, &uin, &uid))
      return 1;

    icq_sendAuthResponseServ(uin, uid, 1, "");

    return 0; // Success
  }

  return 1; // Failure
}



int IcqAuthDeny(WPARAM wParam, LPARAM lParam)
{
  if (icqOnline && wParam)
  {
    DWORD uin;
    uid_str uid;
    HANDLE hContact;


    hContact = HContactFromAuthEvent(wParam);
    if (hContact == INVALID_HANDLE_VALUE)
      return 1;

    if (ICQGetContactSettingUID(hContact, &uin, &uid))
      return 1;

    icq_sendAuthResponseServ(uin, uid, 0, (char *)lParam);

    return 0; // Success
  }

  return 1; // Failure
}



static int cheekySearchId = -1;
static DWORD cheekySearchUin;
static char* cheekySearchUid;
static VOID CALLBACK CheekySearchTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
  ICQSEARCHRESULT isr = {0};

  KillTimer(hwnd, idEvent);

  isr.hdr.cbSize = sizeof(isr);
  if (cheekySearchUin)
  {
    isr.hdr.nick = "";
    isr.uid = NULL;
  }
  else
  {
    isr.hdr.nick = cheekySearchUid;
    isr.uid = cheekySearchUid;
  }
  isr.hdr.firstName = "";
  isr.hdr.lastName = "";
  isr.hdr.email = "";
  isr.uin = cheekySearchUin;

  ICQBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)cheekySearchId, (LPARAM)&isr);
  ICQBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)cheekySearchId, 0);

  cheekySearchId = -1;
}


int IcqBasicSearch(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  {
    char* pszSearch = (char*)lParam;
    DWORD dwUin;

    if (strlennull(pszSearch))
    {
      char pszUIN[255];
      int nHandle = 0;
      unsigned int i, j;

      if (!gbAimEnabled)
      {
        for (i=j=0; (i<strlennull(pszSearch)) && (j<255); i++)
        { // we take only numbers
          if ((pszSearch[i]>=0x30) && (pszSearch[i]<=0x39))
          {
            pszUIN[j] = pszSearch[i];
            j++;
          }
        }
      }
      else
      {
        for (i=j=0; (i<strlennull(pszSearch)) && (j<255); i++)
        { // we remove spaces and slashes
          if ((pszSearch[i]!=0x20) && (pszSearch[i]!='-'))
          {
            pszUIN[j] = pszSearch[i];
            j++;
          }
        }
      }
      pszUIN[j] = 0;
      
      if (strlennull(pszUIN))
      {
        if (IsStringUIN(pszUIN))
          dwUin = atoi(pszUIN);
        else
          dwUin = 0;

        // Cheeky instant UIN search
        if (!dwUin || GetKeyState(VK_CONTROL)&0x8000)
        {
          cheekySearchId = GenerateCookie(0);
          cheekySearchUin = dwUin;
          cheekySearchUid = null_strdup(pszUIN);
          SetTimer(NULL, 0, 10, CheekySearchTimerProc); // The caller needs to get this return value before the results
          nHandle = cheekySearchId;
        }
        else if (icqOnline)
        {
          nHandle = SearchByUin(dwUin);
        }

        // Success
        return nHandle;
      }
    }
  }

  // Failure
  return 0;
}


int IcqSearchByEmail(WPARAM wParam, LPARAM lParam)
{
  if (lParam && icqOnline && (strlennull((char*)lParam) > 0))
  {
    DWORD dwSearchId, dwSecId;

    // Success
    dwSearchId = SearchByEmail((char *)lParam);
    if (gbAimEnabled)
      dwSecId = icq_searchAimByEmail((char *)lParam, dwSearchId);
    else
      dwSecId = 0;
    if (dwSearchId) 
      return dwSearchId;
    else
      return dwSecId;
  }

  return 0; // Failure
}


int IcqSearchByDetails(WPARAM wParam, LPARAM lParam)
{
  if (lParam && icqOnline)
  {
    PROTOSEARCHBYNAME *psbn=(PROTOSEARCHBYNAME*)lParam;


    if (psbn->pszNick || psbn->pszFirstName || psbn->pszLastName)
    {
      // Success
      return SearchByNames(psbn->pszNick, psbn->pszFirstName, psbn->pszLastName);
    }
  }

  return 0; // Failure
}


int IcqCreateAdvSearchUI(WPARAM wParam, LPARAM lParam)
{
  if (lParam && hInst)
  {
    // Success
    return (int)CreateDialog(hInst, MAKEINTRESOURCE(IDD_ICQADVANCEDSEARCH), (HWND)lParam, AdvancedSearchDlgProc);
  }

  return 0; // Failure
}


int IcqSearchByAdvanced(WPARAM wParam, LPARAM lParam)
{
  if (icqOnline && IsWindow((HWND)lParam))
  {
    int nDataLen;
    BYTE* bySearchData;

    if (bySearchData = createAdvancedSearchStructure((HWND)lParam, &nDataLen))
    {
      int result;

      result = icq_sendAdvancedSearchServ(bySearchData, nDataLen);
      SAFE_FREE(&bySearchData);

      return result; // Success
    }
  }

  return 0; // Failure
}



// TODO: Adding needs some more work in general
static HANDLE AddToListByUIN(DWORD dwUin, DWORD dwFlags)
{
  HANDLE hContact;
  int bAdded;

  hContact = HContactFromUIN(dwUin, &bAdded);

  if (hContact)
  {
    if ((!dwFlags & PALF_TEMPORARY) && DBGetContactSettingByte(hContact, "CList", "NotOnList", 1)) 
    {
      DBDeleteContactSetting(hContact, "CList", "NotOnList");
      SetContactHidden(hContact, 0);
    }

    return hContact; // Success
  }

  return NULL; // Failure
}


static HANDLE AddToListByUID(char *szUID, DWORD dwFlags)
{
  HANDLE hContact;
  int bAdded;

  hContact = HContactFromUID(0, szUID, &bAdded);

  if (hContact)
  {
    if ((!dwFlags & PALF_TEMPORARY) && DBGetContactSettingByte(hContact, "CList", "NotOnList", 1)) 
    {
      DBDeleteContactSetting(hContact, "CList", "NotOnList");
      SetContactHidden(hContact, 0);
    }

    return hContact; // Success
  }

  return NULL; // Failure
}



int IcqAddToList(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  { // this should be only from search
    ICQSEARCHRESULT *isr = (ICQSEARCHRESULT*)lParam;

    if (isr->hdr.cbSize == sizeof(ICQSEARCHRESULT))
    {
      if (!isr->uin)
        return (int)AddToListByUID(isr->hdr.nick, wParam);
      else
        return (int)AddToListByUIN(isr->uin, wParam);
    }
  }

  return 0; // Failure
}



int IcqAddToListByEvent(WPARAM wParam, LPARAM lParam)
{
  DBEVENTINFO dbei = {0};
  DWORD uin = 0;
  uid_str uid = {0};


  dbei.cbSize = sizeof(dbei);

  if ((dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, lParam, 0)) == -1)
    return 0;

  dbei.pBlob = (PBYTE)_alloca(dbei.cbBlob + 1);
  dbei.pBlob[dbei.cbBlob] = '\0';

  if (CallService(MS_DB_EVENT_GET, lParam, (LPARAM)&dbei))
    return 0; // failed to get event

  if (strcmpnull(dbei.szModule, gpszICQProtoName))
    return 0; // this event is not ours

  if (dbei.eventType == EVENTTYPE_CONTACTS)
  {
    int i, ci = HIWORD(wParam);
    char* pbOffset, *pbEnd;

    for (i = 0, pbOffset = (char*)dbei.pBlob, pbEnd = pbOffset + dbei.cbBlob; i <= ci; i++)
    {
      pbOffset += strlennull((char*)pbOffset) + 1;  // Nick
      if (pbOffset >= pbEnd) break;
      if (i = ci)
      { // we found the contact, get uid
        if (IsStringUIN((char*)pbOffset))
          uin = atoi((char*)pbOffset);
        else
        {
          uin = 0;
          strcpy(uid, (char*)pbOffset);
        }
      }
      pbOffset += strlennull((char*)pbOffset) + 1;  // Uin
      if (pbOffset >= pbEnd) break;
    }
  }
  else if (dbei.eventType != EVENTTYPE_AUTHREQUEST && dbei.eventType != EVENTTYPE_ADDED)
  {
    return 0;
  }
  else // auth req or added event
  {
    HANDLE hContact = ((HANDLE*)dbei.pBlob)[1]; // this sucks - awaiting new auth system

    if (ICQGetContactSettingUID(hContact, &uin, &uid))
      return 0;
  }

  if (uin != 0)
  {
    return (int)AddToListByUIN(uin, LOWORD(wParam)); // Success
  }
  else if (strlennull(uid))
  { // add aim contact
    return (int)AddToListByUID(uid, LOWORD(wParam)); // Success
  }

  return 0; // Failure
}



int IcqSetNickName(WPARAM wParam, LPARAM lParam)
{
  PBYTE buf = NULL;
  int buflen = 0;

  ICQWriteContactSettingString(NULL, "Nick", (char*)lParam);

  ppackLEWord(&buf, &buflen, 0);  // data length
  ppackTLVLNTS(&buf, &buflen, (char*)lParam, TLV_NICKNAME, 1);
  *(PWORD)buf = buflen - 2;
  IcqChangeInfo(META_SET_FULLINFO_REQ, (LPARAM)buf);

  return 0; // Not defined // TODO: change when definition is ready
}



int IcqChangeInfo(WPARAM wParam, LPARAM lParam)
{
  if (lParam && icqOnline)
  {
    return icq_changeUserDetailsServ((WORD)wParam, (PBYTE)(lParam+2), *(PWORD)lParam); // Success
  }

  return 0; // Failure
}



static int messageRate = 0;
static DWORD lastMessageTick = 0;
int IcqGetInfo(WPARAM wParam, LPARAM lParam)
{
  if (lParam && icqOnline)
  { // TODO: add checking for SGIF_ONOPEN, otherwise max one per 10sec
    CCSDATA* ccs = (CCSDATA*)lParam;
    DWORD dwUin;
    uid_str szUid;

    if (ICQGetContactSettingUID(ccs->hContact, &dwUin, &szUid))
    {
      return 0; // Invalid contact
    }

    messageRate -= (GetTickCount() - lastMessageTick)/10;
    if (messageRate<0) // TODO: this is bad, needs centralising
      messageRate = 0;
    lastMessageTick = GetTickCount();
    messageRate += 67; // max 1.5 msgs/sec when rate is high

    // server kicks if 100 msgs sent instantly, so send max 50 instantly
    if (messageRate < 67*50)
    {
      if (dwUin)
        icq_sendGetInfoServ(dwUin, (ccs->wParam & SGIF_MINIMAL) != 0);
      else
        icq_sendGetAimProfileServ(ccs->hContact, szUid);

      return 0; // Success
    }
  }

  return 1; // Failure
}



int IcqFileAllow(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;
    DWORD dwUin;

    if (ICQGetContactSettingUID(ccs->hContact, &dwUin, NULL))
      return 0; // Invalid contact

    if (dwUin && icqOnline && ccs->hContact && ccs->lParam && ccs->wParam)
    {
      filetransfer* ft = ((filetransfer *)ccs->wParam);

      ft->szSavePath = null_strdup((char *)ccs->lParam);
      AddExpectedFileRecv(ft);

      // Was request received thru DC and have we a open DC, send through that
      if (ft->bDC && IsDirectConnectionOpen(ccs->hContact, DIRECTCONN_STANDARD))
        icq_sendFileAcceptDirect(ccs->hContact, ft);
      else
        icq_sendFileAcceptServ(dwUin, ft, 0);

      return ccs->wParam; // Success
    }
  }

  return 0; // Failure
}



int IcqFileDeny(WPARAM wParam, LPARAM lParam)
{
  int nReturnValue = 1;

  if (lParam)
  {
    CCSDATA *ccs = (CCSDATA *)lParam;
    filetransfer *ft = (filetransfer*)ccs->wParam;
    DWORD dwUin;

    if (ICQGetContactSettingUID(ccs->hContact, &dwUin, NULL))
      return 1; // Invalid contact

    if (icqOnline && dwUin && ccs->wParam && ccs->hContact) 
    {
      // Was request received thru DC and have we a open DC, send through that
      if (ft->bDC && IsDirectConnectionOpen(ccs->hContact, DIRECTCONN_STANDARD))
        icq_sendFileDenyDirect(ccs->hContact, ft, (char*)ccs->lParam);
      else
        icq_sendFileDenyServ(dwUin, ft, (char*)ccs->lParam, 0);

      nReturnValue = 0; // Success
    }
    /* FIXME: ft leaks (but can get double freed?) */
  }

  return nReturnValue;
}



int IcqFileCancel(WPARAM wParam, LPARAM lParam)
{
  if (lParam /*&& icqOnline*/)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;
    DWORD dwUin;

    if (ICQGetContactSettingUID(ccs->hContact, &dwUin, NULL))
      return 1; // Invalid contact

    if (ccs->hContact && dwUin && ccs->wParam)
    {
      filetransfer * ft = (filetransfer * ) ccs->wParam;

      icq_CancelFileTransfer(ccs->hContact, ft);

      return 0; // Success
    }
  }

  return 1; // Failure
}



int IcqFileResume(WPARAM wParam, LPARAM lParam)
{
  if (icqOnline && wParam)
  {
    PROTOFILERESUME *pfr = (PROTOFILERESUME*)lParam;

    icq_sendFileResume((filetransfer *)wParam, pfr->action, pfr->szFilename);

    return 0; // Success
  }

  return 1; // Failure
}



int IcqSendSms(WPARAM wParam, LPARAM lParam)
{
  if (icqOnline && wParam && lParam)
    return icq_sendSMSServ((const char *)wParam, (const char *)lParam);

  return 0; // Failure
}



// Maybe we should be saving these up for batch changing, but I can't be bothered yet
int IcqSetApparentMode(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;
    DWORD uin;
    uid_str uid;

    if (ICQGetContactSettingUID(ccs->hContact, &uin, &uid))
      return 1; // Invalid contact

    if (ccs->hContact)
    {
      // Only 3 modes are supported
      if (ccs->wParam == 0 || ccs->wParam == ID_STATUS_ONLINE || ccs->wParam == ID_STATUS_OFFLINE)
      {
        int oldMode = ICQGetContactSettingWord(ccs->hContact, "ApparentMode", 0);

        // Don't send redundant updates
        if ((int)ccs->wParam != oldMode)
        {
          ICQWriteContactSettingWord(ccs->hContact, "ApparentMode", (WORD)ccs->wParam);

          // Not being online is only an error when in SS mode. This is not handled
          // yet so we just ignore this for now.
          if (icqOnline)
          {
            if (oldMode != 0) // Remove from old list
              icq_sendChangeVisInvis(ccs->hContact, uin, uid, oldMode==ID_STATUS_OFFLINE, 0);
            if (ccs->wParam != 0) // Add to new list
              icq_sendChangeVisInvis(ccs->hContact, uin, uid, ccs->wParam==ID_STATUS_OFFLINE, 1);
          }

          return 0; // Success
        }
      }
    }
  }

  return 1; // Failure
}



int IcqGetAwayMsg(WPARAM wParam,LPARAM lParam)
{
  if (lParam && icqOnline)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;
    DWORD dwUin;
    uid_str szUID;
    WORD wStatus;

    if (ICQGetContactSettingUID(ccs->hContact, &dwUin, &szUID))
      return 0; // Invalid contact

    wStatus = ICQGetContactStatus(ccs->hContact);

    if (dwUin)
    {
      int wMessageType = 0;

      switch(wStatus)
      {

      case ID_STATUS_AWAY:
        wMessageType = MTYPE_AUTOAWAY;
        break;

      case ID_STATUS_NA:
        wMessageType = MTYPE_AUTONA;
        break;

      case ID_STATUS_OCCUPIED:
        wMessageType = MTYPE_AUTOBUSY;
        break;

      case ID_STATUS_DND:
        wMessageType = MTYPE_AUTODND;
        break;

      case ID_STATUS_FREECHAT:
        wMessageType = MTYPE_AUTOFFC;
        break;

      default:
        break;
      }

      if (wMessageType)
      {
        if (gbDCMsgEnabled && IsDirectConnectionOpen(ccs->hContact, DIRECTCONN_STANDARD))
        {
          int iRes = icq_sendGetAwayMsgDirect(ccs->hContact, wMessageType);
          if (iRes) return iRes; // we succeded, return
        }
        return icq_sendGetAwayMsgServ(dwUin, wMessageType, (WORD)(ICQGetContactSettingWord(ccs->hContact, "Version", 0)==9?9:ICQ_VERSION)); // Success
      }
    }
    else
    {
      if (wStatus == ID_STATUS_AWAY)
      {
        return icq_sendGetAimAwayMsgServ(szUID, MTYPE_AUTOAWAY);
      }
    }
  }

  return 0; // Failure
}



static message_cookie_data* CreateMsgCookieData(BYTE bMsgType, HANDLE hContact, DWORD dwUin)
{
  BYTE bAckType;
  WORD wStatus = ICQGetContactStatus(hContact);

  if (!ICQGetContactSettingByte(NULL, "SlowSend", DEFAULT_SLOWSEND))
    bAckType = ACKTYPE_NONE;
  else if ((!dwUin) || (!CheckContactCapabilities(hContact, CAPF_SRV_RELAY)) || 
      (wStatus == ID_STATUS_OFFLINE) || ICQGetContactSettingByte(NULL, "OnlyServerAcks", DEFAULT_ONLYSERVERACKS))
    bAckType = ACKTYPE_SERVER;
  else
    bAckType = ACKTYPE_CLIENT;

  return CreateMessageCookie(bMsgType, bAckType);
}



int IcqSendMessage(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;

    if (ccs->hContact && ccs->lParam)
    {
      WORD wRecipientStatus;
      DWORD dwCookie;
      DWORD dwUin;
      uid_str szUID;
      char* pszText;

      if (ICQGetContactSettingUID(ccs->hContact, &dwUin, &szUID))
      { // Invalid contact
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_MESSAGE, ICQTranslate("The receiver has an invalid user ID."));
        return dwCookie;
      }

      if (dwUin && gbTempVisListEnabled && gnCurrentStatus == ID_STATUS_INVISIBLE)
        makeContactTemporaryVisible(ccs->hContact);  // make us temporarily visible to contact

      pszText = (char*)ccs->lParam;

      wRecipientStatus = ICQGetContactStatus(ccs->hContact);

      // Failure scenarios
      if (!icqOnline)
      {
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_MESSAGE, ICQTranslate("You cannot send messages when you are offline."));
      }
      else if ((wRecipientStatus == ID_STATUS_OFFLINE) && (strlennull(pszText) > 450))
      {
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_MESSAGE, ICQTranslate("Messages to offline contacts must be shorter than 450 characters."));
      }
      // Looks OK
      else
      {
        message_cookie_data* pCookieData;

        if (wRecipientStatus != ID_STATUS_OFFLINE && gbUtfEnabled==2 && !IsUSASCII(pszText, strlennull(pszText)) 
          && CheckContactCapabilities(ccs->hContact, CAPF_UTF) && ICQGetContactSettingByte(ccs->hContact, "UnicodeSend", 1))
        { // text contains national chars and we should send all this as Unicode, so do it
          char* pszUtf = NULL;
          int nStrSize = MultiByteToWideChar(CP_ACP, 0, pszText, strlennull(pszText), (wchar_t*)pszUtf, 0);
          int nRes;

          pszUtf = calloc(nStrSize + 2, sizeof(wchar_t));
          pszUtf[0] = '\0'; // we omit ansi string - not used...
          MultiByteToWideChar(CP_ACP, 0, pszText, strlennull(pszText), (wchar_t*)(pszUtf+1), nStrSize);
          *(WORD*)(pszUtf + 1 + nStrSize*sizeof(wchar_t)) = '\0'; // trailing zeros

          ccs->lParam = (LPARAM)pszUtf; // yeah, this is quite a hack, BE AWARE OF THAT !!!
          ccs->wParam |= PREF_UNICODE;

          nRes = IcqSendMessageW(wParam, lParam);
          ccs->lParam = (LPARAM)pszText;

          SAFE_FREE(&pszUtf); // release memory

          return nRes;
        }

        if (!dwUin)
        { // prepare AIM Html message
          char *mng = MangleXml(pszText, strlennull(pszText));
          char *tmp = (char*)malloc(strlennull(mng) + 28);

          strcpy(tmp, "<HTML><BODY>");
          strcat(tmp, mng);
          SAFE_FREE(&mng);
          strcat(tmp, "</BODY></HTML>");
          pszText = tmp;
        }

        // Set up the ack type
        pCookieData = CreateMsgCookieData(MTYPE_PLAIN, ccs->hContact, dwUin);

#ifdef _DEBUG
        NetLog_Server("Send message - Message cap is %u", CheckContactCapabilities(ccs->hContact, CAPF_SRV_RELAY));
        NetLog_Server("Send message - Contact status is %u", wRecipientStatus);
#endif
        if (dwUin && gbDCMsgEnabled && IsDirectConnectionOpen(ccs->hContact, DIRECTCONN_STANDARD))
        {
          int iRes = icq_SendDirectMessage(dwUin, ccs->hContact, pszText, strlennull(pszText), 1, pCookieData, NULL);
          if (iRes) return iRes; // we succeded, return
        }

        if ((!dwUin || !CheckContactCapabilities(ccs->hContact, CAPF_SRV_RELAY)) || (wRecipientStatus == ID_STATUS_OFFLINE))
        {
          dwCookie = icq_SendChannel1Message(dwUin, szUID, ccs->hContact, pszText, pCookieData);
        }
        else
        {
          WORD wPriority;

          if (wRecipientStatus == ID_STATUS_ONLINE || wRecipientStatus == ID_STATUS_FREECHAT)
            wPriority = 0x0001;
          else
            wPriority = 0x0021;

          dwCookie = icq_SendChannel2Message(dwUin, pszText, strlennull(pszText), wPriority, pCookieData, NULL);
        }

        // This will stop the message dialog from waiting for the real message delivery ack
        if (pCookieData->nAckType == ACKTYPE_NONE)
        {
          icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_MESSAGE, NULL);
          // We need to free this here since we will never see the real ack
          // The actual cookie value will still have to be returned to the message dialog though
          ReleaseCookie(dwCookie);
        }
        if (!dwUin) SAFE_FREE(&pszText);
      }

      return dwCookie; // Success
    }
  }

  return 0; // Failure
}



static char* convertMsgToUserSpecificAnsi(HANDLE hContact, const char* szMsg)
{ // this needs valid "Unicode" buffer from SRMM !!!
  WORD wCP = ICQGetContactSettingWord(hContact, "CodePage", gwAnsiCodepage);
  int nMsgLen = strlennull(szMsg);
  wchar_t* usMsg = (wchar_t*)(szMsg + nMsgLen + 1);
  char* szAnsi = NULL;

  if (wCP != CP_ACP)
  {
    int nStrSize = WideCharToMultiByte(wCP, 0, usMsg, nMsgLen, szAnsi, 0, NULL, NULL);

    szAnsi = malloc(nStrSize + 1);
    WideCharToMultiByte(wCP, 0, usMsg, nMsgLen, szAnsi, nStrSize, NULL, NULL);
    szAnsi[nStrSize] = '\0';
  }
  return szAnsi;
}



int IcqSendMessageW(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;

    if (ccs->hContact && ccs->lParam)
    {
      WORD wRecipientStatus;
      DWORD dwCookie;
      DWORD dwUin;
      uid_str szUID;
      wchar_t* pszText;
      // TODO: this was not working, removed check
      if (!gbUtfEnabled || /*(ccs->wParam & PREF_UNICODE == PREF_UNICODE) ||*/ 
        (!CheckContactCapabilities(ccs->hContact, CAPF_UTF)) || (!ICQGetContactSettingByte(ccs->hContact, "UnicodeSend", 1)))
      {  // send as unicode only if marked as unicode & unicode enabled
        char* szAnsi = convertMsgToUserSpecificAnsi(ccs->hContact, (char*)ccs->lParam);
        int nRes;

        if (szAnsi) ccs->lParam = (LPARAM)szAnsi;
        nRes = IcqSendMessage(wParam, lParam);
        SAFE_FREE(&szAnsi);

        return nRes;
      }

      if (ICQGetContactSettingUID(ccs->hContact, &dwUin, &szUID))
      { // Invalid contact
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_MESSAGE, ICQTranslate("The receiver has an invalid user ID."));
        return dwCookie;
      }

      if (dwUin && gbTempVisListEnabled && gnCurrentStatus == ID_STATUS_INVISIBLE)
        makeContactTemporaryVisible(ccs->hContact); // make us temporarily visible to contact

      pszText = (wchar_t*)((char*)ccs->lParam+strlennull((char*)ccs->lParam)+1); // get the UTF-16 part

      wRecipientStatus = ICQGetContactStatus(ccs->hContact);

      // Failure scenarios
      if (!icqOnline)
      {
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_MESSAGE, ICQTranslate("You cannot send messages when you are offline."));
      }
      // Looks OK
      else
      {
        message_cookie_data* pCookieData;
        BOOL plain_ascii = IsUnicodeAscii(pszText, wcslen(pszText));

        if ((wRecipientStatus == ID_STATUS_OFFLINE) || plain_ascii)
        { // send as plain if no special char or user offline
          char* szAnsi;
          int nRes;

          if (!plain_ascii)
          {
            szAnsi = convertMsgToUserSpecificAnsi(ccs->hContact, (char*)ccs->lParam);
            if (szAnsi) ccs->lParam = (LPARAM)szAnsi;
          }
          nRes = IcqSendMessage(wParam, lParam);
          if (!plain_ascii)
            SAFE_FREE(&szAnsi);

          return nRes;
        };

        // Set up the ack type
        pCookieData = CreateMsgCookieData(MTYPE_PLAIN, ccs->hContact, dwUin);

#ifdef _DEBUG
        NetLog_Server("Send unicode message - Message cap is %u", CheckContactCapabilities(ccs->hContact, CAPF_SRV_RELAY));
        NetLog_Server("Send unicode message - Contact status is %u", wRecipientStatus);
#endif
        if (dwUin && gbDCMsgEnabled && IsDirectConnectionOpen(ccs->hContact, DIRECTCONN_STANDARD))
        {
          char* utf8msg = make_utf8_string(pszText);
          int iRes;

          iRes = icq_SendDirectMessage(dwUin, ccs->hContact, utf8msg, strlennull(utf8msg), 1, pCookieData, CAP_UTF8MSGS);
          SAFE_FREE(&utf8msg);

          if (iRes) return iRes; // we succeded, return
        }

        if (!dwUin || !CheckContactCapabilities(ccs->hContact, CAPF_SRV_RELAY))
        {
          char* utmp;
          char* mng;
          char* tmp;

          if (!dwUin)
          {
            utmp = make_utf8_string(pszText);
            mng = MangleXml(utmp, strlennull(utmp));
            SAFE_FREE(&utmp);
            tmp = (char*)malloc(strlennull(mng) + 28);
            strcpy(tmp, "<HTML><BODY>");
            strcat(tmp, mng);
            SAFE_FREE(&mng);
            strcat(tmp, "</BODY></HTML>");
            pszText = make_unicode_string(tmp);
            SAFE_FREE(&tmp);
          }

          dwCookie = icq_SendChannel1MessageW(dwUin, szUID, ccs->hContact, pszText, pCookieData);

          if (!dwUin) SAFE_FREE(&pszText);
        }
        else
        {
          WORD wPriority;
          char* utf8msg;

          if (wRecipientStatus == ID_STATUS_ONLINE || wRecipientStatus == ID_STATUS_FREECHAT)
            wPriority = 0x0001;
          else
            wPriority = 0x0021;

          utf8msg = make_utf8_string(pszText);
          dwCookie = icq_SendChannel2Message(dwUin, utf8msg, strlennull(utf8msg), wPriority, pCookieData, CAP_UTF8MSGS);

          SAFE_FREE(&utf8msg);
        }

        // This will stop the message dialog from waiting for the real message delivery ack
        if (pCookieData->nAckType == ACKTYPE_NONE)
        {
          icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_MESSAGE, NULL);
          // We need to free this here since we will never see the real ack
          // The actual cookie value will still have to be returned to the message dialog though
          ReleaseCookie(dwCookie);
        }

      }
      return dwCookie; // Success
    }
  }
  return 0; // Failure
}



int IcqSendUrl(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;

    if (ccs->hContact && ccs->lParam)
    {
      DWORD dwCookie;
      WORD wRecipientStatus;
      DWORD dwUin;

      if (ICQGetContactSettingUID(ccs->hContact, &dwUin, NULL))
      { // Invalid contact
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_URL, ICQTranslate("The receiver has an invalid user ID."));
        return dwCookie;
      }

      wRecipientStatus = ICQGetContactStatus(ccs->hContact);

      // Failure
      if (!icqOnline)
      {
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_URL, ICQTranslate("You cannot send messages when you are offline."));
      }
      // Looks OK
      else
      {
        message_cookie_data* pCookieData;
        char* szDesc;
        char* szUrl;
        char* szBody;
        int nBodyLen;
        int nDescLen;
        int nUrlLen;


        // Set up the ack type
        pCookieData = CreateMsgCookieData(MTYPE_URL, ccs->hContact, dwUin);

        // Format the body
        szUrl = (char*)ccs->lParam;
        nUrlLen = strlennull(szUrl);
        szDesc = (char *)ccs->lParam + nUrlLen + 1;
        nDescLen = strlennull(szDesc);
        nBodyLen = nUrlLen + nDescLen + 2;
        szBody = (char *)_alloca(nBodyLen);
        strcpy(szBody, szDesc);
        szBody[nDescLen] = (char)0xFE; // Separator
        strcpy(szBody + nDescLen + 1, szUrl);


        if (gbDCMsgEnabled && IsDirectConnectionOpen(ccs->hContact, DIRECTCONN_STANDARD))
        {
          int iRes = icq_SendDirectMessage(dwUin, ccs->hContact, szBody, nBodyLen, 1, pCookieData, NULL);
          if (iRes) return iRes; // we succeded, return
        }

        // Select channel and send
        if (!CheckContactCapabilities(ccs->hContact, CAPF_SRV_RELAY) ||
          wRecipientStatus == ID_STATUS_OFFLINE)
        {
          dwCookie = icq_SendChannel4Message(dwUin, MTYPE_URL,
            (WORD)nBodyLen, szBody, pCookieData);
        }
        else
        {
          WORD wPriority;

          if (wRecipientStatus == ID_STATUS_ONLINE || wRecipientStatus == ID_STATUS_FREECHAT)
            wPriority = 0x0001;
          else
            wPriority = 0x0021;

          dwCookie = icq_SendChannel2Message(dwUin, szBody, nBodyLen, wPriority, pCookieData, NULL);
        }

        // This will stop the message dialog from waiting for the real message delivery ack
        if (pCookieData->nAckType == ACKTYPE_NONE)
        {
          icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_URL, NULL);
          // We need to free this here since we will never see the real ack
          // The actual cookie value will still have to be returned to the message dialog though
          ReleaseCookie(dwCookie);
        }
      }

      return dwCookie; // Success
    }
  }

  return 0; // Failure
}



int IcqSendContacts(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;

    if (ccs->hContact && ccs->lParam)
    {
      int nContacts;
      int i;
      HANDLE* hContactsList = (HANDLE*)ccs->lParam;
      DWORD dwUin;
      char* szProto;
      WORD wRecipientStatus;
      DWORD dwCookie;

      if (ICQGetContactSettingUID(ccs->hContact, &dwUin, NULL))
      { // Invalid contact
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_CONTACTS, ICQTranslate("The receiver has an invalid user ID."));
        return dwCookie;
      }

      wRecipientStatus = ICQGetContactStatus(ccs->hContact);
      nContacts = HIWORD(ccs->wParam);

      // Failures
      if (!icqOnline)
      {
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_CONTACTS, ICQTranslate("You cannot send messages when you are offline."));
      }
      else if (!hContactsList || (nContacts < 1) || (nContacts > MAX_CONTACTSSEND))
      {
        dwCookie = GenerateCookie(0);
        icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_CONTACTS, ICQTranslate("Bad data (internal error #1)"));
      }
      // OK
      else
      {
        int nBodyLength;
        char szUin[UINMAXLEN];
        char szCount[17];
        struct icq_contactsend_s* contacts = NULL;
        uid_str szUid;


        // Format the body
        // This is kinda messy, but there is no simple way to do it. First
        // we need to calculate the length of the packet.
        if (contacts = (struct icq_contactsend_s*)calloc(sizeof(struct icq_contactsend_s), nContacts))
        {
          nBodyLength = 0;
          for(i = 0; i < nContacts; i++)
          {
            szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContactsList[i], 0);
            if (strcmpnull(szProto, gpszICQProtoName))
              break; // Abort if a non icq contact is found
            if (ICQGetContactSettingUID(hContactsList[i], &contacts[i].uin, &szUid))
              break; // Abort if invalid contact
            contacts[i].uid = contacts[i].uin?NULL:null_strdup(szUid);
            contacts[i].szNick = NickFromHandle(hContactsList[i]);
            // Compute this contact's length
            nBodyLength += getUIDLen(contacts[i].uin, contacts[i].uid) + 1;
            nBodyLength += strlennull(contacts[i].szNick) + 1;
          }

          if (i == nContacts)
          {
            message_cookie_data* pCookieData;
            char* pBody;
            char* pBuffer;

#ifdef _DEBUG
            NetLog_Server("Sending contacts to %d.", dwUin);
#endif
            // Compute count record's length
            _itoa(nContacts, szCount, 10);
            nBodyLength += strlennull(szCount) + 1;

            // Finally we need to copy the contact data into the packet body
            pBuffer = pBody = (char *)calloc(1, nBodyLength);
            strncpy(pBuffer, szCount, nBodyLength);
            pBuffer += strlennull(pBuffer);
            *pBuffer++ = (char)0xFE;
            for (i = 0; i < nContacts; i++)
            {
              if (contacts[i].uin)
              {
                _itoa(contacts[i].uin, szUin, 10);
                strcpy(pBuffer, szUin);
              }
              else
                strcpy(pBuffer, contacts[i].uid);
              pBuffer += strlennull(pBuffer);
              *pBuffer++ = (char)0xFE;
              strcpy(pBuffer, contacts[i].szNick);
              pBuffer += strlennull(pBuffer);
              *pBuffer++ = (char)0xFE;
            }

            // Set up the ack type
            pCookieData = CreateMsgCookieData(MTYPE_CONTACTS, ccs->hContact, dwUin);

            if (gbDCMsgEnabled && IsDirectConnectionOpen(ccs->hContact, DIRECTCONN_STANDARD))
            {
              int iRes = icq_SendDirectMessage(dwUin, ccs->hContact, pBody, nBodyLength, 1, pCookieData, NULL);

              if (iRes)
              {
                SAFE_FREE(&pBody);

                for(i = 0; i < nContacts; i++)
                { // release memory
                  SAFE_FREE(&contacts[i].szNick);
                  SAFE_FREE(&contacts[i].uid);
                }

                SAFE_FREE(&contacts);

                return iRes; // we succeded, return
              }
            }

            // Select channel and send
            if (!CheckContactCapabilities(ccs->hContact, CAPF_SRV_RELAY) ||
              wRecipientStatus == ID_STATUS_OFFLINE)
            {
              dwCookie = icq_SendChannel4Message(dwUin, MTYPE_CONTACTS,
                (WORD)nBodyLength, pBody, pCookieData);
            }
            else
            {
              WORD wPriority;

              if (wRecipientStatus == ID_STATUS_ONLINE || wRecipientStatus == ID_STATUS_FREECHAT)
                wPriority = 0x0001;
              else
                wPriority = 0x0021;

              dwCookie = icq_SendChannel2Message(dwUin, pBody, nBodyLength, wPriority, pCookieData, NULL);
            }

            // This will stop the message dialog from waiting for the real message delivery ack
            if (pCookieData->nAckType == ACKTYPE_NONE)
             {
              icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_CONTACTS, NULL);
              // We need to free this here since we will never see the real ack
              // The actual cookie value will still have to be returned to the message dialog though
              ReleaseCookie(dwCookie);
            }
            SAFE_FREE(&pBody);
          }
          else
          {
            dwCookie = GenerateCookie(0);
            icq_SendProtoAck(ccs->hContact, dwCookie, ACKRESULT_FAILED, ACKTYPE_CONTACTS, ICQTranslate("Bad data (internal error #2)"));
          }

          for(i = 0; i < nContacts; i++)
          {
            SAFE_FREE(&contacts[i].szNick);
            SAFE_FREE(&contacts[i].uid);
          }

          SAFE_FREE(&contacts);
        }
        else
        {
          dwCookie = 0;
        }
      }

      return dwCookie;
    }
  }

  // Exit with Failure
  return 0;
}



int IcqSendFile(WPARAM wParam, LPARAM lParam)
{
  if (lParam && icqOnline)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;

    if (ccs->hContact && ccs->lParam && ccs->wParam)
    {
      HANDLE hContact = ccs->hContact;
      char** files = (char**)ccs->lParam;
      char* pszDesc = (char*)ccs->wParam;
      DWORD dwUin;

      if (ICQGetContactSettingUID(hContact, &dwUin, NULL))
        return 0; // Invalid contact

      if (dwUin)
      {
        if (ICQGetContactStatus(hContact) != ID_STATUS_OFFLINE)
        {
          WORD wClientVersion;

          wClientVersion = ICQGetContactSettingWord(ccs->hContact, "Version", 7);
          if (wClientVersion < 7)
          {
            NetLog_Server("IcqSendFile() can't send to version %u", wClientVersion);
          }
          else
          {
            int i;
            filetransfer* ft;
            struct _stat statbuf;

            // Initialize filetransfer struct
            ft = CreateFileTransfer(hContact, dwUin, (wClientVersion == 7) ? 7: 8);

            for (ft->dwFileCount = 0; files[ft->dwFileCount]; ft->dwFileCount++);
            ft->files = (char **)malloc(sizeof(char *) * ft->dwFileCount);
            ft->dwTotalSize = 0;
            for (i = 0; i < (int)ft->dwFileCount; i++)
            {
              ft->files[i] = null_strdup(files[i]);

              if (_stat(files[i], &statbuf))
                NetLog_Server("IcqSendFile() was passed invalid filename(s)");
              else
                ft->dwTotalSize += statbuf.st_size;
            }
            ft->szDescription = null_strdup(pszDesc);
            ft->dwTransferSpeed = 100;
            ft->sending = 1;
            ft->fileId = -1;
            ft->iCurrentFile = 0;
            ft->dwCookie = AllocateCookie(CKT_FILE, 0, dwUin, ft);
            ft->hConnection = NULL;

            // Send file transfer request
            {
              char szFiles[64];
              char* pszFiles;


              NetLog_Server("Init file send");

              if (ft->dwFileCount == 1)
              {
                pszFiles = strrchr(ft->files[0], '\\');
                if (pszFiles)
                  pszFiles++;
                else
                  pszFiles = ft->files[0];
              }
              else
              {
                null_snprintf(szFiles, 64, ICQTranslate("%d Files"), ft->dwFileCount);
                pszFiles = szFiles;
              }

              // Send packet
              {
                if (ft->nVersion == 7)
                {
                  if (gbDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD))
                  {
                    int iRes = icq_sendFileSendDirectv7(ft, pszFiles); 
                    if (iRes) return (int)(HANDLE)ft; // Success
                  }
                  NetLog_Server("Sending v%u file transfer request through server", 7);
                  icq_sendFileSendServv7(ft, pszFiles);
                }
                else
                {
                  if (gbDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD))
                  {
                    int iRes = icq_sendFileSendDirectv8(ft, pszFiles); 
                    if (iRes) return (int)(HANDLE)ft; // Success
                  }
                  NetLog_Server("Sending v%u file transfer request through server", 8);
                  icq_sendFileSendServv8(ft, pszFiles, ACKTYPE_NONE);
                }
              }
            }

            return (int)(HANDLE)ft; // Success
          }
        }
      }
    }
  }

  return 0; // Failure
}


int IcqSendAuthRequest(WPARAM wParam, LPARAM lParam)
{
  if (lParam && icqOnline)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;
    DWORD dwUin;
    uid_str szUid;

    if (ccs->hContact)
    {
      if (ICQGetContactSettingUID(ccs->hContact, &dwUin, &szUid))
        return 1; // Invalid contact

      if (dwUin && ccs->lParam)
      {
        char *text = (char *)ccs->lParam;
        char *utf;

        utf = ansi_to_utf8(text); // Miranda is ANSI only here

        icq_sendAuthReqServ(dwUin, szUid, utf);

        SAFE_FREE(&utf);

        return 0; // Success
      }
    }
  }

  return 1; // Failure
}



int IcqSendYouWereAdded(WPARAM wParam, LPARAM lParam)
{
  if (lParam && icqOnline)
  {
    CCSDATA* ccs = (CCSDATA*)lParam;

    if (ccs->hContact)
    {
      DWORD dwUin, dwMyUin;

      if (ICQGetContactSettingUID(ccs->hContact, &dwUin, NULL))
        return 1; // Invalid contact

      dwMyUin = ICQGetContactSettingUIN(NULL);

      if (dwUin)
      {
        icq_sendYouWereAddedServ(dwUin, dwMyUin);

        return 0; // Success
      }
    }
  }

  return 1; // Failure
}



int IcqGrantAuthorization(WPARAM wParam, LPARAM lParam)
{
  if (gnCurrentStatus != ID_STATUS_OFFLINE && gnCurrentStatus != ID_STATUS_CONNECTING && wParam != 0)
  {
    DWORD dwUin;
    uid_str szUid;

    if (ICQGetContactSettingUID((HANDLE)wParam, &dwUin, &szUid))
      return 0; // Invalid contact

    // send without reason, do we need any ?
    icq_sendGrantAuthServ(dwUin, szUid, NULL);
  }

  return 0;
}



int IcqRevokeAuthorization(WPARAM wParam, LPARAM lParam)
{
  if (gnCurrentStatus != ID_STATUS_OFFLINE && gnCurrentStatus != ID_STATUS_CONNECTING && wParam != 0)
  {
    DWORD dwUin;
    uid_str szUid;
    char str[MAX_PATH], cap[MAX_PATH];

    if (ICQGetContactSettingUID((HANDLE)wParam, &dwUin, &szUid))
      return 0; // Invalid contact

    if (MessageBoxUtf(NULL, ICQTranslateUtfStatic("Are you sure you want to revoke user's autorisation (this will remove you from his/her list on some clients) ?", str), ICQTranslateUtfStatic("Confirmation", cap), MB_ICONQUESTION | MB_YESNO) != IDYES)
      return 0;

    icq_sendRevokeAuthServ(dwUin, szUid);
  }

  return 0;
}



int IcqSendUserIsTyping(WPARAM wParam, LPARAM lParam)
{
  int nResult = 1;
  HANDLE hContact = (HANDLE)wParam;


  if (hContact && icqOnline)
  {
    if (CheckContactCapabilities(hContact, CAPF_TYPING))
    {
      switch (lParam)
      {
      case PROTOTYPE_SELFTYPING_ON:
        sendTypingNotification(hContact, MTN_BEGUN);
        nResult = 0;
        break;

      case PROTOTYPE_SELFTYPING_OFF:
        sendTypingNotification(hContact, MTN_FINISHED);
        nResult = 0;
        break;

      default:
        break;
      }
    }
  }

  return nResult;
}



/*
   ---------------------------------
   |           Receiving           |
   ---------------------------------
*/

int IcqRecvAwayMsg(WPARAM wParam,LPARAM lParam)
{
  CCSDATA* ccs = (CCSDATA*)lParam;
  PROTORECVEVENT* pre = (PROTORECVEVENT*)ccs->lParam;


  ICQBroadcastAck(ccs->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS,
    (HANDLE)pre->lParam, (LPARAM)pre->szMessage);

  return 0;
}



int IcqRecvMessage(WPARAM wParam, LPARAM lParam)
{
  DBEVENTINFO dbei;
  CCSDATA* ccs = (CCSDATA*)lParam;
  PROTORECVEVENT* pre = (PROTORECVEVENT*)ccs->lParam;


  SetContactHidden(ccs->hContact, 0);

  ZeroMemory(&dbei, sizeof(dbei));
  dbei.cbSize = sizeof(dbei);
  dbei.szModule = gpszICQProtoName;
  dbei.timestamp = pre->timestamp;
  dbei.flags = (pre->flags & PREF_CREATEREAD) ? DBEF_READ : 0;
  dbei.eventType = EVENTTYPE_MESSAGE;
  dbei.cbBlob = strlennull(pre->szMessage) + 1;
  if ( pre->flags & PREF_UNICODE )
    dbei.cbBlob *= ( sizeof( wchar_t )+1 );
  dbei.pBlob = (PBYTE)pre->szMessage;

  CallService(MS_DB_EVENT_ADD, (WPARAM)ccs->hContact, (LPARAM)&dbei);

  // stop contact from typing - some clients do not sent stop notify
  if (CheckContactCapabilities(ccs->hContact, CAPF_TYPING))
    CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)ccs->hContact, PROTOTYPE_CONTACTTYPING_OFF);

  return 0;
}



int IcqRecvUrl(WPARAM wParam, LPARAM lParam)
{
  DBEVENTINFO dbei;
  CCSDATA* ccs = (CCSDATA*)lParam;
  PROTORECVEVENT* pre = (PROTORECVEVENT*)ccs->lParam;
  char* szDesc;


  SetContactHidden(ccs->hContact, 0);

  szDesc = pre->szMessage + strlennull(pre->szMessage) + 1;

  ZeroMemory(&dbei, sizeof(dbei));
  dbei.cbSize = sizeof(dbei);
  dbei.szModule = gpszICQProtoName;
  dbei.timestamp = pre->timestamp;
  dbei.flags = (pre->flags & PREF_CREATEREAD) ? DBEF_READ : 0;
  dbei.eventType = EVENTTYPE_URL;
  dbei.cbBlob = strlennull(pre->szMessage) + strlennull(szDesc) + 2;
  dbei.pBlob = (PBYTE)pre->szMessage;

  CallService(MS_DB_EVENT_ADD, (WPARAM)ccs->hContact, (LPARAM)&dbei);

  return 0;
}



int IcqRecvContacts(WPARAM wParam, LPARAM lParam)
{
  DBEVENTINFO dbei = {0};
  CCSDATA* ccs = (CCSDATA*)lParam;
  PROTORECVEVENT* pre = (PROTORECVEVENT*)ccs->lParam;
  ICQSEARCHRESULT** isrList = (ICQSEARCHRESULT**)pre->szMessage;
  int i;
  char szUin[UINMAXLEN];
  PBYTE pBlob;


  SetContactHidden(ccs->hContact, 0);

  for (i = 0; i < pre->lParam; i++)
  {
    dbei.cbBlob += strlennull(isrList[i]->hdr.nick) + 2; // both trailing zeros
    if (isrList[i]->uin)
      dbei.cbBlob += getUINLen(isrList[i]->uin);
    else 
      dbei.cbBlob += strlennull(isrList[i]->uid);
  }
  dbei.cbSize = sizeof(dbei);
  dbei.szModule = gpszICQProtoName;
  dbei.timestamp = pre->timestamp;
  dbei.flags = (pre->flags & PREF_CREATEREAD) ? DBEF_READ : 0;
  dbei.eventType = EVENTTYPE_CONTACTS;
  dbei.pBlob = (PBYTE)_alloca(dbei.cbBlob);
  for (i = 0, pBlob = dbei.pBlob; i < pre->lParam; i++)
  {
    strcpy(pBlob, isrList[i]->hdr.nick);
    pBlob += strlennull(pBlob) + 1;
    if (isrList[i]->uin)
    {
      _itoa(isrList[i]->uin, szUin, 10);
      strcpy(pBlob, szUin);
    }
    else // aim contact
      strcpy(pBlob, isrList[i]->uid);
    pBlob += strlennull(pBlob) + 1;
  }

  CallService(MS_DB_EVENT_ADD, (WPARAM)ccs->hContact, (LPARAM)&dbei);

  return 0;
}



int IcqRecvFile(WPARAM wParam, LPARAM lParam)
{
  DBEVENTINFO dbei;
  CCSDATA* ccs = (CCSDATA*)lParam;
  PROTORECVEVENT* pre = (PROTORECVEVENT*)ccs->lParam;
  char* szDesc;
  char* szFile;


  SetContactHidden(ccs->hContact, 0);

  szFile = pre->szMessage + sizeof(DWORD);
  szDesc = szFile + strlennull(szFile) + 1;

  ZeroMemory(&dbei, sizeof(dbei));
  dbei.cbSize = sizeof(dbei);
  dbei.szModule = gpszICQProtoName;
  dbei.timestamp = pre->timestamp;
  dbei.flags = (pre->flags & PREF_CREATEREAD) ? DBEF_READ : 0;
  dbei.eventType = EVENTTYPE_FILE;
  dbei.cbBlob = sizeof(DWORD) + strlennull(szFile) + strlennull(szDesc) + 2;
  dbei.pBlob = (PBYTE)pre->szMessage;

  CallService(MS_DB_EVENT_ADD, (WPARAM)ccs->hContact, (LPARAM)&dbei);

  return 0;
}



int IcqRecvAuth(WPARAM wParam, LPARAM lParam)
{
  DBEVENTINFO dbei;
  CCSDATA* ccs = (CCSDATA*)lParam;
  PROTORECVEVENT* pre = (PROTORECVEVENT*)ccs->lParam;


  SetContactHidden(ccs->hContact, 0);

  ZeroMemory(&dbei, sizeof(dbei));
  dbei.cbSize = sizeof(dbei);
  dbei.szModule = gpszICQProtoName;
  dbei.timestamp = pre->timestamp;
  dbei.flags=(pre->flags & PREF_CREATEREAD) ? DBEF_READ : 0;
  dbei.eventType = EVENTTYPE_AUTHREQUEST;
  dbei.cbBlob = pre->lParam;
  dbei.pBlob = (PBYTE)pre->szMessage;

  CallService(MS_DB_EVENT_ADD, (WPARAM)NULL, (LPARAM)&dbei);

  return 0;
}
