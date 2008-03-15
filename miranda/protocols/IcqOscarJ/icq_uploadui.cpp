// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
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
//  Implements Manage Server List dialog
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


static int bListInit = 0;
static HANDLE hItemAll;
static int dwUploadDelay = 1000; // initial setting, it is too low for icq server but good for short updates

static HWND hwndUploadContacts=NULL;
static const UINT settingsControls[]={IDOK};

static WORD* pwGroupIds = NULL;
static int cbGroupIds = 0;

// Init default clist options
static void ResetCListOptions(HWND hwndList)
{
  int i;

  SendMessage(hwndList, CLM_SETBKBITMAP, 0, (LPARAM)(HBITMAP)NULL);
  SendMessage(hwndList, CLM_SETBKCOLOR, GetSysColor(COLOR_WINDOW), 0);
  SendMessage(hwndList, CLM_SETGREYOUTFLAGS, 0, 0);
  SendMessage(hwndList, CLM_SETLEFTMARGIN, 2, 0);
  SendMessage(hwndList, CLM_SETINDENT, 10, 0);
  for(i=0; i<=FONTID_MAX; i++)
    SendMessage(hwndList, CLM_SETTEXTCOLOR, i, GetSysColor(COLOR_WINDOWTEXT));
  SetWindowLong(hwndList, GWL_STYLE, GetWindowLong(hwndList, GWL_STYLE)|CLS_SHOWHIDDEN);
  if (CallService(MS_CLUI_GETCAPS, 0, 0) & CLUIF_HIDEEMPTYGROUPS) // hide empty groups
    SendMessage(hwndList, CLM_SETHIDEEMPTYGROUPS, (WPARAM) TRUE, 0);
}


// Selects the "All contacts" checkbox if all other list entries
// are selected, deselects it if not.
static void UpdateAllContactsCheckmark(HWND hwndList, HANDLE phItemAll)
{
  int check = 1;
  HANDLE hContact;
  HANDLE hItem;

  hContact = ICQFindFirstContact();

  while (hContact)
  {
    hItem = (HANDLE)SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
    if (hItem)
    {
      if (!SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0))
      { // if any of our contacts is unchecked, uncheck all contacts as well
        check = 0;
        break;
      }
    }
    hContact = ICQFindNextContact(hContact);
  }

  SendMessage(hwndList, CLM_SETCHECKMARK, (WPARAM)phItemAll, check);
}



// Loop over all contacts and update the checkmark
// that indicates wether or not they are already uploaded
static int UpdateCheckmarks(HWND hwndList, HANDLE phItemAll)
{
  int bAll = 1;
  HANDLE hContact;
  HANDLE hItem;

  bListInit = 1; // lock CLC events
 
  hContact = ICQFindFirstContact();
  while (hContact)
  {
    hItem = (HANDLE)SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
    if (hItem)
    {
      if (ICQGetContactSettingWord(hContact, "ServerId", 0))
        SendMessage(hwndList, CLM_SETCHECKMARK, (WPARAM)hItem, 1);
      else
        bAll = 0;
    }
    hContact = ICQFindNextContact(hContact);
  }

  // Update the "All contacts" checkmark
  if (phItemAll)
    SendMessage(hwndList, CLM_SETCHECKMARK, (WPARAM)phItemAll, bAll);

  bListInit = 0;

  return bAll;
}



static void DeleteOtherContactsFromControl(HWND hCtrl)
{
  HANDLE hContact;
  HANDLE hItem;

  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
  while (hContact)
  {
    hItem = (HANDLE)SendMessage(hCtrl, CLM_FINDCONTACT, (WPARAM)hContact, 0);
    if (hItem)
    {
      if (!IsICQContact(hContact))
        SendMessage(hCtrl, CLM_DELETEITEM, (WPARAM)hItem, 0);
    }
    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
  }
}



static void AppendToUploadLog(HWND hwndDlg, const unsigned char *fmt, ...)
{
  va_list va;
  unsigned char szText[1024];
  int iItem;

  va_start(va, fmt);
  mir_vsnprintf((char*)szText, sizeof(szText), (char*)fmt, va);
  va_end(va);

  iItem = ListBoxAddStringUtf(GetDlgItem(hwndDlg, IDC_LOG), szText);
  SendDlgItemMessage(hwndDlg, IDC_LOG, LB_SETTOPINDEX, iItem, 0);
}



static void DeleteLastUploadLogLine(HWND hwndDlg)
{
  SendDlgItemMessage(hwndDlg, IDC_LOG, LB_DELETESTRING, SendDlgItemMessage(hwndDlg, IDC_LOG, LB_GETCOUNT, 0, 0)-1, 0);
}



