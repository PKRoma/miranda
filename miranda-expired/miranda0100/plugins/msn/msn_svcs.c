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
#include "resource.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

void __cdecl MSNServerThread(struct ThreadData *info);

extern HINSTANCE hInst;
extern SOCKET msnNSSocket;
extern volatile LONG msnLoggedIn;
extern int msnStatusMode,msnDesiredStatus;

static int MsnGetCaps(WPARAM wParam,LPARAM lParam)
{
	if(wParam==PFLAGNUM_1)
		return PF1_IM|PF1_SERVERCLIST|PF1_AUTHREQ|PF1_BASICSEARCH|PF1_ADDSEARCHRES;
	if(wParam==PFLAGNUM_2)
		return PF2_ONLINE|PF2_SHORTAWAY|PF2_LONGAWAY|PF2_LIGHTDND|PF2_ONTHEPHONE|PF2_OUTTOLUNCH|PF2_INVISIBLE;
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
	UINT id;

	switch(wParam&0xFFFF) {
		case PLI_PROTOCOL: id=IDI_MSN; break;
		default: return (int)(HICON)NULL;	
	}
	return (int)LoadImage(hInst,MAKEINTRESOURCE(id),IMAGE_ICON,GetSystemMetrics(wParam&PLIF_SMALL?SM_CXSMICON:SM_CXICON),GetSystemMetrics(wParam&PLIF_SMALL?SM_CYSMICON:SM_CYICON),0);
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
			DBVARIANT dbv;
			newThread=(struct ThreadData*)malloc(sizeof(struct ThreadData));
			if(DBGetContactSetting(NULL,MSNPROTONAME,"LoginServer",&dbv))
				strcpy(newThread->server,MSN_DEFAULT_LOGIN_SERVER);
			else {
				lstrcpyn(newThread->server,dbv.pszVal,sizeof(newThread->server));
				DBFreeVariant(&dbv);
			}
			newThread->type=SERVER_DISPATCH;
			{	int oldMode=msnStatusMode;
				msnStatusMode=ID_STATUS_CONNECTING;
				ProtoBroadcastAck(MSNPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)oldMode,msnStatusMode);
			}
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

static int searchId=-1;
static char searchEmail[130];
static VOID CALLBACK BasicSearchTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	PROTOSEARCHRESULT psr;

	KillTimer(hwnd,idEvent);
	memset(&psr,0,sizeof(psr));
	psr.cbSize=sizeof(psr);
	psr.nick="";
	psr.firstName="";
	psr.lastName="";
	psr.email=searchEmail;
	ProtoBroadcastAck(MSNPROTONAME,NULL,ACKTYPE_SEARCH,ACKRESULT_DATA,(HANDLE)searchId,(LPARAM)&psr);
	ProtoBroadcastAck(MSNPROTONAME,NULL,ACKTYPE_SEARCH,ACKRESULT_SUCCESS,(HANDLE)searchId,0);
	searchId=-1;
}

static int MsnBasicSearch(WPARAM wParam,LPARAM lParam)
{
	if(searchId!=-1) return 0;   //only one search at a time
	searchId=1;
	lstrcpyn(searchEmail,(char*)lParam,sizeof(searchEmail));
	SetTimer(NULL,0,10,BasicSearchTimerProc);  //the caller needs to get this return value before the results
	return searchId;
}

