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
#include "m_clc.h"
#include "m_clui.h"
#include "genmenu.h"

#define TM_AUTOALPHA  1
#define TM_STATUSBARUPDATE  200
#define MENU_MIRANDAMENU         0xFFFF1234

static HMODULE hUserDll;
static HIMAGELIST himlMirandaIcon;
HWND hwndContactList = NULL;
HWND hwndStatus;
HMENU hMenuMain;
static HANDLE hContactDraggingEvent,hContactDroppedEvent,hContactDragStopEvent;
HWND hwndContactTree;
UINT hMsgGetProfile=0;
UINT uMsgProcessProfile;

extern boolean canloadstatusbar;
boolean OnModulesLoadedCalled=FALSE;

HANDLE hSettingChangedHook=0;

static int transparentFocus=1;
static byte oldhideoffline;
static int lastreqh=0,requr=0,disableautoupd=1;
HANDLE hFrameContactTree;
BYTE showOpts;//for statusbar

typedef struct{
int CycleStartTick;
char *szProto;
int n;
int TimerCreated;
} ProtoTicks,*pProtoTicks;

ProtoTicks CycleStartTick[64];//max 64 protocols 

int CycleTimeInterval=2000;
int CycleIconCount=8;

BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
BOOL (WINAPI *MyAnimateWindow)(HWND hWnd,DWORD dwTime,DWORD dwFlags);

int CluiOptInit(WPARAM wParam,LPARAM lParam);
int SortList(WPARAM wParam,LPARAM lParam);
int LoadCluiServices(void);
int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);
extern int CheckProtocolOrder();

extern void SetAllExtraIcons(HWND hwndList,HANDLE hContact);
extern void ReloadExtraIcons();
extern void LoadExtraImageFunc();
extern int CreateStatusBarhWnd(HWND parent);
extern int CreateStatusBarFrame();
extern int LoadProtocolOrderModule(void);
extern int CLUIFramesUpdateFrame(WPARAM wParam,LPARAM lParam);
extern int ExtraToColumnNum(int extra);
extern void TrayIconUpdateBase(const char *szChangedProto);
void InvalidateDisplayNameCacheEntry(HANDLE hContact);

#define M_CREATECLC  (WM_USER+1)
#define M_SETALLEXTRAICONS (WM_USER+2)

static int CluiModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	MENUITEMINFO mii;
	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize=MENUITEMINFO_V4_SIZE;
	mii.fMask=MIIM_SUBMENU;
	mii.hSubMenu=(HMENU)CallService(MS_CLIST_MENUGETMAIN,0,0);
	SetMenuItemInfo(hMenuMain,0,TRUE,&mii);
	mii.hSubMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);
	SetMenuItemInfo(hMenuMain,1,TRUE,&mii);

	canloadstatusbar=TRUE;
	CheckProtocolOrder();
	SendMessage(hwndContactList,WM_SIZE,0,0);
	CluiProtocolStatusChanged(0,0);
	Sleep(0);
	PostMessage(hwndContactList,M_CREATECLC,0,0);
	
	OnModulesLoadedCalled=TRUE;	
	InvalidateDisplayNameCacheEntry(INVALID_HANDLE_VALUE);
	return 0;
}

pProtoTicks GetProtoTicksByProto(char * szProto)
{
	int i;

						for (i=0;i<64;i++)
						{
							if (CycleStartTick[i].szProto==NULL) break;
							if (strcmp(CycleStartTick[i].szProto,szProto)) continue;
							return(&CycleStartTick[i]);
						}
							for (i=0;i<64;i++)
							{
								if (CycleStartTick[i].szProto==NULL)
								{
									CycleStartTick[i].szProto=mir_strdup(szProto);
									CycleStartTick[i].CycleStartTick=0;
									CycleStartTick[i].n=i;
									return(&CycleStartTick[i]);
								}
							}
return (NULL);
}

static HICON ExtractIconFromPath(const char *path)
{
	char *comma;
	char file[MAX_PATH],fileFull[MAX_PATH];
	int n;
	HICON hIcon;
	{
		char buf[512];
		sprintf(buf,"LoadIcon %s\r\n",path);
	//OutputDebugString(buf);
	}
	lstrcpyn(file,path,sizeof(file));
	comma=strrchr(file,',');
	if(comma==NULL) n=0;
	else {n=atoi(comma+1); *comma=0;}
    CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)file, (LPARAM)fileFull);
	
#ifdef _DEBUG
	{
		char buf[512];
		sprintf(buf,"LoadIconFull %d %s\r\n",n,fileFull);
	//OutputDebugString(buf);
	}
#endif

	hIcon=NULL;
	ExtractIconEx(fileFull,n,NULL,&hIcon,1);
	return hIcon;
}


HICON GetConnectingIconForProto(char *szProto,int b)
{
		char szPath[MAX_PATH], szFullPath[MAX_PATH],*str;
		HICON hIcon=NULL;

		GetModuleFileName(GetModuleHandle(NULL), szPath, MAX_PATH);
		str=strrchr(szPath,'\\');
		b=b-1;
		if(str!=NULL) *str=0;
		_snprintf(szFullPath, sizeof(szFullPath), "%s\\Icons\\proto_conn_%s.dll,%d", szPath, szProto, b+1);
		hIcon=ExtractIconFromPath(szFullPath);
		if (hIcon) return hIcon;

#ifdef _DEBUG
		{
		char buf [256];
		sprintf(buf,"IconNotFound %s %d\r\n",szProto,b);
	//	OutputDebugString(buf);
		}
#endif

	if (!strcmp(szProto,"ICQ"))
	{
		
#ifdef _DEBUG
		char buf [256];
		sprintf(buf,"Icon %d %d\r\n",GetTickCount(),b);
		//OutputDebugString(buf);
#endif
		return(LoadIcon(g_hInst,(LPCSTR)(IDI_ICQC1+b)));
	}

	
		return(hIcon);
}
//wParam = szProto
int GetConnectingIconService(WPARAM wParam,LPARAM lParam)
{
						int b;						
						ProtoTicks *pt=NULL;
						HICON hIcon=NULL;

						char *szProto=(char *)wParam;
						if (!szProto) return 0;

						pt=GetProtoTicksByProto(szProto);

						if (pt!=NULL)
						{
						
						if (pt->CycleStartTick!=0) 
						{					
								b=((GetTickCount()-pt->CycleStartTick)/(CycleTimeInterval/CycleIconCount))%CycleIconCount;
								hIcon=GetConnectingIconForProto(szProto,b);
						};
						}
						
						return (int)hIcon;
};


