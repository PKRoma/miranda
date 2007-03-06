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
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

#include "m_updater.h"

PLUGINLINK* pluginLink;
struct MD5_INTERFACE md5i;
HANDLE hHookUserInfoInit = NULL;
HANDLE hHookOptionInit = NULL;
HANDLE hHookUserMenu = NULL;
HANDLE hHookIdleEvent = NULL;
HANDLE hHookIconsChanged = NULL;
static HANDLE hUserMenuAddServ = NULL;
static HANDLE hUserMenuAuth = NULL;
static HANDLE hUserMenuGrant = NULL;
static HANDLE hUserMenuRevoke = NULL;
static HANDLE hUserMenuXStatus = NULL;

extern HANDLE hServerConn;
CRITICAL_SECTION localSeqMutex;
CRITICAL_SECTION connectionHandleMutex;
HANDLE hsmsgrequest;
HANDLE hxstatusiconchanged;

extern int bHideXStatusUI;

PLUGININFOEX pluginInfo = {
  sizeof(PLUGININFOEX),
  NULL,
  PLUGIN_MAKE_VERSION(0,3,10,1),
  "Support for ICQ network, enhanced.",
  "Joe Kucera, Bio, Martin Öberg, Richard Hughes, Jon Keating, etc",
  "jokusoftware@miranda-im.org",
  "(C) 2000-2007 M.Öberg, R.Hughes, J.Keating, Bio, Angeli-Ka, J.Kucera",
  "http://addons.miranda-im.org/details.php?action=viewfile&id=1683",
  0,  //not transient
  0,   //doesn't replace anything built-in
  {0x847bb03c, 0x408c, 0x4f9b, { 0xaa, 0x5a, 0xf5, 0xc0, 0xb7, 0xb5, 0x60, 0x1e }} //{847BB03C-408C-4f9b-AA5A-F5C0B7B5601E}
};

static char pluginName[64];

static int OnSystemModulesLoaded(WPARAM wParam,LPARAM lParam);
static int OnSystemPreShutdown(WPARAM wParam,LPARAM lParam);
static int icq_PrebuildContactMenu(WPARAM wParam, LPARAM lParam);
static int IconLibIconsChanged(WPARAM wParam, LPARAM lParam);

static BOOL bInited = FALSE;

