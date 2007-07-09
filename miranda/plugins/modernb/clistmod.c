/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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
#include <m_file.h>
#include <m_addcontact.h>
#include "clist.h"
#include "commonprototypes.h"

pfnMyMonitorFromPoint  MyMonitorFromPoint = NULL;
pfnMyMonitorFromWindow MyMonitorFromWindow = NULL;
pfnMyGetMonitorInfo    MyGetMonitorInfo = NULL;

static HANDLE hookSystemShutdown_CListMod=NULL;
HANDLE  hookOptInitialise_CList=NULL,
        hookOptInitialise_Skin=NULL,
        hookContactAdded_CListSettings=NULL;


int CListMod_HideWindow(HWND hwndContactList, int mode);

void GroupMenus_Init(void);
int AddMainMenuItem(WPARAM wParam,LPARAM lParam);
int AddContactMenuItem(WPARAM wParam,LPARAM lParam);
void UninitCListEvents(void);
int ContactSettingChanged(WPARAM wParam,LPARAM lParam);
int ContactAdded(WPARAM wParam,LPARAM lParam);
int GetContactDisplayName(WPARAM wParam,LPARAM lParam);
int CListOptInit(WPARAM wParam,LPARAM lParam);
int SkinOptInit(WPARAM wParam,LPARAM lParam);
int EventsProcessContactDoubleClick(HANDLE hContact);

int TrayIconPauseAutoHide(WPARAM wParam,LPARAM lParam);
int ContactChangeGroup(WPARAM wParam,LPARAM lParam);
void InitTrayMenus(void);


HANDLE hContactIconChangedEvent;
HIMAGELIST hCListImages=NULL;

BOOL (WINAPI *MySetProcessWorkingSetSize)(HANDLE,SIZE_T,SIZE_T);


static HANDLE hSettingChanged;

//returns normal icon or combined with status overlay. Needs to be destroyed.
HICON cliGetIconFromStatusMode(HANDLE hContact, const char *szProto,int status)
{
	HICON hIcon=NULL;
	HICON hXIcon=NULL;
	// check if options is turned on
	BYTE trayOption=DBGetContactSettingByte(NULL,"CLUI","XStatusTray",15);
	if (trayOption&3 && szProto!=NULL)
	{
		// check service exists
		char str[MAXMODULELABELLENGTH];
		strcpy(str,szProto);
		strcat(str,"/GetXStatusIcon");
		if (ServiceExists(str))
		{
			// check status is online
			if (status>ID_STATUS_OFFLINE)
			{
				// get xicon
				hXIcon=(HICON)CallService(str,0,0);
				if (hXIcon)
				{
					// check overlay mode
					if (trayOption&2)
					{
						// get overlay
						HICON MainOverlay=(HICON)GetMainStatusOverlay(status);
						hIcon=ske_CreateJoinedIcon(hXIcon,MainOverlay,(trayOption&4)?192:0);
						DestroyIcon_protect(hXIcon);
                        DestroyIcon_protect(MainOverlay);
					}
					else
					{
						// paint it
						hIcon=hXIcon;
					}
				}
			}
		}
	}
	if (!hIcon)
	{
		hIcon=ske_ImageList_GetIcon(himlCListClc,ExtIconFromStatusMode(hContact,szProto,status),ILD_NORMAL);
	}
	// if not ready take normal icon
	return hIcon;
}
////////// By FYR/////////////
int ExtIconFromStatusMode(HANDLE hContact, const char *szProto,int status)
{
	/*pdisplayNameCacheEntry cacheEntry;
	if ((DBGetContactSettingByte(NULL,"CLC","Meta",0)!=1) && szProto!=NULL)
    {
		if (mir_strcmp(szProto,"MetaContacts")==0)
        {
			hContact=(HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT,(UINT)hContact,0);
			if (hContact!=0)
            {
				szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(UINT)hContact,0);
				status=DBGetContactSettingWord(hContact,szProto,"Status",ID_STATUS_OFFLINE);
			}
        }
    }
    cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
    if (cacheEntry->isTransport>0)  return GetTrasportStatusIconIndex(cacheEntry->isTransport-1,status);
    */
    return pcli->pfnIconFromStatusMode(szProto,status,hContact);
}
/////////// End by FYR ////////
int cli_IconFromStatusMode(const char *szProto,int nStatus, HANDLE hContact)
{
   int result=-1;
   if (hContact && szProto)
   {
       char * szActProto=(char*)szProto;
       char AdvancedService[255]={0};
       int  nActStatus=nStatus;
       HANDLE hActContact=hContact;
       if (!DBGetContactSettingByte(NULL,"CLC","Meta",0) && !mir_strcmp(szActProto,"MetaContacts"))
       {
            // substitute params by mostonline contact datas
           HANDLE hMostOnlineContact=(HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT,(UINT)hActContact,0);
           if (hMostOnlineContact)
           {
                pdisplayNameCacheEntry cacheEntry;
                cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hMostOnlineContact);
                if (cacheEntry && cacheEntry->szProto)
                {
                    szActProto=cacheEntry->szProto;
                    nActStatus=cacheEntry->status;
                    hActContact=hMostOnlineContact;
                }
           }
       }
       _snprintf(AdvancedService,sizeof(AdvancedService),"%s%s",szActProto,"/GetAdvancedStatusIcon");

       if (ServiceExists(AdvancedService))
          result=CallService(AdvancedService,(WPARAM)hActContact, (LPARAM)0);

       if (result==-1 || !(LOWORD(result)))
       {
           //Get normal Icon
           int  basicIcon=saveIconFromStatusMode(szActProto,nActStatus,NULL);
           if (result!=-1 && basicIcon!=1) result|=basicIcon;
           else result=basicIcon;
       }
   }
   else
   {
       result=saveIconFromStatusMode(szProto,nStatus,NULL);
   }
   return result;
}