static void GetLastUploadLogLine(HWND hwndDlg, unsigned char *szBuf, size_t cbBuf)
{
  if (gbUnicodeAPI)
  {
    WCHAR str[MAX_PATH];

    SendDlgItemMessageW(hwndDlg, IDC_LOG, LB_GETTEXT, SendDlgItemMessage(hwndDlg, IDC_LOG, LB_GETCOUNT, 0, 0)-1, (LPARAM)str);
    make_utf8_string_static(str, szBuf, cbBuf);
  }
  else
  {
    char str[MAX_PATH];
    unsigned char *ustr = NULL;

    SendDlgItemMessageA(hwndDlg, IDC_LOG, LB_GETTEXT, SendDlgItemMessage(hwndDlg, IDC_LOG, LB_GETCOUNT, 0, 0)-1, (LPARAM)str);
    utf8_encode(str, &ustr);
    strcpy((char*)szBuf, (char*)ustr);
    SAFE_FREE((void**)&ustr);
  }
}



static int GroupEnumIdsEnumProc(const char *szSetting,LPARAM lParam)
{ 
  if (szSetting && strlennull(szSetting)<5)
  { // it is probably server group
    char val[MAX_PATH+2]; // dummy
    DBVARIANT dbv;
    DBCONTACTGETSETTING cgs;

    dbv.type = DBVT_ASCIIZ;
    dbv.pszVal = val;
    dbv.cchVal = MAX_PATH;

    cgs.szModule=(char*)lParam;
    cgs.szSetting=szSetting;
    cgs.pValue=&dbv;
    if(CallService(MS_DB_CONTACT_GETSETTINGSTATIC,(WPARAM)NULL,(LPARAM)&cgs))
      return 0; // this converts all string types to DBVT_ASCIIZ
    if(dbv.type!=DBVT_ASCIIZ)
    { // it is not a cached server-group name
      return 0;
    }
    pwGroupIds = (WORD*)SAFE_REALLOC(pwGroupIds, (cbGroupIds+1)*sizeof(WORD));
    pwGroupIds[cbGroupIds] = (WORD)strtoul(szSetting, NULL, 0x10);
    cbGroupIds++;
  }
  return 0;
}



static void enumServerGroups()
{
  DBCONTACTENUMSETTINGS dbces;

  char szModule[MAX_PATH+9];

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "SrvGroups");

  dbces.pfnEnumProc = &GroupEnumIdsEnumProc;
  dbces.szModule = szModule;
  dbces.lParam = (LPARAM)szModule;

  CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)NULL, (LPARAM)&dbces);
}



static DWORD sendUploadGroup(WORD wAction, WORD wGroupId, unsigned char* szItemName)
{
  DWORD dwCookie;
  servlistcookie* ack;

  if (ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie)))
  { // we have cookie good, go on
    ack->wGroupId = wGroupId;
    ack->dwAction = SSA_SERVLIST_ACK;
    dwCookie = AllocateCookie(CKT_SERVERLIST, wAction, 0, ack);
    ack->lParam = dwCookie;

    icq_sendGroupUtf(dwCookie, wAction, ack->wGroupId, szItemName, NULL, 0);

    return dwCookie;
  }
  return 0;
}

static DWORD sendUploadBuddy(HANDLE hContact, WORD wAction, DWORD dwUin, char *szUID, WORD wContactId, WORD wGroupId, WORD wItemType)
{
  DWORD dwCookie;
  servlistcookie* ack;

  if (ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie)))
  { // we have cookie good, go on
    ack->hContact = hContact;
    ack->wContactId = wContactId;
    ack->wGroupId = wGroupId;
    ack->dwAction = SSA_SERVLIST_ACK;
    ack->dwUin = dwUin;
    dwCookie = AllocateCookie(CKT_SERVERLIST, wAction, hContact, ack);
    ack->lParam = dwCookie;

    if (wItemType == SSI_ITEM_BUDDY)
      icq_sendServerContact(hContact, dwCookie, wAction, ack->wGroupId, ack->wContactId);
    else
      icq_sendSimpleItem(dwCookie, wAction, dwUin, szUID, ack->wGroupId, ack->wContactId, wItemType);

    return dwCookie;
  }
  return 0;
}

static unsigned char* getServerResultDesc(int wCode)
{
  switch (wCode)
  {
    case 0:   return LPGENUTF("OK");
    case 2:   return LPGENUTF("NOT FOUND");
    case 3:   return LPGENUTF("ALREADY EXISTS");
    case 0xA: return LPGENUTF("INVALID DATA");
    case 0xC: return LPGENUTF("LIST FULL");
    default:  return LPGENUTF("FAILED");
  }
}

