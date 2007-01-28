/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, 
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
#include "commonprototypes.h"

#define TRAYICON_ID_BASE    100
#define TIM_CALLBACK   (WM_USER+1857)
#define TIM_CREATE     (WM_USER+1858)

int cliShowHide(WPARAM wParam,LPARAM lParam);
int g_mutex_bOnTrayRightClick=0;
static VOID CALLBACK TrayCycleTimerProc(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime);


#include "modern_statusbar.h"

BOOL g_bMultiConnectionMode=FALSE;
static UINT WM_TASKBARCREATED;
static int cycleTimerId=0,cycleStep=0;
static int RefreshTimerId=0;   /////by FYR
static VOID CALLBACK RefreshTimerProc(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime); ///// by FYR
void cliTrayIconUpdateBase(char *szChangedProto);///by FYR
BOOL IS_WM_MOUSE_DOWN_IN_TRAY;
struct trayIconInfo_t {
	int id;
	char *szProto;
	HICON hBaseIcon;
	int isBase;
	TCHAR * iconTip;
};
static struct trayIconInfo_t *trayIcon=NULL;
static int trayIconCount;

BOOL g_trayTooltipActive = FALSE;
POINT tray_hover_pos = {0};

// don't move to win2k.h, need new and old versions to work on 9x/2000/XP
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010

