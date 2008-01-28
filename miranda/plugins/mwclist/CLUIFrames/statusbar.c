#include "..\commonheaders.h"

extern HINSTANCE g_hInst;
HANDLE hStatusBarShowToolTipEvent,hStatusBarHideToolTipEvent;
boolean canloadstatusbar=FALSE;
HWND helperhwnd=0;
HANDLE hFrameHelperStatusBar;
extern	 int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);
extern int GetConnectingIconService (WPARAM wParam,LPARAM lParam);
extern int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);
int RecreateStatusBar();

int UseOwnerDrawStatusBar;


#define TM_STATUSBAR 23435234
#define TM_STATUSBARHIDE 23435235
boolean tooltipshoing;
WNDPROC OldWindowProc=NULL;


POINT lastpnt;
RECT OldRc={0};
static	HBITMAP hBmpBackground;
static int backgroundBmpUse;
static COLORREF bkColour;
extern BYTE showOpts;
int extraspace;

int OnStatusBarBackgroundChange()
{
	{	
		DBVARIANT dbv;
		showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",1);		
		bkColour=DBGetContactSettingDword(NULL,"StatusBar","BkColour",CLCDEFAULT_BKCOLOUR);
		if(hBmpBackground) {DeleteObject(hBmpBackground); hBmpBackground=NULL;}
		if(DBGetContactSettingByte(NULL,"StatusBar","UseBitmap",CLCDEFAULT_USEBITMAP)) {
			if(!DBGetContactSettingString(NULL,"StatusBar","BkBitmap",&dbv)) {
				hBmpBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
				mir_free(dbv.pszVal);
			}
		}
		backgroundBmpUse=DBGetContactSettingWord(NULL,"StatusBar","BkBmpUse",CLCDEFAULT_BKBMPUSE);
		extraspace=DBGetContactSettingDword(NULL,"StatusBar","BkExtraSpace",0);
	}

	RecreateStatusBar(pcli->hwndContactList);
	if (pcli->hwndStatus) InvalidateRect(pcli->hwndStatus,NULL,TRUE);
	return 0;
}


void DrawDataForStatusBar(LPDRAWITEMSTRUCT dis)
{
	//LPDRAWITEMSTRUCT dis=(LPDRAWITEMSTRUCT)lParam;
	ProtocolData *PD=(ProtocolData *)dis->itemData;
	char *szProto=(char*)dis->itemData;
	int status,x;
	SIZE textSize;
	boolean NeedDestroy=FALSE;
	HICON hIcon;
	HRGN  hrgn;


	if (PD==NULL){return;}					
	if (dis->hDC==NULL) {return;}

	//clip it

	hrgn = CreateRectRgn(dis->rcItem.left, dis->rcItem.top, 
		dis->rcItem.right, dis->rcItem.bottom); 

	SelectClipRgn(dis->hDC, hrgn);

	szProto=PD->RealName;
#ifdef _DEBUG
	{
		//char buf[512];
		//sprintf(buf,"proto: %s draw at pos: %d\r\n",szProto,dis->rcItem.left);
		//OutputDebugStringA(buf);
	}
#endif

	status=CallProtoService(szProto,PS_GETSTATUS,0,0);
	SetBkMode(dis->hDC,TRANSPARENT);
	x=dis->rcItem.left+extraspace;

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
		if(lstrlenA(szName)<sizeof(szName)-1) lstrcatA(szName," ");
		GetTextExtentPoint32A(dis->hDC,szName,lstrlenA(szName),&textSize);
		TextOutA(dis->hDC,x,(dis->rcItem.top+dis->rcItem.bottom-textSize.cy)>>1,szName,lstrlenA(szName));
		x+=textSize.cx;
	}
	if(showOpts&4) {
		TCHAR *szStatus = pcli->pfnGetStatusModeDescription(status,0);
		if (!szStatus) szStatus = _T("");
		GetTextExtentPoint32(dis->hDC,szStatus,lstrlen(szStatus),&textSize);
		TextOut(dis->hDC,x,(dis->rcItem.top+dis->rcItem.bottom-textSize.cy)>>1,szStatus,lstrlen(szStatus));
	}
	SelectClipRgn(dis->hDC, NULL);
	DeleteObject(hrgn);
}