PLUGININFOEX __declspec(dllexport) *MirandaPluginInfoEx(DWORD mirandaVersion)
{
  // Only load for 0.7.0.12 or greater
  // We need core MD5 interface
  if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 7, 0, 12)) 
  {
    MessageBox( NULL, "ICQ plugin cannot be loaded. It requires Miranda IM 0.7.0.12 or later.", "ICQ Plugin", 
      MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
    return NULL;
  }
  else if (!bInited)
  {
    // Are we running under Unicode Windows version ?
    gbUnicodeAPI = (GetVersion() & 0x80000000) == 0;
    strcpy(pluginName, "IcqOscarJ Protocol");
    if (gbUnicodeAPI)
    {
      strcat(pluginName, " (Unicode)");
      pluginInfo.flags = 1; // UNICODE_AWARE
    }
    pluginInfo.shortName = pluginName;
    MIRANDA_VERSION = mirandaVersion;

    bInited = TRUE;
  }
  return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
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
  PROTOCOLDESCRIPTOR pd = {0};

  pluginLink = link;
  mir_getMD5I( &md5i );

  ghServerNetlibUser = NULL;

  // Are we running under Unicode Windows version ?
  gbUnicodeAPI = (GetVersion() & 0x80000000) == 0;
  // Do we have new LangPack module ready ?
  gbUtfLangpack = ServiceExists(MS_LANGPACK_GETCODEPAGE);
  { // Are we running under unicode Miranda core ?
    char szVer[MAX_PATH];

    CallService(MS_SYSTEM_GETVERSIONTEXT, MAX_PATH, (LPARAM)szVer);
    gbUnicodeCore = (strstr(szVer, "Unicode") != NULL);

    if (strstr(szVer, "alpha") != NULL)
    { // Are we running under Alpha Core
      MIRANDA_VERSION |= 0x80000000;
    }
    else if (MIRANDA_VERSION >= 0x00050000 && strstr(szVer, "Preview") == NULL)
    { // for Final Releases of Miranda 0.5+ clear build number
      MIRANDA_VERSION &= 0xFFFFFF00;
    }
  }

  srand(time(NULL));
  _tzset();

  // Get module name from DLL file name
  {
    char* str1;
    char str2[MAX_PATH];
    int nProtoNameLen;

    GetModuleFileName(hInst, str2, MAX_PATH);
    str1 = strrchr(str2, '\\');
    nProtoNameLen = strlennull(str1);
    if (str1 != NULL && (nProtoNameLen > 5))
    {
      strncpy(gpszICQProtoName, str1+1, nProtoNameLen-5);
      gpszICQProtoName[nProtoNameLen-4] = 0;
    }
    CharUpper(gpszICQProtoName);
  }

  ZeroMemory(gpszPassword, sizeof(gpszPassword));

  icq_FirstRunCheck();

  HookEvent(ME_SYSTEM_MODULESLOADED, OnSystemModulesLoaded);
  HookEvent(ME_SYSTEM_PRESHUTDOWN, OnSystemPreShutdown);

  InitializeCriticalSection(&connectionHandleMutex);
  InitializeCriticalSection(&localSeqMutex);
  InitializeCriticalSection(&modeMsgsMutex);

  // Initialize core modules
  InitDB();       // DB interface
  InitCookies();  // cookie utils
  InitCache();    // contacts cache
  InitRates();    // rate management

  // Register the module
  pd.cbSize = sizeof(pd);
  pd.szName = gpszICQProtoName;
  pd.type   = PROTOTYPE_PROTOCOL;
  CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM)&pd);

  // Initialize status message struct
  ZeroMemory(&modeMsgs, sizeof(icq_mode_messages));

  // Initialize temporary DB settings
  ICQCreateResidentSetting("Status"); // NOTE: XStatus cannot be temporary
  ICQCreateResidentSetting("TemporaryVisible");
  ICQCreateResidentSetting("TickTS");
  ICQCreateResidentSetting("IdleTS");
  ICQCreateResidentSetting("LogonTS");
  ICQCreateResidentSetting("DCStatus");

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
  // Session password API
  ICQCreateServiceFunction(PS_ICQ_SETPASSWORD, IcqSetPassword);
  // ChangeInfo API
  ICQCreateServiceFunction(PS_CHANGEINFOEX, IcqChangeInfoEx);
  // My Avatar API
  ICQCreateServiceFunction(PS_ICQ_GETMYAVATARMAXSIZE, IcqGetMaxAvatarSize);
  ICQCreateServiceFunction(PS_ICQ_ISAVATARFORMATSUPPORTED, IcqAvatarFormatSupported);
  ICQCreateServiceFunction(PS_ICQ_GETMYAVATAR, IcqGetMyAvatar);
  ICQCreateServiceFunction(PS_ICQ_SETMYAVATAR, IcqSetMyAvatar);
  // Custom Status API
  ICQCreateServiceFunction(PS_ICQ_SETCUSTOMSTATUS, IcqSetXStatus); // obsolete (remove in next version)
  ICQCreateServiceFunction(PS_ICQ_GETCUSTOMSTATUS, IcqGetXStatus); // obsolete
  ICQCreateServiceFunction(PS_ICQ_SETCUSTOMSTATUSEX, IcqSetXStatusEx);
  ICQCreateServiceFunction(PS_ICQ_GETCUSTOMSTATUSEX, IcqGetXStatusEx);
  ICQCreateServiceFunction(PS_ICQ_GETCUSTOMSTATUSICON, IcqGetXStatusIcon);
  ICQCreateServiceFunction(PS_ICQ_REQUESTCUSTOMSTATUS, IcqRequestXStatusDetails);

  {
    char pszServiceName[MAX_PATH + 32];

    strcpy(pszServiceName, gpszICQProtoName); strcat(pszServiceName, ME_ICQ_STATUSMSGREQ);
    hsmsgrequest = CreateHookableEvent(pszServiceName);

    strcpy(pszServiceName, gpszICQProtoName); strcat(pszServiceName, ME_ICQ_CUSTOMSTATUS_EXTRAICON_CHANGED);
    hxstatusiconchanged = CreateHookableEvent(pszServiceName);
  }

  InitDirectConns();
  InitOscarFileTransfer();
  InitServerLists();
  icq_InitInfoUpdate();

  // Initialize charset conversion routines
  InitI18N();

  UpdateGlobalSettings();

  gnCurrentStatus = ID_STATUS_OFFLINE;

  ICQCreateServiceFunction(MS_ICQ_ADDSERVCONTACT, IcqAddServerContact);

  ICQCreateServiceFunction(MS_REQ_AUTH, icq_RequestAuthorization);
  ICQCreateServiceFunction(MS_GRANT_AUTH, IcqGrantAuthorization);
  ICQCreateServiceFunction(MS_REVOKE_AUTH, IcqRevokeAuthorization);

  ICQCreateServiceFunction(MS_XSTATUS_SHOWDETAILS, IcqShowXStatusDetails);

  // This must be here - the events are called too early, WTF?
  InitXStatusEvents();

  return 0;
}



