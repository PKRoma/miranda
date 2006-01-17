/*
MirandaPluginInfo IM: the free IM client for Microsoft* Windows*

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

#include "..\commonheaders.h"
#include "..\m_skin_eng.h"
#include "cluiframes.h"



//not needed,now use MS_CLIST_FRAMEMENUNOTIFY service
//HANDLE hPreBuildFrameMenuEvent;//external event from clistmenus
extern BOOL TransparentFlag;
extern RECT new_window_rect;
extern RECT old_window_rect;
int QueueAllFramesUpdating(BYTE queue);
extern int docked;
extern BOOL IsOnDesktop;
extern int dock_prevent_moving;
extern HINSTANCE g_hInst;
extern BOOL ON_EDGE_SIZING;
extern BOOL (WINAPI *MySetLayeredWindowAttributesNew)(HWND,COLORREF,BYTE,DWORD);
extern BOOL skinInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern int OnShowHide(HWND hwnd, int mode);

extern BYTE UseKeyColor;
extern DWORD KeyColor;
//we use dynamic frame list,
//but who wants so huge number of frames ??
#define MAX_FRAMES		16

#define UNCOLLAPSED_FRAME_SIZE		0
#define DEFAULT_TITLEBAR_HEIGHT		18

//legacy menu support
#define frame_menu_lock				1
#define frame_menu_visible			2
#define frame_menu_showtitlebar		3
#define frame_menu_floating			4
extern  int ModifyMenuItemProxy(WPARAM wParam,LPARAM lParam);
static int UpdateTBToolTip(int framepos);
int CLUIFrameSetFloat(WPARAM wParam,LPARAM lParam);
int CLUIFrameResizeFloatingFrame(int framepos);
extern int ProcessCommandProxy(WPARAM wParam,LPARAM lParam);
static HWND CreateSubContainerWindow(HWND parent,int x,int y,int width,int height);

extern int DrawNonFramedObjects(BOOL Erase,RECT *r);
extern int InvalidateFrameImage(WPARAM wParam, LPARAM lParam);
extern BOOL FillRect255Alpha(HDC memdc,RECT *fr);
extern BOOL MyAlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc,BLENDFUNCTION blendFunction);
extern int SmoothAlphaTransition(HWND hwnd, BYTE GoalAlpha, BOOL wParam);

extern int InitFramesMenus(void);
extern int UnitFramesMenu();
int GapBetweenTitlebar;

boolean FramesSysNotStarted=TRUE;

//typedef struct 
//{
//	int order;
//	int realpos;
//}SortData;

wndFrame *Frames=NULL;
int nFramescount=0;
static int alclientFrame=-1;//for fast access to frame with alclient properties
static int NextFrameId=100;

static int sortfunc(const void *a,const void *b)
{
  SortData *sd1,*sd2;
  sd1=(SortData *)a;
  sd2=(SortData *)b;
  if (sd1->order > sd2->order){return(1);};
  if (sd1->order < sd2->order){return(-1);};
  //if (sd1->order = sd2->order){return(0);};
  return (0);
};

HFONT TitleBarFont;
int TitleBarH=DEFAULT_TITLEBAR_HEIGHT;
static boolean resizing=FALSE;

// menus
static HANDLE contMIVisible,contMITitle,contMITBVisible,contMILock,contMIColl,contMIFloating;
static HANDLE contMIAlignRoot;
static HANDLE contMIAlignTop,contMIAlignClient,contMIAlignBottom;
static HANDLE contMIPosRoot;
static HANDLE contMIPosUp,contMIPosDown;
static HANDLE contMIBorder;
static HANDLE MainMIRoot=(HANDLE)-1;

// others
static int ContactListHeight;
static int LastStoreTick=0;

static int lbypos=-1;
static int oldframeheight=-1;
static int curdragbar=-1;
static CRITICAL_SECTION csFrameHook;

static BOOLEAN CLUIFramesFitInSize(void);
static int RemoveItemFromList(int pos,wndFrame **lpFrames,int *FrameItemCount);
HWND hWndExplorerToolBar;
static int GapBetweenFrames=1;



int OnMoving(HWND hwnd,RECT *lParam)
{
  RECT * r;
  int i;
  r=(RECT*)lParam;
  dock_prevent_moving=0;
  for(i=0;i<nFramescount;i++){

    if (!Frames[i].floating && Frames[i].OwnerWindow!=NULL &&Frames[i].OwnerWindow!=(HWND)-2)
    {
      int x;
      int y;
      int dx,dy;
      wndFrame * Frame;
      POINT pt={0};
      RECT wr;
      Frame=&(Frames[i]);

     GetWindowRect(hwnd,&wr);
      ClientToScreen(hwnd,&pt);
      dx=(r->left-wr.left)+pt.x;
      dy=(r->top-wr.top)+pt.y;
      x=Frame->wndSize.left;
      y=Frame->wndSize.top;
      SetWindowPos(Frame->OwnerWindow,NULL,x+dx,y+dy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_ASYNCWINDOWPOS|SWP_DEFERERASE|SWP_NOOWNERZORDER);
    };

  }
  dock_prevent_moving=1;
  return 0;
}
int SetAlpha(BYTE Alpha)
{
  int i;
  for(i=0;i<nFramescount;i++){

    if (!Frames[i].floating && Frames[i].OwnerWindow!=NULL &&Frames[i].OwnerWindow!=(HWND)-2 && Frames[i].visible && !Frames[i].needhide )
    {
      HWND hwnd=Frames[i].OwnerWindow;
    //  if (Alpha==255)
    //  {
    //    SetWindowLong(hwnd,GWL_EXSTYLE,GetWindowLong(hwnd,GWL_EXSTYLE)&~WS_EX_LAYERED);
    //  }
      /*else*/ if (MySetLayeredWindowAttributesNew)
      {
        long l;
        l=GetWindowLong(hwnd,GWL_EXSTYLE);
        if(!(l&WS_EX_LAYERED))
        {
          HWND parent=NULL;
          //parent=GetParent(hwnd);
          if (IsOnDesktop)
          {
            HWND hProgMan=FindWindow(TEXT("Progman"),NULL);
  		      if (IsWindow(hProgMan))     
			        parent=hProgMan;
          }

          ShowWindowNew(hwnd,SW_HIDE);
          SetParent(hwnd,NULL);
          SetWindowLong(hwnd,GWL_EXSTYLE,l|WS_EX_LAYERED);
          SetParent(hwnd,parent);
          if (l&WS_VISIBLE)  ShowWindowNew(hwnd,SW_SHOW);
        }
        MySetLayeredWindowAttributesNew(hwnd, KeyColor,Alpha, LWA_ALPHA|((UseKeyColor)?LWA_COLORKEY:0));
      }
    };
  }
  return 0;
}
int recurs_prevent=0;

int RepaintSubContainers()
{
  int i;
  for(i=0;i<nFramescount;i++)
    if (!Frames[i].floating && Frames[i].OwnerWindow!=(HWND)0 &&Frames[i].OwnerWindow!=(HWND)-2 && Frames[i].visible && !Frames[i].needhide )
    {
      RedrawWindow(Frames[i].hWnd,NULL,NULL,RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_INVALIDATE|RDW_FRAME);
      //skinInvalidateRect(Frames[i].hWnd,NULL,FALSE);
    };
  return 0;
}

int ActivateSubContainers(BOOL active)
{
  int i;
  for(i=0;i<nFramescount;i++)
    if (active&&!Frames[i].floating && Frames[i].OwnerWindow!=(HWND)0 &&Frames[i].OwnerWindow!=(HWND)-2 && Frames[i].visible && !Frames[i].needhide )
    {
      HWND hwnd=Frames[i].OwnerWindow;
     // BringWindowToTop(hwnd);
     //SetWindowPos(hwnd,GetWindowParent(),0,0,0,0,SWP_NOSIZE/*|SWP_NOACTIVATE*/|SWP_NOMOVE); 
     hwnd=Frames[i].hWnd;//OwnerWindow;
     if (DBGetContactSettingByte(NULL,"CList","OnDesktop",0))
     {
      SetWindowPos(Frames[i].OwnerWindow,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
      SetWindowPos(Frames[i].OwnerWindow,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
      //BringWindowToTop(Frames[i].OwnerWindow);
     }
     else SetWindowPos(Frames[i].OwnerWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE | SWP_NOMOVE);
     
     //---+++SetWindowPos(hwnd,HWND_TOP,0,0,0,0,SWP_NOSIZE/*|*/|SWP_NOMOVE);
    };
  //if (active) SetForegroundWindow(pcli->hwndContactList);
  return 0;
}
int SetParentForContainers(HWND parent)
{
  int i;
  if (parent&&parent!=pcli->hwndContactList)
    IsOnDesktop=1;
  else
    IsOnDesktop=0;
  for(i=0;i<nFramescount;i++){
    if (!Frames[i].floating && Frames[i].OwnerWindow!=(HWND)0 &&Frames[i].OwnerWindow!=(HWND)-2 && Frames[i].visible && !Frames[i].needhide )
    {
      HWND hwnd=Frames[i].OwnerWindow;
      SetParent(hwnd,parent);
    };
  }
  return 0;
}
extern void __inline lockfrm();
extern void __inline ulockfrm();

int OnShowHide(HWND hwnd, int mode)
{

  int i;
  lockfrm();
  for(i=0;i<nFramescount;i++){
    if (!Frames[i].floating && Frames[i].OwnerWindow!=(HWND)0 &&Frames[i].OwnerWindow!=(HWND)-2)
    {
	  ulockfrm();	
      ShowWindow(Frames[i].OwnerWindow,(mode==SW_HIDE||!Frames[i].visible||Frames[i].needhide)?SW_HIDE:mode);
      ShowWindow(Frames[i].hWnd,(mode==SW_HIDE||!Frames[i].visible||Frames[i].needhide)?SW_HIDE:mode);
	  lockfrm();
      if (mode!=SW_HIDE)
      {
        SetWindowPos(Frames[i].OwnerWindow,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        if (DBGetContactSettingByte(NULL,"CList","OnDesktop",0)) 
        {
          SetWindowPos(Frames[i].OwnerWindow,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
          SetWindowPos(Frames[i].OwnerWindow,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        }
        else SetWindowPos(Frames[i].OwnerWindow, HWND_NOTOPMOST, 0, 0, 0, 0,SWP_NOACTIVATE|SWP_NOSIZE | SWP_NOMOVE);       
      }
    }
  }
  ulockfrm();
  if (mode!=SW_HIDE) SetForegroundWindow(pcli->hwndContactList);
  return 0;
}
static int RemoveItemFromList(int pos,wndFrame **lpFrames,int *FrameItemCount)
{
  memcpy(&((*lpFrames)[pos]),&((*lpFrames)[pos+1]),sizeof(wndFrame)*(*FrameItemCount-pos-1));
  (*FrameItemCount)--;
  (*lpFrames)=(wndFrame*)realloc((*lpFrames),sizeof(wndFrame)*(*FrameItemCount));
  return 0;
}

static int id2pos(int id)
{
  int i;

  if (FramesSysNotStarted) return -1;

  for (i=0;i<nFramescount;i++)
  {
    if (Frames[i].id==id) return(i);
  };
  return(-1);
};

static int btoint(BOOLEAN b)
{
  if(b) return 1;
  return 0;
}

void __inline lockfrm()
{
  EnterCriticalSection(&csFrameHook);
}

void __inline ulockfrm()
{
  LeaveCriticalSection(&csFrameHook);
}

//////////screen docking,code  from "floating contacts" plugin.

wndFrame* FindFrameByWnd( HWND hwnd )
{
  BOOL		bFound	= FALSE;
  int i;

  if ( hwnd == NULL ) return( NULL );

  for(i=0;i<nFramescount;i++)
  {
    if ((Frames[i].floating)&&(Frames[i].ContainerWnd==hwnd) ){return(&Frames[i]);};
  };

  return( NULL);
}


int QueueAllFramesUpdating(BYTE queue)
{
  int i;
  for(i=0;i<nFramescount;i++)
  {
	  if (!LayeredFlag)
	  {
		  if (queue) 
			  InvalidateRect(Frames[i].hWnd,NULL,FALSE);
		  else
			  ValidateRect(Frames[i].hWnd,NULL);
	  }
    if (Frames[i].PaintCallbackProc)
    {
      Frames[i].bQueued=queue; 
      //if (!queue)
      if (Frames[i].UpdateRgn)
      {
        DeleteObject(Frames[i].UpdateRgn);
      }
      Frames[i].UpdateRgn=0;
    }

  }
    return queue;

}
int FindFrameID(HWND FrameHwnd)
{
  wndFrame * frm=NULL;
  if (FrameHwnd == NULL ) return 0;
  frm=FindFrameByItsHWND(FrameHwnd);
  if (frm)
    return frm->id;
  else return 0;
}
wndFrame * FindFrameByItsHWND(HWND FrameHwnd)
{
  int i;
  if ( FrameHwnd == NULL ) return( NULL );
  for(i=0;i<nFramescount;i++)
  {
    if (Frames[i].hWnd==FrameHwnd){return(&Frames[i]);};
  };
  return NULL;
}

static void DockThumbs( wndFrame *pThumbLeft, wndFrame *pThumbRight, BOOL bMoveLeft )
{
  if ( ( pThumbRight->dockOpt.hwndLeft == NULL ) && ( pThumbLeft->dockOpt.hwndRight == NULL ) )
  {
    pThumbRight->dockOpt.hwndLeft	= pThumbLeft->ContainerWnd;
    pThumbLeft->dockOpt.hwndRight	= pThumbRight->ContainerWnd;
  }
}


static void UndockThumbs( wndFrame *pThumb1, wndFrame *pThumb2 )
{
  if ( ( pThumb1 == NULL ) || ( pThumb2 == NULL ) )
  {
    return;
  }

  if ( pThumb1->dockOpt.hwndRight == pThumb2->ContainerWnd )
  {
    pThumb1->dockOpt.hwndRight = NULL;
  }

  if ( pThumb1->dockOpt.hwndLeft == pThumb2->ContainerWnd )
  {
    pThumb1->dockOpt.hwndLeft = NULL;
  }

  if ( pThumb2->dockOpt.hwndRight == pThumb1->ContainerWnd )
  {
    pThumb2->dockOpt.hwndRight = NULL;
  }

  if ( pThumb2->dockOpt.hwndLeft == pThumb1->ContainerWnd )
  {
    pThumb2->dockOpt.hwndLeft = NULL;
  }
}

BOOLEAN bMoveTogether;

static void PositionThumb( wndFrame *pThumb, short nX, short nY )
{
  wndFrame	*pCurThumb	= &Frames[0];
  wndFrame	*pDockThumb	= pThumb;
  wndFrame	fakeMainWindow;
  wndFrame	fakeTaskBarWindow;
  RECT		rc;
  RECT		rcThumb;
  RECT		rcOld;
  SIZE		sizeScreen;
  int			nNewX;
  int			nNewY;
  int			nOffs		= 10;
  int			nWidth;
  int			nHeight;
  POINT		pt;
  RECT		rcLeft;
  RECT		rcTop;
  RECT		rcRight;
  RECT		rcBottom;
  BOOL		bDocked;
  BOOL		bDockedLeft;
  BOOL		bDockedRight;
  BOOL		bLeading;
  int			frmidx=0;

  if ( pThumb == NULL ) return;

  sizeScreen.cx = GetSystemMetrics( SM_CXSCREEN );
  sizeScreen.cy = GetSystemMetrics( SM_CYSCREEN );

  // Get thumb dimnsions
 GetWindowRect( pThumb->ContainerWnd, &rcThumb );
  nWidth	= rcThumb.right - rcThumb.left;
  nHeight = rcThumb.bottom - rcThumb.top;

  // Docking to the edges of the screen
  nNewX = nX < nOffs ? 0 : nX;
  nNewX = nNewX > ( sizeScreen.cx - nWidth - nOffs ) ? ( sizeScreen.cx - nWidth ) : nNewX;
  nNewY = nY < nOffs ? 0 : nY;
  nNewY = nNewY > ( sizeScreen.cy - nHeight - nOffs ) ? ( sizeScreen.cy - nHeight ) : nNewY;

  bLeading = pThumb->dockOpt.hwndRight != NULL;

  if ( bMoveTogether )
  {
    UndockThumbs( pThumb,  FindFrameByWnd( pThumb->dockOpt.hwndLeft ) );
   GetWindowRect( pThumb->ContainerWnd, &rcOld );
  }

  memset(&fakeMainWindow,0,sizeof(fakeMainWindow));
  fakeMainWindow.ContainerWnd=pcli->hwndContactList;
  fakeMainWindow.floating=TRUE;

  memset(&fakeTaskBarWindow,0,sizeof(fakeTaskBarWindow));
  fakeTaskBarWindow.ContainerWnd=hWndExplorerToolBar;
  fakeTaskBarWindow.floating=TRUE;


  while( pCurThumb != NULL )
  {
    if (pCurThumb->floating) {

      if ( pCurThumb != pThumb )
      {
       GetWindowRect( pThumb->ContainerWnd, &rcThumb );
        OffsetRect( &rcThumb, nX - rcThumb.left, nY - rcThumb.top );

       GetWindowRect( pCurThumb->ContainerWnd, &rc );

        // These are rects we will dock into

        rcLeft.left		= rc.left - nOffs;
        rcLeft.top		= rc.top - nOffs;
        rcLeft.right	= rc.left + nOffs;
        rcLeft.bottom	= rc.bottom + nOffs;

        rcTop.left		= rc.left - nOffs;
        rcTop.top		= rc.top - nOffs;
        rcTop.right		= rc.right + nOffs;
        rcTop.bottom	= rc.top + nOffs;

        rcRight.left	= rc.right - nOffs;
        rcRight.top		= rc.top - nOffs;
        rcRight.right	= rc.right + nOffs;
        rcRight.bottom	= rc.bottom + nOffs;

        rcBottom.left	= rc.left - nOffs;
        rcBottom.top	= rc.bottom - nOffs;
        rcBottom.right	= rc.right + nOffs;
        rcBottom.bottom = rc.bottom + nOffs;


        bDockedLeft		= FALSE;
        bDockedRight	= FALSE;

        // Upper-left
        pt.x	= rcThumb.left;
        pt.y	= rcThumb.top;
        bDocked	= FALSE;

        if ( PtInRect( &rcRight, pt ) )
        {
          nNewX	= rc.right;
          bDocked = TRUE;
        }

        if ( PtInRect( &rcBottom, pt ) )
        {
          nNewY = rc.bottom;

          if ( PtInRect( &rcLeft, pt ) )
          {
            nNewX = rc.left;
          }
        }

        if ( PtInRect( &rcTop, pt ) )
        {
          nNewY		= rc.top;
          bDockedLeft	= bDocked;
        }

        // Upper-right
        pt.x	= rcThumb.right;
        pt.y	= rcThumb.top;
        bDocked	= FALSE;

        if ( !bLeading && PtInRect( &rcLeft, pt ) )
        {
          if ( !bDockedLeft )
          {
            nNewX	= rc.left - nWidth;
            bDocked	= TRUE;
          }
          else if ( rc.right == rcThumb.left )
          {
            bDocked = TRUE;
          }
        }


        if ( PtInRect( &rcBottom, pt ) )
        {
          nNewY = rc.bottom;

          if ( PtInRect( &rcRight, pt ) )
          {
            nNewX = rc.right - nWidth;
          }
        }

        if ( !bLeading && PtInRect( &rcTop, pt ) )
        {
          nNewY			= rc.top;
          bDockedRight	= bDocked;
        }

        if ( bMoveTogether )
        {
          if ( bDockedRight )
          {
            DockThumbs( pThumb, pCurThumb, TRUE );
          }

          if ( bDockedLeft )
          {
            DockThumbs( pCurThumb, pThumb, FALSE );
          }
        }									

        // Lower-left
        pt.x = rcThumb.left;
        pt.y = rcThumb.bottom;

        if ( PtInRect( &rcRight, pt ) )
        {
          nNewX = rc.right;
        }

        if ( PtInRect( &rcTop, pt ) )
        {
          nNewY = rc.top - nHeight;

          if ( PtInRect( &rcLeft, pt ) )
          {
            nNewX = rc.left;
          }
        }


        // Lower-right
        pt.x = rcThumb.right;
        pt.y = rcThumb.bottom;

        if ( !bLeading && PtInRect( &rcLeft, pt ) )
        {
          nNewX = rc.left - nWidth;
        }

        if ( !bLeading && PtInRect( &rcTop, pt ) )
        {
          nNewY = rc.top - nHeight;

          if ( PtInRect( &rcRight, pt ) )
          {
            nNewX = rc.right - nWidth;
          }
        }
      }

    };
    frmidx++;
    if (pCurThumb->ContainerWnd==fakeTaskBarWindow.ContainerWnd){break;};
    if (pCurThumb->ContainerWnd==fakeMainWindow.ContainerWnd){
      pCurThumb=&fakeTaskBarWindow;continue;};
      if (frmidx==nFramescount){
        pCurThumb=&fakeMainWindow;continue;
      }

      pCurThumb = &Frames[frmidx];



  }

  // Adjust coords once again
  nNewX = nNewX < nOffs ? 0 : nNewX;
  nNewX = nNewX > ( sizeScreen.cx - nWidth - nOffs ) ? ( sizeScreen.cx - nWidth ) : nNewX;
  nNewY = nNewY < nOffs ? 0 : nNewY;
  nNewY = nNewY > ( sizeScreen.cy - nHeight - nOffs ) ? ( sizeScreen.cy - nHeight ) : nNewY;


  SetWindowPos(	pThumb->ContainerWnd, 
    HWND_TOPMOST, 
    nNewX, 
    nNewY, 
    0, 
    0,
    SWP_NOSIZE | SWP_NOZORDER|SWP_NOACTIVATE );


  // OK, move all docked thumbs
  if ( bMoveTogether )
  {
    pDockThumb = FindFrameByWnd( pDockThumb->dockOpt.hwndRight );

    PositionThumb( pDockThumb, (short)( nNewX + nWidth ), (short)nNewY );
  }
}


//////////



void GetBorderSize(HWND hwnd,RECT *rect)
{
  RECT wr,cr;
  POINT pt1,pt2;
//  RECT r={0};
//  *rect=r;
//  return;
 GetWindowRect(hwnd,&wr);
  GetClientRect(hwnd,&cr);
  pt1.y=cr.top;pt1.x=cr.left;
  pt2.y=cr.bottom;pt2.x=cr.right;

  ClientToScreen(hwnd,&pt1);
  ClientToScreen(hwnd,&pt2);

  cr.top=pt1.y;cr.left=pt1.x;
  cr.bottom=pt2.y;cr.right=pt2.x;

  rect->top=cr.top-wr.top;
  rect->left=cr.left-wr.left;
  rect->right=wr.right-cr.right;
  rect->bottom=wr.bottom-cr.bottom;
  //if (rect->top+rect->bottom>10){rect->top=rect->bottom=2;};
  //if (rect->left+rect->right>10){rect->top=rect->bottom=2;};

};

//append string
char __inline *AS(char *str,const char *setting,char *addstr)
{
  if(str!=NULL) {
    strcpy(str,setting);
    strcat(str,addstr);
  }
  return str;
}

int DBLoadFrameSettingsAtPos(int pos,int Frameid)
{
  char sadd[15];
  char buf[255];
  //	char *oldtb;

  itoa(pos,sadd,10);

  //DBWriteContactSettingString(0,CLUIFrameModule,strcat("Name",sadd),Frames[Frameid].name);
  //boolean
  Frames[Frameid].collapsed=DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"Collapse",sadd),Frames[Frameid].collapsed);

  Frames[Frameid].Locked					=DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"Locked",sadd),Frames[Frameid].Locked);
  Frames[Frameid].visible					=DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"Visible",sadd),Frames[Frameid].visible);
  Frames[Frameid].TitleBar.ShowTitleBar	=DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"TBVisile",sadd),Frames[Frameid].TitleBar.ShowTitleBar);

  Frames[Frameid].height					=DBGetContactSettingWord(0,CLUIFrameModule,AS(buf,"Height",sadd),Frames[Frameid].height);
  Frames[Frameid].HeightWhenCollapsed		=DBGetContactSettingWord(0,CLUIFrameModule,AS(buf,"HeightCollapsed",sadd),0);
  Frames[Frameid].align					=DBGetContactSettingWord(0,CLUIFrameModule,AS(buf,"Align",sadd),Frames[Frameid].align);

  Frames[Frameid].FloatingPos.x		=DBGetContactSettingRangedWord(0,CLUIFrameModule,AS(buf,"FloatX",sadd),100,0,1024);
  Frames[Frameid].FloatingPos.y		=DBGetContactSettingRangedWord(0,CLUIFrameModule,AS(buf,"FloatY",sadd),100,0,1024);
  Frames[Frameid].FloatingSize.x		=DBGetContactSettingRangedWord(0,CLUIFrameModule,AS(buf,"FloatW",sadd),100,0,1024);
  Frames[Frameid].FloatingSize.y		=DBGetContactSettingRangedWord(0,CLUIFrameModule,AS(buf,"FloatH",sadd),100,0,1024);

  Frames[Frameid].floating			=DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"Floating",sadd),0);
  Frames[Frameid].order				=DBGetContactSettingWord(0,CLUIFrameModule,AS(buf,"Order",sadd),0);

  Frames[Frameid].UseBorder			=DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"UseBorder",sadd),Frames[Frameid].UseBorder);	

  return 0;
}

