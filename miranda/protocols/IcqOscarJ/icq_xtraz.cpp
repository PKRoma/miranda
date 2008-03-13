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
//  Internal Xtraz API
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


extern HANDLE hsmsgrequest;
extern HANDLE hxstatuschanged;


void handleXtrazNotify(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szMsg, int nMsgLen, BOOL bThruDC)
{
  char *szWork, *szEnd, *szNotify, *szQuery;
  int nNotifyLen, nQueryLen;
  HANDLE hContact;

  szNotify = strstr(szMsg, "<NOTIFY>");
  szQuery = strstr(szMsg, "<QUERY>");

  hContact = HContactFromUIN(dwUin, NULL);
  if (hContact) // user sent us xtraz, he supports it
    SetContactCapabilities(hContact, CAPF_XTRAZ);

  if (szNotify && szQuery)
  { // valid request
    szNotify += 8;
    szQuery += 7;
    szEnd = strstr(szMsg, "</NOTIFY>");
    if (!szEnd) szEnd = szMsg + nMsgLen;
    nNotifyLen = (szEnd - szNotify);
    szEnd = strstr(szMsg, "</QUERY>");
    if (!szEnd) szEnd = szNotify;
    szNotify = DemangleXml(szNotify, nNotifyLen);
    nQueryLen = (szEnd - szQuery);
    szQuery = DemangleXml(szQuery, nQueryLen);
    szWork = strstr(szQuery, "<PluginID>");
    szEnd = strstr(szQuery, "</PluginID>");
#ifdef _DEBUG
    NetLog_Server("Query: %s", szQuery);
    NetLog_Server("Notify: %s", szNotify);
#endif
    if (szWork && szEnd)
    { // this is our plugin
      szWork += 10;
      *szEnd = '\0';

      if (!stricmp(szWork, "srvMng") && strstr(szNotify, "AwayStat"))
      {
        char* szSender = strstr(szNotify, "<senderId>");
        char* szEndSend = strstr(szNotify, "</senderId>");

        if (szSender && szEndSend)
        {
          szSender += 10;
          *szEndSend = '\0';

          if ((DWORD)atoi(szSender) == dwUin)
          {
            char *szResponse;
            int nResponseLen;
            char *szXName, *szXMsg, *tmp;
            BYTE dwXId = ICQGetContactXStatus(NULL);

            if (dwXId && validateStatusMessageRequest(hContact, MTYPE_SCRIPT_NOTIFY))
            { // apply privacy rules
              NotifyEventHooks(hsmsgrequest, (WPARAM)MTYPE_SCRIPT_NOTIFY, (LPARAM)dwUin);

              tmp = ICQGetContactSettingUtf(NULL, DBSETTING_XSTATUSNAME, "");
              szXName = MangleXml(tmp, strlennull(tmp));
              SAFE_FREE((void**)&tmp);
              tmp = ICQGetContactSettingUtf(NULL, DBSETTING_XSTATUSMSG, "");
              szXMsg = MangleXml(tmp, strlennull(tmp));
              SAFE_FREE((void**)&tmp);
              
              nResponseLen = 212 + strlennull(szXName) + strlennull(szXMsg) + UINMAXLEN + 2;
              szResponse = (char*)_alloca(nResponseLen + 1);
              // send response
              nResponseLen = null_snprintf(szResponse, nResponseLen, 
                "<ret event='OnRemoteNotification'>"
                "<srv><id>cAwaySrv</id>"
                "<val srv_id='cAwaySrv'><Root>"
                "<CASXtraSetAwayMessage></CASXtraSetAwayMessage>"
                "<uin>%d</uin>"
                "<index>%d</index>"
                "<title>%s</title>"
                "<desc>%s</desc></Root></val></srv></ret>",
                dwLocalUIN, dwXId, szXName, szXMsg);

              SAFE_FREE((void**)&szXName);
              SAFE_FREE((void**)&szXMsg);

              {
                rate_record rr = {0};

                rr.bType = RIT_XSTATUS_RESPONSE;
                rr.dwUin = dwUin;
                rr.dwMid1 = dwMID;
                rr.dwMid2 = dwMID2;
                rr.wCookie = wCookie;
                rr.szData = szResponse;
                rr.bThruDC = bThruDC;
                rr.nRequestType = 0x102;
                EnterCriticalSection(&ratesMutex);
                rr.wGroup = ratesGroupFromSNAC(gRates, ICQ_MSG_FAMILY, ICQ_MSG_RESPONSE);
                LeaveCriticalSection(&ratesMutex);
                if (bThruDC || !handleRateItem(&rr, TRUE))
                  SendXtrazNotifyResponse(dwUin, dwMID, dwMID2, wCookie, szResponse, nResponseLen, bThruDC);
              }
            }
            else if (dwXId)
              NetLog_Server("Privacy: Ignoring XStatus request");
            else
              NetLog_Server("Error: We are not in XStatus, skipping");
          }
          else
            NetLog_Server("Error: Invalid sender information");
        }
        else
          NetLog_Server("Error: Missing sender information");
      }
      else
        NetLog_Server("Error: Unknown plugin \"%s\" in Xtraz message", szWork);
    }
    else 
      NetLog_Server("Error: Missing PluginID in Xtraz message");

    SAFE_FREE((void**)&szNotify);
    SAFE_FREE((void**)&szQuery);
  }
  else 
    NetLog_Server("Error: Invalid Xtraz Notify message");
}