int CreateTimerForConnectingIcon(WPARAM wParam,LPARAM lParam)
{

	int status=(int)wParam;
	char *szProto=(char *)lParam;					
	if (!szProto) return (0);
	if (!status) return (0);
				
				if ((DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",1)==1)&&status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
					{
			
						ProtoTicks *pt=NULL;

						pt=GetProtoTicksByProto(szProto);
						if (pt!=NULL)
						{
						
						if (pt->CycleStartTick==0) 
						{					
						//	sprintf(buf,"SetTimer %d\r\n",pt->n);
						//	OutputDebugString(buf);

							KillTimer(hwndContactList,TM_STATUSBARUPDATE+pt->n);
							SetTimer(hwndContactList,TM_STATUSBARUPDATE+pt->n,(int)(CycleTimeInterval/CycleIconCount)/1,0);
							pt->TimerCreated=1;
							pt->CycleStartTick=GetTickCount();
						};
					};
				}
				return 0;
}
// Restore protocols to the last global status.
// Used to reconnect on restore after standby.
static void RestoreMode()
{
	
	int nStatus;
	
	
	nStatus = DBGetContactSettingWord(NULL, "CList", "Status", ID_STATUS_OFFLINE);
	if (nStatus != ID_STATUS_OFFLINE)
		PostMessage(hwndContactList, WM_COMMAND, nStatus, 0);

}

int OnSettingChanging(WPARAM wParam,LPARAM lParam)
{
DBCONTACTWRITESETTING *dbcws=(DBCONTACTWRITESETTING *)lParam;
	if (wParam==0)
	{
		if ((dbcws->value.type==DBVT_BYTE)&&!strcmp(dbcws->szModule,"CLUI"))
		{
			if (!strcmp(dbcws->szSetting,"SBarShow"))
			{	
				showOpts=dbcws->value.bVal;	
				return(0);
			};
		}
	}
	else
	{		
		if (dbcws==NULL){return(0);};
		
		if (dbcws->value.type==DBVT_ASCIIZ&&!strcmp(dbcws->szSetting,"e-mail"))
		{
			SetAllExtraIcons(hwndContactTree,(HANDLE)wParam);
			return(0);
		};
		if (dbcws->value.type==DBVT_ASCIIZ&&!strcmp(dbcws->szSetting,"Cellular"))
		{		
			SetAllExtraIcons(hwndContactTree,(HANDLE)wParam);
			return(0);
		};
		
		if (dbcws->value.type==DBVT_ASCIIZ&&!strcmp(dbcws->szModule,"UserInfo"))
		{
			if (!strcmp(dbcws->szSetting,(HANDLE)"MyPhone0"))
			{		
				SetAllExtraIcons(hwndContactTree,(HANDLE)wParam);
				return(0);
			};
			if (!strcmp(dbcws->szSetting,(HANDLE)"Mye-mail0"))
			{	
				SetAllExtraIcons(hwndContactTree,(HANDLE)wParam);	
				return(0);
			};
		};
	}
	return(0);
};


// Disconnect all protocols.
// Happens on shutdown and standby.
static void DisconnectAll()
{

	PROTOCOLDESCRIPTOR** ppProtoDesc;
	int nProtoCount;
	int nProto;


	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&nProtoCount, (LPARAM)&ppProtoDesc);
	for (nProto = 0; nProto < nProtoCount; nProto++)
	{
		if (ppProtoDesc[nProto]->type == PROTOTYPE_PROTOCOL)
			CallProtoService(ppProtoDesc[nProto]->szName, PS_SETSTATUS, ID_STATUS_OFFLINE, 0);
	}
	
}

int PreCreateCLC(HWND parent)
{
			hwndContactTree=CreateWindow(CLISTCONTROL_CLASS,"",
					WS_CHILD|WS_CLIPCHILDREN|CLS_CONTACTLIST
					|(DBGetContactSettingByte(NULL,"CList","UseGroups",SETTING_USEGROUPS_DEFAULT)?CLS_USEGROUPS:0)
					|CLS_HIDEOFFLINE
					//|(DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT)?CLS_HIDEOFFLINE:0)
					|(DBGetContactSettingByte(NULL,"CList","HideEmptyGroups",SETTING_HIDEEMPTYGROUPS_DEFAULT)?CLS_HIDEEMPTYGROUPS:0
					|CLS_MULTICOLUMN
					),
					0,0,0,0,parent,NULL,g_hInst,NULL);
	
	return((int)hwndContactTree);
};

int CreateCLC(HWND parent)
{
			//SendMessage(hwndContactList,WM_SIZE,0,0);
			Sleep(0);
	{		
	   // create contact list frame
		CLISTFrame Frame;
		memset(&Frame,0,sizeof(Frame));
		Frame.cbSize=sizeof(CLISTFrame);
		Frame.hWnd=hwndContactTree;
		Frame.align=alClient;
		Frame.hIcon=LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
			//LoadIcon(hInst,MAKEINTRESOURCE(IDI_MIRANDA));
		Frame.Flags=F_VISIBLE|F_SHOWTB|F_SHOWTBTIP;
		Frame.name=(Translate("My Contacts"));
		hFrameContactTree=(HWND)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,(LPARAM)0);
		//free(Frame.name);
	CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_TBTIPNAME,hFrameContactTree),(LPARAM)Translate("My Contacts"));	
	}
	
	ReloadExtraIcons();
	{
		lastreqh=0;
		{
			CallService(MS_CLIST_SETHIDEOFFLINE,(WPARAM)oldhideoffline,0);
		}

		{	int state=DBGetContactSettingByte(NULL,"CList","State",SETTING_STATE_NORMAL);
			if(state==SETTING_STATE_NORMAL) ShowWindow(hwndContactList, SW_SHOW);
			else if(state==SETTING_STATE_MINIMIZED) ShowWindow(hwndContactList, SW_SHOWMINIMIZED);
		}
		
		lastreqh=0;
		disableautoupd=0;
	
	}
hSettingChangedHook=HookEvent(ME_DB_CONTACT_SETTINGCHANGED,OnSettingChanging);
return(0);
};


int GetStatsuBarProtoRect(HWND hwnd,char *szProto,RECT *rc)
{
	int nParts,nPanel;
	ProtocolData *PD;
	
	nParts=SendMessage(hwnd,SB_GETPARTS,0,0);
	FillMemory(rc,sizeof(RECT),0);

    for (nPanel=0;nPanel<nParts;nPanel++)
	{
	PD=(ProtocolData *)SendMessage(hwndStatus,SB_GETTEXT,(WPARAM)nPanel,(LPARAM)0);
	if(PD==NULL){
		return(0);
	};
	
	
	if (!strcmp(szProto,PD->RealName))
		{
			SendMessage(hwnd,SB_GETRECT,(WPARAM)nPanel,(LPARAM)rc);
			return(0);
		};
	};
return (0);
};