int __declspec(dllexport) Unload(void)
{
  if (gbXStatusEnabled) gbXStatusEnabled = 10; // block clist changing

  UninitXStatusEvents();

  UninitServerLists();
  UninitOscarFileTransfer();
  UninitDirectConns();

  NetLib_SafeCloseHandle(&ghDirectNetlibUser, FALSE);
  NetLib_SafeCloseHandle(&ghServerNetlibUser, FALSE);
  UninitRates();
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

  if (hHookIconsChanged)
    UnhookEvent(hHookIconsChanged);

  if (hHookUserInfoInit)
    UnhookEvent(hHookUserInfoInit);

  if (hHookOptionInit)
    UnhookEvent(hHookOptionInit);

  if (hsmsgrequest)
    DestroyHookableEvent(hsmsgrequest);

  if (hxstatusiconchanged)
    DestroyHookableEvent(hxstatusiconchanged);

  if (hHookUserMenu)
    UnhookEvent(hHookUserMenu);

  if (hHookIdleEvent)
    UnhookEvent(hHookIdleEvent);

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


  null_snprintf(szBuffer, sizeof szBuffer, ICQTranslate("%s server connection"), gpszICQProtoName);
  nlu.cbSize = sizeof(nlu);
  nlu.flags = NUF_OUTGOING | NUF_HTTPGATEWAY;
  nlu.szDescriptiveName = szBuffer;
  nlu.szSettingsModule = gpszICQProtoName;
  nlu.szHttpGatewayHello = "http://http.proxy.icq.com/hello";
  nlu.szHttpGatewayUserAgent = "Mozilla/4.08 [en] (WinNT; U ;Nav)";
  nlu.pfnHttpGatewayInit = icq_httpGatewayInit;
  nlu.pfnHttpGatewayBegin = icq_httpGatewayBegin;
  nlu.pfnHttpGatewayWrapSend = icq_httpGatewayWrapSend;
  nlu.pfnHttpGatewayUnwrapRecv = icq_httpGatewayUnwrapRecv;

  ghServerNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

  null_snprintf(szBuffer, sizeof szBuffer, ICQTranslate("%s client-to-client connections"), gpszICQProtoName);
  nlu.flags = NUF_OUTGOING | NUF_INCOMING;
  nlu.szDescriptiveName = szBuffer;
  nlu.szSettingsModule = pszP2PName;
  nlu.minIncomingPorts = 1;
  ghDirectNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

  hHookOptionInit = HookEvent(ME_OPT_INITIALISE, IcqOptInit);
  hHookUserInfoInit = HookEvent(ME_USERINFO_INITIALISE, OnDetailsInit);
  hHookUserMenu = HookEvent(ME_CLIST_PREBUILDCONTACTMENU, icq_PrebuildContactMenu);
  hHookIdleEvent = HookEvent(ME_IDLE_CHANGED, IcqIdleChanged);

  InitAvatars();

  // Init extra optional modules
  InitPopUps();
  InitIconLib();

  hHookIconsChanged = IconLibHookIconsChanged(IconLibIconsChanged);

  {
    char str[MAX_PATH], proto[MAX_PATH], lib[MAX_PATH];
    
    ICQTranslateUtfStatic(gpszICQProtoName, proto);

    GetModuleFileName(hInst, lib, MAX_PATH);

    IconLibDefine(ICQTranslateUtfStatic("Request authorization", str), proto, "req_auth", NULL, lib, -IDI_AUTH_ASK);
    IconLibDefine(ICQTranslateUtfStatic("Grant authorization", str), proto, "grant_auth", NULL, lib, -IDI_AUTH_GRANT);
    IconLibDefine(ICQTranslateUtfStatic("Revoke authorization", str), proto, "revoke_auth", NULL, lib, -IDI_AUTH_REVOKE);
    IconLibDefine(ICQTranslateUtfStatic("Add to server list", str), proto, "add_to_server", NULL, NULL, 0);
  }

  // Initialize IconLib icons
  InitXStatusIcons();
  InitXStatusEvents();
  InitXStatusItems(FALSE);

  {
    CLISTMENUITEM mi;
    char pszServiceName[MAX_PATH+30];

    strcpy(pszServiceName, gpszICQProtoName);
    strcat(pszServiceName, MS_REQ_AUTH);

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.position = 1000030000;
    mi.flags = 0;
    mi.hIcon = IconLibGetIcon("req_auth");
    mi.pszContactOwner = gpszICQProtoName;
    mi.pszName = ICQTranslate("Request authorization");
    mi.pszService = pszServiceName;
    hUserMenuAuth = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
    IconLibReleaseIcon("req_auth");

    strcpy(pszServiceName, gpszICQProtoName);
    strcat(pszServiceName, MS_GRANT_AUTH);

    mi.position = 1000029999;
    mi.hIcon = IconLibGetIcon("grant_auth");
    mi.pszName = ICQTranslate("Grant authorization");
    hUserMenuGrant = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
    IconLibReleaseIcon("grant_auth");

    strcpy(pszServiceName, gpszICQProtoName);
    strcat(pszServiceName, MS_REVOKE_AUTH);

    mi.position = 1000029998;
    mi.hIcon = IconLibGetIcon("revoke_auth");
    mi.pszName = ICQTranslate("Revoke authorization");
    hUserMenuRevoke = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
    IconLibReleaseIcon("revoke_auth");

    strcpy(pszServiceName, gpszICQProtoName);
    strcat(pszServiceName, MS_ICQ_ADDSERVCONTACT);

    mi.position = -2049999999;
    mi.hIcon = IconLibGetIcon("add_to_server");
    mi.pszName = ICQTranslate("Add to server list");
    hUserMenuAddServ = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
    IconLibReleaseIcon("add_to_server");

    strcpy(pszServiceName, gpszICQProtoName);
    strcat(pszServiceName, MS_XSTATUS_SHOWDETAILS);

    mi.position = -2000004999;
    mi.hIcon = NULL; // dynamically updated
    mi.pszName = ICQTranslate("Show custom status details");
    mi.flags=CMIF_NOTOFFLINE;
    hUserMenuXStatus = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
  }

  {
    // TODO: add beta builds support to devel builds :)

    CallService(MS_UPDATE_REGISTERFL, 1683, (WPARAM)&pluginInfo);
  }

  return 0;
}



