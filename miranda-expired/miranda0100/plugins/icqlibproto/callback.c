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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <crtdbg.h>
#include "../../icqlib/icq.h"
#include "resource.h"
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/ui/contactlist/m_clist.h"
#include "../../miranda32/random/ignore/m_ignore.h"
#include "../../miranda32/random/langpack/m_langpack.h"
#include "icqproto.h"

extern int icqNetTestTimerId,icqConnectTimeoutId,icqConnectRetries;
extern int icqConnectStatusMode;
extern HANDLE hEventSentEvent,hLogEvent;
extern int infoQueriesRunning,infoQueryQueueSize,infoQueryTimeout;
extern DWORD *infoQueryQueue;

static HANDLE HContactFromUIN(DWORD uin,int allowAdd)
{
	HANDLE hContact;

	if(DBGetContactSettingDword(NULL,ICQPROTONAME,"UIN",0)==uin) return NULL;
	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact!=NULL) {
		if(DBGetContactSettingDword(hContact,ICQPROTONAME,"UIN",0)==uin) return hContact;
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}
	//not present: add
	if(allowAdd) {
		hContact=(HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
		CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)hContact,(LPARAM)ICQPROTONAME);
		DBWriteContactSettingDword(hContact,ICQPROTONAME,"UIN",uin);
		DBWriteContactSettingByte(hContact,"CList","NotOnList",1);
		DBWriteContactSettingByte(hContact,"CList","Hidden",1);
		return hContact;
	}
	return INVALID_HANDLE_VALUE;
}

//triggered when a login is successful
static void CbLogged(icq_Link *link)
{
	icqIsOnline=1;
	icqConnectRetries=0;
	KillTimer(NULL,icqConnectTimeoutId);
	icqConnectTimeoutId=0;
	{	HANDLE hContact;
		DWORD uin;
		WORD vis;
		char *szProto;

		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
		while(hContact!=NULL) {
			szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
			if(szProto!=NULL && !strcmp(szProto,ICQPROTONAME)) {
				if(uin=DBGetContactSettingDword(hContact,ICQPROTONAME,"UIN",0)) {
					vis=(WORD)DBGetContactSettingWord(hContact,ICQPROTONAME,"ApparentMode",0);
					icq_ContactSetVis(plink,uin,(BYTE)(vis==ID_STATUS_ONLINE));
					icq_ContactSetInvis(plink,uin,(BYTE)(vis==ID_STATUS_OFFLINE));
				}
			}
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
		}
	}
	icq_SendVisibleList(plink);
	icq_SendInvisibleList(plink);
	icq_SendContactList(link);
	icqStatusMode=icqConnectStatusMode;
	ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)ID_STATUS_CONNECTING,icqStatusMode);
	{	CCSDATA ccs;
		ccs.hContact=NULL;
		ccs.wParam=ccs.lParam=0;
		ccs.szProtoService=ICQPROTONAME;
		CallService(ICQPROTONAME PSS_GETINFO,0,(LPARAM)&ccs);
	}
}

void CbDisconnected(icq_Link *link)
{
	HANDLE hContact;
	int oldStatus=icqStatusMode;
	char *szProto;

	icqStatusMode=ID_STATUS_OFFLINE;
	KillTimer(NULL,icqNetTestTimerId);
	ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)oldStatus,ID_STATUS_ONLINE);
	//set all contacts to 'offline'
	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact!=NULL) {
		szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
		if(szProto!=NULL)
			if(!strcmp(szProto,ICQPROTONAME))
				if(DBGetContactSettingDword(hContact,ICQPROTONAME,"UIN",0))
					if(DBGetContactSettingWord(hContact,ICQPROTONAME,"Status",ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE)
						DBWriteContactSettingWord(hContact,ICQPROTONAME,"Status",ID_STATUS_OFFLINE);
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}
	//attempt reconnect if we were kicked off
	if(icqIsOnline) {
		IcqSetStatus(ID_STATUS_OFFLINE,0);
		IcqSetStatus(oldStatus,0);
	}
	else icqIsOnline=0;
}