LRESULT CALLBACK ContactListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{     
	{	LRESULT result;
	MSG m;
	m.hwnd=hwnd;
	m.message=msg;
	m.wParam=wParam;
	m.lParam=lParam;
	if(CallService(MS_CLIST_DOCKINGPROCESSMESSAGE,(WPARAM)&m,(LPARAM)&result))
		return result;
	if(CallService(MS_CLIST_TRAYICONPROCESSMESSAGE,(WPARAM)&m,(LPARAM)&result))
		return result;
	if(CallService(MS_CLIST_HOTKEYSPROCESSMESSAGE,(WPARAM)&m,(LPARAM)&result))
		return result;
	}

	if ( msg == uMsgProcessProfile ) {
		char profile[MAX_PATH];
		int rc;
		// wParam = (ATOM)hProfileAtom, lParam = 0
		if ( GlobalGetAtomName((ATOM)wParam, profile, sizeof(profile)) ) {
			char path[MAX_PATH];
			char file[MAX_PATH];
			char p[MAX_PATH];
			CallService(MS_DB_GETPROFILEPATH,sizeof(path),(LPARAM)&path);
			CallService(MS_DB_GETPROFILENAME,sizeof(file),(LPARAM)&file);
			_snprintf(p,sizeof(p),"%s\\%s",path,file);
			rc=lstrcmp(profile,p) == 0;
			ReplyMessage(rc);
			if ( rc ) ShowWindowAsync(hwnd,SW_SHOW);
		}
		return 0;
	};

	/*
	This registers a window message with RegisterWindowMessage() and then waits for such a message,
	if it gets it, it tries to open a file mapping object and then maps it to this process space,
	it expects 256 bytes of data (incl. NULL) it will then write back the profile it is using the DB to fill in the answer.
	
	  The caller is expected to create this mapping object and tell us the ID we need to open ours.	
	*/
	if (msg==hMsgGetProfile && wParam != 0) { /* got IPC message */
		HANDLE hMap;
		char szName[MAX_PATH];
		int rc=0;
		_snprintf(szName,sizeof(szName),"Miranda::%u", wParam); // caller will tell us the ID of the map
		hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,szName);
		if (hMap != NULL) {
			void *hView=NULL;
			hView=MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, MAX_PATH);
			if (hView) {
				char szFilePath[MAX_PATH], szProfile[MAX_PATH];
				CallService(MS_DB_GETPROFILEPATH,MAX_PATH,(LPARAM)&szFilePath);
				CallService(MS_DB_GETPROFILENAME,MAX_PATH,(LPARAM)&szProfile);
				_snprintf(hView,MAX_PATH,"%s\\%s",szFilePath,szProfile);
				UnmapViewOfFile(hView);
				rc=1;
			}
			CloseHandle(hMap);
		}
		return rc;
	}
    switch (msg)
    {
		case WM_INITMENU:
			{
				if (ServiceExists(MS_CLIST_MENUBUILDMAIN)){CallService(MS_CLIST_MENUBUILDMAIN,0,0);};
			return(0);
			};

	case WM_CREATE:
		CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)GetMenu(hwnd),0);
		DrawMenuBar(hwnd);
		showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",1);		

		//create the status wnd
		//hwndStatus = CreateStatusWindow(WS_CHILD | (DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)?WS_VISIBLE:0), "", hwnd, 0);
		CluiProtocolStatusChanged(0,0);
		
		{	MENUITEMINFO mii;
		ZeroMemory(&mii,sizeof(mii));
		mii.cbSize=MENUITEMINFO_V4_SIZE;
		mii.fMask=MIIM_TYPE|MIIM_DATA;
		himlMirandaIcon=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,1,1);
		ImageList_AddIcon(himlMirandaIcon,LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
		mii.dwItemData=MENU_MIRANDAMENU;
		mii.fType=MFT_OWNERDRAW;
		mii.dwTypeData=NULL;
		SetMenuItemInfo(GetMenu(hwnd),0,TRUE,&mii);
		}
		
		//delay creation of CLC so that it can get the status icons right the first time (needs protocol modules loaded)
		//PostMessage(hwnd,M_CREATECLC,0,0);
		
		hMsgGetProfile=RegisterWindowMessage("Miranda::GetProfile"); // don't localise
		
		if (DBGetContactSettingByte(NULL,"CList","Transparent",0))
		{
			SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
			if (MySetLayeredWindowAttributes) MySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_ALPHA_DEFAULT), LWA_ALPHA);
		}
		transparentFocus=1;

	