void handleXtrazNotifyResponse(DWORD dwUin, HANDLE hContact, WORD wCookie, char* szMsg, int nMsgLen)
{
  char *szMem, *szRes, *szEnd;
  int nResLen;

#ifdef _DEBUG
  NetLog_Server("Received Xtraz Notify Response");
#endif

  szRes = strstr(szMsg, "<RES>");
  szEnd = strstr(szMsg, "</RES>");

  if (szRes && szEnd)
  { // valid response
    char *szNode, *szWork;

    szRes += 5;
    nResLen = szEnd - szRes;

    szMem = szRes = DemangleXml(szRes, nResLen);

#ifdef _DEBUG
    NetLog_Server("Response: %s", szRes);
#endif

    ICQBroadcastAck(hContact, ICQACKTYPE_XTRAZNOTIFY_RESPONSE, ACKRESULT_SUCCESS, (HANDLE)wCookie, (LPARAM)szRes);

NextVal:
    szNode = strstr(szRes, "<val srv_id='");
    if (szNode) szEnd = strstr(szNode, ">"); else szEnd = NULL;

    if (szNode && szEnd)
    {
      *(szEnd-1) = '\0';
      szNode += 13;
      szWork = szEnd + 1;

      if (!stricmp(szNode, "cAwaySrv"))
      {
        szNode = strstr(szWork, "<uin>");
        szEnd = strstr(szWork, "</uin>");

        if (szNode && szEnd)
        {
          szNode += 5;
          *szEnd = '\0';

          if ((DWORD)atoi(szNode) == dwUin)
          {
            int bChanged = FALSE;

            *szEnd = ' ';
            szNode = strstr(szWork, "<index>");
            szEnd = strstr(szWork, "</index>");

            if (szNode && szEnd)
            {
              szNode += 7;
              *szEnd = '\0';
              if (atoi(szNode) != ICQGetContactXStatus(hContact))
              { // this is strange - but go on
                NetLog_Server("Warning: XStatusIds do not match!");
              }
              *szEnd = ' ';
            }
            szNode = strstr(szWork, "<title>");
            szEnd = strstr(szWork, "</title>");

            if (szNode && szEnd)
            { // we got XStatus title, save it
              char *szXName, *szOldXName;

              szNode += 7;
              *szEnd = '\0';
              szXName = DemangleXml(szNode, strlennull(szNode));
              // check if the name changed
              szOldXName = ICQGetContactSettingUtf(hContact, DBSETTING_XSTATUSNAME, NULL);
              if (strcmpnull(szOldXName, szXName))
                bChanged = TRUE;
              SAFE_FREE((void**)&szOldXName);
              ICQWriteContactSettingUtf(hContact, DBSETTING_XSTATUSNAME, szXName);
              SAFE_FREE((void**)&szXName);
              *szEnd = ' ';
            }
            szNode = strstr(szWork, "<desc>");
            szEnd = strstr(szWork, "</desc>");

            if (szNode && szEnd)
            { // we got XStatus mode msg, save it
              char *szXMsg, *szOldXMsg;

              szNode += 6;
              *szEnd = '\0';
              szXMsg = DemangleXml(szNode, strlennull(szNode));
              // check if the decription changed
              szOldXMsg = ICQGetContactSettingUtf(hContact, DBSETTING_XSTATUSNAME, NULL);
              if (strcmpnull(szOldXMsg, szXMsg))
                bChanged = TRUE;
              SAFE_FREE((void**)&szOldXMsg);
              ICQWriteContactSettingUtf(hContact, DBSETTING_XSTATUSMSG, szXMsg);
              SAFE_FREE((void**)&szXMsg);
            }
            ICQBroadcastAck(hContact, ICQACKTYPE_XSTATUS_RESPONSE, ACKRESULT_SUCCESS, (HANDLE)wCookie, 0);

            if (bChanged)
              NotifyEventHooks(hxstatuschanged, (WPARAM)hContact, 0);
          }
          else
            NetLog_Server("Error: Invalid sender information");
        }
        else
          NetLog_Server("Error: Missing sender information");
      }
      else
      {
        char *szSrvEnd = strstr(szEnd, "</srv>");

        if (szSrvEnd && strstr(szSrvEnd, "<val srv_id='"))
        { // check all values !
          szRes = szSrvEnd + 6; // after first value
          goto NextVal;
        }
        // no next val, we were unable to handle packet, write error
        NetLog_Server("Error: Unknown serverId \"%s\" in Xtraz response", szNode);
      }
    }
    else
      NetLog_Server("Error: Missing serverId in Xtraz response");

    SAFE_FREE((void**)&szMem);
  }
  else
    NetLog_Server("Error: Invalid Xtraz Notify response");
}