int DBStoreFrameSettingsAtPos(int pos,int Frameid)
{
  char sadd[16];
  char buf[255];

  itoa(pos,sadd,10);

  DBWriteContactSettingString(0,CLUIFrameModule,AS(buf,"Name",sadd),Frames[Frameid].name);
  //boolean
  DBWriteContactSettingByte(0,CLUIFrameModule,AS(buf,"Collapse",sadd),(BYTE)btoint(Frames[Frameid].collapsed));
  DBWriteContactSettingByte(0,CLUIFrameModule,AS(buf,"Locked",sadd),(BYTE)btoint(Frames[Frameid].Locked));
  DBWriteContactSettingByte(0,CLUIFrameModule,AS(buf,"Visible",sadd),(BYTE)btoint(Frames[Frameid].visible));
  DBWriteContactSettingByte(0,CLUIFrameModule,AS(buf,"TBVisile",sadd),(BYTE)btoint(Frames[Frameid].TitleBar.ShowTitleBar));

  DBWriteContactSettingWord(0,CLUIFrameModule,AS(buf,"Height",sadd),(WORD)Frames[Frameid].height);
  DBWriteContactSettingWord(0,CLUIFrameModule,AS(buf,"HeightCollapsed",sadd),(WORD)Frames[Frameid].HeightWhenCollapsed);
  DBWriteContactSettingWord(0,CLUIFrameModule,AS(buf,"Align",sadd),(WORD)Frames[Frameid].align);
  //FloatingPos	
  DBWriteContactSettingWord(0,CLUIFrameModule,AS(buf,"FloatX",sadd),(WORD)Frames[Frameid].FloatingPos.x);
  DBWriteContactSettingWord(0,CLUIFrameModule,AS(buf,"FloatY",sadd),(WORD)Frames[Frameid].FloatingPos.y);
  DBWriteContactSettingWord(0,CLUIFrameModule,AS(buf,"FloatW",sadd),(WORD)Frames[Frameid].FloatingSize.x);
  DBWriteContactSettingWord(0,CLUIFrameModule,AS(buf,"FloatH",sadd),(WORD)Frames[Frameid].FloatingSize.y);

  DBWriteContactSettingByte(0,CLUIFrameModule,AS(buf,"Floating",sadd),(BYTE)btoint(Frames[Frameid].floating));
  DBWriteContactSettingByte(0,CLUIFrameModule,AS(buf,"UseBorder",sadd),(BYTE)btoint(Frames[Frameid].UseBorder));
  DBWriteContactSettingWord(0,CLUIFrameModule,AS(buf,"Order",sadd),(WORD)Frames[Frameid].order);	
  //DBWriteContactSettingString(0,CLUIFrameModule,AS(buf,"TBName",sadd),Frames[Frameid].TitleBar.tbname);
  return 0;
}

int LocateStorePosition(int Frameid,int maxstored)
{
  int i,storpos;
  char *frmname,settingname[255];

  storpos=-1;
  for(i=0;i<maxstored;i++) {
		mir_snprintf(settingname,sizeof(settingname),"%s%d","Name",i);
    frmname=DBGetStringA(0,CLUIFrameModule,settingname);
    if(frmname==NULL) continue;
    if(strcmpi(frmname,Frames[Frameid].name)==0) {
      storpos=i;
      mir_free(frmname);
      break;
    }
    mir_free(frmname);
  }
  return storpos;
}

int CLUIFramesLoadFrameSettings(int Frameid)
{
  int storpos,maxstored;

  if (FramesSysNotStarted) return -1;

  //lockfrm();must locked in caller
  if(Frameid<0||Frameid>=nFramescount) return -1;

  maxstored=DBGetContactSettingWord(0,CLUIFrameModule,"StoredFrames",-1);
  if(maxstored==-1) return 0;

  storpos=LocateStorePosition(Frameid,maxstored);
  if(storpos==-1) return 0;

  DBLoadFrameSettingsAtPos(storpos,Frameid);
  //ulockfrm();
  return 0;
}

int CLUIFramesStoreFrameSettings(int Frameid)
{
  int maxstored,storpos;

  if (FramesSysNotStarted) return -1;

  //lockfrm();
  if(Frameid<0||Frameid>=nFramescount) return -1;

  maxstored=DBGetContactSettingWord(0,CLUIFrameModule,"StoredFrames",-1);
  if(maxstored==-1) maxstored=0;

  storpos=LocateStorePosition(Frameid,maxstored);
  if(storpos==-1) {storpos=maxstored; maxstored++;}

  DBStoreFrameSettingsAtPos(storpos,Frameid);
  DBWriteContactSettingWord(0,CLUIFrameModule,"StoredFrames",(WORD)maxstored);
  //ulockfrm();
  return 0;
}

int CLUIFramesStoreAllFrames()
{
  int i;
  if (FramesSysNotStarted) return -1;
  lockfrm();
  for(i=0;i<nFramescount;i++)
    CLUIFramesStoreFrameSettings(i);
  ulockfrm();
  return 0;
}

// Get client frame
int CLUIFramesGetalClientFrame(void)
{
  int i;
  if (FramesSysNotStarted) return -1;

  if(alclientFrame!=-1)
    return alclientFrame;

  if(alclientFrame!=-1) {
    /* this value could become invalid if RemoveItemFromList was called,
    * so we double-check */
    if (alclientFrame<nFramescount) {
      if(Frames[alclientFrame].align==alClient) {
        return alclientFrame;
      }
    }
  }	

  for(i=0;i<nFramescount;i++)
    if(Frames[i].align==alClient) {
      alclientFrame=i;
      return i;
    }
    //pluginLink
    return -1;
}

HMENU CLUIFramesCreateMenuForFrame(int frameid,int root,int popuppos,char *addservice)
{
  CLISTMENUITEM mi;
  //TMO_MenuItem tmi;
  HANDLE menuid;
  int framepos=id2pos(frameid);

  if (FramesSysNotStarted) return NULL;

  ZeroMemory(&mi,sizeof(mi));

  mi.cbSize=sizeof(mi);
  mi.hIcon=LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_MIRANDA));
  mi.pszPopupName=(char *)root;
  mi.popupPosition=frameid;
  mi.position=popuppos++;
  mi.pszName=Translate("&FrameTitle");
  mi.flags=CMIF_CHILDPOPUP|CMIF_GRAYED;
  mi.pszContactOwner=(char *)0;
  menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
  if(frameid==-1) contMITitle=menuid;
  else Frames[framepos].MenuHandles.MITitle=menuid;

  popuppos+=100000;
  mi.hIcon=NULL;
  mi.cbSize=sizeof(mi);
  mi.pszPopupName=(char *)root;
  mi.popupPosition=frameid;
  mi.position=popuppos++;
  mi.pszName=Translate("&Visible");
  mi.flags=CMIF_CHILDPOPUP|CMIF_CHECKED;
  mi.pszContactOwner=(char *)0;
  mi.pszService=MS_CLIST_FRAMES_SHFRAME;
  menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
  if(frameid==-1) contMIVisible=menuid;
  else Frames[framepos].MenuHandles.MIVisible=menuid;

  mi.pszPopupName=(char *)root;
  mi.popupPosition=frameid;
  mi.position=popuppos++;
  mi.pszName=Translate("&Show TitleBar");
  mi.flags=CMIF_CHILDPOPUP|CMIF_CHECKED;
  mi.pszService=MS_CLIST_FRAMES_SHFRAMETITLEBAR;
  mi.pszContactOwner=(char *)0;
  menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
  if(frameid==-1) contMITBVisible=menuid;
  else Frames[framepos].MenuHandles.MITBVisible=menuid;


  popuppos+=100000;

  mi.pszPopupName=(char *)root;
  mi.popupPosition=frameid;
  mi.position=popuppos++;
  mi.pszName=Translate("&Locked");
  mi.flags=CMIF_CHILDPOPUP|CMIF_CHECKED;
  mi.pszService=MS_CLIST_FRAMES_ULFRAME;
  mi.pszContactOwner=(char *)0;
  menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
  if(frameid==-1) contMILock=menuid;
  else Frames[framepos].MenuHandles.MILock=menuid;

  mi.pszPopupName=(char *)root;
  mi.popupPosition=frameid;
  mi.position=popuppos++;
  mi.pszName=Translate("&Collapsed");
  mi.flags=CMIF_CHILDPOPUP|CMIF_CHECKED;
  mi.pszService=MS_CLIST_FRAMES_UCOLLFRAME;
  mi.pszContactOwner=(char *)0;
  menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
  if(frameid==-1) contMIColl=menuid;
  else Frames[framepos].MenuHandles.MIColl=menuid;

  //floating
  mi.pszPopupName=(char *)root;
  mi.popupPosition=frameid;
  mi.position=popuppos++;
  mi.pszName=Translate("&Floating Mode");
  mi.flags=CMIF_CHILDPOPUP;
  mi.pszService="Set_Floating";
  mi.pszContactOwner=(char *)0;
  menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
  if(frameid==-1) contMIFloating=menuid;
  else Frames[framepos].MenuHandles.MIFloating=menuid;


  popuppos+=100000;

  mi.pszPopupName=(char *)root;
  mi.popupPosition=frameid;
  mi.position=popuppos++;
  mi.pszName=Translate("&Border");
  mi.flags=CMIF_CHILDPOPUP|CMIF_CHECKED;
  mi.pszService=MS_CLIST_FRAMES_SETUNBORDER;
  mi.pszContactOwner=(char *)0;
  menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
  if(frameid==-1) contMIBorder=menuid;
  else Frames[framepos].MenuHandles.MIBorder=menuid;

  popuppos+=100000;

  {
    //alignment root
    mi.pszPopupName=(char *)root;
    mi.popupPosition=frameid;
    mi.position=popuppos++;
    mi.pszName=Translate("&Align");
    mi.flags=CMIF_CHILDPOPUP|CMIF_ROOTPOPUP;
    mi.pszService="";
    mi.pszContactOwner=(char *)0;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if(frameid==-1) contMIAlignRoot=menuid;
    else Frames[framepos].MenuHandles.MIAlignRoot=menuid;

    mi.flags=CMIF_CHILDPOPUP;
    //align top
    mi.pszPopupName=(char *)menuid;
    mi.popupPosition=frameid;
    mi.position=popuppos++;
    mi.pszName=Translate("&Top");
    mi.pszService=CLUIFRAMESSETALIGNALTOP;
    mi.pszContactOwner=(char *)alTop;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if(frameid==-1) contMIAlignTop=menuid;
    else Frames[framepos].MenuHandles.MIAlignTop=menuid;


    //align client
    mi.position=popuppos++;
    mi.pszName=Translate("&Client");
    mi.pszService=CLUIFRAMESSETALIGNALCLIENT;
    mi.pszContactOwner=(char *)alClient;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if(frameid==-1) contMIAlignClient=menuid;
    else Frames[framepos].MenuHandles.MIAlignClient=menuid;

    //align bottom
    mi.position=popuppos++;
    mi.pszName=Translate("&Bottom");
    mi.pszService=CLUIFRAMESSETALIGNALBOTTOM;
    mi.pszContactOwner=(char *)alBottom;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if(frameid==-1) contMIAlignBottom=menuid;
    else Frames[framepos].MenuHandles.MIAlignBottom=menuid;

  }

  {	//position
    //position root
    mi.pszPopupName=(char *)root;
    mi.popupPosition=frameid;
    mi.position=popuppos++;
    mi.pszName=Translate("&Position");
    mi.flags=CMIF_CHILDPOPUP|CMIF_ROOTPOPUP;
    mi.pszService="";
    mi.pszContactOwner=(char *)0;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if(frameid==-1) contMIPosRoot=menuid;
    else Frames[framepos].MenuHandles.MIPosRoot=menuid;
    //??????
    mi.pszPopupName=(char *)menuid;
    mi.popupPosition=frameid;
    mi.position=popuppos++;
    mi.pszName=Translate("&Up");
    mi.flags=CMIF_CHILDPOPUP;
    mi.pszService=CLUIFRAMESMOVEUPDOWN;
    mi.pszContactOwner=(char *)1;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if(frameid==-1) contMIPosUp=menuid;
    else Frames[framepos].MenuHandles.MIPosUp=menuid;

    mi.pszPopupName=(char *)menuid;
    mi.popupPosition=frameid;
    mi.position=popuppos++;
    mi.pszName=Translate("&Down");
    mi.flags=CMIF_CHILDPOPUP;
    mi.pszService=CLUIFRAMESMOVEUPDOWN;
    mi.pszContactOwner=(char *)-1;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);	
    if(frameid==-1) contMIPosDown=menuid;
    else Frames[framepos].MenuHandles.MIPosDown=menuid;

  }	

  return 0;
}

int ModifyMItem(WPARAM wParam,LPARAM lParam)
{
  return ModifyMenuItemProxy(wParam,lParam);
};


static int CLUIFramesModifyContextMenuForFrame(WPARAM wParam,LPARAM lParam)
{
  int pos;
  CLISTMENUITEM mi;
  //TMO_MenuItem tmi;

  if (FramesSysNotStarted) return -1;

  lockfrm();
  pos=id2pos(wParam);

  if(pos>=0&&pos<nFramescount) {
    memset(&mi,0,sizeof(mi));
    mi.cbSize=sizeof(mi);
    mi.flags=CMIM_NAME|CMIF_CHILDPOPUP;
    mi.pszName=Frames[pos].name;
    ModifyMItem((WPARAM)contMITitle,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].visible) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMIVisible,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].Locked) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMILock,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].TitleBar.ShowTitleBar) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMITBVisible,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].floating) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMIFloating,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if((Frames[pos].UseBorder)) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMIBorder,(LPARAM)&mi);



    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].align&alTop) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMIAlignTop,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].align&alClient) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMIAlignClient,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].align&alBottom) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMIAlignBottom,(LPARAM)&mi);


    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].collapsed) mi.flags|=CMIF_CHECKED;
    if((!Frames[pos].visible)||(Frames[pos].Locked)||(pos==CLUIFramesGetalClientFrame())) mi.flags|=CMIF_GRAYED;
    ModifyMItem((WPARAM)contMIColl,(LPARAM)&mi);
  }
  ulockfrm();
  return 0;
}

int CLUIFramesModifyMainMenuItems(WPARAM wParam,LPARAM lParam)
{
  int pos;
  CLISTMENUITEM mi;
  //TMO_MenuItem tmi;

  if (FramesSysNotStarted) return -1;

  lockfrm();
  pos=id2pos(wParam);

  if (pos>=0&&pos<nFramescount) {
    memset(&mi,0,sizeof(mi));
    mi.cbSize=sizeof(mi);
    mi.flags=CMIM_NAME|CMIF_CHILDPOPUP;
    mi.pszName=Frames[pos].name;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MITitle,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].visible) mi.flags|=CMIF_CHECKED;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIVisible,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].Locked) mi.flags|=CMIF_CHECKED;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MILock,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].TitleBar.ShowTitleBar) mi.flags|=CMIF_CHECKED;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MITBVisible,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].floating) mi.flags|=CMIF_CHECKED;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIFloating,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if((Frames[pos].UseBorder)) mi.flags|=CMIF_CHECKED;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIBorder,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|((Frames[pos].align&alClient)?CMIF_GRAYED:0);
    if(Frames[pos].align&alTop) mi.flags|=CMIF_CHECKED;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIAlignTop,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].align&alClient) mi.flags|=CMIF_CHECKED;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIAlignClient,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|((Frames[pos].align&alClient)?CMIF_GRAYED:0);
    if(Frames[pos].align&alBottom) mi.flags|=CMIF_CHECKED;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIAlignBottom,(LPARAM)&mi);

    /*
    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|((Frames[pos].align&alClient)?CMIF_GRAYED:0);
    if(Frames[pos].align&alTop) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMIAlignTop,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|CMIF_GRAYED;
    if(Frames[pos].align&alClient) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMIAlignClient,(LPARAM)&mi);

    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|((Frames[pos].align&alClient)?CMIF_GRAYED:0);
    if(Frames[pos].align&alBottom) mi.flags|=CMIF_CHECKED;
    ModifyMItem((WPARAM)contMIAlignBottom,(LPARAM)&mi);

    */



    mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
    if(Frames[pos].collapsed) mi.flags|=CMIF_CHECKED;
    if((!Frames[pos].visible)||Frames[pos].Locked||(pos==CLUIFramesGetalClientFrame())) mi.flags|=CMIF_GRAYED;
    CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIColl,(LPARAM)&mi);
  }
  ulockfrm();
  return 0;
}