/*
#ifndef _DEBUG
		// Miranda is starting up! Restore last status mode.
		// This is not done in debug builds because frequent
		// reconnections will get you banned from the servers.
#endif
*/
		{
			int nStatus;	
			
			nStatus = DBGetContactSettingWord(NULL, "CList", "Status", ID_STATUS_OFFLINE);
			if (nStatus != ID_STATUS_OFFLINE)
				PostMessage(hwnd, WM_COMMAND, nStatus, 0);
		}

		return FALSE;

	case M_SETALLEXTRAICONS:
		
		break;
	case M_CREATECLC:
			CreateCLC(hwnd);
		break;

		// Power management
	case WM_POWERBROADCAST:
		{
			switch ((DWORD)wParam)
			{
				
			case PBT_APMSUSPEND:
				// Computer is suspending, disconnect all protocols
				DisconnectAll();
				break;
				
			case PBT_APMRESUMESUSPEND:
				// Computer is resuming, restore all protocols
				RestoreMode();
				break;
				
			}
		}
		break;

	case WM_SYSCOLORCHANGE:
		SendMessage(hwndContactTree,msg,wParam,lParam);
		SendMessage(hwndStatus,msg,wParam,lParam);
		// XXX: only works with 4.71 with 95, IE4.
		SendMessage(hwndStatus,SB_SETBKCOLOR, 0, GetSysColor(COLOR_3DFACE));
		break;
		
	case WM_SIZE:
		{
				RECT rc;


				if(hwndContactList!=NULL){
					CLUIFramesOnClistResize((WPARAM)hwnd,(LPARAM)0);
				};
				GetWindowRect(hwnd, &rc);		
			if(!CallService(MS_CLIST_DOCKINGISDOCKED,0,0))
				{ //if docked, dont remember pos (except for width)
					DBWriteContactSettingDword(NULL,"CList","Height",(DWORD)(rc.bottom - rc.top));
					DBWriteContactSettingDword(NULL,"CList","x",(DWORD)rc.left);
					DBWriteContactSettingDword(NULL,"CList","y",(DWORD)rc.top);
				}
				DBWriteContactSettingDword(NULL,"CList","Width",(DWORD)(rc.right - rc.left));

			
			if(wParam==SIZE_MINIMIZED) {
				if(DBGetContactSettingByte(NULL,"CList","Min2Tray",SETTING_MIN2TRAY_DEFAULT)) {
					ShowWindow(hwnd, SW_HIDE);
					DBWriteContactSettingByte(NULL,"CList","State",SETTING_STATE_HIDDEN);
				}
				else DBWriteContactSettingByte(NULL,"CList","State",SETTING_STATE_MINIMIZED);
			}
			return(0);
		}
			// drop thru
		case WM_MOVE:
			if(!IsIconic(hwnd)) {
				RECT rc;
				GetWindowRect(hwnd, &rc);
				
				if(!CallService(MS_CLIST_DOCKINGISDOCKED,0,0))
				{ //if docked, dont remember pos (except for width)
					DBWriteContactSettingDword(NULL,"CList","Height",(DWORD)(rc.bottom - rc.top));
					DBWriteContactSettingDword(NULL,"CList","x",(DWORD)rc.left);
					DBWriteContactSettingDword(NULL,"CList","y",(DWORD)rc.top);
				}
				DBWriteContactSettingDword(NULL,"CList","Width",(DWORD)(rc.right - rc.left));
			}
			return FALSE;
			
		case WM_SETFOCUS:
				{	
				boolean isfloating;
				if (hFrameContactTree)
						{					
							isfloating=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLOATING,hFrameContactTree),0);
							if (isfloating==FALSE)
							{
								SetFocus(hwndContactTree);
							};
						}
				}
			//SetFocus(hwndContactTree);
			return 0;
			
		case WM_ACTIVATE:
			if(wParam==WA_INACTIVE) {
				if((HWND)wParam!=hwnd)
					if(DBGetContactSettingByte(NULL,"CList","Transparent",SETTING_TRANSPARENT_DEFAULT))
						if(transparentFocus)
							SetTimer(hwnd, TM_AUTOALPHA,250,NULL);
			}
			else {
				if(DBGetContactSettingByte(NULL,"CList","Transparent",SETTING_TRANSPARENT_DEFAULT)) {
					KillTimer(hwnd,TM_AUTOALPHA);
					if (MySetLayeredWindowAttributes) MySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_ALPHA_DEFAULT), LWA_ALPHA);
					transparentFocus=1;
				}
			}
			return DefWindowProc(hwnd,msg,wParam,lParam);
			
		case WM_SETCURSOR:
			if(DBGetContactSettingByte(NULL,"CList","Transparent",SETTING_TRANSPARENT_DEFAULT)) {
				if (!transparentFocus && GetForegroundWindow()!=hwnd && MySetLayeredWindowAttributes) {
					MySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_ALPHA_DEFAULT), LWA_ALPHA);
					transparentFocus=1;
					SetTimer(hwnd, TM_AUTOALPHA,250,NULL);
				}
			}
			return DefWindowProc(hwnd,msg,wParam,lParam);
			
		case WM_NCHITTEST:
			{	LRESULT result;
			result=DefWindowProc(hwnd,WM_NCHITTEST,wParam,lParam);
			if(result==HTSIZE || result==HTTOP || result==HTTOPLEFT || result==HTTOPRIGHT ||
				result==HTBOTTOM || result==HTBOTTOMRIGHT || result==HTBOTTOMLEFT)
				if(DBGetContactSettingByte(NULL,"CLUI","AutoSize",0)) return HTCLIENT;
				return result;
			}
			
		case WM_TIMER:
			

			if ((int)wParam>=TM_STATUSBARUPDATE&&(int)wParam<=TM_STATUSBARUPDATE+64)
			{
					
					int status,i;

					ProtoTicks *pt=NULL;
					for (i=0;i<64;i++)
					{
					
					pt=&CycleStartTick[i];

					if (pt->szProto!=NULL&&pt->TimerCreated==1)
					{
					
							status=CallProtoService(pt->szProto,PS_GETSTATUS,0,0);

							if (!(status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES))
							{													
									pt->CycleStartTick=0;
										KillTimer(hwnd,TM_STATUSBARUPDATE+pt->n);
										pt->TimerCreated=0;
							}
					}

					};

					pt=&CycleStartTick[wParam-TM_STATUSBARUPDATE];
					{
					RECT rc;
					GetStatsuBarProtoRect(hwndStatus,pt->szProto,&rc);
					rc.right=rc.left+GetSystemMetrics(SM_CXSMICON)+1;
#ifdef _DEBUG
				{
					//char buf[512];
					//sprintf(buf,"Invalidate left: %d right: %d\r\n",rc.left,rc.right);
					//OutputDebugString(buf);
				}
#endif

					if(IsWindowVisible(hwndStatus)) InvalidateRect(hwndStatus,&rc,TRUE);
					TrayIconUpdateBase(pt->szProto);
					}
					//SendMessage(hwndStatus,WM_PAINT,0,0);
					UpdateWindow(hwndStatus);
				break;
			}
			
			if ((int)wParam==TM_AUTOALPHA)
			{	int inwnd;
			
			if (GetForegroundWindow()==hwnd) {
				KillTimer(hwnd,TM_AUTOALPHA);
				inwnd=1;
			}
			else {
				POINT pt;
				HWND hwndPt;
				pt.x=(short)LOWORD(GetMessagePos());
				pt.y=(short)HIWORD(GetMessagePos());
				hwndPt=WindowFromPoint(pt);
				inwnd=(hwndPt==hwnd || GetParent(hwndPt)==hwnd);
			}
			if (inwnd!=transparentFocus && MySetLayeredWindowAttributes)
			{ //change
				transparentFocus=inwnd;
				if(transparentFocus) MySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_ALPHA_DEFAULT), LWA_ALPHA);
				else MySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)DBGetContactSettingByte(NULL,"CList","AutoAlpha",SETTING_AUTOALPHA_DEFAULT), LWA_ALPHA);
			}
			if(!transparentFocus) KillTimer(hwnd,TM_AUTOALPHA);
			}
			return TRUE;
			
		case WM_SHOWWINDOW:
			{	static int noRecurse=0;
			if(lParam) break;
			if(noRecurse) break;
			if(!DBGetContactSettingByte(NULL,"CLUI","FadeInOut",0) || !IsWinVer2000Plus()) break;
			if(GetWindowLong(hwnd,GWL_EXSTYLE)&WS_EX_LAYERED) {
				DWORD thisTick,startTick;
				int sourceAlpha,destAlpha;
				if(wParam) {
					sourceAlpha=0;
					destAlpha=(BYTE)DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_AUTOALPHA_DEFAULT);
					MySetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_ALPHA);
					noRecurse=1;
					ShowWindow(hwnd,SW_SHOW);
					noRecurse=0;
				}
				else {
					sourceAlpha=(BYTE)DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_AUTOALPHA_DEFAULT);
					destAlpha=0;
				}
				for(startTick=GetTickCount();;) {
					thisTick=GetTickCount();
					if(thisTick>=startTick+200) break;
					MySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)(sourceAlpha+(destAlpha-sourceAlpha)*(int)(thisTick-startTick)/200), LWA_ALPHA);
				}
				MySetLayeredWindowAttributes(hwnd, RGB(0,0,0), (BYTE)destAlpha, LWA_ALPHA);
			}
			else {
				if(wParam) SetForegroundWindow(hwnd);
				MyAnimateWindow(hwnd,200,AW_BLEND|(wParam?0:AW_HIDE));
				//SetWindowPos(hwndContactTree,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
				CLUIFramesUpdateFrame(-1,0);
			}
			break;
			}
		case WM_MENURBUTTONUP: /* this API is so badly documented at MSDN!! */
			{
				UINT id=0;
				
				id=GetMenuItemID((HMENU)lParam,LOWORD(wParam)); /* LOWORD(wParam) contains the menu pos in its parent menu */
				if (id != (-1)) SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(id,0),0);
				break;
			}
		case WM_SYSCOMMAND:
			if(wParam==SC_MAXIMIZE) return 0;
			return DefWindowProc(hwnd, msg, wParam, lParam);
			
		case WM_COMMAND:

 			if(CallService(MS_CLIST_MENUPROCESSCOMMAND,MAKEWPARAM(LOWORD(wParam),MPCF_MAINMENU),(LPARAM)(HANDLE)NULL)) break;
			switch (LOWORD(wParam))
			{
			case ID_TRAY_EXIT:
			case ID_ICQ_EXIT:
				if(CallService(MS_SYSTEM_OKTOEXIT,0,0))
					DestroyWindow(hwnd);
				break;
			case ID_TRAY_HIDE:
				CallService(MS_CLIST_SHOWHIDE,0,0);
				break;
			case POPUP_NEWGROUP:
				SendMessage(hwndContactTree,CLM_SETHIDEEMPTYGROUPS,0,0);
				CallService(MS_CLIST_GROUPCREATE,0,0);
				break;
			case POPUP_HIDEOFFLINE:
				CallService(MS_CLIST_SETHIDEOFFLINE,(WPARAM)(-1),0);
				break;
			case POPUP_HIDEOFFLINEROOT:
				SendMessage(hwndContactTree,CLM_SETHIDEOFFLINEROOT,!SendMessage(hwndContactTree,CLM_GETHIDEOFFLINEROOT,0,0),0);
				break;
			case POPUP_HIDEEMPTYGROUPS:
				{	int newVal=!(GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_HIDEEMPTYGROUPS);
				DBWriteContactSettingByte(NULL,"CList","HideEmptyGroups",(BYTE)newVal);
				SendMessage(hwndContactTree,CLM_SETHIDEEMPTYGROUPS,newVal,0);
				break;
				}
			case POPUP_DISABLEGROUPS:
				{	int newVal=!(GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_USEGROUPS);
				DBWriteContactSettingByte(NULL,"CList","UseGroups",(BYTE)newVal);
				SendMessage(hwndContactTree,CLM_SETUSEGROUPS,newVal,0);
				break;
				}
			case POPUP_HIDEMIRANDA:
				{
					CallService(MS_CLIST_SHOWHIDE,0,0);
					break;
				}
			}
			return FALSE;
			case WM_KEYDOWN:
				CallService(MS_CLIST_MENUPROCESSHOTKEY,wParam,MPCF_MAINMENU|MPCF_CONTACTMENU);
				if (wParam==VK_F5)
				{
					SendMessage(hwndContactTree,CLM_AUTOREBUILD,0,0);
				};
				break;
				
			case WM_GETMINMAXINFO:
				DefWindowProc(hwnd,msg,wParam,lParam);
				((LPMINMAXINFO)lParam)->ptMinTrackSize.x=18;
				if (requr==0){((LPMINMAXINFO)lParam)->ptMinTrackSize.y=CLUIFramesGetMinHeight();};
				return 0;
				
			case WM_DISPLAYCHANGE:
				SendMessage(hwndContactTree,WM_SIZE,0,0);	//forces it to send a cln_listsizechanged
				break;
				
				//MSG FROM CHILD CONTROL
			case WM_NOTIFY:
				if (((LPNMHDR)lParam)->hwndFrom == hwndContactTree)
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case CLN_NEWCONTACT:
						{
							NMCLISTCONTROL *nm=(NMCLISTCONTROL *)lParam;
							if (nm!=NULL) SetAllExtraIcons(hwndContactTree,nm->hItem );
							break;
						};
					case CLN_LISTREBUILT:
						{
							SetAllExtraIcons(hwndContactTree,0);
							return(FALSE);
						}								
					case CLN_EXPANDED:
						{
							NMCLISTCONTROL *nmc=(NMCLISTCONTROL*)lParam;
							CallService(MS_CLIST_GROUPSETEXPANDED,(WPARAM)nmc->hItem,nmc->action);
							return FALSE;
						}
					case CLN_DRAGGING:
						{
							NMCLISTCONTROL *nmc=(NMCLISTCONTROL*)lParam;
							ClientToScreen(hwnd, &nmc->pt);
							if(!(nmc->flags&CLNF_ISGROUP))
								if(NotifyEventHooks(hContactDraggingEvent,(WPARAM)nmc->hItem,MAKELPARAM(nmc->pt.x,nmc->pt.y))) {
									SetCursor(LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_DROPUSER)));
									return TRUE;
								}
								break;
						}
					case CLN_DRAGSTOP:
						{
							NMCLISTCONTROL *nmc=(NMCLISTCONTROL*)lParam;
							if(!(nmc->flags&CLNF_ISGROUP))
								NotifyEventHooks(hContactDragStopEvent,(WPARAM)nmc->hItem,0);
							break;
						}
					case CLN_DROPPED:
						{
							NMCLISTCONTROL *nmc=(NMCLISTCONTROL*)lParam;
							ClientToScreen(hwnd, &nmc->pt);
							if(!(nmc->flags&CLNF_ISGROUP))
								if(NotifyEventHooks(hContactDroppedEvent,(WPARAM)nmc->hItem,MAKELPARAM(nmc->pt.x,nmc->pt.y))) {
									SetCursor(LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_DROPUSER)));
									return TRUE;
								}
								break;
						}
					case NM_KEYDOWN:
						{	NMKEY *nmkey=(NMKEY*)lParam;
						return CallService(MS_CLIST_MENUPROCESSHOTKEY,nmkey->nVKey,MPCF_MAINMENU|MPCF_CONTACTMENU);
						}
					case CLN_LISTSIZECHANGE:
						{

						NMCLISTCONTROL *nmc=(NMCLISTCONTROL*)lParam;
						RECT rcWindow,rcTree,rcWorkArea;
						int maxHeight,newHeight;

						if (disableautoupd==1){break;};
						if(!DBGetContactSettingByte(NULL,"CLUI","AutoSize",0)) break;

						
						if(CallService(MS_CLIST_DOCKINGISDOCKED,0,0)) break;
						if (hFrameContactTree==0)break;
						maxHeight=DBGetContactSettingByte(NULL,"CLUI","MaxSizeHeight",75);
						GetWindowRect(hwnd,&rcWindow);
						GetWindowRect(hwndContactTree,&rcTree);
						SystemParametersInfo(SPI_GETWORKAREA,0,&rcWorkArea,FALSE);
						if (nmc->pt.y>(rcWorkArea.bottom-rcWorkArea.top)) 
						{
							break;
						};
						if ((nmc->pt.y)==lastreqh)
						{
						//	break;
						}
						lastreqh=nmc->pt.y;
						newHeight=max(nmc->pt.y,3)+1+(rcWindow.bottom-rcWindow.top)-(rcTree.bottom-rcTree.top);
						if (newHeight==(rcWindow.bottom-rcWindow.top)) break;

						if(newHeight>(rcWorkArea.bottom-rcWorkArea.top)*maxHeight/100)
							newHeight=(rcWorkArea.bottom-rcWorkArea.top)*maxHeight/100;
						if(DBGetContactSettingByte(NULL,"CLUI","AutoSizeUpward",0)) {
							rcWindow.top=rcWindow.bottom-newHeight;
							if(rcWindow.top<rcWorkArea.top) rcWindow.top=rcWorkArea.top;
						}
						else {
						 	rcWindow.bottom=rcWindow.top+newHeight;
							if(rcWindow.bottom>rcWorkArea.bottom) rcWindow.bottom=rcWorkArea.bottom;
						}
						if (requr==1){break;};
						requr=1;					
						SetWindowPos(hwnd,0,rcWindow.left,rcWindow.top,rcWindow.right-rcWindow.left,rcWindow.bottom-rcWindow.top,SWP_NOZORDER|SWP_NOACTIVATE);
						GetWindowRect(hwnd,&rcWindow);
						requr=0;
						
						break;

/*
							NMCLISTCONTROL *nmc=(NMCLISTCONTROL*)lParam;
							RECT rcWindow,rcTree,rcWorkArea;
							int maxHeight,newHeight;
							
							if(!DBGetContactSettingByte(NULL,"CLUI","AutoSize",0)) break;
							if(CallService(MS_CLIST_DOCKINGISDOCKED,0,0)) break;
							maxHeight=DBGetContactSettingByte(NULL,"CLUI","MaxSizeHeight",75);
							GetWindowRect(hwnd,&rcWindow);
							GetWindowRect(hwndContactTree,&rcTree);
							SystemParametersInfo(SPI_GETWORKAREA,0,&rcWorkArea,FALSE);
							newHeight=max(nmc->pt.y,9)+1+(rcWindow.bottom-rcWindow.top)-(rcTree.bottom-rcTree.top);
							if(newHeight>(rcWorkArea.bottom-rcWorkArea.top)*maxHeight/100)
								newHeight=(rcWorkArea.bottom-rcWorkArea.top)*maxHeight/100;
							if(DBGetContactSettingByte(NULL,"CLUI","AutoSizeUpward",0)) {
								rcWindow.top=rcWindow.bottom-newHeight;
								if(rcWindow.top<rcWorkArea.top) rcWindow.top=rcWorkArea.top;
							}
							else {
								rcWindow.bottom=rcWindow.top+newHeight;
								if(rcWindow.bottom>rcWorkArea.bottom) rcWindow.bottom=rcWorkArea.bottom;
							}
							SetWindowPos(hwnd,0,rcWindow.left,rcWindow.top,rcWindow.right-rcWindow.left,rcWindow.bottom-rcWindow.top,SWP_NOZORDER|SWP_NOACTIVATE);
							break;
*/
						}
					case NM_CLICK:
						{	NMCLISTCONTROL *nm=(NMCLISTCONTROL*)lParam;
						DWORD hitFlags;
						HANDLE hItem;
						
						hItem=(HANDLE)SendMessage(hwndContactTree,CLM_HITTEST,(WPARAM)&hitFlags,MAKELPARAM(nm->pt.x,nm->pt.y));

							if (hitFlags&CLCHT_ONITEMEXTRA)
							{					
								int v,e;
									v=ExtraToColumnNum(EXTRA_ICON_PROTO);
									e=ExtraToColumnNum(EXTRA_ICON_EMAIL);
									if (!IsHContactGroup(hItem)&&!IsHContactInfo(hItem))
									{
										if(nm->iColumn==v) {
												CallService(MS_USERINFO_SHOWDIALOG,(WPARAM)nm->hItem,0);
											};
										if(nm->iColumn==e) {
												//CallService(MS_USERINFO_SHOWDIALOG,(WPARAM)nm->hItem,0);
												char *email,buf[4096];
												email=DBGetString(nm->hItem,"UserInfo", "Mye-mail0");
												
												
												if (email)
												{
													sprintf(buf,"mailto:%s",email);
													ShellExecute(hwnd,"open",buf,NULL,NULL,SW_SHOW);
												}
												
											};										
								}
							};	
						if(hItem) break;
						if((hitFlags&(CLCHT_NOWHERE|CLCHT_INLEFTMARGIN|CLCHT_BELOWITEMS))==0) break;
						if (DBGetContactSettingByte(NULL,"CLUI","ClientAreaDrag",SETTING_CLIENTDRAG_DEFAULT)) {
							POINT pt;
							pt=nm->pt;
							ClientToScreen(hwndContactTree,&pt);
							return SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE|HTCAPTION,MAKELPARAM(pt.x,pt.y));
						}
						break;
						}
					}
				}
				else if(((LPNMHDR)lParam)->hwndFrom==hwndStatus) {
					switch (((LPNMHDR)lParam)->code) {
					case NM_CLICK:
						{
							unsigned int nParts, nPanel;
							NMMOUSE *nm=(NMMOUSE*)lParam;
							HMENU hMenu;
							RECT rc;
							POINT pt;
							int totcount;
							ProtocolData *PD;
							int menuid;
							
							hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);
							nParts=SendMessage(hwndStatus,SB_GETPARTS,0,0);
							if (nm->dwItemSpec==0xFFFFFFFE) {
								nPanel=nParts-1;
								SendMessage(hwndStatus,SB_GETRECT,nPanel,(LPARAM)&rc);
								if (nm->pt.x < rc.left) return FALSE;
						} else { 
								nPanel=nm->dwItemSpec; 
								SendMessage(hwndStatus,SB_GETRECT,nPanel,(LPARAM)&rc);
								}
						
						//if (nParts>1) hMenu=GetSubMenu(hMenu,nPanel);

								totcount=DBGetContactSettingDword(0,"Protocols","ProtoCount",0);
//							nParts=SendMessage(hwndStatus,SB_GETPARTS,0,0);
								PD=(ProtocolData *)SendMessage(hwndStatus,SB_GETTEXT,(WPARAM)nPanel,(LPARAM)0);
								if(PD==NULL){return(0);};
								menuid=PD->protopos;
								menuid=totcount-menuid-1;
								if (menuid<0){break;};
								hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);
								if(GetSubMenu(hMenu,menuid)) hMenu=GetSubMenu(hMenu,menuid);
						if (hMenu!=NULL)				
						{						
							pt.x=rc.left;
							pt.y=rc.top;
							ClientToScreen(hwndStatus,&pt);
							TrackPopupMenu(hMenu,TPM_BOTTOMALIGN|TPM_LEFTALIGN,pt.x,pt.y,0,hwnd,NULL);
						}
						}
					}
				}
				return FALSE;
		case WM_CONTEXTMENU:
			{	RECT rc;
			POINT pt;

			pt.x=(short)LOWORD(lParam);
			pt.y=(short)HIWORD(lParam);
			// x/y might be -1 if it was generated by a kb click			
			GetWindowRect(hwndContactTree,&rc);
			if ( pt.x == -1 && pt.y == -1) {
				// all this is done in screen-coords!
				GetCursorPos(&pt);
				// the mouse isnt near the window, so put it in the middle of the window
				if (!PtInRect(&rc,pt)) {
					pt.x = rc.left + (rc.right - rc.left) / 2;
					pt.y = rc.top + (rc.bottom - rc.top) / 2; 
				}
			}
			if(PtInRect(&rc,pt)) {
				HMENU hMenu;
				/*
				hMenu=GetSubMenu(LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_CONTEXT)),1);
				CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hMenu,0);
				CheckMenuItem(hMenu,POPUP_HIDEOFFLINE,DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT)?MF_CHECKED:MF_UNCHECKED);
				CheckMenuItem(hMenu,POPUP_HIDEOFFLINEROOT,SendMessage(hwndContactTree,CLM_GETHIDEOFFLINEROOT,0,0)?MF_CHECKED:MF_UNCHECKED);
				CheckMenuItem(hMenu,POPUP_HIDEEMPTYGROUPS,GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_HIDEEMPTYGROUPS?MF_CHECKED:MF_UNCHECKED);
				CheckMenuItem(hMenu,POPUP_DISABLEGROUPS,GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_USEGROUPS?MF_UNCHECKED:MF_CHECKED);
				TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL);
				*/
				hMenu=(HMENU)CallService(MS_CLIST_MENUBUILDGROUP,0,0);
				TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL);

				//DestroyMenu(hMenu);
				return 0;
			}
			GetWindowRect(hwndStatus,&rc);
			if(PtInRect(&rc,pt)) {
				HMENU hMenu;
				if(DBGetContactSettingByte(NULL,"CLUI","SBarRightClk",0))
					hMenu=(HMENU)CallService(MS_CLIST_MENUGETMAIN,0,0);
				else
					hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);
				TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL);
				return 0;
			}
			}
			break;
			
		case WM_MEASUREITEM:
			if(((LPMEASUREITEMSTRUCT)lParam)->itemData==MENU_MIRANDAMENU) {
				((LPMEASUREITEMSTRUCT)lParam)->itemWidth=GetSystemMetrics(SM_CXSMICON)*4/3;
				((LPMEASUREITEMSTRUCT)lParam)->itemHeight=0;
				return TRUE;
			}
			return CallService(MS_CLIST_MENUMEASUREITEM,wParam,lParam);
		case WM_DRAWITEM:
			{	LPDRAWITEMSTRUCT dis=(LPDRAWITEMSTRUCT)lParam;
			if(dis->hwndItem==hwndStatus/*&&IsWindowVisible(hwndStatus)*/) {
				ProtocolData *PD=(ProtocolData *)dis->itemData;
				char *szProto=(char*)dis->itemData;
				int status,x;
				SIZE textSize;
				boolean NeedDestroy=FALSE;
				HICON hIcon;

				if (PD==NULL){break;};					
				szProto=PD->RealName;
#ifdef _DEBUG
				{
					//char buf[512];
					//sprintf(buf,"proto: %s draw at pos: %d\r\n",szProto,dis->rcItem.left);
					//OutputDebugString(buf);
				}
#endif
				
				status=CallProtoService(szProto,PS_GETSTATUS,0,0);
				SetBkMode(dis->hDC,TRANSPARENT);
				x=dis->rcItem.left;

				if(showOpts&1) {
					
					//char buf [256];
					
					if ((DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",1)==1)&&status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
						{
						hIcon=(HICON)GetConnectingIconService((WPARAM)szProto,0);

							if (hIcon)
							{
									NeedDestroy=TRUE;
							}else
							{
								hIcon=LoadSkinnedProtoIcon(szProto,status);
							}
							
					}else					
					{				
					hIcon=LoadSkinnedProtoIcon(szProto,status);
					}
					DrawIconEx(dis->hDC,x,(dis->rcItem.top+dis->rcItem.bottom-GetSystemMetrics(SM_CYSMICON))>>1,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
 					if (NeedDestroy) DestroyIcon(hIcon);
					x+=GetSystemMetrics(SM_CXSMICON)+2;
				}
				else x+=2;
				if(showOpts&2) {
					char szName[64];
					szName[0]=0;
					if (CallProtoService(szProto,PS_GETNAME,sizeof(szName),(LPARAM)szName)) {
						strcpy(szName,szProto);
					} //if
					if(lstrlen(szName)<sizeof(szName)-1) lstrcat(szName," ");
					GetTextExtentPoint32(dis->hDC,szName,lstrlen(szName),&textSize);
					TextOut(dis->hDC,x,(dis->rcItem.top+dis->rcItem.bottom-textSize.cy)>>1,szName,lstrlen(szName));
					x+=textSize.cx;
				}
				if(showOpts&4) {
					char *szStatus;
					szStatus=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,status,0);
					if (!szStatus) szStatus="";
					GetTextExtentPoint32(dis->hDC,szStatus,lstrlen(szStatus),&textSize);
					TextOut(dis->hDC,x,(dis->rcItem.top+dis->rcItem.bottom-textSize.cy)>>1,szStatus,lstrlen(szStatus));
					
				}

			}
			else if(dis->CtlType==ODT_MENU) {
				if(dis->itemData==MENU_MIRANDAMENU) {
					HICON hIcon=ImageList_GetIcon(himlMirandaIcon,0,ILD_NORMAL);
					if (!IsWinVerXPPlus()) {
						FillRect(dis->hDC,&dis->rcItem,GetSysColorBrush(COLOR_MENU));
						if(dis->itemState&ODS_HOTLIGHT)
							DrawEdge(dis->hDC,&dis->rcItem,BDR_RAISEDINNER,BF_RECT);
						else if(dis->itemState&ODS_SELECTED)
							DrawEdge(dis->hDC,&dis->rcItem,BDR_SUNKENOUTER,BF_RECT);
						DrawState(dis->hDC,NULL,NULL,(LPARAM)hIcon,0,(dis->rcItem.right+dis->rcItem.left-GetSystemMetrics(SM_CXSMICON))/2+(dis->itemState&ODS_SELECTED?1:0),(dis->rcItem.bottom+dis->rcItem.top-GetSystemMetrics(SM_CYSMICON))/2+(dis->itemState&ODS_SELECTED?1:0),0,0,DST_ICON|(dis->itemState&ODS_INACTIVE?DSS_DISABLED:DSS_NORMAL));
					} else {
						HBRUSH hBr;
						BOOL bfm=FALSE;
						SystemParametersInfo(SPI_GETFLATMENU,0,&bfm,0);
						if (bfm) {
							/* flat menus: fill with COLOR_MENUHILIGHT and outline with COLOR_HIGHLIGHT, otherwise use COLOR_MENUBAR */
							if (dis->itemState&ODS_SELECTED || dis->itemState&ODS_HOTLIGHT) {
								/* selected or hot lighted, no difference */
								hBr=GetSysColorBrush(COLOR_MENUHILIGHT);
								FillRect(dis->hDC,&dis->rcItem,hBr);
								DeleteObject(hBr);
								/* draw the frame */
								hBr=GetSysColorBrush(COLOR_HIGHLIGHT);
								FrameRect(dis->hDC,&dis->rcItem,hBr);
								DeleteObject(hBr);
							} else {
								/* flush the DC with the menu bar colour (only supported on XP) and then draw the icon */
								hBr=GetSysColorBrush(COLOR_MENUBAR);
								FillRect(dis->hDC,&dis->rcItem,hBr);
								DeleteObject(hBr);
							} //if
							/* draw the icon */
							DrawState(dis->hDC,NULL,NULL,(LPARAM)hIcon,0,(dis->rcItem.right+dis->rcItem.left-GetSystemMetrics(SM_CXSMICON))/2+(dis->itemState&ODS_SELECTED?1:0),(dis->rcItem.bottom+dis->rcItem.top-GetSystemMetrics(SM_CYSMICON))/2+(dis->itemState&ODS_SELECTED?1:0),0,0,DST_ICON|(dis->itemState&ODS_INACTIVE?DSS_DISABLED:DSS_NORMAL));
						} else {
							/* non-flat menus, flush the DC with a normal menu colour */
							FillRect(dis->hDC,&dis->rcItem,GetSysColorBrush(COLOR_MENU));
							if(dis->itemState&ODS_HOTLIGHT) {
								DrawEdge(dis->hDC,&dis->rcItem,BDR_RAISEDINNER,BF_RECT);
							} else if(dis->itemState&ODS_SELECTED) {
								DrawEdge(dis->hDC,&dis->rcItem,BDR_SUNKENOUTER,BF_RECT);
							} //if
							DrawState(dis->hDC,NULL,NULL,(LPARAM)hIcon,0,(dis->rcItem.right+dis->rcItem.left-GetSystemMetrics(SM_CXSMICON))/2+(dis->itemState&ODS_SELECTED?1:0),(dis->rcItem.bottom+dis->rcItem.top-GetSystemMetrics(SM_CYSMICON))/2+(dis->itemState&ODS_SELECTED?1:0),0,0,DST_ICON|(dis->itemState&ODS_INACTIVE?DSS_DISABLED:DSS_NORMAL));
						} //if
					} //if
					DestroyIcon(hIcon);
					return TRUE;
				}
				return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);
			}
			return 0;
			}
		case WM_CLOSE:
			if(DBGetContactSettingByte(NULL,"CList","ToolWindow",SETTING_TOOLWINDOW_DEFAULT))
				CallService(MS_CLIST_SHOWHIDE,0,0);
			else
				SendMessage(hwnd,WM_COMMAND,ID_ICQ_EXIT,0);
			
			return FALSE;
		case WM_DESTROY:
			{
			
			//saving state
			int state=DBGetContactSettingByte(NULL,"CList","State",SETTING_STATE_NORMAL);				
			
			if(hSettingChangedHook!=0){UnhookEvent(hSettingChangedHook);};

			DisconnectAll();
			
			if (state==SETTING_STATE_NORMAL){ShowWindow(hwnd,SW_HIDE);};

			CallService(MS_CLIST_FRAMES_REMOVEFRAME,(WPARAM)hFrameContactTree,(LPARAM)0);
			DestroyWindow(hwndContactTree);


			UnLoadCLUIFramesModule();		
			
			ImageList_Destroy(himlMirandaIcon);
			DBWriteContactSettingByte(NULL,"CList","State",(BYTE)state);
			FreeLibrary(hUserDll);
			PostQuitMessage(0);

			hwndContactList=NULL;
			}
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return TRUE;
}