typedef struct _DllVersionInfo {
	DWORD cbSize;
	DWORD dwMajorVersion;                   // Major version
	DWORD dwMinorVersion;                   // Minor version
	DWORD dwBuildNumber;                    // Build number
	DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT
typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

void mir_strset(TCHAR ** dest, TCHAR *source)
{
	if (*dest) mir_free_and_nill(*dest);
	if (source) *dest=mir_tstrdup(source);
}

static DLLVERSIONINFO dviShell;
BOOL g_MultiConnectionMode=FALSE;
char * g_szConnectingProto=NULL;
static BOOL g_trayMenuOnScreen=FALSE;
int GetStatusVal(int status)
{
	switch(status)
	{
	case ID_STATUS_OFFLINE:               return 50;
	case ID_STATUS_ONLINE:                return 100;
	case ID_STATUS_FREECHAT:              return 110;
	case ID_STATUS_INVISIBLE:             return 120;
	case ID_STATUS_AWAY:                  return 200;
	case ID_STATUS_DND:					  return 210;
	case ID_STATUS_NA:                    return 220;
	case ID_STATUS_OCCUPIED:              return 230;
	case ID_STATUS_ONTHEPHONE:            return 400;
	case ID_STATUS_OUTTOLUNCH:            return 410;	
	}
	if (status<ID_STATUS_OFFLINE && status>0) return 600; //connecting is most priority
	return 0;
}

int GetStatusOrder(int currentStatus, int newStatus)
{
	int current=GetStatusVal(currentStatus);
	int newstat=GetStatusVal(newStatus);
	return (current>newstat)?currentStatus:newStatus;
}

int CListTray_GetGlobalStatus(WPARAM wparam,LPARAM lparam)
{
	int curstatus=0;
	int i;
	int connectingCount=0;
	for (i=0;i<pcli->hClcProtoCount;i++) 
	{
		if(!pcli->pfnGetProtocolVisibility(pcli->clcProto[i].szProto)) continue;
		if (pcli->clcProto[i].dwStatus>=ID_STATUS_CONNECTING &&
			pcli->clcProto[i].dwStatus<ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
		{
			connectingCount++;
			if (connectingCount==1) g_szConnectingProto=pcli->clcProto[i].szProto;
		}
		curstatus=GetStatusOrder(curstatus,pcli->clcProto[i].dwStatus);
	}
	if (connectingCount==0)
	{
		//g_szConnectingProto=NULL;
		g_bMultiConnectionMode=FALSE;
	}
	else if (connectingCount>1) 
		g_bMultiConnectionMode=TRUE;
	else 
		g_bMultiConnectionMode=FALSE;
	return curstatus?curstatus:ID_STATUS_OFFLINE;
}

static BOOL mToolTipTrayTips = FALSE;

static TCHAR szTip[2048];
static TCHAR* TrayIconMakeTooltip(const TCHAR *szPrefix, const char *szProto)
{
	char szProtoName[32];
	TCHAR *szStatus, *szSeparator, *sztProto;
	int tipSize,t,cn;

	if(ServiceExists("mToolTip/ShowTip"))
		tipSize = 2048;
	else
		tipSize = 128;

	if(!mToolTipTrayTips)
		szSeparator = (IsWinVerMEPlus()) ? szSeparator = _T("\n") : _T(" | ");
	else
		szSeparator = _T("\n");

	if (szProto == NULL) 
    {
		PROTOCOLDESCRIPTOR **protos;
		int count, netProtoCount, i;
		CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) &count, (LPARAM) &protos);
		for (i = 0,netProtoCount = 0; i < count; i++) {
			if (protos[i]->type == PROTOTYPE_PROTOCOL)
				netProtoCount++;
		}
		if (netProtoCount == 1)
			for (i = 0; i < count; i++) {
				if (protos[i]->type == PROTOTYPE_PROTOCOL)
					return TrayIconMakeTooltip(szPrefix, protos[i]->szName);
			}
			if (szPrefix && szPrefix[0]) {
				lstrcpyn(szTip, szPrefix, tipSize);
				if (!DBGetContactSettingByte(NULL, "CList", "AlwaysStatus", SETTING_ALWAYSSTATUS_DEFAULT))
					return szTip;
			} else
				szTip[0] = '\0';
			szTip[tipSize - 1] = '\0';

			t=0;
			cn=DBGetContactSettingDword(NULL,"Protocols","ProtoCount",-1);
			if (cn==-1)
				return NULL;

			for(t=0;t<cn;t++) {
				TCHAR *ProtoXStatus=NULL;
				i=pcli->pfnGetProtoIndexByPos(protos, count,t);
				if (i==-1) return TEXT("???");
//			for (i = count - 1; i >= 0; i--) {
				if (protos[i]->type != PROTOTYPE_PROTOCOL || !pcli->pfnGetProtocolVisibility(protos[i]->szName))
					continue;
				// GetXStatus
				{			
					int xstatus=0;
					if (CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0)>ID_STATUS_OFFLINE /*&& (DBGetContactSettingByte(NULL,"CLUI","XStatusTray",15)&3)*/ )	
					{
						char str[MAXMODULELABELLENGTH];
						strcpy(str,protos[i]->szName);
						strcat(str,"/GetXStatus");
						if (ServiceExists(str))
						{
							char * dbTitle="XStatusName";
							char * dbTitle2=NULL;
							xstatus=CallProtoService(protos[i]->szName,"/GetXStatus",(WPARAM)&dbTitle,(LPARAM)&dbTitle2);
							if (dbTitle && xstatus)
							{
								DBVARIANT dbv={0};
								if (!DBGetContactSettingTString(NULL,protos[i]->szName,dbTitle,&dbv))
								{
									if (dbv.ptszVal[0]!=(TCHAR)0)
										ProtoXStatus=mir_tstrdup(dbv.ptszVal);
									//mir_free_and_nill(dbv.ptszVal);
									DBFreeVariant(&dbv);
								}
							}
						}
					}
				}
				//_DEBUG
					//ProtoXStatus=mir_tstrdup(_T("XStatus test name"));
				CallProtoService(protos[i]->szName, PS_GETNAME, sizeof(szProtoName), (LPARAM) szProtoName);
				szStatus = pcli->pfnGetStatusModeDescription(CallProtoService(protos[i]->szName, PS_GETSTATUS, 0, 0), 0);
				if(szStatus) {
					if(mToolTipTrayTips) {
						TCHAR tipline[256];
						#if defined( _UNICODE )
						{	TCHAR* p = a2u( szProtoName );
							mir_sntprintf(tipline, 256, _T("<b>%-12.12s</b>\t%s"), p, szStatus);
							mir_free_and_nill( p );
						}
						#else
							mir_sntprintf(tipline, 256, _T("<b>%-12.12s</b>\t%s"), szProtoName, szStatus);
						#endif
						if (szTip[0])
							_tcsncat(szTip, szSeparator, tipSize - 1 - _tcslen(szTip));
						_tcsncat(szTip, tipline, tipSize - 1 - _tcslen(szTip));
						if (ProtoXStatus)
						{
							TCHAR tipline[256];
							mir_sntprintf(tipline, 256, _T("%-24.24s\n"), ProtoXStatus);
							if (szTip[0])
								_tcsncat(szTip, szSeparator, tipSize - 1 - _tcslen(szTip));
							_tcsncat(szTip, tipline, tipSize - 1 - _tcslen(szTip));
							mir_free_and_nill(ProtoXStatus);
						}
					}
					else {
						if (szTip[0])
							_tcsncat(szTip, szSeparator, tipSize - 1 - _tcslen(szTip));
						#if defined( _UNICODE )
						{	
							TCHAR* p = a2u( szProtoName );
							_tcsncat(szTip, p, SIZEOF(szTip) - 1 - _tcslen(szTip));
							mir_free_and_nill( p );
						}
						#else
							_tcsncat(szTip, szProtoName, SIZEOF(szTip) - 1 - _tcslen(szTip));
						#endif
						_tcsncat(szTip, _T(" "), tipSize - 1 - _tcslen(szTip));
						_tcsncat(szTip, szStatus, tipSize - 1 - _tcslen(szTip));
					}
				}
			}
	} else {
		TCHAR *ProtoXStatus=NULL;
		CallProtoService(szProto, PS_GETNAME, sizeof(szProtoName), (LPARAM) szProtoName);
		#if defined( _UNICODE )
			sztProto = a2u( szProtoName );
		#else
			sztProto = szProtoName;
		#endif
			//Get XStatus
				{			
					int xstatus=0;
					if (CallProtoService(szProto,PS_GETSTATUS,0,0)>ID_STATUS_OFFLINE /* && (DBGetContactSettingByte(NULL,"CLUI","XStatusTray",15)&3)*/ )	
					{
						char str[MAXMODULELABELLENGTH];
						strcpy(str,szProto);
						strcat(str,"/GetXStatus");
						if (ServiceExists(str))
						{
							char * dbTitle="XStatusName";
							char * dbTitle2=NULL;
							xstatus=CallProtoService(szProto,"/GetXStatus",(WPARAM)&dbTitle,(LPARAM)&dbTitle2);
							if (dbTitle && xstatus)
							{
								DBVARIANT dbv={0};
								if (!DBGetContactSettingTString(NULL,szProto,dbTitle,&dbv))
								{
									if (dbv.ptszVal[0]!=(TCHAR)0)
										ProtoXStatus=mir_tstrdup(dbv.ptszVal);
									//mir_free_and_nill(dbv.ptszVal);
									DBFreeVariant(&dbv);
								}
							}
						}
					}
				}
				//_DEBUG
				//ProtoXStatus=mir_tstrdup(_T("XStatus test name"));
		szStatus = pcli->pfnGetStatusModeDescription(CallProtoService(szProto, PS_GETSTATUS, 0, 0), 0);
		if (szPrefix && szPrefix[0]) {
			if (DBGetContactSettingByte(NULL, "CList", "AlwaysStatus", SETTING_ALWAYSSTATUS_DEFAULT)) {
				if(mToolTipTrayTips)
				{
					if (ProtoXStatus)
						mir_sntprintf(szTip, tipSize, _T("%s%s<b>%-12.12s</b>\t%s%s%-24.24s"), szPrefix, szSeparator, sztProto, szStatus,szSeparator,ProtoXStatus);
					else
						mir_sntprintf(szTip, tipSize, _T("%s%s<b>%-12.12s</b>\t%s"), szPrefix, szSeparator, sztProto, szStatus);
				}
				else
					mir_sntprintf(szTip, tipSize, _T("%s%s%s %s"), szPrefix, szSeparator, sztProto, szStatus);
			}
			else
				lstrcpyn(szTip, szPrefix, tipSize);
		} else {
			if(mToolTipTrayTips)
				if (ProtoXStatus) mir_sntprintf(szTip, tipSize, _T("<b>%-12.12s</b>\t%s\n%-24.24s"), sztProto, szStatus,ProtoXStatus);
				else mir_sntprintf(szTip, tipSize, _T("<b>%-12.12s</b>\t%s"), sztProto, szStatus);
			else
				mir_sntprintf(szTip, tipSize, _T("%s %s"), sztProto, szStatus);
		}
		if (ProtoXStatus) mir_free_and_nill(ProtoXStatus);
		#if defined( _UNICODE )
			mir_free_and_nill(sztProto);
		#endif
	}
	return szTip;
}
static int TrayIconAddEventsToTooltip(const char *szProto)
{
};