int GetContactIconC(pdisplayNameCacheEntry cacheEntry)
{
	return ExtIconFromStatusMode(cacheEntry->hContact,cacheEntry->szProto,cacheEntry->szProto==NULL ? ID_STATUS_OFFLINE : cacheEntry->status);
}

//lParam
// 0 - default - return icon id in order: transport status icon, protostatus icon, meta is affected


int GetContactIcon(WPARAM wParam,LPARAM lParam)
{
	char *szProto;
	int status;
	int res;
	szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	if (szProto == NULL)
		status = ID_STATUS_OFFLINE;
	else
		status = DBGetContactSettingWord((HANDLE) wParam, szProto, "Status", ID_STATUS_OFFLINE);
    res=ExtIconFromStatusMode((HANDLE)wParam,szProto,szProto==NULL?ID_STATUS_OFFLINE:status); //by FYR
    if (lParam==0 && res!=-1) res&=0xFFFF;
    return res;
}
void UninitTrayMenu();
void UnLoadContactListModule()  //unhooks noncritical events
{
    UninitTrayMenu();
    UninitCustomMenus();
   // UnloadMainMenu();
   // UnloadStatusMenu();
    UnhookEvent(hookOptInitialise_CList);
    UnhookEvent(hookOptInitialise_Skin);
    UnhookEvent(hSettingChanged);
    UnhookEvent(hookContactAdded_CListSettings);
}
int CListMod_ContactListShutdownProc(WPARAM wParam,LPARAM lParam)
{
    UnhookEvent(hookSystemShutdown_CListMod);
    FreeDisplayNameCache();
    if(g_hMainThread) CloseHandle(g_hMainThread);
    g_hMainThread=NULL;
	return 0;
}