static int OnSystemPreShutdown(WPARAM wParam,LPARAM lParam)
{ // all threads should be terminated here
  if (hServerConn)
  {
    icq_sendCloseConnection();

    icq_serverDisconnect(TRUE);
  }

  icq_InfoUpdateCleanup();

  return 0;
}



void CListShowMenuItem(HANDLE hMenuItem, BYTE bShow)
{
  CLISTMENUITEM mi = {0};

  mi.cbSize = sizeof(mi);
  if (bShow)
    mi.flags = CMIM_FLAGS;
  else
    mi.flags = CMIM_FLAGS | CMIF_HIDDEN;

  CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuItem, (LPARAM)&mi);
}



void CListSetMenuItemIcon(HANDLE hMenuItem, HICON hIcon)
{
  CLISTMENUITEM mi = {0};

  mi.cbSize = sizeof(mi);
  mi.flags = CMIM_FLAGS | CMIM_ICON;

  mi.hIcon = hIcon;
  CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuItem, (LPARAM)&mi);
}



static int icq_PrebuildContactMenu(WPARAM wParam, LPARAM lParam)
{
  BYTE bXStatus;

  CListShowMenuItem(hUserMenuAuth, ICQGetContactSettingByte((HANDLE)wParam, "Auth", 0));
  CListShowMenuItem(hUserMenuGrant, ICQGetContactSettingByte((HANDLE)wParam, "Grant", 0));
  CListShowMenuItem(hUserMenuRevoke, (BYTE)(ICQGetContactSettingByte(NULL, "PrivacyItems", 0) && !ICQGetContactSettingByte((HANDLE)wParam, "Grant", 0)));
  if (gbSsiEnabled && !ICQGetContactSettingWord((HANDLE)wParam, "ServerId", 0) && !ICQGetContactSettingWord((HANDLE)wParam, "SrvIgnoreId", 0))
    CListShowMenuItem(hUserMenuAddServ, 1);
  else
    CListShowMenuItem(hUserMenuAddServ, 0);

  bXStatus = ICQGetContactSettingByte((HANDLE)wParam, DBSETTING_XSTATUSID, 0);
  CListShowMenuItem(hUserMenuXStatus, (BYTE)(bHideXStatusUI ? 0 : bXStatus));
  if (bXStatus && !bHideXStatusUI)
  {
    CListSetMenuItemIcon(hUserMenuXStatus, GetXStatusIcon(bXStatus, LR_SHARED));
  }

  return 0;
}



