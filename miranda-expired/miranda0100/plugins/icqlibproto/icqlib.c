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
#include "../../icqlib/icq.h"
#include "resource.h"
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/core/m_system.h"
#include "../../miranda32/protocols/protocols/m_protomod.h"
#include "../../miranda32/protocols/protocols/m_protosvc.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/ui/contactlist/m_clist.h"
#include "../../miranda32/ui/options/m_options.h"
#include <crtdbg.h>
#include "icqproto.h"

BOOL CALLBACK DlgProcIcqNew(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int LoadIcqServices(void);
int LoadIcqRecvServices(void);
void tcp_disengage(void);
void tcp_engage(void);
int IcqOptInit(WPARAM wParam,LPARAM lParam);

icq_Link *plink;
int icqIsOnline,icqStatusMode=ID_STATUS_OFFLINE,icqUsingProxy;
int icqNetTestTimerId,icqConnectTimeoutId,icqConnectRetries;
extern int icqConnectStatusMode;
HANDLE hEventSentEvent,hLogEvent;
int infoQueriesRunning=0,infoQueryQueueSize=0,infoQueryTimeout=0,infoQueryQueueMax=0;
int testTimeoutsCounter;
DWORD *infoQueryQueue=NULL;

VOID CALLBACK IcqCheckDataTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	icq_Main();

	if(infoQueryTimeout) {
		if(--infoQueryTimeout==0) {
			_RPT0(_CRT_WARN,"query timed out\n");
			infoQueriesRunning=0;
			if(infoQueryQueueSize) {
				infoQueriesRunning=1;
				infoQueryTimeout=300;
				_RPT2(_CRT_WARN,"querying %u, %d left in queue\n",infoQueryQueue[0],infoQueryQueueSize);
				icq_SendInfoReq(plink,infoQueryQueue[0]);
				MoveMemory(infoQueryQueue,infoQueryQueue+1,sizeof(DWORD)*--infoQueryQueueSize);
			}
		}
	}
	if(--testTimeoutsCounter==0) {
		testTimeoutsCounter=10;
		icq_HandleTimeout();
	}
}

VOID CALLBACK IcqConnectTimeoutTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	KillTimer(NULL,idEvent);
	icqConnectTimeoutId=0;
	if(icqIsOnline) return;
	icqConnectRetries++;

	ProtoBroadcastAck(ICQPROTONAME,NULL,ACKTYPE_LOGIN,ACKRESULT_FAILED,NULL,LOGINERR_TIMEOUT);
	IcqSetStatus(ID_STATUS_OFFLINE,0);
	KillTimer(NULL,icqNetTestTimerId);
	IcqSetStatus(icqConnectStatusMode,0);
}

static int IcqlibShutdownProc(WPARAM wParam,LPARAM lParam)
{
	if (icqIsOnline) 
	{
		icq_Logout(plink);
		Sleep(100);
		icq_Disconnect(plink);			
	}
	Sleep(200);
	icq_LinkDelete(plink);

	tcp_disengage();	
	if(infoQueryQueue!=NULL) free(infoQueryQueue);
	FreeSequenceIdData();
	return 0;
}

static VOID CALLBACK KeepAliveTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	if(icqIsOnline) icq_KeepAlive(plink);
}

static int IcqlibSettingChanged(WPARAM wParam,LPARAM lParam)
{
	if(strcmp(((DBCONTACTWRITESETTING*)lParam)->szSetting,"UIN")) return 0;
	if(strcmp(((DBCONTACTWRITESETTING*)lParam)->szModule,ICQPROTONAME)) return 0;
	if((HANDLE)wParam==NULL) return 0;
	//a uin is only ever changed when a contact has just been created
	//so we don't need to have hooked db/contact/added
	if((char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0)==NULL) {
		//support for old routines. New stuff should have already done this
		CallService(MS_PROTO_ADDTOCONTACT,wParam,(LPARAM)ICQPROTONAME);
	}
	icq_ContactAdd(plink, ((DBCONTACTWRITESETTING*)lParam)->value.dVal);
	if(icqIsOnline) icq_SendContactList(plink);
	return 0;
}

static int IcqlibContactDeleted(WPARAM wParam,LPARAM lParam)
{
	icq_ContactRemove(plink, DBGetContactSettingDword((HANDLE)wParam,ICQPROTONAME,"UIN",0));
	if(icqIsOnline) icq_SendContactList(plink);
	return 0;
}