//hiword(wParam)=frameid,loword(wParam)=flag
int CLUIFramesGetFrameOptions(WPARAM wParam,LPARAM lParam)
{
  int pos;
  int retval;

  if (FramesSysNotStarted) return -1;

  lockfrm();
  pos=id2pos(HIWORD(wParam));
  if(pos<0||pos>=nFramescount) {
    ulockfrm();
    return -1;
  }

  switch(LOWORD(wParam))
  {
  case FO_FLAGS:
    retval=0;
    if(Frames[pos].visible) retval|=F_VISIBLE;
    if(!Frames[pos].collapsed) retval|=F_UNCOLLAPSED;
    if(Frames[pos].Locked) retval|=F_LOCKED;
    if(Frames[pos].TitleBar.ShowTitleBar) retval|=F_SHOWTB;
    if(Frames[pos].TitleBar.ShowTitleBarTip) retval|=F_SHOWTBTIP;
    if(!(GetWindowLong(Frames[pos].hWnd,GWL_STYLE)&WS_BORDER)) retval|=F_NOBORDER;
    break;

  case FO_NAME:
    retval=(int)Frames[pos].name;
    break;

  case FO_TBNAME:
    retval=(int)Frames[pos].TitleBar.tbname;
    break;

  case FO_TBTIPNAME:
    retval=(int)Frames[pos].TitleBar.tooltip;
    break;

  case FO_TBSTYLE:
    retval=GetWindowLong(Frames[pos].TitleBar.hwnd,GWL_STYLE);
    break;

  case FO_TBEXSTYLE:
    retval=GetWindowLong(Frames[pos].TitleBar.hwnd,GWL_EXSTYLE);
    break;

  case FO_ICON:
    retval=(int)Frames[pos].TitleBar.hicon;
    break;

  case FO_HEIGHT:
    retval=(int)Frames[pos].height;
    break;

  case FO_ALIGN:
    retval=(int)Frames[pos].align;
    break;
  case FO_FLOATING:
    retval=(int)Frames[pos].floating;
    break;
  default:
    retval=-1;
    break;
  }
  ulockfrm();
  return retval;
}

//hiword(wParam)=frameid,loword(wParam)=flag
int CLUIFramesSetFrameOptions(WPARAM wParam,LPARAM lParam)
{
  int pos;
  int retval; // value to be returned

  if (FramesSysNotStarted) return -1;

  lockfrm();
  pos=id2pos(HIWORD(wParam));
  if(pos<0||pos>=nFramescount) {
    ulockfrm();
    return -1;
  }

  switch(LOWORD(wParam))
  {
  case FO_FLAGS:{
    int flag=lParam;
    int style;

    Frames[pos].dwFlags=flag;
    Frames[pos].visible=FALSE;
    if(flag&F_VISIBLE) Frames[pos].visible=TRUE;

    Frames[pos].collapsed=TRUE;
    if(flag&F_UNCOLLAPSED) Frames[pos].collapsed=FALSE;

    Frames[pos].Locked=FALSE;
    if(flag&F_LOCKED) Frames[pos].Locked=TRUE;

    Frames[pos].UseBorder=TRUE;
    if(flag&F_NOBORDER) Frames[pos].UseBorder=FALSE;

    Frames[pos].TitleBar.ShowTitleBar=FALSE;
    if(flag&F_SHOWTB) Frames[pos].TitleBar.ShowTitleBar=TRUE;

    Frames[pos].TitleBar.ShowTitleBarTip=FALSE;
    if(flag&F_SHOWTBTIP) Frames[pos].TitleBar.ShowTitleBarTip=TRUE;

    SendMessage(Frames[pos].TitleBar.hwndTip,TTM_ACTIVATE,(WPARAM)Frames[pos].TitleBar.ShowTitleBarTip,0);

    style=(int)GetWindowLong(Frames[pos].hWnd,GWL_STYLE);
	style|=!LayeredFlag?WS_BORDER:0;
    if(flag&F_NOBORDER) {style&=(~WS_BORDER);};
    {
      SetWindowLong(Frames[pos].hWnd,GWL_STYLE,(long)style);
      SetWindowLong(Frames[pos].TitleBar.hwnd,GWL_STYLE,(long)style);
    }
    ulockfrm();
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
    SetWindowPos(Frames[pos].TitleBar.hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED|SWP_NOACTIVATE);
    return 0;
                }

  case FO_NAME:
    if(lParam==(LPARAM)NULL) {ulockfrm(); return -1;}
    if(Frames[pos].name!=NULL) free(Frames[pos].name);
    Frames[pos].name=_strdup((char *)lParam);
    ulockfrm();
    return 0;

  case FO_TBNAME:
    if(lParam==(LPARAM)NULL) {ulockfrm(); return(-1);}
    if(Frames[pos].TitleBar.tbname!=NULL) free(Frames[pos].TitleBar.tbname);
    Frames[pos].TitleBar.tbname=_strdup((char *)lParam);
    ulockfrm();
    if (Frames[pos].floating&&(Frames[pos].TitleBar.tbname!=NULL)){SetWindowTextA(Frames[pos].ContainerWnd,Frames[pos].TitleBar.tbname);};
    return 0;

  case FO_TBTIPNAME:
    if(lParam==(LPARAM)NULL) {ulockfrm(); return(-1);}
    if(Frames[pos].TitleBar.tooltip!=NULL) free(Frames[pos].TitleBar.tooltip);
    Frames[pos].TitleBar.tooltip=_strdup((char *)lParam);
    UpdateTBToolTip(pos);
    ulockfrm();
    return 0;

  case FO_TBSTYLE:
    SetWindowLong(Frames[pos].TitleBar.hwnd,GWL_STYLE,lParam);
    ulockfrm();
    return 0;

  case FO_TBEXSTYLE:
    SetWindowLong(Frames[pos].TitleBar.hwnd,GWL_EXSTYLE,lParam);						
    ulockfrm();
    return 0;

  case FO_ICON:
    Frames[pos].TitleBar.hicon=(HICON)lParam;
    ulockfrm();
    return 0;

  case FO_HEIGHT:
    if(lParam<0) {ulockfrm(); return -1;}
    if (Frames[pos].collapsed)
    {
      retval=Frames[pos].height;
      Frames[pos].height=lParam;
      if(!CLUIFramesFitInSize()) Frames[pos].height=retval;
      retval=Frames[pos].height;
      ulockfrm();
    }
    else
    {
      retval=Frames[pos].HeightWhenCollapsed;
      Frames[pos].HeightWhenCollapsed=lParam;
      if(!CLUIFramesFitInSize()) Frames[pos].HeightWhenCollapsed=retval;
      retval=Frames[pos].HeightWhenCollapsed;
      ulockfrm();
    }

    return retval;

  case FO_FLOATING:
    if(lParam<0) {ulockfrm(); return -1;}

    {
      int id=Frames[pos].id;
      Frames[pos].floating=!(lParam);
      ulockfrm();

      CLUIFrameSetFloat(id,1);//lparam=1 use stored width and height
      return(wParam);
    }

  case FO_ALIGN:
    if (!(lParam&alTop||lParam&alBottom||lParam&alClient))
    {
      TRACE("Wrong align option \r\n");
      return (-1);
    };

    if((lParam&alClient)&&(CLUIFramesGetalClientFrame()>=0)) {	//only one alClient frame possible
      alclientFrame=-1;//recalc it
      ulockfrm();
      return -1;
    }
    Frames[pos].align=lParam;

    ulockfrm();
    return(0);
  }
  ulockfrm();
  CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
  return -1;
}

//wparam=lparam=0
static int CLUIFramesShowAll(WPARAM wParam,LPARAM lParam)
{
  int i;

  if (FramesSysNotStarted) return -1;

  for(i=0;i<nFramescount;i++)
    Frames[i].visible=TRUE;
  CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
  return 0;
}

//wparam=lparam=0
int CLUIFramesShowAllTitleBars(WPARAM wParam,LPARAM lParam)
{
  int i;

  if (FramesSysNotStarted) return -1;

  for(i=0;i<nFramescount;i++)
    Frames[i].TitleBar.ShowTitleBar=TRUE;
  CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
  return 0;
}

//wparam=lparam=0
int CLUIFramesHideAllTitleBars(WPARAM wParam,LPARAM lParam)
{
  int i;

  if (FramesSysNotStarted) return -1;

  for(i=0;i<nFramescount;i++)
    Frames[i].TitleBar.ShowTitleBar=FALSE;
  CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
  return 0;
}

//wparam=frameid
int CLUIFramesShowHideFrame(WPARAM wParam,LPARAM lParam)
{
  int pos;

  if (FramesSysNotStarted) return -1;

  lockfrm();
  pos=id2pos(wParam);
  if(pos>=0&&(int)pos<nFramescount)
    Frames[pos].visible=!Frames[pos].visible;
  if (Frames[pos].OwnerWindow!=(HWND)-2)
  {
    if (Frames[pos].OwnerWindow) 
      ShowWindowNew(Frames[pos].OwnerWindow,(Frames[pos].visible&& Frames[pos].collapsed && IsWindowVisible(pcli->hwndContactList))?SW_SHOW/*NOACTIVATE*/:SW_HIDE);
    else if (Frames[pos].visible)
    {
      Frames[pos].OwnerWindow=CreateSubContainerWindow(pcli->hwndContactList,Frames[pos].FloatingPos.x,Frames[pos].FloatingPos.y,10,10);
      SetParent(Frames[pos].hWnd,Frames[pos].OwnerWindow);
      ShowWindowNew(Frames[pos].OwnerWindow,(Frames[pos].visible && Frames[pos].collapsed && IsWindowVisible(pcli->hwndContactList))?SW_SHOW/*NOACTIVATE*/:SW_HIDE);
    }
  }
  if (Frames[pos].floating){CLUIFrameResizeFloatingFrame(pos);};
  ulockfrm();
  if (!Frames[pos].floating) CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
  return 0;
}

//wparam=frameid
int CLUIFramesShowHideFrameTitleBar(WPARAM wParam,LPARAM lParam)
{
  int pos;

  if (FramesSysNotStarted) return -1;

  lockfrm();
  pos=id2pos(wParam);
  if(pos>=0&&(int)pos<nFramescount)
    Frames[pos].TitleBar.ShowTitleBar=!Frames[pos].TitleBar.ShowTitleBar;
  //if (Frames[pos].height>

  ulockfrm();

  CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);


  return 0;
}
//wparam=frameid
//lparam=-1 up ,1 down
int CLUIFramesMoveUpDown(WPARAM wParam,LPARAM lParam)
{
  int pos,i,curpos,curalign,v,tmpval;

  if (FramesSysNotStarted) return -1;
  TRACE("CLUIFramesMoveUpDown\n");
  lockfrm();
  pos=id2pos(wParam);
  if(pos>=0&&(int)pos<nFramescount)	{
    SortData *sd;
    curpos=Frames[pos].order;
    curalign=Frames[pos].align;
    v=0;
    sd=(SortData*)malloc(sizeof(SortData)*nFramescount);
    memset(sd,0,sizeof(SortData)*nFramescount);
    for (i=0;i<nFramescount;i++)
    {
      if (Frames[i].floating||(!Frames[i].visible)||(Frames[i].align!=curalign)){continue;};

      sd[v].order=Frames[i].order;
      sd[v].realpos=i;
      v++;
    };
    if (v==0){ulockfrm();return(0);};
    qsort(sd,v,sizeof(SortData),sortfunc);	
    for (i=0;i<v;i++)
    {
      if (sd[i].realpos==pos)
      {
        if (lParam==-1)
        {
          if (i>=v-1) break;
          tmpval=Frames[sd[i+1].realpos].order;
          Frames[sd[i+1].realpos].order=Frames[pos].order;
          Frames[pos].order=tmpval;
          break;
        };
        if (lParam==+1)
        {
          if (i<1) break;
          tmpval=Frames[sd[i-1].realpos].order;
          Frames[sd[i-1].realpos].order=Frames[pos].order;
          Frames[pos].order=tmpval;
          break;
        };


      };
    };

    if (sd!=NULL){free(sd);};
    CLUIFramesStoreFrameSettings(pos);
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);		

  }
  ulockfrm();
  return(0);
};



//wparam=frameid
//lparam=alignment
int CLUIFramesSetAlign(WPARAM wParam,LPARAM lParam)
{
  if (FramesSysNotStarted) return -1;

  CLUIFramesSetFrameOptions(MAKEWPARAM(FO_ALIGN,wParam),lParam);
  CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);		
  return(0);
}
int CLUIFramesSetAlignalTop(WPARAM wParam,LPARAM lParam)
{
  if (FramesSysNotStarted) return -1;

  return CLUIFramesSetAlign(wParam,alTop);
}
int CLUIFramesSetAlignalBottom(WPARAM wParam,LPARAM lParam)
{
  if (FramesSysNotStarted) return -1;

  return CLUIFramesSetAlign(wParam,alBottom);
}
int CLUIFramesSetAlignalClient(WPARAM wParam,LPARAM lParam)
{
  if (FramesSysNotStarted) return -1;

  return CLUIFramesSetAlign(wParam,alClient);
}


//wparam=frameid
int CLUIFramesLockUnlockFrame(WPARAM wParam,LPARAM lParam)
{
  int pos;

  if (FramesSysNotStarted) return -1;

  lockfrm();
  pos=id2pos(wParam);
  if(pos>=0&&(int)pos<nFramescount)	{
    Frames[pos].Locked=!Frames[pos].Locked;
    CLUIFramesStoreFrameSettings(pos);
  }
  ulockfrm();
  return 0;
}

//wparam=frameid
int CLUIFramesSetUnSetBorder(WPARAM wParam,LPARAM lParam)
{
  RECT rc;
  int FrameId,oldflags;
  HWND hw;
  boolean flt;

  if (FramesSysNotStarted) return -1;

  lockfrm();
  FrameId=id2pos(wParam);
  if (FrameId==-1){ulockfrm();return(-1);};
  flt=
    oldflags=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,wParam),0);
  if (oldflags&F_NOBORDER)
  {
    oldflags&=(~F_NOBORDER);
  }
  else
  {
    oldflags|=F_NOBORDER;
  };
  hw = Frames[FrameId].hWnd;
 GetWindowRect(hw,&rc);

  ulockfrm();
  CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,wParam),oldflags);
  //		CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
  /*
  if (Frames[FrameId].floating){ 
  CLUIFrameResizeFloatingFrame(FrameId);
  };
  */

  {
    SetWindowPos(hw,0,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_DRAWFRAME);
  };
  return(0);
};
//wparam=frameid
int CLUIFramesCollapseUnCollapseFrame(WPARAM wParam,LPARAM lParam)
{
  int FrameId;

  if (FramesSysNotStarted) return -1;

  lockfrm();
  FrameId=id2pos(wParam);
  if(FrameId>=0&&FrameId<nFramescount)
  {
    int oldHeight;

    // do not collapse/uncollapse client/locked/invisible frames
    if(Frames[FrameId].align==alClient&&!(Frames[FrameId].Locked||(!Frames[FrameId].visible)||Frames[FrameId].floating)) 
    {
      RECT rc;
      if(CallService(MS_CLIST_DOCKINGISDOCKED,0,0)) {ulockfrm();return 0;};
      if(DBGetContactSettingByte(NULL,"CLUI","AutoSize",0)) {ulockfrm();return 0;};
     GetWindowRect(pcli->hwndContactList,&rc);

      if(Frames[FrameId].collapsed==TRUE)	{
        rc.bottom-=rc.top;
        rc.bottom-=Frames[FrameId].height;
        Frames[FrameId].HeightWhenCollapsed=Frames[FrameId].height;
        Frames[FrameId].collapsed=FALSE;
      }
      else
      {
        rc.bottom-=rc.top;
        rc.bottom+=Frames[FrameId].HeightWhenCollapsed;			
        Frames[FrameId].collapsed=TRUE;
      }

      SetWindowPos(pcli->hwndContactList,NULL,0,0,rc.right-rc.left,rc.bottom,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE);

      CLUIFramesStoreAllFrames();
      ulockfrm();

      return 0;

    }
    if(Frames[FrameId].Locked||(!Frames[FrameId].visible)) return 0;

    oldHeight=Frames[FrameId].height;

    // if collapsed, uncollapse
    if(Frames[FrameId].collapsed==TRUE)	{
      Frames[FrameId].HeightWhenCollapsed=Frames[FrameId].height;
      Frames[FrameId].height=UNCOLLAPSED_FRAME_SIZE;
      Frames[FrameId].collapsed=FALSE;
    }
    // if uncollapsed, collapse
    else {
      Frames[FrameId].height=Frames[FrameId].HeightWhenCollapsed;
      Frames[FrameId].collapsed=TRUE;
    }

    if (!Frames[FrameId].floating)
    {

      if(!CLUIFramesFitInSize()) {
        //cant collapse,we can resize only for height<alclient frame height
        int alfrm=CLUIFramesGetalClientFrame();

        if(alfrm!=-1) {
          Frames[FrameId].collapsed=FALSE;
          if(Frames[alfrm].height>2*UNCOLLAPSED_FRAME_SIZE) {
            oldHeight=Frames[alfrm].height-UNCOLLAPSED_FRAME_SIZE;
            Frames[FrameId].collapsed=TRUE;
          }
        }else
        {
          int i,sumheight=0;
          for(i=0;i<nFramescount;i++) {
            if((Frames[i].align!=alClient)&&(!Frames[i].floating)&&(Frames[i].visible)&&(!Frames[i].needhide)) {
              sumheight+=(Frames[i].height)+(TitleBarH*btoint(Frames[i].TitleBar.ShowTitleBar))+2;
              return FALSE;
            }
            if(sumheight>ContactListHeight-0-2)
            {
              Frames[FrameId].height=(ContactListHeight-0-2)-sumheight;
            }

          }
        }

        Frames[FrameId].height=oldHeight;

        if(Frames[FrameId].collapsed==FALSE) {

          if (!Frames[FrameId].floating)
          {
          }
          else
          {
            //SetWindowPos(Frames[FrameId].hWnd,HWND_TOP,0,0,Frames[FrameId].wndSize.right-Frames[FrameId].wndSize.left,Frames[FrameId].height,SWP_SHOWWINDOW|SWP_NOMOVE);		  
            SetWindowPos(Frames[FrameId].ContainerWnd,HWND_TOP,0,0,Frames[FrameId].wndSize.right-Frames[FrameId].wndSize.left+6,Frames[FrameId].height+DEFAULT_TITLEBAR_HEIGHT+4,SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOMOVE);		  
          };


          ulockfrm();return -1;};//redraw not needed
      }
    };//floating test
    ulockfrm();
    //CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);		
    if (!Frames[FrameId].floating)
    {
      CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);		
    }
    else
    {
      //SetWindowPos(Frames[FrameId].hWnd,HWND_TOP,0,0,Frames[FrameId].wndSize.right-Frames[FrameId].wndSize.left,Frames[FrameId].height,SWP_SHOWWINDOW|SWP_NOMOVE);		  
      RECT contwnd;
     GetWindowRect(Frames[FrameId].ContainerWnd,&contwnd);
      contwnd.top=contwnd.bottom-contwnd.top;//height
      contwnd.left=contwnd.right-contwnd.left;//width

      contwnd.top-=(oldHeight-Frames[FrameId].height);//newheight
      SetWindowPos(Frames[FrameId].ContainerWnd,HWND_TOP,0,0,contwnd.left,contwnd.top,SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOMOVE);		  
    };
    CLUIFramesStoreAllFrames();
    return(0);
  }
  else
    return -1;

  ulockfrm();

  return 0;
}

static int CLUIFramesLoadMainMenu()
{
  CLISTMENUITEM mi;
  //TMO_MenuItem tmi;
  int i,separator;

  if (FramesSysNotStarted) return -1;

  if (!(ServiceExists(MS_CLIST_REMOVEMAINMENUITEM)))
  {
    //hmm new menu system not used..so display only two items and warning message
    ZeroMemory(&mi,sizeof(mi));
    mi.cbSize=sizeof(mi);
    // create "show all frames" menu
    mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCEA(IDI_MIRANDA));
    mi.flags=CMIF_GRAYED;
    mi.position=10000000;
    mi.pszPopupName=Translate("Frames");
    mi.pszName=Translate("New Menu System not Found...");
    mi.pszService="";
    CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);

    // create "show all frames" menu
    mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCEA(IDI_MIRANDA));
    mi.flags=0;
    mi.position=10100000;
    mi.pszPopupName=Translate("Frames");
    mi.pszName=Translate("Show All Frames");
    mi.pszService=MS_CLIST_FRAMES_SHOWALLFRAMES;
    CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);

    mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCEA(IDI_HELP));
    mi.position=10100001;
    mi.pszPopupName=Translate("Frames");
    mi.flags=CMIF_CHILDPOPUP;
    mi.pszName=Translate("Show All Titlebars");
    mi.pszService=MS_CLIST_FRAMES_SHOWALLFRAMESTB;
    CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);	

    return(0);
  };


  if(MainMIRoot!=(HANDLE)-1) { CallService(MS_CLIST_REMOVEMAINMENUITEM,(WPARAM)MainMIRoot,0); MainMIRoot=(HANDLE)-1;}

  ZeroMemory(&mi,sizeof(mi));
  mi.cbSize=sizeof(mi);

  // create root menu
  mi.hIcon=LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_MIRANDA));
  mi.flags=CMIF_ROOTPOPUP;
  mi.position=3000090000;
  mi.pszPopupName=(char*)-1;
  mi.pszName=Translate("Frames");
  mi.pszService=0;
  MainMIRoot=(HANDLE)CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);

  // create frames menu
  separator=3000200000;
  for (i=0;i<nFramescount;i++) {
    mi.hIcon=Frames[i].TitleBar.hicon;
    mi.flags=CMIF_CHILDPOPUP|CMIF_ROOTPOPUP;
    mi.position=separator;
    mi.pszPopupName=(char*)MainMIRoot;
    mi.pszName=Frames[i].name;
    mi.pszService=0;
    Frames[i].MenuHandles.MainMenuItem=(HANDLE)CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
    CLUIFramesCreateMenuForFrame(Frames[i].id,(int)Frames[i].MenuHandles.MainMenuItem,separator,MS_CLIST_ADDMAINMENUITEM);
    CLUIFramesModifyMainMenuItems(Frames[i].id,0);
    //NotifyEventHooks(hPreBuildFrameMenuEvent,i,(LPARAM)Frames[i].MenuHandles.MainMenuItem);
    CallService(MS_CLIST_FRAMEMENUNOTIFY,(WPARAM)Frames[i].id,(LPARAM)Frames[i].MenuHandles.MainMenuItem);
    separator++;
  }

  separator+=100000;

  // create "show all frames" menu
  mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCEA(IDI_MIRANDA));
  mi.flags=CMIF_CHILDPOPUP;
  mi.position=separator++;
  mi.pszPopupName=(char*)MainMIRoot;
  mi.pszName=Translate("Show All Frames");
  mi.pszService=MS_CLIST_FRAMES_SHOWALLFRAMES;
  CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);

  // create "show all titlebars" menu
  mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCEA(IDI_HELP));
  mi.position=separator++;
  mi.pszPopupName=(char*)MainMIRoot;
  mi.flags=CMIF_CHILDPOPUP;
  mi.pszName=Translate("Show All Titlebars");
  mi.pszService=MS_CLIST_FRAMES_SHOWALLFRAMESTB;
  CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);

  // create "hide all titlebars" menu
  mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCEA(IDI_HELP));
  mi.position=separator++;
  mi.pszPopupName=(char*)MainMIRoot;
  mi.flags=CMIF_CHILDPOPUP;
  mi.pszName=Translate("Hide All Titlebars");
  mi.pszService=MS_CLIST_FRAMES_HIDEALLFRAMESTB;
  CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);

  return 0;
}

