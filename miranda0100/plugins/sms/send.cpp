/*
Miranda SMS Plugin
Copyright (C) 2001  Richard Hughes

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
#include <commctrl.h>
#include <time.h>
#include <stdarg.h>
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/protocols/icqoscar/m_icq.h"

extern HWND hwndStatus;

#define STATUS_INIT        0
#define STATUS_SENT        1
#define STATUS_ACKED       2
#define STATUS_SUCCESS     3
#define STATUS_SENDFAILED  4
#define STATUS_UNDELIVERABLE 5
#define STATUS_BADACK      6
#define STATUS_RCPTFAILED  7
#define STATUS_BADRCPT     8
#define STATUS_NOSMTP      9
static char *statusToText[]={
	"Initialising send",
	"Request sent",
	"Request acknowledged",
	"Message sent",
	"Send failed",
	"Undeliverable",
	"Invalid ack data",
	"Receipt failed",
	"Invalid receipt data",
	"Redirect via e-mail not supported"};

struct SmsSendEntry {
	char *number;
	char *text;
	int iListIndex;
	int status;
	HANDLE hProcess;
	char *ack;
	char *receipt;
	FILETIME startTime;
} static *smsSend;
static int smsSendCount;

static char *GetXMLField(const char *xml,const char *tag1,...)
{
	va_list va;
	int thisLevel,i,start=-1;
	const char *findTag;

	va_start(va,tag1);
	thisLevel=0;
	findTag=tag1;
	for(i=0;xml[i];i++) {
		if(xml[i]=='<') {
			char tag[64],*tagend=strchr(xml+i,'>');
			if(tagend==NULL) return NULL;
			lstrcpyn(tag,xml+i+1,min(sizeof(tag),tagend-xml-i));
			if(tag[0]=='/') {
				if(--thisLevel<0) {
					char *ret;
					if(start==-1) return NULL;
					ret=(char*)malloc(i-start+1);
					lstrcpyn(ret,xml+start,i-start+1);
					return ret;
				}
			}
			else {
				if(++thisLevel==1 && !lstrcmpi(tag,findTag)) {
					findTag=va_arg(va,const char*);
					if(findTag==NULL) start=tagend-xml+1;
					thisLevel=0;
				}
			}
			i=tagend-xml;
		}
	}
	va_end(va);
	return NULL;
}

static void UpdateStatusColumn(int i)
{
	ListView_SetItemText(hwndStatus,smsSend[i].iListIndex,1,statusToText[smsSend[i].status]);
}

void StartSmsSend(const char *number,const char *text)
{
	LVITEM lvi;
	int i;

	i=smsSendCount++;
	smsSend=(SmsSendEntry*)realloc(smsSend,sizeof(SmsSendEntry)*smsSendCount);
	smsSend[i].number=_strdup(number);
	smsSend[i].text=_strdup(text);
	smsSend[i].iListIndex=ListView_GetItemCount(hwndStatus);
	smsSend[i].status=STATUS_INIT;
	smsSend[i].ack=NULL;
	smsSend[i].receipt=NULL;
	GetSystemTimeAsFileTime(&smsSend[i].startTime);

	lvi.mask=LVIF_TEXT|LVIF_PARAM;
	lvi.iItem=smsSend[i].iListIndex;
	lvi.iSubItem=0;
	lvi.pszText=(char*)number;
	lvi.lParam=i;
	smsSend[i].iListIndex=ListView_InsertItem(hwndStatus,&lvi);
	UpdateStatusColumn(i);
	ListView_EnsureVisible(hwndStatus,smsSend[i].iListIndex,FALSE);

	smsSend[i].hProcess=(HANDLE)CallService(MS_ICQ_SENDSMS,(WPARAM)number,(LPARAM)text);
	smsSend[i].status=smsSend[i].hProcess==NULL?STATUS_SENDFAILED:STATUS_SENT;
	UpdateStatusColumn(i);
}

static int ProtoAckHook(WPARAM wParam,LPARAM lParam)
{
	ACKDATA *ack=(ACKDATA*)lParam;
	int i;

	if(ack->type!=ICQACKTYPE_SMS) return 0;
	if(lstrcmp(ack->szModule,"ICQ")) return 0;
	if(ack->result==ACKRESULT_SENTREQUEST) {
		char *szDeliverable,*szId;

		for(i=0;i<smsSendCount;i++)
			if(smsSend[i].hProcess==ack->hProcess) break;
		if(i>=smsSendCount) return 0;
		smsSend[i].ack=_strdup((char*)ack->lParam);
		szDeliverable=GetXMLField(smsSend[i].ack,"sms_response","deliverable",NULL);
		if(!lstrcmpi(szDeliverable,"SMTP"))
			smsSend[i].status=STATUS_NOSMTP;
		else {
			szId=GetXMLField(smsSend[i].ack,"sms_response","message_id",NULL);
			if(szId==NULL)
				smsSend[i].status=STATUS_BADACK;
			else {
				free(szId);
				if(szDeliverable==NULL)
					smsSend[i].status=STATUS_BADACK;
				else {
					smsSend[i].status=lstrcmpi(szDeliverable,"Yes")?STATUS_UNDELIVERABLE:STATUS_ACKED;
				}
			}
		}
		if(szDeliverable)
			free(szDeliverable);
		UpdateStatusColumn(i);
		{	DBEVENTINFO dbei={0};
			dbei.cbSize = sizeof(dbei);
			dbei.szModule = "ICQ";
			dbei.timestamp = time(NULL);
			dbei.flags = DBEF_SENT;
			dbei.eventType = ICQEVENTTYPE_SMS;
			dbei.cbBlob = lstrlen(smsSend[i].text)+lstrlen(smsSend[i].number)+2;
			dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
			strcpy((char*)dbei.pBlob,smsSend[i].number);
			strcpy((char*)dbei.pBlob+strlen(smsSend[i].number)+1,smsSend[i].text);
			CallService(MS_DB_EVENT_ADD, (WPARAM)(HANDLE)NULL, (LPARAM)&dbei);
			free(dbei.pBlob);
		}
	}
	else if(ack->result==ACKRESULT_SUCCESS) {
		char *szAckId,*szRcptId;

		szRcptId=GetXMLField((char*)ack->lParam,"sms_delivery_receipt","message_id",NULL);
		if(szRcptId==NULL) return 0;	 //dunno what to do here
		for(i=0;i<smsSendCount;i++) {
			if(smsSend[i].ack==NULL) continue;
			szAckId=GetXMLField(smsSend[i].ack,"sms_response","message_id",NULL);
			if(szAckId==NULL) continue;
			if(!lstrcmp(szAckId,szRcptId)) {
				char *szDelivered;
				free(szAckId);
				free(szRcptId);
				smsSend[i].receipt=_strdup((char*)ack->lParam);
				szDelivered=GetXMLField(smsSend[i].receipt,"sms_delivery_receipt","delivered",NULL);
				if(szDelivered==NULL)
					smsSend[i].status=STATUS_BADRCPT;
				else {
					if(!lstrcmpi(szDelivered,"Yes"))
						smsSend[i].status=STATUS_SUCCESS;
					else
						smsSend[i].status=STATUS_RCPTFAILED;
					free(szDelivered);
				}
				UpdateStatusColumn(i);
				return 0;
			}
			free(szAckId);
		}
		free(szRcptId);
	}
	return 0;
}

int IsSmsAcked(int i)
{
	return smsSend[i].ack!=NULL;
}

int IsSmsRcpted(int i)
{
	return smsSend[i].receipt!=NULL;
}

void GetSmsStartTime(int i,FILETIME *ft)
{
	*ft=smsSend[i].startTime;
}

char *GetSmsText(int i,int type)
{
	switch(type) {
		case 0: return smsSend[i].number;
		case 1: return smsSend[i].text;
		case 2: return smsSend[i].ack;
		case 3: return smsSend[i].receipt;
	}
	return NULL;
}

void InitSmsSend(void)
{
	HookEvent(ME_PROTO_ACK,ProtoAckHook);
}

void UninitSmsSend(void)
{
	int i;
	for(i=0;i<smsSendCount;i++) {
		free(smsSend[i].number);
		free(smsSend[i].text);
		if(smsSend[i].ack) free(smsSend[i].ack);
		if(smsSend[i].receipt) free(smsSend[i].receipt);
	}
	if(smsSendCount) free(smsSend);
}