static void CbRecvMsg(icq_Link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *msg)
{
	CCSDATA ccs;
	PROTORECVEVENT pre;

	ccs.hContact=HContactFromUIN(uin,1);
	ccs.szProtoService=PSR_MESSAGE;
	ccs.wParam=0;
	ccs.lParam=(LPARAM)&pre;
	pre.flags=0;
	pre.timestamp=TimestampLocalToGMT(YMDHMSToTime(year,month,day,hour,minute,0));
	pre.szMessage=(char*)msg;
	pre.lParam=0;
	CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);
}

static int IcqRecvMessage(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei;
	CCSDATA *ccs=(CCSDATA*)lParam;
	PROTORECVEVENT *pre=(PROTORECVEVENT*)ccs->lParam;

	DBDeleteContactSetting(ccs->hContact,"CList","Hidden");
	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule="ICQ";
	dbei.timestamp=pre->timestamp;
	dbei.flags=pre->flags&PREF_CREATEREAD?DBEF_READ:0;
	dbei.eventType=EVENTTYPE_MESSAGE;
	dbei.cbBlob=strlen(pre->szMessage)+1;
	dbei.pBlob=(PBYTE)pre->szMessage;
	CallService(MS_DB_EVENT_ADD,(WPARAM)ccs->hContact,(LPARAM)&dbei);
	return 0;
}

static void CbRecvUrl(icq_Link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *url, const char *desc)
{
	CCSDATA ccs;
	PROTORECVEVENT pre;
	char *blob;

	ccs.hContact=HContactFromUIN(uin,1);
	ccs.szProtoService=PSR_URL;
	ccs.wParam=0;
	ccs.lParam=(LPARAM)&pre;
	pre.flags=0;
	pre.timestamp=TimestampLocalToGMT(YMDHMSToTime(year,month,day,hour,minute,0));
	blob=(char*)malloc(strlen(url)+strlen(desc)+2);
	strcpy(blob,url);
	strcpy(blob+strlen(url)+1,desc);
	pre.szMessage=blob;
	pre.lParam=0;
	CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);
	free(blob);
}

static int IcqRecvUrl(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei;
	CCSDATA *ccs=(CCSDATA*)lParam;
	PROTORECVEVENT *pre=(PROTORECVEVENT*)ccs->lParam;
	char *pszDescr;

	DBDeleteContactSetting(ccs->hContact,"CList","Hidden");
	pszDescr=pre->szMessage+strlen(pre->szMessage)+1;
	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule="ICQ";
	dbei.timestamp=pre->timestamp;
	dbei.flags=pre->flags&PREF_CREATEREAD?DBEF_READ:0;
	dbei.eventType=EVENTTYPE_URL;
	dbei.cbBlob=strlen(pre->szMessage)+strlen(pszDescr)+2;
	dbei.pBlob=pre->szMessage;
	CallService(MS_DB_EVENT_ADD,(WPARAM)ccs->hContact,(LPARAM)&dbei);
	return 0;
}

//blob is: nick (ASCIIZ), email (ASCIIZ), message (ASCIIZ)
static void CbRecvWebPager(icq_Link *link, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *nick, const char *email, const char *msg)
{
	DBEVENTINFO dbei;

	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule="ICQ";
	dbei.timestamp=TimestampLocalToGMT(YMDHMSToTime(year,month,day,hour,minute,0));
	dbei.flags=0;
	dbei.eventType=ICQEVENTTYPE_WEBPAGER;
	dbei.cbBlob=strlen(nick)+strlen(email)+strlen(msg)+3;
	dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
	CopyMemory(dbei.pBlob,nick,strlen(nick)+1);
	CopyMemory(dbei.pBlob+strlen(nick)+1,email,strlen(email)+1);
	CopyMemory(dbei.pBlob+strlen(nick)+strlen(email)+1,msg,strlen(msg)+1);
	CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);
}