static int CLUILoadTitleBarFont()
{
  char facename[]="MS Shell Dlg";
  HFONT hfont;
  LOGFONTA logfont;
  memset(&logfont,0,sizeof(logfont));
  memcpy(logfont.lfFaceName,facename,sizeof(facename));
  logfont.lfWeight=FW_NORMAL;
  logfont.lfHeight=-10;
  logfont.lfCharSet=DEFAULT_CHARSET;
  hfont=CreateFontIndirectA(&logfont);
  return((int)hfont);
}

static int UpdateTBToolTip(int framepos)
{
  {
    TOOLINFOA ti;

    ZeroMemory(&ti,sizeof(ti));
    ti.cbSize=sizeof(ti);
    ti.lpszText=Frames[framepos].TitleBar.tooltip;
    ti.hinst=g_hInst;
    ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS ;
    ti.uId=(UINT)Frames[framepos].TitleBar.hwnd;

    return(SendMessageA(Frames[framepos].TitleBar.hwndTip,TTM_UPDATETIPTEXT ,(WPARAM)0,(LPARAM)&ti));
  }	

};

//wparam=(CLISTFrame*)clfrm
int CLUIFramesAddFrame(WPARAM wParam,LPARAM lParam)
{
  int style,retval;
  char * CustomName=NULL;
  char buff[255];
  CLISTFrame *clfrm=(CLISTFrame *)wParam;

  if(pcli->hwndContactList==0) return -1;
  if (FramesSysNotStarted) return -1;
  if(clfrm->cbSize!=sizeof(CLISTFrame)) return -1;
  if(!(TitleBarFont)) TitleBarFont=(HFONT)CLUILoadTitleBarFont();


  lockfrm();
  if(nFramescount>=MAX_FRAMES) { ulockfrm(); return -1;}
  Frames=(wndFrame*)realloc(Frames,sizeof(wndFrame)*(nFramescount+1));

  memset(&Frames[nFramescount],0,sizeof(wndFrame));
  if (clfrm->name)
  {
    CustomName=DBGetStringA(NULL,"CUSTOM_CLUI_FRAMES",AS(buff,"CustomName_",clfrm->name));
    Frames[nFramescount].TitleBar.BackColour=(COLORREF)DBGetContactSettingDword(NULL,"CUSTOM_CLUI_FRAMES",AS(buff,"CustomBackColor_",clfrm->name),GetSysColor(COLOR_3DFACE));
    Frames[nFramescount].TitleBar.TextColour=(COLORREF)DBGetContactSettingDword(NULL,"CUSTOM_CLUI_FRAMES",AS(buff,"CustomTextColor_",clfrm->name),GetSysColor(COLOR_WINDOWTEXT));
    if (CustomName)
    {
      if (clfrm->name) mir_free(clfrm->name);
      clfrm->name=strdup(CustomName);
      mir_free(CustomName);
    }
  }
  Frames[nFramescount].id=NextFrameId++;
  Frames[nFramescount].align=clfrm->align;
  Frames[nFramescount].hWnd=clfrm->hWnd;
  Frames[nFramescount].height=clfrm->height;
  Frames[nFramescount].TitleBar.hicon=clfrm->hIcon;
  //Frames[nFramescount].TitleBar.BackColour;
  Frames[nFramescount].floating=FALSE;
  if (clfrm->Flags&F_NO_SUBCONTAINER || !LayeredFlag)
    Frames[nFramescount].OwnerWindow=(HWND)-2;
  else Frames[nFramescount].OwnerWindow=0;

  //override tbbtip
  //clfrm->Flags|=F_SHOWTBTIP;
  //
  if (DBGetContactSettingByte(0,CLUIFrameModule,"RemoveAllBorders",0)==1)
  {
    clfrm->Flags|=F_NOBORDER;
  };
  Frames[nFramescount].dwFlags=clfrm->Flags;

  if(clfrm->name==NULL||MyStrLen(clfrm->name)==0) {
    Frames[nFramescount].name=(char *)malloc(255);
    GetClassNameA(Frames[nFramescount].hWnd,Frames[nFramescount].name,255);
  }
  else
    Frames[nFramescount].name=_strdup(clfrm->name);		
  if(clfrm->TBname==NULL||MyStrLen(clfrm->TBname)==0)
    Frames[nFramescount].TitleBar.tbname=_strdup(Frames[nFramescount].name);
  else
    Frames[nFramescount].TitleBar.tbname=_strdup(clfrm->TBname);
  Frames[nFramescount].needhide=FALSE;
  Frames[nFramescount].TitleBar.ShowTitleBar=(clfrm->Flags&F_SHOWTB?TRUE:FALSE);
  Frames[nFramescount].TitleBar.ShowTitleBarTip=(clfrm->Flags&F_SHOWTBTIP?TRUE:FALSE);

  Frames[nFramescount].collapsed = (clfrm->Flags&F_UNCOLLAPSED)?FALSE:TRUE;


  Frames[nFramescount].Locked = clfrm->Flags&F_LOCKED?TRUE:FALSE;
  Frames[nFramescount].visible = clfrm->Flags&F_VISIBLE?TRUE:FALSE;

  Frames[nFramescount].UseBorder=((clfrm->Flags&F_NOBORDER)||LayeredFlag)?FALSE:TRUE;

  //Frames[nFramescount].OwnerWindow=0;



  Frames[nFramescount].TitleBar.hwnd
    =CreateWindowA(CLUIFrameTitleBarClassName,Frames[nFramescount].name,
    (DBGetContactSettingByte(0,CLUIFrameModule,"RemoveAllTitleBarBorders",1)?0:WS_BORDER)

    |WS_CHILD|WS_CLIPCHILDREN|
    (Frames[nFramescount].TitleBar.ShowTitleBar?WS_VISIBLE:0)|
    WS_CLIPCHILDREN,
    0,0,0,0,pcli->hwndContactList,NULL,g_hInst,NULL);
  SetWindowLong(Frames[nFramescount].TitleBar.hwnd,GWL_USERDATA,Frames[nFramescount].id);


  Frames[nFramescount].TitleBar.hwndTip 
    =CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
    WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
    CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT,
    pcli->hwndContactList, NULL, g_hInst,
    NULL);

  SetWindowPos(Frames[nFramescount].TitleBar.hwndTip, HWND_TOPMOST,0, 0, 0, 0,
    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  {
    TOOLINFOA ti;
    int res;

    ZeroMemory(&ti,sizeof(ti));
    ti.cbSize=sizeof(ti);
    ti.lpszText="";
    ti.hinst=g_hInst;
    ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS ;
    ti.uId=(UINT)Frames[nFramescount].TitleBar.hwnd;
    res=SendMessageA(Frames[nFramescount].TitleBar.hwndTip,TTM_ADDTOOL,(WPARAM)0,(LPARAM)&ti);
  }

  SendMessageA(Frames[nFramescount].TitleBar.hwndTip,TTM_ACTIVATE,(WPARAM)Frames[nFramescount].TitleBar.ShowTitleBarTip,0);

  Frames[nFramescount].oldstyles=GetWindowLong(Frames[nFramescount].hWnd,GWL_STYLE);
  Frames[nFramescount].TitleBar.oldstyles=GetWindowLong(Frames[nFramescount].TitleBar.hwnd,GWL_STYLE);
  //Frames[nFramescount].FloatingPos.x=

  retval=Frames[nFramescount].id;
  Frames[nFramescount].order=nFramescount+1;
  nFramescount++;


  CLUIFramesLoadFrameSettings(id2pos(retval));
  if (Frames[nFramescount-1].collapsed==FALSE)
  {
    Frames[nFramescount-1].height=0;
  }

  // create frame


  //    else Frames[nFramescount-1].height=Frames[nFramescount-1].HeightWhenCollapsed;

  style=GetWindowLong(Frames[nFramescount-1].hWnd,GWL_STYLE);
  style&=(~WS_BORDER);
  style|=(((Frames[nFramescount-1].UseBorder)&&!LayeredFlag)?WS_BORDER:0);
  SetWindowLong(Frames[nFramescount-1].hWnd,GWL_STYLE,style);
  SetWindowLong(Frames[nFramescount-1].TitleBar.hwnd,GWL_STYLE,style);
  SetWindowLong(Frames[nFramescount-1].TitleBar.hwnd,GWL_STYLE,GetWindowLong(Frames[nFramescount-1].TitleBar.hwnd,GWL_STYLE)&~(WS_VSCROLL|WS_HSCROLL));

  if (Frames[nFramescount-1].order==0){Frames[nFramescount-1].order=nFramescount;};
  ulockfrm();


  alclientFrame=-1;//recalc it
  CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);

  if (Frames[nFramescount-1].floating)
  {

    Frames[nFramescount-1].floating=FALSE;
    //SetWindowPos(Frames[nFramescount-1].hw
    CLUIFrameSetFloat(retval,1);//lparam=1 use stored width and height
  }
  else
    CLUIFrameSetFloat(retval,2);


  return retval;
}

static int CLUIFramesRemoveFrame(WPARAM wParam,LPARAM lParam)
{
  int pos;
  if (FramesSysNotStarted) return -1;
  lockfrm();
  pos=id2pos(wParam);

  if (pos<0||pos>nFramescount){ulockfrm();return(-1);};

  TRACE("Remove Frame: '");
  TRACE(Frames[pos].name);
  TRACE("'\n");
  if (Frames[pos].name!=NULL) free(Frames[pos].name);
  if (Frames[pos].TitleBar.tbname!=NULL) free(Frames[pos].TitleBar.tbname);
  if (Frames[pos].TitleBar.tooltip!=NULL) free(Frames[pos].TitleBar.tooltip);
  /*
  {
  //		CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MainMenuItem,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].TitleBar..MenuHandles.MIAlignBottom,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIAlignClient,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIAlignRoot,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIAlignTop,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIBorder,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIColl,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIFloating,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MILock,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MITBVisible,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MITitle,0);
  CallService(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIVisible,0);
  }
  */	
  DestroyWindow(Frames[pos].hWnd);
  Frames[pos].hWnd=(HWND)-1;
  DestroyWindow(Frames[pos].TitleBar.hwnd);
  Frames[pos].TitleBar.hwnd=(HWND)-1;
  DestroyWindow(Frames[pos].ContainerWnd);
  Frames[pos].ContainerWnd=(HWND)-1;
  DestroyMenu(Frames[pos].TitleBar.hmenu);

  RemoveItemFromList(pos,&Frames,&nFramescount);

  ulockfrm();
  skinInvalidateRect(pcli->hwndContactList,NULL,TRUE);
  CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);
  skinInvalidateRect(pcli->hwndContactList,NULL,TRUE);

  return(0);
};


int CLUIFramesForceUpdateTB(const wndFrame *Frame)
{
  if (Frame->TitleBar.hwnd!=0) RedrawWindow(Frame->TitleBar.hwnd,NULL,NULL,RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE|RDW_INVALIDATE|RDW_FRAME);
  //UpdateWindow(Frame->TitleBar.hwnd);
  return 0;
}

int CLUIFramesForceUpdateFrame(const wndFrame *Frame)
{
  TRACE("CLUIFramesForceUpdateFrame\n");
  if (Frame->hWnd!=0) 
  {
    RedrawWindow(Frame->hWnd,NULL,NULL,RDW_UPDATENOW|RDW_FRAME|RDW_ERASE|RDW_INVALIDATE);	
    UpdateWindow(Frame->hWnd);
  };
  if(Frame->floating)
  {
    if (Frame->ContainerWnd!=0)  RedrawWindow(Frame->ContainerWnd,NULL,NULL,RDW_UPDATENOW|RDW_ALLCHILDREN|RDW_ERASE|RDW_INVALIDATE|RDW_FRAME);
    //UpdateWindow(Frame->ContainerWnd);
  };
  return 0;
}

int CLUIFrameMoveResize(const wndFrame *Frame)
{
  TRACE("CLUIFrameMoveResize\n");
  // we need to show or hide the frame?
  if(Frame->visible&&(!Frame->needhide)) {
    if (Frame->OwnerWindow!=(HWND)-2 &&Frame->OwnerWindow)         
    {
      //          ShowWindowNew(Frame->OwnerWindow,SW_SHOW);
    }
    ShowWindowNew(Frame->hWnd,SW_SHOW/*NOACTIVATE*/);
    ShowWindowNew(Frame->TitleBar.hwnd,Frame->TitleBar.ShowTitleBar==TRUE?SW_SHOW/*NOACTIVATE*/:SW_HIDE);
  }
  else {
    if (Frame->OwnerWindow)         
    {
      ShowWindowNew(Frame->OwnerWindow,SW_HIDE);
    }
    ShowWindowNew(Frame->hWnd,SW_HIDE);
    ShowWindowNew(Frame->TitleBar.hwnd,SW_HIDE);
    return(0);
  }

  if (Frame->OwnerWindow&&Frame->OwnerWindow!=(HWND)-2 )         
  {
    RECT pr;
    POINT Off={0};

    ClientToScreen(pcli->hwndContactList,&Off);
   GetWindowRect(pcli->hwndContactList,&pr);
    //dock_prevent_moving=0;

    SetWindowPos(Frame->OwnerWindow,NULL,Frame->wndSize.left+Off.x,Frame->wndSize.top+Off.y,
      Frame->wndSize.right-Frame->wndSize.left,
      Frame->wndSize.bottom-Frame->wndSize.top,SWP_NOZORDER|SWP_NOACTIVATE/*|SWP_NOREDRAW*/);   //--=-=

    SetWindowPos(Frame->hWnd,NULL,0,0,
      Frame->wndSize.right-Frame->wndSize.left,
      Frame->wndSize.bottom-Frame->wndSize.top,SWP_NOZORDER|SWP_NOACTIVATE);
    // set titlebar position
    if(Frame->TitleBar.ShowTitleBar) {
      SetWindowPos(Frame->TitleBar.hwnd,NULL,Frame->wndSize.left,Frame->wndSize.top-TitleBarH-GapBetweenTitlebar,
        Frame->wndSize.right-Frame->wndSize.left,
        TitleBarH,SWP_NOZORDER|SWP_NOACTIVATE	);
      // dock_prevent_moving=1;
    }

  }
  else
  {
    // set frame position
    SetWindowPos(Frame->hWnd,NULL,Frame->wndSize.left,Frame->wndSize.top,
      Frame->wndSize.right-Frame->wndSize.left,
      Frame->wndSize.bottom-Frame->wndSize.top,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW);
    // set titlebar position
    if(Frame->TitleBar.ShowTitleBar) {
      SetWindowPos(Frame->TitleBar.hwnd,NULL,Frame->wndSize.left,Frame->wndSize.top-TitleBarH-GapBetweenTitlebar,
        Frame->wndSize.right-Frame->wndSize.left,
        TitleBarH,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW	);

    }
  }
  //	Sleep(0);
  return 0;
}

BOOLEAN CLUIFramesFitInSize(void)
{
  int i;
  int sumheight=0;
  int tbh=0; // title bar height
  int clientfrm;
  clientfrm=CLUIFramesGetalClientFrame();
  if(clientfrm!=-1)
    tbh=TitleBarH*btoint(Frames[clientfrm].TitleBar.ShowTitleBar);

  for(i=0;i<nFramescount;i++) {
    if((Frames[i].align!=alClient)&&(!Frames[i].floating)&&(Frames[i].visible)&&(!Frames[i].needhide)) {
      sumheight+=(Frames[i].height)+(TitleBarH*btoint(Frames[i].TitleBar.ShowTitleBar))+2/*+btoint(Frames[i].UseBorder)*2*/;
      if(sumheight>ContactListHeight-tbh-2)
        return FALSE;
    }
  }
  return TRUE;
}

int CLUIFramesGetMinHeight()
{
  int i,tbh,clientfrm,sumheight=0;
  RECT border;
  int allbord=0;
  if(pcli->hwndContactList==NULL) return 0; 
  lockfrm();

  // search for alClient frame and get the titlebar's height
  tbh=0;
  clientfrm=CLUIFramesGetalClientFrame();
  if(clientfrm!=-1)
    tbh=TitleBarH*btoint(Frames[clientfrm].TitleBar.ShowTitleBar);

  for(i=0;i<nFramescount;i++)
  {
    if((Frames[i].align!=alClient)&&(Frames[i].visible)&&(!Frames[i].needhide)&&(!Frames[i].floating))
    {
      RECT wsize; 

     GetWindowRect(Frames[i].hWnd,&wsize);
      sumheight+=(wsize.bottom-wsize.top)+(TitleBarH*btoint(Frames[i].TitleBar.ShowTitleBar))+3;
    }
  };
  ulockfrm();
  GetBorderSize(pcli->hwndContactList,&border);

  //GetWindowRect(pcli->hwndContactList,&winrect);
  //GetClientRect(pcli->hwndContactList,&clirect);
  //	clirect.bottom-=clirect.top;
  //	clirect.bottom+=border.top+border.bottom;
  //allbord=(winrect.bottom-winrect.top)-(clirect.bottom-clirect.top);

  //TODO minsize
  sumheight+=DBGetContactSettingByte(NULL,"CLUI","TopClientMargin",0);
  sumheight+=DBGetContactSettingByte(NULL,"CLUI","BottomClientMargin",0); //$$ BOTTOM border
  return  max(DBGetContactSettingWord(NULL,"CLUI","MinHeight",0),
        (sumheight+border.top+border.bottom+allbord+tbh+3)       );
}