#define ACTION_NONE             0
#define ACTION_ADDBUDDY         1
#define ACTION_ADDBUDDYAUTH     2
#define ACTION_REMOVEBUDDY      3
#define ACTION_ADDGROUP         4
#define ACTION_REMOVEGROUP      5
#define ACTION_UPDATESTATE      6
#define ACTION_MOVECONTACT      7
#define ACTION_ADDVISIBLE       8
#define ACTION_REMOVEVISIBLE    9
#define ACTION_ADDINVISIBLE     10
#define ACTION_REMOVEINVISIBLE  11

#define STATE_READY         1
#define STATE_REGROUP       2
#define STATE_ITEMS         3
#define STATE_VISIBILITY    5
#define STATE_CONSOLIDATE   4

#define M_PROTOACK      (WM_USER+100)
#define M_UPLOADMORE    (WM_USER+101)
#define M_INITCLIST     (WM_USER+102)
static BOOL CALLBACK DlgProcUploadList(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
  static int working;
  static HANDLE hProtoAckHook;
  static int currentSequence;
  static int currentAction;
  static int currentState;
  static HANDLE hCurrentContact;
  static int lastAckResult = 0;
  static WORD wNewContactId;
  static WORD wNewGroupId;
  static unsigned char *szNewGroupName;
  static WORD wNewVisibilityId;

  switch(message)
  {

  case WM_INITDIALOG:
    {
      unsigned char str[MAX_PATH];

      ICQTranslateDialog(hwndDlg);
      working = 0;
      hProtoAckHook = NULL;
      currentState = STATE_READY;

      ResetCListOptions(GetDlgItem(hwndDlg, IDC_CLIST));

      AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Select contacts you want to store on server."), str, MAX_PATH));
      AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Ready..."), str, MAX_PATH));
    }
    return TRUE;

    // The M_PROTOACK message is received when the
    // server has responded to our last update packet
  case M_PROTOACK:
    {
      int bMulti = 0;
      ACKDATA *ack = (ACKDATA*)lParam;
      unsigned char szLastLogLine[MAX_PATH];
      unsigned char str[MAX_PATH];

      // Is this an ack we are waiting for?
      if (strcmpnull(ack->szModule, gpszICQProtoName))
        break;

      if (ack->type == ICQACKTYPE_RATEWARNING)
      { // we are sending tooo fast, slow down the process
        if (ack->hProcess != (HANDLE)1) break; // check class
        if (ack->lParam == 2 || ack->lParam == 3) // check status
        {
          GetLastUploadLogLine(hwndDlg, szLastLogLine, MAX_PATH);
          DeleteLastUploadLogLine(hwndDlg);
          AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Server rate warning -> slowing down the process."), str, MAX_PATH));
          AppendToUploadLog(hwndDlg, szLastLogLine);

          dwUploadDelay *= 2;

          break;
        }
        if (ack->lParam == 4) dwUploadDelay /= 2; // the rate is ok, turn up
      }

      if (ack->type != ICQACKTYPE_SERVERCLIST)
        break;

      if ((int)ack->hProcess != currentSequence)
        break;

      lastAckResult = ack->result == ACKRESULT_SUCCESS ? 0 : 1;

      switch (currentAction)
      {
      case ACTION_ADDBUDDY:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          ICQWriteContactSettingByte(hCurrentContact, "Auth", 0);
          ICQWriteContactSettingWord(hCurrentContact, "ServerId", wNewContactId);
          ICQWriteContactSettingWord(hCurrentContact, "SrvGroupId", wNewGroupId);
          break;
        }
        else
        { // If the server refused to add the contact without authorization,
          // we try again _with_ authorization TLV
          DWORD dwUIN;
          uid_str szUID;

          ICQWriteContactSettingByte(hCurrentContact, "Auth", 1);

          if (!ICQGetContactSettingUID(hCurrentContact, &dwUIN, &szUID))
          {
            currentAction = ACTION_ADDBUDDYAUTH;
            currentSequence = sendUploadBuddy(hCurrentContact, ICQ_LISTS_ADDTOLIST, dwUIN, szUID, wNewContactId, wNewGroupId, SSI_ITEM_BUDDY);
          }

          return FALSE;
        }
        
      case ACTION_ADDBUDDYAUTH:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          ICQWriteContactSettingWord(hCurrentContact, "ServerId", wNewContactId);
          ICQWriteContactSettingWord(hCurrentContact, "SrvGroupId", wNewGroupId);
        }
        else
        {
          ICQDeleteContactSetting(hCurrentContact, "Auth");
          FreeServerID(wNewContactId, SSIT_ITEM);
        }

        break;

      case ACTION_REMOVEBUDDY:
        if (ack->result == ACKRESULT_SUCCESS)
        { // clear obsolete settings
          FreeServerID(wNewContactId, SSIT_ITEM);
          ICQDeleteContactSetting(hCurrentContact, "ServerId");
          ICQDeleteContactSetting(hCurrentContact, "SrvGroupId");
          ICQDeleteContactSetting(hCurrentContact, "Auth");
        }
        break;

      case ACTION_ADDGROUP:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          void* groupData;
          int groupSize;
          servlistcookie* ack;

          setServListGroupName(wNewGroupId, szNewGroupName); // add group to list
          setServListGroupLinkID(szNewGroupName, wNewGroupId); // grouppath is known

          groupData = collectGroups(&groupSize);
          groupData = SAFE_REALLOC(groupData, groupSize+2);
          *(((WORD*)groupData)+(groupSize>>1)) = wNewGroupId; // add this new group id
          groupSize += 2;

          ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));
          if (ack)
          {
            DWORD dwCookie; // we do not use this

            ack->dwAction = SSA_SERVLIST_ACK;
            dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_UPDATEGROUP, 0, ack);

            icq_sendGroupUtf(dwCookie, ICQ_LISTS_UPDATEGROUP, 0, ack->szGroupName, groupData, groupSize);
          }
          SAFE_FREE((void**)&groupData);
        }
        else
          FreeServerID(wNewGroupId, SSIT_GROUP);

        SAFE_FREE((void**)&szNewGroupName);
        break;

      case ACTION_REMOVEGROUP:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          void* groupData;
          int groupSize;
          servlistcookie* ack;

          FreeServerID(wNewGroupId, SSIT_GROUP);
          setServListGroupName(wNewGroupId, NULL); // remove group from list
          removeGroupPathLinks(wNewGroupId); // grouppath is known

          groupData = collectGroups(&groupSize);

          ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));
          if (ack)
          {
            DWORD dwCookie; // we do not use this

            ack->dwAction = SSA_SERVLIST_ACK;
            dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_UPDATEGROUP, 0, ack);

            icq_sendGroupUtf(dwCookie, ICQ_LISTS_UPDATEGROUP, 0, ack->szGroupName, groupData, groupSize);
          }
          SAFE_FREE((void**)&groupData);
        }
        break;

      case ACTION_UPDATESTATE:
        // do nothing
        break;

      case ACTION_MOVECONTACT:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          FreeServerID(ICQGetContactSettingWord(hCurrentContact, "ServerId", 0), SSIT_ITEM);
          ICQWriteContactSettingWord(hCurrentContact, "ServerId", wNewContactId);
          ICQWriteContactSettingWord(hCurrentContact, "SrvGroupId", wNewGroupId);
          dwUploadDelay *= 2; // we double the delay here (2 packets)
        }
        break;

      case ACTION_ADDVISIBLE:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          ICQWriteContactSettingWord(hCurrentContact, "SrvPermitId", wNewContactId);
        }
        else
          FreeServerID(wNewContactId, SSIT_ITEM);
        break;

      case ACTION_ADDINVISIBLE:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          ICQWriteContactSettingWord(hCurrentContact, "SrvDenyId", wNewContactId);
        }
        else
          FreeServerID(wNewContactId, SSIT_ITEM);
        break;

      case ACTION_REMOVEVISIBLE:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          FreeServerID(wNewContactId, SSIT_ITEM);
          ICQWriteContactSettingWord(hCurrentContact, "SrvPermitId", 0);
        }
        break;

      case ACTION_REMOVEINVISIBLE:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          FreeServerID(wNewContactId, SSIT_ITEM);
          ICQWriteContactSettingWord(hCurrentContact, "SrvDenyId", 0);
        }
        break;
      }
      
      // Update the log window
      GetLastUploadLogLine(hwndDlg, szLastLogLine, MAX_PATH);
      DeleteLastUploadLogLine(hwndDlg);
      AppendToUploadLog(hwndDlg, (unsigned char*)"%s%s", szLastLogLine,
        ICQTranslateUtfStatic(getServerResultDesc(ack->lParam), str, MAX_PATH));

      if (!bMulti) 
      {
        SetTimer(hwndDlg, M_UPLOADMORE, dwUploadDelay, 0); // delay
      }
    }
    break;

  case WM_TIMER:
    {
      switch (wParam) 
      { 
        case M_UPLOADMORE: 
          KillTimer(hwndDlg, M_UPLOADMORE);
          if (currentAction == ACTION_MOVECONTACT)
            dwUploadDelay /= 2; // turn it back

          PostMessage(hwndDlg, M_UPLOADMORE, 0, 0);

          return 0;
      }
    }

    // The M_UPLOADMORE window message is received when the user presses 'Update'
    // and every time an ack from the server has been taken care of.
  case M_UPLOADMORE:
    {
      HANDLE hContact;
      HANDLE hItem;
      DWORD dwUin;
      uid_str szUid;
      unsigned char *pszNick;
      unsigned char *pszGroup;
      int isChecked;
      int isOnServer;
      BOOL bUidOk;
      unsigned char str[MAX_PATH];
      HWND hwndList = GetDlgItem(hwndDlg, IDC_CLIST);

      switch (currentState)
      {
      case STATE_REGROUP:

      // TODO: iterate over all checked groups and create if needed
      // if creation requires reallocation of groups do it here

        currentState = STATE_ITEMS;
        hCurrentContact = NULL;
        PostMessage(hwndDlg, M_UPLOADMORE, 0, 0);
        break;

      case STATE_ITEMS:
        // Iterate over all contacts until one is found that
        // needs to be updated on the server
        if (hCurrentContact == NULL)
          hContact = ICQFindFirstContact();
        else // we do not want to go thru all contacts over and over again
        {
          hContact = hCurrentContact;
          if (lastAckResult) // if the last operation on this contact fail, do not do it again, go to next
            hContact = ICQFindNextContact(hContact);
        }

        while (hContact)
        {
          hCurrentContact = hContact;

          hItem = (HANDLE)SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
          if (hItem)
          {
            isChecked = SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0) != 0;
            isOnServer = ICQGetContactSettingWord(hContact, "ServerId", 0) != 0;

            bUidOk = !ICQGetContactSettingUID(hContact, &dwUin, &szUid);

            // Is this one out of sync?
            if (bUidOk && (isChecked != isOnServer))
            {
              // Only upload custom nicks
              pszNick = UniGetContactSettingUtf(hContact, "CList", "MyHandle", NULL);

              if (isChecked)
              {  // Queue for uploading
                pszGroup = ICQGetContactCListGroup(hContact);
                if (!strlennull(pszGroup)) pszGroup = (unsigned char*)null_strdup(DEFAULT_SS_GROUP);

                // Get group ID from cache, if not ready use parent group, if still not ready create one
                wNewGroupId = getServListGroupLinkID(pszGroup);
                if (!wNewGroupId && strstrnull(pszGroup, "\\") != NULL)
                { // if it is sub-group, take master parent
                  strstrnull(pszGroup, "\\")[0] = '\0'; 
                  wNewGroupId = getServListGroupLinkID(pszGroup);
                }
                if (!wNewGroupId && currentAction != ACTION_ADDGROUP)
                { // if the group still does not exist and there was no try before, try to add group
                  AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Adding group \"%s\"..."), str, MAX_PATH), pszGroup);

                  wNewGroupId = GenerateServerId(SSIT_GROUP);
                  szNewGroupName = pszGroup;
                  currentAction = ACTION_ADDGROUP;
                  currentSequence = sendUploadGroup(ICQ_LISTS_ADDTOLIST, wNewGroupId, pszGroup);
                  SAFE_FREE((void**)&pszNick);

                  return FALSE;
                }

                SAFE_FREE((void**)&pszGroup);

                AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Uploading %s..."), str, MAX_PATH), pszNick ? pszNick : (unsigned char*)strUID(dwUin, szUid));

                currentAction = ACTION_ADDBUDDY;

                if (wNewGroupId)
                {
                  wNewContactId = GenerateServerId(SSIT_ITEM);

                  currentSequence = sendUploadBuddy(hCurrentContact, ICQ_LISTS_ADDTOLIST, dwUin, szUid,
                    wNewContactId, wNewGroupId, SSI_ITEM_BUDDY);
                  SAFE_FREE((void**)&pszNick);

                  return FALSE;
                }
                else
                {
                  unsigned char szLastLogLine[MAX_PATH];
                  // Update the log window with the failure and continue with next contact
                  GetLastUploadLogLine(hwndDlg, szLastLogLine, MAX_PATH);
                  DeleteLastUploadLogLine(hwndDlg);
                  AppendToUploadLog(hwndDlg, (unsigned char*)"%s%s", szLastLogLine, ICQTranslateUtfStatic(LPGENUTF("FAILED"), str, MAX_PATH));
                  AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("No upload group available"), str, MAX_PATH));
                  NetLog_Server("Upload failed, no group");
                  currentState = STATE_READY;
                }
              }
              else
              { // Queue for deletion
                if (pszNick)
                  AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Deleting %s..."), str, MAX_PATH), pszNick);
                else
                  AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Deleting %s..."), str, MAX_PATH), strUID(dwUin, szUid));

                wNewGroupId = ICQGetContactSettingWord(hContact, "SrvGroupId", 0);
                wNewContactId = ICQGetContactSettingWord(hContact, "ServerId", 0);
                currentAction = ACTION_REMOVEBUDDY;
                currentSequence = sendUploadBuddy(hContact, ICQ_LISTS_REMOVEFROMLIST, dwUin, szUid,
                  wNewContactId, wNewGroupId, SSI_ITEM_BUDDY);
              }
              SAFE_FREE((void**)&pszNick);

              break;
            }
            else if (bUidOk && isChecked)
            { // the contact is and should be on server, check if it is in correct group, move otherwise
              WORD wCurrentGroupId = ICQGetContactSettingWord(hContact, "SrvGroupId", 0);

              pszGroup = ICQGetContactCListGroup(hContact);
              if (!strlennull(pszGroup)) pszGroup = (unsigned char*)null_strdup(DEFAULT_SS_GROUP);
              wNewGroupId = getServListGroupLinkID(pszGroup);
              if (!wNewGroupId && strstrnull(pszGroup, "\\") != NULL)
              { // if it is sub-group, take master parent
                strstrnull(pszGroup, "\\")[0] = '\0'; 
                wNewGroupId = getServListGroupLinkID(pszGroup);
              }
              if (!wNewGroupId && currentAction != ACTION_ADDGROUP)
              { // if the group still does not exist and there was no try before, try to add group
                AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Adding group \"%s\"..."), str, MAX_PATH), pszGroup);

                wNewGroupId = GenerateServerId(SSIT_GROUP);
                szNewGroupName = pszGroup;
                currentAction = ACTION_ADDGROUP;
                currentSequence = sendUploadGroup(ICQ_LISTS_ADDTOLIST, wNewGroupId, pszGroup);

                return FALSE;
              }
              if (wNewGroupId && (wNewGroupId != wCurrentGroupId))
              { // we have a group the contact should be in, move it
                WORD wCurrentContactId = ICQGetContactSettingWord(hContact, "ServerId", 0);
                BYTE bAuth = ICQGetContactSettingByte(hContact, "Auth", 0);

                pszNick = UniGetContactSettingUtf(hContact, "CList", "MyHandle", NULL);

                if (pszNick)
                  AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Moving %s to group \"%s\"..."), str, MAX_PATH), pszNick, pszGroup);
                else 
                  AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Moving %s to group \"%s\"..."), str, MAX_PATH), strUID(dwUin, szUid), pszGroup);

                currentAction = ACTION_MOVECONTACT;
                wNewContactId = GenerateServerId(SSIT_ITEM);
                sendUploadBuddy(hContact, ICQ_LISTS_REMOVEFROMLIST, dwUin, szUid, wCurrentContactId, wCurrentGroupId, SSI_ITEM_BUDDY);
                currentSequence = sendUploadBuddy(hContact, ICQ_LISTS_ADDTOLIST, dwUin, szUid, wNewContactId, wNewGroupId, SSI_ITEM_BUDDY);
                SAFE_FREE((void**)&pszNick);
                SAFE_FREE((void**)&pszGroup);

                break;
              }
              SAFE_FREE((void**)&pszGroup);
            }
          }
          hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
        }
        if (!hContact) 
        {
          currentState = STATE_VISIBILITY;
          hCurrentContact = NULL;
          PostMessage(hwndDlg, M_UPLOADMORE, 0, 0);
        }
        break;

      case STATE_VISIBILITY:
        // Iterate over all contacts until one is found that
        // needs to be updated on the server
        if (hCurrentContact == NULL)
          hContact = ICQFindFirstContact();
        else // we do not want to go thru all contacts over and over again
        {
          hContact = hCurrentContact;
          if (lastAckResult) // if the last operation on this contact fail, do not do it again, go to next
            hContact = ICQFindNextContact(hContact);
        }

        while (hContact)
        {
          WORD wApparentMode = ICQGetContactSettingWord(hContact, "ApparentMode", 0);
          WORD wDenyId = ICQGetContactSettingWord(hContact, "SrvDenyId", 0);
          WORD wPermitId = ICQGetContactSettingWord(hContact, "SrvPermitId", 0);
          WORD wIgnoreId = ICQGetContactSettingWord(hContact, "SrvIgnoreId", 0);

          hCurrentContact = hContact;
          ICQGetContactSettingUID(hContact, &dwUin, &szUid);

          if (wApparentMode == ID_STATUS_ONLINE)
          { // contact is on the visible list
            if (wPermitId == 0)
            {
              currentAction = ACTION_ADDVISIBLE;
              wNewContactId = GenerateServerId(SSIT_ITEM);
              AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Adding %s to visible list..."), str, MAX_PATH), strUID(dwUin, szUid));
              currentSequence = sendUploadBuddy(hContact, ICQ_LISTS_ADDTOLIST, dwUin, szUid, wNewContactId, 0, SSI_ITEM_PERMIT);
              break;
            }
          }
          if (wApparentMode == ID_STATUS_OFFLINE)
          { // contact is on the invisible list
            if (wDenyId == 0 && wIgnoreId == 0)
            {
              currentAction = ACTION_ADDINVISIBLE;
              wNewContactId = GenerateServerId(SSIT_ITEM);
              AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Adding %s to invisible list..."), str, MAX_PATH), strUID(dwUin, szUid));
              currentSequence = sendUploadBuddy(hContact, ICQ_LISTS_ADDTOLIST, dwUin, szUid, wNewContactId, 0, SSI_ITEM_DENY);
              break;
            }
          }
          if (wApparentMode != ID_STATUS_ONLINE)
          { // contact is not on visible list
            if (wPermitId != 0)
            {
              currentAction = ACTION_REMOVEVISIBLE;
              wNewContactId = wPermitId;
              AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Deleting %s from visible list..."), str, MAX_PATH), strUID(dwUin, szUid));
              currentSequence = sendUploadBuddy(hContact, ICQ_LISTS_REMOVEFROMLIST, dwUin, szUid, wNewContactId, 0, SSI_ITEM_PERMIT);
              break;
            }
          }
          if (wApparentMode != ID_STATUS_OFFLINE)
          { // contact is not on invisible list
            if (wDenyId != 0)
            {
              currentAction = ACTION_REMOVEINVISIBLE;
              wNewContactId = wDenyId;
              AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Deleting %s from invisible list..."), str, MAX_PATH), strUID(dwUin, szUid));
              currentSequence = sendUploadBuddy(hContact, ICQ_LISTS_REMOVEFROMLIST, dwUin, szUid, wNewContactId, 0, SSI_ITEM_DENY);
              break;
            }
          }
          hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
        }
        if (!hContact) 
        {
          currentState = STATE_CONSOLIDATE;
          AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Cleaning groups"), str, MAX_PATH));
          EnableDlgItem(hwndDlg, IDCANCEL, FALSE);
          enumServerGroups();
          PostMessage(hwndDlg, M_UPLOADMORE, 0, 0);
        }
        break;

      case STATE_CONSOLIDATE: // updage group data, remove redundant groups
        if (currentAction == ACTION_UPDATESTATE)
          DeleteLastUploadLogLine(hwndDlg);

        if (cbGroupIds) // some groups in the list
        {
          void* groupData;
          int groupSize;

          cbGroupIds--;
          wNewGroupId = pwGroupIds[cbGroupIds];

          if (groupData = collectBuddyGroup(wNewGroupId, &groupSize))
          { // the group is still not empty, just update it
            unsigned char* pszGroup = getServListGroupName(wNewGroupId);
            servlistcookie* ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));

            ack->dwAction = SSA_SERVLIST_ACK;
            ack->wGroupId = wNewGroupId;
            currentSequence = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_UPDATEGROUP, 0, ack);
            ack->lParam = currentSequence;
            currentAction = ACTION_UPDATESTATE;
            AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Updating group \"%s\"..."), str, MAX_PATH), pszGroup);

            icq_sendGroupUtf(currentSequence, ICQ_LISTS_UPDATEGROUP, wNewGroupId, pszGroup, groupData, groupSize);

            SAFE_FREE((void**)&pszGroup);
          }
          else // the group is empty, delete it if it does not have sub-groups
          {
            if (!CheckServerID((WORD)(wNewGroupId+1), 0) || getServListGroupLevel((WORD)(wNewGroupId+1)) == 0)
            { // is next id an sub-group, if yes, we cannot delete this group
              unsigned char *pszGroup = getServListGroupName(wNewGroupId);
              currentAction = ACTION_REMOVEGROUP;
              AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Deleting group \"%s\"..."), str, MAX_PATH), pszGroup); 
              currentSequence = sendUploadGroup(ICQ_LISTS_REMOVEFROMLIST, wNewGroupId, pszGroup);
              SAFE_FREE((void**)&pszGroup);
            }
            else // update empty group
            {
              unsigned char *pszGroup = getServListGroupName(wNewGroupId);
              servlistcookie *ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));

              ack->dwAction = SSA_SERVLIST_ACK;
              ack->wGroupId = wNewGroupId;
              currentSequence = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_UPDATEGROUP, 0, ack);
              ack->lParam = currentSequence;
              currentAction = ACTION_UPDATESTATE;
              AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("Updating group \"%s\"..."), str, MAX_PATH), pszGroup);

              icq_sendGroupUtf(currentSequence, ICQ_LISTS_UPDATEGROUP, wNewGroupId, pszGroup, 0, 0);

              SAFE_FREE((void**)&pszGroup);
            }
          }
          SAFE_FREE((void**)&groupData); // free the memory
        }
        else
        { // all groups processed
          SAFE_FREE((void**)&pwGroupIds);
          currentState = STATE_READY;
        }
        break;
      }

      if (currentState == STATE_READY)
      {
        // All contacts are in sync
        AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("All operations complete"), str, MAX_PATH));
        EnableDlgItem(hwndDlg, IDCANCEL, TRUE);
        SetDlgItemTextUtf(hwndDlg, IDCANCEL, ICQTranslateUtfStatic(LPGENUTF("Close"), str, MAX_PATH));
        sendAddEnd();
        working = 0;