static int CluiIconsChanged(WPARAM wParam,LPARAM lParam)
{
	ImageList_ReplaceIcon(himlMirandaIcon,0,LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
	DrawMenuBar(hwndContactList);
	ReloadExtraIcons();
	SetAllExtraIcons(hwndContactTree,0);

	return 0;
}

static HANDLE hRenameMenuItem;

static int MenuItem_PreBuild(WPARAM wParam, LPARAM lParam) 
{
	char cls[128];
	HANDLE hItem;
	HWND hwndClist = GetFocus();
	CLISTMENUITEM mi;
	
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.flags = CMIM_FLAGS;
	GetClassName(hwndClist,cls,sizeof(cls));
	hwndClist = (!lstrcmp(CLISTCONTROL_CLASS,cls))?hwndClist:hwndContactList;
	hItem = (HANDLE)SendMessage(hwndClist,CLM_GETSELECTION,0,0);
	if (!hItem) {
		mi.flags = CMIM_FLAGS | CMIF_HIDDEN;
	}
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hRenameMenuItem, (LPARAM)&mi);
	return 0;
}

static int MenuItem_RenameContact(WPARAM wParam,LPARAM lParam)
{
	char cls[128];
	HANDLE hItem;
	HWND hwndClist = GetFocus();
	GetClassName(hwndClist,cls,sizeof(cls));
	// worst case scenario, the rename is sent to the main contact list
	hwndClist = (!lstrcmp(CLISTCONTROL_CLASS,cls))?hwndClist:hwndContactList;
	hItem = (HANDLE)SendMessage(hwndClist,CLM_GETSELECTION,0,0);
	if(hItem) {
		SetFocus(hwndClist);
		SendMessage(hwndClist,CLM_EDITLABEL,(WPARAM)hItem,0);
	}
	return 0;
}

