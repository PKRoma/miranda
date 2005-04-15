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
#include "m_clui.h"
#include "clist.h"

#define TRAYICON_ID_BASE    100
#define TIM_CALLBACK   (WM_USER+1857)
#define TIM_CREATE     (WM_USER+1858)

int EventsProcessTrayDoubleClick(void);
int ShowHide(WPARAM wParam,LPARAM lParam);
static VOID CALLBACK TrayCycleTimerProc(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime);

extern HIMAGELIST hCListImages;
extern int currentStatusMenuItem,currentDesiredStatusMode;
extern BOOL (WINAPI *MySetProcessWorkingSetSize)(HANDLE,SIZE_T,SIZE_T);
extern int GetProtoIndexByPos(PROTOCOLDESCRIPTOR ** proto, int protoCnt, int Pos);
extern int GetProtocolVisibility(char * ProtoName);


static UINT WM_TASKBARCREATED;
static int cycleTimerId=0,cycleStep=0;
static int RefreshTimerId=0;   /////by FYR
static VOID CALLBACK RefreshTimerProc(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime); ///// by FYR
void TrayIconUpdateBase(const char *szChangedProto);///by FYR

struct trayIconInfo_t {
	int id;
	char *szProto;
	HICON hBaseIcon;
	int isBase;
};
static struct trayIconInfo_t *trayIcon=NULL;
static int trayIconCount;

// don't move to win2k.h, need new and old versions to work on 9x/2000/XP
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010
typedef struct {
        DWORD cbSize;
        HWND hWnd;
        UINT uID;
        UINT uFlags;
        UINT uCallbackMessage;
        HICON hIcon;
        CHAR   szTip[128];
        DWORD dwState;
        DWORD dwStateMask;
        CHAR   szInfo[256];
        union {
            UINT  uTimeout;
            UINT  uVersion;
        };
        CHAR   szInfoTitle[64];
        DWORD dwInfoFlags;
} NOTIFYICONDATA_NEW;

typedef struct _DllVersionInfo
{
        DWORD cbSize;
        DWORD dwMajorVersion;                   // Major version
        DWORD dwMinorVersion;                   // Minor version
        DWORD dwBuildNumber;                    // Build number
        DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT
typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

static DLLVERSIONINFO dviShell;

static char *TrayIconMakeTooltip(const char *szPrefix,const char *szProto)
{
	static char szTip[128];
	char szProtoName[32];
	char *szStatus, *szSeparator;

	szSeparator=(IsWinVerMEPlus())?szSeparator="\n":" | ";

	if(szProto==NULL) {
		PROTOCOLDESCRIPTOR **protos;
		int count,netProtoCount,i;
		CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
		for(i=0,netProtoCount=0;i<count;i++) if(protos[i]->type==PROTOTYPE_PROTOCOL && (GetProtocolVisibility(protos[i]->szName)!=0) ) netProtoCount++;
		if(netProtoCount==1)
			for(i=0;i<count;i++) if(protos[i]->type==PROTOTYPE_PROTOCOL && (GetProtocolVisibility(protos[i]->szName)!=0)) return TrayIconMakeTooltip(szPrefix,protos[i]->szName);
		if(szPrefix && szPrefix[0]) { 
			lstrcpyn(szTip,szPrefix,sizeof(szTip));
			if(!DBGetContactSettingByte(NULL,"CList","AlwaysStatus",SETTING_ALWAYSSTATUS_DEFAULT))
				return szTip;
		}
		else szTip[0]='\0';
		szTip[sizeof(szTip)-1]='\0';
		for(i=count-1;i>=0;i--) {
			if(protos[i]->type!=PROTOTYPE_PROTOCOL || (GetProtocolVisibility(protos[i]->szName)==0)) continue;
			CallProtoService(protos[i]->szName,PS_GETNAME,sizeof(szProtoName),(LPARAM)szProtoName);
			szStatus=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0),0);
			if(szTip[0]) strncat(szTip,szSeparator,sizeof(szTip)-1-strlen(szTip));
			strncat(szTip,szProtoName,sizeof(szTip)-1-strlen(szTip));
			strncat(szTip," ",sizeof(szTip)-1-strlen(szTip));
			strncat(szTip,szStatus,sizeof(szTip)-1-strlen(szTip));
		}
	}
	else {
		CallProtoService(szProto,PS_GETNAME,sizeof(szProtoName),(LPARAM)szProtoName);
		szStatus=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,CallProtoService(szProto,PS_GETSTATUS,0,0),0);
		if(szPrefix && szPrefix[0]) {
			if(DBGetContactSettingByte(NULL,"CList","AlwaysStatus",SETTING_ALWAYSSTATUS_DEFAULT))
				_snprintf(szTip,sizeof(szTip),"%s%s%s %s",szPrefix,szSeparator,szProtoName,szStatus);
			else lstrcpyn(szTip,szPrefix,sizeof(szTip));
		}
		else _snprintf(szTip,sizeof(szTip),"%s %s",szProtoName,szStatus);
	}
	return szTip;
}