int CLUIGetCapsService(WPARAM wParam,LPARAM lParam)
{
	if (lParam)
	{
		switch (lParam)
		{
		case 0:
			return 0;
		case CLUIF2_PLUGININFO:
			return (int)&pluginInfo;
		case CLUIF2_CLISTTYPE:
	#ifdef UNICODE
				return 0x0107;
	#else
				return 0x0007;
	#endif
		case CLUIF2_EXTRACOLUMNCOUNT:
			return EXTRA_ICON_COUNT;
		case CLUIF2_USEREXTRASTART:
			return EXTRA_ICON_ADV3;
		}
		return 0;
	}
	else
	{
		switch (wParam)
		{
		case CLUICAPS_FLAGS1:
			return CLUIF_HIDEEMPTYGROUPS|CLUIF_DISABLEGROUPS|CLUIF_HASONTOPOPTION|CLUIF_HASAUTOHIDEOPTION;
		}
	}
	return 0;
}
int PreLoadContactListModule()
{
	//initialize firstly hooks
	//clist interface is empty yet so handles should check
	hSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED,ContactSettingChanged);
	return 0;
}

int LoadContactListModule(void)
{
	CreateServiceFunction(MS_CLUI_GETCAPS,CLUIGetCapsService);

	InitDisplayNameCache();
	hookSystemShutdown_CListMod  = HookEvent(ME_SYSTEM_SHUTDOWN,CListMod_ContactListShutdownProc);
	hookOptInitialise_CList      = HookEvent(ME_OPT_INITIALISE,CListOptInit);
	hookOptInitialise_Skin       = HookEvent(ME_OPT_INITIALISE,SkinOptInit);
	
	hookContactAdded_CListSettings = HookEvent(ME_DB_CONTACT_ADDED,ContactAdded);
	hContactIconChangedEvent     = CreateHookableEvent(ME_CLIST_CONTACTICONCHANGED);
	CreateServiceFunction(MS_CLIST_TRAYICONPROCESSMESSAGE,cli_TrayIconProcessMessage);
	CreateServiceFunction(MS_CLIST_PAUSEAUTOHIDE,TrayIconPauseAutoHide);
	CreateServiceFunction(MS_CLIST_CONTACTCHANGEGROUP,ContactChangeGroup);
	CreateServiceFunction(MS_CLIST_TOGGLEHIDEOFFLINE,ToggleHideOffline);
	CreateServiceFunction(MS_CLIST_GETCONTACTICON,GetContactIcon);

	MySetProcessWorkingSetSize=(BOOL (WINAPI*)(HANDLE,SIZE_T,SIZE_T))GetProcAddress(GetModuleHandle(TEXT("kernel32")),"SetProcessWorkingSetSize");
	hCListImages = ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 32, 0);
	InitCustomMenus();
	InitTray();
	{
		HINSTANCE hUser = GetModuleHandleA("USER32");
		MyMonitorFromPoint  = ( pfnMyMonitorFromPoint )GetProcAddress( hUser,"MonitorFromPoint" );
		MyMonitorFromWindow = ( pfnMyMonitorFromWindow )GetProcAddress( hUser, "MonitorFromWindow" );
		#if defined( _UNICODE )
			MyGetMonitorInfo = ( pfnMyGetMonitorInfo )GetProcAddress( hUser, "GetMonitorInfoW");
		#else
			MyGetMonitorInfo = ( pfnMyGetMonitorInfo )GetProcAddress( hUser, "GetMonitorInfoA");
		#endif
	}
	return 0;
}

/*
Begin of Hrk's code for bug
*/
#define GWVS_HIDDEN 1
#define GWVS_VISIBLE 2
#define GWVS_COVERED 3
#define GWVS_PARTIALLY_COVERED 4

int GetWindowVisibleState(HWND, int, int);
__inline DWORD GetDIBPixelColor(int X, int Y, int Width, int Height, int ByteWidth, BYTE * ptr)
{
	DWORD res=0;
	if (X>=0 && X<Width && Y>=0 && Y<Height && ptr)
		res=*((DWORD*)(ptr+ByteWidth*(Height-Y-1)+X*4));
	return res;
}