//int CLUIFramesResize(const RECT newsize, int mode)
//{
//	int sumheight=9999999,newheight;
//	int prevframe,prevframebottomline;
//	int tbh,curfrmtbh;
//	int drawitems;
//	int clientfrm;
//	int i,j;
//	int sepw=GapBetweenFrames;
//	SortData *sdarray;
//	
//	
//	GapBetweenTitlebar=(int)DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",1);
//	GapBetweenFrames=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);
//	sepw=GapBetweenFrames;
//
//	if(nFramescount<1) return 0; 
//
//	
//	newheight=newsize.bottom-newsize.top;
//
//	
//
//	// search for alClient frame and get the titlebar's height
//	tbh=0;
//	clientfrm=CLUIFramesGetalClientFrame();
//	if(clientfrm!=-1)
//		tbh=(TitleBarH+GapBetweenTitlebar)*btoint(Frames[clientfrm].TitleBar.ShowTitleBar);
//	
//	for(i=0;i<nFramescount;i++)
//	{
//		if (!Frames[i].floating)
//		{
//		Frames[i].needhide=FALSE;
//		Frames[i].wndSize.left=0;
//		Frames[i].wndSize.right=newsize.right-0;
//		};
//	};
//	{
//		//sorting stuff
//		sdarray=(SortData*)malloc(sizeof(SortData)*nFramescount);
//		if (sdarray==NULL){return(-1);};
//		for(i=0;i<nFramescount;i++)
//		{	sdarray[i].order=Frames[i].order;
//			sdarray[i].realpos=i;
//		};
//		qsort(sdarray,nFramescount,sizeof(SortData),sortfunc);
//
//	}
//
//	drawitems=nFramescount;
//	
//	while(sumheight>(newheight-tbh)&&drawitems>0) {
//		sumheight=0;
//		drawitems=0;
//		for(i=0;i<nFramescount;i++)	{
//			if(((Frames[i].align!=alClient))&&(!Frames[i].floating)&&(Frames[i].visible)&&(!Frames[i].needhide)) {
//				drawitems++;
//				curfrmtbh=(TitleBarH+GapBetweenTitlebar)*btoint(Frames[i].TitleBar.ShowTitleBar);
//				sumheight+=(Frames[i].height)+curfrmtbh+sepw+(Frames[i].UseBorder?2:0);
//				if(sumheight>newheight-tbh) {
//					sumheight-=(Frames[i].height)+curfrmtbh+sepw;
//					Frames[i].needhide=TRUE;
//					drawitems--;
//					break;
//				}
//			}
//		}
//	}
//
//	prevframe=-1;
//	prevframebottomline=0;
//	for(j=0;j<nFramescount;j++) {
//		//move all alTop frames
//		i=sdarray[j].realpos;
//		if((!Frames[i].needhide)&&(!Frames[i].floating)&&(Frames[i].visible)&&(Frames[i].align==alTop)) {
//			curfrmtbh=(TitleBarH+GapBetweenTitlebar)*btoint(Frames[i].TitleBar.ShowTitleBar);
//			Frames[i].wndSize.top=prevframebottomline+sepw+(curfrmtbh);
//			Frames[i].wndSize.bottom=Frames[i].height+Frames[i].wndSize.top+(Frames[i].UseBorder?2:0);
//			Frames[i].prevvisframe=prevframe;
//			prevframe=i;
//			prevframebottomline=Frames[i].wndSize.bottom;
//			if(prevframebottomline>newheight){
//				//prevframebottomline-=Frames[i].height+(curfrmtbh+1);
//				//Frames[i].needhide=TRUE;
//			}
//		}
//	}
//
//	if(sumheight<newheight) {
//		for(j=0;j<nFramescount;j++)	{
//			//move alClient frame
//			i=sdarray[j].realpos;
//			if((!Frames[i].needhide)&&(!Frames[i].floating)&&(Frames[i].visible)&&(Frames[i].align==alClient)) {			
//				int oldh;
//				Frames[i].wndSize.top=prevframebottomline+sepw+(tbh);
//				Frames[i].wndSize.bottom=Frames[i].wndSize.top+newheight-sumheight-tbh-sepw;
//
//				oldh=Frames[i].height;
//				Frames[i].height=Frames[i].wndSize.bottom-Frames[i].wndSize.top;
//				Frames[i].prevvisframe=prevframe;
//				prevframe=i;
//				prevframebottomline=Frames[i].wndSize.bottom;	
//				if(prevframebottomline>newheight) {
//					//prevframebottomline-=Frames[i].height+(tbh+1);
//					//Frames[i].needhide=TRUE;
//				}
//				break;
//			}
//		}
//	}
//
//	//newheight
//	prevframebottomline=newheight+sepw;
//	//prevframe=-1;
//	for(j=nFramescount-1;j>=0;j--) {
//		//move all alBottom frames
//		i=sdarray[j].realpos;
//		if((Frames[i].visible)&&(!Frames[i].floating)&&(!Frames[i].needhide)&&(Frames[i].align==alBottom)) {
//			curfrmtbh=(TitleBarH+GapBetweenTitlebar)*btoint(Frames[i].TitleBar.ShowTitleBar);
//
//			Frames[i].wndSize.bottom=prevframebottomline-sepw;
//			Frames[i].wndSize.top=Frames[i].wndSize.bottom-Frames[i].height-(Frames[i].UseBorder?2:0);
//			Frames[i].prevvisframe=prevframe;
//			prevframe=i;
//			prevframebottomline=Frames[i].wndSize.top/*-1*/-curfrmtbh;
//			if(prevframebottomline>newheight) {
//			
//			}
//		}
//	}
//
//	if (sdarray!=NULL){free(sdarray);sdarray=NULL;};
//	return 0;
//}

//Thread: Modern Contact List discussion 
//View Single Post 	
// Today, 11:21 AM 	  #291 	
//sje 
//Miranda Wizard
// 
//
//Join Date: March 2005
//Location: Melbourne, Australia
//Posts: 228 
//i made changes to the function CLUIFramesResize in cluiframes.c (ln 1982 in avatar mod) to remove the gap before the first frame:
//
//
//Code:
int CLUIFramesResizeFrames(const RECT newsize);
int SizeMoveNewSizes();

int CLUIFramesResize(const RECT newsize)
{
   TRACE("CLUIFramesResize\n");
   CLUIFramesResizeFrames(newsize);
   SizeMoveNewSizes();
   return 0;
}
int CLUIFramesResizeFrames(const RECT newsize)
{
  int sumheight=9999999,newheight;
  int prevframe,prevframebottomline;
  int tbh,curfrmtbh;
  int drawitems;
  int clientfrm;
  int i,j;
  int sepw=GapBetweenFrames;
  int topBorder=newsize.top;
  SortData *sdarray;


  GapBetweenTitlebar=(int)DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",1);
  GapBetweenFrames=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);
  sepw=GapBetweenFrames;

  if(nFramescount<1) return 0; 
    //TRACE("CLUIFramesResizeFrames\n");

  newheight=newsize.bottom-newsize.top;



  // search for alClient frame and get the titlebar's height
  tbh=0;
  clientfrm=CLUIFramesGetalClientFrame();
  if(clientfrm!=-1)
    tbh=(TitleBarH+GapBetweenTitlebar)*btoint(Frames[clientfrm].TitleBar.ShowTitleBar);

  for(i=0;i<nFramescount;i++)
  {
    if (!Frames[i].floating)
    {
      Frames[i].needhide=FALSE;
      Frames[i].wndSize.left=newsize.left;
      Frames[i].wndSize.right=newsize.right;

    };
  };
  {
    //sorting stuff
    sdarray=(SortData*)malloc(sizeof(SortData)*nFramescount);
    if (sdarray==NULL){return(-1);};
    for(i=0;i<nFramescount;i++)
    {sdarray[i].order=Frames[i].order;
    sdarray[i].realpos=i;
    };
    qsort(sdarray,nFramescount,sizeof(SortData),sortfunc);

  }

  drawitems=nFramescount;

  while(sumheight>(newheight-tbh)&&drawitems>0) {
    sumheight=0;
    drawitems=0;
    for(i=0;i<nFramescount;i++){
      if(((Frames[i].align!=alClient))&&(!Frames[i].floating)&&(Frames[i].visible)&&(!Frames[i].needhide)) {
        drawitems++;
        curfrmtbh=(TitleBarH+GapBetweenTitlebar)*btoint(Frames[i].TitleBar.ShowTitleBar);
        sumheight+=(Frames[i].height)+curfrmtbh+(i > 0 ? sepw : 0)+(Frames[i].UseBorder?2:0);
        if(sumheight>newheight-tbh) {
          sumheight-=(Frames[i].height)+curfrmtbh + (i > 0 ? sepw : 0);
          Frames[i].needhide=TRUE;
          drawitems--;
          break;
        }
      }
    }
  }

  prevframe=-1;
  prevframebottomline=topBorder;
  for(j=0;j<nFramescount;j++) {
    //move all alTop frames
    i=sdarray[j].realpos;
    if((!Frames[i].needhide)&&(!Frames[i].floating)&&(Frames[i].visible)&&(Frames[i].align==alTop)) {
      curfrmtbh=(TitleBarH+GapBetweenTitlebar)*btoint(Frames[i].TitleBar.ShowTitleBar);
      Frames[i].wndSize.top=prevframebottomline+(i > 0 ? sepw : 0)+(curfrmtbh);
      Frames[i].wndSize.bottom=Frames[i].height+Frames[i].wndSize.top+(Frames[i].UseBorder?2:0);
      Frames[i].prevvisframe=prevframe;
      prevframe=i;
      prevframebottomline=Frames[i].wndSize.bottom;
      if(prevframebottomline>newheight){
        //prevframebottomline-=Frames[i].height+(curfrmtbh+1);
        //Frames[i].needhide=TRUE;
      }
    }
  }

  if(sumheight<newheight) {
    for(j=0;j<nFramescount;j++){
      //move alClient frame
      i=sdarray[j].realpos;
      if((!Frames[i].needhide)&&(!Frames[i].floating)&&(Frames[i].visible)&&(Frames[i].align==alClient)) {
        int oldh;
        Frames[i].wndSize.top=prevframebottomline+(j > 0 ? sepw : 0)+(tbh);
        Frames[i].wndSize.bottom=Frames[i].wndSize.top+newheight-sumheight-tbh-(j > 0 ? sepw : 0);

        oldh=Frames[i].height;
        Frames[i].height=Frames[i].wndSize.bottom-Frames[i].wndSize.top;
        Frames[i].prevvisframe=prevframe;
        prevframe=i;
        prevframebottomline=Frames[i].wndSize.bottom;
        if(prevframebottomline>newheight) {
          //prevframebottomline-=Frames[i].height+(tbh+1);
          //Frames[i].needhide=TRUE;
        }
        break;
      }
    }
  }

  //newheight
  prevframebottomline=newheight+sepw+topBorder;
  //prevframe=-1;
  for(j=nFramescount-1;j>=0;j--) {
    //move all alBottom frames
    i=sdarray[j].realpos;
    if((Frames[i].visible)&&(!Frames[i].floating)&&(!Frames[i].needhide)&&(Frames[i].align==alBottom)) {
      curfrmtbh=(TitleBarH+GapBetweenTitlebar)*btoint(Frames[i].TitleBar.ShowTitleBar);

      Frames[i].wndSize.bottom=prevframebottomline-(j > 0 ? sepw : 0);
      Frames[i].wndSize.top=Frames[i].wndSize.bottom-Frames[i].height-(Frames[i].UseBorder?2:0);
      Frames[i].prevvisframe=prevframe;
      prevframe=i;
      prevframebottomline=Frames[i].wndSize.top/*-1*/-curfrmtbh;
      if(prevframebottomline>newheight) {

      }
    }
  }
  for(i=0;i<nFramescount;i++)
    if(Frames[i].TitleBar.ShowTitleBar) 
        SetRect(&Frames[i].TitleBar.wndSize,Frames[i].wndSize.left,Frames[i].wndSize.top-TitleBarH-GapBetweenTitlebar,Frames[i].wndSize.right,Frames[i].wndSize.top-GapBetweenTitlebar);
  if (sdarray!=NULL){free(sdarray);sdarray=NULL;};
  return 0;
 }

 int SizeMoveNewSizes()
 {
  int i;
  for(i=0;i<nFramescount;i++)
  {

    if (Frames[i].floating){
      CLUIFrameResizeFloatingFrame(i);
    }else
    {
      CLUIFrameMoveResize(&Frames[i]);
    };
  }
  return 0;
}


int CLUIFramesApplyNewSizes(int mode)
{
  int i;
  TRACE("CLUIFramesApplyNewSizes\n");
  dock_prevent_moving=0;
  for(i=0;i<nFramescount;i++){
    if ( (mode==1 && Frames[i].OwnerWindow!=(HWND)-2 && Frames[i].OwnerWindow) ||
      (mode==2 && Frames[i].OwnerWindow==(HWND)-2) ||
      (mode==3) )
      if (Frames[i].floating){
        CLUIFrameResizeFloatingFrame(i);
      }else
      {
        CLUIFrameMoveResize(&Frames[i]);
      };
     //&& CLUIFramesForceUpdateFrame(&Frames[i]);
     //&& CLUIFramesForceUpdateTB(&Frames[i]);
  }
  if (IsWindowVisible(pcli->hwndContactList))
  {
    DrawNonFramedObjects(1,0);
    InvalidateFrameImage(0,0);
  }
  dock_prevent_moving=1;
  return 0;
}

int CLUIFramesUpdateFrame(WPARAM wParam,LPARAM lParam)
{
  if (FramesSysNotStarted) return -1;
  TRACE("CLUIFramesUpdateFrame\n");
  if(wParam==-1) { CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0); return 0;}
  if(lParam&FU_FMPOS)	CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,1);
  lockfrm();
  wParam=id2pos(wParam);
  if(wParam<0||(int)wParam>=nFramescount) { ulockfrm(); return -1;}
  if(lParam&FU_TBREDRAW)	CLUIFramesForceUpdateTB(&Frames[wParam]);
  if(lParam&FU_FMREDRAW)	CLUIFramesForceUpdateFrame(&Frames[wParam]);
  //if (){};
  ulockfrm();

  return 0;
}




int CLUIFramesOnClistResize2(WPARAM wParam,LPARAM lParam, int mode)
{
  RECT nRect;
  int tick;
  GapBetweenFrames=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);
  GapBetweenTitlebar=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",1);
  if (FramesSysNotStarted) return -1;
  TRACE("CLUIFramesOnClistResize2\n");
  lockfrm();
  //if (resizing){return(0);};
  //resizing=TRUE;
  GetClientRect(pcli->hwndContactList,&nRect);
  if (lParam && lParam!=1)
  {
    if (mode!=2)
    {
      RECT oldRect;
      POINT pt;
      RECT * newRect=(RECT *)lParam;
      int dl,dt,dr,db;
     GetWindowRect((HWND)wParam,&oldRect);
      pt.x=nRect.left;
      pt.y=nRect.top;
      ClientToScreen(pcli->hwndContactList,&pt);
      dl=pt.x-oldRect.left;
      dt=pt.y-oldRect.top;
      dr=(oldRect.right-oldRect.left)-(nRect.right-nRect.left)-dl;
      db=(oldRect.bottom-oldRect.top)-(nRect.bottom-nRect.top)-dt;
      nRect.left=newRect->left+dl;
      nRect.top=newRect->top+dt;
      nRect.bottom=newRect->bottom-db;
      nRect.right=newRect->right-dr;
    }
    else if (0)
    {
      nRect=*((RECT*)lParam);
      OffsetRect(&nRect,-nRect.left,-nRect.top);
    }
  }


  //if(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1))GetWindowRect(pcli->hwndStatus,&rcStatus);
  //else rcStatus.top=rcStatus.bottom=0;
  // nRect.top--;
  /* $$$	rcStatus.top=rcStatus.bottom=0;


  nRect.bottom-=nRect.top;
  nRect.bottom-=(rcStatus.bottom-rcStatus.top);
  nRect.right-=nRect.left;
  nRect.left=0;
  nRect.top=0;
  ContactListHeight=nRect.bottom; $$$*/

  nRect.left+=DBGetContactSettingByte(NULL,"CLUI","LeftClientMargin",0); //$$ Left BORDER SIZE
  nRect.right-=DBGetContactSettingByte(NULL,"CLUI","RightClientMargin",0); //$$ Left BORDER SIZE
  nRect.top+=DBGetContactSettingByte(NULL,"CLUI","TopClientMargin",0); //$$ TOP border
  nRect.bottom-=DBGetContactSettingByte(NULL,"CLUI","BottomClientMargin",0); //$$ BOTTOM border     ContactListHeight=nRect.bottom-nRect.top; //$$
  //	dock_prevent_moving=0;
  tick=GetTickCount();
  CLUIFramesResize(nRect);
  if (mode==0) CLUIFramesApplyNewSizes(3);
  //   dock_prevent_moving=1;
  //resizing=FALSE;	
  ulockfrm();
  tick=GetTickCount()-tick;

  //	if (pcli->hwndContactList!=0) skinInvalidateRect(pcli->hwndContactList,NULL,TRUE);
  //	if (pcli->hwndContactList!=0) UpdateWindow(pcli->hwndContactList);
  //    for(i=0;i<nFramescount;i++){

  //        if (Frames[i].floating){
  //           CLUIFrameResizeFloatingFrame(i);
  //       }else if (mode!=2)
  //       {
  //           CLUIFrameMoveResize(&Frames[i]);
  //       };
  //
  //   }
  //if(lParam==2) RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_UPDATENOW|RDW_ALLCHILDREN|RDW_ERASE|RDW_INVALIDATE);


  Sleep(0);

  //dont save to database too many times
  if(GetTickCount()-LastStoreTick>1000){ CLUIFramesStoreAllFrames();LastStoreTick=GetTickCount();};

  return 0;
}

int SizeFramesByWindowRect(RECT *r, HDWP * PosBatch, int mode)
{
   RECT nRect;
   if (FramesSysNotStarted) return -1;
   lockfrm();
   GapBetweenTitlebar=(int)DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",1);
   GapBetweenFrames=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);
   nRect.left=0;
   nRect.top=0;
   nRect.right=r->right-r->left;
   nRect.bottom=r->bottom-r->top;
   nRect.left+=DBGetContactSettingByte(NULL,"CLUI","LeftClientMargin",0); //$$ Left BORDER SIZE
   nRect.right-=DBGetContactSettingByte(NULL,"CLUI","RightClientMargin",0); //$$ Left BORDER SIZE
   nRect.top+=DBGetContactSettingByte(NULL,"CLUI","TopClientMargin",0); //$$ TOP border
   nRect.bottom-=DBGetContactSettingByte(NULL,"CLUI","BottomClientMargin",0); //$$ BOTTOM border     ContactListHeight=nRect.bottom-nRect.top; //$$
   CLUIFramesResizeFrames(nRect);
   {
     int i;
     for(i=0;i<nFramescount;i++)
     {
       int dx;
       int dy;
       dx=0;//new_window_rect.left-old_window_rect.left;
       dy=0;//_window_rect.top-old_window_rect.top;
       if (!Frames[i].floating)
       {
         if (Frames[i].visible && !Frames[i].needhide && !IsWindowVisible(Frames[i].hWnd))
         {
           ShowWindow(Frames[i].hWnd,SW_SHOW);
           if (Frames[i].TitleBar.ShowTitleBar) ShowWindow(Frames[i].TitleBar.hwnd,SW_SHOW);
         }
         if (Frames[i].OwnerWindow && (int)(Frames[i].OwnerWindow)!=-2 )
         {
           if (!(mode&2))
           {
            HWND hwnd;
            hwnd=GetParent(Frames[i].OwnerWindow);
            *PosBatch=DeferWindowPos(*PosBatch,Frames[i].OwnerWindow,NULL,Frames[i].wndSize.left+r->left,Frames[i].wndSize.top+r->top,
                Frames[i].wndSize.right-Frames[i].wndSize.left, Frames[i].wndSize.bottom-Frames[i].wndSize.top,SWP_NOZORDER|SWP_NOACTIVATE); 
            SetWindowPos(Frames[i].hWnd,NULL,0,0,
                Frames[i].wndSize.right-Frames[i].wndSize.left, Frames[i].wndSize.bottom-Frames[i].wndSize.top,SWP_NOZORDER|SWP_NOACTIVATE/*|SWP_NOSENDCHANGING*/); 
           }
           //Frame
           if(Frames[i].TitleBar.ShowTitleBar) 
           {
           SetWindowPos(Frames[i].TitleBar.hwnd,NULL,Frames[i].wndSize.left+dx,Frames[i].wndSize.top-TitleBarH-GapBetweenTitlebar+dy,
              Frames[i].wndSize.right-Frames[i].wndSize.left,TitleBarH,SWP_NOZORDER|SWP_NOACTIVATE	);
           SetRect(&Frames[i].TitleBar.wndSize,Frames[i].wndSize.left,Frames[i].wndSize.top-TitleBarH-GapBetweenTitlebar,Frames[i].wndSize.right,Frames[i].wndSize.top-GapBetweenTitlebar);
           UpdateWindow(Frames[i].TitleBar.hwnd);
           }
         }
         else 
         {
           if(1)
           {
           int res=0;
           // set frame position
           res=SetWindowPos(Frames[i].hWnd,NULL,Frames[i].wndSize.left+dx,Frames[i].wndSize.top+dy,
             Frames[i].wndSize.right-Frames[i].wndSize.left,
             Frames[i].wndSize.bottom-Frames[i].wndSize.top,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSENDCHANGING);
           }
           if(1)
           {
           // set titlebar position
           if(Frames[i].TitleBar.ShowTitleBar) 
           {
             SetWindowPos(Frames[i].TitleBar.hwnd,NULL,Frames[i].wndSize.left+dx,Frames[i].wndSize.top-TitleBarH-GapBetweenTitlebar+dy,
               Frames[i].wndSize.right-Frames[i].wndSize.left,TitleBarH,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSENDCHANGING	);
             SetRect(&Frames[i].TitleBar.wndSize,Frames[i].wndSize.left,Frames[i].wndSize.top-TitleBarH-GapBetweenTitlebar,Frames[i].wndSize.right,Frames[i].wndSize.top-GapBetweenTitlebar);
             
           }
           }
           UpdateWindow(Frames[i].hWnd);
           if(Frames[i].TitleBar.ShowTitleBar) UpdateWindow(Frames[i].TitleBar.hwnd);         
       };
     }

   }
   //CLUIFramesApplyNewSizes(3);
   ulockfrm();
   if(GetTickCount()-LastStoreTick>1000)
   { 
     CLUIFramesStoreAllFrames();
     LastStoreTick=GetTickCount();
   };
   }
   return 0;
}

int CheckFramesPos(RECT *wr)
{
  int i;
  if (FramesSysNotStarted) return -1;
  GapBetweenTitlebar=(int)DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",1);
  GapBetweenFrames=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);
  lockfrm();
  for(i=0;i<nFramescount;i++)
  {
    int dx;
    int dy;
    dx=0;//new_window_rect.left-old_window_rect.left;
    dy=0;//_window_rect.top-old_window_rect.top;
    if (!Frames[i].floating&&Frames[i].visible)
    {
      if (!(Frames[i].OwnerWindow && (int)(Frames[i].OwnerWindow)!=-2))
      {
        RECT r;
        BOOL NeedRepsition=0;
        GetWindowRect(Frames[i].hWnd,&r);
        if (r.top-wr->top!=Frames[i].wndSize.top ||r.left-wr->left!=Frames[i].wndSize.left)
          SetWindowPos(Frames[i].hWnd,NULL,Frames[i].wndSize.left, Frames[i].wndSize.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE); 
      }
      if (Frames[i].TitleBar.ShowTitleBar)  
      {
        RECT r;
        BOOL NeedRepsition=0;
        GetWindowRect(Frames[i].TitleBar.hwnd,&r);
        if (r.top-wr->top!=Frames[i].wndSize.top-TitleBarH-GapBetweenTitlebar || r.left-wr->left!=Frames[i].wndSize.left)
        {
          SetWindowPos(Frames[i].TitleBar.hwnd,NULL,Frames[i].wndSize.left+dx,Frames[i].wndSize.top-TitleBarH-GapBetweenTitlebar+dy,
          Frames[i].wndSize.right-Frames[i].wndSize.left,TitleBarH,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE); 
          SetRect(&Frames[i].TitleBar.wndSize,Frames[i].wndSize.left,Frames[i].wndSize.top-TitleBarH-GapBetweenTitlebar,Frames[i].wndSize.right,Frames[i].wndSize.top-GapBetweenTitlebar);
        }
      }
    }
  }
  ulockfrm();
  return 0;
}

