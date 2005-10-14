// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin  berg, Sam Kothari, Robert Rainwater
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
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


PLUGINLINK* pluginLink;
HANDLE hHookUserInfoInit = NULL;
HANDLE hHookOptionInit = NULL;
HANDLE hHookUserMenu = NULL;
HANDLE hHookIdleEvent = NULL;
static HANDLE hUserMenuAuth = NULL;
static HANDLE hUserMenuGrant = NULL;

extern HANDLE hServerConn;
extern icq_mode_messages modeMsgs;
extern CRITICAL_SECTION modeMsgsMutex;
CRITICAL_SECTION localSeqMutex;
CRITICAL_SECTION connectionHandleMutex;
HANDLE hsmsgrequest;

PLUGININFO pluginInfo = {
  sizeof(PLUGININFO),
  "IcqOscarJ Protocol",
  PLUGIN_MAKE_VERSION(0,3,6,6),
  "Support for ICQ network, enhanced.",
  "Joe Kucera, Bio, Martin Öberg, Richard Hughes, Jon Keating, etc",
  "jokusoftware@users.sourceforge.net",
  "(C) 2000-2005 M.Öberg, R.Hughes, J.Keating, Bio, J.Kucera",
  "http://www.miranda-im.org/download/details.php?action=viewfile&id=1683",
  0,  //not transient
  0   //doesn't replace anything built-in
};

static int OnSystemModulesLoaded(WPARAM wParam,LPARAM lParam);
static int icq_PrebuildContactMenu(WPARAM wParam, LPARAM lParam);



PLUGININFO __declspec(dllexport) *MirandaPluginInfo(DWORD mirandaVersion)
{
  // Only load for 0.4.0.1 or greater
  // Miranda IM v0.4.0.1 contained important DB bug fix
  if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 1)) 
  {
    return NULL;
  }
  else
  {
    MIRANDA_VERSION = mirandaVersion;
    return &pluginInfo;
  }
}



BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
  hInst = hinstDLL;

  return TRUE;
}



static HANDLE ICQCreateServiceFunction(const char* szService,  MIRANDASERVICE serviceProc)
{
  char str[MAX_PATH + 32];
  strcpy(str, gpszICQProtoName);
  strcat(str, szService);
  return CreateServiceFunction(str, serviceProc);
}



