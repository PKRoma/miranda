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
#include <windows.h>
#include <stdio.h>
#include <crtdbg.h>
#include "../../icqlib/icq.h"
#include "resource.h"
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/ui/contactlist/m_clist.h"
#include "../../miranda32/random/skin/m_skin.h"
#include "icqproto.h"

void CbDisconnected(icq_Link *link);
VOID CALLBACK IcqCheckDataTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime);
VOID CALLBACK IcqConnectTimeoutTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime);

extern int icqNetTestTimerId,icqConnectTimeoutId,icqConnectRetries;
extern int infoQueriesRunning,infoQueryQueueSize,infoQueryTimeout,testTimeoutsCounter,infoQueryQueueMax;
int icqConnectStatusMode;
extern DWORD *infoQueryQueue;
extern HINSTANCE hInst;

static int IcqGetCaps(WPARAM wParam,LPARAM lParam)
{
	if(wParam==PFLAGNUM_1)
		return PF1_IM|PF1_URL|PF1_FILE|PF1_MODEMSG|PF1_AUTHREQ|PF1_ADDED|PF1_VISLIST|PF1_INVISLIST|PF1_PEER2PEER|PF1_BASICSEARCH|PF1_EXTSEARCH|PF1_FILERESUME|PF1_ADDSEARCHRES;
		//note: ICQ does support PF1_INDIVMODEMSG|PF1_NEWUSER|PF1_CONTACT, but this library doesn't
	if(wParam==PFLAGNUM_2)
		return PF2_ONLINE|PF2_INVISIBLE|PF2_SHORTAWAY|PF2_LONGAWAY|PF2_LIGHTDND|PF2_HEAVYDND|PF2_FREECHAT;
	if(wParam==PFLAGNUM_3)
		return PF2_SHORTAWAY|PF2_LONGAWAY|PF2_LIGHTDND|PF2_HEAVYDND|PF2_FREECHAT;
	return 0;
}

static int IcqGetName(WPARAM wParam,LPARAM lParam)
{
	lstrcpyn((char*)lParam,"ICQ",wParam);
	return 0;
}

static int IcqLoadIcon(WPARAM wParam,LPARAM lParam)
{
	UINT id;

	switch(wParam&0xFFFF) {
		case PLI_PROTOCOL: id=IDI_ICQ; break;
		default: return (int)(HICON)NULL;	
	}
	return (int)LoadImage(hInst,MAKEINTRESOURCE(id),IMAGE_ICON,GetSystemMetrics(wParam&PLIF_SMALL?SM_CXSMICON:SM_CXICON),GetSystemMetrics(wParam&PLIF_SMALL?SM_CYSMICON:SM_CYICON),0);
}

int IcqSetStatus(WPARAM wParam,LPARAM lParam)
{
	int oldStatus=icqStatusMode;
	int newStatus=MirandaStatusToIcq(wParam);

	if(wParam==ID_STATUS_OFFLINE) {
		if(icqConnectTimeoutId) {
			KillTimer(NULL,icqConnectTimeoutId);
			KillTimer(NULL,icqNetTestTimerId);
			icqConnectTimeoutId=0;
		}
		if(icqIsOnline) {
			icq_Logout(plink);
			Sleep(200);
		}
		icq_Disconnect(plink);
		Sleep(100);
		if(icqIsOnline)	{
			icqIsOnline=0;
			CbDisconnected(plink);
			//timer is killed by CbDisconnected()
		}
		icqStatusMode=ID_STATUS_OFFLINE;
		ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)oldStatus,icqStatusMode);
	}
	else {
		if(!icqIsOnline) {
			char *server;
			WORD port;
			DBVARIANT dbv={0};

			if(DBGetContactSetting(NULL,ICQPROTONAME,"Server",&dbv))
				server="icq.mirabilis.com";
			else server=dbv.pszVal;
			port=DBGetContactSettingWord(NULL,ICQPROTONAME,"Port",4000);
			testTimeoutsCounter=10;
			infoQueriesRunning=0;
			infoQueryQueueSize=0;
			icqNetTestTimerId=SetTimer(NULL,0,100,IcqCheckDataTimer);
			if (icq_Connect(plink, server, port)==-1)
			{
				//SetStatusTextEx("Error Connecting",STATUS_SLOT_ICQ);
				//KillTimer(ghwnd,TM_TIMEOUT);
				ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_NOSERVER);
			}
			DBFreeVariant(&dbv);
			icq_Login(plink, newStatus);
			icqConnectTimeoutId=SetTimer(NULL,0,TIMEOUT_CONNECT,IcqConnectTimeoutTimer);
			icqStatusMode=(ID_STATUS_CONNECTING+icqConnectRetries);
			icqConnectStatusMode=wParam;
			ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)oldStatus,icqStatusMode);
		}
		else
		{
			icq_ChangeStatus(plink, newStatus);
			icqStatusMode=wParam;
			ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)oldStatus,icqStatusMode);
		}
	}
	return 0;
}

