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
//  Implements Manage Server List dialog
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



extern char gpszICQProtoName[MAX_PATH];
extern HANDLE hInst;
extern HANDLE ghServerNetlibUser;
extern int gnCurrentStatus;

static int bListInit = 0;
static HANDLE hItemAll;

static HWND hwndUploadContacts=NULL;
static const UINT settingsControls[]={IDOK};

static WORD* pwGroupIds = NULL;
static int cbGroupIds = 0;

// Selects the "All contacts" checkbox if all other list entries
// are selected, deselects it if not.
static void UpdateAllContactsCheckmark(HWND hwndList, HANDLE phItemAll)
{
  int check = 1;
  HANDLE hContact;
  HANDLE hItem;
  char* szProto;

  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

  while (hContact)
  {
    hItem = (HANDLE)SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
    if (hItem)
    {
      szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
      if (szProto && !lstrcmp(szProto, gpszICQProtoName))
        if (!SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0))
        { // if any of our contacts is unchecked, uncheck all contacts as well
          check = 0;
          break;
        }
    }
    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
  }

  SendMessage(hwndList, CLM_SETCHECKMARK, (WPARAM)phItemAll, check);
}



// Loop over all contacts and update the checkmark
// that indicates wether or not they are already uploaded
static int UpdateCheckmarks(HWND hwndDlg, HANDLE phItemAll)
{
  int bAll = 1;
  HANDLE hContact;
  HANDLE hItem;
  char* szProto;

  bListInit = 1; // lock CLC events
 
  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
  while (hContact)
  {
    hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0);
     if (hItem)
    {
      szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
      if (szProto && !lstrcmp(szProto, gpszICQProtoName))
      {
        if (DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0))
          SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM)hItem, 1);
        else
          bAll = 0;
      }
    }
    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
  }

  // Update the "All contacts" checkmark
  if (phItemAll)
    SendMessage(hwndDlg, CLM_SETCHECKMARK, (WPARAM)phItemAll, bAll);

  bListInit = 0;

  return bAll;
}



static void AppendToUploadLog(HWND hwndDlg, const char *fmt, ...)
{
  va_list va;
  char szText[1024];
  int iItem;

  va_start(va, fmt);
  _vsnprintf(szText, sizeof(szText), fmt, va);
  va_end(va);

  iItem = SendDlgItemMessage(hwndDlg, IDC_LOG, LB_ADDSTRING, 0, (LPARAM)szText);
  SendDlgItemMessage(hwndDlg, IDC_LOG, LB_SETTOPINDEX, iItem, 0);
}



static void DeleteLastUploadLogLine(HWND hwndDlg)
{
  SendDlgItemMessage(hwndDlg, IDC_LOG, LB_DELETESTRING, SendDlgItemMessage(hwndDlg, IDC_LOG, LB_GETCOUNT, 0, 0)-1, 0);
}



static void GetLastUploadLogLine(HWND hwndDlg, char *szBuf)
{
  SendDlgItemMessage(hwndDlg, IDC_LOG, LB_GETTEXT, SendDlgItemMessage(hwndDlg, IDC_LOG, LB_GETCOUNT, 0, 0)-1, (LPARAM)szBuf);
}