static void CbUserOnline(icq_Link *link, unsigned long uin, unsigned long status, unsigned long ip, unsigned short port, unsigned long real_ip, unsigned char tcp_flag)
{	
	HANDLE hContact;

	hContact=HContactFromUIN(uin,0);
	if(hContact==INVALID_HANDLE_VALUE) return;
	DBWriteContactSettingDword(hContact,"ICQ","IP",ip);
	DBWriteContactSettingDword(hContact,"ICQ","RealIP",real_ip);
	DBWriteContactSettingWord(hContact,"ICQ","Port",port);
	DBWriteContactSettingWord(hContact,"ICQ","Status",(WORD)IcqStatusToMiranda(status));
	//leave the status til last because most people will hook that change
}

static void CbUserOffline(icq_Link *link, unsigned long uin)
{
	HANDLE hContact;

	hContact=HContactFromUIN(uin,0);
	if(hContact==INVALID_HANDLE_VALUE) return;
	DBWriteContactSettingWord(hContact,"ICQ","Status",ID_STATUS_OFFLINE);
}

//sent to acknowledge that a contact's status mode has changed
static void CbUserStatusUpdate(icq_Link *link, unsigned long uin, unsigned long status)
{
	HANDLE hContact;
	WORD mirandaStatus=(WORD)IcqStatusToMiranda(status);

	hContact=HContactFromUIN(uin,0);
	if(hContact==INVALID_HANDLE_VALUE) return;
	if(mirandaStatus!=DBGetContactSettingWord(hContact,"ICQ","Status",ID_STATUS_OFFLINE))
		DBWriteContactSettingWord(hContact,"ICQ","Status",mirandaStatus);
}

//sent in response to a simple get user details
static void CbInfoReply(icq_Link *link, unsigned long uin, const char *nick, const char *first, const char *last, const char *email, char auth)
{
	HANDLE hContact;

	hContact=HContactFromUIN(uin,0);
	if(hContact==INVALID_HANDLE_VALUE) return;
	_RPT2(_CRT_WARN,"query reply for %u, hContact=%x\n",uin,hContact);
	if(nick[0]) DBWriteContactSettingString(hContact,"ICQ","Nick",nick);
	else DBDeleteContactSetting(hContact,"ICQ","Nick");
	if(first[0]) DBWriteContactSettingString(hContact,"ICQ","FirstName",first);
	else DBDeleteContactSetting(hContact,"ICQ","FirstName");
	if(last[0]) DBWriteContactSettingString(hContact,"ICQ","LastName",last);
	else DBDeleteContactSetting(hContact,"ICQ","LastName");
	if(email[0]) DBWriteContactSettingString(hContact,"ICQ","e-mail",email);
	else DBDeleteContactSetting(hContact,"ICQ","e-mail");
	DBWriteContactSettingByte(hContact,"ICQ","Auth",(BYTE)auth);
	ProtoBroadcastAck(ICQPROTONAME,hContact,ACKTYPE_GETINFO,ACKRESULT_SUCCESS,(HANDLE)2,0);

	if(--infoQueriesRunning<3 && infoQueryQueueSize) {
		infoQueryTimeout=300;      //30 secs
		icq_SendInfoReq(link, infoQueryQueue[0]);
		infoQueriesRunning++;
		infoQueryQueueSize--;
		_RPT3(_CRT_WARN,"sent query for %u, %d running, %d queued\n",infoQueryQueue[0],infoQueriesRunning,infoQueryQueueSize);
		MoveMemory(infoQueryQueue,infoQueryQueue+1,sizeof(DWORD)*infoQueryQueueSize);
	}
}

//sent in response to an extended get user info
static void CbExtInfoReply(icq_Link *link, unsigned long uin, const char *city, unsigned short country_code, char country_stat, const char *state, unsigned short age, char gender, const char *phone, const char *hp, const char *about)
{
	HANDLE hContact;

	hContact=HContactFromUIN(uin,0);
	if(hContact==INVALID_HANDLE_VALUE) return;
	DBWriteContactSettingString(hContact,"ICQ","City",city);
	if(country_code==0 || country_code==0xFFFF) DBDeleteContactSetting(hContact,"ICQ","Country");
	else DBWriteContactSettingString(hContact,"ICQ","Country",icq_GetCountryName(country_code));
	//what's country_stat?
	DBWriteContactSettingString(hContact,"ICQ","State",state);
	DBWriteContactSettingWord(hContact,"ICQ","Age",age);
	DBWriteContactSettingByte(hContact,"ICQ","Gender",(BYTE)(gender==0?'?':(gender==1?'F':'M')));
	DBWriteContactSettingString(hContact,"ICQ","Phone",phone);
	DBWriteContactSettingString(hContact,"ICQ","Homepage",hp);
	DBWriteContactSettingString(hContact,"ICQ","About",about);
	ProtoBroadcastAck(ICQPROTONAME,hContact,ACKTYPE_GETINFO,ACKRESULT_SUCCESS,(HANDLE)2,1);
}