//        SendMessage(hwndList, CLM_SETGREYOUTFLAGS,0,0);
        UpdateCheckmarks(hwndList, hItemAll);
//        EnableWindow(hwndList, FALSE);
        if (hProtoAckHook)
          UnhookEvent(hProtoAckHook);
      }
      break;
    }
    
    
  case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
      case IDOK:
        SendDlgItemMessage(hwndDlg, IDC_LOG, LB_RESETCONTENT, 0, 0);
        if (!icqOnline)
        {
          unsigned char str[MAX_PATH];

          AppendToUploadLog(hwndDlg, ICQTranslateUtfStatic(LPGENUTF("You have to be online to sychronize the server-list !"), str, MAX_PATH));
          break;
        }
        working = 1;
        hCurrentContact = NULL;
        currentState = STATE_REGROUP;
        currentAction = ACTION_NONE;
        icq_ShowMultipleControls(hwndDlg, settingsControls, SIZEOF(settingsControls), SW_HIDE);
//        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETGREYOUTFLAGS, 0xFFFFFFFF, 0);
//        InvalidateRect(GetDlgItem(hwndDlg, IDC_CLIST), NULL, FALSE);
        EnableDlgItem(hwndDlg, IDC_CLIST, FALSE);
        hProtoAckHook = HookEventMessage(ME_PROTO_ACK, hwndDlg, M_PROTOACK);
        sendAddStart(1);
        PostMessage(hwndDlg, M_UPLOADMORE, 0, 0);
        break;
        
      case IDCANCEL: // TODO: this must be clean
        DestroyWindow(hwndDlg);
        break;
      }
    }
    break;
    
  case WM_NOTIFY:
    {
      switch(((NMHDR*)lParam)->idFrom)
      {
        
      case IDC_CLIST:
        {
          HWND hClist = GetDlgItem(hwndDlg, IDC_CLIST);

          switch(((NMHDR*)lParam)->code)
          {
            
          case CLN_OPTIONSCHANGED:
            {
              ResetCListOptions(hClist);
            }
            break;
            
          case CLN_NEWCONTACT:
          case CLN_CONTACTMOVED:
            {
              // Delete non-icq contacts
              DeleteOtherContactsFromControl(hClist);
              if (hItemAll)
                UpdateAllContactsCheckmark(hClist, hItemAll);
            }
            break;

          case CLN_LISTREBUILT:
            {
              int bCheck;

              // Delete non-icq contacts
              DeleteOtherContactsFromControl(hClist);

              if (!bListInit) // do not enter twice
                bCheck = UpdateCheckmarks(hClist, NULL);

              if (!hItemAll) // Add the "All contacts" item
              {
                CLCINFOITEM cii = {0};

                cii.cbSize = sizeof(cii);
                cii.flags = CLCIIF_GROUPFONT | CLCIIF_CHECKBOX;
                cii.pszText = ICQTranslate("** All contacts **");
                hItemAll = (HANDLE)SendMessage(hClist, CLM_ADDINFOITEM, 0, (LPARAM)&cii);
              }

              SendMessage(hClist, CLM_SETCHECKMARK, (WPARAM)hItemAll, bCheck);
            }
            break;
            
          case CLN_CHECKCHANGED:
            {
              NMCLISTCONTROL *nm = (NMCLISTCONTROL*)lParam;
              HANDLE hContact;
              HANDLE hItem;

              if (bListInit) break;

              if (nm->flags&CLNF_ISINFO)
              {
                int check;

                check = SendMessage(hClist, CLM_GETCHECKMARK, (WPARAM)hItemAll, 0);
                
                hContact = ICQFindFirstContact();
                while (hContact)
                {
                  hItem = (HANDLE)SendMessage(hClist, CLM_FINDCONTACT, (WPARAM)hContact, 0);
                  if (hItem)
                    SendMessage(hClist, CLM_SETCHECKMARK, (WPARAM)hItem, check);
                  hContact = ICQFindNextContact(hContact);
                }
              }
              else
              {
                UpdateAllContactsCheckmark(hClist, hItemAll);
              }
            }
            break;
          }
        }
        break;
      }
    }
    break;
    
  case WM_CLOSE:
    DestroyWindow(hwndDlg);
    break;

  case WM_DESTROY:
    if (hProtoAckHook)
      UnhookEvent(hProtoAckHook);
    if (working)
      sendAddEnd();
    hwndUploadContacts = NULL;
    working = 0;
    break;
  }

  return FALSE;
}



void ShowUploadContactsDialog(void)
{
  if (hwndUploadContacts == NULL)
  {
    hItemAll = NULL;
    hwndUploadContacts = CreateDialogUtf(hInst, MAKEINTRESOURCE(IDD_ICQUPLOADLIST), NULL, DlgProcUploadList);
  }

  SetForegroundWindow(hwndUploadContacts);
}
