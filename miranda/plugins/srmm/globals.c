/*
SRMM

Copyright 2000-2005 Miranda ICQ/IM project, 
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

extern HINSTANCE g_hInst;

struct GlobalMessageData *g_dat;
static HANDLE g_hDbEvent, g_hAck, g_hIconsChanged;

static int dbaddedevent(WPARAM wParam, LPARAM lParam);
static int ackevent(WPARAM wParam, LPARAM lParam);

typedef struct IconDefStruct 
{
	const char *szName;
	const char *szDescr;
	int defIconID;
} IconList;

static const IconList iconList[] = 
{
	{ "INCOMING", LPGEN("Incoming message (10x10)"), IDI_INCOMING },
	{ "OUTGOING", LPGEN("Outgoing message (10x10)"), IDI_OUTGOING },
	{ "NOTICE",   LPGEN("Notice (10x10)"),           IDI_NOTICE   },
};


HANDLE hIconLibItem[SIZEOF(iconList)];

static void InitIcons(void)
{
	TCHAR szFile[MAX_PATH];
	char szSettingName[100];
	SKINICONDESC sid = {0};
	unsigned i;

	GetModuleFileName(g_hInst, szFile, SIZEOF(szFile));

	sid.cbSize = sizeof(SKINICONDESC);
	sid.ptszDefaultFile = szFile;
	sid.pszName = szSettingName;
	sid.pszSection = LPGEN("Messaging");
	sid.flags = SIDF_PATH_TCHAR;
	sid.cx = 10; sid.cy = 10;

	for (i = 0; i < SIZEOF(iconList); i++) 
	{
		mir_snprintf(szSettingName, sizeof(szSettingName), "SRMM_%s", iconList[i].szName);
		sid.pszDescription = (char*)iconList[i].szDescr;
		sid.iDefaultIndex = -iconList[i].defIconID;
		hIconLibItem[i] = (HANDLE)CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
	}	
}

static int IconsChanged(WPARAM wParam, LPARAM lParam)
{
	FreeMsgLogIcons();
	LoadMsgLogIcons();
	
	return 0;
}

void InitGlobals()
{
	g_dat = (struct GlobalMessageData *)mir_alloc(sizeof(struct GlobalMessageData));
	g_dat->hMessageWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	g_hDbEvent = HookEvent(ME_DB_EVENT_ADDED, dbaddedevent);
	g_hAck = HookEvent(ME_PROTO_ACK, ackevent);
	g_hIconsChanged = HookEvent(ME_SKIN2_ICONSCHANGED, IconsChanged);

	ReloadGlobals();
	InitIcons();
}

void FreeGlobals()
{
	mir_free(g_dat);

	if (g_hDbEvent) UnhookEvent(g_hDbEvent);
	if (g_hAck) UnhookEvent(g_hAck);
	if (g_hIconsChanged) UnhookEvent(g_hIconsChanged);
}

void ReloadGlobals()
{
	g_dat->flags = 0;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWINFOLINE, SRMSGDEFSET_SHOWINFOLINE))
		g_dat->flags |= SMF_SHOWINFO;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE))
		g_dat->flags |= SMF_SHOWBTNS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDBUTTON, SRMSGDEFSET_SENDBUTTON))
		g_dat->flags |= SMF_SENDBTN;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPING, SRMSGDEFSET_SHOWTYPING))
		g_dat->flags |= SMF_SHOWTYPING;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN))
		g_dat->flags |= SMF_SHOWTYPINGWIN;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGNOWIN, SRMSGDEFSET_SHOWTYPINGNOWIN))
		g_dat->flags |= SMF_SHOWTYPINGTRAY;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST))
		g_dat->flags |= SMF_SHOWTYPINGCLIST;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWLOGICONS, SRMSGDEFSET_SHOWLOGICONS))
		g_dat->flags |= SMF_SHOWICONS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTIME, SRMSGDEFSET_SHOWTIME))
		g_dat->flags |= SMF_SHOWTIME;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AVATARENABLE, SRMSGDEFSET_AVATARENABLE))
		g_dat->flags |= SMF_AVATAR;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWDATE, SRMSGDEFSET_SHOWDATE))
		g_dat->flags |= SMF_SHOWDATE;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSECS, SRMSGDEFSET_SHOWSECS))
		g_dat->flags |= SMF_SHOWSECS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDENAMES, SRMSGDEFSET_HIDENAMES))
		g_dat->flags |= SMF_HIDENAMES;
	g_dat->openFlags = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_POPFLAGS, SRMSGDEFSET_POPFLAGS);
}

static int dbaddedevent(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	if (hContact) 
	{
		HWND h = WindowList_Find(g_dat->hMessageWindowList, hContact);
		if (h) SendMessage(h, HM_DBEVENTADDED, (WPARAM)hContact, lParam);
	}
	return 0;
}

static int ackevent(WPARAM wParam, LPARAM lParam)
{
	ACKDATA *pAck = (ACKDATA *)lParam;
	
	if (!pAck) return 0;
	else if (pAck->type==ACKTYPE_AVATAR) {
		HWND h = WindowList_Find(g_dat->hMessageWindowList, (HANDLE)pAck->hContact);
		if(h) SendMessage(h, HM_AVATARACK, wParam, lParam);
	}
	else if (pAck->type==ACKTYPE_MESSAGE) {
		HWND h = WindowList_Find(g_dat->hMessageWindowList, (HANDLE)pAck->hContact);
		if(h) SendMessage(h, HM_EVENTSENT, wParam, lParam);
	}
	return 0;
}
