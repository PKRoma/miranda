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
#include "commonprototypes.h"
#include "m_skinbutton.h"
#include "m_toolbar.h"

#define MS_CLUI_SHOWMAINMENU    "CList/ShowMainMenu"
#define MS_CLUI_SHOWSTATUSMENU  "CList/ShowStatusMenu"
#define MS_JABBER_SHOWBOOKMARK  "JABBER/Bookmarks"

#define MS_TTB_TOGGLEHIDEOFFLINE  "CList/ToggleHideOffline"

/*
1. Init module
1.1 Create services
1.2 Register hooks
1.3 Call Creative

2. Creative
2.1. create bars
2.2. create custom buttons

3. listening and creating of module side buttons 

4. Runtime
4.1	Positioning
4.2. Hiding
4.3. Painting (probably it will be auto repainted)

5. OPTIONS // I hate this

Button icons will be customize thought ico lib

Icon id is TBB_szButtonName_Primary or TBB_szButtonName_Secondary
the section will be. 

'ToolBars/ButtonName' or	'ToolBars/ButtonName Pressed'

The custom executed icons should be 

*/

typedef struct _tagMTBButtonInfo
{
	HWND   hWindow;
	HWND   hwndToolBar;
	char * szButtonID;						//
	char * szButtonName;					// The name of button (untranslated)
	char * szService;						// Service to be executed
	TCHAR * szTooltip;
	TCHAR * szTooltipPressed;
	int	    nOrderValue;						// Order of button
	BYTE    position;
	BOOL    bPushButton;						// 2 icon state button.
	BYTE	bSeparator;						//0 - No, 2- flex, 1 -fixed
}MTBBUTTONINFO;

#define mir_safe_free(a) if(a) mir_free(a);
void sttMTBButtonInfoDestructor(void * input)
{
   MTBBUTTONINFO * mtbi=(MTBBUTTONINFO *)input;
   mir_safe_free(mtbi->szButtonID);
   mir_safe_free(mtbi->szButtonName);
   mir_safe_free(mtbi->szService);
   mir_safe_free(mtbi->szTooltip);
   mir_safe_free(mtbi->szTooltipPressed);
   mir_safe_free(mtbi);
}

typedef struct _tagMTBInfo
{
	int		cbSize;
	HANDLE	hFrame;
	HWND	hWnd;
	SortedList * pButtonList;
	int		nButtonWidth;
	int     nButtonHeight;
	BOOL	bFlatButtons;
	XPTHANDLE	mtbXPTheme;
}MTBINFO;


#define MIRANDATOOLBARCLASSNAME "MirandaToolBar"
enum {
	MTBM_FIRST = WM_USER,
	MTBM_ADDBUTTON,				// will be broad casted thought bars to add button
	MTBM_REPOSBUTTONS,
	MTBM_LAYEREDPAINT,
	MTBM_SETBUTTONSTATE,
	MTBM_GETBUTTONSTATE,
	MTBM_LAST
};

enum {
	SEPARATOR_NOT=0,
	SEPARATOR_FIXED,
	SEPARATOR_FLEX
};

typedef struct _tagTBGlobalDat
{
	HANDLE hToolBarWindowList;

	//hooks and services handles
	HANDLE hehModulesLoaded;
	HANDLE hehSystemShutdown;
	HANDLE hehTBModuleLoaded;
	HANDLE hehSettingsChanged;
	HANDLE hsvcToolBarAddButton;
	HANDLE hsvcToolBarRemoveButton;
	HANDLE hsvcToolBarGetButtonState;
	HANDLE hsvcToolBarSetButtonState;

	HBITMAP mtb_hBmpBackground;
	COLORREF mtb_bkColour;
	WORD mtb_backgroundBmpUse;
	BOOL mtb_useWinColors; 
	HANDLE hHideOfflineButton;

	SortedList * listOfButtons;
	CRITICAL_SECTION cs;
} TBGlobalDat;

static TBGlobalDat tbdat ={0};

