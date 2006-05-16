/*
Miranda plugin template, originally by Richard Hughes
http://miranda-icq.sourceforge.net/

This file is placed in the public domain. Anybody is free to use or
modify it as they wish with no restriction.
There is no warranty.
*/
#include "natcommon.h"

HINSTANCE hInst;
PLUGINLINK *pluginLink;
HANDLE hNetlib=NULL;
HANDLE hEventUpdatedIP=NULL;

PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"NATted",
	PLUGIN_MAKE_VERSION(0,5,0,0),
	"This plugin will find out what the external IP is and then allow protocols to use this information",
	"egoDust",
	"egodust@users.sourceforge.net",
	"",
	"http://miranda-icq.sourceforge.net/",
	0,
	0
};

static void doHTTPquery(void)
{
	NETLIBHTTPREQUEST nlr = {0}, *reply = NULL;
	nlr.cbSize=sizeof(nlr);
	nlr.requestType=REQUEST_GET;
	nlr.flags=NLHRF_GENERATEHOST;
	nlr.szUrl=NAT_SCRIPT_URL;

	reply = (NETLIBHTTPREQUEST*) CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlib,(LPARAM)&nlr);
	if (reply) {

		int j;

		for (j=0; j<reply->headersCount; j++) { /* we got some headers, get IP header? */
			if (strcmp(reply->headers[j].szName,"IP")==0) {
				/* notify hook! */
				NETLIBIPINFO ip = {0};

				ip.cbSize=sizeof(ip);
				ip.szIP=reply->headers[j].szValue;
				ip.szSite=NAT_SCRIPT_URL;

				NotifyEventHooks(hEventUpdatedIP,0,(LPARAM)&ip);

				break;
			} //if
		} //for

		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)reply);
	} else { /* failed, damn! */
	} //if

	return;
}

static void queryThread(void * arg)
{
	for (;;) {
		if (Miranda_Terminated()) return;
		doHTTPquery();
		SleepEx(1000 * (60 * 10), TRUE);  /* goto sleep and wake up when APC'd */
	} //for
}

static int systemModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	NETLIBUSER nu={0};
	nu.cbSize=sizeof(nu);
	nu.szSettingsModule="NATted";
	nu.szDescriptiveName=Translate("NAT helper service main connection");
	nu.flags=NUF_OUTGOING | NUF_HTTPCONNS;
	hNetlib=(HANDLE)CallService(MS_NETLIB_REGISTERUSER,0,(LPARAM)&nu);

	forkthread(queryThread,0,NULL);

	return 0;
}

static int doHostCheck(WPARAM wParam, LPARAM lParam)
{
	forkthread(queryThread,0,NULL);
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	return &pluginInfo;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink=link;
	HookEvent(ME_SYSTEM_MODULESLOADED, systemModulesLoaded);
	hEventUpdatedIP=CreateHookableEvent(ME_NETLIB_EXTERNAL_IP_FETCHED);
	CreateServiceFunction(MS_NETLIB_EXTERNAL_IP_FORCECHECK, doHostCheck);
	return 0;
}

int __declspec(dllexport) Unload(void)
{
	return 0;
}