#include "..\commonheaders.h"

extern HINSTANCE g_hInst;
HANDLE hStatusBarShowToolTipEvent,hStatusBarHideToolTipEvent;
extern HWND hwndStatus;
extern HWND hwndContactList;
boolean canloadstatusbar=FALSE;
HWND helperhwnd=0;
HANDLE hFrameHelperStatusBar;
extern	 int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);

#define TM_STATUSBAR 23435234
#define TM_STATUSBARHIDE 23435235
boolean tooltipshoing;


POINT lastpnt;
RECT OldRc={0};

LRESULT CALLBACK StatusHelperProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_CREATE:
		{
			{
				tooltipshoing=FALSE;
				//HWND label;
				//label=CreateWindow("static","Top window",WS_VISIBLE|WS_CHILD	,2,2,120,60,hwnd,NULL,g_hInst,0);
				//SendMessage(label,WM_SETFONT,(WPARAM)TitleBarFont,0);
			}
			return(FALSE);

		};
	case WM_GETMINMAXINFO:{
			RECT rct;
			if (hwndStatus==0){break;};
			GetWindowRect(hwndStatus,&rct);
			memset((LPMINMAXINFO)lParam,0,sizeof(MINMAXINFO));
			((LPMINMAXINFO)lParam)->ptMinTrackSize.x=5;
			((LPMINMAXINFO)lParam)->ptMinTrackSize.y=rct.bottom-rct.top;
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=1600;
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=rct.bottom-rct.top;
			return(0);
	}

	case WM_NCHITTEST:
		{
	
		};
	case WM_SHOWWINDOW:
		{
			DBWriteContactSettingByte(0,"CLUI","ShowSBar",(BYTE)(wParam?1:0));
				if (tooltipshoing){
					NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
					tooltipshoing=FALSE;
				};
			return(0);
			//
		};
	case WM_TIMER:
		{
			if (wParam==TM_STATUSBARHIDE)
			{
				KillTimer(hwnd,TM_STATUSBARHIDE);
				
				if (tooltipshoing){
					NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
					tooltipshoing=FALSE;
				};
				

			};

			if (wParam==TM_STATUSBAR)
			{
				POINT pt;
				KillTimer(hwnd,TM_STATUSBAR);

				GetCursorPos(&pt);
				if (pt.x==lastpnt.x&&pt.y==lastpnt.y)
				{
					{
	 				int i,nParts;
					ProtocolData *PD;
					RECT rc;
						ScreenToClient(hwndStatus,&pt);
						nParts=SendMessage(hwndStatus,SB_GETPARTS,0,0);
						for(i=0;i<nParts;i++) {
							SendMessage(hwndStatus,SB_GETRECT,i,(LPARAM)&rc);
							if(PtInRect(&rc,pt)) {							
								PD=(ProtocolData *)SendMessage(hwndStatus,SB_GETTEXT,i,(LPARAM)0);
								if(PD==NULL){return(0);};
								
								NotifyEventHooks(hStatusBarShowToolTipEvent,(WPARAM)PD->RealName,0);
								SetTimer(hwnd,TM_STATUSBARHIDE,DBGetContactSettingWord(NULL,"CLUIFrames","HideToolTipTime",5000),0);
								tooltipshoing=TRUE;

								break;
							}
						}

						
						
					}
					
					
					
				};
				
			};
		
		return(0);
		};
	
	case WM_SETCURSOR:
		{

				{		
					POINT pt;
					GetCursorPos(&pt);
					if (pt.x==lastpnt.x&&pt.y==lastpnt.y)
					{
						return(0);
					};
					lastpnt=pt;
					if (tooltipshoing){
							KillTimer(hwnd,TM_STATUSBARHIDE);				
							NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
							tooltipshoing=FALSE;		
						};
						KillTimer(hwnd,TM_STATUSBAR);
						SetTimer(hwnd,TM_STATUSBAR,DBGetContactSettingWord(NULL,"CLC","InfoTipHoverTime",750),0);
						return(0);
				};			

		};
	case WM_NOTIFY:
		{
			if (lParam==0){return(0);};
		if (((LPNMHDR)lParam)->hwndFrom == hwndStatus)
			{
				
			if (((LPNMHDR)lParam)->code == WM_NCHITTEST)
				{
				LPNMMOUSE lpnmmouse = (LPNMMOUSE) lParam;
			/*
				{		
					GetCursorPos(&lastpnt);
						if (tooltipshoing){
							KillTimer(hwnd,TM_STATUSBARHIDE);				
							NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
							tooltipshoing=FALSE;		
						};
						KillTimer(hwnd,TM_STATUSBAR);
						SetTimer(hwnd,TM_STATUSBAR,DBGetContactSettingWord(NULL,"CLC","InfoTipHoverTime",750),0);
						return(0);
				};
				*/
				} ;
			};
		};



	
	case WM_CONTEXTMENU:
				KillTimer(hwnd,TM_STATUSBARHIDE);
				
				if (tooltipshoing){
					NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
				};
				tooltipshoing=FALSE;

	
	case WM_MEASUREITEM:
	case WM_DRAWITEM:
		{
			//parent do all work for us
			return(SendMessage(hwndContactList,msg,wParam,lParam));
		};