static HANDLE AddToListByEmail(const char *email,DWORD flags)
{
	HANDLE hContact;
	char *szProto;
	DBVARIANT dbv;
	char urlEmail[130];

	UrlEncode(email,urlEmail,sizeof(urlEmail));
	//check not already on list
	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact!=NULL) {
		szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
		if(szProto!=NULL && !strcmp(szProto,MSNPROTONAME)) {
			if(!DBGetContactSetting(hContact,MSNPROTONAME,"e-mail",&dbv)) {
				if(!strcmp(dbv.pszVal,email)) {
					if((!flags&PALF_TEMPORARY) && DBGetContactSettingByte(hContact,"CList","NotOnList",1)) {
						DBDeleteContactSetting(hContact,"CList","NotOnList");
						DBDeleteContactSetting(hContact,"CList","Hidden");
						if(msnLoggedIn) MSN_SendPacket(msnNSSocket,"ADD","FL %s %s",urlEmail,urlEmail);
						else hContact=NULL;
					}
					DBFreeVariant(&dbv);
					return hContact;
				}
				DBFreeVariant(&dbv);
			}
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}
	//not already there: add
	hContact=(HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
	CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)hContact,(LPARAM)MSNPROTONAME);
	DBWriteContactSettingString(hContact,MSNPROTONAME,"e-mail",email);
	if(flags&PALF_TEMPORARY) {
		DBWriteContactSettingByte(hContact,"CList","NotOnList",1);
		DBWriteContactSettingByte(hContact,"CList","Hidden",1);
	}
	else {
		if(msnLoggedIn) MSN_SendPacket(msnNSSocket,"ADD","FL %s %s",urlEmail,urlEmail);
		else hContact=NULL;
	}
	return hContact;
}

static int MsnAddToList(WPARAM wParam,LPARAM lParam)
{
	PROTOSEARCHRESULT *psr=(PROTOSEARCHRESULT*)lParam;

	if(psr->cbSize!=sizeof(PROTOSEARCHRESULT)) return (int)(HANDLE)NULL;
	return (int)AddToListByEmail(psr->email,wParam);
}

static int MsnAuthAllow(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei;
	char *nick,*firstName,*lastName,*email;
	char urlNick[130],urlEmail[130];

	if(!msnLoggedIn) return 1;
	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	if((dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,wParam,0))==(DWORD)(-1)) return 1;
	dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
	if(CallService(MS_DB_EVENT_GET,wParam,(LPARAM)&dbei)) {free(dbei.pBlob); return 1;}
	if(dbei.eventType!=EVENTTYPE_AUTHREQUEST) {free(dbei.pBlob); return 1;}
	if(strcmp(dbei.szModule,MSNPROTONAME)) {free(dbei.pBlob); return 1;}
	nick=(char*)(dbei.pBlob+sizeof(DWORD));
	firstName=nick+strlen(nick)+1;
	lastName=firstName+strlen(firstName)+1;
	email=lastName+strlen(lastName)+1;
	UrlEncode(nick,urlNick,sizeof(urlNick));
	UrlEncode(email,urlEmail,sizeof(urlEmail));
	free(dbei.pBlob);
	MSN_SendPacket(msnNSSocket,"ADD","AL %s %s",urlEmail,urlNick);
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
	char *msg;

	msg=(char*)malloc(strlen((char*)ccs->lParam)*2+1);
	Utf8Encode((char*)ccs->lParam,msg,strlen((char*)ccs->lParam)*2+1);
	s=Switchboards_SocketFromHContact(ccs->hContact);
	if(s==SOCKET_ERROR) {
		MSN_SendPacket(msnNSSocket,"XFR","SB");
		return MsgQueue_Add(ccs->hContact,msg,ccs->wParam);
	}
	seq=MsgQueue_AllocateUniqueSeq();
	MSN_SendPacket(s,"MSG","U %d\r\nContent-Type: text/plain; charset=UTF-8\r\n\r\n%s",strlen(msg)+43,msg);
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
	CreateServiceFunction(MSNPROTONAME PS_AUTHALLOW,MsnAuthAllow);
	CreateServiceFunction(MSNPROTONAME PSS_GETINFO,MsnGetInfo);
	CreateServiceFunction(MSNPROTONAME PSS_MESSAGE,MsnSendMessage);
	CreateServiceFunction(MSNPROTONAME PSR_MESSAGE,MsnRecvMessage);
	return 0;
}