static int TrayIconAdd(HWND hwnd,const char *szProto,const char *szIconProto,int status)
{
	NOTIFYICONDATA nid={0};
	NOTIFYICONDATA_NEW nidn={0};
	int i;

	for(i=0;i<trayIconCount;i++) if(trayIcon[i].id==0) break;
	trayIcon[i].id=TRAYICON_ID_BASE+i;
	trayIcon[i].szProto=(char*)szProto;

	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	nid.uID = trayIcon[i].id;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = TIM_CALLBACK;
	trayIcon[i].hBaseIcon=ImageList_GetIcon(hCListImages,IconFromStatusMode(szIconProto?szIconProto:trayIcon[i].szProto,status),ILD_NORMAL);
	nid.hIcon = trayIcon[i].hBaseIcon;	

	if (dviShell.dwMajorVersion>=5) {
		nidn.cbSize=sizeof(nidn);
		nidn.hWnd=nid.hWnd;
		nidn.uID=nid.uID;
		nidn.uFlags=nid.uFlags|NIF_INFO;
		nidn.uCallbackMessage=nid.uCallbackMessage;
		nidn.hIcon=nid.hIcon;
		lstrcpyn(nidn.szTip, TrayIconMakeTooltip(NULL,trayIcon[i].szProto),sizeof(nidn.szTip));
		Shell_NotifyIcon(NIM_ADD, (void*)&nidn);
	} else {
		lstrcpyn(nid.szTip, TrayIconMakeTooltip(NULL,trayIcon[i].szProto),sizeof(nid.szTip));
		Shell_NotifyIcon(NIM_ADD, &nid);
	}
	trayIcon[i].isBase=1;
	return i;
}

static void TrayIconRemove(HWND hwnd,const char *szProto)
{
	int i;
	NOTIFYICONDATA nid={0};
	NOTIFYICONDATA_NEW nidn={0};

	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		if(lstrcmp(szProto,trayIcon[i].szProto)) continue;
		if (dviShell.dwMajorVersion>=5) {
			nidn.cbSize=sizeof(nidn);
			nidn.hWnd=nid.hWnd;
			nidn.uID=trayIcon[i].id;
			Shell_NotifyIcon(NIM_DELETE, (void*)&nidn);
		} else {
			nid.uID = trayIcon[i].id;
			Shell_NotifyIcon(NIM_DELETE, &nid);
		}		
		DestroyIcon(trayIcon[i].hBaseIcon);
		trayIcon[i].id=0;
		break;
	}
}

static int TrayIconInit(HWND hwnd)
{
	int count,netProtoCount,i;
	int averageMode=0;
	PROTOCOLDESCRIPTOR **protos;

	if(cycleTimerId) {KillTimer(NULL,cycleTimerId); cycleTimerId=0;}
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	for(i=0,netProtoCount=0;i<count;i++) {
		if(protos[i]->type!=PROTOTYPE_PROTOCOL || (GetProtocolVisibility(protos[i]->szName)==0)) continue;
		cycleStep=i;
		netProtoCount++;
		if(averageMode==0) averageMode=CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0);
		else if(averageMode!=CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0)) {averageMode=-1; break;}
	}
	trayIconCount=count;
	trayIcon=(struct trayIconInfo_t*)mir_calloc(sizeof(struct trayIconInfo_t),count);
	if(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_MULTI &&
	  (averageMode<=0 || DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT))) {
		int i;
		for(i=count-1;i>=0;i--) 
		{	
			int j;
			j=GetProtoIndexByPos(protos,count,i);
			if (j>-1)
				if(protos[j]->type==PROTOTYPE_PROTOCOL && (GetProtocolVisibility(protos[j]->szName)!=0)) TrayIconAdd(hwnd,protos[j]->szName,NULL,CallProtoService(protos[j]->szName,PS_GETSTATUS,0,0));
		}
	}
	else if(averageMode<=0 && DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_SINGLE) {
		DBVARIANT dbv={DBVT_DELETED};
		char *szProto;
		if(DBGetContactSetting(NULL,"CList","PrimaryStatus",&dbv)) szProto=NULL;
		else szProto=dbv.pszVal;
		TrayIconAdd(hwnd,NULL,szProto,szProto?CallProtoService(szProto,PS_GETSTATUS,0,0):CallService(MS_CLIST_GETSTATUSMODE,0,0));
		DBFreeVariant(&dbv);
	}
	else TrayIconAdd(hwnd,NULL,NULL,averageMode);
	if(averageMode<=0 && DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_CYCLE)
		cycleTimerId=SetTimer(NULL,0,DBGetContactSettingWord(NULL,"CList","CycleTime",SETTING_CYCLETIME_DEFAULT)*1000,TrayCycleTimerProc);
	return 0;
}