static int TrayIconAdd(HWND hwnd,const char *szProto,const char *szIconProto,int status)
{
	NOTIFYICONDATA nid={0};
	int i;

	for (i = 0; i < trayIconCount; i++) {
		if (trayIcon[i].id == 0)
			break;
	}
	trayIcon[i].id=TRAYICON_ID_BASE+i;
	trayIcon[i].szProto=(char*)szProto;
	
	nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = hwnd;
	nid.uID = trayIcon[i].id;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = TIM_CALLBACK;
	trayIcon[i].hBaseIcon=GetIconFromStatusMode(NULL,szIconProto?szIconProto:trayIcon[i].szProto,status);
	nid.hIcon = trayIcon[i].hBaseIcon;	

	if (dviShell.dwMajorVersion>=5)
		nid.uFlags |= NIF_INFO;

	TrayIconMakeTooltip(NULL, trayIcon[i].szProto);
	if(!mToolTipTrayTips)
		lstrcpyn(nid.szTip, szTip, SIZEOF(nid.szTip));
	mir_strset(&(trayIcon[i].iconTip),szTip);
	Shell_NotifyIcon(NIM_ADD, &nid);

	trayIcon[i].isBase=1;
	return i;
}

static void TrayIconRemove(HWND hwnd,const char *szProto)
{
	int i;
	NOTIFYICONDATA nid={0};

	nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = hwnd;
	for(i=0;i<trayIconCount;i++) {
		if (trayIcon[i].id == 0)
			continue;
		if (lstrcmpA(szProto, trayIcon[i].szProto))
			continue;

		nid.uID = trayIcon[i].id;
		Shell_NotifyIcon(NIM_DELETE, &nid);

		DestroyIcon_protect(trayIcon[i].hBaseIcon);
		if (trayIcon[i].iconTip) mir_free_and_nill(trayIcon[i].iconTip);
		trayIcon[i].id=0;
		break;
	}
}
int GetAverageMode()
{
	int count,netProtoCount,i;
	int averageMode=0;
	PROTOCOLDESCRIPTOR **protos;
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	for(i=0,netProtoCount=0;i<count;i++) {
		if(protos[i]->type!=PROTOTYPE_PROTOCOL || (pcli->pfnGetProtocolVisibility(protos[i]->szName)==0)) continue;
		cycleStep=i;
		netProtoCount++;
		if(averageMode==0) averageMode=CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0);
		else if(averageMode!=CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0)) {averageMode=-1; break;}
	}
	return averageMode;
}

static int TrayIconInit(HWND hwnd)
{
	int count;
	int averageMode=0;
	PROTOCOLDESCRIPTOR **protos;
	mToolTipTrayTips = FALSE;
	if(ServiceExists("mToolTip/ShowTip"))
		mToolTipTrayTips = TRUE;

	if (cycleTimerId) {
		KillTimer(NULL, cycleTimerId); 
		cycleTimerId = 0;
	}
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	averageMode=GetAverageMode();
	if (DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_MULTI)
	{
		int i;
		int netProtoCount=0; 
		for(i=0,netProtoCount=0;i<count;i++) if(protos[i]->type==PROTOTYPE_PROTOCOL && (pcli->pfnGetProtocolVisibility(protos[i]->szName)!=0) ) netProtoCount++;

		trayIconCount=netProtoCount;
	}
	else trayIconCount=1;
	trayIcon=(struct trayIconInfo_t*)mir_calloc(count * sizeof(struct trayIconInfo_t));
	memset(trayIcon,0,count*sizeof(struct trayIconInfo_t));
	if(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_MULTI &&
		(averageMode<=0 || DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT))) {
			int i;
			for(i=count-1;i>=0;i--) 
			{	
				int j;
				j=pcli->pfnGetProtoIndexByPos(protos,count,i);
				if (j>-1)
					if(protos[j]->type==PROTOTYPE_PROTOCOL && (pcli->pfnGetProtocolVisibility(protos[j]->szName)!=0)) TrayIconAdd(hwnd,protos[j]->szName,NULL,CallProtoService(protos[j]->szName,PS_GETSTATUS,0,0));
			}
		}
	else if((averageMode==-1 || (averageMode>=0&&DBGetContactSettingByte(NULL,"CList","AlwaysPrimary",0))) && DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_SINGLE) {
		DBVARIANT dbv={DBVT_DELETED};
		char *szProto;
		if(DBGetContactSetting(NULL,"CList","PrimaryStatus",&dbv)) szProto=NULL;
		else szProto=dbv.pszVal;
		TrayIconAdd(hwnd,NULL,szProto,szProto?CallProtoService(szProto,PS_GETSTATUS,0,0):CallService(MS_CLIST_GETSTATUSMODE,0,0));
		DBFreeVariant(&dbv);
	}
	else 
		TrayIconAdd(hwnd,NULL,NULL,CListTray_GetGlobalStatus(0,0) /*averageMode*/);
	if(averageMode<=0 && DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_CYCLE)
		cycleTimerId=SetTimer(NULL,0,DBGetContactSettingWord(NULL,"CList","CycleTime",SETTING_CYCLETIME_DEFAULT)*1000,TrayCycleTimerProc);
	return 0;
}