#define tbcheck if (!tbdat.hehTBModuleLoaded) return
#define tblock EnterCriticalSection(&tbdat.cs)
#define tbunlock LeaveCriticalSection(&tbdat.cs)

COLORREF sttGetColor(char * module, char * color, COLORREF defColor);

static LRESULT CALLBACK ToolBarProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
static int ToolBarLayeredPaintProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData);

static int svcToolBarAddButton(WPARAM wParam, LPARAM lParam);
static int svcToolBarRemoveButton(WPARAM wParam, LPARAM lParam);
static int svcToolBarGetButtonState(WPARAM wParam, LPARAM lParam);
static int svcToolBarSetButtonState(WPARAM wParam, LPARAM lParam);

static int ehhModulesLoaded(WPARAM wParam, LPARAM lParam);
static int ehhSystemShutdown(WPARAM wParam, LPARAM lParam);
static int ehhSettingsChanged( WPARAM wParam, LPARAM lParam );

static void mtbDefButtonRegistration();

static void sttSetButtonPressed( HANDLE hButton, BOOL bPressed );
static HWND sttCreateToolBarFrame( HWND hwndParent, char * szCaption, int nHeight );

static int _MTB_OnBackgroundSettingsChanged(WPARAM wParam, LPARAM lParam)
{
	if(tbdat.mtb_hBmpBackground) 
	{
		DeleteObject(tbdat.mtb_hBmpBackground); 
		tbdat.mtb_hBmpBackground=NULL;
	}
	if (g_CluiData.fDisableSkinEngine)
	{
		DBVARIANT dbv;
		tbdat.mtb_bkColour=sttGetColor("ToolBar","BkColour",CLCDEFAULT_BKCOLOUR);
		if(DBGetContactSettingByte(NULL,"ToolBar","UseBitmap",CLCDEFAULT_USEBITMAP)) {
			if(!DBGetContactSetting(NULL,"ToolBar","BkBitmap",&dbv)) {
				tbdat.mtb_hBmpBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
				DBFreeVariant(&dbv);
			}
		}
		tbdat.mtb_useWinColors = DBGetContactSettingByte(NULL, "ToolBar", "UseWinColours", CLCDEFAULT_USEWINDOWSCOLOURS);
		tbdat.mtb_backgroundBmpUse = DBGetContactSettingWord(NULL, "ToolBar", "BkBmpUse", CLCDEFAULT_BKBMPUSE);
	}	
	PostMessage(pcli->hwndContactList,WM_SIZE,0,0);
	return 0;
}


void InitTBModule()
{
#ifndef SELFBUILD
	return;
#endif
	tbdat.hehModulesLoaded=HookEvent(ME_SYSTEM_MODULESLOADED, ehhModulesLoaded);
	tbdat.hehSystemShutdown=HookEvent(ME_SYSTEM_SHUTDOWN, ehhSystemShutdown);
	{	//create window class
		WNDCLASS wndclass={0};
		if (GetClassInfo(g_hInst,_T(MIRANDATOOLBARCLASSNAME),&wndclass) ==0)
		{
			wndclass.style         = 0;
			wndclass.lpfnWndProc   = ToolBarProc;
			wndclass.cbClsExtra    = 0;
			wndclass.cbWndExtra    = 0;
			wndclass.hInstance     = g_hInst;
			wndclass.hIcon         = NULL;
			wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
			wndclass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
			wndclass.lpszMenuName  = NULL;
			wndclass.lpszClassName = _T(MIRANDATOOLBARCLASSNAME);
			RegisterClass(&wndclass);
		}	  
	}
	tbdat.listOfButtons=li.List_Create(0,1);
	InitializeCriticalSection(&tbdat.cs);
	tbdat.hehTBModuleLoaded=CreateHookableEvent(ME_TB_MODULELOADED);
}