static void TrayIconDestroy(HWND hwnd)
{
	NOTIFYICONDATA nid={0};
	int i;

	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		nid.uID = trayIcon[i].id;
		Shell_NotifyIcon(NIM_DELETE, &nid);
		DestroyIcon(trayIcon[i].hBaseIcon);
	}
	if (trayIcon) mir_free(trayIcon);
	trayIcon=NULL;
	trayIconCount=0;
}

//called when Explorer crashes and the taskbar is remade
static void TrayIconTaskbarCreated(HWND hwnd)
{
	TrayIconDestroy(hwnd);
	TrayIconInit(hwnd);
}

static int TrayIconUpdate(HICON hNewIcon,const char *szNewTip,const char *szPreferredProto, int isBase)
{
	NOTIFYICONDATA nid={0};
	NOTIFYICONDATA_NEW nidn={0};
	int i;

	nid.cbSize = sizeof(nid);
	nid.hWnd = (HWND)CallService(MS_CLUI_GETHWND,0,0);
	nid.uFlags = NIF_ICON | NIF_TIP;
	nid.hIcon = hNewIcon;			

	nidn.cbSize=sizeof(nidn);
	nidn.hWnd=nid.hWnd;
	nidn.uFlags=NIF_ICON|NIF_TIP;
	nidn.hIcon=nid.hIcon;

	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		if(lstrcmp(trayIcon[i].szProto,szPreferredProto)) continue;
		nid.uID = trayIcon[i].id;
		if (dviShell.dwMajorVersion>=5) {
			nidn.uID=nid.uID;
			lstrcpyn(nidn.szTip,TrayIconMakeTooltip(szNewTip,trayIcon[i].szProto),sizeof(nidn.szTip));
			Shell_NotifyIcon(NIM_MODIFY, (void*)&nidn);
		} else {
			lstrcpyn(nid.szTip,TrayIconMakeTooltip(szNewTip,trayIcon[i].szProto),sizeof(nid.szTip));
			Shell_NotifyIcon(NIM_MODIFY, &nid);
		}
		trayIcon[i].isBase=isBase;
		return i;
	}

	for(i=0;i<trayIconCount;i++) 
	{
		if(trayIcon[i].id==0) continue;
		nid.uID = trayIcon[i].id;
		if (dviShell.dwMajorVersion>=5) {
			nidn.uID=nid.uID;
			lstrcpyn(nidn.szTip,TrayIconMakeTooltip(szNewTip,trayIcon[i].szProto),sizeof(nidn.szTip));
			Shell_NotifyIcon(NIM_MODIFY, (void*)&nidn);
		} else {
			lstrcpyn(nid.szTip,TrayIconMakeTooltip(szNewTip,trayIcon[i].szProto),sizeof(nid.szTip));
			Shell_NotifyIcon(NIM_MODIFY, &nid);
		}
		trayIcon[i].isBase=isBase;
		if(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI ) 
		{
			if(RefreshTimerId) {KillTimer(NULL,RefreshTimerId); RefreshTimerId=0;}
			RefreshTimerId=SetTimer(NULL,0,DBGetContactSettingWord(NULL,"CList","CycleTime",SETTING_CYCLETIME_DEFAULT)*1000,RefreshTimerProc);	// by FYR (Setting timer)
			return i;	//by FYR (to change only one icon)
		}		
	}
	return -1;
}

////////////////////////////////////////////////////////////
///// Need to refresh trays icon  after timely changing/////
////////////////////////////////////////////////////////////
static VOID CALLBACK RefreshTimerProc(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	int count,i;
	PROTOCOLDESCRIPTOR **protos;
	
	if(RefreshTimerId) {KillTimer(NULL,RefreshTimerId); RefreshTimerId=0;}

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	for (i=0; i<count; i++)
		if(protos[i]->type==PROTOTYPE_PROTOCOL &&
			(GetProtocolVisibility(protos[i]->szName)!=0))
			TrayIconUpdateBase(protos[i]->szName);

}
//////// End by FYR /////////