int __declspec(dllexport) Load(PLUGINLINK *link)
{
  PROTOCOLDESCRIPTOR pd;

  pluginLink = link;

  gbUnicodeAPI = (GetVersion() & 0x80000000) == 0;

  srand(time(NULL));
  _tzset();

  // Get module name from DLL file name
  {
    char* str1;
    char str2[MAX_PATH];

    GetModuleFileName(hInst, str2, MAX_PATH);
    str1 = strrchr(str2, '\\');
    if (str1 != NULL && (strlennull(str1+1) > 4))
    {
      strncpy(gpszICQProtoName, str1+1, strlennull(str1+1)-4);
      gpszICQProtoName[strlennull(str1+1)-3] = 0;
    }
    CharUpper(gpszICQProtoName);
  }

  ZeroMemory(gpszPassword, sizeof(gpszPassword));

  icq_FirstRunCheck();

  HookEvent(ME_SYSTEM_MODULESLOADED, OnSystemModulesLoaded);

  InitializeCriticalSection(&connectionHandleMutex);
  InitializeCriticalSection(&localSeqMutex);
  InitializeCriticalSection(&modeMsgsMutex);
  InitCookies();

  // Register the module
  ZeroMemory(&pd, sizeof(pd));
  pd.cbSize = sizeof(pd);
  pd.szName = gpszICQProtoName;
  pd.type   = PROTOTYPE_PROTOCOL;
  CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM)&pd);

  // Initialize status message struct
  ZeroMemory(&modeMsgs, sizeof(icq_mode_messages));

  // contacts cache
  InitCache();

  // Reset a bunch of session specific settings
  ResetSettingsOnLoad();

  // Setup services
  ICQCreateServiceFunction(PS_GETCAPS, IcqGetCaps);
  ICQCreateServiceFunction(PS_GETNAME, IcqGetName);
  ICQCreateServiceFunction(PS_LOADICON, IcqLoadIcon);
  ICQCreateServiceFunction(PS_SETSTATUS, IcqSetStatus);
  ICQCreateServiceFunction(PS_GETSTATUS, IcqGetStatus);
  ICQCreateServiceFunction(PS_SETAWAYMSG, IcqSetAwayMsg);
  ICQCreateServiceFunction(PS_AUTHALLOW, IcqAuthAllow);
  ICQCreateServiceFunction(PS_AUTHDENY, IcqAuthDeny);
  ICQCreateServiceFunction(PS_BASICSEARCH, IcqBasicSearch);
  ICQCreateServiceFunction(PS_SEARCHBYEMAIL, IcqSearchByEmail);
  ICQCreateServiceFunction(MS_ICQ_SEARCHBYDETAILS, IcqSearchByDetails);
  ICQCreateServiceFunction(PS_SEARCHBYNAME, IcqSearchByDetails);
  ICQCreateServiceFunction(PS_CREATEADVSEARCHUI, IcqCreateAdvSearchUI);
  ICQCreateServiceFunction(PS_SEARCHBYADVANCED, IcqSearchByAdvanced);
  ICQCreateServiceFunction(MS_ICQ_SENDSMS, IcqSendSms);
  ICQCreateServiceFunction(PS_ADDTOLIST, IcqAddToList);
  ICQCreateServiceFunction(PS_ADDTOLISTBYEVENT, IcqAddToListByEvent);
  ICQCreateServiceFunction(PS_FILERESUME, IcqFileResume);
  ICQCreateServiceFunction(PS_SET_NICKNAME, IcqSetNickName);
  ICQCreateServiceFunction(PSS_GETINFO, IcqGetInfo);
  ICQCreateServiceFunction(PSS_MESSAGE, IcqSendMessage);
  ICQCreateServiceFunction(PSS_MESSAGE"W", IcqSendMessageW);
  ICQCreateServiceFunction(PSS_URL, IcqSendUrl);
  ICQCreateServiceFunction(PSS_CONTACTS, IcqSendContacts);
  ICQCreateServiceFunction(PSS_SETAPPARENTMODE, IcqSetApparentMode);
  ICQCreateServiceFunction(PSS_GETAWAYMSG, IcqGetAwayMsg);
  ICQCreateServiceFunction(PSS_FILEALLOW, IcqFileAllow);
  ICQCreateServiceFunction(PSS_FILEDENY, IcqFileDeny);
  ICQCreateServiceFunction(PSS_FILECANCEL, IcqFileCancel);
  ICQCreateServiceFunction(PSS_FILE, IcqSendFile);
  ICQCreateServiceFunction(PSR_AWAYMSG, IcqRecvAwayMsg);
  ICQCreateServiceFunction(PSR_FILE, IcqRecvFile);
  ICQCreateServiceFunction(PSR_MESSAGE, IcqRecvMessage);
  ICQCreateServiceFunction(PSR_URL, IcqRecvUrl);
  ICQCreateServiceFunction(PSR_CONTACTS, IcqRecvContacts);
  ICQCreateServiceFunction(PSR_AUTH, IcqRecvAuth);
  ICQCreateServiceFunction(PSS_AUTHREQUEST, IcqSendAuthRequest);
  ICQCreateServiceFunction(PSS_ADDED, IcqSendYouWereAdded);
  ICQCreateServiceFunction(PSS_USERISTYPING, IcqSendUserIsTyping);
  ICQCreateServiceFunction(PS_GETAVATARINFO, IcqGetAvatarInfo);

  {
    char pszServiceName[MAX_PATH + 32];

    strcpy(pszServiceName, gpszICQProtoName); strcat(pszServiceName, ME_ICQ_STATUSMSGREQ);
    hsmsgrequest=CreateHookableEvent(pszServiceName);
  }

  InitDirectConns();
  InitServerLists();
  icq_InitInfoUpdate();

  // Initialize charset conversion routines
  InitI18N();

  UpdateGlobalSettings();

  gnCurrentStatus = ID_STATUS_OFFLINE;

  hHookUserMenu = HookEvent(ME_CLIST_PREBUILDCONTACTMENU, icq_PrebuildContactMenu);

  {
    CLISTMENUITEM mi;
    char pszServiceName[MAX_PATH+30];

    strcpy(pszServiceName, gpszICQProtoName);
    strcat(pszServiceName, MS_REQ_AUTH);
    CreateServiceFunction(pszServiceName, icq_RequestAuthorization);

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.position = 1000030000;
    mi.flags = 0;
    mi.hIcon = NULL; // TODO: add icon
    mi.pszContactOwner = gpszICQProtoName;
    mi.pszName = Translate("Request authorization");
    mi.pszService = pszServiceName;
    hUserMenuAuth = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);

    strcpy(pszServiceName, gpszICQProtoName);
    strcat(pszServiceName, MS_GRANT_AUTH);
    CreateServiceFunction(pszServiceName, IcqGrantAuthorization);

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.position = 1000029999;
    mi.flags = 0;
    mi.hIcon = NULL; // TODO: add icon
    mi.pszContactOwner = gpszICQProtoName;
    mi.pszName = Translate("Grant authorization");
    mi.pszService = pszServiceName;
    hUserMenuGrant = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
  }

  InitXStatusEvents();

  return 0;
}