//sent when a message or url actually goes. id should match the value returned when the
//send was initiated
static void CbRequestNotify(icq_Link *link, unsigned long id, int type, int arg, void *data)
{
	struct sequenceIdInfoType *sii;

	/*{char str[256];
	wsprintf(str,"notify: id=%d type=%d arg=%d\n",id,type,arg);
	OutputDebugString(str);
	}*/
	NotifyEventHooks(hEventSentEvent,id,type);	 //back compatibility only
	sii=FindSequenceIdInfo(id);
	if(sii!=NULL) {
		int result=-1;
		if(sii->acktype==ACKTYPE_MESSAGE || sii->acktype==ACKTYPE_URL) {
			if(type==ICQ_NOTIFY_SUCCESS) result=ACKRESULT_SUCCESS;
			else if(type==ICQ_NOTIFY_FAILED) result=ACKRESULT_FAILED;
		}
		if(result!=-1) {
			ProtoBroadcastAck(ICQPROTONAME,sii->hContact,sii->acktype,result,(HANDLE)id,0);
			RemoveSequenceIdInfo(id);
		}
	}
}

static void CbRecvAwayMsg(icq_Link *link, unsigned long id, const char *msg)
{
	CCSDATA ccs;
	PROTORECVEVENT pre;
	struct sequenceIdInfoType *sii;

	sii=FindSequenceIdInfo(id);
	if(sii!=NULL && sii->acktype==ACKTYPE_AWAYMSG) {
		ccs.szProtoService=PSR_AWAYMSG;
		ccs.hContact=sii->hContact;
		ccs.wParam=sii->lParam;
		ccs.lParam=(LPARAM)&pre;
		pre.flags=0;
		pre.szMessage=(char*)msg;
		pre.timestamp=time(NULL);
		pre.lParam=id;
		CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);
		RemoveSequenceIdInfo(id);
	}
}

static int IcqRecvAwayMsg(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs=(CCSDATA*)lParam;
	PROTORECVEVENT *pre=(PROTORECVEVENT*)ccs->lParam;
	ProtoBroadcastAck(ICQPROTONAME,ccs->hContact,ACKTYPE_AWAYMSG,ACKRESULT_SUCCESS,(HANDLE)pre->lParam,(LPARAM)pre->szMessage);
	return 0;
}