static int TrayIconSetBaseInfo(HICON hIcon,const char *szPreferredProto)
{
	int i;

	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		if(lstrcmp(trayIcon[i].szProto,szPreferredProto)) continue;
		DestroyIcon(trayIcon[i].hBaseIcon);
		trayIcon[i].hBaseIcon=hIcon;
		return i;
	}
	//if there wasn't a specific icon, there will only be one suitable
	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		DestroyIcon(trayIcon[i].hBaseIcon);
		trayIcon[i].hBaseIcon=hIcon;
		return i;
	}
	return -1;
}

void TrayIconUpdateWithImageList(int iImage,const char *szNewTip,char *szPreferredProto)
{
	HICON hIcon;

	hIcon=ImageList_GetIcon(hCListImages,iImage,ILD_NORMAL);
	TrayIconUpdate(hIcon,szNewTip,szPreferredProto,0);
	DestroyIcon(hIcon);
}


static VOID CALLBACK TrayCycleTimerProc(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	int count;
	PROTOCOLDESCRIPTOR **protos;

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	for(cycleStep++;;cycleStep++) {
		if(cycleStep>=count) cycleStep=0;
		if(protos[cycleStep]->type==PROTOTYPE_PROTOCOL && (GetProtocolVisibility(protos[cycleStep]->szName)!=0)) break;
	}
	DestroyIcon(trayIcon[0].hBaseIcon);
	trayIcon[0].hBaseIcon=ImageList_GetIcon(hCListImages,IconFromStatusMode(protos[cycleStep]->szName,CallProtoService(protos[cycleStep]->szName,PS_GETSTATUS,0,0)),ILD_NORMAL);
	if(trayIcon[0].isBase)
		TrayIconUpdate(trayIcon[0].hBaseIcon,NULL,NULL,1);
}

void TrayIconUpdateBase(const char *szChangedProto)
{
	int i,count,netProtoCount,changed=-1;
	PROTOCOLDESCRIPTOR **protos;
	int averageMode=0;
	HWND hwnd=(HWND)CallService(MS_CLUI_GETHWND,0,0);
	if(cycleTimerId) {KillTimer(NULL,cycleTimerId); cycleTimerId=0;}
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	for(i=0,netProtoCount=0;i<count;i++) {
		if(protos[i]->type!=PROTOTYPE_PROTOCOL || (GetProtocolVisibility(protos[i]->szName)==0)) continue;
		netProtoCount++;
		if(!lstrcmp(szChangedProto,protos[i]->szName)) cycleStep=i;
		if(averageMode==0) averageMode=CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0);
		else if(averageMode!=CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0)) {averageMode=-1; break;}
	}
	if(netProtoCount>1) {
		if(averageMode>0) {
			if(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_MULTI) {
				if(DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT))
					changed=TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages,IconFromStatusMode(szChangedProto,averageMode),ILD_NORMAL),szChangedProto);
				else if(trayIcon && trayIcon[0].szProto!=NULL) {
					TrayIconDestroy(hwnd);
					TrayIconInit(hwnd);
				}
				else changed=TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages,IconFromStatusMode(NULL,averageMode),ILD_NORMAL),NULL);
			}
			else changed=TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages,IconFromStatusMode(NULL,averageMode),ILD_NORMAL),NULL);
		}
		else {
			switch(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)) {
				case SETTING_TRAYICON_SINGLE:
				{	DBVARIANT dbv={DBVT_DELETED};
					char *szProto;
					int status;
					if(DBGetContactSetting(NULL,"CList","PrimaryStatus",&dbv)) szProto=NULL;
					else szProto=dbv.pszVal;
//					changed=TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages,IconFromStatusMode(szProto,szProto?CallProtoService(szProto,PS_GETSTATUS,0,0):CallService(MS_CLIST_GETSTATUSMODE,0,0)),ILD_NORMAL),NULL);

					status=CallProtoService(szChangedProto,PS_GETSTATUS,0,0);
					if ((DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",1)==1)&&status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
					{
						//
						HICON hIcon;

						hIcon=(HICON)CallService("CLUI/GetConnectingIconForProtocol",(WPARAM)szChangedProto,0);;										
						if (hIcon)
						{
						changed=TrayIconSetBaseInfo(hIcon,NULL);						
						TrayIconUpdate(hIcon,NULL,NULL,1);
						
						DestroyIcon(hIcon);
						DBFreeVariant(&dbv);
						break;
						}
											
						//return;
					}
					else
					{
							changed=TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages,IconFromStatusMode(szProto,szProto?CallProtoService(szProto,PS_GETSTATUS,0,0):CallService(MS_CLIST_GETSTATUSMODE,0,0)),ILD_NORMAL),NULL);
					}



					DBFreeVariant(&dbv);
					break;
				}
				case SETTING_TRAYICON_CYCLE:
					cycleTimerId=SetTimer(NULL,0,DBGetContactSettingWord(NULL,"CList","CycleTime",SETTING_CYCLETIME_DEFAULT)*1000,TrayCycleTimerProc);
					changed=TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages,IconFromStatusMode(szChangedProto,CallProtoService(szChangedProto,PS_GETSTATUS,0,0)),ILD_NORMAL),NULL);
					break;
				case SETTING_TRAYICON_MULTI:
					if (!trayIcon) 
					{
						TrayIconRemove(NULL,NULL);
					} else if(DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT))
						changed=TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages,IconFromStatusMode(szChangedProto,CallProtoService(szChangedProto,PS_GETSTATUS,0,0)),ILD_NORMAL),szChangedProto);
					else {
						TrayIconDestroy(hwnd);
						TrayIconInit(hwnd);
					}
					break;
			}
		}
	}
	else 
	{
//changed=TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages,IconFromStatusMode(NULL,averageMode),ILD_NORMAL),NULL);


					int status=CallProtoService(szChangedProto,PS_GETSTATUS,0,0);
					if ((DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",1)==1)&&status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
					{
						//
						HICON hIcon;

						hIcon=(HICON)CallService("CLUI/GetConnectingIconForProtocol",(WPARAM)szChangedProto,0);;										
						if (hIcon)
						{
						changed=TrayIconSetBaseInfo(hIcon,NULL);						
						TrayIconUpdate(hIcon,NULL,NULL,1);					

						DestroyIcon(hIcon);
						//return;
						}
					}
			//??
				changed=TrayIconSetBaseInfo(ImageList_GetIcon(hCListImages,IconFromStatusMode(NULL,averageMode),ILD_NORMAL),NULL);
	};














	if(changed!=-1 && trayIcon[changed].isBase)
		TrayIconUpdate(trayIcon[changed].hBaseIcon,NULL,szChangedProto/*trayIcon[changed].szProto*/,1);  // by FYR (No suitable protocol)
}

void TrayIconSetToBase(char *szPreferredProto)
{
	int i;

	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		if(lstrcmp(trayIcon[i].szProto,szPreferredProto)) continue;
		TrayIconUpdate(trayIcon[i].hBaseIcon,NULL,szPreferredProto,1);
		return;
	}
	//if there wasn't a specific icon, there will only be one suitable
	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		TrayIconUpdate(trayIcon[i].hBaseIcon,NULL,szPreferredProto,1);
		return;
	}
}

