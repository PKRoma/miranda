#include "../commonheaders.h"
#include "cluiframes.h"
extern BOOL InvalidateRectZ(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern BOOL DrawIconExS(HDC hdc,int xLeft,int yTop,HICON hIcon,int cxWidth,int cyWidth,UINT istepIfAniCur,HBRUSH hbrFlickerFreeDraw,UINT diFlags);
extern BOOL DrawTextS(HDC hdc, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format);
extern BOOL TextOutS(HDC hdc, int x, int y, LPCTSTR lpString, int nCount);

extern HINSTANCE g_hInst;

HANDLE hStatusBarShowToolTipEvent,hStatusBarHideToolTipEvent;
extern HWND hwndStatus;
extern HWND hwndContactList;
boolean canloadstatusbar=FALSE;
HWND helperhwnd=0;
HANDLE hFrameHelperStatusBar;
extern   int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);
extern int GetConnectingIconService (WPARAM wParam,LPARAM lParam);
extern int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);
int TestCursorOnBorders();
int RecreateStatusBar();
int SizingOnBorder(POINT pt,int size);

void DrawBackGround(HWND hwnd, HDC mdc);
int UseOwnerDrawStatusBar;
LRESULT CALLBACK StatusHelperProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#include "m_skin.h"

#define TM_STATUSBAR 23435234
#define TM_STATUSBARHIDE 23435235
boolean tooltipshoing;
WNDPROC OldWindowProc=NULL;


POINT lastpnt;
RECT OldRc={0};
static  HBITMAP hBmpBackground;
static int backgroundBmpUse;
static COLORREF bkColour;
int showOpts;
int extraspace;


int OnStatusBarBackgroundChange()
{
    {   
        DBVARIANT dbv;
        showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",1);     
        bkColour=DBGetContactSettingDword(NULL,"StatusBar","BkColour",CLCDEFAULT_BKCOLOUR);
        if (hBmpBackground) {
            DeleteObject(hBmpBackground); hBmpBackground=NULL;
        }
        if (DBGetContactSettingByte(NULL,"StatusBar","UseBitmap",CLCDEFAULT_USEBITMAP)) {
            if (!DBGetContactSetting(NULL,"StatusBar","BkBitmap",&dbv)) {
                hBmpBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
                mir_free(dbv.pszVal);
            }
        }
        backgroundBmpUse=DBGetContactSettingWord(NULL,"StatusBar","BkBmpUse",CLCDEFAULT_BKBMPUSE);
        extraspace=DBGetContactSettingDword(NULL,"StatusBar","BkExtraSpace",0);
    };
    RecreateStatusBar(CallService(MS_CLUI_GETHWND,0,0));

    if (hwndStatus) {
        CluiProtocolStatusChanged(0,0);
        InvalidateRectZ(hwndStatus,NULL,TRUE);
            //DrawBackGround(hwndStatus,0);
    }

    return 0;
}