void CListTray_TrayIconDestroy(HWND hwnd)
{
	NOTIFYICONDATA nid={0};
	int i;

	nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = hwnd;
	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		nid.uID = trayIcon[i].id;
		Shell_NotifyIcon(NIM_DELETE, &nid);
		DestroyIcon_protect(trayIcon[i].hBaseIcon);
		if (trayIcon[i].iconTip) mir_free_and_nill(trayIcon[i].iconTip);
	}
	if (trayIcon) mir_free_and_nill(trayIcon);
	trayIcon=NULL;
	trayIconCount=0;
}

//called when Explorer crashes and the taskbar is remade
static void TrayIconTaskbarCreated(HWND hwnd)
{
	CListTray_TrayIconDestroy(hwnd);
	TrayIconInit(hwnd);
}

static int TrayIconUpdate(HICON hNewIcon,const TCHAR *szNewTip,const char *szPreferredProto, int isBase)
{
	NOTIFYICONDATA nid={0};
	int i;

	nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATA_V1_SIZE;
	nid.hWnd = pcli->hwndContactList;
	nid.uFlags = NIF_ICON | NIF_TIP;
	nid.hIcon = hNewIcon;			
	if (!hNewIcon) 
		return 0;

	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		if(trayIcon[i].szProto && szPreferredProto && lstrcmpA(trayIcon[i].szProto,szPreferredProto)) continue;
		nid.uID = trayIcon[i].id;

		TrayIconMakeTooltip(szNewTip, trayIcon[i].szProto);
		mir_strset(&(trayIcon[i].iconTip),szTip);
		if(!mToolTipTrayTips)
			lstrcpyn(nid.szTip, szTip, SIZEOF(nid.szTip));
		Shell_NotifyIcon(NIM_MODIFY, &nid);

		trayIcon[i].isBase=isBase;
		return i;
	}
	/*   if (boolstrcmpi("MetaContacts",szPreferredProto))
	{
	if (DBGetContactSettingByte(NULL,"MetaContacts","SubcontactWindows",0)) return -1;
	}
	*/
	for(i=0;i<trayIconCount;i++) 
	{
		if(trayIcon[i].id==0) continue;
		nid.uID = trayIcon[i].id;

		TrayIconMakeTooltip(szNewTip, trayIcon[i].szProto);
		mir_strset(&(trayIcon[i].iconTip),szTip);
		if(!mToolTipTrayTips)
			lstrcpyn(nid.szTip, szTip, SIZEOF(nid.szTip));
		Shell_NotifyIcon(NIM_MODIFY, &nid);
		trayIcon[i].isBase=isBase;

		if(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT) == SETTING_TRAYICON_MULTI ) 
		{
			if(RefreshTimerId) {KillTimer(NULL,RefreshTimerId); RefreshTimerId=0;}
			RefreshTimerId=SetTimer(NULL,0,DBGetContactSettingWord(NULL,"CList","CycleTime",SETTING_CYCLETIME_DEFAULT)*1005,RefreshTimerProc);	// by FYR (Setting timer)
			return -1;	//by FYR (to change only one icon)
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
			(pcli->pfnGetProtocolVisibility(protos[i]->szName)!=0))
			cliTrayIconUpdateBase(protos[i]->szName);

}
//////// End by FYR /////////


static int TrayIconSetBaseInfo(HICON hIcon, char *szPreferredProto)
{
	int i;
	if (szPreferredProto)
	{
		for(i=0;i<trayIconCount;i++) {
			if(trayIcon[i].id==0) continue;
			if(lstrcmpA(trayIcon[i].szProto,szPreferredProto)) continue;
			DestroyIcon_protect(trayIcon[i].hBaseIcon);
			trayIcon[i].hBaseIcon=hIcon;
			return i;
		}
		// if average mode ==-1 	
		if ((pcli->pfnGetProtocolVisibility(szPreferredProto)) && 
			(GetAverageMode()==-1) && 
			(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_MULTI) &&
			!(DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT)))
			return -1;
	}
	//if there wasn't a specific icon, there will only be one suitable
	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		DestroyIcon_protect(trayIcon[i].hBaseIcon);
		trayIcon[i].hBaseIcon=hIcon;
		return i;
	}
	DestroyIcon_protect(hIcon);
	return -1;
}

void cliTrayIconUpdateWithImageList(int iImage,const TCHAR *szNewTip,char *szPreferredProto)
{
	HICON hIcon;

	hIcon=SkinEngine_ImageList_GetIcon(himlCListClc,iImage,ILD_NORMAL);
	TrayIconUpdate(hIcon,szNewTip,szPreferredProto,0);
	DestroyIcon_protect(hIcon);
}


static VOID CALLBACK TrayCycleTimerProc(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	int count;
	PROTOCOLDESCRIPTOR **protos;
	int iteration=0;

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	for(cycleStep++;;cycleStep++) {
		if(cycleStep>=count) {cycleStep=0; iteration++;}
		if(protos[cycleStep]->type==PROTOTYPE_PROTOCOL && (pcli->pfnGetProtocolVisibility(protos[cycleStep]->szName)!=0)) break;
		if (iteration>5) break;
	}
	DestroyIcon_protect(trayIcon[0].hBaseIcon);
	trayIcon[0].hBaseIcon=GetIconFromStatusMode(NULL,protos[cycleStep]->szName,CallProtoService(protos[cycleStep]->szName,PS_GETSTATUS,0,0));
	if(trayIcon[0].isBase)
		TrayIconUpdate(trayIcon[0].hBaseIcon,NULL,NULL,1);
}