void TrayIconIconsChanged(void)
{
	HWND hwnd=(HWND)CallService(MS_CLUI_GETHWND,0,0);
	TrayIconDestroy(hwnd);
	TrayIconInit(hwnd);
}

static int autoHideTimerId;
static VOID CALLBACK TrayIconAutoHideTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	HWND hwndClui;
	KillTimer(hwnd,idEvent);
	hwndClui=(HWND)CallService(MS_CLUI_GETHWND,0,0);
	if(GetActiveWindow()==hwndClui) return;
	ShowWindow(hwndClui,SW_HIDE);
	if(MySetProcessWorkingSetSize!=NULL)
		MySetProcessWorkingSetSize(GetCurrentProcess(),-1,-1);
}

int TrayIconPauseAutoHide(WPARAM wParam,LPARAM lParam)
{
	if(DBGetContactSettingByte(NULL,"CList","AutoHide",SETTING_AUTOHIDE_DEFAULT)) {
		if(GetActiveWindow()!=(HWND)CallService(MS_CLUI_GETHWND,0,0)) {
			KillTimer(NULL,autoHideTimerId);
			autoHideTimerId=SetTimer(NULL,0,1000*DBGetContactSettingWord(NULL,"CList","HideTime",SETTING_HIDETIME_DEFAULT),TrayIconAutoHideTimer);
		}
	}
	return 0;
}