int CLUIFramesOnClistResize(WPARAM wParam,LPARAM lParam)
{
  RECT nRect;
  int tick;
  GapBetweenFrames=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);
  GapBetweenTitlebar=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",1);

  if (FramesSysNotStarted) return -1;
  TRACE("CLUIFramesOnClistResize\n");
  lockfrm();
  //if (resizing){return(0);};
  //resizing=TRUE;
  GetClientRect(pcli->hwndContactList,&nRect);
  //$$$ Fixed borders 
  if (lParam && lParam!=1 && lParam!=2)
  {
    RECT oldRect;
    POINT pt;
    RECT * newRect=(RECT *)lParam;
    int dl,dt,dr,db;
   GetWindowRect((HWND)wParam,&oldRect);
    pt.x=nRect.left;
    pt.y=nRect.top;
    ClientToScreen(pcli->hwndContactList,&pt);
    dl=pt.x-oldRect.left;
    dt=pt.y-oldRect.top;
    dr=(oldRect.right-oldRect.left)-(nRect.right-nRect.left)-dl;
    db=(oldRect.bottom-oldRect.top)-(nRect.bottom-nRect.top)-dt;
    nRect.left=newRect->left+dl;
    nRect.top=newRect->top+dt;
    nRect.bottom=newRect->bottom-db;
    nRect.right=newRect->right-dr;
  }


  //if(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1))GetWindowRect(pcli->hwndStatus,&rcStatus);
  //else rcStatus.top=rcStatus.bottom=0;
  // nRect.top--;
  /* $$$	rcStatus.top=rcStatus.bottom=0;


  nRect.bottom-=nRect.top;
  nRect.bottom-=(rcStatus.bottom-rcStatus.top);
  nRect.right-=nRect.left;
  nRect.left=0;
  nRect.top=0;
  ContactListHeight=nRect.bottom; $$$*/

  nRect.left+=DBGetContactSettingByte(NULL,"CLUI","LeftClientMargin",0); //$$ Left BORDER SIZE
  nRect.right-=DBGetContactSettingByte(NULL,"CLUI","RightClientMargin",0); //$$ Left BORDER SIZE
  nRect.top+=DBGetContactSettingByte(NULL,"CLUI","TopClientMargin",0); //$$ TOP border
  nRect.bottom-=DBGetContactSettingByte(NULL,"CLUI","BottomClientMargin",0); //$$ BOTTOM border

  ContactListHeight=nRect.bottom-nRect.top; //$$

  tick=GetTickCount();


  CLUIFramesResize(nRect);

  CLUIFramesApplyNewSizes(2);
  CLUIFramesApplyNewSizes(1);

  //resizing=FALSE;	
  ulockfrm();
  tick=GetTickCount()-tick;

  if (pcli->hwndContactList!=0) skinInvalidateRect(pcli->hwndContactList,NULL,TRUE);
  if (pcli->hwndContactList!=0) UpdateWindow(pcli->hwndContactList);

  if(lParam==2) RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_UPDATENOW|RDW_ALLCHILDREN|RDW_ERASE|RDW_INVALIDATE);


  Sleep(0);

  //dont save to database too many times
  if(GetTickCount()-LastStoreTick>1000){ CLUIFramesStoreAllFrames();LastStoreTick=GetTickCount();};

  return 0;
}

static	HBITMAP hBmpBackground;
static int backgroundBmpUse;
static COLORREF bkColour;
static COLORREF SelBkColour;
boolean AlignCOLLIconToLeft; //will hide frame icon

int OnFrameTitleBarBackgroundChange(WPARAM wParam,LPARAM lParam)
{
  {	
    DBVARIANT dbv={0};

    AlignCOLLIconToLeft=DBGetContactSettingByte(NULL,"FrameTitleBar","AlignCOLLIconToLeft",0);

    bkColour=DBGetContactSettingDword(NULL,"FrameTitleBar","BkColour",CLCDEFAULT_BKCOLOUR);
    SelBkColour=DBGetContactSettingDword(NULL,"FrameTitleBar","TextColour",CLCDEFAULT_TEXTCOLOUR);
    if(hBmpBackground) {DeleteObject(hBmpBackground); hBmpBackground=NULL;}
    if(DBGetContactSettingByte(NULL,"FrameTitleBar","UseBitmap",CLCDEFAULT_USEBITMAP)) {
      if(!DBGetContactSetting(NULL,"FrameTitleBar","BkBitmap",&dbv)) {
        hBmpBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
        mir_free(dbv.pszVal);
        DBFreeVariant(&dbv);
      }
    }
    backgroundBmpUse=DBGetContactSettingWord(NULL,"FrameTitleBar","BkBmpUse",CLCDEFAULT_BKBMPUSE);
  };

  //		RecreateStatusBar(CallService(MS_CLUI_GETHWND,0,0));
  //		if (pcli->hwndStatus) skinInvalidateRect(pcli->hwndStatus,NULL,TRUE);
  CLUIFramesOnClistResize(0,0);
  return 0;
}

/*void DrawBackGroundTTB(HWND hwnd,HDC mhdc)
{
HDC hdcMem,hdc;
RECT clRect,*rcPaint;

int yScroll=0;
int y;
PAINTSTRUCT paintst={0};	
HBITMAP hBmpOsb,hOldBmp;
DWORD style=GetWindowLong(hwnd,GWL_STYLE);
int grey=0;
HFONT oFont;
HBRUSH hBrushAlternateGrey=NULL;

HFONT hFont;

//skinInvalidateRect(hwnd,0,FALSE);

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

//oFont=SelectObject(hdc,hFont);
SetBkMode(hdc,TRANSPARENT);
paintst.fErase=FALSE;
//DeleteObject(hFont);
if (!mhdc)
{		
EndPaint(hwnd,&paintst);	
}
}
}

*/
int DrawTitleBar(HDC hdcMem2,RECT rect,int Frameid)
{
  int pos;
  HDC hdcMem;
  HFONT hoTTBFont;
  RECT rc=rect;
  HBRUSH hBack,hoBrush; 
  HBITMAP b1,b2;
  hdcMem=CreateCompatibleDC(hdcMem2);


  hoTTBFont=SelectObject(hdcMem,TitleBarFont);
  hBack=GetSysColorBrush(COLOR_3DFACE);
  hoBrush=SelectObject(hdcMem,hBack);  
  lockfrm();
  pos=id2pos(Frameid);
  
  if (pos>=0&&pos<nFramescount)
  {
	  GetClientRect(Frames[pos].TitleBar.hwnd,&rc);
      if (Frames[pos].floating)
      {
        
        rc.bottom=rc.top+TitleBarH;
        Frames[pos].TitleBar.wndSize=rc;
      }
      else
      {
        Frames[pos].TitleBar.wndSize=rc;
      }
	  b1=CreateBitmap32(rc.right-rc.left,rc.bottom-rc.top);
	  b2=SelectObject(hdcMem,b1);
      if (Frames[pos].floating)
      {
        FillRect(hdcMem,&rc,hBack);
        //SelectObject(hdcMem,hoBrush);       
        SkinDrawGlyph(hdcMem,&rc,&rc,"Main,ID=FrameCaption");              
      }
      else
      {
		if (!LayeredFlag)
		{
			 BltBackImage(Frames[pos].TitleBar.hwnd,hdcMem,&rc);
		}
		else  BitBlt(hdcMem,0,0,rc.right-rc.left,rc.bottom-rc.top,hdcMem2,rect.left,rect.top,SRCCOPY);
        SkinDrawGlyph(hdcMem,&rc,&rc,"Main,ID=FrameCaption");
      }
      if (SelBkColour) 
		  SetTextColor(hdcMem,SelBkColour);
      if (!AlignCOLLIconToLeft)
      {

        if(Frames[pos].TitleBar.hicon!=NULL)	{
          DrawIconEx_Fix(hdcMem,rc.left +2,rc.top+((TitleBarH>>1)-(GetSystemMetrics(SM_CYSMICON)>>1)),Frames[pos].TitleBar.hicon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
          TextOutSA(hdcMem,rc.left+GetSystemMetrics(SM_CXSMICON)+4,rc.top+2,Frames[pos].TitleBar.tbname,MyStrLen(Frames[pos].TitleBar.tbname));
        }
        else
          TextOutSA(hdcMem,rc.left+2,rc.top+2,Frames[pos].TitleBar.tbname,MyStrLen(Frames[pos].TitleBar.tbname));

      }else
      //if (!Frames[pos].floating){
        TextOutSA(hdcMem,rc.left+GetSystemMetrics(SM_CXSMICON)+2,rc.top+2,Frames[pos].TitleBar.tbname,MyStrLen(Frames[pos].TitleBar.tbname));
      //}
	  //else TextOut(hdcMem,rc.left+GetSystemMetrics(SM_CXSMICON)+2,rc.top+2,Frames[pos].TitleBar.tbname,MyStrLen(Frames[pos].TitleBar.tbname));


      if (!AlignCOLLIconToLeft)
      {						
        DrawIconEx_Fix(hdcMem,Frames[pos].TitleBar.wndSize.right-GetSystemMetrics(SM_CXSMICON)-2,rc.top+((TitleBarH>>1)-(GetSystemMetrics(SM_CXSMICON)>>1)),Frames[pos].collapsed?LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN):LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT),GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
      }
      else
      {
        DrawIconEx_Fix(hdcMem,rc.left,rc.top+((TitleBarH>>1)-(GetSystemMetrics(SM_CXSMICON)>>1)),Frames[pos].collapsed?LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN):LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT),GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
      }


  }
  {
	  BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	  if (Frames[pos].floating || (!LayeredFlag))
	  {
		  HRGN rgn=CreateRectRgn(rect.left,rect.top,rect.right,rect.bottom);
		  SelectClipRgn(hdcMem2,rgn);
		  BitBlt(hdcMem2,rect.left,rect.top,rc.right-rc.left,rc.bottom-rc.top,hdcMem,0,0,SRCCOPY);
		  DeleteObject(rgn);
	  }
	  else
		  BitBlt(hdcMem2,rect.left,rect.top,rc.right-rc.left,rc.bottom-rc.top,hdcMem,0,0,SRCCOPY);
		  //MyAlphaBlend(hdcMem2,rect.left,rect.top,rc.right-rc.left,rc.bottom-rc.top,hdcMem,0,0,rc.right-rc.left,rc.bottom-rc.top,bf);
  }
  ulockfrm();
  SelectObject(hdcMem,b2);
  DeleteObject(b1);
  SelectObject(hdcMem,hoTTBFont);
  SelectObject(hdcMem,hoBrush);
  DeleteDC(hdcMem);
  return 0;
}
//int OldDrawTitleBar(HDC dc,RECT rect,int Frameid)
//{
//  HDC hdcMem;
//  HBITMAP hBmpOsb,hoBmp;
//  HBRUSH hBack,hoBrush;
//  HDC paintDC;
//  int pos;
//   //GetClientRect(hwnd,&rect);	
//  paintDC=dc;
//
//  //					paintDC = BeginPaint(hwnd, &paintStruct);
//  //rect=paintStruct.rcPaint;
//
//
//  hdcMem=CreateCompatibleDC(paintDC);// paintDC;//
//  hBmpOsb=CreateBitmap32(rect.right,rect.bottom);
//  hoBmp=SelectObject(hdcMem,hBmpOsb);
//
//  hoTTBFont=SelectObject(hdcMem,TitleBarFont);
//  SetBkMode(hdcMem,TRANSPARENT);
//
//  hBack=GetSysColorBrush(COLOR_3DFACE);
//  hoBrush=SelectObject(hdcMem,hBack);
//
//
//
//
//  lockfrm();
//  pos=id2pos(Frameid);
//  
//
//
//  if (pos>=0&&pos<nFramescount)
//  {
//    RECT rc;
//      //GetClientRect(Frames[pos].TitleBar.hwnd,&Frames[pos].TitleBar.wndSize);
//      //GetClientRect(Frames[pos].TitleBar.hwnd,&rc);
//       rc=rect;
//      Frames[pos].TitleBar.wndSize=rect;
//
//    {   
//      //set font charset
//      HFONT oFont;
//      HFONT hf=GetStockObject(DEFAULT_GUI_FONT);
//      oFont=SelectObject(hdcMem,hf);
//      if (Frames[pos].floating)
//      {
//        HBRUSH hBack, hoBrush;
//        hBack=GetSysColorBrush(COLOR_3DFACE);
//        hoBrush=SelectObject(hdcMem,hBack);
//        FillRect(hdcMem,&rect,hBack);
//        SelectObject(hdcMem,hoBrush);       
//        SkinDrawGlyph(hdcMem,&rc,&rc,"Main,ID=FrameCaption");       
//        FillRect255Alpha(hdcMem,&rc);
//        ///SkinDrawWindowBack(Frames[pos].TitleBar.hwnd,hdcMem,&rc,"Main Window/Backgrnd");
//      }
//      else
//        SkinDrawGlyph(hdcMem,&rc,&rc,"Main,ID=FrameCaption");
//      //DrawBackGroundTTB(Frames[pos].TitleBar.hwnd,hdcMem);
//      //set font charset
//      //			SelectObject(hdcMem,hf);
//
//      if (SelBkColour) SetTextColor(hdcMem,SelBkColour);
//
//
//      if (!AlignCOLLIconToLeft)
//      {
//
//        if(Frames[pos].TitleBar.hicon!=NULL)	{
//          //(TitleBarH>>1)-(GetSystemMetrics(SM_CXSMICON)>>1)
//          DrawIconEx_Fix(hdcMem,2,((TitleBarH>>1)-(GetSystemMetrics(SM_CYSMICON)>>1)),Frames[pos].TitleBar.hicon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
//          TextOutS(hdcMem,GetSystemMetrics(SM_CYSMICON)+4,2,Frames[pos].TitleBar.tbname,MyStrLen(Frames[pos].TitleBar.tbname));
//        }
//        else
//          TextOutS(hdcMem,2,2,Frames[pos].TitleBar.tbname,MyStrLen(Frames[pos].TitleBar.tbname));
//
//      }else
//      {
//        TextOutS(hdcMem,GetSystemMetrics(SM_CXSMICON)+2,2,Frames[pos].TitleBar.tbname,MyStrLen(Frames[pos].TitleBar.tbname));
//      }
//
//
//      if (!AlignCOLLIconToLeft)
//      {						
//        DrawIconEx_Fix(hdcMem,Frames[pos].TitleBar.wndSize.right-GetSystemMetrics(SM_CXSMICON)-2,((TitleBarH>>1)-(GetSystemMetrics(SM_CXSMICON)>>1)),Frames[pos].collapsed?LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN):LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT),GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
//      }
//      else
//      {
//        DrawIconEx_Fix(hdcMem,0,((TitleBarH>>1)-(GetSystemMetrics(SM_CXSMICON)>>1)),Frames[pos].collapsed?LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN):LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT),GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
//      }
//
//      DeleteObject(hf);
//      SelectObject(hdcMem,oFont);
//      //DeleteObject(hFront);
//      {
//
//        BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
//        MyAlphaBlend(paintDC,rect.left,rect.top+rc.top,rc.right-rc.left,rc.bottom-rc.top,hdcMem,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,bf);
//      }
//    }
//  };
//  ulockfrm();
//
//
//  SelectObject(hdcMem,hoBmp);
//  SelectObject(hdcMem,hoBrush);
//  SelectObject(hdcMem,hoTTBFont);
//  DeleteDC(hdcMem);
//  DeleteObject(hBack);
//  DeleteObject(hBmpOsb);
//  //EndPaint(hwnd, &paintStruct);
//  return 0;
//
//};
//
//
//for old multiwindow
#define MPCF_CONTEXTFRAMEMENU		3
POINT ptOld;
short	nLeft			= 0;
short	nTop			= 0;

