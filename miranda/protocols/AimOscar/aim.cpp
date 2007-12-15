#include "aim.h"
PLUGINLINK *pluginLink;
#define AIM_OSCAR_VERSION "\0\0\0\x07"
char* AIM_CLIENT_ID_STRING="Miranda Oscar Plugin, version 0.0.0.7";
char AIM_CAP_MIRANDA[]="MirandaA\0\0\0\0\0\0\0";
PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	"AIM OSCAR Plugin - Version 7(Avatar Test Build)",
	PLUGIN_MAKE_VERSION(0,0,0,7),
	"Provides basic support for AOL® OSCAR Instant Messenger protocol. [Built: "__DATE__" "__TIME__"]",
	"Aaron Myles Landwehr",
	"aaron@miranda-im.org",
	"© 2005-2006 Aaron Myles Landwehr",
	"http://www.snaphat.com/oscar",
	0,		//not transient
	0,		//doesn't replace anything built-in
    {0xb4ef58c4, 0x4458, 0x4e47, { 0xa7, 0x67, 0x5c, 0xae, 0xe5, 0xe7, 0xc, 0x81 }} //{B4EF58C4-4458-4e47-A767-5CAEE5E70C81}
};
oscar_data conn = {0};
file_transfer* fu;
MD5_INTERFACE  md5i;
extern "C" __declspec(dllexport) bool WINAPI DllMain(HINSTANCE hinstDLL,DWORD /*fdwReason*/,LPVOID /*lpvReserved*/)
{
	conn.hInstance = hinstDLL;
	return TRUE;
}
extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	unsigned long mv=_htonl(mirandaVersion);
	memcpy((char*)&AIM_CAP_MIRANDA[8],&mv,sizeof(DWORD));
	memcpy((char*)&AIM_CAP_MIRANDA[12],(char*)&AIM_OSCAR_VERSION,sizeof(DWORD));
	return &pluginInfo;
}
static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
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
	GetModuleFileName(conn.hInstance, str2, MAX_PATH);
    str1 = strrchr(str2, '\\');
	if (str1 != NULL && (lstrlen(str1 + 1) > 4))
	{
		char store[MAX_PATH];
		strlcpy(store, str1 + 1, lstrlen(str1 + 1) - 3);
		AIM_PROTOCOL_NAME=new char[lstrlen(store)+1];
		memcpy(AIM_PROTOCOL_NAME,store,lstrlen(store)+1);
	}
	CharUpper(AIM_PROTOCOL_NAME);
	GROUP_ID_KEY=strlcat(AIM_PROTOCOL_NAME,AIM_MOD_GI);
	ID_GROUP_KEY=strlcat(AIM_PROTOCOL_NAME,AIM_MOD_IG);
	FILE_TRANSFER_KEY=strlcat(AIM_PROTOCOL_NAME,AIM_KEY_FT);
	//create some events
	conn.hAvatarEvent=CreateEvent (NULL,false,false,NULL);//DO want it to autoreset after setevent()
	conn.hAwayMsgEvent=CreateEvent(NULL,false,false,NULL);
	//end location of memory
	pluginLink = link;
	mir_getMD5I( &md5i );
	conn.status=ID_STATUS_OFFLINE;
	pd.cbSize = sizeof(pd);
    pd.szName = AIM_PROTOCOL_NAME;
    pd.type = PROTOTYPE_PROTOCOL;
    CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) & pd);
	InitializeCriticalSection(&modeMsgsMutex);
	InitializeCriticalSection(&statusMutex);
	InitializeCriticalSection(&connectionMutex);
	InitializeCriticalSection(&SendingMutex);
	InitializeCriticalSection(&avatarMutex);

	InitIcons();

	if(DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FR, 0)==0)
		DialogBox(conn.hInstance, MAKEINTRESOURCE(IDD_AIMACCOUNT), NULL, first_run_dialog);
	if(DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA, 0))
		ForkThread(aim_keepalive_thread,NULL);
	CreateServices();
	return 0;
}
int ExtraIconsRebuild(WPARAM wParam, LPARAM lParam);
int ExtraIconsApply(WPARAM wParam, LPARAM lParam);
int ModulesLoaded(WPARAM /*wParam*/,LPARAM /*lParam*/)
{
	char store[MAX_PATH];
	CallService(MS_DB_GETPROFILEPATH, MAX_PATH-1,(LPARAM)&store);
	if(store[lstrlen(store)-1]=='\\')
		store[lstrlen(store)-1]='\0';
	CWD=(char*)new char[lstrlen(store)+1];
	memcpy(CWD,store,lstrlen(store));
	memcpy(&CWD[lstrlen(store)],"\0",1);
	DBVARIANT dbv;
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

	if (DBGetContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, &dbv))
		DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, AIM_DEFAULT_SERVER);
	else
		DBFreeVariant(&dbv);
	
	if(DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, -1)==-1)
		DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, DEFAULT_GRACE_PERIOD);

	if(DBGetContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, &dbv))
	{
		if (!DBGetContactSettingString(NULL, AIM_PROTOCOL_NAME, OLD_KEY_PW, &dbv))
		{
			DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, dbv.pszVal);
			DBDeleteContactSetting(NULL, AIM_PROTOCOL_NAME, OLD_KEY_PW);
			DBFreeVariant(&dbv);
		}
	}
	else
	{
		DBFreeVariant(&dbv);
	}

	if(DBGetContactSettingByte(NULL,AIM_PROTOCOL_NAME,AIM_KEY_DM,-1)==-1)
	{
		int i=DBGetContactSettingByte(NULL,AIM_PROTOCOL_NAME,OLD_KEY_DM,-1);
		if(i!=-1)
		{
			if(i==1)
				DBWriteContactSettingByte(NULL,AIM_PROTOCOL_NAME,AIM_KEY_DM,0);
			else
				DBWriteContactSettingByte(NULL,AIM_PROTOCOL_NAME,AIM_KEY_DM,1);
			DBDeleteContactSetting(NULL, AIM_PROTOCOL_NAME, OLD_KEY_DM);
		}
	}
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_OPT_INITIALISE, OptionsInit);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_USERINFO_INITIALISE, UserInfoInit);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_IDLE_CHANGED,IdleChanged);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_CLIST_EXTRA_LIST_REBUILD, ExtraIconsRebuild);
	conn.hookEvent[conn.hookEvent_size++]=HookEvent(ME_CLIST_EXTRA_IMAGE_APPLY, ExtraIconsApply);
	if(conn.hookEvent_size>HOOKEVENT_SIZE)
	{
		MessageBox( NULL, "AimOSCAR has detected that a buffer overrun has occured in it's 'conn.hookEvent' array. Please recompile with a larger HOOKEVENT_SIZE declared. AimOSCAR will now shut Miranda-IM down.", AIM_PROTOCOL_NAME, MB_OK );
		exit(1);
	}
	offline_contacts();
	aim_links_init();
	return 0;
}
int PreBuildContactMenu(WPARAM wParam,LPARAM /*lParam*/)
{
	CLISTMENUITEM mi;
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	//see if we should add the html away message context menu items
	if(DBGetContactSettingWord((HANDLE)wParam,AIM_PROTOCOL_NAME,"Status",ID_STATUS_OFFLINE)==ID_STATUS_AWAY)
	{
		mi.flags=CMIM_FLAGS|CMIF_NOTOFFLINE;
	}
	else
	{
		mi.flags=CMIM_FLAGS|CMIF_NOTOFFLINE|CMIF_HIDDEN;
	}
	CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)conn.hHTMLAwayContextMenuItem,(LPARAM)&mi);
	char* item= new char[lstrlen(AIM_KEY_BI)+10];
	mir_snprintf(item,lstrlen(AIM_KEY_BI)+10,AIM_KEY_BI"%d",1);
	if(!DBGetContactSettingWord((HANDLE)wParam, AIM_PROTOCOL_NAME, item,0)&&conn.state==1)
	{
		mi.flags=CMIM_FLAGS|CMIF_NOTONLINE;
	}
	else
	{
		mi.flags=CMIM_FLAGS|CMIF_NOTONLINE|CMIF_HIDDEN;
	}
	delete[] item;
	CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)conn.hAddToServerListContextMenuItem,(LPARAM)&mi);
	return 0;
}
int PreShutdown(WPARAM /*wParam*/,LPARAM /*lParam*/)
{
	conn.shutting_down=1;
	if(conn.hDirectBoundPort)
	{
		conn.freeing_DirectBoundPort=1;
		SOCKET s = CallService(MS_NETLIB_GETSOCKET, LPARAM(conn.hDirectBoundPort), 0);
		if (s != INVALID_SOCKET) shutdown(s, 2);
	}
	if(conn.hServerPacketRecver)
	{
		SOCKET s = CallService(MS_NETLIB_GETSOCKET, LPARAM(conn.hServerPacketRecver), 0);
		if (s != INVALID_SOCKET) shutdown(s, 2);
	}
	if(conn.hServerConn)
	{
		SOCKET s = CallService(MS_NETLIB_GETSOCKET, LPARAM(conn.hServerConn), 0);
		if (s != INVALID_SOCKET) shutdown(s, 2);
	}
	if(conn.hMailConn)
	{
		SOCKET s = CallService(MS_NETLIB_GETSOCKET, LPARAM(conn.hMailConn), 0);
		if (s != INVALID_SOCKET) shutdown(s, 2);
	}
	return 0;
}
extern "C" int __declspec(dllexport) Unload(void)
{
	if(conn.hDirectBoundPort)
		Netlib_CloseHandle(conn.hDirectBoundPort);
	if(conn.hServerConn)
		Netlib_CloseHandle(conn.hServerConn);
	conn.hServerConn=0;

	Netlib_CloseHandle(conn.hNetlib);
	conn.hNetlib=0;
	Netlib_CloseHandle(conn.hNetlibPeer);
	conn.hNetlibPeer=0;

	for(unsigned int i=0;i<conn.hookEvent_size;i++)
		UnhookEvent(conn.hookEvent[i]);
	for(unsigned int j=0;j<conn.services_size;j++)
		DestroyServiceFunction(conn.services[j]);
	DeleteCriticalSection(&modeMsgsMutex);
	DeleteCriticalSection(&statusMutex);
	DeleteCriticalSection(&connectionMutex);
	DeleteCriticalSection(&SendingMutex);
	DeleteCriticalSection(&avatarMutex);

	aim_links_destroy();
	delete[] CWD;
	delete[] conn.szModeMsg;
	delete[] COOKIE;
	delete[] MAIL_COOKIE;
	delete[] AVATAR_COOKIE;
	delete[] GROUP_ID_KEY;
	delete[] ID_GROUP_KEY;
	delete[] FILE_TRANSFER_KEY;
	delete[] AIM_PROTOCOL_NAME;
	return 0;
}