static void CbFileNotify(icq_FileSession *session, int type, int arg, void *data)
{
	HANDLE hContact;
	/*{char str[256];
	wsprintf(str,"file notify: session=%p type=%d arg=%d\n",session,type,arg);
	OutputDebugString(str);
	}*/
	hContact=HContactFromUIN(session->remote_uin,0);
	if(hContact==INVALID_HANDLE_VALUE) return;
	switch(type) {
		case FILE_NOTIFY_DATAPACKET:
		case FILE_NOTIFY_STATUS:
		{	PROTOFILETRANSFERSTATUS fts;
			int result;
			fts.cbSize=sizeof(fts);
			fts.hContact=hContact;
			fts.sending=session->direction==FILE_STATUS_SENDING;
			fts.files=session->files;
			fts.totalFiles=session->total_files;
			fts.currentFileNumber=session->current_file_num;
			fts.totalBytes=session->total_bytes;
			fts.totalProgress=session->total_transferred_bytes;
			fts.workingDir=session->working_dir;
			fts.currentFile=session->current_file;
			fts.currentFileSize=session->current_file_size;
			fts.currentFileProgress=session->current_file_progress;
			switch(session->status) {
				case FILE_STATUS_LISTENING:
					result=ACKRESULT_SENTREQUEST; break;
				case FILE_STATUS_CONNECTED:
					result=ACKRESULT_CONNECTED; break;
				case FILE_STATUS_CONNECTING:
					result=ACKRESULT_CONNECTING; break;
				case FILE_STATUS_INITIALIZING:
					result=ACKRESULT_INITIALISING; break;
				case FILE_STATUS_NEXT_FILE:
					result=ACKRESULT_NEXTFILE; break;
				case FILE_STATUS_SENDING:
				case FILE_STATUS_RECEIVING:
					result=ACKRESULT_DATA; break;
			}
			ProtoBroadcastAck(ICQPROTONAME,hContact,ACKTYPE_FILE,result,session,(LPARAM)&fts);
			break;
		}
		case FILE_NOTIFY_CLOSE:
			ProtoBroadcastAck(ICQPROTONAME,hContact,ACKTYPE_FILE,ACKRESULT_SUCCESS,session,0);
			break;
	}
}

//'you were added'
static void CbRecvAdded(icq_Link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *nick, const char *first, const char *last, const char *email)
{
	DBEVENTINFO dbei;
	PBYTE pCurBlob;
	
	//blob is: uin(DWORD), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ)
	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule="ICQ";
	dbei.timestamp=TimestampLocalToGMT(YMDHMSToTime(year,month,day,hour,minute,0));
	dbei.flags=0;
	dbei.eventType=EVENTTYPE_ADDED;
	dbei.cbBlob=sizeof(DWORD)+strlen(nick)+strlen(first)+strlen(last)+strlen(email)+4;
	pCurBlob=dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
	CopyMemory(pCurBlob,&uin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
	CopyMemory(pCurBlob,nick,strlen(nick)+1); pCurBlob+=strlen(nick)+1;
	CopyMemory(pCurBlob,first,strlen(first)+1); pCurBlob+=strlen(first)+1;
	CopyMemory(pCurBlob,last,strlen(last)+1); pCurBlob+=strlen(last)+1;
	CopyMemory(pCurBlob,email,strlen(email)+1); pCurBlob+=strlen(email)+1;
	CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);
}

static void CbRecvAuthReq(icq_Link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *nick, const char *first, const char *last, const char *email, const char *reason)
{
	DBEVENTINFO dbei;
	PBYTE pCurBlob;
	
	//blob is: uin(DWORD), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)
	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule="ICQ";
	dbei.timestamp=TimestampLocalToGMT(YMDHMSToTime(year,month,day,hour,minute,0));
	dbei.flags=0;
	dbei.eventType=EVENTTYPE_AUTHREQUEST;
	dbei.cbBlob=sizeof(DWORD)+strlen(nick)+strlen(first)+strlen(last)+strlen(email)+strlen(reason)+5;
	pCurBlob=dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
	CopyMemory(pCurBlob,&uin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
	CopyMemory(pCurBlob,nick,strlen(nick)+1); pCurBlob+=strlen(nick)+1;
	CopyMemory(pCurBlob,first,strlen(first)+1); pCurBlob+=strlen(first)+1;
	CopyMemory(pCurBlob,last,strlen(last)+1); pCurBlob+=strlen(last)+1;
	CopyMemory(pCurBlob,email,strlen(email)+1); pCurBlob+=strlen(email)+1;
	CopyMemory(pCurBlob,reason,strlen(reason)+1);
	CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);
}

//in response to a search
static void CbUserFound(icq_Link *link, unsigned long uin, const char *nick, const char *first, const char *last, const char *email, char auth)
{
	ICQSEARCHRESULT isr;

	isr.hdr.cbSize=sizeof(isr);
	isr.hdr.nick=(char*)nick;
	isr.hdr.firstName=(char*)first;
	isr.hdr.lastName=(char*)last;
	isr.hdr.email=(char*)email;
	isr.uin=uin;
	isr.auth=auth;
	ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_SEARCH,ACKRESULT_DATA,(HANDLE)1,(LPARAM)&isr);
}