int __declspec(dllexport) Unload(void)
{
  if (gbXStatusEnabled) gbXStatusEnabled = 10; // block clist changing

  if (hServerConn)
  {
    icq_packet packet;

    packet.wLen = 0;
    write_flap(&packet, ICQ_CLOSE_CHAN);
    sendServPacket(&packet);

    icq_serverDisconnect(TRUE);
  }

  UninitServerLists();
  UninitDirectConns();
  icq_InfoUpdateCleanup();

  Netlib_CloseHandle(ghDirectNetlibUser);
  Netlib_CloseHandle(ghServerNetlibUser);
  UninitCookies();
  UninitCache();
  DeleteCriticalSection(&modeMsgsMutex);
  DeleteCriticalSection(&localSeqMutex);
  DeleteCriticalSection(&connectionHandleMutex);
  SAFE_FREE(&modeMsgs.szAway);
  SAFE_FREE(&modeMsgs.szNa);
  SAFE_FREE(&modeMsgs.szOccupied);
  SAFE_FREE(&modeMsgs.szDnd);
  SAFE_FREE(&modeMsgs.szFfc);


  if (hHookUserInfoInit)
    UnhookEvent(hHookUserInfoInit);

  if (hHookOptionInit)
    UnhookEvent(hHookOptionInit);

  if (hsmsgrequest)
    DestroyHookableEvent(hsmsgrequest);

  if (hHookUserMenu)
    UnhookEvent(hHookUserMenu);

  if (hHookIdleEvent)
    UnhookEvent(hHookIdleEvent);

  UninitXStatusEvents();

  return 0;
}


