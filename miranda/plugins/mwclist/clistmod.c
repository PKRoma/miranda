/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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

extern int DefaultImageListColorDepth;

int AddMainMenuItem(WPARAM wParam,LPARAM lParam);
int AddContactMenuItem(WPARAM wParam,LPARAM lParam);
int InitCustomMenus(void);
void UninitCustomMenus(void);
int MenuProcessCommand(WPARAM wParam,LPARAM lParam);
int ContactSettingChanged(WPARAM wParam,LPARAM lParam);
int CListOptInit(WPARAM wParam,LPARAM lParam);
int ContactChangeGroup(WPARAM wParam,LPARAM lParam);
int HotkeysProcessMessage(WPARAM wParam,LPARAM lParam);
void InitTrayMenus(void);

HIMAGELIST hCListImages;

HANDLE hStatusModeChangeEvent,hContactIconChangedEvent;
extern BYTE nameOrder[];
extern int currentDesiredStatusMode;

static HANDLE hSettingChanged, hProtoAckHook;

////////// By FYR/////////////
int ExtIconFromStatusMode(HANDLE hContact, const char *szProto,int status)
{
	if ( DBGetContactSettingByte( NULL, "CLC", "Meta", 0 ) == 1 )
		return pcli->pfnIconFromStatusMode(szProto,status,hContact);

	if ( szProto != NULL )
		if (strcmp(szProto,"MetaContacts") == 0 ) {
			hContact=(HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT,(UINT)hContact,0);
			if ( hContact != 0 ) {
				szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(UINT)hContact,0);
				status=DBGetContactSettingWord(hContact,szProto,"Status",ID_STATUS_OFFLINE);
			}
		}

	return pcli->pfnIconFromStatusMode(szProto,status,hContact);
}

/////////// End by FYR ////////

static int ProtocolAck(WPARAM wParam,LPARAM lParam)
{
	ACKDATA *ack=(ACKDATA*)lParam;
	if (ack->type==ACKTYPE_AWAYMSG && ack->lParam) {
		DBVARIANT dbv;
		if (!DBGetContactSetting(ack->hContact, "CList", "StatusMsg", &dbv)) {
			if (!strcmp(dbv.pszVal, (char *)ack->lParam)) {
				DBFreeVariant(&dbv);
				return 0;
			}
			DBFreeVariant(&dbv);
		}
		if ( DBGetContactSettingByte(NULL,"CList","ShowStatusMsg",0) || DBGetContactSettingByte(ack->hContact,"CList","StatusMsgAuto",0))
         DBWriteContactSettingString(ack->hContact, "CList", "StatusMsg", (char *)ack->lParam);
	}

	return 0;
}

static int SetStatusMode(WPARAM wParam, LPARAM lParam)
{
	//todo: check wParam is valid so people can't use this to run random menu items
	MenuProcessCommand(MAKEWPARAM(LOWORD(wParam), MPCF_MAINMENU), 0);
	return 0;
}

static int GetStatusMode(WPARAM wParam, LPARAM lParam)
{
	return currentDesiredStatusMode;
}

static int ContactListShutdownProc(WPARAM wParam,LPARAM lParam)
{
	UnhookEvent(hProtoAckHook);
	UninitCustomMenus();
	return 0;
}

int LoadContactListModule(void)
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact!=NULL) {
		DBWriteContactSettingString(hContact, "CList", "StatusMsg", "");
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	hCListImages = (HIMAGELIST)CallService(MS_CLIST_GETICONSIMAGELIST, 0, 0);
	DefaultImageListColorDepth=DBGetContactSettingDword(NULL,"CList","DefaultImageListColorDepth",ILC_COLOR32);
	
	hProtoAckHook = (HANDLE) HookEvent(ME_PROTO_ACK, ProtocolAck);
	HookEvent(ME_OPT_INITIALISE,CListOptInit);
	HookEvent(ME_SYSTEM_SHUTDOWN,ContactListShutdownProc);
	hSettingChanged=HookEvent(ME_DB_CONTACT_SETTINGCHANGED,ContactSettingChanged);
	hStatusModeChangeEvent=CreateHookableEvent(ME_CLIST_STATUSMODECHANGE);
	hContactIconChangedEvent=CreateHookableEvent(ME_CLIST_CONTACTICONCHANGED);
	CreateServiceFunction(MS_CLIST_CONTACTCHANGEGROUP,ContactChangeGroup);
	CreateServiceFunction(MS_CLIST_HOTKEYSPROCESSMESSAGE,HotkeysProcessMessage);
	CreateServiceFunction(MS_CLIST_SETSTATUSMODE, SetStatusMode);
	CreateServiceFunction(MS_CLIST_GETSTATUSMODE, GetStatusMode);

	InitCustomMenus();
	InitTrayMenus();
	return 0;
}
