/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-2  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include <time.h>
#include "../../miranda32/ui/contactlist/m_clist.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"

extern volatile LONG msnLoggedIn;
extern int msnStatusMode,msnDesiredStatus;

/* The command queue is a widget to channel stuff that needs to be called by
the main thread from the MSN worker threads to there.
*/
struct CmdQueueEntry {
	int type;
	void *data;
};
static CRITICAL_SECTION csCmdQueue;
static int cmdQueueCount,cmdQueueAlloced;
static struct CmdQueueEntry *cmdQueue;
#define CMDQUEUE_PROTOACK        1
#define CMDQUEUE_CHAINRECV       2
#define CMDQUEUE_DBWRITESETTING  3
#define CMDQUEUE_DBCREATECONTACT 4
#define CMDQUEUE_DBAUTHREQUEST   5

struct CmdQueueData_DbCreateContact {
	int temporary;
	HANDLE hWaitEvent;
	HANDLE *phContact;
	char emailAndNick[1];
};

/* NOTE:
This should really be done with core/m_system.h:MS_SYSTEM_WAITONHANDLE, but
I want to keep the plugin compatible with 0.1.1.0 for now.
*/
VOID CALLBACK MSNMainTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	struct CmdQueueEntry entry;

	EnterCriticalSection(&csCmdQueue);
	while(cmdQueueCount) {
		entry=cmdQueue[0];
		memmove(cmdQueue,cmdQueue+1,sizeof(struct CmdQueueEntry)*--cmdQueueCount);
		LeaveCriticalSection(&csCmdQueue);

		switch(entry.type) {
			case CMDQUEUE_PROTOACK:
				CallService(MS_PROTO_BROADCASTACK,0,(LPARAM)entry.data);
				break;
			case CMDQUEUE_CHAINRECV:
				CallService(MS_PROTO_CHAINRECV,0,(LPARAM)entry.data);
				break;
			case CMDQUEUE_DBWRITESETTING:
				CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)*(HANDLE*)entry.data,(LPARAM)((PBYTE)entry.data+sizeof(HANDLE)));
				break;
			case CMDQUEUE_DBCREATECONTACT:
				{	HANDLE hContact;
					struct CmdQueueData_DbCreateContact *dat=(struct CmdQueueData_DbCreateContact*)entry.data;
					char *nick;
					//somebody else could have created it in the meantime
					if((hContact=MSN_HContactFromEmail(dat->emailAndNick,NULL,0,0))==NULL) {
						hContact=(HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
						CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)hContact,(LPARAM)MSNPROTONAME);
						DBWriteContactSettingString(hContact,MSNPROTONAME,"e-mail",dat->emailAndNick);
						nick=dat->emailAndNick+strlen(dat->emailAndNick)+1;
						if(nick[0]) DBWriteContactSettingString(hContact,MSNPROTONAME,"Nick",nick);
						if(dat->temporary)
							DBWriteContactSettingByte(hContact,"CList","NotOnList",1);
					}
					*dat->phContact=hContact;
					SetEvent(dat->hWaitEvent);
				}
				break;
			case CMDQUEUE_DBAUTHREQUEST:
				{	DBEVENTINFO dbei;
					PBYTE pCurBlob;
					char *email,*nick;

					email=(char*)entry.data;
					nick=(char*)entry.data+strlen(email)+1;
					//blob is: 0(DWORD), nick(ASCIIZ), ""(ASCIIZ), ""(ASCIIZ), email(ASCIIZ), ""(ASCIIZ)
					ZeroMemory(&dbei,sizeof(dbei));
					dbei.cbSize=sizeof(dbei);
					dbei.szModule=MSNPROTONAME;
					dbei.timestamp=(DWORD)time(NULL);
					dbei.flags=0;
					dbei.eventType=EVENTTYPE_AUTHREQUEST;
					dbei.cbBlob=sizeof(DWORD)+strlen(nick)+strlen(email)+5;
					pCurBlob=dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
					*(PDWORD)pCurBlob=0; pCurBlob+=sizeof(DWORD);
					strcpy((char*)pCurBlob,nick); pCurBlob+=strlen(nick)+1;
					*pCurBlob='\0'; pCurBlob++;	   //firstName
					*pCurBlob='\0'; pCurBlob++;	   //lastName
					strcpy((char*)pCurBlob,email); pCurBlob+=strlen(email)+1;
					*pCurBlob='\0';         	   //reason
					CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);
				}
				break;
		}
		if(entry.data) free(entry.data);

		EnterCriticalSection(&csCmdQueue);
	}
	LeaveCriticalSection(&csCmdQueue);
}

void CmdQueue_Init(void)
{
	InitializeCriticalSection(&csCmdQueue);
	cmdQueueCount=cmdQueueAlloced=0;
	cmdQueue=NULL;
}

void CmdQueue_Uninit(void)
{
	if(cmdQueue!=NULL) free(cmdQueue);
	DeleteCriticalSection(&csCmdQueue);
}

static int CmdQueue_Add(int type,void *data)
{
	int ret;

	EnterCriticalSection(&csCmdQueue);
	if(cmdQueueCount>=cmdQueueAlloced) {
		cmdQueueAlloced+=8;
		cmdQueue=(struct CmdQueueEntry*)realloc(cmdQueue,sizeof(struct CmdQueueEntry)*cmdQueueAlloced);
	}
	cmdQueue[cmdQueueCount].type=type;
	cmdQueue[cmdQueueCount].data=data;
	ret=cmdQueueCount++;
	LeaveCriticalSection(&csCmdQueue);
	return ret;
}

