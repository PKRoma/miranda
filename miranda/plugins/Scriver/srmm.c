/*
Scriver

Copyright 2000-2009 Miranda ICQ/IM project,

all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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
#include "commonheaders.h"

int OnLoadModule(void);
int OnUnloadModule(void);

struct MM_INTERFACE mmi;
struct UTF8_INTERFACE utfi;

PLUGINLINK *pluginLink;
HINSTANCE g_hInst;

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
#ifdef _UNICODE
	"Scriver (Unicode)",
#else
	"Scriver",
#endif
	PLUGIN_MAKE_VERSION(2, 8, 0, 42),
	"Scriver - send and receive instant messages",
	"Miranda IM Development Team",
	"the_leech@users.berlios.de",
	"Copyright (c) 2000-2009 Miranda IM Project",
	"http://www.miranda-im.org",
	UNICODE_AWARE,
	DEFMOD_SRMESSAGE,            // replace internal version (if any)
#ifdef _UNICODE
    {0xae3005a8, 0x14d9, 0x4e8f, { 0x86, 0xe3, 0xc7, 0xf0, 0x5, 0xb6, 0xf2, 0x8b }} // {AE3005A8-14D9-4e8f-86E3-C7F005B6F28B}
#else
    {0xb22c7147, 0x5919, 0x49c9, { 0xa2, 0xa9, 0xfc, 0x82, 0x95, 0xaf, 0x48, 0xb4 }} // {B22C7147-5919-49c9-A2A9-FC8295AF48B4}
#endif
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	g_hInst = hinstDLL;
	return TRUE;
}

__declspec(dllexport)
	 PLUGININFOEX *MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 8, 0, 0))
		return NULL;
	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_SRMM, MIID_CHAT, MIID_LAST};
__declspec(dllexport)
     const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

int __declspec(dllexport) Load(PLUGINLINK * link)
{
	pluginLink = link;

	// set the memory manager
	mir_getMMI( &mmi );
	mir_getUTFI( &utfi );

	InitSendQueue();
	return OnLoadModule();
}

int __declspec(dllexport) Unload(void)
{
	DestroySendQueue();
	return OnUnloadModule();
}