/*
	case WM_PAINT:
		{
		return(0);
		};
*/
	case WM_MOVE:
		PostMessage(hwndStatus,WM_MOVE,wParam,lParam);
	case WM_SIZE:
		{
			RECT rc;
			int b;
		if (hwndStatus!=0)
		{
			
			
 
			GetClientRect(hwnd,&rc);
			
			b=LOWORD(lParam);
			if (b!=0&&(rc.right-rc.left)!=(OldRc.right-OldRc.left))
			{
				OldRc=rc;
			if (canloadstatusbar)
			{	
				if(DBGetContactSettingByte(NULL,"CLUI","EqualSections",1)) {
					CluiProtocolStatusChanged(0,0);
					}
			};
			};
			if(msg==WM_SIZE) PostMessage(hwndStatus,WM_SIZE,wParam,lParam);
			if (hwndStatus!=0) InvalidateRect(hwndStatus,NULL,TRUE);
			return(0);
		};
		};

	default:
			return DefWindowProc(hwnd, msg, wParam, lParam);

	};
	return DefWindowProc(hwnd, msg, wParam, lParam);
}



HWND CreateStatusHelper(HWND parent)
{
	WNDCLASS wndclass;
	char pluginname[]="Statushelper";

    wndclass.style         = 0;
    wndclass.lpfnWndProc   = StatusHelperProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = g_hInst;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = pluginname;
	RegisterClass(&wndclass);

	return(CreateWindow(pluginname,pluginname,
	/*WS_THICKFRAME|*/WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,
					0,0,0,0,parent,NULL,g_hInst,NULL));
	};

int CreateStatusBarFrame()
{
				CLISTFrame Frame;
				int h;
				RECT rc;

				memset(&Frame,0,sizeof(Frame));
				Frame.cbSize=sizeof(CLISTFrame);
				Frame.hWnd=helperhwnd;
				Frame.align=alBottom;
				Frame.hIcon=LoadSkinnedIcon (SKINICON_OTHER_MIRANDA);
				Frame.Flags=(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)?F_VISIBLE:0)|F_LOCKED|F_NOBORDER;
				GetWindowRect(helperhwnd,&rc);
				h=rc.bottom-rc.top;
				Frame.height=(h==0)?18:h;


				Frame.name=(Translate("Status"));
				hFrameHelperStatusBar=(HANDLE)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,(LPARAM)0);
				
				//frameopt=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,hFrameHelperStatusBar),0);
				//frameopt=(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",0)?F_VISIBLE:0)|F_LOCKED|F_NOBORDER;
				//CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,hFrameHelperStatusBar),frameopt);

	hStatusBarShowToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_SHOW_TOOLTIP);
	hStatusBarHideToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_HIDE_TOOLTIP);
				
	return(0);
};

int CreateStatusBarhWnd(HWND parent)
{
			helperhwnd=CreateStatusHelper(parent);

			//create the status wnd
			hwndStatus = CreateStatusWindow(
				
				(DBGetContactSettingByte(0,"CLUI","SBarUseSizeGrip",TRUE)?SBARS_SIZEGRIP:0)|
				WS_CHILD | (DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)?WS_VISIBLE:0), "", helperhwnd, 0);
return((int)hwndStatus);
};