int LoadIcqlibModule(void)
{
	DBVARIANT dbv;
	DWORD uin;
	PROTOCOLDESCRIPTOR pd;

	HookEvent(ME_OPT_INITIALISE,IcqOptInit);
	if(!DBGetContactSettingByte(NULL,ICQPROTONAME,"Enable",0)) return 0;

	ZeroMemory(&pd,sizeof(pd));
	pd.cbSize=sizeof(pd);
	pd.szName=ICQPROTONAME;
	pd.type=PROTOTYPE_PROTOCOL;
	CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);

	HookEvent(ME_SYSTEM_SHUTDOWN,IcqlibShutdownProc);
	HookEvent(ME_DB_CONTACT_SETTINGCHANGED,IcqlibSettingChanged);  //this is the 'contact added' hook
	HookEvent(ME_DB_CONTACT_DELETED,IcqlibContactDeleted);
	hLogEvent=CreateHookableEvent(ME_ICQ_LOG);
	hEventSentEvent=CreateHookableEvent(ME_ICQ_EVENTSENT);
	LoadIcqServices();
	//GEN INIT & ICQLIB INIT
	tcp_engage();

	//fire up keepalive timer
	SetTimer(NULL,0,120000,KeepAliveTimer);

	uin=DBGetContactSettingDword(NULL,ICQPROTONAME,"UIN",0);
	{	char *password;
		DBVARIANT dbvPass;
		DBGetContactSetting(NULL,ICQPROTONAME,"Password",&dbvPass);
		CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbvPass.pszVal)+1,(LPARAM)dbvPass.pszVal);
		password=dbvPass.pszVal;
		if(DBGetContactSetting(NULL,ICQPROTONAME,"Nick",&dbv))
			plink=icq_LinkNew(uin, password, "Miranda", 1);
		else {
			plink=icq_LinkNew(uin, password, dbv.pszVal, 1);
			DBFreeVariant(&dbv);
		}
		DBFreeVariant(&dbvPass);
	}

	LoadIcqRecvServices();

	icqUsingProxy=DBGetContactSettingByte(NULL,ICQPROTONAME,"UseProxy",0);
	if(icqUsingProxy) {
		char *host,*user,*password;
		WORD port;
		BYTE useAuth;
		DBVARIANT dbvHost={0},dbvUser={0},dbvPass={0};

		if(DBGetContactSetting(NULL,ICQPROTONAME,"ProxyHost",&dbvHost)) {host=NULL; icqUsingProxy=0;}
		else host=dbvHost.pszVal;
		port=DBGetContactSettingWord(NULL,ICQPROTONAME,"ProxyPort",1080);
		useAuth=DBGetContactSettingByte(NULL,ICQPROTONAME,"ProxyAuth",0);
		if(useAuth) {
			if(DBGetContactSetting(NULL,ICQPROTONAME,"ProxyUser",&dbvUser)) {user=""; useAuth=0;}
			else user=dbvUser.pszVal;
			if(DBGetContactSetting(NULL,ICQPROTONAME,"ProxyPassword",&dbvPass)) {password=""; useAuth=0;}
			else {
				CallService(MS_DB_CRYPT_DECODESTRING,strlen(dbv.pszVal)+1,(LPARAM)dbv.pszVal);
				password=dbvPass.pszVal;
			}
		}
		else {
			user="";
			password="";
		}
		if(icqUsingProxy) icq_SetProxy(plink,host,port,useAuth,user,password);
		DBFreeVariant(&dbvHost);
		DBFreeVariant(&dbvUser);
		DBFreeVariant(&dbvPass);
	}

	//add contacts & convert pre-protoplug modules to use ICQ
	{	HANDLE hContact;
		DWORD uin;
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
		while(hContact!=NULL) {
			if(uin=DBGetContactSettingDword(hContact,ICQPROTONAME,"UIN",0)) {
				if(NULL==(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0))
					CallService(MS_PROTO_ADDTOCONTACT,(WPARAM)hContact,(LPARAM)ICQPROTONAME);
				DBWriteContactSettingWord(hContact,ICQPROTONAME,"Status",ID_STATUS_OFFLINE);
				icq_ContactAdd(plink,uin);
			}
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
		}
	}
	return 0;
}

void tcp_engage(void)
{
	WSADATA wsadata;

	WSAStartup(MAKEWORD(1,1),&wsadata);

	icq_LogLevel = ICQ_LOG_MESSAGE;
}

void tcp_disengage(void)
{	
	WSACleanup();
}