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
	"AIM OSCAR Plugin - Alpha 8.1(7.2 for Goons)",
	PLUGIN_MAKE_VERSION(0,0,0,1),
	"Provides very basic support for AOL® OSCAR Instant Messenger protocol.",
	"Aaron Myles Landwehr",
	"aaron@snaphat.com",
	"© 2006 Aaron Myles Landwehr",
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
	conn.hTimer = SetTimer(NULL, 0, KEEPALIVE_TIMER, aim_keepalive_connection);
	CreateServices();
	return 0;
}
extern "C" int __declspec(dllexport) Unload(void)
{
	return 0;
}