int TrayIconProcessMessage(WPARAM wParam,LPARAM lParam)
{
	MSG *msg=(MSG*)wParam;
	switch(msg->message) {
		case WM_CREATE: {			
			WM_TASKBARCREATED=RegisterWindowMessage("TaskbarCreated");
			PostMessage(msg->hwnd,TIM_CREATE,0,0);			
			break;
		}
		case WM_DRAWITEM:
			return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);
			break;
		case WM_MEASUREITEM:
			return CallService(MS_CLIST_MENUMEASUREITEM,wParam,lParam);
			break;
		case TIM_CREATE:
			TrayIconInit(msg->hwnd);
			break;
		case WM_ACTIVATE:
			if(DBGetContactSettingByte(NULL,"CList","AutoHide",SETTING_AUTOHIDE_DEFAULT)) {
				if(LOWORD(msg->wParam)==WA_INACTIVE)
					autoHideTimerId=SetTimer(NULL,0,1000*DBGetContactSettingWord(NULL,"CList","HideTime",SETTING_HIDETIME_DEFAULT),TrayIconAutoHideTimer);
				else KillTimer(NULL,autoHideTimerId);
			}
			break;
		case WM_DESTROY:
			TrayIconDestroy(msg->hwnd);
			break;
		case TIM_CALLBACK:
			if (msg->lParam==WM_MBUTTONUP)
			{
				ShowHide(0,0);				
			}
			else if (msg->lParam==(DBGetContactSettingByte(NULL,"CList","Tray1Click",SETTING_TRAY1CLICK_DEFAULT)?WM_LBUTTONUP:WM_LBUTTONDBLCLK))
			{
				if ((GetAsyncKeyState(VK_CONTROL)&0x8000)) ShowHide(0,0);
				else {
					if(EventsProcessTrayDoubleClick()) ShowHide(0,0);
				}				
			}
			else if (msg->lParam == WM_RBUTTONUP)
			{
				POINT pt;	
				HMENU hMenu;
				/*
				
				MENUITEMINFO mi;
				
				
				hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_CONTEXT));
				hMenu = GetSubMenu(hMenu,0);
				CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hMenu,0);
				
				ZeroMemory(&mi,sizeof(mi));
				mi.cbSize=MENUITEMINFO_V4_SIZE;
				mi.fMask=MIIM_SUBMENU|MIIM_TYPE;
				mi.fType=MFT_STRING;
				mi.hSubMenu=(HMENU)CallService(MS_CLIST_MENUGETMAIN,0,0);
				mi.dwTypeData=Translate("&Main Menu");
				InsertMenuItem(hMenu,1,TRUE,&mi);
				mi.hSubMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);
				mi.dwTypeData=Translate("&Status");
				InsertMenuItem(hMenu,2,TRUE,&mi);
				SetMenuDefaultItem(hMenu,ID_TRAY_HIDE,FALSE);
				*/
				hMenu=(HMENU)CallService(MS_CLIST_MENUBUILDTRAY,(WPARAM)0,(LPARAM)0);

				SetForegroundWindow(msg->hwnd);
				SetFocus(msg->hwnd);
				GetCursorPos(&pt);
				TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, msg->hwnd, NULL);
			}
			*((LRESULT*)lParam)=0;
			return TRUE;
		default:
			if(msg->message==WM_TASKBARCREATED) {
				TrayIconTaskbarCreated(msg->hwnd);
				*((LRESULT*)lParam)=0;
				return TRUE;
			}
	}
	
	return FALSE;
}

int CListTrayNotify(WPARAM wParam,LPARAM lParam)
{
	MIRANDASYSTRAYNOTIFY *msn=(MIRANDASYSTRAYNOTIFY*)lParam;
	if (msn && msn->cbSize==sizeof(MIRANDASYSTRAYNOTIFY) && msn->szInfo && msn->szInfoTitle) {
		if (trayIcon) {
			NOTIFYICONDATA_NEW nid={0};
			nid.cbSize=sizeof(nid);
			nid.hWnd=(HWND)CallService(MS_CLUI_GETHWND,0,0);
			if (msn->szProto) {
				int j;
				for (j=0;j<trayIconCount;j++) {
					if (trayIcon[j].szProto != NULL) {
						if (strcmp(msn->szProto,trayIcon[j].szProto)==0) {
							nid.uID=trayIcon[j].id;
							j=trayIconCount;
							continue;
						}
					} else {
						if (trayIcon[j].isBase) {
							nid.uID=trayIcon[j].id;
							j=trayIconCount;
							continue;
						}
					}
				} //for
			} else {
				nid.uID=trayIcon[0].id;
			}
			nid.uFlags=NIF_INFO;
			lstrcpyn(nid.szInfo,msn->szInfo,sizeof(nid.szInfo));
			lstrcpyn(nid.szInfoTitle,msn->szInfoTitle,sizeof(nid.szInfoTitle));
			nid.uTimeout=msn->uTimeout;
			nid.dwInfoFlags=msn->dwInfoFlags;
			return Shell_NotifyIcon(NIM_MODIFY,(void*)&nid) == 0;
		}
		return 2;
	}
	return 1;
}

//////////////////////////////TRAY MENU/////////////////////////
HANDLE hTrayMenuObject;

HANDLE hTrayMainMenuItemProxy;
HANDLE hHideShowMainMenuItem;
HANDLE hTrayStatusMenuItemProxy;
HANDLE hPreBuildTrayMenuEvent;