void cliTrayIconUpdateBase(char *szChangedProto)
{
	int i,count,netProtoCount,changed=-1;
	PROTOCOLDESCRIPTOR **protos;
	int averageMode=0;
	
	HWND hwnd=pcli->hwndContactList;
	if (!szChangedProto) return;
	if(cycleTimerId) {KillTimer(NULL,cycleTimerId); cycleTimerId=0;}
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	for(i=0,netProtoCount=0;i<count;i++) {
		if(protos[i]->type!=PROTOTYPE_PROTOCOL || (pcli->pfnGetProtocolVisibility(protos[i]->szName)==0)) continue;
		netProtoCount++;
		if(!lstrcmpA(szChangedProto,protos[i]->szName)) cycleStep=i;
		if(averageMode==0) averageMode=CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0);
		else if(averageMode!=CallProtoService(protos[i]->szName,PS_GETSTATUS,0,0)) {averageMode=-1; break;}
	}

	if(netProtoCount>1) {
		if(averageMode>=ID_STATUS_OFFLINE) {
			if(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_MULTI) {
				if(DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT))				
					changed=TrayIconSetBaseInfo(GetIconFromStatusMode(NULL,szChangedProto,averageMode),szChangedProto);
				else if(trayIcon && trayIcon[0].szProto!=NULL) {
					CListTray_TrayIconDestroy(hwnd);
					TrayIconInit(hwnd);
				}
				else 
					changed=TrayIconSetBaseInfo(GetIconFromStatusMode(NULL,NULL,averageMode),NULL);
			}
			else 
			{
				if(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_SINGLE
					&& DBGetContactSettingByte(NULL,"CList","AlwaysPrimary",0))
				{
					DBVARIANT dbv={DBVT_DELETED};
					char *szProto;
					if(DBGetContactSetting(NULL,"CList","PrimaryStatus",&dbv)) szProto=NULL;
					else szProto=dbv.pszVal;
					changed=TrayIconSetBaseInfo(GetIconFromStatusMode(NULL,szProto,averageMode),NULL);
					if (szProto) mir_free_and_nill(szProto);
				}
				else
					changed=TrayIconSetBaseInfo(GetIconFromStatusMode(NULL,NULL,averageMode),NULL);
			}
		}
		else {
			switch(DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)) 
			{
			case SETTING_TRAYICON_SINGLE:
				{	
					DBVARIANT dbv={DBVT_DELETED};
					char *szProto;
					int status;
					if(DBGetContactSetting(NULL,"CList","PrimaryStatus",&dbv)) szProto=NULL;
					else szProto=dbv.pszVal;
					status=CallProtoService(szChangedProto,PS_GETSTATUS,0,0);
					
					if ((g_StatusBarData.connectingIcon==1)&&status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
					{
						//
						HICON hIcon;
						// 1 check if multi connecting icon
						CListTray_GetGlobalStatus(0,0);
						if (g_bMultiConnectionMode)
							if (_strcmpi(szChangedProto,g_szConnectingProto))
								return;
							else 
								hIcon=(HICON)CLUI_GetConnectingIconService((WPARAM)GLOBAL_PROTO_NAME/*(WPARAM)szChangedProto*/,1);
						else
							hIcon=(HICON)CLUI_GetConnectingIconService((WPARAM)szChangedProto,0);										
						if (hIcon)
						{
							changed=TrayIconSetBaseInfo(hIcon,NULL);						
							//TrayIconUpdate(hIcon,NULL,NULL,1);

							//DestroyIcon_protect(hIcon);
							DBFreeVariant(&dbv);
							break;
						}
						hIcon=hIcon;
						//return;
					}
					else
					{
						changed=TrayIconSetBaseInfo(GetIconFromStatusMode(NULL,szProto,szProto?CallProtoService(szProto,PS_GETSTATUS,0,0):CallService(MS_CLIST_GETSTATUSMODE,0,0)),NULL);
					}
					DBFreeVariant(&dbv);
					break;
				}
			case SETTING_TRAYICON_CYCLE:
				cycleTimerId=SetTimer(NULL,0,DBGetContactSettingWord(NULL,"CList","CycleTime",SETTING_CYCLETIME_DEFAULT)*1000,TrayCycleTimerProc);
				changed=TrayIconSetBaseInfo(GetIconFromStatusMode(NULL,szChangedProto,averageMode),NULL);
				break;
			case SETTING_TRAYICON_MULTI:
				if (!trayIcon) 
				{
					TrayIconRemove(NULL,NULL);
				} else 

					if(DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT))
					{
						if (pcli->pfnGetProtocolVisibility(szChangedProto))
						{

							int status;
							status=CallProtoService(szChangedProto,PS_GETSTATUS,0,0);
							if ((g_StatusBarData.connectingIcon==1)&&status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
							{
								//
								HICON hIcon;
								hIcon=(HICON)CLUI_GetConnectingIconService((WPARAM)szChangedProto,0);;										
								if (hIcon)
								{
									changed=TrayIconSetBaseInfo(hIcon,szChangedProto);						
									TrayIconUpdate(hIcon,NULL,szChangedProto,1);					
									DestroyIcon_protect(hIcon);
								}
							}
							else
								changed=TrayIconSetBaseInfo(GetIconFromStatusMode(NULL,szChangedProto,CallProtoService(szChangedProto,PS_GETSTATUS,0,0)),szChangedProto);
						}
					}
					else 
					{       if (pcli->pfnGetProtocolVisibility(szChangedProto))
					{
						int i;
						int avg;
						avg=GetAverageMode();
						i=TrayIconSetBaseInfo(GetIconFromStatusMode(NULL,szChangedProto,CallProtoService(szChangedProto,PS_GETSTATUS,0,0)),szChangedProto);
						if (i<0 || (avg!=-1))
						{
							CListTray_TrayIconDestroy(hwnd);
							TrayIconInit(hwnd);
						}
						else 
						{
							int status;
							changed=i;
							status=CallProtoService(szChangedProto,PS_GETSTATUS,0,0);
							if ((g_StatusBarData.connectingIcon==1)&&status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
							{
								//
								HICON hIcon;
								hIcon=(HICON)CLUI_GetConnectingIconService((WPARAM)szChangedProto,0);;										
								if (hIcon)
								{
									changed=TrayIconSetBaseInfo(hIcon,szChangedProto);						
									TrayIconUpdate(hIcon,NULL,szChangedProto,1);					
									DestroyIcon_protect(hIcon);
								}
							}
						}
					}
					}
					break;
			}
		}
	}
	else if (pcli->pfnGetProtocolVisibility(szChangedProto))
	{
		int status=CallProtoService(szChangedProto,PS_GETSTATUS,0,0);
		BOOL workAround;
		workAround=(status>=ID_STATUS_OFFLINE && status<=ID_STATUS_IDLE);
		if ((g_StatusBarData.connectingIcon==1)&&status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
		{
			//
			HICON hIcon;

			hIcon=(HICON)CLUI_GetConnectingIconService((WPARAM)szChangedProto,0);;										
			if (hIcon)
			{
				changed=TrayIconSetBaseInfo(hIcon,NULL);						
				TrayIconUpdate(hIcon,NULL,NULL,1);					

				DestroyIcon_protect(hIcon);
				//return;
			}
		}
		if (workAround)	
		{ 
			//BYTE b=DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT);
			//if (b==SETTING_TRAYICON_MULTI)
			changed=TrayIconSetBaseInfo(GetIconFromStatusMode(NULL,szChangedProto,averageMode),workAround?szChangedProto:NULL);
		}
	};

	if(changed!=-1) // && trayIcon[changed].isBase)
		TrayIconUpdate(trayIcon[changed].hBaseIcon,NULL,szChangedProto/*trayIcon[changed].szProto*/,1);  // by FYR (No suitable protocol)

}