int GetWindowVisibleState(HWND hWnd, int iStepX, int iStepY) {
	RECT rc = { 0 };
	POINT pt = { 0 };
	register int    i = 0,
                    j = 0,
                    width = 0,
                    height = 0,
                    iCountedDots = 0,
                    iNotCoveredDots = 0;
	HWND hAux = 0;

	if (hWnd == NULL) {
		SetLastError(0x00000006); //Wrong handle
		return -1;
	}
	//Some defaults now. The routine is designed for thin and tall windows.
	if (iStepX <= 0) iStepX = 8;
	if (iStepY <= 0) iStepY = 16;

	if (IsIconic(hWnd) || !IsWindowVisible(hWnd))
		return GWVS_HIDDEN;
	else {
		int hstep,vstep;
		BITMAP bmp;
		HBITMAP WindowImage;
		int maxx=0;
		int maxy=0;
		int wx=0;
		int dx,dy;
		BYTE *ptr=NULL;
		HRGN rgn=NULL;
		WindowImage=g_CluiData.fLayered?ske_GetCurrentWindowImage():0;
		if (WindowImage&&g_CluiData.fLayered)
		{
			GetObject(WindowImage,sizeof(BITMAP),&bmp);
			ptr=bmp.bmBits;
			maxx=bmp.bmWidth;
			maxy=bmp.bmHeight;
			wx=bmp.bmWidthBytes;
		}
		else
		{
			RECT rc;
			int i=0;
			rgn=CreateRectRgn(0,0,1,1);
			GetWindowRect(hWnd,&rc);
			GetWindowRgn(hWnd,rgn);
			OffsetRgn(rgn,rc.left,rc.top);
			GetRgnBox(rgn,&rc);
			i=i;
			//maxx=rc.right;
			//maxy=rc.bottom;
		}
		GetWindowRect(hWnd, &rc);
		{
			RECT rcMonitor={0};
			Docking_GetMonitorRectFromWindow(hWnd,&rcMonitor);
			rc.top=rc.top<rcMonitor.top?rcMonitor.top:rc.top;
			rc.left=rc.left<rcMonitor.left?rcMonitor.left:rc.left;
			rc.bottom=rc.bottom>rcMonitor.bottom?rcMonitor.bottom:rc.bottom;
			rc.right=rc.right>rcMonitor.right?rcMonitor.right:rc.right;
		}
		width = rc.right - rc.left;
		height = rc.bottom- rc.top;
		dx=-rc.left;
		dy=-rc.top;
		hstep=width/iStepX;
		vstep=height/iStepY;
		hstep=hstep>0?hstep:1;
		vstep=vstep>0?vstep:1;

		for (i = rc.top; i < rc.bottom; i+=vstep) {
			pt.y = i;
			for (j = rc.left; j < rc.right; j+=hstep) {
				BOOL po=FALSE;
				pt.x = j;
				if (rgn)
					po=PtInRegion(rgn,j,i);
				else
				{
					DWORD a=(GetDIBPixelColor(j+dx,i+dy,maxx,maxy,wx,ptr)&0xFF000000)>>24;
					a=((a*g_CluiData.bCurrentAlpha)>>8);
					po=(a>16);
				}
				if (po||(!rgn&&ptr==0))
				{
					BOOL hWndFound=FALSE;
					HWND hAuxOld=NULL;
					hAux = WindowFromPoint(pt);
					do
					{
						if (hAux==hWnd)
						{
							hWndFound=TRUE;
							break;
						}
						//hAux = GetParent(hAux);
						hAuxOld=hAux;
						hAux = GetAncestor(hAux,GA_ROOTOWNER);
						if (hAuxOld==hAux)
						{
							TCHAR buf[255];
							GetClassName(hAux,buf,SIZEOF(buf));
							if (!lstrcmp(buf,CLUIFrameSubContainerClassName))
							{
								hWndFound=TRUE;
								break;
							}
						}
					}while(hAux!= NULL &&hAuxOld!=hAux);

					if (hWndFound) //There's  window!
						iNotCoveredDots++; //Let's count the not covered dots.
					//{
					//		  //bPartiallyCovered = TRUE;
					//           //iCountedDots++;
					//	    //break;
					//}
					//else
					iCountedDots++; //Let's keep track of how many dots we checked.
				}
			}
		}
		if (rgn) DeleteObject(rgn);
		if ( iCountedDots - iNotCoveredDots<2) //Every dot was not covered: the window is visible.
			return GWVS_VISIBLE;
		else if (iNotCoveredDots == 0) //They're all covered!
			return GWVS_COVERED;
		else //There are dots which are visible, but they are not as many as the ones we counted: it's partially covered.
			return GWVS_PARTIALLY_COVERED;
	}
}
BYTE g_bCalledFromShowHide=0;
int cliShowHide(WPARAM wParam,LPARAM lParam)
{
	BOOL bShow = FALSE;

	int iVisibleState = GetWindowVisibleState(pcli->hwndContactList,0,0);
	int method;
	method=DBGetContactSettingByte(NULL, "ModernData", "HideBehind", 0);; //(0-none, 1-leftedge, 2-rightedge);
	if (method)
	{
		if (DBGetContactSettingByte(NULL, "ModernData", "BehindEdge", 0)==0 && lParam!=1)
		{
			//hide
			CLUI_HideBehindEdge();
		}
		else
		{
			CLUI_ShowFromBehindEdge();
		}
		bShow=TRUE;
		iVisibleState=GWVS_HIDDEN;
	}

	if (!method && DBGetContactSettingByte(NULL, "ModernData", "BehindEdge", 0)>0)
	{
		g_CluiData.bBehindEdgeSettings=DBGetContactSettingByte(NULL, "ModernData", "BehindEdge", 0);
		CLUI_ShowFromBehindEdge();
		g_CluiData.bBehindEdgeSettings=0;
		g_CluiData.nBehindEdgeState=0;
		DBDeleteContactSetting(NULL, "ModernData", "BehindEdge");
	}

	//bShow is FALSE when we enter the switch if no hide behind edge.
	switch (iVisibleState) {
		case GWVS_PARTIALLY_COVERED:
			//If we don't want to bring it to top, we can use a simple break. This goes against readability ;-) but the comment explains it.
		case GWVS_COVERED: //Fall through (and we're already falling)
			if (DBGetContactSettingByte(NULL,"CList","OnDesktop",0) || !DBGetContactSettingByte(NULL, "CList", "BringToFront", SETTING_BRINGTOFRONT_DEFAULT)) break;
		case GWVS_HIDDEN:
			bShow = TRUE; break;
		case GWVS_VISIBLE: //This is not needed, but goes for readability.
			bShow = FALSE; break;
		case -1: //We can't get here, both pcli->hwndContactList and iStepX and iStepY are right.
			return 0;
	}
	if(bShow == TRUE || lParam) {
		HMONITOR (WINAPI *MyMonitorFromWindow)(HWND,DWORD);
		RECT rcScreen,rcWindow;
		int offScreen=0;

		SystemParametersInfo(SPI_GETWORKAREA,0,&rcScreen,FALSE);
		GetWindowRect(pcli->hwndContactList,&rcWindow);

		callProxied_CLUIFrames_ActivateSubContainers(TRUE);
		CLUI_ShowWindowMod(pcli->hwndContactList, SW_RESTORE);

		if (!DBGetContactSettingByte(NULL,"CList","OnDesktop",0))
		{
			callProxied_CLUIFrames_OnShowHide(pcli->hwndContactList,1);	//TO BE PROXIED
			SetWindowPos(pcli->hwndContactList, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |SWP_NOACTIVATE);
			g_bCalledFromShowHide=1;
			//BringWindowToTop(pcli->hwndContactList);
			if (!DBGetContactSettingByte(NULL,"CList","OnTop",SETTING_ONTOP_DEFAULT))
				//&& ((DBGetContactSettingByte(NULL, "CList", "BringToFront", SETTING_BRINGTOFRONT_DEFAULT) /*&& iVisibleState>=2*/)))
				SetWindowPos(pcli->hwndContactList, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			//SetForegroundWindow(pcli->hwndContactList);
			g_bCalledFromShowHide=0;
		}
		else
		{
			SetWindowPos(pcli->hwndContactList, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			callProxied_CLUIFrames_OnShowHide(pcli->hwndContactList,1);
			SetForegroundWindow(pcli->hwndContactList);
		}
		DBWriteContactSettingByte(NULL,"CList","State",SETTING_STATE_NORMAL);
		//this forces the window onto the visible screen
		MyMonitorFromWindow=(HMONITOR (WINAPI *)(HWND,DWORD))GetProcAddress(GetModuleHandle(TEXT("USER32")),"MonitorFromWindow");
		if(MyMonitorFromWindow) {
			if(MyMonitorFromWindow(pcli->hwndContactList,0)==NULL) {
				BOOL (WINAPI *MyGetMonitorInfoA)(HMONITOR,LPMONITORINFO);
				MONITORINFO mi={0};
				HMONITOR hMonitor=MyMonitorFromWindow(pcli->hwndContactList,2);
				MyGetMonitorInfoA=(BOOL (WINAPI *)(HMONITOR,LPMONITORINFO))GetProcAddress(GetModuleHandle(TEXT("USER32")),"GetMonitorInfoA");
				mi.cbSize=sizeof(mi);
				MyGetMonitorInfoA(hMonitor,&mi);
				rcScreen=mi.rcWork;
				offScreen=1;
			}
		}
		else {
			RECT rcDest;
			if(IntersectRect(&rcDest,&rcScreen,&rcWindow)==0) offScreen=1;
		}
		if(offScreen) {
			if(rcWindow.top>=rcScreen.bottom) OffsetRect(&rcWindow,0,rcScreen.bottom-rcWindow.bottom);
			else if(rcWindow.bottom<=rcScreen.top) OffsetRect(&rcWindow,0,rcScreen.top-rcWindow.top);
			if(rcWindow.left>=rcScreen.right) OffsetRect(&rcWindow,rcScreen.right-rcWindow.right,0);
			else if(rcWindow.right<=rcScreen.left) OffsetRect(&rcWindow,rcScreen.left-rcWindow.left,0);
			SetWindowPos(pcli->hwndContactList,0,rcWindow.left,rcWindow.top,rcWindow.right-rcWindow.left,rcWindow.bottom-rcWindow.top,SWP_NOZORDER);
		}
		//if (DBGetContactSettingByte(NULL,"CList","OnDesktop",0))
		//    SetWindowPos(pcli->hwndContactList, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	}
	else { //It needs to be hidden

		CListMod_HideWindow(pcli->hwndContactList, SW_HIDE);

		DBWriteContactSettingByte(NULL,"CList","State",SETTING_STATE_HIDDEN);
		if(MySetProcessWorkingSetSize!=NULL && DBGetContactSettingByte(NULL,"CList","DisableWorkingSet",1)) MySetProcessWorkingSetSize(GetCurrentProcess(),-1,-1);
	}
	return 0;
}

int CListMod_HideWindow(HWND hwndContactList, int mode)
{
	KillTimer(pcli->hwndContactList,1/*TM_AUTOALPHA*/);
	if (!CLUI_HideBehindEdge())  return CLUI_SmoothAlphaTransition(pcli->hwndContactList, 0, 1);
	return 0;
}