//traymenu exec param(ownerdata)
typedef struct{
char *szServiceName;
int Param1;
}TrayMenuExecParam,*lpTrayMenuExecParam;

/*
wparam=handle to the menu item returned by MS_CLIST_ADDCONTACTMENUITEM
return 0 on success.
*/
static int RemoveTrayMenuItem(WPARAM wParam,LPARAM lParam)
{
	CallService(MO_REMOVEMENUITEM,wParam,0);
	return 0;
}


static int BuildTrayMenu(WPARAM wParam,LPARAM lParam)
{
	int tick;
	HMENU hMenu;
	ListParam param;

	memset(&param,0,sizeof(param));
	param.MenuObjectHandle=(int)hTrayMenuObject;
	param.rootlevel=-1;

	//hMenu=hMainMenu;
	hMenu=CreatePopupMenu();
	//hMenu=wParam;
	tick=GetTickCount();
	
	NotifyEventHooks(hPreBuildTrayMenuEvent,0,0);

	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);
	//DrawMenuBar((HWND)CallService("CLUI/GetHwnd",0,0));
	tick=GetTickCount()-tick;
	return (int)hMenu;
}

static int AddTrayMenuItem(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM *mi=(CLISTMENUITEM*)lParam;
	TMO_MenuItem tmi;
	OptParam op;

	if(mi->cbSize!=sizeof(CLISTMENUITEM)) return 0;

	memset(&tmi,0,sizeof(tmi));
	tmi.cbSize=sizeof(tmi);
	tmi.flags=mi->flags;
	tmi.hIcon=mi->hIcon;
	tmi.hotKey=mi->hotKey;
	tmi.pszName=mi->pszName;
	tmi.position=mi->position;

	//pszPopupName for new system mean root level
        //pszPopupName for old system mean that exists popup
	tmi.root=(int)mi->pszPopupName;

	tmi.ownerdata=NULL;
	
	{
		lpTrayMenuExecParam mmep;
		mmep=(lpTrayMenuExecParam)mir_alloc(sizeof(TrayMenuExecParam));
		if(mmep==NULL){return(0);};
		
		//we need just one parametr.
		mmep->szServiceName=mir_strdup(mi->pszService);
		mmep->Param1=mi->popupPosition;
		
		tmi.ownerdata=mmep;
	}
	op.Handle=CallService(MO_ADDNEWMENUITEM,(WPARAM)hTrayMenuObject,(LPARAM)&tmi);
	op.Setting=OPT_MENUITEMSETUNIQNAME;
	op.Value=(int)mi->pszService;
	CallService(MO_SETOPTIONSMENUITEM,(WPARAM)0,(LPARAM)&op);
	return(op.Handle);

//	mainItemCount++;
//	return MENU_CUSTOMITEMMAIN|(mainMenuItem[mainItemCount-1].id);
}

int TrayMenuCheckService(WPARAM wParam,LPARAM lParam) {
//not used
	return(0);
};

int TrayMenuonAddService(WPARAM wParam,LPARAM lParam) {

	MENUITEMINFO *mii=(MENUITEMINFO* )wParam;
	if (mii==NULL) return 0;

	if (hHideShowMainMenuItem==(HANDLE)lParam)
	{
		mii->fMask|=MIIM_STATE;
		mii->fState|=MFS_DEFAULT;

	}
	if (hTrayMainMenuItemProxy==(HANDLE)lParam)
	{
				mii->fMask|=MIIM_SUBMENU;
				//mi.fType=MFT_STRING;
				mii->hSubMenu=(HMENU)CallService(MS_CLIST_MENUGETMAIN,0,0);		
	}

	if (hTrayStatusMenuItemProxy==(HANDLE)lParam)
	{
				mii->fMask|=MIIM_SUBMENU;
				//mi.fType=MFT_STRING;
				mii->hSubMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);		
	}
	
	return(TRUE);
};


//called with:
//wparam - ownerdata
//lparam - lparam from winproc
int TrayMenuExecService(WPARAM wParam,LPARAM lParam) {
	if (wParam!=0)
	{
		lpTrayMenuExecParam mmep=(lpTrayMenuExecParam)wParam;	
		if (!strcmp(mmep->szServiceName,"Help/AboutCommand"))
		{
			//bug in help.c,it used wparam as parent window handle without reason.
			mmep->Param1=0;
		};
		CallService(mmep->szServiceName,mmep->Param1,lParam);	
	};
	return(1);
};
int FreeOwnerDataTrayMenu (WPARAM wParam,LPARAM lParam)
{

	lpTrayMenuExecParam mmep;
	mmep=(lpTrayMenuExecParam)lParam;
	if (mmep!=NULL){
		FreeAndNil(&mmep->szServiceName);
		FreeAndNil(&mmep);
	}

return(0);
};