int IcqGetStatus(WPARAM wParam,LPARAM lParam)
{
	return icqStatusMode;
}

static int IcqSetAwayMsg(WPARAM wParam,LPARAM lParam)
{
	icq_ChangeAwayMessage(plink,MirandaStatusToIcq(wParam),(const char *)lParam);
	return 0;
}

static int IcqAuthAllow(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei;
	DWORD uin;

	if(!icqIsOnline) return 1;
	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.cbBlob=sizeof(DWORD);
	dbei.pBlob=(PBYTE)&uin;
	if(CallService(MS_DB_EVENT_GET,wParam,(LPARAM)&dbei)) return 1;
	if(dbei.eventType!=EVENTTYPE_AUTHREQUEST) return 1;
	if(strcmp(dbei.szModule,ICQPROTONAME)) return 1;
	if(uin<=1) return 1;
	icq_SendAuthMsg(plink,uin);
	return 0;
}

static int IcqBasicSearch(WPARAM wParam,LPARAM lParam)
{
	DWORD uin;
	if(!icqIsOnline) return (int)(HANDLE)NULL;
	uin=strtoul((char*)lParam,NULL,10);
	icq_SendSearchUINReq(plink, (unsigned long)uin);
	return 1;  //icq can only run one search at a time, so the handle is irrelevant
}

static int IcqSearchByEmail(WPARAM wParam,LPARAM lParam)
{
	if(!icqIsOnline) return (int)(HANDLE)NULL;
	icq_SendSearchReq(plink,(char*)lParam,"","","");
	return 1;
}

static int IcqSearchByDetails(WPARAM wParam,LPARAM lParam)
{
	ICQDETAILSSEARCH *isr=(ICQDETAILSSEARCH*)lParam;
	if(!icqIsOnline) return (int)(HANDLE)NULL;
	icq_SendSearchReq(plink,"",isr->nick,isr->firstName,isr->lastName);
	return 1;
}