static int GroupEnumIdsEnumProc(const char *szSetting,LPARAM lParam)
{ 
  if (szSetting && strlen(szSetting)<5)
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
      return 0;
    if(dbv.type!=DBVT_ASCIIZ)
    { // it is not a cached server-group name
      return 0;
    }
    pwGroupIds = (WORD*)realloc(pwGroupIds, (cbGroupIds+1)*sizeof(WORD));
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


static DWORD sendUploadGroup(WORD wAction, WORD wGroupId, char* szItemName)
{
  DWORD dwCookie;
  servlistcookie* ack;

  if (ack = (servlistcookie*)malloc(sizeof(servlistcookie)))
  { // we have cookie good, go on
    ack->hContact = NULL;
    ack->wContactId = 0;
    ack->wGroupId = wGroupId;
    ack->dwAction = SSA_SERVLIST_ACK;
    ack->dwUin = 0;
    dwCookie = AllocateCookie(wAction, 0, ack);
    ack->lParam = dwCookie;

    icq_sendGroup(dwCookie, wAction, ack->wGroupId, szItemName, NULL, 0);

    return dwCookie;
  }
  return 0;
}

static DWORD sendUploadBuddy(HANDLE hContact, WORD wAction, DWORD dwUin, WORD wContactId, WORD wGroupId, char* szItemName, char* szNote, BYTE bAuth, WORD wItemType)
{
  DWORD dwCookie;
  servlistcookie* ack;

  if (ack = (servlistcookie*)malloc(sizeof(servlistcookie)))
  { // we have cookie good, go on
    ack->hContact = hContact;
    ack->wContactId = wContactId;
    ack->wGroupId = wGroupId;
    ack->dwAction = SSA_SERVLIST_ACK;
    ack->dwUin = dwUin;
    dwCookie = AllocateCookie(wAction, dwUin, ack);
    ack->lParam = dwCookie;

    icq_sendBuddy(dwCookie, wAction, dwUin, ack->wGroupId, ack->wContactId, szItemName, szNote, bAuth, wItemType);

    return dwCookie;
  }
  return 0;
}

#define ACTION_ADDBUDDY       0
#define ACTION_ADDBUDDYAUTH   1
#define ACTION_REMOVEBUDDY    2
#define ACTION_ADDGROUP       3
#define ACTION_REMOVEGROUP    4
#define ACTION_UPDATESTATE    5
#define ACTION_MOVECONTACT    6

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
  static DWORD dwCurrentUin;
  static WORD wNewContactId;
  static WORD wNewGroupId;
  static char* szNewGroupName;
  static WORD wNewVisibilityId;

  switch(message)
  {

  case WM_INITDIALOG:
    {
      TranslateDialogDefault(hwndDlg);
      working = 0;
      hProtoAckHook = NULL;
      currentState = STATE_READY;

      AppendToUploadLog(hwndDlg, Translate("Select contacts you want to store on server."));
      AppendToUploadLog(hwndDlg, Translate("Ready..."));
    }
    return TRUE;

    // The M_PROTOACK message is received when the
    // server has responded to our last update packet
  case M_PROTOACK:
    {
      int bMulti = 0;
      ACKDATA *ack = (ACKDATA*)lParam;
      char szLastLogLine[256];

      // Is this an ack we are waiting for?
      if (lstrcmp(ack->szModule, gpszICQProtoName))
        break;

      if (ack->type != ICQACKTYPE_SERVERCLIST)
        break;

      if ((int)ack->hProcess != currentSequence)
        break;

      switch (currentAction)
      {
      case ACTION_ADDBUDDY:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          DBWriteContactSettingByte(hCurrentContact, gpszICQProtoName, "Auth", 0);
          DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "ServerId", wNewContactId);
          DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "SrvGroupId", wNewGroupId);
          break;
        }
        else
        { // If the server refused to add the contact without authorization,
          // we try again _with_ authorization TLV

          char* pszNick = NULL;
          char* pszNote = NULL;
          DBVARIANT dbv;

          // Only upload custom nicks
          if (!DBGetContactSetting(hCurrentContact, "CList", "MyHandle", &dbv))
          {
            if (dbv.pszVal && strlen(dbv.pszVal) > 0)
              pszNick = _strdup(dbv.pszVal);
            DBFreeVariant(&dbv);
          }

          if (!DBGetContactSetting(hCurrentContact, "UserInfo", "MyNotes", &dbv))
          {
            if (dbv.pszVal && strlen(dbv.pszVal) > 0)
              pszNote = _strdup(dbv.pszVal);
            DBFreeVariant(&dbv);
          }

          currentAction = ACTION_ADDBUDDYAUTH;
          currentSequence = sendUploadBuddy(hCurrentContact, ICQ_LISTS_ADDTOLIST, dwCurrentUin, wNewContactId, wNewGroupId, pszNick, pszNote, 1, SSI_ITEM_BUDDY);

          SAFE_FREE(&pszNick);
          SAFE_FREE(&pszNote);

          return FALSE;
        }
        
      case ACTION_ADDBUDDYAUTH:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          DBWriteContactSettingByte(hCurrentContact, gpszICQProtoName, "Auth", 1);
          DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "ServerId", wNewContactId);
          DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "SrvGroupId", wNewGroupId);
        }
        else
          FreeServerID(wNewContactId, SSIT_ITEM);

        break;

      case ACTION_REMOVEBUDDY:
        if (ack->result == ACKRESULT_SUCCESS)
        { // clear obsolete settings
          FreeServerID(wNewContactId, SSIT_ITEM);
          DBDeleteContactSetting(hCurrentContact, gpszICQProtoName, "ServerId");
          DBDeleteContactSetting(hCurrentContact, gpszICQProtoName, "SrvGroupId");
          DBDeleteContactSetting(hCurrentContact, gpszICQProtoName, "Auth");
        }
        break;

      case ACTION_ADDGROUP:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          void* groupData;
          int groupSize;
          servlistcookie* ack;

          setServerGroupName(wNewGroupId, szNewGroupName); // add group to list
          setServerGroupID(szNewGroupName, wNewGroupId); // grouppath is known

          groupData = collectGroups(&groupSize);
          groupData = realloc(groupData, groupSize+2);
          *(((WORD*)groupData)+(groupSize>>1)) = wNewGroupId; // add this new group id
          groupSize += 2;

          ack = (servlistcookie*)malloc(sizeof(servlistcookie));
          if (ack)
          {
            DWORD dwCookie; // we do not use this

            ack->wGroupId = 0;
            ack->dwAction = SSA_SERVLIST_ACK;
            ack->szGroupName = NULL;
            dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, ack);

            icq_sendGroup(dwCookie, ICQ_LISTS_UPDATEGROUP, 0, ack->szGroupName, groupData, groupSize);
          }
          SAFE_FREE(&groupData);
        }
        else
          FreeServerID(wNewGroupId, SSIT_GROUP);

        SAFE_FREE(&szNewGroupName);
        break;

      case ACTION_REMOVEGROUP:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          void* groupData;
          int groupSize;
          servlistcookie* ack;

          FreeServerID(wNewGroupId, SSIT_GROUP);
          setServerGroupName(wNewGroupId, NULL); // remove group from list
          removeGroupPathLinks(wNewGroupId); // grouppath is known

          groupData = collectGroups(&groupSize);

          ack = (servlistcookie*)malloc(sizeof(servlistcookie));
          if (ack)
          {
            DWORD dwCookie; // we do not use this

            ack->wGroupId = 0;
            ack->dwAction = SSA_SERVLIST_ACK;
            ack->szGroupName = NULL;
            dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, ack);

            icq_sendGroup(dwCookie, ICQ_LISTS_UPDATEGROUP, 0, ack->szGroupName, groupData, groupSize);
          }
          SAFE_FREE(&groupData);
        }
        break;

      case ACTION_UPDATESTATE:
        // do nothing
        break;

      case ACTION_MOVECONTACT:
        if (ack->result == ACKRESULT_SUCCESS)
        {
          FreeServerID((WORD)DBGetContactSettingWord(hCurrentContact, gpszICQProtoName, "ServerId", 0), SSIT_ITEM);
          DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "ServerId", wNewContactId);
          DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "SrvGroupId", wNewGroupId);
        }
        break;
      }
      
      // Update the log window
      GetLastUploadLogLine(hwndDlg, szLastLogLine);
      DeleteLastUploadLogLine(hwndDlg);
      AppendToUploadLog(hwndDlg, "%s%s", szLastLogLine,
        Translate(ack->result == ACKRESULT_SUCCESS ? "OK" : "FAILED"));

      if (!bMulti) 
      {
        SetTimer(hwndDlg, M_UPLOADMORE, 800, 0); // delay 800ms
      }
    }
    break;

  case WM_TIMER:
    {
      switch (wParam) 
      { 
        case M_UPLOADMORE: 
          KillTimer(hwndDlg, M_UPLOADMORE);
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
      char* pszNick;
      char* pszNote;
      char* pszGroup;
      int isChecked;
      int isOnServer;

      switch (currentState)
      {
      case STATE_REGROUP:

      // TODO: iterate over all checked groups and create if needed
      // if creation requires reallocation of groups do it here

        currentState = STATE_ITEMS;
        PostMessage(hwndDlg, M_UPLOADMORE, 0, 0);
        break;

      case STATE_ITEMS:
        // Iterate over all contacts until one is found that
        // needs to be updated on the server
        hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

        while (hContact)
        {
          hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0);
          if (hItem)
          {
            isChecked = SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM)hItem, 0) != 0;
            isOnServer = DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0) != 0;
            dwUin = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0);

            // Is this one out of sync?
            if (dwUin && (isChecked != isOnServer))
            {
              DBVARIANT dbv;

              hCurrentContact = hContact;
              dwCurrentUin = dwUin;
            
              // Only upload custom nicks
              pszNick = NULL;
              if (!DBGetContactSetting(hContact, "CList", "MyHandle", &dbv))
              {
                if (dbv.pszVal && strlen(dbv.pszVal) > 0)
                  pszNick = _strdup(dbv.pszVal);
                DBFreeVariant(&dbv);
              }
              pszNote = NULL;
              if (!DBGetContactSetting(hContact, "UserInfo", "MyNotes", &dbv))
              {
                if (dbv.pszVal && strlen(dbv.pszVal) > 0)
                  pszNote = _strdup(dbv.pszVal);
                DBFreeVariant(&dbv);
              }

              if (isChecked)
              {  // Queue for uploading
                pszGroup = _strdup(DEFAULT_SS_GROUP);
                if (!DBGetContactSetting(hContact, "CList", "Group", &dbv))
                {
                  if (dbv.pszVal && strlen(dbv.pszVal) > 0)
                    pszGroup = _strdup(dbv.pszVal);
                  DBFreeVariant(&dbv);
                }

                // Get group ID from cache, if not ready use parent group, if still not ready create one
                wNewGroupId = getServerGroupID(pszGroup);
                if (!wNewGroupId && strstr(pszGroup, "\\") != NULL)
                { // if it is sub-group, take master parent
                  strstr(pszGroup, "\\")[0] = '\0'; 
                  wNewGroupId = getServerGroupID(pszGroup);
                }
                if (!wNewGroupId && currentAction != ACTION_ADDGROUP)
                { // if the group still does not exist and there was no try before, try to add group
                  AppendToUploadLog(hwndDlg, Translate("Adding group \"%s\"..."), pszGroup);

                  wNewGroupId = GenerateServerId(SSIT_GROUP);
                  szNewGroupName = pszGroup;
                  currentAction = ACTION_ADDGROUP;
                  currentSequence = sendUploadGroup(ICQ_LISTS_ADDTOLIST, wNewGroupId, pszGroup);
                  SAFE_FREE(&pszNick);
                  SAFE_FREE(&pszNote);

                  return FALSE;
                }

                SAFE_FREE(&pszGroup);

                if (pszNick)
                  AppendToUploadLog(hwndDlg, Translate("Uploading %s..."), pszNick);
                else 
                  AppendToUploadLog(hwndDlg, Translate("Uploading %u..."), dwUin);

                currentAction = ACTION_ADDBUDDY;

                if (wNewGroupId)
                {
                  wNewContactId = GenerateServerId(SSIT_ITEM);

                  currentSequence = sendUploadBuddy(hCurrentContact, ICQ_LISTS_ADDTOLIST, dwUin, 
                    wNewContactId, wNewGroupId, pszNick, pszNote, 0, SSI_ITEM_BUDDY);
                  SAFE_FREE(&pszNick);
                  SAFE_FREE(&pszNote);

                  return FALSE;
                }
                else
                {
                  char szLastLogLine[256];
                  // Update the log window with the failure and continue with next contact
                  GetLastUploadLogLine(hwndDlg, szLastLogLine);
                  DeleteLastUploadLogLine(hwndDlg);
                  AppendToUploadLog(hwndDlg, "%s%s", szLastLogLine, Translate("FAILED"));
                  AppendToUploadLog(hwndDlg, Translate("No upload group available"));
                  Netlib_Logf(ghServerNetlibUser, "Upload failed, no group");
                  currentState = STATE_READY;
                }
              }
              else
              { // Queue for deletion
                if (pszNick)
                  AppendToUploadLog(hwndDlg, Translate("Deleting %s..."), pszNick);
                else 
                  AppendToUploadLog(hwndDlg, Translate("Deleting %u..."), dwUin);

                wNewGroupId = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvGroupId", 0);
                wNewContactId = DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0);
                currentAction = ACTION_REMOVEBUDDY;
                currentSequence = sendUploadBuddy(hContact, ICQ_LISTS_REMOVEFROMLIST, dwUin, 
                  wNewContactId, wNewGroupId, NULL, NULL, 0, SSI_ITEM_BUDDY);
              }
              SAFE_FREE(&pszNick);
              SAFE_FREE(&pszNote);

              break;
            }
            else if (dwUin && isChecked)
            { // the contact is and should be on server, check if it is in correct group, move otherwise
              WORD wCurrentGroupId = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvGroupId", 0);
              DBVARIANT dbv;

              pszGroup = _strdup(DEFAULT_SS_GROUP);
              if (!DBGetContactSetting(hContact, "CList", "Group", &dbv))
              {
                if (dbv.pszVal && strlen(dbv.pszVal) > 0)
                  pszGroup = _strdup(dbv.pszVal);
                DBFreeVariant(&dbv);
              }
              wNewGroupId = getServerGroupID(pszGroup);
              if (!wNewGroupId && strstr(pszGroup, "\\") != NULL)
              { // if it is sub-group, take master parent
                strstr(pszGroup, "\\")[0] = '\0'; 
                wNewGroupId = getServerGroupID(pszGroup);
              }
              if (!wNewGroupId && currentAction != ACTION_ADDGROUP)
              { // if the group still does not exist and there was no try before, try to add group
                AppendToUploadLog(hwndDlg, Translate("Adding group \"%s\"..."), pszGroup);

                wNewGroupId = GenerateServerId(SSIT_GROUP);
                szNewGroupName = pszGroup;
                currentAction = ACTION_ADDGROUP;
                currentSequence = sendUploadGroup(ICQ_LISTS_ADDTOLIST, wNewGroupId, pszGroup);

                return FALSE;
              }
              if (wNewGroupId && (wNewGroupId != wCurrentGroupId))
              { // we have a group the contact should be in, move it
                WORD wCurrentContactId = DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0);
                BYTE bAuth = DBGetContactSettingByte(hContact, gpszICQProtoName, "Auth", 0);

                pszNick = NULL;
                if (!DBGetContactSetting(hContact, "CList", "MyHandle", &dbv))
                {
                  if (dbv.pszVal && strlen(dbv.pszVal) > 0)
                    pszNick = _strdup(dbv.pszVal);
                  DBFreeVariant(&dbv);
                }
                pszNote = NULL;
                if (!DBGetContactSetting(hContact, "UserInfo", "MyNotes", &dbv))
                {
                  if (dbv.pszVal && strlen(dbv.pszVal) > 0)
                    pszNote = _strdup(dbv.pszVal);
                  DBFreeVariant(&dbv);
                }
                if (pszNick)
                  AppendToUploadLog(hwndDlg, Translate("Moving %s to group \"%s\"..."), pszNick, pszGroup);
                else 
                  AppendToUploadLog(hwndDlg, Translate("Moving %u to group \"%s\"..."), dwUin, pszGroup);

                hCurrentContact = hContact;
                currentAction = ACTION_MOVECONTACT;
                wNewContactId = GenerateServerId(SSIT_ITEM);
                currentSequence = sendUploadBuddy(hContact, ICQ_LISTS_ADDTOLIST, dwUin, wNewContactId, wNewGroupId, pszNick, pszNote, bAuth, SSI_ITEM_BUDDY);
                sendUploadBuddy(hContact, ICQ_LISTS_REMOVEFROMLIST, dwUin, wCurrentContactId, wCurrentGroupId, NULL, NULL, 0, SSI_ITEM_BUDDY);
                SAFE_FREE(&pszNick);
                SAFE_FREE(&pszNote);
                SAFE_FREE(&pszGroup);

                break;
              }
              SAFE_FREE(&pszGroup);
            }
          }
          hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
        }
        if (!hContact) 
        {
          currentState = STATE_VISIBILITY;
          PostMessage(hwndDlg, M_UPLOADMORE, 0, 0);
        }
        break;

      case STATE_VISIBILITY:
        // TODO: sync privacy items on server 

        currentState = STATE_CONSOLIDATE;
        AppendToUploadLog(hwndDlg, Translate("Cleaning groups"));
        EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
        enumServerGroups();
        PostMessage(hwndDlg, M_UPLOADMORE, 0, 0);
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
            char* pszGroup = getServerGroupName(wNewGroupId);
            servlistcookie* ack = (servlistcookie*)malloc(sizeof(servlistcookie));

            ack->dwAction = SSA_SERVLIST_ACK;
            ack->wContactId = 0;
            ack->wGroupId = wNewGroupId;
            ack->hContact = NULL;
            currentSequence = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, ack);
            ack->lParam = currentSequence;
            currentAction = ACTION_UPDATESTATE;
            AppendToUploadLog(hwndDlg, Translate("Updating group \"%s\"..."), pszGroup);

            icq_sendGroup(currentSequence, ICQ_LISTS_UPDATEGROUP, wNewGroupId, pszGroup, groupData, groupSize);

            SAFE_FREE(&pszGroup);
          }
          else // the group is empty, delete it if it does not have sub-groups
          {
            if (!CheckServerID((WORD)(wNewGroupId+1), 0) || countGroupLevel((WORD)(wNewGroupId+1)) == 0)
            { // is next id an sub-group, if yes, we cannot delete this group
              char* pszGroup = getServerGroupName(wNewGroupId);
              currentAction = ACTION_REMOVEGROUP;
              AppendToUploadLog(hwndDlg, Translate("Deleting group \"%s\"..."), pszGroup); 
              currentSequence = sendUploadGroup(ICQ_LISTS_REMOVEFROMLIST, wNewGroupId, pszGroup);
              SAFE_FREE(&pszGroup);
            }
          }
          SAFE_FREE(&groupData); // free the memory
        }
        else
        { // all groups processed
          SAFE_FREE(&pwGroupIds);
          currentState = STATE_READY;
        }
        break;
      }

      if (currentState == STATE_READY)
      {
        // All contacts are in sync
        AppendToUploadLog(hwndDlg, Translate("All operations complete"));
        EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), TRUE);
        SetDlgItemText(hwndDlg, IDCANCEL, Translate("Close"));
        sendAddEnd();
        working = 0;
        SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_SETGREYOUTFLAGS,0,0);
        UpdateCheckmarks(hwndDlg, hItemAll);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CLIST), FALSE);
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
        if (gnCurrentStatus == ID_STATUS_OFFLINE || gnCurrentStatus == ID_STATUS_CONNECTING)
        {
          AppendToUploadLog(hwndDlg, Translate("You have to be online to sychronize the server-list !"));
          break;
        }
        working = 1;
        currentState = STATE_REGROUP;
        icq_ShowMultipleControls(hwndDlg, settingsControls, sizeof(settingsControls)/sizeof(settingsControls[0]), SW_HIDE);
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETGREYOUTFLAGS, 0xFFFFFFFF, 0);
        InvalidateRect(GetDlgItem(hwndDlg, IDC_CLIST), NULL, FALSE);
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
          switch(((NMHDR*)lParam)->code)
          {
            
          case CLN_OPTIONSCHANGED:
            {
              int i;
              
              SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETLEFTMARGIN, 2, 0);
              SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKBITMAP, 0, (LPARAM)(HBITMAP)NULL);
              SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKCOLOR, GetSysColor(COLOR_WINDOW), 0);
              SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETGREYOUTFLAGS, working?0xFFFFFFFF:0, 0);
              for (i=0; i<=FONTID_MAX; i++)
                SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETTEXTCOLOR, i, GetSysColor(COLOR_WINDOWTEXT));
            }
            break;
            
          case CLN_LISTREBUILT:
            {
              HANDLE hContact;
              HANDLE hItem;
              char* szProto;
              int bCheck;

              // Delete non-icq contacts
              hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
              while (hContact)
              {
                hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0);
                if (hItem)
                {
                  szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
                  if (szProto == NULL || lstrcmp(szProto, gpszICQProtoName))
                    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_DELETEITEM, (WPARAM)hItem, 0);
                }
                hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
              }

              bCheck = UpdateCheckmarks(hwndDlg, NULL);

              if (!hItemAll) // Add the "All contacts" item
              {
                CLCINFOITEM cii = {0};

                cii.cbSize = sizeof(cii);
                cii.flags = CLCIIF_GROUPFONT | CLCIIF_CHECKBOX;
                cii.pszText = Translate("** All contacts **");
                hItemAll = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM)&cii);
              }

              SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM)hItemAll, bCheck);
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

                check = SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM)hItemAll, 0);
                
                hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
                while (hContact)
                {
                  hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0);
                  if (hItem)
                    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM)hItem, check);
                  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
                }
              }
              else
              {
                UpdateAllContactsCheckmark(GetDlgItem(hwndDlg, IDC_CLIST), hItemAll);
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
    hwndUploadContacts = CreateDialog(hInst, MAKEINTRESOURCE(IDD_ICQUPLOADLIST), NULL, DlgProcUploadList);
  }

  SetForegroundWindow(hwndUploadContacts);
}