static int OnSystemModulesLoaded(WPARAM wParam,LPARAM lParam)
{
  NETLIBUSER nlu = {0};
  char pszP2PName[MAX_PATH+3];
  char pszGroupsName[MAX_PATH+10];
  char pszSrvGroupsName[MAX_PATH+10];
  char szBuffer[MAX_PATH+64];
  char* modules[5] = {0,0,0,0,0};

  InitXStatusItems();

  strcpy(pszP2PName, gpszICQProtoName);
  strcat(pszP2PName, "P2P");

  strcpy(pszGroupsName, gpszICQProtoName);
  strcat(pszGroupsName, "Groups");
  strcpy(pszSrvGroupsName, gpszICQProtoName);
  strcat(pszSrvGroupsName, "SrvGroups");
  modules[0] = gpszICQProtoName;
  modules[1] = pszP2PName;
  modules[2] = pszGroupsName;
  modules[3] = pszSrvGroupsName;
  CallService("DBEditorpp/RegisterModule",(WPARAM)modules,(LPARAM)4);


  null_snprintf(szBuffer, sizeof szBuffer, Translate("%s server connection"), gpszICQProtoName);
  nlu.cbSize = sizeof(nlu);
  nlu.flags = NUF_OUTGOING | NUF_HTTPCONNS; 
  nlu.szDescriptiveName = szBuffer;
  nlu.szSettingsModule = gpszICQProtoName;

  if (ICQGetContactSettingByte(NULL, "UseGateway", 0))
  {
    nlu.flags |= NUF_HTTPGATEWAY;
    nlu.szHttpGatewayHello = "http://http.proxy.icq.com/hello";
    nlu.szHttpGatewayUserAgent = "Mozilla/4.08 [en] (WinNT; U ;Nav)";
    nlu.pfnHttpGatewayInit = icq_httpGatewayInit;
    nlu.pfnHttpGatewayBegin = icq_httpGatewayBegin;
    nlu.pfnHttpGatewayWrapSend = icq_httpGatewayWrapSend;
    nlu.pfnHttpGatewayUnwrapRecv = icq_httpGatewayUnwrapRecv;
  }
  ghServerNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

  null_snprintf(szBuffer, sizeof szBuffer, Translate("%s client-to-client connections"), gpszICQProtoName);
  nlu.flags = NUF_OUTGOING | NUF_INCOMING;
  nlu.szDescriptiveName = szBuffer;
  nlu.szSettingsModule = pszP2PName;
  nlu.minIncomingPorts = 1;
  ghDirectNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

  hHookOptionInit = HookEvent(ME_OPT_INITIALISE, IcqOptInit);
  hHookUserInfoInit = HookEvent(ME_USERINFO_INITIALISE, OnDetailsInit);

  hHookIdleEvent = HookEvent(ME_IDLE_CHANGED, IcqIdleChanged);

  InitDB();
  InitPopUps();

  InitXStatusEvents();

  return 0;
}



static int icq_PrebuildContactMenu(WPARAM wParam, LPARAM lParam)
{
  CLISTMENUITEM mi;
  
  ZeroMemory(&mi, sizeof(mi));
  mi.cbSize = sizeof(mi);
  if (!DBGetContactSettingByte((HANDLE)wParam, gpszICQProtoName, "Auth", 0))
    mi.flags = CMIM_FLAGS | CMIM_NAME | CMIF_HIDDEN;
  else
    mi.flags = CMIM_FLAGS | CMIM_NAME;
  mi.pszName = Translate("Request authorization");

  CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hUserMenuAuth, (LPARAM) & mi);

  if (!DBGetContactSettingByte((HANDLE)wParam, gpszICQProtoName, "Grant", 0))
    mi.flags = CMIM_FLAGS | CMIM_NAME | CMIF_HIDDEN;
  else
    mi.flags = CMIM_FLAGS | CMIM_NAME;
  mi.pszName = Translate("Grant authorization");

  CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hUserMenuGrant, (LPARAM) & mi);

  return 0;
}



void UpdateGlobalSettings()
{ 
  gbAimEnabled = ICQGetContactSettingByte(NULL, "AimEnabled", DEFAULT_AIM_ENABLED);
  gbUtfEnabled = ICQGetContactSettingByte(NULL, "UtfEnabled", DEFAULT_UTF_ENABLED);
  gwAnsiCodepage = ICQGetContactSettingWord(NULL, "AnsiCodePage", DEFAULT_ANSI_CODEPAGE);
  gbDCMsgEnabled = ICQGetContactSettingByte(NULL, "DirectMessaging", DEFAULT_DCMSG_ENABLED);
  gbTempVisListEnabled = ICQGetContactSettingByte(NULL, "TempVisListEnabled", DEFAULT_TEMPVIS_ENABLED);
  gbSsiEnabled = ICQGetContactSettingByte(NULL, "UseServerCList", DEFAULT_SS_ENABLED);
  gbAvatarsEnabled = ICQGetContactSettingByte(NULL, "AvatarsEnabled", DEFAULT_AVATARS_ENABLED);
  gbXStatusEnabled = ICQGetContactSettingByte(NULL, "XStatusEnabled", DEFAULT_XSTATUS_ENABLED);
}