static HANDLE AddToListByUin(DWORD uin,DWORD flags)
{
	HANDLE hContact;

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact!=NULL) {
		if(!strcmp((char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0),ICQPROTONAME)) {
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
	return hContact;
}

static int IcqAddToList(WPARAM wParam,LPARAM lParam)
{
	ICQSEARCHRESULT *isr=(ICQSEARCHRESULT*)lParam;

	if(isr->hdr.cbSize!=sizeof(ICQSEARCHRESULT)) return (int)(HANDLE)NULL;
	if(isr->uin<=1) return (int)(HANDLE)NULL;
	return (int)AddToListByUin(isr->uin,wParam);
}

static int IcqAddToListByEvent(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei;
	DWORD uin;

	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.cbBlob=sizeof(DWORD);
	dbei.pBlob=(PBYTE)&uin;
	if(CallService(MS_DB_EVENT_GET,lParam,(LPARAM)&dbei)) return (int)(HANDLE)NULL;
	if(dbei.eventType!=EVENTTYPE_AUTHREQUEST && dbei.eventType!=EVENTTYPE_ADDED) return (int)(HANDLE)NULL;
	if(strcmp(dbei.szModule,ICQPROTONAME)) return (int)(HANDLE)NULL;
	if(uin<=1) return (int)(HANDLE)NULL;
	return (int)AddToListByUin(uin,wParam);
}

static int IcqGetInfo(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs=(CCSDATA*)lParam;
	DWORD uin=DBGetContactSettingDword(ccs->hContact,ICQPROTONAME,"UIN",0);

	if(uin==0 || !icqIsOnline) return 1;
	if(infoQueriesRunning<3) {
		infoQueryTimeout=300;	//30 seconds
		icq_SendInfoReq(plink,uin);
		infoQueriesRunning++;
		_RPT2(_CRT_WARN,"basic query %u, %d running\n",uin,infoQueriesRunning);
	}
	else {
		int i;
		for(i=0;i<infoQueryQueueSize;i++) if(infoQueryQueue[i]==uin) break;
		if(i>=infoQueryQueueSize) {
			if(infoQueryQueueSize==infoQueryQueueMax) infoQueryQueue=(DWORD*)realloc(infoQueryQueue,sizeof(DWORD)*++infoQueryQueueMax);
			infoQueryQueue[infoQueryQueueSize++]=uin;
			_RPT2(_CRT_WARN,"queued query for %u, %d queued\n",uin,infoQueryQueueSize);
		}
		else _RPT1(_CRT_WARN,"query for %u already queued\n",uin);
	}
	if(!(ccs->wParam&SGIF_MINIMAL)) {
		icq_SendExtInfoReq(plink,uin);
	}
	return 0;
}

//old interface, maintained for backward compatibility
static int SendIcqMessage(WPARAM wParam,LPARAM lParam)
{
	ICQSENDMESSAGE *ism=(ICQSENDMESSAGE*)lParam;
	HANDLE hContact;

	if(ism->cbSize!=sizeof(ICQSENDMESSAGE)) return 0;
	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact!=NULL) {
		if(!strcmp((char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0),ICQPROTONAME)) {
			if(DBGetContactSettingDword(hContact,"ICQ","UIN",0)==ism->uin) {
				CCSDATA ccs;
				ccs.hContact=hContact;
				ccs.wParam=ism->routeOverride<<16;
				ccs.lParam=(LPARAM)ism->pszMessage;
				return CallService(ICQPROTONAME PSS_MESSAGE,0,(LPARAM)&ccs);
			}
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}
	return 0;
}

static int IcqSendMessage(WPARAM wParam,LPARAM lParam)
{
	int route;
	CCSDATA *ccs=(CCSDATA*)lParam;
	DWORD uin,seq;

	uin=DBGetContactSettingDword(ccs->hContact,ICQPROTONAME,"UIN",0);
	if(uin==0 || !icqIsOnline) return 0;
	if(DBGetContactSettingWord(ccs->hContact,ICQPROTONAME,"Status",ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
		route=ccs->wParam&PIMF_ROUTE_MASK;
	else
		route=PIMF_ROUTE_THRUSERVER;
	if(DBGetContactSettingDword(ccs->hContact,ICQPROTONAME,"IP",0)==0)
		route=PIMF_ROUTE_THRUSERVER;
	switch(route) {
		case PIMF_ROUTE_DIRECT: seq=icq_SendMessage(plink,uin,(char*)ccs->lParam,ICQ_SEND_DIRECT); break;
		case PIMF_ROUTE_THRUSERVER: seq=icq_SendMessage(plink,uin,(char*)ccs->lParam,ICQ_SEND_THRUSERVER); break;
		case PIMF_ROUTE_BESTWAY: seq=icq_SendMessage(plink,uin,(char*)ccs->lParam,ICQ_SEND_BESTWAY); break;
		default: seq=icq_SendMessage(plink,uin,(char*)ccs->lParam,(BYTE)DBGetContactSettingByte(NULL,ICQPROTONAME,"SendThruServer",(BYTE)ICQ_SEND_BESTWAY)); break;
	}
	AddSequenceIdInfo(seq,ACKTYPE_MESSAGE,ccs->hContact,0);
	return seq;
}

static int IcqSendUrl(WPARAM wParam,LPARAM lParam)
{
	BYTE route;
	CCSDATA *ccs=(CCSDATA*)lParam;
	char *pszDescr;
	DWORD uin,seq;

	pszDescr=(char*)ccs->lParam+strlen((char*)ccs->lParam)+1;
	uin=DBGetContactSettingDword(ccs->hContact,ICQPROTONAME,"UIN",0);
	if(uin==0 || !icqIsOnline) return 0;
	if(DBGetContactSettingWord(ccs->hContact,ICQPROTONAME,"Status",ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
		route=(BYTE)DBGetContactSettingByte(NULL,ICQPROTONAME,"SendThruServer",(BYTE)ICQ_SEND_BESTWAY);
	else
		route=ICQ_SEND_THRUSERVER;
	seq=icq_SendURL(plink,uin,(char*)ccs->lParam,pszDescr,route);
	AddSequenceIdInfo(seq,ACKTYPE_URL,ccs->hContact,0);
	return seq;
}

static int IcqGetAwayMsg(WPARAM wParam,LPARAM lParam)
{
	int status,mirandaStatus;
	CCSDATA *ccs=(CCSDATA*)lParam;
	DWORD seq;

	if(!icqIsOnline) return 0;
	mirandaStatus=DBGetContactSettingWord(ccs->hContact,ICQPROTONAME,"Status",ID_STATUS_OFFLINE);
	status=MirandaStatusToIcq(mirandaStatus);
	switch(status) {
		case STATUS_AWAY:
		case STATUS_NA:
		case STATUS_OCCUPIED:
		case STATUS_DND:
		case STATUS_FREE_CHAT:
			seq=icq_TCPSendAwayMessageReq(plink,DBGetContactSettingDword(ccs->hContact,ICQPROTONAME,"UIN",0),status);
			AddSequenceIdInfo(seq,ACKTYPE_AWAYMSG,ccs->hContact,mirandaStatus);
			return seq;
		default:
			return 0;
	}
}

static int IcqFileAllow(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs=(CCSDATA*)lParam;
	int ret;

	ret=(int)icq_AcceptFileRequest(plink,DBGetContactSettingDword(ccs->hContact,ICQPROTONAME,"UIN",0),ccs->wParam);
	icq_FileSessionSetWorkingDir((icq_FileSession*)ret,(char*)ccs->lParam);
	return ret;
}

static int IcqFileDeny(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs=(CCSDATA*)lParam;
	icq_RefuseFileRequest(plink,DBGetContactSettingDword(ccs->hContact,ICQPROTONAME,"UIN",0),ccs->wParam,(char*)ccs->lParam);
	return 0;
}

static int IcqSendFile(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs=(CCSDATA*)lParam;
	return (int)icq_SendFileRequest(plink,DBGetContactSettingDword(ccs->hContact,ICQPROTONAME,"UIN",0),(char*)ccs->wParam,(char**)ccs->lParam);
}

static int IcqSetApparentMode(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs=(CCSDATA*)lParam;
	DWORD uin=DBGetContactSettingDword(ccs->hContact,"ICQ","UIN",0);

	if(uin==0) return 1;
	if(ccs->wParam && ccs->wParam!=ID_STATUS_ONLINE && ccs->wParam!=ID_STATUS_OFFLINE) return 1;
	DBWriteContactSettingWord(ccs->hContact,ICQPROTONAME,"ApparentMode",(WORD)ccs->wParam);
	icq_ContactSetVis(plink,uin,(BYTE)(ccs->wParam==ID_STATUS_ONLINE));
	icq_ContactSetInvis(plink,uin,(BYTE)(ccs->wParam==ID_STATUS_OFFLINE));
	return 0;
}

int LoadIcqServices(void)
{
	CreateServiceFunction(ICQPROTONAME PS_GETCAPS,IcqGetCaps);
	CreateServiceFunction(ICQPROTONAME PS_GETNAME,IcqGetName);
	CreateServiceFunction(ICQPROTONAME PS_LOADICON,IcqLoadIcon);
	CreateServiceFunction(ICQPROTONAME PS_SETSTATUS,IcqSetStatus);
	CreateServiceFunction(ICQPROTONAME PS_GETSTATUS,IcqGetStatus);
	CreateServiceFunction(ICQPROTONAME PS_SETAWAYMSG,IcqSetAwayMsg);
	CreateServiceFunction(ICQPROTONAME PS_AUTHALLOW,IcqAuthAllow);
	//CreateServiceFunction(ICQPROTONAME PS_AUTHDENY,IcqAuthDeny);	//icqlib doesn't support it AFAIK
	CreateServiceFunction(ICQPROTONAME PS_BASICSEARCH,IcqBasicSearch);
	CreateServiceFunction(MS_ICQ_SEARCHBYEMAIL,IcqSearchByEmail);
	CreateServiceFunction(MS_ICQ_SEARCHBYDETAILS,IcqSearchByDetails);
	CreateServiceFunction(ICQPROTONAME PS_ADDTOLIST,IcqAddToList);
	CreateServiceFunction(ICQPROTONAME PS_ADDTOLISTBYEVENT,IcqAddToListByEvent);
	CreateServiceFunction(ICQPROTONAME PSS_GETINFO,IcqGetInfo);
	CreateServiceFunction(ICQPROTONAME PSS_MESSAGE,IcqSendMessage);
	CreateServiceFunction(MS_ICQ_SENDMESSAGE,SendIcqMessage);	//maintained for back compatibility only
	CreateServiceFunction(ICQPROTONAME PSS_URL,IcqSendUrl);
	CreateServiceFunction(ICQPROTONAME PSS_GETAWAYMSG,IcqGetAwayMsg);
	CreateServiceFunction(ICQPROTONAME PSS_FILEALLOW,IcqFileAllow);
	CreateServiceFunction(ICQPROTONAME PSS_FILEDENY,IcqFileDeny);
	//CreateServiceFunction(ICQPROTONAME PSS_FILECANCEL,IcqFileCancel);	 //not yet implemented
	CreateServiceFunction(ICQPROTONAME PSS_FILE,IcqSendFile);
	CreateServiceFunction(ICQPROTONAME PSS_SETAPPARENTMODE,IcqSetApparentMode);
	return 0;
}