static int IconLibIconsChanged(WPARAM wParam, LPARAM lParam)
{
  CListSetMenuItemIcon(hUserMenuAuth, IconLibGetIcon("req_auth"));
  IconLibReleaseIcon("req_auth");
  CListSetMenuItemIcon(hUserMenuGrant, IconLibGetIcon("grant_auth"));
  IconLibReleaseIcon("grant_auth");
  CListSetMenuItemIcon(hUserMenuRevoke, IconLibGetIcon("revoke_auth"));
  IconLibReleaseIcon("revoke_auth");
  CListSetMenuItemIcon(hUserMenuAddServ, IconLibGetIcon("add_to_server"));
  IconLibReleaseIcon("add_to_server");

  ChangedIconsXStatus();

  return 0;
}



void UpdateGlobalSettings()
{
  if (ghServerNetlibUser)
  {
    NETLIBUSERSETTINGS nlus = {0};

    nlus.cbSize = sizeof(NETLIBUSERSETTINGS);
    if (CallService(MS_NETLIB_GETUSERSETTINGS, (WPARAM)ghServerNetlibUser, (LPARAM)&nlus))
    {
      if (nlus.useProxy && nlus.proxyType == PROXYTYPE_HTTP)
        gbGatewayMode = 1;
      else
        gbGatewayMode = 0;
    }
    else
      gbGatewayMode = 0;
  }

  gbSecureLogin = ICQGetContactSettingByte(NULL, "SecureLogin", DEFAULT_SECURE_LOGIN);
  gbAimEnabled = ICQGetContactSettingByte(NULL, "AimEnabled", DEFAULT_AIM_ENABLED);
  gbUtfEnabled = ICQGetContactSettingByte(NULL, "UtfEnabled", DEFAULT_UTF_ENABLED);
  gwAnsiCodepage = ICQGetContactSettingWord(NULL, "AnsiCodePage", DEFAULT_ANSI_CODEPAGE);
  gbDCMsgEnabled = ICQGetContactSettingByte(NULL, "DirectMessaging", DEFAULT_DCMSG_ENABLED);
  gbTempVisListEnabled = ICQGetContactSettingByte(NULL, "TempVisListEnabled", DEFAULT_TEMPVIS_ENABLED);
  gbSsiEnabled = ICQGetContactSettingByte(NULL, "UseServerCList", DEFAULT_SS_ENABLED);
  gbAvatarsEnabled = ICQGetContactSettingByte(NULL, "AvatarsEnabled", DEFAULT_AVATARS_ENABLED);
  gbXStatusEnabled = ICQGetContactSettingByte(NULL, "XStatusEnabled", DEFAULT_XSTATUS_ENABLED);
}
