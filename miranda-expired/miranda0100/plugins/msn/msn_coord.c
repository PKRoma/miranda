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
#include "../../miranda32/ui/contactlist/m_clist.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"

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
#define CMDQUEUE_DBWRITESETTING  2

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
			case CMDQUEUE_DBWRITESETTING:
				CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)*(HANDLE*)entry.data,(LPARAM)((PBYTE)entry.data+sizeof(HANDLE)));
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

int CmdQueue_AddDbWriteSetting(HANDLE hContact,const char *szModule,const char *szSetting,DBVARIANT *dbv)
{
	int bytesReqd,ofs;
	DBCONTACTWRITESETTING *cws;
	char *pBuf;

	bytesReqd=sizeof(HANDLE)+sizeof(DBCONTACTWRITESETTING)+strlen(szModule)+1+strlen(szSetting+1);
	if(dbv->type==DBVT_BLOB) bytesReqd+=dbv->cpbVal;
	else if(dbv->type==DBVT_ASCIIZ) bytesReqd+=strlen(dbv->pszVal)+1;
	pBuf=(char*)malloc(bytesReqd);
	*(HANDLE*)pBuf=hContact;
	cws=(DBCONTACTWRITESETTING*)pBuf+sizeof(HANDLE);
	ofs=sizeof(DBCONTACTWRITESETTING)+sizeof(HANDLE);
	strcpy(pBuf+ofs,szModule);
	cws->szModule=pBuf+ofs;
	ofs+=strlen(szModule);
	strcpy(pBuf+ofs,szSetting);
	cws->szSetting=pBuf+ofs;
	ofs+=strlen(szSetting);
	memcpy(&cws->value,dbv,sizeof(DBVARIANT));
	if(dbv->type==DBVT_BLOB) memcpy(pBuf+ofs,dbv->pbVal,dbv->cpbVal);
	else if(dbv->type==DBVT_ASCIIZ) strcpy(pBuf+ofs,dbv->pszVal);
	return CmdQueue_Add(CMDQUEUE_DBWRITESETTING,cws);
}

int CmdQueue_AddDbWriteSettingString(HANDLE hContact,const char *szModule,const char *szSetting,const char *pszVal)
{
	DBVARIANT dbv;
	dbv.type=DBVT_ASCIIZ;
	dbv.pszVal=(char*)pszVal;
	return CmdQueue_AddDbWriteSetting(hContact,szModule,szSetting,&dbv);
}