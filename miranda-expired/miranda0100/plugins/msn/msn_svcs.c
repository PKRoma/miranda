/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include "msn_global.h"
//#include "resource.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

extern SOCKET msnSock;
extern int msnLoggedIn;

static int MsnGetCaps(WPARAM wParam,LPARAM lParam)
{
	if(wParam==PFLAGNUM_1)
		return PF1_IM|PF1_BASICSEARCH|PF1_ADDSEARCHRES|PF1_SERVERCLIST;
	if(wParam==PFLAGNUM_2)
		return PF2_ONLINE|PF2_SHORTAWAY|PF2_LONGAWAY|PF2_LIGHTDND|PF2_ONTHEPHONE|PF2_OUTTOLUNCH;
	if(wParam==PFLAGNUM_3)
		return 0;
	return 0;
}

static int MsnGetName(WPARAM wParam,LPARAM lParam)
{
	lstrcpyn((char*)lParam,"MSN",wParam);
	return 0;
}

static int MsnLoadIcon(WPARAM wParam,LPARAM lParam)
{
	//this function returns completely wrong icons. Major icon fixage required
	return 0;
}

int MsnSetStatus(WPARAM wParam,LPARAM lParam)
{
	if (wParam==ID_STATUS_OFFLINE)
	{
			MSN_Logout();
	}
	else
	{
		if (!msnSock)
		{
			if (MSN_Login("messenger.hotmail.com",1863)==FALSE)
			{ //failed
				msnSock=0;
			}
		}
		else
		{//change status
			char *szMode;
			int newMode;
			newMode=wParam;
			switch(wParam) {
				case ID_STATUS_NA: szMode="AWY"; break;
				case ID_STATUS_AWAY: szMode="BRB"; break;
				case ID_STATUS_DND:	newMode=ID_STATUS_OCCUPIED;
				case ID_STATUS_OCCUPIED: szMode="BSY"; break;
				case ID_STATUS_ONTHEPHONE: szMode="PHN"; break;
				case ID_STATUS_OUTTOLUNCH: szMode="LUN"; break;
				//"IDL"=idle ???
				default: szMode="NLN"; newMode=ID_STATUS_ONLINE; break;
			}
			if (msnSock && msnLoggedIn) {
				MSN_SendPacket(msnSock,"CHG",szMode);
			}
		}
		
	}
	return 0;
}

int MsnGetStatus(WPARAM wParam,LPARAM lParam)
{
	return ID_STATUS_OFFLINE; //msnStatusMode;
}

static int MsnBasicSearch(WPARAM wParam,LPARAM lParam)
{
	return 1;
}

static HANDLE AddToListByUin(DWORD uin,DWORD flags)
{
	/*HANDLE hContact;

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact!=NULL) {
		if(!strcmp((char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0),MSNPROTONAME)) {
			if(DBGetContactSettingDword(hContact,ICQPROTONAME,"UIN",0)==uin) {
				if((!flags&PALF_TEMPORARY)) {
					DBDeleteContactSetting(hContact,"CList","NotOnList");
					DBDeleteContactSetting(hContact,"CList","Hidden");
				}
				return hContact;
			}
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}
	hContact=(HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
	CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)hContact,(LPARAM)ICQPROTONAME);
	DBWriteContactSettingDword(hContact,ICQPROTONAME,"UIN",uin);
	if(flags&PALF_TEMPORARY) {
		DBWriteContactSettingByte(hContact,"CList","NotOnList",1);
		DBWriteContactSettingByte(hContact,"CList","Hidden",1);
	}
	return hContact;*/
	return 0;
}

static int MsnAddToList(WPARAM wParam,LPARAM lParam)
{
	PROTOSEARCHRESULT *psr=(PROTOSEARCHRESULT*)lParam;

	if(psr->cbSize!=sizeof(PROTOSEARCHRESULT)) return (int)(HANDLE)NULL;
	return 0;
}

static int MsnGetInfo(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs=(CCSDATA*)lParam;
	return 0;
}

static int MsnSendMessage(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs=(CCSDATA*)lParam;

	return 0;//seq;
}

int LoadMsnServices(void)
{
	CreateServiceFunction(MSNPROTONAME PS_GETCAPS,MsnGetCaps);
	CreateServiceFunction(MSNPROTONAME PS_GETNAME,MsnGetName);
	CreateServiceFunction(MSNPROTONAME PS_LOADICON,MsnLoadIcon);
	CreateServiceFunction(MSNPROTONAME PS_SETSTATUS,MsnSetStatus);
	CreateServiceFunction(MSNPROTONAME PS_GETSTATUS,MsnGetStatus);
	CreateServiceFunction(MSNPROTONAME PS_BASICSEARCH,MsnBasicSearch);
	CreateServiceFunction(MSNPROTONAME PS_ADDTOLIST,MsnAddToList);
	CreateServiceFunction(MSNPROTONAME PSS_GETINFO,MsnGetInfo);
	CreateServiceFunction(MSNPROTONAME PSS_MESSAGE,MsnSendMessage);
	return 0;
}