void cliTrayIconSetToBase(char *szPreferredProto)
{
	int i;

	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		if(lstrcmpA(trayIcon[i].szProto,szPreferredProto)) continue;
		TrayIconUpdate(trayIcon[i].hBaseIcon,NULL,szPreferredProto,1);
		return;
	}
	//if there wasn't a specific icon, there will only be one suitable
	for(i=0;i<trayIconCount;i++) {
		if(trayIcon[i].id==0) continue;
		TrayIconUpdate(trayIcon[i].hBaseIcon,NULL,trayIcon[i].szProto/* szPreferredProto*/,1);
		//return; //Need to restore all icons if no preffered found
	}
	return;
}

static int autoHideTimerId;

#define TOOLTIP_TOLERANCE 5

static VOID CALLBACK TrayIconAutoHideTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	HWND hwndClui, ActiveWindow;
	KillTimer(hwnd,idEvent);
	hwndClui=pcli->hwndContactList;
	ActiveWindow=GetActiveWindow();
	if(ActiveWindow==hwndClui) return;
	if (CLUI_CheckOwnedByClui(ActiveWindow)) return; 
	//CLUI_ShowWindowMod(hwndClui,SW_HIDE);
	CListMod_HideWindow(hwndClui, SW_HIDE);
	if(MySetProcessWorkingSetSize!=NULL)
		MySetProcessWorkingSetSize(GetCurrentProcess(),-1,-1);
}

int TrayIconPauseAutoHide(WPARAM wParam,LPARAM lParam)
{
	if(DBGetContactSettingByte(NULL,"CList","AutoHide",SETTING_AUTOHIDE_DEFAULT)) {
		if(GetActiveWindow()!=pcli->hwndContactList
			&&GetWindow(GetParent(GetActiveWindow()),GW_OWNER) != pcli->hwndContactList )
		{   
			HWND h1,h2,h3,h4;
			h1=GetActiveWindow();
			h2=GetParent(h1);
			h3=GetWindow(h2,GW_OWNER);
			h4=pcli->hwndContactList;

			KillTimer(NULL,autoHideTimerId);
			autoHideTimerId=SetTimer(NULL,0,1000*DBGetContactSettingWord(NULL,"CList","HideTime",SETTING_HIDETIME_DEFAULT),TrayIconAutoHideTimer);
		}
	}
	return 0;
}


static BYTE s_LastHoverIconID=0;

static void CALLBACK TrayHideToolTipTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD elapsed)
{
	if (g_trayTooltipActive)
	{
		POINT pt;
		GetCursorPos(&pt);
		if(abs(pt.x - tray_hover_pos.x)>TOOLTIP_TOLERANCE || abs(pt.y - tray_hover_pos.y)>TOOLTIP_TOLERANCE)
		{
			CallService("mToolTip/HideTip", 0, 0);
			g_trayTooltipActive = FALSE;
			KillTimer(hwnd,TIMERID_TRAYHOVER_2);
		}
	}
	else KillTimer(hwnd,TIMERID_TRAYHOVER_2);
}

