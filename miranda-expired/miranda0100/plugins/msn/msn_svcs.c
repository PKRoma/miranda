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
#include <process.h>
//#include "resource.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

void __cdecl MSNServerThread(struct ThreadData *info);

extern SOCKET msnNSSocket;
extern volatile LONG msnLoggedIn;
extern int msnStatusMode,msnDesiredStatus;

static int MsnGetCaps(WPARAM wParam,LPARAM lParam)
{
	if(wParam==PFLAGNUM_1)
		return PF1_IM|PF1_SERVERCLIST;
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
	msnDesiredStatus=wParam;
	MSN_DebugLog(MSN_LOG_DEBUG,"PS_SETSTATUS(%d,0)",wParam);
	if(msnDesiredStatus==ID_STATUS_OFFLINE) {
		if(msnLoggedIn) MSN_SendPacket(msnNSSocket,"OUT",NULL);
	}
	else {
		if(!msnLoggedIn && !(msnStatusMode>=ID_STATUS_CONNECTING && msnStatusMode<ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)) {
			struct ThreadData *newThread;
			newThread=(struct ThreadData*)malloc(sizeof(struct ThreadData));
			strcpy(newThread->server,"messenger.hotmail.com");	//NB: msmsgs 3.60 hardcodes 64.4.13.33
			newThread->type=SERVER_DISPATCH;
			ProtoBroadcastAck(MSNPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)msnStatusMode,ID_STATUS_CONNECTING);
			msnStatusMode=ID_STATUS_CONNECTING;
			_beginthread(MSNServerThread,0,newThread);
		}
		else {   //change status
			if(msnLoggedIn)
				MSN_SendPacket(msnNSSocket,"CHG",MirandaStatusToMSN(msnDesiredStatus));
		}
		
	}
	return 0;
}

int MsnGetStatus(WPARAM wParam,LPARAM lParam)
{
	return msnStatusMode;
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
	SOCKET s;
	int seq;

	s=Switchboards_SocketFromHContact(ccs->hContact);
	if(s==SOCKET_ERROR) {
		MSN_SendPacket(msnNSSocket,"XFR","SB");
		return MsgQueue_Add(ccs->hContact,(char*)ccs->lParam,ccs->wParam);
	}
	seq=MsgQueue_AllocateUniqueSeq();
	//TODO? UTF-8 encoding
	MSN_SendPacket(s,"MSG","U %d\r\nContent-Type: text/plain\r\n\r\n%s",strlen((char*)ccs->lParam)+28,(char*)ccs->lParam);
	CmdQueue_AddProtoAck(ccs->hContact,ACKTYPE_MESSAGE,ACKRESULT_SUCCESS,(HANDLE)seq,0);  //need a delay so caller gets seq before ack
	return seq;
}

static int MsnRecvMessage(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei;
	CCSDATA *ccs=(CCSDATA*)lParam;
	PROTORECVEVENT *pre=(PROTORECVEVENT*)ccs->lParam;

	DBDeleteContactSetting(ccs->hContact,"CList","Hidden");
	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule=MSNPROTONAME;
	dbei.timestamp=pre->timestamp;
	dbei.flags=pre->flags&PREF_CREATEREAD?DBEF_READ:0;
	dbei.eventType=EVENTTYPE_MESSAGE;
	dbei.cbBlob=strlen(pre->szMessage)+1;
	dbei.pBlob=(PBYTE)pre->szMessage;
	CallService(MS_DB_EVENT_ADD,(WPARAM)ccs->hContact,(LPARAM)&dbei);
	return 0;
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
	CreateServiceFunction(MSNPROTONAME PSR_MESSAGE,MsnRecvMessage);
	return 0;
}