static int ehhModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	CallService(MS_BACKGROUNDCONFIG_REGISTER,(WPARAM)"ToolBar Background/ToolBar",0);
	HookEvent(ME_BACKGROUNDCONFIG_CHANGED,_MTB_OnBackgroundSettingsChanged);
	_MTB_OnBackgroundSettingsChanged(0,0);

	tbdat.hToolBarWindowList=(HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST,0,0);
	
	CreateServiceFunction(MS_TB_ADDBUTTON,svcToolBarAddButton);
	CreateServiceFunction(MS_TB_SETBUTTONSTATE, svcToolBarSetButtonState);
	
	tbdat.hehSettingsChanged=HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ehhSettingsChanged );
	//CreateSubFrames
	//toggle the 'hide offline contacts' flag and call CLUI   
	//wParam=0
	//lParam=0

	//Create bar ???
	{
		HWND hwndClist=(HWND) CallService(MS_CLUI_GETHWND,0,0);
		sttCreateToolBarFrame( hwndClist, "ToolBar #1", 24);
	}
	mtbDefButtonRegistration();
	

	NotifyEventHooks(ME_TB_MODULELOADED, 0, 0);	
	return 0;
}

static int ehhSystemShutdown(WPARAM wParam, LPARAM lParam)
{
	//Remove services;
	UnhookEvent(tbdat.hehSettingsChanged);
	UnhookEvent(tbdat.hehModulesLoaded);
	UnhookEvent(tbdat.hehSystemShutdown);	
	EnterCriticalSection(&tbdat.cs);
	tbdat.hehTBModuleLoaded=NULL;

	li_ListDestruct(tbdat.listOfButtons,sttMTBButtonInfoDestructor);  //TO DO li_destruct call

	LeaveCriticalSection(&tbdat.cs);
	DeleteCriticalSection(&tbdat.cs);

	return 0;
}


static int ehhSettingsChanged( WPARAM wParam, LPARAM lParam )
{
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;
	if ((HANDLE)wParam!=NULL) return 0;
	if (tbdat.hHideOfflineButton==NULL) return 0;	
	if (!mir_strcmp(cws->szModule,"CList"))
	{
		if (!mir_strcmp(cws->szSetting,"HideOffline"))
			sttSetButtonPressed(tbdat.hHideOfflineButton, (BOOL) DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT) );
	}
	return 0;
}
static void sttUpdateButtonState(MTBBUTTONINFO * mtbi)
{
	HANDLE ilIcon;
	int bufsize = strlen(mtbi->szButtonID)+max(sizeof(TOOLBARBUTTON_ICONIDPREFIX""TOOLBARBUTTON_ICONIDPRIMARYSUFFIX),sizeof(TOOLBARBUTTON_ICONIDPREFIX""TOOLBARBUTTON_ICONIDSECONDARYSUFFIX)) +2;
	char *iconId=(char*)_alloca(bufsize);
	char * translatedName=Translate(mtbi->szButtonName);
	int namebufsize=strlen(translatedName)+strlen(Translate(TOOLBARBUTTON_ICONNAMEPRESSEDSUFFIX))+2;
	char * iconName=(char*)_alloca(namebufsize);
    
	_snprintf(iconId, bufsize,TOOLBARBUTTON_ICONIDPREFIX"%s%s",mtbi->szButtonID, (mtbi->bPushButton)?TOOLBARBUTTON_ICONIDSECONDARYSUFFIX:TOOLBARBUTTON_ICONIDPRIMARYSUFFIX);
	_snprintf(iconName, namebufsize,"%s %s",translatedName, (mtbi->bPushButton)?Translate(TOOLBARBUTTON_ICONNAMEPRESSEDSUFFIX):"");	

	ilIcon=RegisterIcolibIconHandle(iconId, "ToolBar", iconName, _T("toolbar.dll"),1, g_hInst, IDI_RESETVIEW );
	SendMessage(mtbi->hWindow, MBM_SETICOLIBHANDLE, 0, (LPARAM) ilIcon );	
	SendMessage(mtbi->hWindow, BUTTONADDTOOLTIP, (WPARAM)((mtbi->bPushButton) ? mtbi->szTooltipPressed : mtbi->szTooltip), 0);
	
}
/*UNSAFE SHOULDBE CALLED FROM TOOLBAR WINDOW PROC ONLY */ 