LRESULT CALLBACK CLUIFrameTitleBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  RECT rect;
  int Frameid,Framemod,direction;
  int xpos,ypos;

  Frameid=(GetWindowLong(hwnd,GWL_USERDATA));
  memset(&rect,0,sizeof(rect));


  switch(msg)
  {
  case WM_CREATE:
    SendMessage(hwnd,WM_SETFONT,(WPARAM)TitleBarFont,0);
    return FALSE;
  case WM_MEASUREITEM:
    return CallService(MS_CLIST_MENUMEASUREITEM,wParam,lParam);	
  case WM_DRAWITEM:
    return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);
  case WM_USER+100:
    return 1;    
  case WM_ENABLE:
    if (hwnd!=0) skinInvalidateRect(hwnd,NULL,FALSE);
    return 0;
    /*
    case WM_PRINT:
    case WM_PRINTCLIENT:
    skinInvalidateRect(hwnd,NULL,FALSE);
    {
    RECT rect;
    HDC dc;
    GetClientRect(hwnd,&rect);
    //DrawTitleBar(wParam,rect,Frameid);
    SendMessage(hwnd,WM_PAINT,0,0);
    SendMessage(hwnd,WM_NCPAINT,1,0);
    dc=GetDC(hwnd);
    SendMessage(hwnd,WM_ERASEBKGND,dc,0);
    ReleaseDC(hwnd,dc);
    SendMessage(hwnd,WM_PAINT,0,0);
    //				UpdateWindow(hwnd);
    return(0);
    }
    */
    /*
    case WM_NCPAINT:
    {

    //	if(wParam==1) break;
    {	POINT ptTopLeft={0,0};
    HRGN hClientRgn;
    ClientToScreen(hwnd,&ptTopLeft);
    hClientRgn=CreateRectRgn(0,0,1,1);
    CombineRgn(hClientRgn,(HRGN)wParam,NULL,RGN_COPY);
    OffsetRgn(hClientRgn,-ptTopLeft.x,-ptTopLeft.y);
    InvalidateRgn(hwnd,hClientRgn,FALSE);
    DeleteObject(hClientRgn);
    UpdateWindow(hwnd);
    }
    //return(0);
    };
    */


  case WM_COMMAND:


    if (ServiceExists(MO_CREATENEWMENUOBJECT))
    {
      //if ( CallService(MS_CLIST_MENUPROCESSCOMMAND,MAKEWPARAM(LOWORD(wParam),0),(LPARAM)Frameid) ){break;};
      if (ProcessCommandProxy(MAKEWPARAM(LOWORD(wParam),0),(LPARAM)Frameid) ) break;
    }else
    {
      if ( CallService(MS_CLIST_MENUPROCESSCOMMAND,MAKEWPARAM(LOWORD(wParam),MPCF_CONTEXTFRAMEMENU),(LPARAM)Frameid) ){break;};

    };


    if(HIWORD(wParam)==0) {//mouse events for self created menu
      int framepos=id2pos(Frameid);
      if (framepos==-1){break;};

      switch(LOWORD(wParam))
      {
      case frame_menu_lock:
        Frames[framepos].Locked=!Frames[framepos].Locked;
        break;
      case frame_menu_visible:
        Frames[framepos].visible=!Frames[framepos].visible;
        break;
      case frame_menu_showtitlebar:
        Frames[framepos].TitleBar.ShowTitleBar=!Frames[framepos].TitleBar.ShowTitleBar;
        break;
      case frame_menu_floating:
        CLUIFrameSetFloat(Frameid,0);
        break;
      }
      CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
    }

    break;
  case WM_RBUTTONDOWN:
    {
      HMENU hmenu;
      POINT pt;
      GetCursorPos(&pt);

      if (ServiceExists(MS_CLIST_MENUBUILDFRAMECONTEXT))
      {
        hmenu=(HMENU)CallService(MS_CLIST_MENUBUILDFRAMECONTEXT,Frameid,0);
      }
      else
      {//legacy menu support
        int framepos=id2pos(Frameid);
        lockfrm();
        if (framepos==-1){ulockfrm();break;};
        hmenu=CreatePopupMenu();
        //				Frames[Frameid].TitleBar.hmenu=hmenu;
        AppendMenuA(hmenu,MF_STRING|MF_DISABLED|MF_GRAYED,15,Frames[framepos].name);
        AppendMenuA(hmenu,MF_SEPARATOR,16,"");

        if (Frames[framepos].Locked)
        {AppendMenuA(hmenu,MF_STRING|MF_CHECKED,frame_menu_lock,Translate("Lock Frame"));}
        else{AppendMenuA(hmenu,MF_STRING,frame_menu_lock,Translate("Lock Frame"));};

        if (Frames[framepos].visible)
        {AppendMenuA(hmenu,MF_STRING|MF_CHECKED,frame_menu_visible,Translate("Visible"));}
        else{AppendMenuA(hmenu,MF_STRING,frame_menu_visible,Translate("Visible") );};

        if (Frames[framepos].TitleBar.ShowTitleBar)
        {AppendMenuA(hmenu,MF_STRING|MF_CHECKED,frame_menu_showtitlebar,Translate("Show TitleBar") );}
        else{AppendMenuA(hmenu,MF_STRING,frame_menu_showtitlebar,Translate("Show TitleBar") );};

        if (Frames[framepos].floating)
        {AppendMenuA(hmenu,MF_STRING|MF_CHECKED,frame_menu_floating,Translate("Floating") );}
        else{AppendMenuA(hmenu,MF_STRING,frame_menu_floating,Translate("Floating") );};

        //err=GetMenuItemCount(hmenu)
        ulockfrm();
      };

      TrackPopupMenu(hmenu,TPM_LEFTALIGN,pt.x,pt.y,0,hwnd,0);
      DestroyMenu(hmenu);
    }
    break;
  case WM_LBUTTONDBLCLK:
    {	
      Framemod=-1;
      lbypos=-1;oldframeheight=-1;ReleaseCapture();
      CallService(MS_CLIST_FRAMES_UCOLLFRAME,Frameid,0);
      lbypos=-1;oldframeheight=-1;ReleaseCapture();
    }
    break;

  case WM_LBUTTONUP:
    {
      if(GetCapture()!=hwnd){break;}; 
      curdragbar=-1;lbypos=-1;oldframeheight=-1;ReleaseCapture();
      break;
    };
  case WM_LBUTTONDOWN:
    {

      int framepos=id2pos(Frameid);
      lockfrm();
      if (framepos==-1){ulockfrm();break;};
      {
        if (Frames[framepos].floating)
        {

          POINT pt;
          GetCursorPos(&pt);
          Frames[framepos].TitleBar.oldpos=pt;
        };
      };

      //ScreenToClient(Frames[framepos].ContainerWnd,&Frames[framepos].TitleBar.oldpos);

      if ((!(wParam&MK_CONTROL))&&Frames[framepos].Locked&&(!(Frames[framepos].floating)))
      {
        if (DBGetContactSettingByte(NULL,"CLUI","ClientAreaDrag",0)) {
          POINT pt;
          int res;
          //pt=nm->pt;
          GetCursorPos(&pt);
          res=SendMessage(GetParent(hwnd), WM_SYSCOMMAND, SC_MOVE|HTCAPTION,MAKELPARAM(pt.x,pt.y));
          return res;
        }
      };

      if (Frames[framepos].floating)
      {
        RECT rc;
        GetCursorPos(&ptOld);
        //ClientToScreen(hwnd,&ptOld);
       GetWindowRect( hwnd, &rc );

        nLeft	= (short)rc.left;
        nTop	= (short)rc.top;
      };
      ulockfrm();
      SetCapture(hwnd);		


      break;
    };
  case WM_MOUSEMOVE:
    {
      POINT pt,pt2;
      RECT wndr;
      int pos;
      //tbinfo
      {
        char TBcapt[255];			

        lockfrm();
        pos=id2pos(Frameid);

        if (pos!=-1)
        {
          int oldflags;


          sprintf(TBcapt,"%s - h:%d, vis:%d, fl:%d, fl:(%d,%d,%d,%d),or: %d",
            Frames[pos].name,Frames[pos].height,Frames[pos].visible,Frames[pos].floating,
            Frames[pos].FloatingPos.x,Frames[pos].FloatingPos.y,
            Frames[pos].FloatingSize.x,Frames[pos].FloatingSize.y,
            Frames[pos].order			
            );

          oldflags=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,Frames[pos].id),(LPARAM)0);
          if (!(oldflags&F_SHOWTBTIP))
          {
            oldflags|=F_SHOWTBTIP;
            //CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,Frames[pos].id),(LPARAM)oldflags);
          };
          //CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_TBTIPNAME,Frames[pos].id),(LPARAM)TBcapt);
        };

        ulockfrm();

      }				
      //
      if ((wParam&MK_LBUTTON)/*&&(wParam&MK_CONTROL)*/)
      {
        RECT rcMiranda;
        RECT rcwnd,rcOverlap;
        POINT newpt,ofspt,curpt,newpos;
        //if(GetCapture()!=hwnd){break;}; 
        //curdragbar=-1;lbypos=-1;oldframeheight=-1;ReleaseCapture();
        lockfrm();
        pos=id2pos(Frameid);
        if (Frames[pos].floating)
        {

          GetCursorPos(&curpt);
          rcwnd.bottom=curpt.y+5;
          rcwnd.top=curpt.y;
          rcwnd.left=curpt.x;
          rcwnd.right=curpt.x+5;

         GetWindowRect(pcli->hwndContactList, &rcMiranda );
          //GetWindowRect( Frames[pos].ContainerWnd, &rcwnd );
          //IntersectRect( &rcOverlap, &rcwnd, &rcMiranda )
          if (IsWindowVisible(pcli->hwndContactList) &&IntersectRect( &rcOverlap, &rcwnd, &rcMiranda ))
          {
            int id=Frames[pos].id;


            ulockfrm();

            ofspt.x=0;ofspt.y=0;
            ClientToScreen(Frames[pos].TitleBar.hwnd,&ofspt);
            ofspt.x=curpt.x-ofspt.x;ofspt.y=curpt.y-ofspt.y;

            CLUIFrameSetFloat(id,0);
            newpt.x=0;newpt.y=0;
            ClientToScreen(Frames[pos].TitleBar.hwnd,&newpt);
            SetCursorPos(newpt.x+ofspt.x,newpt.y+ofspt.y);
            GetCursorPos(&curpt);							
            lockfrm();
            Frames[pos].TitleBar.oldpos=curpt;
            ulockfrm();
            return(0);
          };

        }
        else
        {
          int id=Frames[pos].id;

          GetCursorPos(&curpt);
          rcwnd.bottom=curpt.y+5;
          rcwnd.top=curpt.y;
          rcwnd.left=curpt.x;
          rcwnd.right=curpt.x+5;

         GetWindowRect(pcli->hwndContactList, &rcMiranda );
          //GetWindowRect( Frames[pos].ContainerWnd, &rcwnd );
          //IntersectRect( &rcOverlap, &rcwnd, &rcMiranda )


          if (!IntersectRect( &rcOverlap, &rcwnd, &rcMiranda ) )	
          {


            ulockfrm();
            GetCursorPos(&curpt);
           GetWindowRect( Frames[pos].hWnd, &rcwnd );
            rcwnd.left=rcwnd.right-rcwnd.left;
            rcwnd.top=rcwnd.bottom-rcwnd.top;
            newpos.x=curpt.x;newpos.y=curpt.y;
            if (curpt.x>=(rcMiranda.right-1)){newpos.x=curpt.x+5;};
            if (curpt.x<=(rcMiranda.left+1)){newpos.x=curpt.x-(rcwnd.left)-5;};

            if (curpt.y>=(rcMiranda.bottom-1)){newpos.y=curpt.y+5;};
            if (curpt.y<=(rcMiranda.top+1)){newpos.y=curpt.y-(rcwnd.top)-5;};


            ofspt.x=0;ofspt.y=0;
            //ClientToScreen(Frames[pos].TitleBar.hwnd,&ofspt);
           GetWindowRect(Frames[pos].TitleBar.hwnd,&rcwnd);
            ofspt.x=curpt.x-ofspt.x;ofspt.y=curpt.y-ofspt.y;

            Frames[pos].FloatingPos.x=newpos.x;
            Frames[pos].FloatingPos.y=newpos.y;
            CLUIFrameSetFloat(id,0);				
            //SetWindowPos(Frames[pos].ContainerWnd,0,newpos.x,newpos.y,0,0,SWP_NOSIZE);

            lockfrm();
            newpt.x=0;newpt.y=0;
            ClientToScreen(Frames[pos].TitleBar.hwnd,&newpt);

           GetWindowRect( Frames[pos].hWnd, &rcwnd );
            SetCursorPos(newpt.x+(rcwnd.right-rcwnd.left)/2,newpt.y+(rcwnd.bottom-rcwnd.top)/2);
            GetCursorPos(&curpt);

            Frames[pos].TitleBar.oldpos=curpt;
            ulockfrm();

            return(0);
          };

        };
        ulockfrm();						
        //return(0);
      };

      if(wParam&MK_LBUTTON) {
        int newh=-1,prevold;

        if(GetCapture()!=hwnd){break;}; 					

        lockfrm();
        pos=id2pos(Frameid);

        if (Frames[pos].floating)
        {
          GetCursorPos(&pt);
          if ((Frames[pos].TitleBar.oldpos.x!=pt.x)||(Frames[pos].TitleBar.oldpos.y!=pt.y))
          {

            pt2=pt;
            ScreenToClient(hwnd,&pt2);
           GetWindowRect(Frames[pos].ContainerWnd,&wndr);
            {
              int dX,dY;
              POINT ptNew;

              ptNew.x = pt.x;
              ptNew.y = pt.y;							
              //ClientToScreen( hwnd, &ptNew );				

              dX = ptNew.x - ptOld.x;
              dY = ptNew.y - ptOld.y;

              nLeft	+= (short)dX;
              nTop	+= (short)dY;

              if (!(wParam&MK_CONTROL))
              {
                PositionThumb( &Frames[pos], nLeft, nTop );
              }else
              {

                SetWindowPos(	Frames[pos].ContainerWnd, 
                  HWND_TOPMOST, 
                  nLeft, 
                  nTop, 
                  0, 
                  0,
                  SWP_NOSIZE |SWP_NOACTIVATE| SWP_NOZORDER );
              };

              ptOld = ptNew;



            }

            pt.x=nLeft;
            pt.y=nTop;
            Frames[pos].TitleBar.oldpos=pt;
          };
          ulockfrm();
          //break;
          return(0);
        };


        if(Frames[pos].prevvisframe!=-1) {
          GetCursorPos(&pt);

          if ((Frames[pos].TitleBar.oldpos.x==pt.x)&&(Frames[pos].TitleBar.oldpos.y==pt.y))
          {ulockfrm();break;};

          ypos=rect.top+pt.y;xpos=rect.left+pt.x;
          Framemod=-1;

          if(Frames[pos].align==alBottom)	{
            direction=-1;
            Framemod=pos;
          }
          else {
            direction=1;
            Framemod=Frames[pos].prevvisframe;
          }
          if(Frames[Framemod].Locked) {ulockfrm();break;};
          if(curdragbar!=-1&&curdragbar!=pos) {ulockfrm();break;};

          if(lbypos==-1) {
            curdragbar=pos;
            lbypos=ypos;
            oldframeheight=Frames[Framemod].height;
            SetCapture(hwnd);
            {ulockfrm();break;};
          }
          else
          {
            //	if(GetCapture()!=hwnd){ulockfrm();break;}; 
          };

          newh=oldframeheight+direction*(ypos-lbypos);
          if(newh>0)	{
            prevold=Frames[Framemod].height;
            Frames[Framemod].height=newh;
            if(!CLUIFramesFitInSize()) { Frames[Framemod].height=prevold; ulockfrm();return TRUE;}
            Frames[Framemod].height=newh;
            if(newh>3) Frames[Framemod].collapsed=TRUE;

          }
          Frames[pos].TitleBar.oldpos=pt;
        }
        ulockfrm();
        if (newh>0){CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);};
        break;
      }
      curdragbar=-1;lbypos=-1;oldframeheight=-1;ReleaseCapture();
    }
    break;
  case WM_PRINT:
  case WM_PRINTCLIENT:
    {
      //if (lParam&PRF_CLIENT)
      {
        GetClientRect(hwnd,&rect);
		if (!LayeredFlag)
		{
			BltBackImage(hwnd,(HDC)wParam,&rect);
		}
        DrawTitleBar((HDC)wParam,rect,Frameid);
      }
      break;
    }
  case WM_SIZE:
	  {
		  InvalidateRect(hwnd,NULL,FALSE);
		  return DefWindowProc(hwnd, msg, wParam, lParam);
	  }
  case WM_PAINT:	
    {
      HDC paintDC;
      if (Frames[id2pos(Frameid)].floating || (!LayeredFlag))
      {	   
        GetClientRect(hwnd,&rect);	
		paintDC = GetDC(hwnd);
		DrawTitleBar(paintDC,rect,Frameid);
		ReleaseDC(hwnd,paintDC);
		ValidateRect(hwnd,NULL);
        //paintDC = BeginPaint(hwnd, &paintStruct);		       
        //DrawTitleBar(paintDC,rect,Frameid);
        //EndPaint(hwnd, &paintStruct);					
      }
      return DefWindowProc(hwnd, msg, wParam, lParam); 
    }
  default:return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return TRUE;
}
int CLUIFrameResizeFloatingFrame(int framepos)
{

  int width,height;
  RECT rect;

  if (!Frames[framepos].floating){return(0);};
  if (Frames[framepos].ContainerWnd==0){return(0);};
  GetClientRect(Frames[framepos].ContainerWnd,&rect);

  width=rect.right-rect.left;
  height=rect.bottom-rect.top;

  Frames[framepos].visible?ShowWindowNew(Frames[framepos].ContainerWnd,SW_SHOW/*NOACTIVATE*/):ShowWindowNew(Frames[framepos].ContainerWnd,SW_HIDE);



  if (Frames[framepos].TitleBar.ShowTitleBar)
  {
    ShowWindowNew(Frames[framepos].TitleBar.hwnd,SW_SHOW/*NOACTIVATE*/);
    //if (Frames[framepos].Locked){return(0);};
    Frames[framepos].height=height-DEFAULT_TITLEBAR_HEIGHT;

    SetWindowPos(Frames[framepos].TitleBar.hwnd,HWND_TOP,0,0,width,DEFAULT_TITLEBAR_HEIGHT,SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_DRAWFRAME);
    SetWindowPos(Frames[framepos].hWnd,HWND_TOP,0,DEFAULT_TITLEBAR_HEIGHT,width,height-DEFAULT_TITLEBAR_HEIGHT,SWP_SHOWWINDOW);

  }
  else
  {
    //SetWindowPos(Frames[framepos].TitleBar.hwnd,HWND_TOP,0,0,width,DEFAULT_TITLEBAR_HEIGHT,SWP_SHOWWINDOW|SWP_NOMOVE);
    //if (Frames[framepos].Locked){return(0);};
    Frames[framepos].height=height;
    ShowWindowNew(Frames[framepos].TitleBar.hwnd,SW_HIDE);
    SetWindowPos(Frames[framepos].hWnd,HWND_TOP,0,0,width,height,SWP_SHOWWINDOW|SWP_NOACTIVATE);		

  };
  //			CLUIFramesForceUpdateFrame(&Frames[framepos]);
  if (Frames[framepos].ContainerWnd!=0) UpdateWindow(Frames[framepos].ContainerWnd);
  //GetClientRect(Frames[framepos].TitleBar.hwnd,&Frames[framepos].TitleBar.wndSize);
 GetWindowRect(Frames[framepos].hWnd,&Frames[framepos].wndSize);
  //Frames[framepos].height=Frames[framepos].wndSize.bottom-Frames[framepos].wndSize.top;
  //GetClientRect(Frames[framepos].hWnd,&Frames[framepos].wndSize);
  //Frames[framepos].height=Frames[framepos].wndSize.bottom-Frames[framepos].wndSize.top;
  return(0);
};

static int CLUIFrameOnMainMenuBuild(WPARAM wParam,LPARAM lParam)
{
  CLUIFramesLoadMainMenu();
  return 0;
}

LRESULT CALLBACK CLUIFrameSubContainerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

  RECT rect;
  int Frameid;

  Frameid=(GetWindowLong(hwnd,GWL_USERDATA));
  memset(&rect,0,sizeof(rect));

  /*
  if ((msg == WM_MOVE) || (msg == WM_MOVING) || (msg == WM_NCLBUTTONDOWN) || (msg == WM_SYSCOMMAND)  )
  {
  if (ServiceExists("Utils/SnapWindowProc"))
  {
  SnapWindowProc_t SnapInfo;
  memset(&SnapInfo,0,sizeof(SnapInfo));

  SnapInfo.hWnd = hwnd;
  SnapInfo.wParam = wParam;
  SnapInfo.lParam = lParam;
  if (CallService("Utils/SnapWindowProc",(WPARAM)&SnapInfo,msg)!=0){return(TRUE);};	
  };
  };
  */
  switch(msg) {
  case WM_ACTIVATE:   
    {
      if(TransparentFlag)
      {
        BYTE alpha;
        if ((wParam!=WA_INACTIVE || ((HWND)lParam==hwnd) || GetParent((HWND)lParam)==hwnd))
        {
          alpha=DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_ALPHA_DEFAULT);
          SetWindowPos((GetParent((HWND)lParam)),HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);            
          TRACE("SMOTH ALPHA CALLED FROM DEACTIVATION_3\n");
          SmoothAlphaTransition(hwnd, alpha, 1);
        }
      }

    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
  case WM_NOTIFY:
    TRACE("WM_NOTIFY\n");
    return SendMessage((HWND)CallService(MS_CLUI_GETHWND,0,0),msg,wParam,lParam); 
  case WM_PARENTNOTIFY:  
    TRACE("WM_PARENT_NOTIFY\n");
    return SendMessage((HWND)CallService(MS_CLUI_GETHWND,0,0),msg,wParam,lParam); 
  case WM_SYSCOMMAND:
    TRACE("WM_SYSCOMMAND\n");
    return SendMessage((HWND)CallService(MS_CLUI_GETHWND,0,0),msg,wParam,lParam); 
      //SetWindowPos(Frames[framepos].TitleBar.hwndTip, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE  );
  //  {
  //  //  int framepos;
  //   // lockfrm();
  //   // framepos=id2pos(Frameid);
  //    //SetWindowPos(Frames[framepos].TitleBar.hwndTip, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE  );
  //  //  ulockfrm();
  //  //  return(0);
  //  };
  case WM_MOVE:
    {
      if (docked) 
        return 1;
    }
    break;
  case WM_WINDOWPOSCHANGING:
    {
      if (dock_prevent_moving)
      {
        WINDOWPOS *wp;
        wp=(WINDOWPOS*)lParam;
        wp->flags|=SWP_NOMOVE/*|SWP_NOSIZE*/;
        wp->flags&=(wp->flags&~SWP_NOACTIVATE);
        return DefWindowProc(hwnd, msg, wParam, lParam);
      }
      break;
    }
  case WM_WINDOWPOSCHANGED:
    {
      if (docked && dock_prevent_moving)
        return 0;
    }
    break;
    /*	case WM_GETMINMAXINFO:
    //DefWindowProc(hwnd,msg,wParam,lParam);
    {
    int framepos;
    MINMAXINFO minmax;

    lockfrm();
    framepos=id2pos(Frameid);
    if(framepos<0||framepos>=nFramescount){ulockfrm();break;};
    if (!Frames[framepos].minmaxenabled){ulockfrm();break;};
    if (Frames[framepos].ContainerWnd==0){ulockfrm();break;};

    if (Frames[framepos].Locked)
    {
    RECT rct;

   GetWindowRect(hwnd,&rct);
    ((LPMINMAXINFO)lParam)->ptMinTrackSize.x=rct.right-rct.left;
    ((LPMINMAXINFO)lParam)->ptMinTrackSize.y=rct.bottom-rct.top;
    ((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=rct.right-rct.left;
    ((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=rct.bottom-rct.top;
    //ulockfrm();
    //return(0);
    };


    memset(&minmax,0,sizeof(minmax));
    if (SendMessage(Frames[framepos].hWnd,WM_GETMINMAXINFO,(WPARAM)0,(LPARAM)&minmax)==0)
    {
    RECT border;
    int tbh=TitleBarH*btoint(Frames[framepos].TitleBar.ShowTitleBar);
    GetBorderSize(hwnd,&border);
    if (minmax.ptMaxTrackSize.x!=0&&minmax.ptMaxTrackSize.y!=0){

    ((LPMINMAXINFO)lParam)->ptMinTrackSize.x=minmax.ptMinTrackSize.x;
    ((LPMINMAXINFO)lParam)->ptMinTrackSize.y=minmax.ptMinTrackSize.y;
    ((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=minmax.ptMaxTrackSize.x+border.left+border.right;
    ((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=minmax.ptMaxTrackSize.y+tbh+border.top+border.bottom;
    }; 

    }
    else
    {

    ulockfrm();
    return(DefWindowProc(hwnd, msg, wParam, lParam));
    };
    ulockfrm();


    }
    //return 0;

    case WM_MOVE:
    {
    int framepos;
    RECT rect;


    lockfrm();
    framepos=id2pos(Frameid);

    if(framepos<0||framepos>=nFramescount){ulockfrm();break;};
    if (Frames[framepos].ContainerWnd==0){ulockfrm();return(0);};

   GetWindowRect(Frames[framepos].ContainerWnd,&rect);
    Frames[framepos].FloatingPos.x=rect.left;
    Frames[framepos].FloatingPos.y=rect.top;
    Frames[framepos].FloatingSize.x=rect.right-rect.left;
    Frames[framepos].FloatingSize.y=rect.bottom-rect.top;

    CLUIFramesStoreFrameSettings(framepos);		
    ulockfrm();

    return(0);
    };

    case WM_SIZE:
    {
    int framepos;
    RECT rect;


    lockfrm();
    framepos=id2pos(Frameid);

    if(framepos<0||framepos>=nFramescount){ulockfrm();break;};
    if (Frames[framepos].ContainerWnd==0){ulockfrm();return(0);};
    CLUIFrameResizeFloatingFrame(framepos);

   GetWindowRect(Frames[framepos].ContainerWnd,&rect);
    Frames[framepos].FloatingPos.x=rect.left;
    Frames[framepos].FloatingPos.y=rect.top;
    Frames[framepos].FloatingSize.x=rect.right-rect.left;
    Frames[framepos].FloatingSize.y=rect.bottom-rect.top;

    CLUIFramesStoreFrameSettings(framepos);
    ulockfrm();

    return(0);
    };
    case WM_CLOSE:
    {
    DestroyWindow(hwnd);
    break;
    };
    case WM_DESTROY:
    {
    //{ CLUIFramesStoreAllFrames();};
    return(0);
    };
    */
  };

  return DefWindowProc(hwnd, msg, wParam, lParam);
};

