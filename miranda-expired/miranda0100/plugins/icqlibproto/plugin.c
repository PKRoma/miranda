/* 
 * ICQ OSCAR Protocol Plugin for Miranda
 * Copyright (C) 2001 Jon Keating <jon@licq.org>
 *
 * All distributed forms of this file must retain this notice.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * If you do find this useful, please contact me if you wish to
 * send me girls, food, or hardware.  And yes, that is in the
 * preferred order.
 */

#include <windows.h>
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/ui/contactlist/m_clist.h"
#include "../../miranda32/random/skin/m_skin.h"
#include "../../miranda32/ui/options/m_options.h"

int LoadIcqlibModule(void);

HINSTANCE hInst;
PLUGINLINK *pluginLink;

PLUGININFO pluginInfo =
{
	sizeof(PLUGININFO),
	"ICQ v5 protocol",
	PLUGIN_MAKE_VERSION(0, 1, 1, 0),
	"The protocol used to communicate with the old ICQ servers.",
	"Richard Hughes",
	"cyreve@users.sourceforge.net",
	"© 2001 Richard Hughes",
	"http://miranda-icq.sourceforge.net/",
	0,
	DEFMOD_PROTOCOLICQ
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;

	return TRUE;
}

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if(mirandaVersion<PLUGIN_MAKE_VERSION(0,1,1,0)) return NULL;
	return &pluginInfo;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;
	LoadIcqlibModule();

	return 0;
}

int __declspec(dllexport) Unload(void)
{
	return 0;
}
