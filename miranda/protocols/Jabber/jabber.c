/*

Jabber Protocol Plugin for Miranda IM
Copyright (C) 2002-2004  Santithorn Bunchua

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "jabber.h"
#include "jabber_ssl.h"
#include "jabber_iq.h"
#include "resource.h"
#include <richedit.h>

HINSTANCE hInst;
PLUGINLINK *pluginLink;

PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
	"Jabber Protocol",
	PLUGIN_MAKE_VERSION(0,1,1,8),
	//"Jabber protocol plugin for Miranda IM",
	"Jabber protocol plugin for Miranda IM (pre-0.1.1.9 cvs "__DATE__")",
	"Santithorn Bunchua",
	"keh@au.edu",
	"(c) 2002-2004 Santithorn Bunchua",
	"http://jabber.au.edu/miranda",
	0,
	0
};

HANDLE hMainThread;
DWORD jabberMainThreadId;
char *jabberProtoName;	// "JABBER"
char *jabberModuleName;	// "Jabber"
CRITICAL_SECTION mutex;
HANDLE hNetlibUser;
// Main jabber server connection thread global variables
struct ThreadData *jabberThreadInfo;
BOOL jabberConnected;
BOOL jabberOnline;
int jabberStatus;
int jabberDesiredStatus;
BOOL modeMsgStatusChangePending;
BOOL jabberChangeStatusMessageOnly;
char *jabberJID = NULL;
char *streamId;
DWORD jabberLocalIP;
UINT jabberCodePage;
JABBER_MODEMSGS modeMsgs;
//char *jabberModeMsg;
CRITICAL_SECTION modeMsgMutex;
char *jabberVcardPhotoFileName;
char *jabberVcardPhotoType;
BOOL jabberSendKeepAlive;
HICON jabberIcon[JABBER_ICON_TOTAL];

HANDLE hWndListGcLog;
HANDLE hEventSettingChanged;
HANDLE hEventContactDeleted;
HANDLE hMenuAgent;
HANDLE hMenuChangePassword;
HANDLE hMenuGroupchat;
HANDLE hMenuRequestAuth;
HANDLE hMenuGrantAuth;
// SSL-related global variable
HANDLE hLibSSL;
PVOID jabberSslCtx;

HWND hwndJabberAgents;
HWND hwndJabberGroupchat;
HWND hwndJabberJoinGroupchat;
HWND hwndAgentReg;
HWND hwndAgentRegInput;
HWND hwndAgentManualReg;
HWND hwndRegProgress;
HWND hwndJabberVcard;
HWND hwndMucVoiceList;
HWND hwndMucMemberList;
HWND hwndMucModeratorList;
HWND hwndMucBanList;
HWND hwndMucAdminList;
HWND hwndMucOwnerList;
HWND hwndJabberChangePassword;

int JabberOptInit(WPARAM wParam, LPARAM lParam);
int JabberUserInfoInit(WPARAM wParam, LPARAM lParam);
int JabberMsgUserTyping(WPARAM wParam, LPARAM lParam);
int JabberSvcInit(void);
int JabberMenuHandleAgents(WPARAM wParam, LPARAM lParam);
int JabberMenuHandleChangePassword(WPARAM wParam, LPARAM lParam);
int JabberMenuHandleVcard(WPARAM wParam, LPARAM lParam);
int JabberMenuHandleRequestAuth(WPARAM wParam, LPARAM lParam);
int JabberMenuHandleGrantAuth(WPARAM wParam, LPARAM lParam);
int JabberMenuPrebuildContactMenu(WPARAM wParam, LPARAM lParam);

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	hInst = hModule;
	return TRUE;
}

__declspec(dllexport) PLUGININFO *MirandaPluginInfo(DWORD mirandaVersion)
{
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0,3,3,0)) {
		MessageBox(NULL, "The Jabber protocol plugin cannot be loaded. It requires Miranda IM 0.3.3 or later.", "Jabber Protocol Plugin", MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST);
		return NULL;
	}
	return &pluginInfo;
}

int __declspec(dllexport) Unload(void)
{
	int i;

#ifdef _DEBUG
	OutputDebugString("Unloading...");
#endif
	JabberGcLogUninit();
	UnhookEvent(hEventSettingChanged);
	UnhookEvent(hEventContactDeleted);
	JabberWsUninit();
	JabberSslUninit();
	JabberListUninit();
	JabberIqUninit();
	JabberSerialUninit();
	DeleteCriticalSection(&modeMsgMutex);
	DeleteCriticalSection(&mutex);
	free(modeMsgs.szOnline);
	free(modeMsgs.szAway);
	free(modeMsgs.szNa);
	free(modeMsgs.szDnd);
	free(modeMsgs.szFreechat);
	//free(jabberModeMsg);
	free(jabberModuleName);
	free(jabberProtoName);
	if (jabberVcardPhotoFileName) {
		DeleteFile(jabberVcardPhotoFileName);
		free(jabberVcardPhotoFileName);
	}
	if (jabberVcardPhotoType) free(jabberVcardPhotoType);
	if (streamId) free(streamId);

	for (i=0; i<JABBER_ICON_TOTAL; i++)
		DestroyIcon(jabberIcon[i]);

#ifdef _DEBUG
	OutputDebugString("Finishing Unload, returning to Miranda");
	//OutputDebugString("========== Memory leak from JABBER");
	//_CrtDumpMemoryLeaks();
	//OutputDebugString("========== End memory leak from JABBER");
#endif
	return 0;
}

static int PreShutdown(WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
	OutputDebugString("PreShutdown...");
#endif

	WindowList_Broadcast(hWndListGcLog, WM_JABBER_SHUTDOWN, 0, 0);
	if (hwndJabberAgents) SendMessage(hwndJabberAgents, WM_CLOSE, 0, 0);
	if (hwndJabberGroupchat) SendMessage(hwndJabberGroupchat, WM_CLOSE, 0, 0);
	if (hwndJabberJoinGroupchat) SendMessage(hwndJabberJoinGroupchat, WM_CLOSE, 0, 0);
	if (hwndAgentReg) SendMessage(hwndAgentReg, WM_CLOSE, 0, 0);
	if (hwndAgentRegInput) SendMessage(hwndAgentRegInput, WM_CLOSE, 0, 0);
	if (hwndRegProgress) SendMessage(hwndRegProgress, WM_CLOSE, 0, 0);
	if (hwndJabberVcard) SendMessage(hwndJabberVcard, WM_CLOSE, 0, 0);
	if (hwndMucVoiceList) SendMessage(hwndMucVoiceList, WM_CLOSE, 0, 0);
	if (hwndMucMemberList) SendMessage(hwndMucMemberList, WM_CLOSE, 0, 0);
	if (hwndMucModeratorList) SendMessage(hwndMucModeratorList, WM_CLOSE, 0, 0);
	if (hwndMucBanList) SendMessage(hwndMucBanList, WM_CLOSE, 0, 0);
	if (hwndMucAdminList) SendMessage(hwndMucAdminList, WM_CLOSE, 0, 0);
	if (hwndMucOwnerList) SendMessage(hwndMucOwnerList, WM_CLOSE, 0, 0);
	if (hwndJabberChangePassword) SendMessage(hwndJabberChangePassword, WM_CLOSE, 0, 0);

	hwndJabberAgents = NULL;
	hwndJabberGroupchat = NULL;
	hwndJabberJoinGroupchat = NULL;
	hwndAgentReg = NULL;
	hwndAgentRegInput = NULL;
	hwndAgentManualReg = NULL;
	hwndRegProgress = NULL;
	hwndJabberVcard = NULL;
	hwndMucVoiceList = NULL;
	hwndMucMemberList = NULL;
	hwndMucModeratorList = NULL;
	hwndMucBanList = NULL;
	hwndMucAdminList = NULL;
	hwndMucOwnerList = NULL;
	hwndJabberChangePassword = NULL;

	return 0;
}

static int ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	JabberWsInit();
	JabberSslInit();
	HookEvent(ME_USERINFO_INITIALISE, JabberUserInfoInit);
	return 0;
}

static void JabberIconInit()
{
	int i;
	static int iconList[] = {
		IDI_GCOWNER,
		IDI_GCADMIN,
		IDI_GCMODERATOR,
		IDI_GCVOICE
	};

	for (i=0; i<JABBER_ICON_TOTAL; i++)
		jabberIcon[i] = LoadImage(hInst, MAKEINTRESOURCE(iconList[i]), IMAGE_ICON, 0, 0, 0);
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	PROTOCOLDESCRIPTOR pd;
	HANDLE hContact;
	char text[_MAX_PATH];
	char *p, *q;
	char *szProto;
	CLISTMENUITEM mi, clmi;

	GetModuleFileName(hInst, text, sizeof(text));
	p = strrchr(text, '\\');
	p++;
	q = strrchr(p, '.');
	*q = '\0';
	jabberProtoName = _strdup(p);
	_strupr(jabberProtoName);

	jabberModuleName = _strdup(jabberProtoName);
	_strlwr(jabberModuleName);
	jabberModuleName[0] = toupper(jabberModuleName[0]);

	JabberLog("Setting protocol/module name to '%s/%s'", jabberProtoName, jabberModuleName);

	pluginLink = link;
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hMainThread, THREAD_SET_CONTEXT, FALSE, 0);
	jabberMainThreadId = GetCurrentThreadId();
	hLibSSL = NULL;
	hwndJabberAgents = NULL;
	hwndJabberGroupchat = NULL;
	hwndJabberJoinGroupchat = NULL;
	hwndAgentReg = NULL;
	hwndAgentRegInput = NULL;
	hwndAgentManualReg = NULL;
	hwndRegProgress = NULL;
	hwndJabberVcard = NULL;
	hwndMucVoiceList = NULL;
	hwndMucMemberList = NULL;
	hwndMucModeratorList = NULL;
	hwndMucBanList = NULL;
	hwndMucAdminList = NULL;
	hwndMucOwnerList = NULL;
	hwndJabberChangePassword = NULL;
	hWndListGcLog = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);

	HookEvent(ME_OPT_INITIALISE, JabberOptInit);
	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);

	// Register protocol module
	ZeroMemory(&pd, sizeof(PROTOCOLDESCRIPTOR));
	pd.cbSize = sizeof(PROTOCOLDESCRIPTOR);
	pd.szName = jabberProtoName;
	pd.type = PROTOTYPE_PROTOCOL;
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) &pd);

	memset(&mi, 0, sizeof(CLISTMENUITEM));
	mi.cbSize = sizeof(CLISTMENUITEM);
	memset(&clmi, 0, sizeof(CLISTMENUITEM));
	clmi.cbSize = sizeof(CLISTMENUITEM);
	clmi.flags = CMIM_FLAGS | CMIF_GRAYED;

	// Add Jabber menu to the main menu
	// "Agents..."
	mi.pszPopupName = jabberModuleName;
	mi.popupPosition = 500090000;
	wsprintf(text, "%s/Agents", jabberModuleName);
	CreateServiceFunction(text, JabberMenuHandleAgents);
	mi.pszName = Translate("Agents...");
	mi.position = 2000050000;
	mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_AGENTS));
	mi.pszService = text;
	hMenuAgent = (HANDLE) CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMenuAgent, (LPARAM) &clmi);
	// "Change Password..."
	wsprintf(text, "%s/ChangePassword", jabberModuleName);
	CreateServiceFunction(text, JabberMenuHandleChangePassword);
	mi.pszName = Translate("Change Password...");
	mi.position = 2000050001;
	mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_KEYS));
	mi.pszService = text;
	hMenuChangePassword = (HANDLE) CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMenuChangePassword, (LPARAM) &clmi);
	// "Multi-User Conference..."
	wsprintf(text, "%s/Groupchat", jabberModuleName);
	CreateServiceFunction(text, JabberMenuHandleGroupchat);
	mi.pszName = Translate("Multi-User Conference...");
	mi.position = 2000050002;
	mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_GROUP));
	mi.pszService = text;
	hMenuGroupchat = (HANDLE) CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMenuGroupchat, (LPARAM) &clmi);
	// "Personal vCard..."
	wsprintf(text, "%s/Vcard", jabberModuleName);
	CreateServiceFunction(text, JabberMenuHandleVcard);
	mi.pszName = Translate("Personal vCard...");
	mi.position = 2000050003;
	mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_VCARD));
	mi.pszService = text;
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);

	// Add contact menu
	// "Request authorization"
	sprintf(text, "%s/RequestAuth", jabberModuleName);
	CreateServiceFunction(text, JabberMenuHandleRequestAuth);
	mi.pszName = Translate("Request authorization");
	mi.position = -2000001001;
	mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_REQUEST));
	mi.pszService = text;
	mi.pszContactOwner = jabberProtoName;
	hMenuRequestAuth = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) &mi);
	// "Grant authorization"
	sprintf(text, "%s/GrantAuth", jabberModuleName);
	CreateServiceFunction(text, JabberMenuHandleGrantAuth);
	mi.pszName = Translate("Grant authorization");
	mi.position = -2000001000;
	mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_GRANT));
	mi.pszService = text;
	mi.pszContactOwner = jabberProtoName;
	hMenuGrantAuth = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) &mi);

	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, JabberMenuPrebuildContactMenu);

	// Set all contacts to offline
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if(szProto!=NULL && !strcmp(szProto, jabberProtoName)) {
			if (DBGetContactSettingWord(hContact, jabberProtoName, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
				DBWriteContactSettingWord(hContact, jabberProtoName, "Status", ID_STATUS_OFFLINE);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	streamId = NULL;
	jabberThreadInfo = NULL;
	jabberConnected = FALSE;
	jabberOnline = FALSE;
	modeMsgStatusChangePending = FALSE;
	jabberStatus = ID_STATUS_OFFLINE;
	jabberChangeStatusMessageOnly = FALSE;
	jabberVcardPhotoFileName = NULL;
	jabberVcardPhotoType = NULL;
	memset((char *) &modeMsgs, 0, sizeof(JABBER_MODEMSGS));
	//jabberModeMsg = NULL;
	jabberCodePage = DBGetContactSettingWord(NULL, jabberProtoName, "CodePage", CP_ACP);

	InitializeCriticalSection(&mutex);
	InitializeCriticalSection(&modeMsgMutex);

	JabberIconInit();
	srand((unsigned) time(NULL));
	JabberSerialInit();
	JabberIqInit();
	JabberListInit();
	JabberSvcInit();
	JabberGcLogInit();

	return 0;
}