void sttReposButtons(MTBINFO * mti)
{
	RECT rcClient;
	HDWP hdwp;
	int  nBarSize;
	int  nFlexSeparatorsCount=0;
	int  nUsedWidth=0;
	int  i=0;
	int  nextX=0;
	int  nFreeSpace;

	GetClientRect(mti->hWnd, &rcClient);
    nBarSize=rcClient.right-rcClient.left;

	//calculating count of visible buttons and separators
	for (i=0; i<mti->pButtonList->realCount; i++)
	{
		MTBBUTTONINFO * mtbi=(MTBBUTTONINFO *)mti->pButtonList->items[i];
		if ( mtbi->bSeparator==2 )
			nFlexSeparatorsCount++;
		else
		{
			int width=(mtbi->bSeparator==1)? 4 : mti->nButtonWidth+1;
			if (nUsedWidth+width>=nBarSize) break;
			nUsedWidth+=width;
		}
	}
	nFreeSpace=nBarSize-nUsedWidth;
	
	hdwp=BeginDeferWindowPos( mti->pButtonList->realCount );
	for (i=0; i<mti->pButtonList->realCount; i++)
	{
		MTBBUTTONINFO * mtbi=(MTBBUTTONINFO *)mti->pButtonList->items[i];
		if (mtbi->hWindow)
		{
			if (hdwp)
			{
				if (nextX+mti->nButtonWidth +1 <= nBarSize)
					hdwp=DeferWindowPos(hdwp, mtbi->hWindow, NULL, nextX, 0, 0,	0,	SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
				else
					hdwp=DeferWindowPos(hdwp, mtbi->hWindow, NULL, nextX, 0, 0,	0,	SWP_NOSIZE|SWP_NOZORDER|SWP_HIDEWINDOW);
			}
			nextX+=mti->nButtonWidth+1;
		}
		else if ( mtbi->bSeparator==1 )
		{
			nextX+=4;
		}
		else if (nFlexSeparatorsCount>0)
		{
			nextX+=nFreeSpace/nFlexSeparatorsCount;
			nFreeSpace-=nFreeSpace/nFlexSeparatorsCount;
			nFlexSeparatorsCount--;
		}
	}
	if (hdwp) EndDeferWindowPos(hdwp);
}

static int svcToolBarAddButton(WPARAM wParam, LPARAM lParam)
{
	int result=0;

	tbcheck 0;
	tblock;
	{	
		TBButton * bi=(TBButton *)lParam;
		MTBBUTTONINFO * mtbi=mir_alloc(sizeof(MTBBUTTONINFO));
		li.List_InsertPtr(tbdat.listOfButtons,mtbi);
		memset(mtbi,0,sizeof(MTBBUTTONINFO));
		if (!(bi->tbbFlags&TBBF_ISSEPARATOR))
		{
			mtbi->szButtonName=mir_strdup(bi->pszButtonName);
			mtbi->szService=mir_strdup(bi->pszServiceName);
			mtbi->szButtonID=mir_strdup(bi->pszButtonID);
			mtbi->bPushButton=(bi->tbbFlags&TBBF_PUSHED)?TRUE:FALSE;
			mtbi->szTooltip=mir_a2t(Translate(bi->pszTooltipUp));
			mtbi->szTooltipPressed=mir_a2t(Translate(bi->pszTooltipDn));
			mtbi->bSeparator=SEPARATOR_NOT;
			
			//database pos read			
			mtbi->nOrderValue=0;//read order (defPos)
			//check visibility

		}		
		else
		{
			mtbi->nOrderValue=bi->defPos;
			mtbi->bSeparator= (((bi->tbbFlags & TBBF_FLEXSIZESEPARATOR) == TBBF_FLEXSIZESEPARATOR)? 	SEPARATOR_FLEX :
					   		  ((bi->tbbFlags & TBBF_ISSEPARATOR) == TBBF_ISSEPARATOR)? SEPARATOR_FIXED : SEPARATOR_NOT);
		}
	}	
	// then call adding of such button to toolbar
	{
		MTBBUTTONINFO * mtbi;
		WindowList_Broadcast(tbdat.hToolBarWindowList, MTBM_ADDBUTTON, (WPARAM)&mtbi, lParam);
		if (mtbi)
		{
			HWND hwndButton=(HWND)mtbi->hWindow;
			TBButton * bi=(TBButton *)lParam;
			if (hwndButton)
			{			
				mtbi->szButtonName=mir_strdup(bi->pszButtonName);
				mtbi->szService=mir_strdup(bi->pszServiceName);
				mtbi->szButtonID=mir_strdup(bi->pszButtonID);
				mtbi->bPushButton=(bi->tbbFlags&TBBF_PUSHED)?TRUE:FALSE;
				mtbi->szTooltip=mir_a2t(Translate(bi->pszTooltipUp));
				mtbi->szTooltipPressed=mir_a2t(Translate(bi->pszTooltipDn));
				{
					char * buttonId=_alloca(sizeof("ToolBar.")+strlen(mtbi->szButtonID)+2);
					strcpy(buttonId,"ToolBar.");
					strcat(buttonId,mtbi->szButtonID);
					SendMessage(hwndButton, BUTTONSETID, 0 ,(LPARAM) buttonId );
				}
				sttUpdateButtonState( mtbi );
			}
			else
			{
				mtbi->bSeparator= (((bi->tbbFlags & TBBF_FLEXSIZESEPARATOR) == TBBF_FLEXSIZESEPARATOR)? 2 : 
									((bi->tbbFlags & TBBF_ISSEPARATOR) == TBBF_ISSEPARATOR)? 1 : 0); 
			}
			SendMessage( mtbi->hwndToolBar, MTBM_REPOSBUTTONS, 0, 0);
			result=(int)hwndButton;
		}
	}
	tbunlock;
	return result;
}
static int svcToolBarRemoveButton(WPARAM wParam, LPARAM lParam)
{
	return 0;
}
static int svcToolBarGetButtonState(WPARAM wParam, LPARAM lParam)
{
	int res=-1;
	WindowList_Broadcast(tbdat.hToolBarWindowList, MTBM_GETBUTTONSTATE, wParam, (LPARAM) &res);
	return res;
}
static int svcToolBarSetButtonState(WPARAM wParam, LPARAM lParam)
{
	WindowList_Broadcast(tbdat.hToolBarWindowList, MTBM_SETBUTTONSTATE, wParam, lParam);
	return 0;
}

static HWND sttCreateToolBarFrame( HWND hwndParent, char * szCaption, int nHeight )
{
	return CreateWindow(_T(MIRANDATOOLBARCLASSNAME), _T(MIRANDATOOLBARCLASSNAME), WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,0,0,0,nHeight,hwndParent,NULL,g_hInst, (void*) szCaption);
}

int sttButtonPressed(MTBINFO * pMTBInfo, HWND hwndbutton)
{
    MTBBUTTONINFO * mtbi=(MTBBUTTONINFO *)GetWindowLong(hwndbutton, GWL_USERDATA);
	if (mtbi && mtbi->hWindow==hwndbutton && mtbi->hwndToolBar==pMTBInfo->hWnd)
	{
		if (mtbi->szService && ServiceExists(mtbi->szService))
			return CallService(mtbi->szService, 0, 0);
	}
	return 0;
}
static BOOL _MTB_DrawBackground(HWND hwnd, HDC hdc, RECT * rect, MTBINFO * pMTBInfo)
{
	if (g_CluiData.fDisableSkinEngine || !g_CluiData.fLayered)
	{	
		RECT rc;
		HBRUSH hbr;
		
		if (rect) 
			rc=*rect;
		else
			GetClientRect(hwnd,&rc);

		if (!(tbdat.mtb_backgroundBmpUse && tbdat.mtb_hBmpBackground) && tbdat.mtb_useWinColors)
		{
			if (xpt_IsThemed(pMTBInfo->mtbXPTheme))
			{
				xpt_DrawTheme(pMTBInfo->mtbXPTheme,pMTBInfo->hWnd, hdc, 0, 0, &rc, &rc);
			}
			else
			{
				hbr=GetSysColorBrush(COLOR_3DFACE);
				FillRect(hdc, &rc, hbr);
			}
		}
		else if (!tbdat.mtb_hBmpBackground && !tbdat.mtb_useWinColors)
		{			
			hbr=CreateSolidBrush(tbdat.mtb_bkColour);
			FillRect(hdc, &rc, hbr);
			DeleteObject(hbr);
		}
		else 
		{
			DrawBackGround(hwnd,hdc,tbdat.mtb_hBmpBackground,tbdat.mtb_bkColour,tbdat.mtb_backgroundBmpUse);
		}
	}
	return TRUE;
}
static LRESULT CALLBACK ToolBarProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	MTBINFO * pMTBInfo=(MTBINFO *)GetWindowLong(hwnd, GWL_USERDATA);
	switch (msg) 
	{	
	case WM_CREATE:
		{
			HANDLE hFrame=NULL;
			CLISTFrame Frame={0};
			CREATESTRUCT * lpcs = (CREATESTRUCT *) lParam;
			//create internal info
			MTBINFO * pMTBInfo = (MTBINFO *) mir_alloc( sizeof(MTBINFO) );
			memset( pMTBInfo, 0, sizeof(MTBINFO) );
			pMTBInfo->cbSize = sizeof(MTBINFO);
			SetWindowLong( hwnd, GWL_USERDATA, (LONG) pMTBInfo );

			//register self frame
			Frame.cbSize=sizeof(CLISTFrame);
			Frame.hWnd=hwnd;
			Frame.align=alTop;
			Frame.hIcon=LoadSkinnedIcon (SKINICON_OTHER_MIRANDA);
			Frame.Flags=(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)?F_VISIBLE:0)|F_LOCKED|F_NOBORDER|F_NO_SUBCONTAINER;
			Frame.height=lpcs->cy;
			Frame.name=(char*) lpcs->lpCreateParams;
			hFrame=(HANDLE)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,(LPARAM)0);
			CallService(MS_SKINENG_REGISTERPAINTSUB,(WPARAM)Frame.hWnd,(LPARAM)ToolBarLayeredPaintProc); //$$$$$ register sub for frame		
			pMTBInfo->hFrame = hFrame;
			pMTBInfo->hWnd = hwnd;

			pMTBInfo->nButtonHeight	= Frame.height; //TODO: replace to option specified
			pMTBInfo->nButtonWidth	= Frame.height;
			pMTBInfo->pButtonList=li.List_Create(0,1);
			//add self to window list
			WindowList_Add(tbdat.hToolBarWindowList, hwnd, NULL);
			pMTBInfo->mtbXPTheme=xpt_AddThemeHandle(hwnd,L"TOOLBAR");
			return 0;
		}
	case WM_DESTROY:
		{
			xpt_FreeThemeForWindow(hwnd);
			WindowList_Remove( tbdat.hToolBarWindowList, hwnd );
			SetWindowLong( hwnd, GWL_USERDATA, (LONG) 0 );
			li_ListDestruct( pMTBInfo->pButtonList, sttMTBButtonInfoDestructor );  //TODO Create buttonlist destructor
			mir_free( pMTBInfo );
			return 0;
		}
	case WM_COMMAND:
		{
			if (HIWORD(wParam)==BN_CLICKED && lParam!=0 )
				  sttButtonPressed(pMTBInfo, (HWND) lParam );
			return TRUE;
		}
	case MTBM_ADDBUTTON:
		{
			//Adding button
			MTBBUTTONINFO * mtbi;
			MTBBUTTONINFO ** retMTBI=(MTBBUTTONINFO **)wParam;
			TBButton * pButton=(TBButton *)lParam;
			HWND hwndButton = NULL;
			if (!(pButton->tbbFlags & TBBF_ISSEPARATOR))
			{
				hwndButton= CreateWindow(SKINBUTTONCLASS /*MIRANDABUTTONCLASS*/, _T(""), BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP , 0, 0, pMTBInfo->nButtonWidth, pMTBInfo->nButtonHeight, 
											hwnd, (HMENU) IDC_SELECTMODE, g_hInst, NULL);
			}
			mtbi=mir_alloc(sizeof(MTBBUTTONINFO));
			memset(mtbi,0,sizeof(MTBBUTTONINFO));
			mtbi->hWindow=hwndButton;
			mtbi->hwndToolBar=hwnd;

			li.List_Insert(pMTBInfo->pButtonList,mtbi,pMTBInfo->pButtonList->realCount);
			if (retMTBI) *retMTBI=mtbi;
			if (hwndButton) 
			{	
				if (pMTBInfo->bFlatButtons)			
					SendMessage(hwndButton, BUTTONSETASFLATBTN, 0, 1 );
				SetWindowLong(hwndButton,GWL_USERDATA,(LONG)mtbi);
				SendMessage(hwndButton, MBM_UPDATETRANSPARENTFLAG, 0, 2);
			}
			return (LRESULT)hwndButton;			
		}

	case WM_SIZE: //fall through
	case MTBM_REPOSBUTTONS:
		{
			if (pMTBInfo && pMTBInfo->hWnd == hwnd)
			{
				sttReposButtons(pMTBInfo);
				return 0;
			}			
			break;
		}
	case WM_ERASEBKGND:
			return _MTB_DrawBackground(hwnd, (HDC)wParam, NULL, pMTBInfo);
	
	case WM_NCPAINT:				
	case WM_PAINT:
		{
			BOOL ret=FALSE;
			PAINTSTRUCT ps;
			if (g_CluiData.fDisableSkinEngine || !g_CluiData.fLayered)
			{
				HBRUSH hbr=CreateSolidBrush(RGB(255,0,255));
				BeginPaint(hwnd,&ps);
				ret=_MTB_DrawBackground(hwnd, ps.hdc, &ps.rcPaint, pMTBInfo);	
				EndPaint(hwnd,&ps);
			}
			/*if (!ret)
			{
				ValidateRect(hwnd, NULL);
				return TRUE;
			}  */
			return DefWindowProc(hwnd, msg, wParam, lParam);
			//return TRUE;
		}
	case WM_NOTIFY:
		{
			if (((LPNMHDR) lParam)->code==BUTTONNEEDREDRAW) 
				pcli->pfnInvalidateRect(hwnd, NULL, FALSE);
			return 0;
		}
	case MTBM_LAYEREDPAINT:
		{
			int i;
			RECT MyRect={0};
			HDC hDC=(HDC)wParam;
			GetWindowRect(hwnd,&MyRect);
			SkinDrawGlyph(hDC,&MyRect,&MyRect,"Bar,ID=ToolBar,Part=Background");
			for (i=0; i<pMTBInfo->pButtonList->realCount; i++)
			{
				RECT childRect;
				POINT Offset;
				MTBBUTTONINFO * mtbi=(MTBBUTTONINFO *)pMTBInfo->pButtonList->items[i];
				GetWindowRect(mtbi->hWindow,&childRect);
				Offset.x=childRect.left-MyRect.left;;
				Offset.y=childRect.top-MyRect.top;
				SendMessage(mtbi->hWindow, BUTTONDRAWINPARENT, (WPARAM)hDC,(LPARAM)&Offset);
			}	
			return 0;
		}
	case MTBM_SETBUTTONSTATE:
		{
			HWND hButton = (HWND)wParam;
			MTBBUTTONINFO *mtbi=NULL;
			int i;
			if (!hButton) return 0;
			for (i=0; i<pMTBInfo->pButtonList->realCount; i++)
			{
				mtbi=(MTBBUTTONINFO*)pMTBInfo->pButtonList->items[i];
				if (mtbi->hWindow == hButton)
				{
					mtbi->bPushButton=(BOOL)lParam;
				    sttUpdateButtonState(mtbi);
					pcli->pfnInvalidateRect(hwnd, NULL, FALSE);
				    break;
				}
			}						
		}
	default :
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}		
	return TRUE;
}

static int ToolBarLayeredPaintProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData)
{
	return SendMessage(hWnd, MTBM_LAYEREDPAINT,(WPARAM)hDC,0);
}

static HANDLE _sttRegisterButton(char * pszButtonID, char * pszButtonName, char * pszServiceName,
					   char * pszTooltipUp, char * pszTooltipDn, int icoDefIdx, int defResource, int defResource2)
{
	TBButton tbb;
	memset(&tbb,0, sizeof(TBButton));
	tbb.cbSize=sizeof(TBButton);
	if (pszButtonID) 	
	{
		tbb.pszButtonID=pszButtonID;
		tbb.pszButtonName=pszButtonName;
		tbb.pszServiceName=pszServiceName;
		tbb.pszTooltipUp=pszTooltipUp;
		tbb.pszTooltipDn=pszTooltipDn;
		{
			char buf[255];
			_snprintf(buf,sizeof(buf),"%s%s%s",TOOLBARBUTTON_ICONIDPREFIX, pszTooltipUp, TOOLBARBUTTON_ICONIDPRIMARYSUFFIX);
			RegisterIcolibIconHandle( buf, "ToolBar", pszButtonName , _T("toolbar.dll"),icoDefIdx, g_hInst, defResource );
		}
		if (pszTooltipDn)
		{
			char buf[255];
			_snprintf(buf,sizeof(buf),"%s%s%s",TOOLBARBUTTON_ICONIDPREFIX, pszButtonID, TOOLBARBUTTON_ICONIDSECONDARYSUFFIX);
			RegisterIcolibIconHandle( buf, "ToolBar", pszTooltipDn , _T("toolbar.dll"),icoDefIdx+1, g_hInst, defResource2 );
		}
	}
	else 
	{		
		if ((BYTE)pszButtonName) tbb.tbbFlags=TBBF_FLEXSIZESEPARATOR;
		else tbb.tbbFlags=TBBF_ISSEPARATOR;
	}
	
	return (HANDLE)CallService(MS_TB_ADDBUTTON,0, (LPARAM)&tbb);
}