static char* getXmlPidItem(const char* szData, int nLen)
{
  const char *szPid, *szEnd;

  szPid = strstr(szData, "<PID>");
  szEnd = strstr(szData, "</PID>");

  if (szPid && szEnd)
  {
    szPid += 5;

    return DemangleXml(szPid, szEnd - szPid);
  }
  return NULL;
}



void handleXtrazInvitation(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szMsg, int nMsgLen, BOOL bThruDC)
{
  HANDLE hContact;
  char* szPluginID;

  hContact = HContactFromUIN(dwUin, NULL);
  if (hContact) // user sent us xtraz, he supports it
    SetContactCapabilities(hContact, CAPF_XTRAZ);

  szPluginID = getXmlPidItem(szMsg, nMsgLen);
  if (!strcmpnull(szPluginID, "ICQChatRecv"))
  { // it is a invitation to multi-user chat
  }
  else
  {
    NetLog_Uni(bThruDC, "Error: Unknown plugin \"%s\" in Xtraz message", szPluginID);
  }
  SAFE_FREE((void**)&szPluginID);
}



void handleXtrazData(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szMsg, int nMsgLen, BOOL bThruDC)
{
  HANDLE hContact;
  char* szPluginID;

  hContact = HContactFromUIN(dwUin, NULL);
  if (hContact) // user sent us xtraz, he supports it
    SetContactCapabilities(hContact, CAPF_XTRAZ);

  szPluginID = getXmlPidItem(szMsg, nMsgLen);
  if (!strcmpnull(szPluginID, "viewCard"))
  { // it is a greeting card
    char *szWork, *szEnd, *szUrl, *szNum;

    szWork = strstr(szMsg, "<InD>");
    szEnd = strstr(szMsg, "</InD>");
    if (szWork && szEnd)
    {
      int nDataLen = szEnd - szWork;

      szUrl = (char*)_alloca(nDataLen);
      memcpy(szUrl, szWork+5, nDataLen);
      szUrl[nDataLen - 5] = '\0';

      if (!strnicmp(szUrl, "view_", 5))
      {
        szNum = szUrl + 5;
        szWork = strstr(szUrl, ".html");
        if (szWork)
        {
          strcpy(szWork, ".php");
          strcat(szWork, szWork+5);
        }
        while (szWork = strstr(szUrl, "&amp;"))
        { // unescape &amp; code
          strcpy(szWork+1, szWork+5);
        }
        szWork = (char*)SAFE_MALLOC(nDataLen + MAX_PATH);
        ICQTranslateUtfStatic("Greeting card:", szWork, MAX_PATH);
        strcat(szWork, "\r\nhttp://www.icq.com/friendship/pages/view_page_");
        strcat(szWork, szNum);

        // Create message to notify user
        {
          CCSDATA ccs;
          PROTORECVEVENT pre = {0};
          int bAdded;

          ccs.szProtoService = PSR_MESSAGE;
          ccs.hContact = HContactFromUIN(dwUin, &bAdded);
          ccs.wParam = 0;
          ccs.lParam = (LPARAM)&pre;
          pre.timestamp = time(NULL);
          pre.szMessage = szWork;
          pre.flags = PREF_UTF;

          CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
        }
        SAFE_FREE((void**)&szWork);
      }
      else
        NetLog_Uni(bThruDC, "Error: Non-standard greeting card message");
    }
    else
      NetLog_Uni(bThruDC, "Error: Malformed greeting card message");
  }
  else
  {
    NetLog_Uni(bThruDC, "Error: Unknown plugin \"%s\" in Xtraz message", szPluginID);
  }
  SAFE_FREE((void**)&szPluginID);
}