static void CALLBACK TrayToolTipTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD elapsed)
{
	if(!g_trayTooltipActive && !g_trayMenuOnScreen) 
	{
		CLCINFOTIP ti = {0};	
		POINT pt;
		GetCursorPos(&pt);
		if(abs(pt.x-tray_hover_pos.x)<=TOOLTIP_TOLERANCE && abs(pt.y-tray_hover_pos.y)<=TOOLTIP_TOLERANCE) 
		{
			TCHAR * szTipCur=szTip;
			{
				int n=s_LastHoverIconID-100;
				if (n>=0 && n<trayIconCount)
				{
                    //TODO event support
					szTipCur=trayIcon[n].iconTip;
					//TrayIconMakeTooltip(NULL,trayIcon[n].szProto);
				}
			}
			ti.rcItem.left=pt.x-10;
			ti.rcItem.right=pt.x+10;
			ti.rcItem.top=pt.y-10;
			ti.rcItem.bottom=pt.y+10;
			ti.cbSize = sizeof(ti);
			ti.isTreeFocused = GetFocus() == pcli->hwndContactList ? 1 : 0;
			#if defined( _UNICODE )
			{	char* p = u2a( szTipCur );
	        	CallService("mToolTip/ShowTip", (WPARAM)p, (LPARAM)&ti);
				mir_free_and_nill( p );			
			}
			#else
	        	CallService("mToolTip/ShowTip", (WPARAM)szTipCur, (LPARAM)&ti);
			#endif
			GetCursorPos(&tray_hover_pos);
			SetTimer(pcli->hwndContactList, TIMERID_TRAYHOVER_2, 600, TrayHideToolTipTimerProc);
			g_trayTooltipActive = TRUE;		
		}
	}
	KillTimer(hwnd, id);
	
}
int cli_TrayIconProcessMessage(WPARAM wParam,LPARAM lParam)
{
	MSG *msg=(MSG*)wParam;
	switch(msg->message) {
case WM_CREATE: {			
	WM_TASKBARCREATED=RegisterWindowMessage(TEXT("TaskbarCreated"));
	PostMessage(msg->hwnd,TIM_CREATE,0,0);	
	return FALSE;
				}
case WM_EXITMENULOOP:
	if (g_trayMenuOnScreen) 
		g_trayMenuOnScreen=FALSE;
	break;

case WM_DRAWITEM:
	return CallService(MS_CLIST_MENUDRAWITEM,msg->wParam,msg->lParam);
	break;
case WM_MEASUREITEM:
	return CallService(MS_CLIST_MENUMEASUREITEM,msg->wParam,msg->lParam);
	break;
case TIM_CREATE:
	TrayIconInit(msg->hwnd);
	break;
case WM_ACTIVATE:
	{
		HWND h1,h2,h4;
		SetCursor(LoadCursor(NULL, IDC_ARROW));	
		h1=(HWND)msg->lParam;
		h2=h1?GetParent(h1):NULL;
		h4=pcli->hwndContactList;
		if(DBGetContactSettingByte(NULL,"CList","AutoHide",SETTING_AUTOHIDE_DEFAULT)) {
			if(LOWORD(msg->wParam)==WA_INACTIVE && h2!=h4)
				autoHideTimerId=SetTimer(NULL,0,1000*DBGetContactSettingWord(NULL,"CList","HideTime",SETTING_HIDETIME_DEFAULT),TrayIconAutoHideTimer);
			else KillTimer(NULL,autoHideTimerId);
		}
	}
	break;
case WM_DESTROY:
	CListTray_TrayIconDestroy(msg->hwnd);
	return FALSE;

case TIM_CALLBACK:
	if (msg->lParam==WM_MBUTTONDOWN || msg->lParam==WM_LBUTTONDOWN || msg->lParam==WM_RBUTTONDOWN && g_trayTooltipActive)
	{
		CallService("mToolTip/HideTip", 0, 0);
		g_trayTooltipActive = FALSE;
	}
	if (msg->lParam==WM_MBUTTONUP)
	{
		cliShowHide(0,0);				
	}
	else if ((GetAsyncKeyState(VK_CONTROL)&0x8000) && msg->lParam == WM_LBUTTONDOWN && !DBGetContactSettingByte(NULL,"CList","Tray1Click",SETTING_TRAY1CLICK_DEFAULT))
	{
		POINT pt;	
		HMENU hMenu;
		hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,(WPARAM)0,(LPARAM)0);
		g_mutex_bOnTrayRightClick=1;
		IS_WM_MOUSE_DOWN_IN_TRAY=1;
		SetForegroundWindow(msg->hwnd);
		SetFocus(msg->hwnd);
		GetCursorPos(&pt);
        g_trayMenuOnScreen=TRUE;
		TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, msg->hwnd, NULL);
        PostMessage(msg->hwnd, WM_NULL, 0, 0);		
		g_mutex_bOnTrayRightClick=0;
		IS_WM_MOUSE_DOWN_IN_TRAY=0;
	}
	else if (msg->lParam==WM_MBUTTONDOWN ||msg->lParam==WM_LBUTTONDOWN ||msg->lParam==WM_RBUTTONDOWN)
	{
		IS_WM_MOUSE_DOWN_IN_TRAY=1;
	}
	else if (msg->lParam==(DBGetContactSettingByte(NULL,"CList","Tray1Click",SETTING_TRAY1CLICK_DEFAULT)?WM_LBUTTONUP:WM_LBUTTONDBLCLK))
	{
		g_mutex_bOnTrayRightClick=1;
		if ((GetAsyncKeyState(VK_CONTROL)&0x8000)) cliShowHide(0,0);
		else {
			if(pcli->pfnEventsProcessTrayDoubleClick()) 
				cliShowHide(0,0);
		}				
	}
	else if (msg->lParam == WM_RBUTTONUP)
	{
		POINT pt;	
		HMENU hMenu;
		/*

		MENUITEMINFO mi;


		hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDR_CONTEXT));
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
		g_mutex_bOnTrayRightClick=1;
		
		SetForegroundWindow(msg->hwnd);
		SetFocus(msg->hwnd);
		
		GetCursorPos(&pt);
		g_trayMenuOnScreen=TRUE;
		TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, msg->hwnd, NULL);		
		PostMessage(msg->hwnd, WM_NULL, 0, 0);
		

	}
		else if (msg->lParam == WM_MOUSEMOVE) {
			s_LastHoverIconID=msg->wParam;
			if(g_trayTooltipActive) 
			{
				POINT pt;
				GetCursorPos(&pt);
				if(abs(pt.x - tray_hover_pos.x)>TOOLTIP_TOLERANCE || abs(pt.y - tray_hover_pos.y)>TOOLTIP_TOLERANCE) 
				{
					CallService("mToolTip/HideTip", 0, 0);
					g_trayTooltipActive = FALSE;
					ReleaseCapture();
				}
				break;
			}
			else
			{
				GetCursorPos(&tray_hover_pos);
				SetTimer(pcli->hwndContactList, TIMERID_TRAYHOVER, 600, TrayToolTipTimerProc);
			}
		}
		else break;
		*((LRESULT*)lParam)=0;
	return TRUE;

default:
	if(msg->message==WM_TASKBARCREATED) {
		TrayIconTaskbarCreated(msg->hwnd);
		*((LRESULT*)lParam)=0;
		return TRUE;
	}

	}
	//return saveTrayIconProcessMessage(wParam, lParam);
	return FALSE;
}
int cliCListTrayNotify(MIRANDASYSTRAYNOTIFY *msn)
{
 #if defined(_UNICODE)
	if(msn->dwInfoFlags & NIIF_INTERN_UNICODE) {
		if (msn && msn->cbSize == sizeof(MIRANDASYSTRAYNOTIFY) && msn->szInfo && msn->szInfoTitle) {
			if (trayIcon) {
				NOTIFYICONDATAW nid = {0};
				nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATAW_V1_SIZE;
				nid.hWnd = pcli->hwndContactList;
				if (msn->szProto) {
					int j;
					for (j = 0; j < trayIconCount; j++) {
						if (trayIcon[j].szProto != NULL) {
							if (strcmp(msn->szProto, trayIcon[j].szProto) == 0) {
								nid.uID = trayIcon[j].id;
								j = trayIconCount;
								continue;
							}
						} else {
							if (trayIcon[j].isBase) {
								nid.uID = trayIcon[j].id;
								j = trayIconCount;
								continue;
							}
						}
					} //for
				} else {
					nid.uID = trayIcon[0].id;
				}
				nid.uFlags = NIF_INFO;
				lstrcpynW(nid.szInfo, (WCHAR *)msn->szInfo, SIZEOF(nid.szInfo));
				lstrcpynW(nid.szInfoTitle, (WCHAR *)msn->szInfoTitle, SIZEOF(nid.szInfoTitle));
				nid.szInfo[SIZEOF(nid.szInfo) - 1] = 0;
				nid.szInfoTitle[SIZEOF(nid.szInfoTitle) - 1] = 0;
				nid.uTimeout = msn->uTimeout;
				nid.dwInfoFlags = (msn->dwInfoFlags & ~NIIF_INTERN_UNICODE);
				return Shell_NotifyIconW(NIM_MODIFY, (void*) &nid) == 0;
			}
			return 2;
		}
	}
	else {
#endif        
		if (msn && msn->cbSize == sizeof(MIRANDASYSTRAYNOTIFY) && msn->szInfo && msn->szInfoTitle) {
			if (trayIcon) {
				NOTIFYICONDATAA nid = {0};
				nid.cbSize = ( dviShell.dwMajorVersion >= 5 ) ? sizeof(nid) : NOTIFYICONDATAA_V1_SIZE;
				nid.hWnd = pcli->hwndContactList;
				if (msn->szProto) {
					int j;
					for (j = 0; j < trayIconCount; j++) {
						if (trayIcon[j].szProto != NULL) {
							if (strcmp(msn->szProto, trayIcon[j].szProto) == 0) {
								nid.uID = trayIcon[j].id;
								j = trayIconCount;
								continue;
							}
						} else {
							if (trayIcon[j].isBase) {
								nid.uID = trayIcon[j].id;
								j = trayIconCount;
								continue;
							}
						}
					} //for
				} else {
					nid.uID = trayIcon[0].id;
				}
				nid.uFlags = NIF_INFO;
				lstrcpynA(nid.szInfo, msn->szInfo, sizeof(nid.szInfo));
				lstrcpynA(nid.szInfoTitle, msn->szInfoTitle, sizeof(nid.szInfoTitle));
				nid.szInfo[sizeof(nid.szInfo) - 1] = 0;
				nid.szInfoTitle[sizeof(nid.szInfoTitle) - 1] = 0;
				nid.uTimeout = msn->uTimeout;
				nid.dwInfoFlags = msn->dwInfoFlags;
				return Shell_NotifyIconA(NIM_MODIFY, (void*) &nid) == 0;
			}
			return 2;
		}
#if defined(_UNICODE)
	}
#endif    
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
		if (!mir_strcmp(mmep->szServiceName,"Help/AboutCommand"))
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
	tmp.name=Translate("TrayMenu");
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
		mi.hIcon=LoadSmallIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_FINDUSER));
		mi.pszService="FindAdd/FindAddCommand";
		mi.pszName=Translate("&Find/Add Contacts...");
		AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);
		DestroyIcon_protect(mi.hIcon);


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
		mi.hIcon=LoadSmallIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_OPTIONS));
		mi.pszService="Options/OptionsCommand";
		mi.pszName=Translate("&Options...");
		AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);
		DestroyIcon_protect(mi.hIcon);

		memset(&mi,0,sizeof(mi));
		mi.cbSize=sizeof(mi);
		mi.position=500000;
		mi.hIcon=LoadSmallIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MIRANDA));
		mi.pszService="Help/AboutCommand";
		mi.pszName=Translate("&About");
		AddTrayMenuItem((WPARAM)0,(LPARAM)&mi);
		DestroyIcon_protect(mi.hIcon);

	};
}


void UninitTrayMenu()
{
    if (hTrayMenuObject && ServiceExists(MO_REMOVEMENUOBJECT))
        CallService(MO_REMOVEMENUOBJECT,(WPARAM)hTrayMenuObject,0);
    hTrayMenuObject=NULL;
}
//////////////////////////////END TRAY MENU/////////////////////////
void cliTrayIconIconsChanged(void)
{
	HWND hwnd = pcli->hwndContactList;
	CListTray_TrayIconDestroy(hwnd);
	TrayIconInit(hwnd);
}

void InitTray(void)
{
	HINSTANCE hLib;
	//////////////////////////////END TRAY MENU/////////////////////////
	hLib=LoadLibrary(TEXT("shell32.dll"));
	if (hLib) {
		DLLGETVERSIONPROC proc;
		dviShell.cbSize=sizeof(dviShell);
		proc=(DLLGETVERSIONPROC)GetProcAddress(hLib,"DllGetVersion");
		if (!IsBadCodePtr(proc)) {
			proc(&dviShell);
		}
		FreeLibrary(hLib);
	}
	if (dviShell.dwMajorVersion>=5) {
		CreateServiceFunction(MS_CLIST_SYSTRAY_NOTIFY,(MIRANDASERVICE)pcli->pfnCListTrayNotify);
	}

	InitTrayMenus();
	return;
}