/*

Copyright 2005 Sam Kothari (egoDust)

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

#include "itr.h"
#include "oscar_svc.h"
#include "svc.h"
#include "miranda.h"

char g_Name[MAX_PATH];
HINSTANCE g_hInst=0;
PLUGINLINK * pluginLink; // don't rename
PLUGININFO OscarPluginInfo = {
	sizeof(PLUGININFO),
	"Oscar",
	PLUGIN_MAKE_VERSION(0,1,0,0),
	"OSCAR implementation to connect to AIM/ICQ via libfaim (thanks gaim) - alpha alpha alpha alpha alpha alpha alpha!",
	"egoDust",	
	"egodust@users.sf.net",
	"See AUTHORS file, libfaim authors, gaim, me",
	"<none>",
	0,
	0
};

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
	g_hInst=hInst;
	char buf[MAX_PATH];
	if ( GetModuleFileName(hInst, buf, sizeof(buf)) ) {
		char * p = strrchr(buf, '\\');
		if ( p && ++p ) {
			strcpy(g_Name, p);
			p = strchr(g_Name, '.');
			if ( p != NULL ) *p=0;
			CharUpper(g_Name);
		}
	}	
    return TRUE;
}

int __declspec(dllexport) Load(PLUGINLINK * link)
{
	PROTOCOLDESCRIPTOR pd;
	pluginLink=link;
	//WSADATA wsa;
	DisableThreadLibraryCalls(g_hInst);
	/* sigh. */
	//WSAStartup(MAKEWORD(1,1), &wsa);
	/* register the protocol */
	pd.cbSize = sizeof(PROTOCOLDESCRIPTOR);
	pd.type = PROTOTYPE_PROTOCOL;
	pd.szName = g_Name;
	CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);
	/* register options */
	LoadOptionsServices();
	/* setup any service callbacks */
	LoadServices();
	/* set all contacts to offline */
	Miranda_SetContactsOffline();
	return 0;
}

PLUGININFO * __declspec(dllexport) MirandaPluginInfo(DWORD MirandaVersion)
{
	return &OscarPluginInfo;
}

int __declspec(dllexport) Unload(void)
{
	UnloadServices();
	//WSACleanup();
	return 0;
}