// Functions really sending Xtraz stuff
DWORD SendXtrazNotifyRequest(HANDLE hContact, char* szQuery, char* szNotify, int bForced)
{
  char *szQueryBody;
  char *szNotifyBody;
  DWORD dwUin;
  int nBodyLen;
  char *szBody;
  DWORD dwCookie;
  message_cookie_data* pCookieData;

  if (ICQGetContactSettingUID(hContact, &dwUin, NULL))
    return 0; // Invalid contact

  if (!CheckContactCapabilities(hContact, CAPF_XTRAZ) && !bForced)
    return 0; // Contact does not support xtraz, do not send anything

  szQueryBody = MangleXml(szQuery, strlennull(szQuery));
  szNotifyBody = MangleXml(szNotify, strlennull(szNotify));
  nBodyLen = strlennull(szQueryBody) + strlennull(szNotifyBody) + 41;
  szBody = (char*)_alloca(nBodyLen);
  nBodyLen = null_snprintf(szBody, nBodyLen, "<N><QUERY>%s</QUERY><NOTIFY>%s</NOTIFY></N>", szQueryBody, szNotifyBody);
  SAFE_FREE((void**)&szQueryBody);
  SAFE_FREE((void**)&szNotifyBody);

  // Set up the ack type
  pCookieData = CreateMessageCookie(MTYPE_SCRIPT_NOTIFY, ACKTYPE_CLIENT);
  dwCookie = AllocateCookie(CKT_MESSAGE, 0, hContact, (void*)pCookieData);

  // have we a open DC, send through that
  if (gbDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
    icq_sendXtrazRequestDirect(hContact, dwCookie, szBody, nBodyLen, MTYPE_SCRIPT_NOTIFY);
  else
    icq_sendXtrazRequestServ(dwUin, dwCookie, szBody, nBodyLen, pCookieData);

  return dwCookie;
}



void SendXtrazNotifyResponse(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szResponse, int nResponseLen, BOOL bThruDC)
{
  char *szResBody = MangleXml(szResponse, nResponseLen);
  int nBodyLen = strlennull(szResBody) + 21;
  char *szBody = (char*)_alloca(nBodyLen);
  HANDLE hContact = HContactFromUIN(dwUin, NULL);

  if (hContact != INVALID_HANDLE_VALUE && !CheckContactCapabilities(hContact, CAPF_XTRAZ))
  {
    SAFE_FREE((void**)&szResBody);
    return; // Contact does not support xtraz, do not send anything
  }

  nBodyLen = null_snprintf(szBody, nBodyLen, "<NR><RES>%s</RES></NR>", szResBody);
  SAFE_FREE((void**)&szResBody);

  // Was request received thru DC and have we a open DC, send through that
  if (bThruDC && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
    icq_sendXtrazResponseDirect(hContact, wCookie, szBody, nBodyLen, MTYPE_SCRIPT_NOTIFY);
  else
    icq_sendXtrazResponseServ(dwUin, dwMID, dwMID2, wCookie, szBody, nBodyLen, MTYPE_SCRIPT_NOTIFY);
}