int CmdQueue_AddProtoAck(HANDLE hContact,int type,int result,HANDLE hProcess,LPARAM lParam)
{
	ACKDATA *ack=(ACKDATA*)malloc(sizeof(ACKDATA));
	memset(ack,0,sizeof(ACKDATA));
	ack->cbSize=sizeof(ACKDATA);
	ack->hContact=hContact;
	ack->hProcess=hProcess;
	ack->lParam=lParam;
	ack->result=result;
	ack->szModule=MSNPROTONAME;
	ack->type=type;
	return CmdQueue_Add(CMDQUEUE_PROTOACK,ack);
}

int CmdQueue_AddChainRecv(CCSDATA *ccs)
{
	PBYTE dat;
	PROTORECVEVENT *pre=(PROTORECVEVENT*)ccs->lParam;

	dat=(PBYTE)malloc(sizeof(CCSDATA)+sizeof(PROTORECVEVENT)+strlen(pre->szMessage)+1);
	memcpy(dat,ccs,sizeof(CCSDATA));
	((CCSDATA*)dat)->lParam=(LPARAM)(dat+sizeof(CCSDATA));
	memcpy(dat+sizeof(CCSDATA),pre,sizeof(PROTORECVEVENT));
	((PROTORECVEVENT*)(dat+sizeof(CCSDATA)))->szMessage=(char*)dat+sizeof(CCSDATA)+sizeof(PROTORECVEVENT);
	strcpy(dat+sizeof(CCSDATA)+sizeof(PROTORECVEVENT),pre->szMessage);
	return CmdQueue_Add(CMDQUEUE_CHAINRECV,dat);
}

int CmdQueue_AddDbWriteSetting(HANDLE hContact,const char *szModule,const char *szSetting,DBVARIANT *dbv)
{
	int bytesReqd,ofs;
	DBCONTACTWRITESETTING *cws;
	char *pBuf;

	bytesReqd=sizeof(HANDLE)+sizeof(DBCONTACTWRITESETTING)+strlen(szModule)+1+strlen(szSetting)+1;
	if(dbv->type==DBVT_BLOB) bytesReqd+=dbv->cpbVal;
	else if(dbv->type==DBVT_ASCIIZ) bytesReqd+=strlen(dbv->pszVal)+1;
	pBuf=(char*)malloc(bytesReqd);
	*(HANDLE*)pBuf=hContact;
	cws=(DBCONTACTWRITESETTING*)(pBuf+sizeof(HANDLE));
	ofs=sizeof(DBCONTACTWRITESETTING)+sizeof(HANDLE);
	strcpy(pBuf+ofs,szModule);
	cws->szModule=pBuf+ofs;
	ofs+=strlen(szModule)+1;
	strcpy(pBuf+ofs,szSetting);
	cws->szSetting=pBuf+ofs;
	ofs+=strlen(szSetting)+1;
	memcpy(&cws->value,dbv,sizeof(DBVARIANT));
	if(dbv->type==DBVT_BLOB) {memcpy(pBuf+ofs,dbv->pbVal,dbv->cpbVal); cws->value.pbVal=pBuf+ofs;}
	else if(dbv->type==DBVT_ASCIIZ) {strcpy(pBuf+ofs,dbv->pszVal); cws->value.pszVal=pBuf+ofs;}
	return CmdQueue_Add(CMDQUEUE_DBWRITESETTING,pBuf);
}

int CmdQueue_AddDbWriteSettingString(HANDLE hContact,const char *szModule,const char *szSetting,const char *pszVal)
{
	DBVARIANT dbv;
	dbv.type=DBVT_ASCIIZ;
	dbv.pszVal=(char*)pszVal;
	return CmdQueue_AddDbWriteSetting(hContact,szModule,szSetting,&dbv);
}

int CmdQueue_AddDbWriteSettingWord(HANDLE hContact,const char *szModule,const char *szSetting,WORD wVal)
{
	DBVARIANT dbv;
	dbv.type=DBVT_WORD;
	dbv.wVal=wVal;
	return CmdQueue_AddDbWriteSetting(hContact,szModule,szSetting,&dbv);
}

int CmdQueue_AddDbCreateContact(const char *email,const char *nick,int temporary,HANDLE hWaitEvent,HANDLE *phContact)
{
	struct CmdQueueData_DbCreateContact *dat;
	dat=(struct CmdQueueData_DbCreateContact*)malloc(strlen(email)+strlen(nick)+1+sizeof(struct CmdQueueData_DbCreateContact));
	dat->temporary=temporary;
	dat->hWaitEvent=hWaitEvent;
	dat->phContact=phContact;
	strcpy(dat->emailAndNick,email);
	strcpy(dat->emailAndNick+strlen(email)+1,nick);
	return CmdQueue_Add(CMDQUEUE_DBCREATECONTACT,dat);
}

int CmdQueue_AddDbAuthRequest(const char *email,const char *nick)
{
	PBYTE dat;
	dat=(PBYTE)malloc(strlen(email)+strlen(nick)+2);
	strcpy((char*)dat,email);
	strcpy((char*)dat+strlen(email)+1,nick);
	return CmdQueue_Add(CMDQUEUE_DBAUTHREQUEST,dat);
}