void InitTrayMenus(void)
{
	TMenuParam tmp;
	OptParam op;
	
	CreateServiceFunction("CLISTMENUSTRAY/ExecService",TrayMenuExecService);
	CreateServiceFunction("CLISTMENUSTRAY/FreeOwnerDataTrayMenu",FreeOwnerDataTrayMenu);
	CreateServiceFunction("CLISTMENUSTRAY/TrayMenuonAddService",TrayMenuonAddService);

	CreateServiceFunction(MS_CLIST_ADDTRAYMENUITEM,AddTrayMenuItem);
	CreateServiceFunction(MS_CLIST_REMOVETRAYMENUITEM,RemoveTrayMenuItem);
	CreateServiceFunction(MS_CLIST_MENUBUILDTRAY,BuildTrayMenu);
	hPreBuildTrayMenuEvent=CreateHookableEvent(ME_CLIST_PREBUILDTRAYMENU);



	//Tray menu
	memset(&tmp,0,sizeof(tmp));
	tmp.cbSize=sizeof(tmp);
	tmp.CheckService=NULL;
	tmp.ExecService="CLISTMENUSTRAY/ExecService";
	tmp.name="Tray Menu";
	hTrayMenuObject=(HANDLE)CallService(MO_CREATENEWMENUOBJECT,(WPARAM)0,(LPARAM)&tmp);
	
	
	op.Handle=(int)hTrayMenuObject;
	op.Setting=OPT_USERDEFINEDITEMS;
	op.Value=(int)TRUE;
	CallService(MO_SETOPTIONSMENUOBJECT,(WPARAM)0,(LPARAM)&op);
	
	op.Handle=(int)hTrayMenuObject;
	op.Setting=OPT_MENUOBJECT_SET_FREE_SERVICE;
	op.Value=(int)"CLISTMENUSTRAY/FreeOwnerDataTrayMenu";
	CallService(MO_SETOPTIONSMENUOBJECT,(WPARAM)0,(LPARAM)&op);

	op.Handle=(int)hTrayMenuObject;
	op.Setting=OPT_MENUOBJECT_SET_ONADD_SERVICE;
	op.Value=(int)"CLISTMENUSTRAY/TrayMenuonAddService";
	CallService(MO_SETOPTIONSMENUOBJECT,(WPARAM)0,(LPARAM)&op);

	{	
	//add  exit command to menu
	CLISTMENUITEM mi;

	memset(&mi,0,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=900000;
	mi.pszService="CloseAction";
	mi.pszName=Translate("E&xit");
	AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);

	memset(&mi,0,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=100000;
	mi.pszService=MS_CLIST_SHOWHIDE;
	mi.pszName=Translate("&Hide/Show");
	hHideShowMainMenuItem=(HANDLE)AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);

	memset(&mi,0,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=200000;
	mi.hIcon=LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_FINDUSER));
	mi.pszService="FindAdd/FindAddCommand";
	mi.pszName=Translate("&Find/Add Contacts...");
	AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);
	
	
	
	memset(&mi,0,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=300000;
	mi.pszService="";
	mi.pszName=Translate("&Main Menu");
	hTrayMainMenuItemProxy=(HANDLE)AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);

	memset(&mi,0,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=300100;
	mi.pszService="";
	mi.pszName=Translate("&Status");
	hTrayStatusMenuItemProxy=(HANDLE)AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);

	memset(&mi,0,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=400000;
	mi.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_OPTIONS));
	mi.pszService="Options/OptionsCommand";
	mi.pszName=Translate("&Options...");
	
	AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);


	memset(&mi,0,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=500000;
	mi.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MIRANDA));
	mi.pszService="Help/AboutCommand";
	mi.pszName=Translate("&About");
	
	AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);
	
	};
}

//////////////////////////////END TRAY MENU/////////////////////////

void InitTray(void)
{
	HINSTANCE hLib;

	hLib=LoadLibrary("shell32.dll");
	if (hLib) {
		DLLGETVERSIONPROC proc;
		dviShell.cbSize=sizeof(dviShell);
		proc=(DLLGETVERSIONPROC)GetProcAddress(hLib,"DllGetVersion");
		if (proc) {
			proc(&dviShell);
		}
		FreeLibrary(hLib);
	}
	if (dviShell.dwMajorVersion>=5) {
		CreateServiceFunction(MS_CLIST_SYSTRAY_NOTIFY,CListTrayNotify);
	}
	
	InitTrayMenus();
	return;
}