static void CbSearchDone(icq_Link *link)
{
	ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_SEARCH,ACKRESULT_SUCCESS,(HANDLE)1,0);
}

static void CbWrongPassword(icq_Link *link)
{
//	PlaySoundEvent("MICQ_Error");
	//this messagebox perhaps ought not to be here
	MessageBox(NULL, Translate("Your password is incorrect"), "Miranda ICQ", MB_OK);
	ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_WRONGPASSWORD);
}

static void CbInvalidUIN(icq_Link *link)
{
#ifdef _DEBUG
	OutputDebugString("Miranda made a request with an invalid UIN\n");
#endif
}

//just random logging events
static void CbLog(icq_Link *link, time_t time, unsigned char level, const char *str)
{
#ifdef _DEBUG
	OutputDebugString(str);
#endif
	NotifyEventHooks(hLogEvent,level,(LPARAM)str);
}

static void CbRecvFileReq(icq_Link *link, unsigned long uin,
       unsigned char hour, unsigned char minute, unsigned char day,
       unsigned char month, unsigned short year, const char *descr,
       const char *filename, unsigned long filesize, unsigned long seq)
{
	CCSDATA ccs;
	PROTORECVFILE prf;
	char *files[2];

	ccs.hContact=HContactFromUIN(uin,1);
	ccs.szProtoService=PSR_FILE;
	ccs.wParam=0;
	ccs.lParam=(LPARAM)&prf;
	prf.flags=0;
	prf.timestamp=TimestampLocalToGMT(YMDHMSToTime(year,month,day,hour,minute,0));
	prf.szDescription=(char*)descr;
	prf.pFiles=files;
	files[0]=(char*)filename;
	files[1]=NULL;
	prf.lParam=seq;
	CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);
}

static int IcqRecvFile(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei;
	CCSDATA *ccs=(CCSDATA*)lParam;
	PROTORECVFILE *prf=(PROTORECVFILE*)ccs->lParam;

	//blob is: sequenceid(DWORD),filename(ASCIIZ),description(ASCIIZ)
	DBDeleteContactSetting(ccs->hContact,"CList","Hidden");
	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule="ICQ";
	dbei.timestamp=prf->timestamp;
	dbei.flags=prf->flags&PREF_CREATEREAD?DBEF_READ:0;
	dbei.eventType=EVENTTYPE_FILE;
	dbei.cbBlob=4+strlen(prf->szDescription)+strlen(prf->pFiles[0])+2;
	dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
	*(PDWORD)dbei.pBlob=prf->lParam;
	CopyMemory(dbei.pBlob+4,prf->pFiles[0],strlen(prf->pFiles[0])+1);
	CopyMemory(dbei.pBlob+4+strlen(prf->pFiles[0])+1,prf->szDescription,strlen(prf->szDescription)+1);
	CallService(MS_DB_EVENT_ADD,(WPARAM)ccs->hContact,(LPARAM)&dbei);
	return 0;
}

static void CbMetaUserFound(icq_Link *link, unsigned short seq2, unsigned long uin, const char *nick, const char *first, const char *last, const char *email, char auth)
{
}

static void CbMetaUserInfo(icq_Link *link, unsigned short seq2, const char *nick, const char *first, const char *last, const char *pri_eml, const char *sec_eml, const char *old_eml, const char *city, const char *state, const char *phone, const char *fax, const char *street, const char *cellular, unsigned long zip, unsigned short country, unsigned char timezone, unsigned char auth, unsigned char webaware, unsigned char hideip)
{
}

static void CbMetaUserWork(icq_Link *link, unsigned short seq2, const char *wcity, const char *wstate, const char *wphone, const char *wfax, const char *waddress, unsigned long wzip, unsigned short wcountry, const char *company, const char *department, const char *job, unsigned short occupation, const char *whomepage)
{
}

static void CbMetaUserMore(icq_Link *link, unsigned short seq2, unsigned short age, unsigned char gender, const char *homepage, unsigned char byear, unsigned char bmonth, unsigned char bday, unsigned char lang1, unsigned char lang2, unsigned char lang3)
{
}