int LoadCLUIModule(void)
{
	WNDCLASS wndclass;
	DBVARIANT dbv;
	char titleText[256];
	ATOM	a;
	int laster;
	canloadstatusbar=FALSE;
	hFrameContactTree=0;
	
	
	hUserDll = LoadLibrary("user32.dll");
	if (hUserDll) {
		MySetLayeredWindowAttributes = (BOOL (WINAPI *)(HWND,COLORREF,BYTE,DWORD))GetProcAddress(hUserDll, "SetLayeredWindowAttributes");
		MyAnimateWindow=(BOOL (WINAPI*)(HWND,DWORD,DWORD))GetProcAddress(hUserDll,"AnimateWindow");
	}
	uMsgProcessProfile=RegisterWindowMessage("Miranda::ProcessProfile");

	HookEvent(ME_SYSTEM_MODULESLOADED,CluiModulesLoaded);
	HookEvent(ME_SKIN_ICONSCHANGED,CluiIconsChanged);
	HookEvent(ME_OPT_INITIALISE,CluiOptInit);
	hContactDraggingEvent=CreateHookableEvent(ME_CLUI_CONTACTDRAGGING);
	hContactDroppedEvent=CreateHookableEvent(ME_CLUI_CONTACTDROPPED);
	hContactDragStopEvent=CreateHookableEvent(ME_CLUI_CONTACTDRAGSTOP);
	LoadCluiServices();

	CreateServiceFunction("CLUI/GetConnectingIconForProtocol",GetConnectingIconService);

    wndclass.style         = ( IsWinVerXPPlus() && DBGetContactSettingByte(NULL,"CList", "WindowShadow",0) == 1 ? CS_DROPSHADOW : 0) ;
    wndclass.lpfnWndProc   = ContactListWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = g_hInst;
    wndclass.hIcon         = LoadSkinnedIcon (SKINICON_OTHER_MIRANDA);
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wndclass.lpszMenuName  = MAKEINTRESOURCE(IDR_CLISTMENU);
    wndclass.lpszClassName = MIRANDACLASS;
	
	a=RegisterClass(&wndclass);
	
	if(DBGetContactSetting(NULL,"CList","TitleText",&dbv))
		lstrcpyn(titleText,MIRANDANAME,sizeof(titleText));
	else {
		lstrcpyn(titleText,dbv.pszVal,sizeof(titleText));
		DBFreeVariant(&dbv);
	}
	
	
	oldhideoffline=DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT);

	hwndContactList= CreateWindowEx(DBGetContactSettingByte(NULL,"CList","ToolWindow",SETTING_TOOLWINDOW_DEFAULT) ? WS_EX_TOOLWINDOW : 0,
						  MIRANDACLASS,
						  titleText,
						  (DBGetContactSettingByte(NULL,"CLUI","ShowCaption",SETTING_SHOWCAPTION_DEFAULT)?WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX:0)|WS_POPUPWINDOW|WS_THICKFRAME|WS_CLIPCHILDREN,							  
						  (int)DBGetContactSettingDword(NULL,"CList","x",50),
						  (int)DBGetContactSettingDword(NULL,"CList","y",50),
						  (int)DBGetContactSettingDword(NULL,"CList","Width",150),
						  (int)DBGetContactSettingDword(NULL,"CList","Height",200),
						  NULL,
						  NULL,
						  g_hInst,
						  NULL);

	if ( DBGetContactSettingByte(NULL,"CList","OnDesktop",0) )
	{
		HWND hProgMan=FindWindow("Progman",NULL);
		if (IsWindow(hProgMan)) SetParent(hwndContactList,hProgMan);
	}


	laster=GetLastError();
	PreCreateCLC(hwndContactList);
	
	LoadCLUIFramesModule();
	LoadExtraImageFunc();	
				// create status bar frame
	CreateStatusBarhWnd(hwndContactList);				
	CreateStatusBarFrame();

	{	//int state=DBGetContactSettingByte(NULL,"CList","State",SETTING_STATE_NORMAL);
		hMenuMain=GetMenu(hwndContactList);
		if(!DBGetContactSettingByte(NULL,"CLUI","ShowMainMenu",SETTING_SHOWMAINMENU_DEFAULT)) SetMenu(hwndContactList,NULL);
		SetWindowPos(hwndContactList, DBGetContactSettingByte(NULL,"CList","OnTop",SETTING_ONTOP_DEFAULT) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}
	
	 {	CLISTMENUITEM mi;
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.flags=0;
	mi.pszContactOwner=NULL;    //on every contact
	CreateServiceFunction("CList/RenameContactCommand",MenuItem_RenameContact);
	mi.position=2000050000;
	mi.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_RENAME));
	mi.pszName=Translate("&Rename");
	mi.pszService="CList/RenameContactCommand";
	hRenameMenuItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);
	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, MenuItem_PreBuild);
	}
	
	 LoadProtocolOrderModule();
	 lastreqh=0;
	 
	return 0;
}