void DrawBackGround(HWND hwnd,HDC mhdc)
{
	HDC hdcMem,hdc;
	RECT clRect,*rcPaint;

	int yScroll=0;
	int y;
	PAINTSTRUCT paintst={0};
	HBITMAP hBmpOsb,holdbmp;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	int grey=0;
	HFONT oFont;
	HBRUSH hBrushAlternateGrey=NULL;

	HFONT hFont;

	//InvalidateRect(hwnd,0,FALSE);

	hFont=(HFONT)SendMessage(hwnd,WM_GETFONT,0,0);

	if (mhdc)
	{
		hdc=mhdc;
		rcPaint=NULL;
	}else
	{
		hdc=BeginPaint(hwnd,&paintst);
		rcPaint=&(paintst.rcPaint);
	}

	GetClientRect(hwnd,&clRect);
	if(rcPaint==NULL) rcPaint=&clRect;
	if (rcPaint->right-rcPaint->left==0||rcPaint->top-rcPaint->bottom==0) rcPaint=&clRect;
	y=-yScroll;
	hdcMem=CreateCompatibleDC(hdc);
	hBmpOsb=CreateBitmap(clRect.right,clRect.bottom,1,GetDeviceCaps(hdc,BITSPIXEL),NULL);
	holdbmp=SelectObject(hdcMem,hBmpOsb);
	oFont=SelectObject(hdcMem,hFont);
	SetBkMode(hdcMem,TRANSPARENT);
	{	HBRUSH hBrush,hoBrush;

	hBrush=CreateSolidBrush(bkColour);
	hoBrush=(HBRUSH)SelectObject(hdcMem,hBrush);
	FillRect(hdcMem,rcPaint,hBrush);
	SelectObject(hdcMem,hoBrush);
	DeleteObject(hBrush);
	if(hBmpBackground) {
		BITMAP bmp;
		HDC hdcBmp,holdbackbmp;
		int x,y;
		int maxx,maxy;
		int destw,desth;

		GetObject(hBmpBackground,sizeof(bmp),&bmp);
		hdcBmp=CreateCompatibleDC(hdcMem);
		holdbackbmp=SelectObject(hdcBmp,hBmpBackground);
		y=backgroundBmpUse&CLBF_SCROLL?-yScroll:0;
		maxx=backgroundBmpUse&CLBF_TILEH?clRect.right:1;
		maxy=backgroundBmpUse&CLBF_TILEV?maxy=rcPaint->bottom:y+1;
		switch(backgroundBmpUse&CLBM_TYPE) {
				case CLB_STRETCH:
					if(backgroundBmpUse&CLBF_PROPORTIONAL) {
						if(clRect.right*bmp.bmHeight<clRect.bottom*bmp.bmWidth) {
							desth=clRect.bottom;
							destw=desth*bmp.bmWidth/bmp.bmHeight;
						}
						else {
							destw=clRect.right;
							desth=destw*bmp.bmHeight/bmp.bmWidth;
						}
					}
					else {
						destw=clRect.right;
						desth=clRect.bottom;
					}
					break;
				case CLB_STRETCHH:
					if(backgroundBmpUse&CLBF_PROPORTIONAL) {
						destw=clRect.right;
						desth=destw*bmp.bmHeight/bmp.bmWidth;
					}
					else {
						destw=clRect.right;
						desth=bmp.bmHeight;
					}
					break;
				case CLB_STRETCHV:
					if(backgroundBmpUse&CLBF_PROPORTIONAL) {
						desth=clRect.bottom;
						destw=desth*bmp.bmWidth/bmp.bmHeight;
					}
					else {
						destw=bmp.bmWidth;
						desth=clRect.bottom;
					}
					break;
				default:    //clb_topleft
					destw=bmp.bmWidth;
					desth=bmp.bmHeight;
					break;
		}
		desth=clRect.bottom -clRect.top;
		for(;y<maxy;y+=desth) {
			if(y<rcPaint->top-desth) continue;
			for(x=0;x<maxx;x+=destw)
				StretchBlt(hdcMem,x,y,destw,desth,hdcBmp,0,0,bmp.bmWidth,bmp.bmHeight,SRCCOPY);
		}
		SelectObject(hdcBmp,holdbackbmp);
		DeleteDC(hdcBmp);
	}
	}

	//call to draw icons
	{
		DRAWITEMSTRUCT ds;
		int nParts,nPanel;
		ProtocolData *PD;
		RECT rc,clrc;
		int startoffset,sectwidth;

		memset(&ds,0,sizeof(ds));
		ds.hwndItem=hwnd;
		ds.hDC=hdcMem;


		startoffset=DBGetContactSettingDword(NULL,"StatusBar","FirstIconOffset",0);
		extraspace=DBGetContactSettingDword(NULL,"StatusBar","BkExtraSpace",0);

		nParts=SendMessage(hwnd,SB_GETPARTS,0,0);
		memset(&rc,0,sizeof(RECT));
		GetClientRect(hwnd,&clrc);
		clrc.right-=clrc.left;
		clrc.right-=startoffset;
		sectwidth=clrc.right/nParts;

		for (nPanel=0;nPanel<nParts;nPanel++)
		{
			PD=(ProtocolData *)SendMessage(pcli->hwndStatus,SB_GETTEXT,(WPARAM)nPanel,(LPARAM)0);
			if(PD==NULL){
				continue;
			}
			SendMessage(hwnd,SB_GETRECT,(WPARAM)nPanel,(LPARAM)&rc);
			//rc.left+=startoffset;
			//if (rc.left>=rc.right) rc.left=rc.right-1;
			rc.top=0;
			rc.left=nPanel*sectwidth+startoffset;
			rc.right=rc.left+sectwidth-1;
			ds.rcItem=rc;
			ds.itemData=(DWORD)PD;
			ds.itemID=nPanel;

			DrawDataForStatusBar(&ds);
	}	}

	BitBlt(hdc,rcPaint->left,rcPaint->top,rcPaint->right-rcPaint->left,rcPaint->bottom-rcPaint->top,hdcMem,rcPaint->left,rcPaint->top,SRCCOPY);

	SelectObject(hdcMem,holdbmp);
	SelectObject(hdcMem,oFont);
	DeleteObject(hBmpOsb);
	DeleteDC(hdcMem);
	paintst.fErase=FALSE;
	//DeleteObject(hFont);
	if (!mhdc)
		EndPaint(hwnd,&paintst);	
}

