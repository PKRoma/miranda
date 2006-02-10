/*
Miranda plugin template, originally by Richard Hughes
http://miranda-icq.sourceforge.net/

This file is placed in the public domain. Anybody is free to use or
modify it as they wish with no restriction.
There is no warranty.
*/
#include "aim.h"
PLUGINLINK *pluginLink;
#define AIM_OSCAR_VERSION "\0\0\0\x01"
char	AIM_CAP_MIRANDA[16]="MirandaA\0\0\0\0\0\0\0";
PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"AIM OSCAR Plugin - Alpha 8.2",
	PLUGIN_MAKE_VERSION(0,0,0,1),
	"Provides very basic support for AOL® OSCAR Instant Messenger protocol.",
	"Aaron Myles Landwehr",
	"aaron@snaphat.com",
	"© 2005-2006 Aaron Myles Landwehr",
	"http://www.snaphat.com/oscar",
	0,		//not transient
	0		//doesn't replace anything built-in
};
oscar_data conn;
file_transfer* fu;
extern "C" __declspec(dllexport) bool WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	conn.hInstance = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	unsigned long mv=htonl(mirandaVersion);
	memcpy((char*)&AIM_CAP_MIRANDA[8],&mv,sizeof(DWORD));
	memcpy((char*)&AIM_CAP_MIRANDA[12],(char*)&AIM_OSCAR_VERSION,sizeof(DWORD));
	return &pluginInfo;
}
char* CWD;//current working directory
char* AIM_PROTOCOL_NAME;
char* GROUP_ID_KEY;
char* ID_GROUP_KEY;
char* FILE_TRANSFER_KEY;
CRITICAL_SECTION modeMsgsMutex;
extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	PROTOCOLDESCRIPTOR pd;
	char *str1;
	//start locating memory for dynamically created structures
	char str2[MAX_PATH];
	char store[MAX_PATH];
	GetCurrentDirectory(sizeof(store),store);
	CWD=(char*)realloc(CWD,strlen(store)+1);
	memcpy(CWD,store,strlen(store));
	memcpy(&CWD[strlen(store)],"\0",1);
	GetModuleFileName(conn.hInstance, str2, MAX_PATH);
    str1 = strrchr(str2, '\\');
	if (str1 != NULL && (strlen(str1 + 1) > 4))
	{
		ZeroMemory(store,sizeof(store));
		strncpy(store, str1 + 1, strlen(str1 + 1) - 4);
		AIM_PROTOCOL_NAME=(char*)realloc(AIM_PROTOCOL_NAME,strlen(store)+1);
		memcpy(AIM_PROTOCOL_NAME,store,strlen(store));
		memcpy(&AIM_PROTOCOL_NAME[strlen(store)],"\0",1);
	}
	CharUpper(AIM_PROTOCOL_NAME);

	char groupid_key[32];//group to id
	ZeroMemory(groupid_key,sizeof(groupid_key));
	memcpy(groupid_key,AIM_PROTOCOL_NAME,strlen(AIM_PROTOCOL_NAME));
	memcpy(&groupid_key[strlen(AIM_PROTOCOL_NAME)],AIM_MOD_GI,strlen(AIM_MOD_GI));
	GROUP_ID_KEY=(char*)realloc(GROUP_ID_KEY,strlen(groupid_key)+1);
	memcpy(GROUP_ID_KEY,groupid_key,strlen(groupid_key));
	memcpy(&GROUP_ID_KEY[strlen(groupid_key)],"\0",1);
	char idgroup_key[32];//id to group
	ZeroMemory(idgroup_key,sizeof(idgroup_key));
	memcpy(idgroup_key,AIM_PROTOCOL_NAME,strlen(AIM_PROTOCOL_NAME));
	memcpy(&idgroup_key[strlen(AIM_PROTOCOL_NAME)],AIM_MOD_IG,strlen(AIM_MOD_IG));
	ID_GROUP_KEY=(char*)realloc(ID_GROUP_KEY,strlen(idgroup_key)+1);
	memcpy(ID_GROUP_KEY,idgroup_key,strlen(idgroup_key));
	memcpy(&ID_GROUP_KEY[strlen(idgroup_key)],"\0",1);
	char filetransfer_key[32];//
	ZeroMemory(filetransfer_key,sizeof(filetransfer_key));
	memcpy(filetransfer_key,AIM_PROTOCOL_NAME,strlen(AIM_PROTOCOL_NAME));
	memcpy(&filetransfer_key[strlen(AIM_PROTOCOL_NAME)],AIM_KEY_FT,strlen(AIM_KEY_FT));
	FILE_TRANSFER_KEY=(char*)realloc(FILE_TRANSFER_KEY,strlen(filetransfer_key)+1);
	memcpy(FILE_TRANSFER_KEY,filetransfer_key,strlen(filetransfer_key));
	memcpy(&FILE_TRANSFER_KEY[strlen(filetransfer_key)],"\0",1);
	//end location of memory
	pluginLink = link;
	conn.status=ID_STATUS_OFFLINE;
	pd.cbSize = sizeof(pd);
    pd.szName = AIM_PROTOCOL_NAME;
    pd.type = PROTOTYPE_PROTOCOL;
    CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) & pd);
	InitializeCriticalSection(&modeMsgsMutex);
	InitializeCriticalSection(&statusMutex);
	InitializeCriticalSection(&connectionMutex);
	ForkThread(aim_keepalive_thread,NULL);
	CreateServices();
	return 0;
}
int ModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	DBVARIANT dbv,dbv2;
	NETLIBUSER nlu;
	ZeroMemory(&nlu, sizeof(nlu));
    nlu.cbSize = sizeof(nlu);
    nlu.flags = NUF_OUTGOING | NUF_HTTPCONNS;
    nlu.szSettingsModule = AIM_PROTOCOL_NAME;
    nlu.szDescriptiveName = "AOL Instant Messenger server connection";
    conn.hNetlib = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM) & nlu);
	char szP2P[128];
	mir_snprintf(szP2P, sizeof(szP2P), "%sP2P", AIM_PROTOCOL_NAME);
	nlu.flags = NUF_OUTGOING | NUF_INCOMING;
	nlu.szDescriptiveName = "AOL Instant Messenger Client-to-client connection";
	nlu.szSettingsModule = szP2P;
	nlu.minIncomingPorts = 1;
	conn.hNetlibPeer = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM) & nlu);
	if (DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, &dbv))
	{
		DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, AIM_DEFAULT_SERVER);
	}
	if (DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DG, &dbv2))
	{
		DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DG, AIM_DEFAULT_GROUP);
	}
	if(DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, -1)==-1)
		DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, DEFAULT_GRACE_PERIOD);
	if(DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA, -1)==-1)
		DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA, DEFAULT_KEEPALIVE_TIMER);
	if(dbv.pszVal)
		DBFreeVariant(&dbv);
	if(dbv2.pszVal)
		DBFreeVariant(&dbv2);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_OPT_INITIALISE, OptionsInit);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_USERINFO_INITIALISE, UserInfoInit);
	if(conn.hookEvent_size>HOOKEVENT_SIZE)
	{
		MessageBox( NULL, "AimOSCAR has detected that a buffer overrun has occured in it's 'conn.hookEvent' array. Please recompile with a larger HOOKEVENT_SIZE declared. AimOSCAR will now shut Miranda-IM down.", AIM_PROTOCOL_NAME, MB_OK );
		exit(1);
	}
	offline_contacts();
	return 0;
}
int PreBuildContactMenu(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM mi;
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	if(DBGetContactSettingWord((HANDLE)wParam,AIM_PROTOCOL_NAME,"Status",ID_STATUS_OFFLINE)==ID_STATUS_AWAY)
	{
		mi.flags=CMIM_FLAGS|CMIF_NOTOFFLINE;
	}
	else
	{
		mi.flags=CMIM_FLAGS|CMIF_NOTOFFLINE|CMIF_HIDDEN;
	}
	CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)conn.hHTMLAwayContextMenuItem,(LPARAM)&mi);
	return 0;
}
int PreShutdown(WPARAM wParam,LPARAM lParam)
{
	if(conn.hServerConn)
		Netlib_CloseHandle(conn.hServerConn);
	conn.hServerConn=0;
	if(conn.hDirectBoundPort)
		Netlib_CloseHandle(conn.hDirectBoundPort);
	conn.hDirectBoundPort=0;
	Netlib_CloseHandle(conn.hNetlib);
	conn.hNetlib=0;
	Netlib_CloseHandle(conn.hNetlibPeer);
	conn.hNetlibPeer=0;
	for(unsigned int i=0;i<conn.hookEvent_size;i++)
		UnhookEvent(conn.hookEvent[i]);
	free(CWD);
	free(AIM_PROTOCOL_NAME);
	free(conn.szModeMsg);
	free(COOKIE);
	free(GROUP_ID_KEY);
	free(ID_GROUP_KEY);
	free(FILE_TRANSFER_KEY);
	DeleteCriticalSection(&modeMsgsMutex);
	DeleteCriticalSection(&statusMutex);
	DeleteCriticalSection(&connectionMutex);
	conn.hTimer=0;
	return 0;
}
extern "C" int __declspec(dllexport) Unload(void)
{
	return 0;
}