static void CbMetaUserAbout(icq_Link *link, unsigned short seq2, const char *about)
{
}

static void CbMetaUserInterests(icq_Link *link, unsigned short seq2, unsigned char num, unsigned short icat1, const char *int1, unsigned short icat2, const char *int2, unsigned short icat3, const char *int3, unsigned short icat4, const char *int4)
{
}

static void CbMetaUserAffiliations(icq_Link *link, unsigned short seq2, unsigned char anum, unsigned short acat1, const char *aff1, unsigned short acat2, const char *aff2, unsigned short acat3, const char *aff3, unsigned short acat4, const char *aff4, unsigned char bnum, unsigned short bcat1, const char *back1, unsigned short bcat2, const char *back2, unsigned short bcat3, const char *back3, unsigned short bcat4, const char *back4)
{
}

static void CbMetaUserHomePageCategory(icq_Link *link, unsigned short seq2, unsigned char num, unsigned short hcat1, const char *htext1)
{
}

//in response to a 'create new user'
static void CbNewUIN(icq_Link *link, unsigned long uin)
{
}

int LoadIcqRecvServices(void)
{
	CreateServiceFunction(ICQPROTONAME PSR_MESSAGE,IcqRecvMessage);
	CreateServiceFunction(ICQPROTONAME PSR_URL,IcqRecvUrl);
	CreateServiceFunction(ICQPROTONAME PSR_FILE,IcqRecvFile);
	CreateServiceFunction(ICQPROTONAME PSR_AWAYMSG,IcqRecvAwayMsg);
	// register callbacks
	plink->icq_Logged = CbLogged;
	plink->icq_Disconnected = CbDisconnected;
	plink->icq_RecvMessage = CbRecvMsg;
	plink->icq_UserOnline = CbUserOnline;
	plink->icq_UserOffline = CbUserOffline;
	plink->icq_UserStatusUpdate = CbUserStatusUpdate;
	plink->icq_InfoReply = CbInfoReply;
	plink->icq_ExtInfoReply = CbExtInfoReply;
	plink->icq_RecvURL = CbRecvUrl;
	plink->icq_RecvFileReq=CbRecvFileReq;
	plink->icq_RecvAdded = CbRecvAdded;
	plink->icq_UserFound = CbUserFound;
	plink->icq_SearchDone = CbSearchDone;
	plink->icq_RequestNotify = CbRequestNotify;
	plink->icq_RecvAwayMsg = CbRecvAwayMsg;
	plink->icq_FileNotify = CbFileNotify;
	plink->icq_Log = CbLog;
	plink->icq_WrongPassword = CbWrongPassword;
	plink->icq_InvalidUIN = CbInvalidUIN;
	plink->icq_RecvAuthReq = CbRecvAuthReq;
	plink->icq_MetaUserFound = CbMetaUserFound;
	plink->icq_MetaUserInfo = CbMetaUserInfo;
	plink->icq_MetaUserWork = CbMetaUserWork;
	plink->icq_MetaUserMore = CbMetaUserMore;
	plink->icq_MetaUserAbout = CbMetaUserAbout;
	plink->icq_MetaUserInterests = CbMetaUserInterests;
	plink->icq_MetaUserAffiliations = CbMetaUserAffiliations;
	plink->icq_MetaUserHomePageCategory = CbMetaUserHomePageCategory;
	plink->icq_NewUIN = CbNewUIN;
	plink->icq_RecvWebPager = CbRecvWebPager;
/*	  not hooked:
  void (*icq_RecvMailExpress)(struct icq_Link *link,unsigned char hour,
       unsigned char minute, unsigned char day, unsigned char month,
       unsigned short year, const char *nick, const char *email,
       const char *msg);
  void (*icq_RecvChatReq)(struct icq_Link *link, unsigned long uin,
       unsigned char hour, unsigned char minute, unsigned char day,
       unsigned char month, unsigned short year, const char *descr,
       unsigned long seq);
  void (*icq_SetTimeout)(struct icq_Link *link, long interval);
*/
	return 0;
}