LRESULT CALLBACK StatusBarOwnerDrawProc(          HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
if (UseOwnerDrawStatusBar)
{
	switch(uMsg)
	{
	case WM_ERASEBKGND:
		{

			//DrawBackGround(hwnd);
			return 0;
		}
	case WM_PRINT:
		{
			DrawBackGround(hwnd,(HDC)wParam);
			return 0;
		}
	case WM_PAINT:
		{
			DrawBackGround(hwnd,0);
			return 0;
		}
	}

}
return (CallWindowProc(OldWindowProc,hwnd,uMsg,wParam,lParam)
		);
}

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

		}
	case WM_GETMINMAXINFO:{
			RECT rct;
			if (pcli->hwndStatus==0){break;}
			GetWindowRect(pcli->hwndStatus,&rct);
			memset((LPMINMAXINFO)lParam,0,sizeof(MINMAXINFO));
			((LPMINMAXINFO)lParam)->ptMinTrackSize.x=5;
			((LPMINMAXINFO)lParam)->ptMinTrackSize.y=rct.bottom-rct.top;
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=1600;
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=rct.bottom-rct.top;
			return(0);
	}

	case WM_NCHITTEST:
		{
	
		}
	case WM_SHOWWINDOW:
		{
			{
				int res;
				if (hFrameHelperStatusBar)
				{
					res=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS,hFrameHelperStatusBar),0);
					DBWriteContactSettingByte(0,"CLUI","ShowSBar",(BYTE)((res&F_VISIBLE)?1:0));
				}
			}
			

			if (tooltipshoing){
					NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
					tooltipshoing=FALSE;
				}
			return(0);
			//
		}
	case WM_TIMER:
		{
			if (wParam==TM_STATUSBARHIDE)
			{
				KillTimer(hwnd,TM_STATUSBARHIDE);
				
				if (tooltipshoing){
					NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
					tooltipshoing=FALSE;
				}
				

			}

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
						ScreenToClient(pcli->hwndStatus,&pt);
						nParts=SendMessage(pcli->hwndStatus,SB_GETPARTS,0,0);
						for(i=0;i<nParts;i++) {
							SendMessage(pcli->hwndStatus,SB_GETRECT,i,(LPARAM)&rc);
							if(PtInRect(&rc,pt)) {							
								PD=(ProtocolData *)SendMessage(pcli->hwndStatus,SB_GETTEXT,i,(LPARAM)0);
								if(PD==NULL){return(0);}
								
								NotifyEventHooks(hStatusBarShowToolTipEvent,(WPARAM)PD->RealName,0);
								SetTimer(hwnd,TM_STATUSBARHIDE,DBGetContactSettingWord(NULL,"CLUIFrames","HideToolTipTime",5000),0);
								tooltipshoing=TRUE;

								break;
							}
						}

						
						
					}
					
					
					
				}
				
			}
		
		return(0);
		}
	
	case WM_SETCURSOR:
		{

				{		
					POINT pt;
					GetCursorPos(&pt);
					if (pt.x==lastpnt.x&&pt.y==lastpnt.y)
					{
						return(0);
					}
					lastpnt=pt;
					if (tooltipshoing){
							KillTimer(hwnd,TM_STATUSBARHIDE);				
							NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
							tooltipshoing=FALSE;		
						}
						KillTimer(hwnd,TM_STATUSBAR);
						SetTimer(hwnd,TM_STATUSBAR,DBGetContactSettingWord(NULL,"CLC","InfoTipHoverTime",750),0);
						return(0);
				}			

		}
	case WM_NOTIFY:
		{
			if (lParam==0){return(0);}
		if (((LPNMHDR)lParam)->hwndFrom == pcli->hwndStatus)
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
						}
						KillTimer(hwnd,TM_STATUSBAR);
						SetTimer(hwnd,TM_STATUSBAR,DBGetContactSettingWord(NULL,"CLC","InfoTipHoverTime",750),0);
						return(0);
				}
				*/
				} ;
			}
		}



	
	case WM_CONTEXTMENU:
				KillTimer(hwnd,TM_STATUSBARHIDE);
				
				if (tooltipshoing){
					NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
				}
				tooltipshoing=FALSE;

	
	case WM_MEASUREITEM:
	case WM_DRAWITEM:
		{
			//parent do all work for us
			return(SendMessage(pcli->hwndContactList,msg,wParam,lParam));
		}