void DrawDataForStatusBar(DRAWITEMSTRUCT * dis)
{
            //LPDRAWITEMSTRUCT dis=(LPDRAWITEMSTRUCT)lParam;
    ProtocolData *PD=(ProtocolData *)dis->itemData;
    char *szProto=(char*)dis->itemData;
    int status,x;
    SIZE textSize;
    RECT tr;
    boolean NeedDestroy=FALSE;
    HICON hIcon;
           //     HRGN hrgn;
    static HICON oldIcon;

    if (PD==NULL) {
        return;
    };                 
    if (dis->hDC==NULL) {
        return;
    };


                //clip it

            //	hrgn = CreateRectRgn(dis->rcItem.left, dis->rcItem.top, 
            //	dis->rcItem.right, dis->rcItem.bottom); 

            //	SelectClipRgn(dis->hDC, hrgn);

    szProto=PD->RealName;
    if (IsBadCodePtr((FARPROC)szProto)) return;
    status=CallProtoService(szProto,PS_GETSTATUS,0,0);
    SetBkMode(dis->hDC,TRANSPARENT);
    SetTextColor(dis->hDC,DBGetContactSettingDword(NULL,"StatusBar","TextColour",CLCDEFAULT_TEXTCOLOUR));
    x=dis->rcItem.left+extraspace;

    if (showOpts&1) {

                    //char buf [256];

        if ((DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",1)==1)&&status>=ID_STATUS_CONNECTING&&status<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES) {
            hIcon=(HICON)GetConnectingIconService((WPARAM)szProto,0);

            if (hIcon) {
                NeedDestroy=TRUE;
            }
            else {
                hIcon=LoadSkinnedProtoIcon(szProto,status);
            }

        }
        else {
            hIcon=LoadSkinnedProtoIcon(szProto,status);
        }
        DrawIconExS(dis->hDC,x,(dis->rcItem.top+dis->rcItem.bottom-GetSystemMetrics(SM_CYSMICON))>>1,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
        if (NeedDestroy) DestroyIcon(hIcon);
        x+=GetSystemMetrics(SM_CXSMICON)+2;
    }
    else x+=2;
    if (showOpts&2) {
        char szName[64];
        szName[0]=0;
        if (CallProtoService(szProto,PS_GETNAME,sizeof(szName),(LPARAM)szName)) {
            strcpy(szName,szProto);
        } //if

        if (lstrlen(szName)<sizeof(szName)-1) lstrcat(szName," ");
        GetTextExtentPoint32(dis->hDC,szName,lstrlen(szName),&textSize);
        tr=dis->rcItem;
        tr.left=x;

        DrawTextS(dis->hDC,szName,lstrlen(szName),&tr,DT_VCENTER);

        x+=textSize.cx;
    }
    if (showOpts&4) {
        char *szStatus;
        szStatus=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,status,0);
        if (!szStatus) szStatus="";
        GetTextExtentPoint32(dis->hDC,szStatus,lstrlen(szStatus),&textSize);
        tr=dis->rcItem;
        tr.left=x;
        DrawTextS(dis->hDC,szStatus,lstrlen(szStatus),&tr,DT_LEFT|DT_VCENTER|DT_SINGLELINE);    
    }
}

LRESULT CALLBACK StatusBarOwnerDrawProc(          HWND hwnd,
                                                  UINT uMsg,
                                                  WPARAM wParam,
                                                  LPARAM lParam
                                       )
{
    if (UseOwnerDrawStatusBar) {
        switch (uMsg) {
            case WM_PRINT:
                {
                    DrawBackGround(hwnd,(HDC)wParam);
                    return TRUE;
                }
            case WM_PRINTCLIENT:
                {
       //     DrawBackGround(hwnd,(HDC)wParam);
                    return TRUE;
                }
            case WM_ERASEBKGND:
                {

    //	 DrawBackGround(hwnd,(HDC)wParam);
                    return FALSE;

                }
            case WM_NCPAINT:
                return 1;
            case WM_PAINT:
                {   
                    HDC hdc;
                    PAINTSTRUCT ps;
                    {
                        HWND h;
                        h=GetParent(GetParent(hwnd));              
                        if (h!=(HWND)CallService(MS_CLUI_GETHWND,0,0)) {
                            hdc=BeginPaint(hwnd,&ps);
                            TRACE("STATUSBAR PAINT\n");       
                            DrawBackGround(hwnd,ps.hdc);
                            ps.fErase=FALSE;
                            EndPaint(hwnd,&ps);
                        }
                        else {

                    // SkinUpdateWindow(NULL,hwnd);
                    //TRACE("SkinUpdateWindow on Statusbar WM_PAINT\n");
                            InvalidateFrameImage((WPARAM)hwnd,0);
                            ps.fErase=FALSE;

                        }
                    }
                    return DefWindowProc(hwnd, uMsg, wParam, lParam);
                }
            case WM_LBUTTONDOWN:
                {
                    return CallWindowProc(StatusHelperProc,hwnd,uMsg,wParam,lParam);
                }
        };

    };
    return(CallWindowProc(OldWindowProc,hwnd,uMsg,wParam,lParam));
}

LRESULT CALLBACK StatusHelperProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
  //  case WM_PRINT:
        case WM_PRINTCLIENT:
        case WM_ERASEBKGND:
        case WM_NCPAINT:
   // case WM_PAINT:
            return 1;


        case WM_USER+100:
            return 2;
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
                if (hwndStatus==0) {
                    break;
                };
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
                break;
            };
        case WM_SHOWWINDOW:
            {
                {
                    int res;
                    if (hFrameHelperStatusBar) {
                        res=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS,hFrameHelperStatusBar),0);
                        if (res>0)
                            DBWriteContactSettingByte(0,"CLUI","ShowSBar",(BYTE)((res&F_VISIBLE)?1:0));
                    }
                }


                if (tooltipshoing) {
                    NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
                    tooltipshoing=FALSE;
                };
                return(0);
            //
            };
        case WM_TIMER:
            {
                if (wParam==TM_STATUSBARHIDE) {
                    KillTimer(hwnd,TM_STATUSBARHIDE);

                    if (tooltipshoing) {
                        NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
                        tooltipshoing=FALSE;
                    };


                };

                if (wParam==TM_STATUSBAR) {
                    POINT pt;
                    KillTimer(hwnd,TM_STATUSBAR);

                    GetCursorPos(&pt);
                    if (pt.x==lastpnt.x&&pt.y==lastpnt.y) {
                        {
                            int i,nParts;
                            ProtocolData *PD;
                            RECT rc;
                            ScreenToClient(hwndStatus,&pt);
                            nParts=SendMessage(hwndStatus,SB_GETPARTS,0,0);
                            for (i=0;i<nParts;i++) {
                                SendMessage(hwndStatus,SB_GETRECT,i,(LPARAM)&rc);
                                if (PtInRect(&rc,pt)) {
                                    PD=(ProtocolData *)SendMessage(hwndStatus,SB_GETTEXT,i,(LPARAM)0);
                                    if (PD==NULL) {
                                        return(0);
                                    };

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
                    SendMessage(GetParent(hwnd),msg,wParam,lParam);
                    if (pt.x==lastpnt.x&&pt.y==lastpnt.y) {
                        return(TestCursorOnBorders());
                    };
                    lastpnt=pt;
                    if (tooltipshoing) {
                        KillTimer(hwnd,TM_STATUSBARHIDE);               
                        NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
                        tooltipshoing=FALSE;        
                    };
                    KillTimer(hwnd,TM_STATUSBAR);
                    SetTimer(hwnd,TM_STATUSBAR,DBGetContactSettingWord(NULL,"CLC","InfoTipHoverTime",750),0);

                    return(TestCursorOnBorders());
                };          

            };
        case WM_LBUTTONDOWN:
            {   
                int k=0;
                int OnFreePoint=0;
                POINT pt;
                pt.x = LOWORD(lParam); 
                pt.y = HIWORD(lParam); 
                ClientToScreen(hwndStatus,&pt);
                if (SizingOnBorder(pt,0)==0 && OnFreePoint) {
                    SendMessage(GetParent(GetParent(hwnd)), WM_SYSCOMMAND, SC_MOVE|HTCAPTION,MAKELPARAM(pt.x,pt.y));     
                    return 0;
                }
                break;
            }


        case WM_NOTIFY:
            {
                if (lParam==0) {
                    return(0);
                };
                if (((LPNMHDR)lParam)->hwndFrom == hwndStatus) {


                    if (((LPNMHDR)lParam)->code == WM_NCHITTEST) {
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
                    };
                };
            };




        case WM_CONTEXTMENU:
            KillTimer(hwnd,TM_STATUSBARHIDE);

            if (tooltipshoing) {
                NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
            };
            tooltipshoing=FALSE;


        case WM_MEASUREITEM: 

        case WM_DRAWITEM:
            {
            //parent do all work for us
                return(SendMessage(hwndContactList,msg,wParam,lParam));
            };

            {
        //   DrawBackGround(hwnd,wParam);
        //   return 0;
            }
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
                if (hwndStatus!=0) {



                    GetClientRect(hwnd,&rc);

                    b=LOWORD(lParam);
                    if (b!=0&&(rc.right-rc.left)!=(OldRc.right-OldRc.left)) {
                        OldRc=rc;
                        if (canloadstatusbar) {
                            if (DBGetContactSettingByte(NULL,"CLUI","EqualSections",1)) {
                                CluiProtocolStatusChanged(0,0);
                            }
                        };
                    };
                    if (msg==WM_SIZE) {
                        PostMessage(hwndStatus,WM_SIZE,wParam,lParam);
                // if (hwndStatus!=0) InvalidateRectZ(hwndStatus,NULL,TRUE);
                    }

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
    WNDCLASS wndclass={0};
    char pluginname[]="Statushelper";


    if (GetClassInfo(g_hInst,pluginname,&wndclass) ==0) {

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

    };

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
    Frame.Flags=(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)?F_VISIBLE:0)|F_LOCKED|F_NOBORDER|F_NO_SUBCONTAINER;
    GetWindowRect(helperhwnd,&rc);
    h=rc.bottom-rc.top;
    Frame.height=(h==0)?20:h;


    Frame.name=(Translate("Status"));
    hFrameHelperStatusBar=(HANDLE)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,(LPARAM)0);
    CallService(MS_SKINENG_REGISTERPAINTSUB,(WPARAM)Frame.hWnd,(LPARAM)StatusPaintCallbackProc); //$$$$$ register sub for frame



    //hStatusBarShowToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_SHOW_TOOLTIP);
    return((int)hFrameHelperStatusBar);
}
int RecreateStatusBar(HWND parent)
{
    if (hwndStatus) DestroyWindow(hwndStatus);
    hwndStatus=0;
    if (hFrameHelperStatusBar) CallService(MS_CLIST_FRAMES_REMOVEFRAME,(WPARAM)hFrameHelperStatusBar,0);

    helperhwnd=CreateStatusHelper(parent);
    UseOwnerDrawStatusBar=DBGetContactSettingByte(NULL,"CLUI","UseOwnerDrawStatusBar",1);

            //create the status wnd
    hwndStatus = CreateStatusWindow(

                                   (DBGetContactSettingByte(0,"CLUI","SBarUseSizeGrip",TRUE)&&(!UseOwnerDrawStatusBar)?SBARS_SIZEGRIP:0)|
                                   WS_CHILD | (DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)?WS_VISIBLE:0), "", helperhwnd, 0);




    OldWindowProc=(WNDPROC)GetWindowLong(hwndStatus,GWL_WNDPROC);
    SetWindowLong(hwndStatus,GWL_WNDPROC,(LONG)&StatusBarOwnerDrawProc);
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

    return((int)hwndStatus);
};