static void sttSetButtonPressed( HANDLE hButton, BOOL bPressed )
{
	CallService(MS_TB_SETBUTTONSTATE, (WPARAM) hButton, (LPARAM) (bPressed ? TBST_PUSHED : TBST_RELEASED) );
}

static void mtbDefButtonRegistration()
{
	
	_sttRegisterButton( NULL, (char*)FALSE, NULL, NULL, NULL, 0, 0, 0 );

	_sttRegisterButton( "MainMenu", "Main Menu", MS_CLUI_SHOWMAINMENU,
		"Main menu", NULL,  2 , IDI_RESETVIEW, IDI_RESETVIEW  );

	tbdat.hHideOfflineButton=_sttRegisterButton( "ShowHideOffline","Show/Hide offline contacts", MS_CLIST_TOGGLEHIDEOFFLINE,
					    "Hide offline contacts", "Show offline contacts", 1 , IDI_RESETVIEW, IDI_RESETVIEW  );
	
	sttSetButtonPressed(tbdat.hHideOfflineButton, (BOOL) DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT) );

    _sttRegisterButton( "JabberBookmarks","Jabber Bookmarks", MS_JABBER_SHOWBOOKMARK,
		"Jabber Bookmark", NULL,  3 , IDI_RESETVIEW, IDI_RESETVIEW  );

	_sttRegisterButton( NULL, (char*)TRUE, NULL, NULL, NULL, 0, 0, 0 );

	_sttRegisterButton( "FindUser","Find User", "FindAdd/FindAddCommand",
		"Find User", NULL,  4 , IDI_RESETVIEW, IDI_RESETVIEW  );
	
	_sttRegisterButton( NULL, (char*)TRUE, NULL, NULL, NULL, 0, 0, 0 );
	
	_sttRegisterButton( "Options","Options", "Options/OptionsCommand",
		"Options", NULL,  5 , IDI_RESETVIEW, IDI_RESETVIEW  );

	_sttRegisterButton( NULL, (char*)FALSE, NULL, NULL, NULL, 0, 0, 0 );

}