static HWND CreateSubContainerWindow(HWND parent,int x,int y,int width,int height)
{
  HWND hwnd;
  hwnd=CreateWindowEx(MySetLayeredWindowAttributesNew?WS_EX_LAYERED:0,TEXT(CLUIFrameSubContainerClassName),TEXT("aaaa"),WS_POPUP|!LayeredFlag?WS_BORDER:0/*|WS_THICKFRAME*/,x,y,width,height,parent,0,g_hInst,0);
  SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)&~(WS_CAPTION|WS_BORDER));
  if (IsOnDesktop)
  {
    HWND hProgMan=FindWindow(TEXT("Progman"),NULL);
    if (IsWindow(hProgMan)) 
      SetParent(hwnd,hProgMan);
  }
  

  return hwnd;
};



//static int CLUIFrameContainerWndProc
LRESULT CALLBACK CLUIFrameContainerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

  RECT rect;
  int Frameid;

  Frameid=(GetWindowLong(hwnd,GWL_USERDATA));
  memset(&rect,0,sizeof(rect));
  /*
  if ((msg == WM_MOVE) || (msg == WM_MOVING) || (msg == WM_NCLBUTTONDOWN) || (msg == WM_SYSCOMMAND)  )
  {
  if (ServiceExists("Utils/SnapWindowProc"))
  {
  SnapWindowProc_t SnapInfo;
  memset(&SnapInfo,0,sizeof(SnapInfo));

  SnapInfo.hWnd = hwnd;
  SnapInfo.wParam = wParam;
  SnapInfo.lParam = lParam;
  if (CallService("Utils/SnapWindowProc",(WPARAM)&SnapInfo,msg)!=0){return(TRUE);};	
  };
  };
  */
  switch(msg)
  {

  case WM_CREATE:
    {
      int framepos;
      lockfrm();
      framepos=id2pos(Frameid);
      //SetWindowPos(Frames[framepos].TitleBar.hwndTip, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE  );
      ulockfrm();
      return(0);
    };
  case WM_GETMINMAXINFO:
    //DefWindowProc(hwnd,msg,wParam,lParam);
    {
      int framepos;
      MINMAXINFO minmax;

      lockfrm();
      framepos=id2pos(Frameid);
      if(framepos<0||framepos>=nFramescount){ulockfrm();break;};
      if (!Frames[framepos].minmaxenabled){ulockfrm();break;};
      if (Frames[framepos].ContainerWnd==0){ulockfrm();break;};

      if (Frames[framepos].Locked)
      {
        RECT rct;

       GetWindowRect(hwnd,&rct);
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.x=rct.right-rct.left;
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.y=rct.bottom-rct.top;
        ((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=rct.right-rct.left;
        ((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=rct.bottom-rct.top;
        //ulockfrm();
        //return(0);
      };


      memset(&minmax,0,sizeof(minmax));
      if (SendMessage(Frames[framepos].hWnd,WM_GETMINMAXINFO,(WPARAM)0,(LPARAM)&minmax)==0)
      {
        RECT border;
        int tbh=TitleBarH*btoint(Frames[framepos].TitleBar.ShowTitleBar);
        GetBorderSize(hwnd,&border);
        if (minmax.ptMaxTrackSize.x!=0&&minmax.ptMaxTrackSize.y!=0){

          ((LPMINMAXINFO)lParam)->ptMinTrackSize.x=minmax.ptMinTrackSize.x;
          ((LPMINMAXINFO)lParam)->ptMinTrackSize.y=minmax.ptMinTrackSize.y;
          ((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=minmax.ptMaxTrackSize.x+border.left+border.right;
          ((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=minmax.ptMaxTrackSize.y+tbh+border.top+border.bottom;
        }; 

      }
      else
      {

        ulockfrm();
        return(DefWindowProc(hwnd, msg, wParam, lParam));
      };
      ulockfrm();


    }
    //return 0;

  case WM_MOVE:
    {
      int framepos;
      RECT rect;

      TRACE("FRAME MOVING\n");
      lockfrm();
      framepos=id2pos(Frameid);

      if(framepos<0||framepos>=nFramescount){ulockfrm();break;};
      if (Frames[framepos].ContainerWnd==0){ulockfrm();return(0);};

     GetWindowRect(Frames[framepos].ContainerWnd,&rect);
      Frames[framepos].FloatingPos.x=rect.left;
      Frames[framepos].FloatingPos.y=rect.top;
      Frames[framepos].FloatingSize.x=rect.right-rect.left;
      Frames[framepos].FloatingSize.y=rect.bottom-rect.top;

      CLUIFramesStoreFrameSettings(framepos);		
      ulockfrm();

      return(0);
    };

  case WM_SIZE:
    {
      int framepos;
      RECT rect;


      lockfrm();
      framepos=id2pos(Frameid);

      if(framepos<0||framepos>=nFramescount){ulockfrm();break;};
      if (Frames[framepos].ContainerWnd==0){ulockfrm();return(0);};
      CLUIFrameResizeFloatingFrame(framepos);

     GetWindowRect(Frames[framepos].ContainerWnd,&rect);
      Frames[framepos].FloatingPos.x=rect.left;
      Frames[framepos].FloatingPos.y=rect.top;
      Frames[framepos].FloatingSize.x=rect.right-rect.left;
      Frames[framepos].FloatingSize.y=rect.bottom-rect.top;

      CLUIFramesStoreFrameSettings(framepos);
      ulockfrm();

      return(0);
    };
  case WM_CLOSE:
    {
      DestroyWindow(hwnd);
      break;
    };
  case WM_DESTROY:
    {
      //{ CLUIFramesStoreAllFrames();};
      return(0);
    };
    /*
    case WM_COMMAND:
    case WM_NOTIFY:
    return(SendMessage(pcli->hwndContactList,msg,wParam,lParam));
    */


  };
  return DefWindowProc(hwnd, msg, wParam, lParam);
};
static HWND CreateContainerWindow(HWND parent,int x,int y,int width,int height)
{
  return(CreateWindow(TEXT("FramesContainer"),TEXT("aaaa"),WS_POPUP|WS_THICKFRAME,x,y,width,height,parent,0,g_hInst,0));
};


int CLUIFrameSetFloat(WPARAM wParam,LPARAM lParam)
{	
  int hwndtmp,hwndtooltiptmp;

  lockfrm();
  wParam=id2pos(wParam);
  if(wParam>=0&&(int)wParam<nFramescount)

    //parent=GetParent(Frames[wParam].hWnd);
    if (Frames[wParam].floating || (lParam&2))
    {
      //SetWindowLong(Frames[wParam].hWnd,GWL_STYLE,Frames[wParam].oldstyles);
      //SetWindowLong(Frames[wParam].TitleBar.hwnd,GWL_STYLE,Frames[wParam].TitleBar.oldstyles);
      if (Frames[wParam].OwnerWindow!=(HWND)-2 &&Frames[wParam].visible)
      {
        if (Frames[wParam].OwnerWindow==0) Frames[wParam].OwnerWindow=CreateSubContainerWindow(pcli->hwndContactList,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,10,10);
        ShowWindowNew(Frames[wParam].OwnerWindow,(Frames[wParam].visible && Frames[wParam].collapsed && IsWindowVisible(pcli->hwndContactList))?SW_SHOW/*NOACTIVATE*/:SW_HIDE);
        SetParent(Frames[wParam].hWnd,Frames[wParam].OwnerWindow);
        SetParent(Frames[wParam].TitleBar.hwnd,pcli->hwndContactList);
        SetWindowLong(Frames[wParam].OwnerWindow,GWL_USERDATA,Frames[wParam].id);
        Frames[wParam].floating=FALSE;
        if (!(lParam&2))
        {
          DestroyWindow(Frames[wParam].ContainerWnd);
          Frames[wParam].ContainerWnd=0;
        }
      }
      else
      {
        SetParent(Frames[wParam].hWnd,pcli->hwndContactList);
        SetParent(Frames[wParam].TitleBar.hwnd,pcli->hwndContactList);
        Frames[wParam].floating=FALSE;
        DestroyWindow(Frames[wParam].ContainerWnd);
        Frames[wParam].ContainerWnd=0;
      }
    }
    else
    {
      RECT recttb,rectw,border;
      int temp;
      int neww,newh;
      BOOLEAN locked;

      Frames[wParam].oldstyles=GetWindowLong(Frames[wParam].hWnd,GWL_STYLE);
      Frames[wParam].TitleBar.oldstyles=GetWindowLong(Frames[wParam].TitleBar.hwnd,GWL_STYLE);
      locked=Frames[wParam].Locked;
      Frames[wParam].Locked=FALSE;
      Frames[wParam].minmaxenabled=FALSE;

     GetWindowRect(Frames[wParam].hWnd,&rectw);
     GetWindowRect(Frames[wParam].TitleBar.hwnd,&recttb);
      if (!Frames[wParam].TitleBar.ShowTitleBar){
        recttb.top=recttb.bottom=recttb.left=recttb.right=0;
      };

      Frames[wParam].ContainerWnd=CreateContainerWindow(pcli->hwndContactList,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,10,10);




      SetParent(Frames[wParam].hWnd,Frames[wParam].ContainerWnd);
      SetParent(Frames[wParam].TitleBar.hwnd,Frames[wParam].ContainerWnd);
      if (Frames[wParam].OwnerWindow!=(HWND)-2 && Frames[wParam].OwnerWindow!=0)
      {
        DestroyWindow(Frames[wParam].OwnerWindow);
        Frames[wParam].OwnerWindow=0;
      }

      //SetWindowPos(Frames[wParam].TitleBar.hwnd,HWND_TOP,0,0,0,0,SWP_NOSIZE);
      //SetWindowPos(Frames[wParam].hWnd,HWND_TOP,0,recttb.bottom-recttb.top,0,0,SWP_NOSIZE);
      GetBorderSize(Frames[wParam].ContainerWnd,&border);


      SetWindowLong(Frames[wParam].ContainerWnd,GWL_USERDATA,Frames[wParam].id);
      if ((lParam==1))
      {
        if ((Frames[wParam].FloatingPos.x!=0)&&(Frames[wParam].FloatingPos.y!=0))
        {
          if (Frames[wParam].FloatingPos.x<20){Frames[wParam].FloatingPos.x=40;};
          if (Frames[wParam].FloatingPos.y<20){Frames[wParam].FloatingPos.y=40;};

          SetWindowPos(Frames[wParam].ContainerWnd,HWND_TOPMOST,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,Frames[wParam].FloatingSize.x,Frames[wParam].FloatingSize.y,SWP_HIDEWINDOW|SWP_NOACTIVATE);
        }else
        {
          SetWindowPos(Frames[wParam].ContainerWnd,HWND_TOPMOST,120,120,140,140,SWP_HIDEWINDOW|SWP_NOACTIVATE);
        };
      }
      else if (lParam==0)
      {
        neww=rectw.right-rectw.left+border.left+border.right;
        newh=(rectw.bottom-rectw.top)+(recttb.bottom-recttb.top)+border.top+border.bottom;
        if (neww<20){neww=40;};
        if (newh<20){newh=40;};
        if (Frames[wParam].FloatingPos.x<20){Frames[wParam].FloatingPos.x=40;};
        if (Frames[wParam].FloatingPos.y<20){Frames[wParam].FloatingPos.y=40;};

        SetWindowPos(Frames[wParam].ContainerWnd,HWND_TOPMOST,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,neww,newh,SWP_HIDEWINDOW|SWP_NOACTIVATE);
      };


      SetWindowTextA(Frames[wParam].ContainerWnd,Frames[wParam].TitleBar.tbname);

      temp=GetWindowLong(Frames[wParam].ContainerWnd,GWL_EXSTYLE);
      temp|=WS_EX_TOOLWINDOW|WS_EX_TOPMOST ;
      SetWindowLong(Frames[wParam].ContainerWnd,GWL_EXSTYLE,temp);

      //SetWindowLong(Frames[wParam].hWnd,GWL_STYLE,WS_POPUP|(Frames[wParam].oldstyles&(~WS_CHILD)));
      //SetWindowLong(Frames[wParam].TitleBar.hwnd,GWL_STYLE,WS_POPUP|(Frames[wParam].TitleBar.oldstyles&(~WS_CHILD)));

      Frames[wParam].floating=TRUE;
      Frames[wParam].Locked=locked;

    }
    CLUIFramesStoreFrameSettings(wParam);
    Frames[wParam].minmaxenabled=TRUE;
    hwndtooltiptmp=(int)Frames[wParam].TitleBar.hwndTip;

    hwndtmp=(int)Frames[wParam].ContainerWnd;
    ulockfrm();
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
    SendMessage((HWND)hwndtmp,WM_SIZE,0,0);


    SetWindowPos((HWND)hwndtooltiptmp, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE|SWP_NOACTIVATE  );

    return 0;
}


static CLUIFrameOnModulesLoad(WPARAM wParam,LPARAM lParam)
{
  CLUIFramesLoadMainMenu(0,0);
  CLUIFramesCreateMenuForFrame(-1,-1,000010000,MS_CLIST_ADDCONTEXTFRAMEMENUITEM);
}
static CLUIFrameOnModulesUnload(WPARAM wParam,LPARAM lParam)
{
  //

  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIVisible, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMITitle, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMITBVisible, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMILock, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIColl, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIFloating, 1 ); 	
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIAlignTop, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIAlignClient, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIAlignBottom, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIBorder, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIAlignRoot, 1 );


  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIPosUp, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIPosDown, 1 );
  CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIPosRoot, 1 );


  CallService( MO_REMOVEMENUITEM, ( LPARAM )contMIVisible, 1 );
  CallService( MO_REMOVEMENUITEM, ( LPARAM )contMITitle, 1 );
  CallService( MO_REMOVEMENUITEM, ( LPARAM )contMITBVisible, 1 );
  CallService( MO_REMOVEMENUITEM, ( LPARAM )contMILock, 1 );
  CallService( MO_REMOVEMENUITEM, ( LPARAM )contMIColl, 1 );
  CallService( MO_REMOVEMENUITEM, ( LPARAM )contMIFloating, 1 ); 	
  CallService( MO_REMOVEMENUITEM, ( LPARAM )contMIBorder, 1 );
  CallService( MO_REMOVEMENUITEM, ( LPARAM )contMIAlignRoot, 1 );
  CallService( MO_REMOVEMENUITEM, ( LPARAM )contMIPosRoot, 1 );
}


int LoadCLUIFramesModule(void)
{
  WNDCLASS wndclass;
  WNDCLASS cntclass;
  WNDCLASS subconclass;

  wndclass.style         = CS_DBLCLKS;//|CS_HREDRAW|CS_VREDRAW ;
  wndclass.lpfnWndProc   = CLUIFrameTitleBarProc;
  wndclass.cbClsExtra    = 0;
  wndclass.cbWndExtra    = 0;
  wndclass.hInstance     = g_hInst;
  wndclass.hIcon         = NULL;
  wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wndclass.hbrBackground = NULL;
  wndclass.lpszMenuName  = NULL;
  wndclass.lpszClassName = TEXT(CLUIFrameTitleBarClassName);
  RegisterClass(&wndclass);

  subconclass.style         = CS_DBLCLKS;//|CS_HREDRAW|CS_VREDRAW ;
  subconclass.lpfnWndProc   = CLUIFrameSubContainerProc;
  subconclass.cbClsExtra    = 0;
  subconclass.cbWndExtra    = 0;
  subconclass.hInstance     = g_hInst;
  subconclass.hIcon         = NULL;
  subconclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  subconclass.hbrBackground = NULL;
  subconclass.lpszMenuName  = NULL;
  subconclass.lpszClassName = TEXT(CLUIFrameSubContainerClassName);
  RegisterClass(&subconclass);

  pcli->hwndContactList=(HWND)CallService(MS_CLUI_GETHWND,0,0);
  //pcli->hwndStatus=FindWindowEx(pcli->hwndContactList,NULL,"msctls_statusbar32",NULL);
  //container helper

  cntclass.style         = CS_DBLCLKS/*|CS_HREDRAW|CS_VREDRAW*/|( IsWinVerXPPlus()  ? CS_DROPSHADOW : 0);
  cntclass.lpfnWndProc   = CLUIFrameContainerWndProc;
  cntclass.cbClsExtra    = 0;
  cntclass.cbWndExtra    = 0;
  cntclass.hInstance     = g_hInst;
  cntclass.hIcon         = NULL;
  cntclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  cntclass.hbrBackground = NULL;
  cntclass.lpszMenuName  = NULL;
  cntclass.lpszClassName = TEXT("FramesContainer");
  RegisterClass(&cntclass);
  //end container helper

  GapBetweenFrames=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);

  nFramescount=0;
  InitializeCriticalSection(&csFrameHook);
  InitFramesMenus();

  HookEvent(ME_SYSTEM_MODULESLOADED,CLUIFrameOnModulesLoad);
  HookEvent(ME_CLIST_PREBUILDFRAMEMENU,CLUIFramesModifyContextMenuForFrame);
  HookEvent(ME_CLIST_PREBUILDMAINMENU,CLUIFrameOnMainMenuBuild);
  HookEvent(ME_SYSTEM_PRESHUTDOWN,  CLUIFrameOnModulesUnload);

  CreateServiceFunction(MS_CLIST_FRAMES_ADDFRAME,CLUIFramesAddFrame);
  CreateServiceFunction(MS_CLIST_FRAMES_REMOVEFRAME,CLUIFramesRemoveFrame);

  CreateServiceFunction(MS_CLIST_FRAMES_SETFRAMEOPTIONS,CLUIFramesSetFrameOptions);
  CreateServiceFunction(MS_CLIST_FRAMES_GETFRAMEOPTIONS,CLUIFramesGetFrameOptions);
  CreateServiceFunction(MS_CLIST_FRAMES_UPDATEFRAME,CLUIFramesUpdateFrame);

  CreateServiceFunction(MS_CLIST_FRAMES_SHFRAMETITLEBAR,CLUIFramesShowHideFrameTitleBar);
  CreateServiceFunction(MS_CLIST_FRAMES_SHOWALLFRAMESTB,CLUIFramesShowAllTitleBars);
  CreateServiceFunction(MS_CLIST_FRAMES_HIDEALLFRAMESTB,CLUIFramesHideAllTitleBars);
  CreateServiceFunction(MS_CLIST_FRAMES_SHFRAME,CLUIFramesShowHideFrame);
  CreateServiceFunction(MS_CLIST_FRAMES_SHOWALLFRAMES,CLUIFramesShowAll);

  CreateServiceFunction(MS_CLIST_FRAMES_ULFRAME,CLUIFramesLockUnlockFrame);
  CreateServiceFunction(MS_CLIST_FRAMES_UCOLLFRAME,CLUIFramesCollapseUnCollapseFrame);
  CreateServiceFunction(MS_CLIST_FRAMES_SETUNBORDER,CLUIFramesSetUnSetBorder);

  CreateServiceFunction(CLUIFRAMESSETALIGN,CLUIFramesSetAlign);
  CreateServiceFunction(CLUIFRAMESMOVEUPDOWN,CLUIFramesMoveUpDown);


  CreateServiceFunction(CLUIFRAMESSETALIGNALTOP,CLUIFramesSetAlignalTop);
  CreateServiceFunction(CLUIFRAMESSETALIGNALCLIENT,CLUIFramesSetAlignalClient);
  CreateServiceFunction(CLUIFRAMESSETALIGNALBOTTOM,CLUIFramesSetAlignalBottom);

  CreateServiceFunction("Set_Floating",CLUIFrameSetFloat);
  hWndExplorerToolBar	=FindWindowEx(0,0,TEXT("Shell_TrayWnd"),NULL);
  OnFrameTitleBarBackgroundChange(0,0);
  FramesSysNotStarted=FALSE;
  return 0;
}

int UnLoadCLUIFramesModule(void)
{
  int i;
  FramesSysNotStarted=TRUE;
  CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);
  CLUIFramesStoreAllFrames();
  lockfrm();
  //UnitFramesMenu();
  for (i=0;i<nFramescount;i++) {
    //		RemoveContextFrameMenuItem((WPARAM)Frames[i].,(LPARAM)0)
    DestroyWindow(Frames[i].hWnd);
    Frames[i].hWnd=(HWND)-1;
    DestroyWindow(Frames[i].TitleBar.hwnd);
    Frames[i].TitleBar.hwnd=(HWND)-1;
    DestroyWindow(Frames[i].ContainerWnd);
    Frames[i].ContainerWnd=(HWND)-1;
    DestroyMenu(Frames[i].TitleBar.hmenu);
    DestroyWindow(Frames[i].OwnerWindow );
    Frames[i].OwnerWindow=(HWND)-2;

    if (Frames[i].name!=NULL) free(Frames[i].name);
    if (Frames[i].TitleBar.tbname!=NULL) free(Frames[i].TitleBar.tbname);
  }
  if(Frames) free(Frames);
  Frames=NULL;
  nFramescount=0;
  UnregisterClass(TEXT(CLUIFrameTitleBarClassName),g_hInst);
  DeleteObject(TitleBarFont);
  ulockfrm();
  DeleteCriticalSection(&csFrameHook);
  UnitFramesMenu();
  return 0;
}