/*
	case WM_PAINT:
		{
		return(0);
		}
*/
	case WM_MOVE:
		PostMessage(pcli->hwndStatus,WM_MOVE,wParam,lParam);
	case WM_SIZE:
		{
			RECT rc;
			int b;
		if (pcli->hwndStatus!=0)
		{
			
			
 
			GetClientRect(hwnd,&rc);
			
			b=LOWORD(lParam);
			if (b!=0&&(rc.right-rc.left)!=(OldRc.right-OldRc.left))
			{
				OldRc=rc;
			if (canloadstatusbar)
			{	
				if(DBGetContactSettingByte(NULL,"CLUI","UseOwnerDrawStatusBar",0)||DBGetContactSettingByte(NULL,"CLUI","EqualSections",1)) {
					CluiProtocolStatusChanged(0,0);
					}
			}
			}
			if(msg==WM_SIZE) PostMessage(pcli->hwndStatus,WM_SIZE,wParam,lParam);
			if (pcli->hwndStatus!=0) InvalidateRect(pcli->hwndStatus,NULL,TRUE);
			return(0);
		}
		}

	default:
			return DefWindowProc(hwnd, msg, wParam, lParam);

	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND CreateStatusHelper(HWND parent)
{
	WNDCLASS wndclass={0};
	TCHAR pluginname[] = _T("Statushelper");

	if (GetClassInfo(g_hInst,pluginname,&wndclass) == 0 ) {
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
	}

	return(CreateWindow(pluginname,pluginname,
		/*WS_THICKFRAME|*/WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,
		0,0,0,0,parent,NULL,g_hInst,NULL));
}

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
	Frame.Flags=(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)?F_VISIBLE:0)|F_LOCKED|F_NOBORDER|F_TCHAR;
	GetWindowRect(helperhwnd,&rc);
	h=rc.bottom-rc.top;
	Frame.height=(h==0)?20:h;


	Frame.tname=_T("Status");
	Frame.TBtname=TranslateT("Status Bar");
	hFrameHelperStatusBar=(HANDLE)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,(LPARAM)0);


	return((int)hFrameHelperStatusBar);
}

int RecreateStatusBar(HWND parent)
{
	if (pcli->hwndStatus) {
		FreeProtocolData();
		DestroyWindow(pcli->hwndStatus);
	}
	pcli->hwndStatus=0;
	if (hFrameHelperStatusBar) CallService(MS_CLIST_FRAMES_REMOVEFRAME,(WPARAM)hFrameHelperStatusBar,0);

	helperhwnd=CreateStatusHelper(parent);
	UseOwnerDrawStatusBar=DBGetContactSettingByte(NULL,"CLUI","UseOwnerDrawStatusBar",0);

	//create the status wnd
	pcli->hwndStatus = CreateStatusWindow(
		( DBGetContactSettingByte(0,"CLUI","SBarUseSizeGrip",TRUE) && (!UseOwnerDrawStatusBar)?SBARS_SIZEGRIP:0)|
		WS_CHILD | (DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)?WS_VISIBLE:0), _T(""), helperhwnd, 0);

	OldWindowProc=(WNDPROC)GetWindowLong(pcli->hwndStatus,GWL_WNDPROC);
	SetWindowLong(pcli->hwndStatus,GWL_WNDPROC,(LONG)&StatusBarOwnerDrawProc);
	CreateStatusBarFrame();
	{
		SetWindowPos(helperhwnd,NULL,1,1,1,1,SWP_NOZORDER);
		CluiProtocolStatusChanged(0,0);
		CallService(MS_CLIST_FRAMES_UPDATEFRAME,-1,0);
	}

	return 0;
}

int CreateStatusBarhWnd(HWND parent)
{	
	RecreateStatusBar(parent);
	OnStatusBarBackgroundChange();

	hStatusBarShowToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_SHOW_TOOLTIP);
	hStatusBarHideToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_HIDE_TOOLTIP);
	return((int)pcli->hwndStatus);
}
