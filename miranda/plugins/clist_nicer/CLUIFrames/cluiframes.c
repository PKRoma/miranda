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

#include "../commonheaders.h"
#include "cluiframes.h"
void RefreshButtons();
HFONT __fastcall ChangeToFont(HDC hdc, struct ClcData *dat, int id, int *fontHeight);

extern pfnDrawAlpha pDrawAlpha;
extern HIMAGELIST himlExtraImages;
extern int g_shutDown;
extern HWND g_hwndViewModeFrame, g_hwndEventArea;
extern StatusItems_t *StatusItems;
extern struct ClcData *g_clcData;
extern int mf_updatethread_running;

extern DWORD WINAPI MF_UpdateThread(LPVOID p);
extern HANDLE hThreadMFUpdate;;

HANDLE hExtraImageListRebuilding, hExtraImageApplying;
HANDLE hStatusBarShowToolTipEvent, hStatusBarHideToolTipEvent;
HANDLE g_hEventThread = 0;

//#include "m_skin_eng.h"

//not needed,now use MS_CLIST_FRAMEMENUNOTIFY service
//HANDLE hPreBuildFrameMenuEvent;//external event from clistmenus

LOGFONT TitleBarLogFont={0};

extern HINSTANCE g_hInst;
extern struct CluiData g_CluiData;

//we use dynamic frame list,
//but who wants so huge number of frames ??
#define MAX_FRAMES		16

#define UNCOLLAPSED_FRAME_SIZE		0

//legacy menu support
#define frame_menu_lock				1
#define frame_menu_visible			2
#define frame_menu_showtitlebar		3
#define frame_menu_floating			4
#define frame_menu_skinned          5

extern  int ModifyMenuItemProxy(WPARAM wParam,LPARAM lParam);
static int UpdateTBToolTip(int framepos);
int CLUIFrameSetFloat(WPARAM wParam,LPARAM lParam);
int CLUIFrameResizeFloatingFrame(int framepos);
extern int ProcessCommandProxy(WPARAM wParam,LPARAM lParam);
extern int InitFramesMenus(void);
extern int UnitFramesMenu();
static int CLUIFramesReSort();

boolean FramesSysNotStarted=TRUE;
HPEN g_hPenCLUIFrames = 0;

typedef struct {
    int order;
    int realpos;
} SortData;

static SortData g_sd[MAX_FRAMES];

static HHOOK g_hFrameHook = 0;

/*
static int InstallDialogBoxHook(void)
{
    g_hFrameHook = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)FrameHookProc,
									NULL, GetCurrentThreadId());
	return (g_hFrameHook != NULL);
}

static int RemoveDialogBoxHook(void)
{
    UnhookWindowsHookEx(g_hFrameHook);
	g_hFrameHook = NULL;
	return 0;
}
*/
static int sortfunc(const void *a,const void *b)
{
    SortData *sd1,*sd2;
    sd1=(SortData *)a;
    sd2=(SortData *)b;
    if (sd1->order > sd2->order) {
        return(1);
    }
    if (sd1->order < sd2->order) {
        return(-1);
    }
    return(0);
}

#define CLUIFRAMESSETALIGN					"CLUIFramesSetAlign"
#define CLUIFRAMESSETALIGNALTOP				"CLUIFramesSetAlignalTop"
#define CLUIFRAMESSETALIGNALCLIENT			"CLUIFramesSetAlignalClient"
#define CLUIFRAMESSETALIGNALBOTTOM			"CLUIFramesSetAlignalBottom"
#define CLUIFRAMESMOVEUP					"CLUIFramesMoveUp"
#define CLUIFRAMESMOVEDOWN					"CLUIFramesMoveDown"

//static wndFrame Frames[MAX_FRAMES];
static wndFrame *Frames=NULL;

wndFrame *wndFrameCLC = NULL, *wndFrameEventArea = NULL, *wndFrameViewMode = NULL;

static int nFramescount = 0;
static int alclientFrame=-1;//for fast access to frame with alclient properties
static int NextFrameId = 100;

static int TitleBarH=DEFAULT_TITLEBAR_HEIGHT;
static boolean resizing=FALSE;

// menus
static HANDLE contMIVisible,contMITitle,contMITBVisible,contMILock,contMIColl,contMIFloating;
static HANDLE contMIAlignRoot;
static HANDLE contMIAlignTop,contMIAlignClient,contMIAlignBottom;
static HANDLE contMIBorder, contMISkinned;
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

    for (i=0;i<nFramescount;i++) {
        if (Frames[i].id==id) return(i);
    };
    return(-1);
};

static int __forceinline btoint(BOOLEAN b)
{
    return(b ? 1 : 0);
}

static void __forceinline lockfrm()
{
    if (FramesSysNotStarted == FALSE)
        EnterCriticalSection(&csFrameHook);
}

static void __forceinline ulockfrm()
{
    //if (FramesSysNotStarted == FALSE)
        LeaveCriticalSection(&csFrameHook);
}

static wndFrame* FindFrameByWnd( HWND hwnd )
{
    BOOL        bFound  = FALSE;
    int i;

    if ( hwnd == NULL ) return( NULL );

    for (i=0;i<nFramescount;i++) {
        if ((Frames[i].floating)&&(Frames[i].ContainerWnd==hwnd) ) {
            return(&Frames[i]);
        }
    }
    return( NULL);
}


static void DockThumbs( wndFrame *pThumbLeft, wndFrame *pThumbRight, BOOL bMoveLeft )
{
    if ( ( pThumbRight->dockOpt.hwndLeft == NULL ) && ( pThumbLeft->dockOpt.hwndRight == NULL ) ) {
        pThumbRight->dockOpt.hwndLeft   = pThumbLeft->ContainerWnd;
        pThumbLeft->dockOpt.hwndRight   = pThumbRight->ContainerWnd;
    }
}


static void UndockThumbs( wndFrame *pThumb1, wndFrame *pThumb2 )
{
    if ( ( pThumb1 == NULL ) || ( pThumb2 == NULL ) ) {
        return;
    }

    if ( pThumb1->dockOpt.hwndRight == pThumb2->ContainerWnd ) {
        pThumb1->dockOpt.hwndRight = NULL;
    }

    if ( pThumb1->dockOpt.hwndLeft == pThumb2->ContainerWnd ) {
        pThumb1->dockOpt.hwndLeft = NULL;
    }

    if ( pThumb2->dockOpt.hwndRight == pThumb1->ContainerWnd ) {
        pThumb2->dockOpt.hwndRight = NULL;
    }

    if ( pThumb2->dockOpt.hwndLeft == pThumb1->ContainerWnd ) {
        pThumb2->dockOpt.hwndLeft = NULL;
    }
}

BOOLEAN bMoveTogether;

static void PositionThumb( wndFrame *pThumb, short nX, short nY )
{
    wndFrame    *pCurThumb  = &Frames[0];
    wndFrame    *pDockThumb = pThumb;
    wndFrame    fakeMainWindow;
    wndFrame    fakeTaskBarWindow;
    RECT        rc;
    RECT        rcThumb;
    RECT        rcOld;
    SIZE        sizeScreen;
    int         nNewX;
    int         nNewY;
    int         nOffs       = 10;
    int         nWidth;
    int         nHeight;
    POINT       pt;
    RECT        rcLeft;
    RECT        rcTop;
    RECT        rcRight;
    RECT        rcBottom;
    BOOL        bDocked;
    BOOL        bDockedLeft;
    BOOL        bDockedRight;
    BOOL        bLeading;
    int         frmidx=0;

    if ( pThumb == NULL ) return;

    sizeScreen.cx = GetSystemMetrics( SM_CXSCREEN );
    sizeScreen.cy = GetSystemMetrics( SM_CYSCREEN );

    // Get thumb dimnsions
    GetWindowRect( pThumb->ContainerWnd, &rcThumb );
    nWidth  = rcThumb.right - rcThumb.left;
    nHeight = rcThumb.bottom - rcThumb.top;

    // Docking to the edges of the screen
    nNewX = nX < nOffs ? 0 : nX;
    nNewX = nNewX > ( sizeScreen.cx - nWidth - nOffs ) ? ( sizeScreen.cx - nWidth ) : nNewX;
    nNewY = nY < nOffs ? 0 : nY;
    nNewY = nNewY > ( sizeScreen.cy - nHeight - nOffs ) ? ( sizeScreen.cy - nHeight ) : nNewY;

    bLeading = pThumb->dockOpt.hwndRight != NULL;

    if ( bMoveTogether ) {
        UndockThumbs( pThumb,  FindFrameByWnd( pThumb->dockOpt.hwndLeft ) );
        GetWindowRect( pThumb->ContainerWnd, &rcOld );
    }

    memset(&fakeMainWindow,0,sizeof(fakeMainWindow));
    fakeMainWindow.ContainerWnd=pcli->hwndContactList;
    fakeMainWindow.floating=TRUE;

    memset(&fakeTaskBarWindow,0,sizeof(fakeTaskBarWindow));
    fakeTaskBarWindow.ContainerWnd=hWndExplorerToolBar;
    fakeTaskBarWindow.floating=TRUE;


    while ( pCurThumb != NULL ) {
        if (pCurThumb->floating) {

            if ( pCurThumb != pThumb ) {
                GetWindowRect( pThumb->ContainerWnd, &rcThumb );
                OffsetRect( &rcThumb, nX - rcThumb.left, nY - rcThumb.top );

                GetWindowRect( pCurThumb->ContainerWnd, &rc );

                rcLeft.left     = rc.left - nOffs;
                rcLeft.top      = rc.top - nOffs;
                rcLeft.right    = rc.left + nOffs;
                rcLeft.bottom   = rc.bottom + nOffs;

                rcTop.left      = rc.left - nOffs;
                rcTop.top       = rc.top - nOffs;
                rcTop.right     = rc.right + nOffs;
                rcTop.bottom    = rc.top + nOffs;

                rcRight.left    = rc.right - nOffs;
                rcRight.top     = rc.top - nOffs;
                rcRight.right   = rc.right + nOffs;
                rcRight.bottom  = rc.bottom + nOffs;

                rcBottom.left   = rc.left - nOffs;
                rcBottom.top    = rc.bottom - nOffs;
                rcBottom.right  = rc.right + nOffs;
                rcBottom.bottom = rc.bottom + nOffs;


                bDockedLeft     = FALSE;
                bDockedRight    = FALSE;

            // Upper-left
                pt.x    = rcThumb.left;
                pt.y    = rcThumb.top;
                bDocked = FALSE;

                if ( PtInRect( &rcRight, pt ) ) {
                    nNewX   = rc.right;
                    bDocked = TRUE;
                }

                if ( PtInRect( &rcBottom, pt ) ) {
                    nNewY = rc.bottom;

                    if ( PtInRect( &rcLeft, pt ) ) {
                        nNewX = rc.left;
                    }
                }

                if ( PtInRect( &rcTop, pt ) ) {
                    nNewY       = rc.top;
                    bDockedLeft = bDocked;
                }

            // Upper-right
                pt.x    = rcThumb.right;
                pt.y    = rcThumb.top;
                bDocked = FALSE;

                if ( !bLeading && PtInRect( &rcLeft, pt ) ) {
                    if ( !bDockedLeft ) {
                        nNewX   = rc.left - nWidth;
                        bDocked = TRUE;
                    }
                    else if ( rc.right == rcThumb.left ) {
                        bDocked = TRUE;
                    }
                }


                if ( PtInRect( &rcBottom, pt ) ) {
                    nNewY = rc.bottom;

                    if ( PtInRect( &rcRight, pt ) ) {
                        nNewX = rc.right - nWidth;
                    }
                }

                if ( !bLeading && PtInRect( &rcTop, pt ) ) {
                    nNewY           = rc.top;
                    bDockedRight    = bDocked;
                }

                if ( bMoveTogether ) {
                    if ( bDockedRight ) {
                        DockThumbs( pThumb, pCurThumb, TRUE );
                    }

                    if ( bDockedLeft ) {
                        DockThumbs( pCurThumb, pThumb, FALSE );
                    }
                }

            // Lower-left
                pt.x = rcThumb.left;
                pt.y = rcThumb.bottom;

                if ( PtInRect( &rcRight, pt ) ) {
                    nNewX = rc.right;
                }

                if ( PtInRect( &rcTop, pt ) ) {
                    nNewY = rc.top - nHeight;

                    if ( PtInRect( &rcLeft, pt ) ) {
                        nNewX = rc.left;
                    }
                }


            // Lower-right
                pt.x = rcThumb.right;
                pt.y = rcThumb.bottom;

                if ( !bLeading && PtInRect( &rcLeft, pt ) ) {
                    nNewX = rc.left - nWidth;
                }

                if ( !bLeading && PtInRect( &rcTop, pt ) ) {
                    nNewY = rc.top - nHeight;

                    if ( PtInRect( &rcRight, pt ) ) {
                        nNewX = rc.right - nWidth;
                    }
                }
            }

        };
        frmidx++;
        if (pCurThumb->ContainerWnd==fakeTaskBarWindow.ContainerWnd) {
            break;
        };
        if (pCurThumb->ContainerWnd==fakeMainWindow.ContainerWnd) {
            pCurThumb=&fakeTaskBarWindow;continue;
        };
        if (frmidx==nFramescount) {
            pCurThumb=&fakeMainWindow;continue;
        }

        pCurThumb = &Frames[frmidx];



    }

    // Adjust coords once again
    nNewX = nNewX < nOffs ? 0 : nNewX;
    nNewX = nNewX > ( sizeScreen.cx - nWidth - nOffs ) ? ( sizeScreen.cx - nWidth ) : nNewX;
    nNewY = nNewY < nOffs ? 0 : nNewY;
    nNewY = nNewY > ( sizeScreen.cy - nHeight - nOffs ) ? ( sizeScreen.cy - nHeight ) : nNewY;


    SetWindowPos(   pThumb->ContainerWnd,
                    0,
                    nNewX,
                    nNewY,
                    0,
                    0,
                    SWP_NOSIZE | SWP_NOZORDER );


    // OK, move all docked thumbs
    if ( bMoveTogether ) {
        pDockThumb = FindFrameByWnd( pDockThumb->dockOpt.hwndRight );

        PositionThumb( pDockThumb, (short)( nNewX + nWidth ), (short)nNewY );
    }
}


//////////



void GetBorderSize(HWND hwnd,RECT *rect)
{
    RECT wr,cr;
    POINT pt1,pt2;

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
char __forceinline *AS(char *str,const char *setting,char *addstr)
{
    if (str!=NULL) {
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

    _itoa(pos,sadd,10);

    //DBWriteContactSettingString(0,CLUIFrameModule,strcat("Name",sadd),Frames[Frameid].name);
    //boolean
    Frames[Frameid].collapsed = DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"Collapse",sadd),Frames[Frameid].collapsed);

    Frames[Frameid].Locked                  =DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"Locked",sadd),Frames[Frameid].Locked);
    Frames[Frameid].visible                 =DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"Visible",sadd),Frames[Frameid].visible);
    Frames[Frameid].TitleBar.ShowTitleBar   =DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"TBVisile",sadd),Frames[Frameid].TitleBar.ShowTitleBar);

    Frames[Frameid].height                  =DBGetContactSettingWord(0,CLUIFrameModule,AS(buf,"Height",sadd),Frames[Frameid].height);
    Frames[Frameid].HeightWhenCollapsed     =DBGetContactSettingWord(0,CLUIFrameModule,AS(buf,"HeightCollapsed",sadd),0);
    Frames[Frameid].align                   =DBGetContactSettingWord(0,CLUIFrameModule,AS(buf,"Align",sadd),Frames[Frameid].align);

    Frames[Frameid].FloatingPos.x       =DBGetContactSettingRangedWord(0,CLUIFrameModule,AS(buf,"FloatX",sadd),100,0,1024);
    Frames[Frameid].FloatingPos.y       =DBGetContactSettingRangedWord(0,CLUIFrameModule,AS(buf,"FloatY",sadd),100,0,1024);
    Frames[Frameid].FloatingSize.x      =DBGetContactSettingRangedWord(0,CLUIFrameModule,AS(buf,"FloatW",sadd),100,0,1024);
    Frames[Frameid].FloatingSize.y      =DBGetContactSettingRangedWord(0,CLUIFrameModule,AS(buf,"FloatH",sadd),100,0,1024);

    Frames[Frameid].floating            =DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"Floating",sadd),0);
    Frames[Frameid].order               =DBGetContactSettingWord(0,CLUIFrameModule,AS(buf,"Order",sadd),0);

    Frames[Frameid].UseBorder           =DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"UseBorder",sadd),Frames[Frameid].UseBorder);
    Frames[Frameid].Skinned             =DBGetContactSettingByte(0,CLUIFrameModule,AS(buf,"Skinned",sadd),Frames[Frameid].Skinned);
    return 0;
}

int DBStoreFrameSettingsAtPos(int pos,int Frameid)
{
    char sadd[16];
    char buf[255];

    _itoa(pos,sadd,10);

    DBWriteContactSettingTString(0,CLUIFrameModule,AS(buf,"Name",sadd),Frames[Frameid].name);
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

    DBWriteContactSettingByte(0,CLUIFrameModule,AS(buf,"Skinned",sadd),Frames[Frameid].Skinned);
    //DBWriteContactSettingString(0,CLUIFrameModule,AS(buf,"TBName",sadd),Frames[Frameid].TitleBar.tbname);
    return 0;
}

int LocateStorePosition(int Frameid,int maxstored)
{
    int i;
    LPTSTR frmname;
    char settingname[255];
    if(Frames[Frameid].name==NULL) return -1;

    for (i=0;i<maxstored;i++) {
        mir_snprintf(settingname,sizeof(settingname),"Name%d",i);
        frmname=DBGetStringT(0,CLUIFrameModule,settingname);
        if (frmname==NULL) continue;
        if (lstrcmpi(frmname,Frames[Frameid].name)==0) {
            mir_free(frmname);
            return i;
        }
        mir_free(frmname);
    }
    return -1;
}

int CLUIFramesLoadFrameSettings(int Frameid)
{
    int storpos,maxstored;

    if (FramesSysNotStarted) return -1;

    if (Frameid<0||Frameid>=nFramescount) 
		return -1;

    maxstored = DBGetContactSettingWord(0,CLUIFrameModule,"StoredFrames", -1);
    if (maxstored == -1) 
        return 0;

    storpos=LocateStorePosition(Frameid,maxstored);
    if (storpos==-1) 
        return 0;

    DBLoadFrameSettingsAtPos(storpos,Frameid);
    return 0;
}

int CLUIFramesStoreFrameSettings(int Frameid)
{
    int maxstored,storpos;

    if (FramesSysNotStarted) 
		return -1;

    if (Frameid<0||Frameid>=nFramescount) 
		return -1;

    maxstored=DBGetContactSettingWord(0,CLUIFrameModule,"StoredFrames",-1);
    if (maxstored==-1) 
		maxstored=0;

    storpos=LocateStorePosition(Frameid,maxstored);
	if (storpos==-1) {
        storpos=maxstored; 
		maxstored++;
	}

    DBStoreFrameSettingsAtPos(storpos,Frameid);
    DBWriteContactSettingWord(0,CLUIFrameModule,"StoredFrames",(WORD)maxstored);
    return 0;
}

int CLUIFramesStoreAllFrames()
{
    int i;
    
	if (FramesSysNotStarted) 
		return -1;
    
	if(g_shutDown)
        return -1;

    lockfrm();
    for (i=0;i<nFramescount;i++)
        CLUIFramesStoreFrameSettings(i);
    ulockfrm();
    return 0;
}

// Get client frame
int CLUIFramesGetalClientFrame(void)
{
    int i;
    if (FramesSysNotStarted) 
        return -1;

    if (alclientFrame!=-1)
        return alclientFrame;

    if (alclientFrame!=-1) {
    /* this value could become invalid if RemoveItemFromList was called,
     * so we double-check */
        if (alclientFrame<nFramescount) {
            if (Frames[alclientFrame].align==alClient) {
                return alclientFrame;
            }
        }
    }

    for (i=0;i<nFramescount;i++)
        if (Frames[i].align==alClient) {
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
    mi.icolibItem=LoadSkinnedIconHandle(SKINICON_OTHER_MIRANDA); //LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_MIRANDA));
    mi.pszPopupName=(char *)root;
    mi.popupPosition=frameid;
    mi.position=popuppos++;
    mi.pszName=Translate("&FrameTitle");
    mi.flags=CMIF_CHILDPOPUP|CMIF_GRAYED|CMIF_ICONFROMICOLIB;
    mi.pszContactOwner=(char *)0;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if (frameid==-1) contMITitle=menuid;
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
    if (frameid==-1) contMIVisible=menuid;
    else Frames[framepos].MenuHandles.MIVisible=menuid;

    mi.pszPopupName=(char *)root;
    mi.popupPosition=frameid;
    mi.position=popuppos++;
    mi.pszName=Translate("&Show TitleBar");
    mi.flags=CMIF_CHILDPOPUP|CMIF_CHECKED;
    mi.pszService=MS_CLIST_FRAMES_SHFRAMETITLEBAR;
    mi.pszContactOwner=(char *)0;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if (frameid==-1) contMITBVisible=menuid;
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
    if (frameid==-1) contMILock=menuid;
    else Frames[framepos].MenuHandles.MILock=menuid;

    mi.pszPopupName=(char *)root;
    mi.popupPosition=frameid;
    mi.position=popuppos++;
    mi.pszName=Translate("&Collapsed");
    mi.flags=CMIF_CHILDPOPUP|CMIF_CHECKED;
    mi.pszService=MS_CLIST_FRAMES_UCOLLFRAME;
    mi.pszContactOwner=(char *)0;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if (frameid==-1) contMIColl=menuid;
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
    if (frameid==-1) contMIFloating=menuid;
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
    if (frameid==-1) contMIBorder=menuid;
    else Frames[framepos].MenuHandles.MIBorder=menuid;

    popuppos+=100000;

    mi.pszPopupName=(char *)root;
    mi.popupPosition=frameid;
    mi.position=popuppos++;
    mi.pszName=Translate("&Skinned frame");
    mi.flags=CMIF_CHILDPOPUP|CMIF_CHECKED;
    mi.pszService=MS_CLIST_FRAMES_SETSKINNED;
    mi.pszContactOwner=(char *)0;
    menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
    if (frameid==-1) contMISkinned=menuid;
    else Frames[framepos].MenuHandles.MISkinned=menuid;

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
        if (frameid==-1) contMIAlignRoot=menuid;
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
        if (frameid==-1) contMIAlignTop=menuid;
        else Frames[framepos].MenuHandles.MIAlignTop=menuid;


        //align client
        mi.position=popuppos++;
        mi.pszName=Translate("&Client");
        mi.pszService=CLUIFRAMESSETALIGNALCLIENT;
        mi.pszContactOwner=(char *)alClient;
        menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
        if (frameid==-1) contMIAlignClient=menuid;
        else Frames[framepos].MenuHandles.MIAlignClient=menuid;

        //align bottom
        mi.position=popuppos++;
        mi.pszName=Translate("&Bottom");
        mi.pszService=CLUIFRAMESSETALIGNALBOTTOM;
        mi.pszContactOwner=(char *)alBottom;
        menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);
        if (frameid==-1) contMIAlignBottom=menuid;
        else Frames[framepos].MenuHandles.MIAlignBottom=menuid;

    }

    {   //position
        //position root
        mi.pszPopupName=(char *)root;
        mi.popupPosition=frameid;
        mi.position=popuppos++;
        mi.pszName=Translate("&Position");
        mi.flags=CMIF_CHILDPOPUP|CMIF_ROOTPOPUP;
        mi.pszService="";
        mi.pszContactOwner=(char *)0;
        menuid=(HANDLE)CallService(addservice,0,(LPARAM)&mi);

        mi.pszPopupName=(char *)menuid;
        mi.popupPosition=frameid;
        mi.position=popuppos++;
        mi.pszName=Translate("&Up");
        mi.flags=CMIF_CHILDPOPUP;
        mi.pszService=CLUIFRAMESMOVEUP;
        mi.pszContactOwner=(char *)1;
        CallService(addservice,0,(LPARAM)&mi);

        mi.pszPopupName=(char *)menuid;
        mi.popupPosition=frameid;
        mi.position=popuppos++;
        mi.pszName=Translate("&Down");
        mi.flags=CMIF_CHILDPOPUP;
        mi.pszService=CLUIFRAMESMOVEDOWN;
        mi.pszContactOwner=(char *)-1;
        CallService(addservice,0,(LPARAM)&mi);

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

    if (FramesSysNotStarted) 
		return -1;

    lockfrm();
    pos=id2pos(wParam);

    if (pos>=0&&pos<nFramescount) {
        memset(&mi,0,sizeof(mi));
        mi.cbSize=sizeof(mi);
        mi.flags=CMIM_NAME|CMIF_CHILDPOPUP|CMIF_TCHAR;
        mi.ptszName=Frames[pos].TitleBar.tbname ? Frames[pos].TitleBar.tbname : Frames[pos].name;
        ModifyMItem((WPARAM)contMITitle,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].visible) mi.flags|=CMIF_CHECKED;
        ModifyMItem((WPARAM)contMIVisible,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].Locked) mi.flags|=CMIF_CHECKED;
        ModifyMItem((WPARAM)contMILock,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].TitleBar.ShowTitleBar) mi.flags|=CMIF_CHECKED;
        ModifyMItem((WPARAM)contMITBVisible,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].floating) mi.flags|=CMIF_CHECKED;
        ModifyMItem((WPARAM)contMIFloating,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if ((Frames[pos].UseBorder)) mi.flags|=CMIF_CHECKED;
        ModifyMItem((WPARAM)contMIBorder,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if ((Frames[pos].Skinned)) mi.flags|=CMIF_CHECKED;
        ModifyMItem((WPARAM)contMISkinned,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].align&alTop) mi.flags|=CMIF_CHECKED;
        ModifyMItem((WPARAM)contMIAlignTop,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].align&alClient) mi.flags|=CMIF_CHECKED;
        ModifyMItem((WPARAM)contMIAlignClient,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].align&alBottom) mi.flags|=CMIF_CHECKED;
        ModifyMItem((WPARAM)contMIAlignBottom,(LPARAM)&mi);


        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (!Frames[pos].collapsed) mi.flags|=CMIF_CHECKED;
        if ((!Frames[pos].visible)||(Frames[pos].Locked)||(pos==CLUIFramesGetalClientFrame())) mi.flags|=CMIF_GRAYED;
        ModifyMItem((WPARAM)contMIColl,(LPARAM)&mi);
    }
    ulockfrm();
    return 0;
}

int CLUIFramesModifyMainMenuItems(WPARAM wParam,LPARAM lParam)
{
    int pos;
    CLISTMENUITEM mi;

    if (FramesSysNotStarted) 
		return -1;

    lockfrm();
    pos=id2pos(wParam);

    if (pos>=0&&pos<nFramescount) {
        memset(&mi,0,sizeof(mi));
        mi.cbSize=sizeof(mi);
        mi.flags=CMIM_NAME|CMIF_CHILDPOPUP|CMIF_TCHAR;
        mi.ptszName=Frames[pos].TitleBar.tbname ? Frames[pos].TitleBar.tbname : Frames[pos].name;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MITitle,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].visible) mi.flags|=CMIF_CHECKED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIVisible,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].Locked) mi.flags|=CMIF_CHECKED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MILock,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].TitleBar.ShowTitleBar) mi.flags|=CMIF_CHECKED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MITBVisible,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].floating) mi.flags|=CMIF_CHECKED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIFloating,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if ((Frames[pos].UseBorder)) mi.flags|=CMIF_CHECKED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIBorder,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if ((Frames[pos].Skinned)) mi.flags|=CMIF_CHECKED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MISkinned,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|((Frames[pos].align&alClient)?CMIF_GRAYED:0);
        if (Frames[pos].align&alTop) mi.flags|=CMIF_CHECKED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIAlignTop,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (Frames[pos].align&alClient) mi.flags|=CMIF_CHECKED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIAlignClient,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|((Frames[pos].align&alClient)?CMIF_GRAYED:0);
        if (Frames[pos].align&alBottom) mi.flags|=CMIF_CHECKED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIAlignBottom,(LPARAM)&mi);

        mi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
        if (!Frames[pos].collapsed) mi.flags|=CMIF_CHECKED;
        if ((!Frames[pos].visible)||Frames[pos].Locked||(pos==CLUIFramesGetalClientFrame())) mi.flags|=CMIF_GRAYED;
        CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)Frames[pos].MenuHandles.MIColl,(LPARAM)&mi);
    }
    ulockfrm();
    return 0;
}

int CLUIFramesGetFrameOptions(WPARAM wParam,LPARAM lParam)
{
    int pos;
    int retval;

    if (FramesSysNotStarted) return -1;

    lockfrm();
    pos=id2pos(HIWORD(wParam));
    if (pos<0||pos>=nFramescount) {
        ulockfrm();
        return -1;
    }

    switch (LOWORD(wParam)) {
        case FO_FLAGS:
            retval=0;
            if (Frames[pos].visible) retval|=F_VISIBLE;
            if (!Frames[pos].collapsed) retval|=F_UNCOLLAPSED;
            if (Frames[pos].Locked) retval|=F_LOCKED;
            if (Frames[pos].TitleBar.ShowTitleBar) retval|=F_SHOWTB;
            if (Frames[pos].TitleBar.ShowTitleBarTip) retval|=F_SHOWTBTIP;
            if (Frames[pos].Skinned) retval|=F_SKINNED;
            if (!(GetWindowLong(Frames[pos].hWnd,GWL_STYLE)&WS_BORDER)) retval|=F_NOBORDER;
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

int CLUIFramesSetFrameOptions(WPARAM wParam,LPARAM lParam)
{
    int pos;
    int retval; // value to be returned

    if (FramesSysNotStarted) 
		return -1;

    lockfrm();
    pos=id2pos(HIWORD(wParam));
    if (pos<0||pos>=nFramescount) {
        ulockfrm();
        return -1;
    }

    switch (LOWORD(wParam)) {
        case FO_FLAGS:{
                int flag=lParam;
                int style;

                Frames[pos].dwFlags=flag;
                Frames[pos].visible=FALSE;
                if (flag&F_VISIBLE) Frames[pos].visible=TRUE;

                Frames[pos].collapsed=TRUE;
                if (flag&F_UNCOLLAPSED) Frames[pos].collapsed=FALSE;

                Frames[pos].Locked=FALSE;
                if (flag&F_LOCKED) Frames[pos].Locked=TRUE;

                Frames[pos].UseBorder=TRUE;
                if (flag&F_NOBORDER) Frames[pos].UseBorder=FALSE;

                Frames[pos].TitleBar.ShowTitleBar=FALSE;
                if (flag&F_SHOWTB) Frames[pos].TitleBar.ShowTitleBar=TRUE;

                Frames[pos].TitleBar.ShowTitleBarTip=FALSE;
                if (flag&F_SHOWTBTIP) Frames[pos].TitleBar.ShowTitleBarTip=TRUE;

                SendMessage(Frames[pos].TitleBar.hwndTip,TTM_ACTIVATE,(WPARAM)Frames[pos].TitleBar.ShowTitleBarTip,0);

                style=(int)GetWindowLong(Frames[pos].hWnd,GWL_STYLE);
                style|=WS_BORDER;
                style|=CLS_SKINNEDFRAME;

				if (flag&F_NOBORDER)
                    style&=(~WS_BORDER);

                Frames[pos].Skinned = FALSE;
                if(flag & F_SKINNED)
                    Frames[pos].Skinned = TRUE;

                if(!(flag & F_SKINNED))
                    style &= ~CLS_SKINNEDFRAME;

                SetWindowLong(Frames[pos].hWnd,GWL_STYLE,(LONG)style);
                SetWindowLong(Frames[pos].TitleBar.hwnd,GWL_STYLE,(LONG)style & ~(WS_VSCROLL | WS_HSCROLL));

                ulockfrm();
                CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
                SetWindowPos(Frames[pos].TitleBar.hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
                return 0;
            }

        case FO_NAME:
            if (lParam==(LPARAM)NULL) {
                ulockfrm(); 
				return -1;
            }
            if (Frames[pos].name!=NULL) 
				free(Frames[pos].name);
            Frames[pos].name=mir_tstrdup((LPTSTR)lParam);
            ulockfrm();
            return 0;

        case FO_TBNAME:
            if (lParam==(LPARAM)NULL) {
                ulockfrm(); 
				return(-1);
            }
            if (Frames[pos].TitleBar.tbname!=NULL) 
				free(Frames[pos].TitleBar.tbname);
            Frames[pos].TitleBar.tbname=mir_tstrdup((LPTSTR)lParam);
            ulockfrm();
            if (Frames[pos].floating&&(Frames[pos].TitleBar.tbname!=NULL))
                SetWindowText(Frames[pos].ContainerWnd,Frames[pos].TitleBar.tbname);
            return 0;

        case FO_TBTIPNAME:
            if (lParam==(LPARAM)NULL) {
                ulockfrm(); 
				return(-1);
            }
            if (Frames[pos].TitleBar.tooltip!=NULL) 
				free(Frames[pos].TitleBar.tooltip);
            Frames[pos].TitleBar.tooltip=mir_tstrdup((LPTSTR)lParam);
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
            if (lParam<0) {
                ulockfrm(); 
				return -1;
            }
            retval=Frames[pos].height;
            Frames[pos].height=lParam;
            if (!CLUIFramesFitInSize()) 
				Frames[pos].height=retval;
            retval=Frames[pos].height;
            ulockfrm();
            return retval;

        case FO_FLOATING:
            if (lParam<0) {
                ulockfrm(); 
				return -1;
            }
            {
                int id=Frames[pos].id;
                Frames[pos].floating=!(lParam);
                ulockfrm();

                CLUIFrameSetFloat(id,1);//lparam=1 use stored width and height
                return(wParam);
            }
        case FO_ALIGN:
            if (!(lParam&alTop||lParam&alBottom||lParam&alClient)) {
				ulockfrm();
                return(-1);
            }
            if ((lParam&alClient)&&(CLUIFramesGetalClientFrame()>=0)) {  //only one alClient frame possible
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

static int CLUIFramesShowAll(WPARAM wParam,LPARAM lParam)
{
    int i;

    if (FramesSysNotStarted) return -1;

    for (i=0;i<nFramescount;i++)
        Frames[i].visible=TRUE;
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
    return 0;
}

int CLUIFramesShowAllTitleBars(WPARAM wParam,LPARAM lParam)
{
    int i;

    if (FramesSysNotStarted) return -1;

    for (i=0;i<nFramescount;i++) {
        Frames[i].TitleBar.ShowTitleBar=TRUE;
        SetWindowPos(Frames[i].hWnd, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
    }
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
    RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
    return 0;
}

//wparam=lparam=0
int CLUIFramesHideAllTitleBars(WPARAM wParam,LPARAM lParam)
{
    int i;

    if (FramesSysNotStarted) return -1;

    for (i=0;i<nFramescount;i++) {
        Frames[i].TitleBar.ShowTitleBar=FALSE;
        SetWindowPos(Frames[i].hWnd, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
    }
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
    RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
    return 0;
}

//wparam=frameid
int CLUIFramesShowHideFrame(WPARAM wParam,LPARAM lParam)
{
    int pos;

    if (FramesSysNotStarted) 
		return -1;

    lockfrm();
    pos=id2pos(wParam);
	if(pos>=0 && !lstrcmp(Frames[pos].name, _T("My Contacts")))
		Frames[pos].visible = 1;
	else {
		if (pos>=0&&(int)pos<nFramescount)
			Frames[pos].visible=!Frames[pos].visible;
		if (Frames[pos].floating)
			CLUIFrameResizeFloatingFrame(pos);
	}
    ulockfrm();
    if (!Frames[pos].floating) 
		CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
    RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
    return 0;
}

//wparam=frameid
int CLUIFramesShowHideFrameTitleBar(WPARAM wParam,LPARAM lParam)
{
    int pos;

    if (FramesSysNotStarted) 
		return -1;

    lockfrm();
    pos=id2pos(wParam);
    if (pos>=0&&(int)pos<nFramescount) {
        Frames[pos].TitleBar.ShowTitleBar=!Frames[pos].TitleBar.ShowTitleBar;
        SetWindowPos(Frames[pos].hWnd, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
    }
    ulockfrm();
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
    RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
    return 0;
}


//wparam=frameid
//lparam=-1 up ,1 down
int CLUIFramesMoveUpDown(WPARAM wParam,LPARAM lParam)
{
    int pos,i,curpos,curalign,v,tmpval;

    if (FramesSysNotStarted) 
		return -1;

    lockfrm();
    pos=id2pos(wParam);
    if (pos>=0&&(int)pos<nFramescount) {
        curpos=Frames[pos].order;
        curalign=Frames[pos].align;
        v = 0;
        memset(g_sd, 0, sizeof(SortData) * MAX_FRAMES);
        for (i=0;i<nFramescount;i++) {
            if (Frames[i].floating||(!Frames[i].visible)||(Frames[i].align!=curalign))
                continue;
            g_sd[v].order=Frames[i].order;
            g_sd[v].realpos=i;
            v++;
        }
        if (v==0) {
            ulockfrm();
			return(0);
        }
        qsort(g_sd,v,sizeof(SortData),sortfunc);
        for (i=0;i<v;i++) {
            if (g_sd[i].realpos==pos) {
                if (lParam==-1) {
                    if (i<1) break;
                    tmpval=Frames[g_sd[i-1].realpos].order;
                    Frames[g_sd[i-1].realpos].order=Frames[pos].order;
                    Frames[pos].order=tmpval;
                    break;
                }
                if (lParam==1) {
                    if (i>v-1) break;
                    tmpval=Frames[g_sd[i+1].realpos].order;
                    Frames[g_sd[i+1].realpos].order=Frames[pos].order;
                    Frames[pos].order=tmpval;
                    break;
                }
            }
        }
        ulockfrm();
        CLUIFramesReSort();
        //CLUIFramesStoreAllFrames();
        CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);
		PostMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
    }
    return(0);
}

static int CLUIFramesMoveUp(WPARAM wParam,LPARAM lParam)
{
	return(CLUIFramesMoveUpDown(wParam, -1));
}

static int CLUIFramesMoveDown(WPARAM wParam,LPARAM lParam)
{
    return(CLUIFramesMoveUpDown(wParam, 1));
}


//wparam=frameid
//lparam=alignment
int CLUIFramesSetAlign(WPARAM wParam,LPARAM lParam)
{
    if (FramesSysNotStarted) return -1;

    CLUIFramesSetFrameOptions(MAKEWPARAM(FO_ALIGN,wParam),lParam);
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);
    RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
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

    if (FramesSysNotStarted) 
		return -1;

    lockfrm();
    pos=id2pos(wParam);
    if (pos>=0&&(int)pos<nFramescount) {
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

    if (FramesSysNotStarted) 
		return -1;

    lockfrm();
    FrameId=id2pos(wParam);
    if (FrameId==-1) {
        ulockfrm();
		return(-1);
    }
    flt = oldflags = CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,wParam),0);

	if (oldflags&F_NOBORDER)
        oldflags&=(~F_NOBORDER);
    else
        oldflags|=F_NOBORDER;

    hw = Frames[FrameId].hWnd;
    GetWindowRect(hw,&rc);
    ulockfrm();
    CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,wParam),oldflags);
    SetWindowPos(hw,0,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_DRAWFRAME|SWP_NOZORDER);
    return(0);
}

//wparam=frameid
int CLUIFramesSetUnSetSkinned(WPARAM wParam,LPARAM lParam)
{
    RECT rc;
    int FrameId,oldflags;
    HWND hw;
    boolean flt;

    if (FramesSysNotStarted) 
		return -1;

    lockfrm();
    FrameId=id2pos(wParam);
    if (FrameId==-1) {
        ulockfrm();
		return(-1);
    }
    flt = oldflags = CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,wParam),0);

	if (oldflags & F_SKINNED)
        oldflags &= ~F_SKINNED;
    else
        oldflags |= F_SKINNED;

    hw = Frames[FrameId].hWnd;
    GetWindowRect(hw,&rc);
    ulockfrm();
    CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,wParam),oldflags);
    SetWindowPos(hw,0,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_DRAWFRAME|SWP_NOZORDER);
    return(0);
}

//wparam=frameid
int CLUIFramesCollapseUnCollapseFrame(WPARAM wParam,LPARAM lParam)
{
    int FrameId;

    if (FramesSysNotStarted) 
		return -1;

    TitleBarH = g_CluiData.titleBarHeight;
    lockfrm();
    FrameId=id2pos(wParam);
    if (FrameId>=0&&FrameId<nFramescount) {
        int oldHeight;

        // do not collapse/uncollapse client/locked/invisible frames
        if (Frames[FrameId].align==alClient&&!(Frames[FrameId].Locked||(!Frames[FrameId].visible)||Frames[FrameId].floating)) {
            RECT rc;
            if (CallService(MS_CLIST_DOCKINGISDOCKED,0,0)) {
                ulockfrm();
				return 0;
            }
            if (DBGetContactSettingByte(NULL,"CLUI","AutoSize",0)) {
                ulockfrm();
				return 0;
            }
            GetWindowRect(pcli->hwndContactList,&rc);

            if (Frames[FrameId].collapsed==TRUE) {
                rc.bottom-=rc.top;
                rc.bottom-=Frames[FrameId].height;
                Frames[FrameId].HeightWhenCollapsed=Frames[FrameId].height;
                Frames[FrameId].collapsed=FALSE;
            }
            else {
                rc.bottom-=rc.top;
                rc.bottom+=Frames[FrameId].HeightWhenCollapsed;
                Frames[FrameId].collapsed=TRUE;
            }

            SetWindowPos(pcli->hwndContactList,NULL,0,0,rc.right-rc.left,rc.bottom,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE);

            CLUIFramesStoreAllFrames();
            ulockfrm();
            RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
            return 0;

        }
		if (Frames[FrameId].Locked||(!Frames[FrameId].visible)) {
			ulockfrm();
			return 0;  
		}

        oldHeight=Frames[FrameId].height;

        // if collapsed, uncollapse
        if (Frames[FrameId].collapsed==TRUE) {
            Frames[FrameId].HeightWhenCollapsed=Frames[FrameId].height;
            Frames[FrameId].height=UNCOLLAPSED_FRAME_SIZE;
            Frames[FrameId].collapsed=FALSE;
        }
        // if uncollapsed, collapse
        else {
            Frames[FrameId].height=Frames[FrameId].HeightWhenCollapsed;
            Frames[FrameId].collapsed=TRUE;
        }

        if (!Frames[FrameId].floating) {

            if (!CLUIFramesFitInSize()) {
            //cant collapse,we can resize only for height<alclient frame height
                int alfrm=CLUIFramesGetalClientFrame();

                if (alfrm!=-1) {
                    Frames[FrameId].collapsed=FALSE;
                    if (Frames[alfrm].height>2*UNCOLLAPSED_FRAME_SIZE) {
                        oldHeight=Frames[alfrm].height-UNCOLLAPSED_FRAME_SIZE;
                        Frames[FrameId].collapsed=TRUE;
                    }
                }
                else {
                    int i,sumheight=0;

                    for (i=0;i<nFramescount;i++) {
                        if ((Frames[i].align!=alClient)&&(!Frames[i].floating)&&(Frames[i].visible)&&(!Frames[i].needhide)) {
                            sumheight+=(Frames[i].height)+(TitleBarH*btoint(Frames[i].TitleBar.ShowTitleBar))+2;
							ulockfrm();
                            return FALSE;
                        }
                        if (sumheight>ContactListHeight-0-2)
                            Frames[FrameId].height=(ContactListHeight-0-2)-sumheight;
                    }
                }
                Frames[FrameId].height=oldHeight;
                if (Frames[FrameId].collapsed==FALSE) {
                    if (Frames[FrameId].floating)
                        SetWindowPos(Frames[FrameId].ContainerWnd,HWND_TOP,0,0,Frames[FrameId].wndSize.right-Frames[FrameId].wndSize.left+6,Frames[FrameId].height+DEFAULT_TITLEBAR_HEIGHT+4,SWP_SHOWWINDOW|SWP_NOMOVE);
                    ulockfrm();
					return -1;
                }
            }
        }
        ulockfrm();
        if (!Frames[FrameId].floating)
            CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);
        else {
            RECT contwnd;
            GetWindowRect(Frames[FrameId].ContainerWnd,&contwnd);
            contwnd.top=contwnd.bottom-contwnd.top;//height
            contwnd.left=contwnd.right-contwnd.left;//width

            contwnd.top-=(oldHeight-Frames[FrameId].height);//newheight
            SetWindowPos(Frames[FrameId].ContainerWnd,HWND_TOP,0,0,contwnd.left,contwnd.top,SWP_SHOWWINDOW|SWP_NOMOVE);
        }
        RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
        CLUIFramesStoreAllFrames();
        return(0);
    }
	else {
		ulockfrm();
        return -1;
	}
    ulockfrm();
    return 0;
}

static int CLUIFramesLoadMainMenu()
{
    CLISTMENUITEM mi;
    int i,separator;

    if (FramesSysNotStarted) 
		return -1;

    if (MainMIRoot!=(HANDLE)-1) {
        CallService(MS_CLIST_REMOVEMAINMENUITEM,(WPARAM)MainMIRoot,0);
        MainMIRoot = (HANDLE) -1;
    }

    ZeroMemory(&mi,sizeof(mi));
    mi.cbSize=sizeof(mi);

    // create root menu
    mi.icolibItem=LoadSkinnedIconHandle(SKINICON_OTHER_MIRANDA); //LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_MIRANDA));
    mi.flags=CMIF_ROOTPOPUP|CMIF_ICONFROMICOLIB;
    mi.position=3000090000;
    mi.pszPopupName=(char*)-1;
    mi.pszName=Translate("Frames");
    mi.pszService=0;
    MainMIRoot=(HANDLE)CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);

    // create frames menu
    separator=3000200000;
    for (i=0;i<nFramescount;i++) {
        mi.hIcon=Frames[i].TitleBar.hicon;
        mi.flags=CMIF_CHILDPOPUP|CMIF_ROOTPOPUP|CMIF_TCHAR;
        mi.position=separator;
        mi.pszPopupName=(char*)MainMIRoot;
        mi.ptszName=Frames[i].TitleBar.tbname ? Frames[i].TitleBar.tbname : Frames[i].name;
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
    mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_MIRANDA));
    mi.flags=CMIF_CHILDPOPUP;
    mi.position=separator++;
    mi.pszPopupName=(char*)MainMIRoot;
    mi.pszName=Translate("Show All Frames");
    mi.pszService=MS_CLIST_FRAMES_SHOWALLFRAMES;
    CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);

    // create "show all titlebars" menu
    mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_HELP));
    mi.position=separator++;
    mi.pszPopupName=(char*)MainMIRoot;
    mi.flags=CMIF_CHILDPOPUP;
    mi.pszName=Translate("Show All Titlebars");
    mi.pszService=MS_CLIST_FRAMES_SHOWALLFRAMESTB;
    CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);

    // create "hide all titlebars" menu
    mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_HELP));
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
    LOGFONT logfont;
    memset(&logfont,0,sizeof(logfont));
    memcpy(logfont.lfFaceName,facename,sizeof(facename));
    logfont.lfWeight=FW_NORMAL;
    logfont.lfHeight=-10;
    hfont=CreateFontIndirect(&logfont);
    return((int)hfont);
}

static int UpdateTBToolTip(int framepos)
{
    {
        TOOLINFO ti;

        ZeroMemory(&ti,sizeof(ti));
        ti.cbSize=sizeof(ti);
        ti.lpszText=Frames[framepos].TitleBar.tooltip;
        ti.hinst=g_hInst;
        ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS ;
        ti.uId=(UINT)Frames[framepos].TitleBar.hwnd;

        return(SendMessage(Frames[framepos].TitleBar.hwndTip,TTM_UPDATETIPTEXT,(WPARAM)0,(LPARAM)&ti));
    }

};

int FrameNCPaint(HWND hwnd, WNDPROC oldWndProc, WPARAM wParam, LPARAM lParam, BOOL hasTitleBar)
{
    HDC hdc;
    RECT rcWindow, rc;
    HWND hwndParent = GetParent(hwnd);
    LRESULT result;

    if(hwndParent != pcli->hwndContactList || !g_CluiData.bSkinnedScrollbar)
        result = CallWindowProc(oldWndProc, hwnd, WM_NCPAINT, wParam, lParam);
    else
        result = 0;

    if(pcli && pcli->hwndContactList && GetParent(hwnd) == pcli->hwndContactList) {
        if(GetWindowLong(hwnd, GWL_STYLE) & CLS_SKINNEDFRAME) {
            StatusItems_t *item = StatusItems ? (hasTitleBar ?  &StatusItems[ID_EXTBKOWNEDFRAMEBORDERTB - ID_STATUS_OFFLINE] : &StatusItems[ID_EXTBKOWNEDFRAMEBORDER - ID_STATUS_OFFLINE]) : 0;
            HDC realDC;
            HBITMAP hbmDraw, hbmOld;

            if(item == 0)
                return 0;

            GetWindowRect(hwnd, &rcWindow);
            rc.left = rc.top = 0;
            rc.right = rcWindow.right - rcWindow.left;
            rc.bottom = rcWindow.bottom - rcWindow.top;

            hdc = realDC = GetWindowDC(hwnd);
            if(hwnd == pcli->hwndContactTree) {
                realDC = CreateCompatibleDC(hdc);
                hbmDraw = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
                hbmOld = SelectObject(realDC, hbmDraw);
            }

            ExcludeClipRect(realDC, item->MARGIN_LEFT, item->MARGIN_TOP, rc.right - item->MARGIN_RIGHT, rc.bottom - item->MARGIN_BOTTOM);

            BitBlt(realDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, g_CluiData.hdcBg, rcWindow.left - g_CluiData.ptW.x, rcWindow.top - g_CluiData.ptW.y, SRCCOPY);

            DrawAlpha(realDC, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT, item->GRADIENT,
                      item->CORNER, item->BORDERSTYLE, item->imageItem);
            if(hwnd == pcli->hwndContactTree) {
                ExcludeClipRect(hdc, item->MARGIN_LEFT, item->MARGIN_TOP, rc.right - item->MARGIN_RIGHT, rc.bottom - item->MARGIN_BOTTOM);
                BitBlt(hdc, 0, 0, rc.right, rc.bottom, realDC, 0, 0, SRCCOPY);
                SelectObject(realDC, hbmOld);
                DeleteObject(hbmDraw);
                DeleteDC(realDC);
            }
            ReleaseDC(hwnd, hdc);
            return 0;
        }
        else if(GetWindowLong(hwnd, GWL_STYLE) & WS_BORDER) {
            HPEN hPenOld;
            HBRUSH brold;

            hdc = GetWindowDC(hwnd);
            hPenOld = SelectObject(hdc, g_hPenCLUIFrames);
            GetWindowRect(hwnd, &rcWindow);
            rc.left = rc.top = 0;
            rc.right = rcWindow.right - rcWindow.left;
            rc.bottom = rcWindow.bottom - rcWindow.top;
            brold = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
            Rectangle(hdc, 0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top);
            SelectObject(hdc, hPenOld);
            SelectObject(hdc, brold);
            ReleaseDC(hwnd, hdc);
            return 0;
        }
    }
    return result;
}

int FrameNCCalcSize(HWND hwnd, WNDPROC oldWndProc, WPARAM wParam, LPARAM lParam, BOOL hasTitleBar)
{
    StatusItems_t *item = StatusItems ? (hasTitleBar ?  &StatusItems[ID_EXTBKOWNEDFRAMEBORDERTB - ID_STATUS_OFFLINE] : &StatusItems[ID_EXTBKOWNEDFRAMEBORDER - ID_STATUS_OFFLINE]) : 0;
    LRESULT orig = oldWndProc ? CallWindowProc(oldWndProc, hwnd, WM_NCCALCSIZE, wParam, lParam) : 0;
    NCCALCSIZE_PARAMS *nccp = (NCCALCSIZE_PARAMS *)lParam;
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

    if(item == 0 || pcli == 0)
        return orig;

    if(item->IGNORED || !(dwStyle & CLS_SKINNEDFRAME) || GetParent(hwnd) != pcli->hwndContactList)
        return orig;

    nccp->rgrc[0].left += item->MARGIN_LEFT;
    nccp->rgrc[0].right -= item->MARGIN_RIGHT;
    nccp->rgrc[0].bottom -= item->MARGIN_BOTTOM;
    nccp->rgrc[0].top += item->MARGIN_TOP;
    return WVR_REDRAW;
}

static LRESULT CALLBACK FramesSubClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int i;

    WNDPROC oldWndProc = 0;
    BOOL    hasTitleBar = FALSE;

    for(i = 0; i < nFramescount; i++) {
        if(Frames[i].hWnd == hwnd) {
            oldWndProc = Frames[i].wndProc;
            hasTitleBar = Frames[i].TitleBar.ShowTitleBar;
        }
    }
    switch(msg) {
        case WM_NCPAINT:
            {
                return FrameNCPaint(hwnd, oldWndProc ? oldWndProc : DefWindowProc, wParam, lParam, hasTitleBar);
            }
        case WM_NCCALCSIZE:
            {
                return FrameNCCalcSize(hwnd, oldWndProc, wParam, lParam, hasTitleBar);
            }
        case WM_PRINTCLIENT:
            return 0;
        default:
            break;
    }
    if(oldWndProc)
        return CallWindowProc(oldWndProc, hwnd, msg, wParam, lParam);
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*
 * re-sort all frames and correct frame ordering
 */

static int CLUIFramesReSort()
{
    int v = 0, i;
	int order = 1;

    lockfrm();
	memset(g_sd,0,sizeof(SortData) * MAX_FRAMES);
    for (i=0;i<nFramescount;i++) {
		if(Frames[i].align != alTop)
			continue;
        g_sd[v].order=Frames[i].order;
        g_sd[v].realpos=i;
        v++;
    }
    if (v > 0) {
	    qsort(g_sd, v, sizeof(SortData), sortfunc);
		for(i = 0; i < v; i++)
			Frames[g_sd[i].realpos].order = order++;
    }

	memset(g_sd,0,sizeof(SortData) * MAX_FRAMES);
	v = 0;
    for (i=0;i<nFramescount;i++) {
		if(Frames[i].align != alBottom)
			continue;
        g_sd[v].order=Frames[i].order;
        g_sd[v].realpos=i;
        v++;
    }
    if (v > 0) {
	    qsort(g_sd, v, sizeof(SortData), sortfunc);
		for(i = 0; i < v; i++)
			Frames[g_sd[i].realpos].order = order++;
    }
	CLUIFramesStoreAllFrames();
	ulockfrm();
	return(0);
}

//wparam=(CLISTFrame*)clfrm
int CLUIFramesAddFrame(WPARAM wParam,LPARAM lParam)
{
    int style,retval;
    char * CustomName=NULL;
    CLISTFrame *clfrm=(CLISTFrame *)wParam;

    if (pcli->hwndContactList==0) return -1;
    if (FramesSysNotStarted) return -1;
    if (clfrm->cbSize!=sizeof(CLISTFrame)) return -1;

    lockfrm();
    if (nFramescount>=MAX_FRAMES) {
        ulockfrm(); 
		return -1;
    }
    if(Frames == NULL) {
        Frames=(wndFrame*)malloc(sizeof(wndFrame) * (MAX_FRAMES + 2));
        ZeroMemory(Frames, sizeof(wndFrame) * (MAX_FRAMES + 2));
    }
    memset(&Frames[nFramescount],0,sizeof(wndFrame));

    Frames[nFramescount].id = NextFrameId++;
    Frames[nFramescount].align=clfrm->align;
    Frames[nFramescount].hWnd=clfrm->hWnd;
    Frames[nFramescount].height=clfrm->height;
    Frames[nFramescount].TitleBar.hicon=clfrm->hIcon;
    //Frames[nFramescount].TitleBar.BackColour;
    Frames[nFramescount].floating=FALSE;

    if (clfrm->Flags & F_NO_SUBCONTAINER)
        Frames[nFramescount].OwnerWindow = (HWND)-2;
    else 
        Frames[nFramescount].OwnerWindow = pcli->hwndContactList;

    SetClassLong(clfrm->hWnd, GCL_STYLE, GetClassLong(clfrm->hWnd, GCL_STYLE) & ~(CS_VREDRAW | CS_HREDRAW));
    SetWindowLong(clfrm->hWnd, GWL_STYLE, GetWindowLong(clfrm->hWnd, GWL_STYLE) | WS_CLIPCHILDREN);

	if(GetCurrentThreadId() == GetWindowThreadProcessId(clfrm->hWnd, NULL)) {
		if(clfrm->hWnd != pcli->hwndContactTree && clfrm->hWnd != g_hwndViewModeFrame && clfrm->hWnd != g_hwndEventArea) {
			Frames[nFramescount].wndProc = (WNDPROC)GetWindowLong(clfrm->hWnd, GWL_WNDPROC);
			SetWindowLong(clfrm->hWnd, GWL_WNDPROC, (LONG)FramesSubClassProc);
		}
	}

    if(clfrm->hWnd == g_hwndEventArea)
        wndFrameEventArea = &Frames[nFramescount];
    else if(clfrm->hWnd == pcli->hwndContactTree)
        wndFrameCLC = &Frames[nFramescount];
    else if(clfrm->hWnd == g_hwndViewModeFrame)
        wndFrameViewMode = &Frames[nFramescount];

    Frames[nFramescount].dwFlags=clfrm->Flags;

    if (clfrm->name==NULL||((clfrm->Flags&F_UNICODE) ? lstrlenW(clfrm->wname) : lstrlenA(clfrm->name))==0) {
        Frames[nFramescount].name=(LPTSTR)malloc(255 * sizeof(TCHAR));
        GetClassName(Frames[nFramescount].hWnd,Frames[nFramescount].name,255);
    }
    else
        Frames[nFramescount].name=(clfrm->Flags&F_UNICODE) ? mir_u2t(clfrm->wname) : mir_a2t(clfrm->name);
    if (IsBadCodePtr((FARPROC)clfrm->TBname) || clfrm->TBname==NULL
        || ((clfrm->Flags&F_UNICODE) ? lstrlenW(clfrm->TBwname) : lstrlenA(clfrm->TBname)) == 0)
        Frames[nFramescount].TitleBar.tbname=mir_tstrdup(Frames[nFramescount].name);
    else
        Frames[nFramescount].TitleBar.tbname=(clfrm->Flags&F_UNICODE) ? mir_u2t(clfrm->TBwname) : mir_a2t(clfrm->TBname);
    Frames[nFramescount].needhide=FALSE;
    Frames[nFramescount].TitleBar.ShowTitleBar=(clfrm->Flags&F_SHOWTB?TRUE:FALSE);
    Frames[nFramescount].TitleBar.ShowTitleBarTip=(clfrm->Flags&F_SHOWTBTIP?TRUE:FALSE);

    Frames[nFramescount].collapsed = clfrm->Flags&F_UNCOLLAPSED?FALSE:TRUE;
    Frames[nFramescount].Locked = clfrm->Flags&F_LOCKED?TRUE:FALSE;
    Frames[nFramescount].visible = clfrm->Flags&F_VISIBLE?TRUE:FALSE;

    Frames[nFramescount].UseBorder=(clfrm->Flags&F_NOBORDER)?FALSE:TRUE;
    Frames[nFramescount].Skinned=(clfrm->Flags&F_SKINNED)?TRUE:FALSE;

    // create frame
    Frames[nFramescount].TitleBar.hwnd = 
        CreateWindow(CLUIFrameTitleBarClassName,Frames[nFramescount].name,
                      (DBGetContactSettingByte(0, CLUIFrameModule, "RemoveAllTitleBarBorders", 1) ? 0 : WS_BORDER)
                      | WS_CHILD|WS_CLIPCHILDREN | (Frames[nFramescount].TitleBar.ShowTitleBar?WS_VISIBLE:0) |
                      WS_CLIPCHILDREN, 0, 0, 0, 0, pcli->hwndContactList, NULL, g_hInst, NULL);

    SetWindowLong(Frames[nFramescount].TitleBar.hwnd,GWL_USERDATA,Frames[nFramescount].id);

    Frames[nFramescount].TitleBar.hwndTip = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
															CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT,
															pcli->hwndContactList, NULL, g_hInst, NULL);

    SetWindowPos(Frames[nFramescount].TitleBar.hwndTip, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
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

    SendMessage(Frames[nFramescount].TitleBar.hwndTip,TTM_ACTIVATE,(WPARAM)Frames[nFramescount].TitleBar.ShowTitleBarTip,0);

    Frames[nFramescount].oldstyles=GetWindowLong(Frames[nFramescount].hWnd,GWL_STYLE);
    Frames[nFramescount].TitleBar.oldstyles=GetWindowLong(Frames[nFramescount].TitleBar.hwnd,GWL_STYLE);
        //Frames[nFramescount].FloatingPos.x=

    retval=Frames[nFramescount].id;
    Frames[nFramescount].order=nFramescount+1;
    nFramescount++;

    CLUIFramesLoadFrameSettings(id2pos(retval));
    style=GetWindowLong(Frames[nFramescount-1].hWnd,GWL_STYLE);
    style &= ~(WS_BORDER);
    style|=((Frames[nFramescount-1].UseBorder)?WS_BORDER:0);

    style |= Frames[nFramescount-1].Skinned ? CLS_SKINNEDFRAME : 0;

    SetWindowLong(Frames[nFramescount-1].hWnd,GWL_STYLE,style);
    SetWindowLong(Frames[nFramescount-1].TitleBar.hwnd,GWL_STYLE,style & ~(WS_VSCROLL | WS_HSCROLL));

	if (Frames[nFramescount-1].order==0) {
        Frames[nFramescount-1].order=nFramescount;
    }

	ulockfrm();

    alclientFrame=-1;//recalc it
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);

    if (Frames[nFramescount-1].floating) {
        Frames[nFramescount-1].floating=FALSE;
        CLUIFrameSetFloat(retval,1);//lparam=1 use stored width and height
    }
    RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
    RefreshButtons();
    return retval;
}

static int CLUIFramesRemoveFrame(WPARAM wParam,LPARAM lParam)
{
    int pos;
    if (FramesSysNotStarted) 
		return -1;
    
	lockfrm();
    pos=id2pos(wParam);

    if (pos<0||pos>nFramescount) {
        ulockfrm();
		return(-1);
    }

    if (Frames[pos].name!=NULL) 
		free(Frames[pos].name);
    if (Frames[pos].TitleBar.tbname!=NULL) 
		free(Frames[pos].TitleBar.tbname);
    if (Frames[pos].TitleBar.tooltip!=NULL) 
		free(Frames[pos].TitleBar.tooltip);

    DestroyWindow(Frames[pos].hWnd);
    Frames[pos].hWnd=(HWND)-1;
    DestroyWindow(Frames[pos].TitleBar.hwnd);
    Frames[pos].TitleBar.hwnd=(HWND)-1;
    DestroyWindow(Frames[pos].ContainerWnd);
    Frames[pos].ContainerWnd=(HWND)-1;
    DestroyMenu(Frames[pos].TitleBar.hmenu);

    RemoveItemFromList(pos,&Frames,&nFramescount);

    ulockfrm();
    InvalidateRect(pcli->hwndContactList,NULL,TRUE);
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);
    InvalidateRect(pcli->hwndContactList,NULL,TRUE);
    return(0);
}

int CLUIFramesForceUpdateTB(const wndFrame *Frame)
{
    if (Frame->TitleBar.hwnd!=0) 
		RedrawWindow(Frame->TitleBar.hwnd,NULL,NULL,RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE|RDW_INVALIDATE|RDW_FRAME);
    return 0;
}

int CLUIFramesForceUpdateFrame(const wndFrame *Frame)
{
    if (Frame->hWnd!=0)
        RedrawWindow(Frame->hWnd,NULL,NULL,RDW_UPDATENOW|RDW_FRAME|RDW_ERASE|RDW_INVALIDATE);

    if (Frame->floating)
        if (Frame->ContainerWnd!=0)  RedrawWindow(Frame->ContainerWnd,NULL,NULL,RDW_UPDATENOW|RDW_ALLCHILDREN|RDW_ERASE|RDW_INVALIDATE|RDW_FRAME);
    return 0;
}

int CLUIFrameMoveResize(const wndFrame *Frame)
{
    TitleBarH = g_CluiData.titleBarHeight;
    // we need to show or hide the frame?
    if (Frame->visible&&(!Frame->needhide)) {
        ShowWindow(Frame->hWnd,SW_SHOW);
        ShowWindow(Frame->TitleBar.hwnd,Frame->TitleBar.ShowTitleBar==TRUE?SW_SHOW:SW_HIDE);
    }
    else {
        ShowWindow(Frame->hWnd,SW_HIDE);
        ShowWindow(Frame->TitleBar.hwnd,SW_HIDE);
        return(0);
    }

    SetWindowPos(Frame->hWnd,NULL,Frame->wndSize.left + g_CluiData.bCLeft, Frame->wndSize.top + g_CluiData.topOffset,
                 (Frame->wndSize.right-Frame->wndSize.left),
                 (Frame->wndSize.bottom - Frame->wndSize.top),SWP_NOZORDER|SWP_NOREDRAW);
    if (Frame->TitleBar.ShowTitleBar) {
        SetWindowPos(Frame->TitleBar.hwnd,NULL,Frame->wndSize.left + g_CluiData.bCLeft, Frame->wndSize.top + g_CluiData.topOffset -TitleBarH,
                     (Frame->wndSize.right-Frame->wndSize.left),
                     TitleBarH + (Frame->UseBorder ? (!Frame->collapsed ? (Frame->align == alClient ? 0 : 2) : 1) : 0), SWP_NOZORDER);
    }
    return 0;
}

BOOLEAN CLUIFramesFitInSize(void)
{
    int i;
    int sumheight=0;
    int tbh=0; // title bar height
    int clientfrm;

    TitleBarH = g_CluiData.titleBarHeight;

    clientfrm=CLUIFramesGetalClientFrame();
    if (clientfrm!=-1)
        tbh=TitleBarH*btoint(Frames[clientfrm].TitleBar.ShowTitleBar);

    for (i=0;i<nFramescount;i++) {
        if ((Frames[i].align!=alClient)&&(!Frames[i].floating)&&(Frames[i].visible)&&(!Frames[i].needhide)) {
            sumheight+=(Frames[i].height)+(TitleBarH*btoint(Frames[i].TitleBar.ShowTitleBar))+2/*+btoint(Frames[i].UseBorder)*2*/;
            if (sumheight>ContactListHeight-tbh-2)
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
    
	if (pcli->hwndContactList==NULL) 
		return 0;
    
	lockfrm();

    TitleBarH = g_CluiData.titleBarHeight;
    // search for alClient frame and get the titlebar's height
    tbh=0;
    clientfrm=CLUIFramesGetalClientFrame();
    if (clientfrm!=-1)
        tbh=TitleBarH*btoint(Frames[clientfrm].TitleBar.ShowTitleBar);

    for (i=0;i<nFramescount;i++) {
        if ((Frames[i].align!=alClient)&&(Frames[i].visible)&&(!Frames[i].needhide)&&(!Frames[i].floating)) {
            RECT wsize;

            GetWindowRect(Frames[i].hWnd,&wsize);
            sumheight+=(wsize.bottom-wsize.top)+(TitleBarH*btoint(Frames[i].TitleBar.ShowTitleBar))+3;
        }
    }
    ulockfrm();
    GetBorderSize(pcli->hwndContactList,&border);
    return(sumheight+border.top+border.bottom+allbord+tbh+3);
}

int SizeMoveNewSizes()
{
    int i;
    for (i=0;i<nFramescount;i++) {

        if (Frames[i].floating) {
            CLUIFrameResizeFloatingFrame(i);
        }
        else {
            CLUIFrameMoveResize(&Frames[i]);
        };
    }
    return 0;
}

/*
 * changed Nightwish
 * gap calculation was broken. Now, it doesn't calculate and store the gaps in Frames[] anymore.
 * instead, it remembers the smallest wndSize.top value (which has to be the top frame) and then passes
 * the gap to all following frame(s) to the actual resizing function which just adds the gap to
 * wndSize.top and corrects the frame height accordingly.

 * Title bar gap has been removed (can be simulated by using a clist_nicer skin item for frame title bars
 * and setting the bottom margin of the skin item
 */

int CLUIFramesResize(const RECT newsize)
{
    int sumheight=9999999,newheight;
    int prevframe,prevframebottomline;
    int tbh,curfrmtbh;
    int drawitems;
    int clientfrm, clientframe = -1;
    int i,j;
    int sepw;
    int topOff = 0, botOff = 0, last_bottomtop;;

    GapBetweenFrames=g_CluiData.gapBetweenFrames;
    sepw=GapBetweenFrames;

    if (nFramescount<1 || g_shutDown) 
        return 0;

    newheight=newsize.bottom-newsize.top;
    TitleBarH = g_CluiData.titleBarHeight;

    // search for alClient frame and get the titlebar's height
    tbh=0;
    clientfrm=CLUIFramesGetalClientFrame();
    if (clientfrm!=-1)
        tbh = (TitleBarH)*btoint(Frames[clientfrm].TitleBar.ShowTitleBar);

    for (i=0;i<nFramescount;i++) {
        if (!Frames[i].floating) {
            Frames[i].needhide=FALSE;
            Frames[i].wndSize.left=0;
            Frames[i].wndSize.right=newsize.right-newsize.left;
        }
    }
    {
        //sorting stuff
		memset(g_sd, 0, sizeof(SortData) * MAX_FRAMES);
        for (i=0;i<nFramescount;i++) {
            g_sd[i].order=Frames[i].order;
            g_sd[i].realpos=i;
        }
        qsort(g_sd, nFramescount, sizeof(SortData), sortfunc);

    }
    drawitems=nFramescount;
    while (sumheight>(newheight-tbh)&&drawitems>0) {
        sumheight=0;
        drawitems=0;
        for (i=0;i<nFramescount;i++) {
            if (((Frames[i].align!=alClient))&&(!Frames[i].floating)&&(Frames[i].visible)&&(!Frames[i].needhide)) {
                drawitems++;
                curfrmtbh=(TitleBarH)*btoint(Frames[i].TitleBar.ShowTitleBar);
                sumheight+=(Frames[i].height)+curfrmtbh+(i > 0 ? sepw : 0)+(Frames[i].UseBorder?2:0);
                if (sumheight>newheight-tbh) {
                    sumheight-=(Frames[i].height)+curfrmtbh + (i > 0 ? sepw : 0);
                    Frames[i].needhide=TRUE;
                    drawitems--;
                    break;
                }
            }
        }
    }

    prevframe=-1;
    prevframebottomline=0;
    for (j=0;j<nFramescount;j++) {
        //move all alTop frames
        i = g_sd[j].realpos;
        if ((!Frames[i].needhide)&&(!Frames[i].floating)&&(Frames[i].visible)&&(Frames[i].align==alTop)) {
            curfrmtbh=(TitleBarH)*btoint(Frames[i].TitleBar.ShowTitleBar);
            Frames[i].wndSize.top=prevframebottomline+(prevframebottomline > 0 ? sepw : 0)+(curfrmtbh);
            Frames[i].wndSize.bottom=Frames[i].height+Frames[i].wndSize.top+(Frames[i].UseBorder?2:0);
            Frames[i].prevvisframe=prevframe;
            prevframe=i;
            prevframebottomline=Frames[i].wndSize.bottom;
            topOff = prevframebottomline;
        }
    }

    if (sumheight<newheight) {
        for (j=0;j<nFramescount;j++) {
            //move alClient frame
            i = g_sd[j].realpos;
            if ((!Frames[i].needhide)&&(!Frames[i].floating)&&(Frames[i].visible)&&(Frames[i].align==alClient)) {
                int oldh;
                Frames[i].wndSize.top=prevframebottomline+(prevframebottomline > 0 ? sepw : 0)+(tbh);
                Frames[i].wndSize.bottom=Frames[i].wndSize.top+newheight-sumheight-tbh-((prevframebottomline > 0) ? sepw : 0);
                clientframe = i;
                oldh=Frames[i].height;
                Frames[i].height=Frames[i].wndSize.bottom-Frames[i].wndSize.top;
                Frames[i].prevvisframe=prevframe;
                prevframe=i;
                prevframebottomline=Frames[i].wndSize.bottom;
                if (prevframebottomline>newheight) {
                    //prevframebottomline-=Frames[i].height+(tbh+1);
                    //Frames[i].needhide=TRUE;
                }
                break;
            }
        }
    }

    //newheight
    prevframebottomline = last_bottomtop = newheight;
    //prevframe=-1;
    for (j=nFramescount-1;j>=0;j--) {
        //move all alBottom frames
        i = g_sd[j].realpos;
        if ((Frames[i].visible)&&(!Frames[i].floating)&&(!Frames[i].needhide)&&(Frames[i].align==alBottom)) {
            curfrmtbh=(TitleBarH)*btoint(Frames[i].TitleBar.ShowTitleBar);
            Frames[i].wndSize.bottom=prevframebottomline-((prevframebottomline < newheight) ? sepw : 0);
            Frames[i].wndSize.top=Frames[i].wndSize.bottom-Frames[i].height-(Frames[i].UseBorder?2:0);
            Frames[i].prevvisframe=prevframe;
            prevframe=i;
            prevframebottomline=Frames[i].wndSize.top-curfrmtbh;
            botOff = prevframebottomline;
            last_bottomtop = Frames[i].wndSize.top - curfrmtbh;
        }
    }

    // correct client frame bottom gap if there is no other top frame.

    if (clientframe != -1) {
        Frames[clientframe].wndSize.bottom = last_bottomtop - (last_bottomtop < newheight ? sepw : 0);
        Frames[clientframe].height=Frames[clientframe].wndSize.bottom-Frames[clientframe].wndSize.top;
    }
    return 0;
}

int CLUIFramesUpdateFrame(WPARAM wParam,LPARAM lParam)
{
    if (FramesSysNotStarted) 
		return -1;
    if (wParam==-1) {
        CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0); 
		return 0;
    }
    if (lParam&FU_FMPOS)
        CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,1);
    
	lockfrm();
    wParam=id2pos(wParam);
    if (wParam<0||(int)wParam>=nFramescount) {
        ulockfrm(); 
		return -1;
    }
    if (lParam&FU_TBREDRAW)  
		CLUIFramesForceUpdateTB(&Frames[wParam]);
    if (lParam&FU_FMREDRAW)  
		CLUIFramesForceUpdateFrame(&Frames[wParam]);
    ulockfrm();
    return 0;
}

int dock_prevent_moving=0;

int CLUIFramesApplyNewSizes(int mode)
{
  int i;
  dock_prevent_moving=0;
  for(i=0;i<nFramescount;i++) {
    if ( (mode==1 && Frames[i].OwnerWindow!=(HWND)-2 && Frames[i].OwnerWindow) ||
      (mode==2 && Frames[i].OwnerWindow==(HWND)-2) ||
      (mode==3) )
      if (Frames[i].floating)
        CLUIFrameResizeFloatingFrame(i);
      else
        CLUIFrameMoveResize(&Frames[i]);
  }
  dock_prevent_moving=1;
  return 0;
}

RECT old_window_rect = {0}, new_window_rect = {0};

int SizeFramesByWindowRect(RECT *r)
{
    RECT nRect;
    DWORD noSize = 0;

    if (FramesSysNotStarted)
        return -1;
    
    TitleBarH = g_CluiData.titleBarHeight;
    lockfrm();
    GapBetweenFrames = g_CluiData.gapBetweenFrames;

    nRect = *r;

    nRect.bottom -= (g_CluiData.statusBarHeight + g_CluiData.bottomOffset);
    nRect.right -= g_CluiData.bCRight;
    nRect.left = g_CluiData.bCLeft;
    nRect.top = g_CluiData.topOffset;
    ContactListHeight = nRect.bottom - nRect.top;

    CLUIFramesResize(nRect);
    {
        int i;
        for (i=0;i<nFramescount;i++) {
            int dx;
            int dy;
            dx=new_window_rect.left-old_window_rect.left;
            dy=new_window_rect.top-old_window_rect.top;
            if (!Frames[i].floating) {
                if (Frames[i].OwnerWindow && Frames[i].OwnerWindow != (HWND)-2) {
                    /*
                    if(Frames[i].wndSize.right - Frames[i].wndSize.left == Frames[i].oldWndSize.right - Frames[i].oldWndSize.left && 
                       Frames[i].wndSize.bottom - Frames[i].wndSize.top == Frames[i].oldWndSize.bottom - Frames[i].oldWndSize.top)
                        noSize = SWP_NOSIZE;
                    else {
                        noSize = 0;
                        CopyRect(&Frames[i].oldWndSize, &Frames[i].wndSize);
                    }*/
                    SetWindowPos(Frames[i].hWnd, NULL, Frames[i].wndSize.left + g_CluiData.bCLeft, Frames[i].wndSize.top + g_CluiData.topOffset,
                                 (Frames[i].wndSize.right - Frames[i].wndSize.left),
                                 (Frames[i].wndSize.bottom - Frames[i].wndSize.top), SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW | SWP_NOCOPYBITS | noSize);

                    if (Frames[i].TitleBar.ShowTitleBar) {
                        SetWindowPos(Frames[i].TitleBar.hwnd, NULL, Frames[i].wndSize.left + g_CluiData.bCLeft, Frames[i].wndSize.top + g_CluiData.topOffset - TitleBarH,
                                     (Frames[i].wndSize.right - Frames[i].wndSize.left),
                                     TitleBarH + (Frames[i].UseBorder ? (!Frames[i].collapsed ? (Frames[i].align == alClient ? 0 : 2) : 1) : 0), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOCOPYBITS);
                    }
                }
                else {
                    int res=0;
           // set frame position
                    SetWindowPos(Frames[i].hWnd,NULL,Frames[i].wndSize.left + g_CluiData.bCLeft, Frames[i].wndSize.top + g_CluiData.topOffset,
                                 (Frames[i].wndSize.right - Frames[i].wndSize.left),
                                 (Frames[i].wndSize.bottom-Frames[i].wndSize.top), SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSENDCHANGING | SWP_NOCOPYBITS | SWP_NOREDRAW);

           // set titlebar position
                    if (Frames[i].TitleBar.ShowTitleBar) {
                        SetWindowPos(Frames[i].TitleBar.hwnd, NULL, Frames[i].wndSize.left + g_CluiData.bCLeft, Frames[i].wndSize.top + g_CluiData.topOffset - TitleBarH,
                                     (Frames[i].wndSize.right - Frames[i].wndSize.left),
                                     TitleBarH + (Frames[i].UseBorder ? (!Frames[i].collapsed ? (Frames[i].align == alClient ? 0 : 2) : 1) : 0), SWP_NOZORDER | SWP_NOACTIVATE|SWP_NOCOPYBITS | SWP_NOREDRAW);
                    }
                    //UpdateWindow(Frames[i].hWnd);
                    if (Frames[i].TitleBar.ShowTitleBar)
                        UpdateWindow(Frames[i].TitleBar.hwnd);
                };
            }

        }
        if (GetTickCount()-LastStoreTick>1000) {
            CLUIFramesStoreAllFrames();
            LastStoreTick=GetTickCount();
        }
    }
    ulockfrm();
    return 0;
}

int CLUIFramesOnClistResize(WPARAM wParam,LPARAM lParam)
{
    RECT nRect,rcStatus;
    int tick;
    GapBetweenFrames = g_CluiData.gapBetweenFrames;

    if (FramesSysNotStarted || g_shutDown)
        return -1;
    
	lockfrm();

    GetClientRect(pcli->hwndContactList,&nRect);
    if (lParam && lParam!=1) {
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

    rcStatus.top=rcStatus.bottom=0;

    nRect.bottom -= (g_CluiData.statusBarHeight + g_CluiData.bottomOffset);
    nRect.right -= g_CluiData.bCRight;
    nRect.left = g_CluiData.bCLeft;
    nRect.top = g_CluiData.topOffset;
    ContactListHeight = nRect.bottom - nRect.top;

    tick=GetTickCount();

    CLUIFramesResize(nRect);
    CLUIFramesApplyNewSizes(3);

    ulockfrm();
    tick=GetTickCount()-tick;

    if (pcli->hwndContactList!=0) 
		InvalidateRect(pcli->hwndContactList,NULL,TRUE);
    if (pcli->hwndContactList!=0) 
		UpdateWindow(pcli->hwndContactList);

    Sleep(0);

    if (GetTickCount()-LastStoreTick>2000) {
        CLUIFramesStoreAllFrames();
		LastStoreTick=GetTickCount();
    }
    return 0;
}

static  HBITMAP hBmpBackground;
static int backgroundBmpUse;
static COLORREF bkColour;
static COLORREF SelBkColour;
boolean AlignCOLLIconToLeft; //will hide frame icon

int OnFrameTitleBarBackgroundChange()
{
	DBVARIANT dbv;

	AlignCOLLIconToLeft=DBGetContactSettingByte(NULL,"FrameTitleBar","AlignCOLLIconToLeft",0);

	bkColour=DBGetContactSettingDword(NULL,"FrameTitleBar","BkColour",CLCDEFAULT_BKCOLOUR);
	//SelBkColour=DBGetContactSettingDword(NULL,"FrameTitleBar","SelBkColour",0);

	if (hBmpBackground) {
		DeleteObject(hBmpBackground); hBmpBackground=NULL;
	}
	if (DBGetContactSettingByte(NULL,"FrameTitleBar","UseBitmap",CLCDEFAULT_USEBITMAP)) {
		if (!DBGetContactSetting(NULL,"FrameTitleBar","BkBitmap",&dbv)) {
			hBmpBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
			mir_free(dbv.pszVal);
		}
	}
	backgroundBmpUse=DBGetContactSettingWord(NULL,"FrameTitleBar","BkBmpUse",CLCDEFAULT_BKBMPUSE);

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
static int DrawTitleBar(HDC dc,RECT rect,int Frameid)
{
    HDC hdcMem;
    HBITMAP hBmpOsb,hoBmp;
    HBRUSH hBack,hoBrush;
    int pos;
    StatusItems_t *item = &StatusItems[ID_EXTBKFRAMETITLE - ID_STATUS_OFFLINE];

    TitleBarH = g_CluiData.titleBarHeight;

    hdcMem=CreateCompatibleDC(dc);
    hBmpOsb = CreateCompatibleBitmap(dc, rect.right, rect.bottom);
    hoBmp=SelectObject(hdcMem,hBmpOsb);

    SetBkMode(hdcMem,TRANSPARENT);

    hBack=GetSysColorBrush(COLOR_3DFACE);
    hoBrush=SelectObject(hdcMem,hBack);

    lockfrm();
    pos=id2pos(Frameid);

    if (pos>=0&&pos<nFramescount) {
        HFONT oFont;
        int fHeight, fontTop;
        GetClientRect(Frames[pos].TitleBar.hwnd,&Frames[pos].TitleBar.wndSize);

        if (g_clcData) {
            oFont = ChangeToFont(hdcMem, g_clcData, FONTID_FRAMETITLE, &fHeight);
        }
        else {
            oFont = SelectObject(hdcMem, GetStockObject(DEFAULT_GUI_FONT));
            fHeight = 10;
        }
        fontTop = (TitleBarH - fHeight) / 2;

        if(g_CluiData.bWallpaperMode && !Frames[pos].floating)
            SkinDrawBg(Frames[pos].TitleBar.hwnd, hdcMem);

        if (!item->IGNORED && pDrawAlpha != NULL) {
            RECT rc = Frames[pos].TitleBar.wndSize;
            rc.top += item->MARGIN_TOP; rc.bottom -= item->MARGIN_BOTTOM;
            rc.left += item->MARGIN_LEFT; rc.right -= item->MARGIN_RIGHT;
            DrawAlpha(hdcMem, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
                      item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
            SetTextColor(hdcMem, item->TEXTCOLOR);
        }
        else if (g_clcData) {
            FillRect(hdcMem,&rect,hBack);
            SetTextColor(hdcMem, g_clcData->fontInfo[FONTID_FRAMETITLE].colour);
        }
        else {
            FillRect(hdcMem,&rect,hBack);
            SetTextColor(hdcMem, GetSysColor(COLOR_BTNTEXT));
        }


        if (!AlignCOLLIconToLeft) {
            if (Frames[pos].TitleBar.hicon != NULL) {
                DrawIconEx(hdcMem, 6 + g_CluiData.bClipBorder, ((TitleBarH>>1) - 8), Frames[pos].TitleBar.hicon, 16, 16, 0, NULL, DI_NORMAL);
                TextOut(hdcMem, 24 + g_CluiData.bClipBorder, fontTop, Frames[pos].TitleBar.tbname, lstrlen(Frames[pos].TitleBar.tbname));
            }
            else
                TextOut(hdcMem, 6 + g_CluiData.bClipBorder, fontTop, Frames[pos].TitleBar.tbname,lstrlen(Frames[pos].TitleBar.tbname));
        }
        else
            TextOut(hdcMem, 18 + g_CluiData.bClipBorder, fontTop, Frames[pos].TitleBar.tbname,lstrlen(Frames[pos].TitleBar.tbname));


        if (!AlignCOLLIconToLeft)
            DrawIconEx(hdcMem, Frames[pos].TitleBar.wndSize.right - 22, ((TitleBarH>>1) - 8), Frames[pos].collapsed?LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN):LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT), 16, 16, 0, NULL,DI_NORMAL);
        else
            DrawIconEx(hdcMem, 0, ((TitleBarH>>1)-8), Frames[pos].collapsed ? LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN):LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT),16,16,0,NULL,DI_NORMAL);
        SelectObject(hdcMem, oFont);
    }
    ulockfrm();
    BitBlt(dc,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,hdcMem,rect.left,rect.top,SRCCOPY);
    SelectObject(hdcMem,hoBmp);
    SelectObject(hdcMem,hoBrush);
    DeleteDC(hdcMem);
    DeleteObject(hBack);
    DeleteObject(hBmpOsb);
    return 0;
}

#define MPCF_CONTEXTFRAMEMENU		3
POINT ptOld;
short   nLeft           = 0;
short   nTop            = 0;

LRESULT CALLBACK CLUIFrameTitleBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    int Frameid,Framemod,direction;
    int xpos,ypos;

    Frameid=(GetWindowLong(hwnd,GWL_USERDATA));
    memset(&rect,0,sizeof(rect));

    switch (msg) {
        case WM_CREATE:
            return FALSE;
        case WM_MEASUREITEM:
            return CallService(MS_CLIST_MENUMEASUREITEM,wParam,lParam);
        case WM_DRAWITEM:
            return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);

        case WM_ENABLE:
            if (hwnd!=0) InvalidateRect(hwnd,NULL,FALSE);
            return 0;
        case WM_SIZE:
            return 0;
    /*
        case WM_PRINT:
        case WM_PRINTCLIENT:
            InvalidateRect(hwnd,NULL,FALSE);
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
            if (ServiceExists(MO_CREATENEWMENUOBJECT)) {
                if (ProcessCommandProxy(MAKEWPARAM(LOWORD(wParam),0),(LPARAM)Frameid) )
                    break;
            }
            else {
                if (CallService(MS_CLIST_MENUPROCESSCOMMAND,MAKEWPARAM(LOWORD(wParam),MPCF_CONTEXTFRAMEMENU),(LPARAM)Frameid) )
                    break;
            }

            if (HIWORD(wParam)==0) {//mouse events for self created menu
                int framepos=id2pos(Frameid);
                if (framepos==-1)
                    break;

                switch (LOWORD(wParam)) {
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

                if (ServiceExists(MS_CLIST_MENUBUILDFRAMECONTEXT)) {
                    hmenu=(HMENU)CallService(MS_CLIST_MENUBUILDFRAMECONTEXT,Frameid,0);
                }
                else {
                    int framepos=id2pos(Frameid);
                    
					lockfrm();
                    if (framepos==-1) {
                        ulockfrm();
						break;
                    }
                    hmenu=CreatePopupMenu();
                    AppendMenu(hmenu,MF_STRING|MF_DISABLED|MF_GRAYED,15,Frames[framepos].name);
                    AppendMenu(hmenu,MF_SEPARATOR,16,_T(""));

                    if (Frames[framepos].Locked)
                        AppendMenu(hmenu,MF_STRING|MF_CHECKED,frame_menu_lock,TranslateT("Lock Frame"));
                    else
                        AppendMenu(hmenu,MF_STRING,frame_menu_lock,TranslateT("Lock Frame"));

                    if (Frames[framepos].visible)
                        AppendMenu(hmenu,MF_STRING|MF_CHECKED,frame_menu_visible,TranslateT("Visible"));
                    else
                        AppendMenu(hmenu,MF_STRING,frame_menu_visible,TranslateT("Visible") );

                    if (Frames[framepos].TitleBar.ShowTitleBar)
                        AppendMenu(hmenu,MF_STRING|MF_CHECKED,frame_menu_showtitlebar,TranslateT("Show TitleBar") );
                    else
                        AppendMenu(hmenu,MF_STRING,frame_menu_showtitlebar,TranslateT("Show TitleBar") );

                    if (Frames[framepos].Skinned)
                        AppendMenu(hmenu,MF_STRING|MF_CHECKED,frame_menu_skinned,TranslateT("Skinned frame") );
                    else
                        AppendMenu(hmenu,MF_STRING,frame_menu_skinned,TranslateT("Skinned frame") );

                    if (Frames[framepos].floating)
                        AppendMenu(hmenu,MF_STRING|MF_CHECKED,frame_menu_floating,TranslateT("Floating") );
                    else
                        AppendMenu(hmenu,MF_STRING,frame_menu_floating,TranslateT("Floating") );

                    ulockfrm();
                }
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
                if (GetCapture()!=hwnd) {
                    break;
                };
                curdragbar=-1;lbypos=-1;oldframeheight=-1;ReleaseCapture();
                RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
                break;
            };
        case WM_LBUTTONDOWN:
            {

                int framepos=id2pos(Frameid);
                lockfrm();
                if (framepos==-1) {
                    ulockfrm();
                    break;
                }
                if (Frames[framepos].floating) {

                    POINT pt;
                    GetCursorPos(&pt);
                    Frames[framepos].TitleBar.oldpos=pt;
                }

                if ((!(wParam&MK_CONTROL))&&Frames[framepos].Locked&&(!(Frames[framepos].floating))) {
                    if (DBGetContactSettingByte(NULL,"CLUI","ClientAreaDrag",0)) {
                        POINT pt;
                        GetCursorPos(&pt);
						ulockfrm();
                        return SendMessage(GetParent(hwnd), WM_SYSCOMMAND, SC_MOVE|HTCAPTION,MAKELPARAM(pt.x,pt.y));
                    }
                }
                if (Frames[framepos].floating) {
                    RECT rc;
                    GetCursorPos(&ptOld);
                    GetWindowRect( hwnd, &rc );
                    nLeft   = (short)rc.left;
                    nTop    = (short)rc.top;
                }
                ulockfrm();
                SetCapture(hwnd);
                break;
            }
        case WM_MOUSEMOVE:
            {
                POINT pt,pt2;
                RECT wndr;
                int pos;
                {
                    char TBcapt[255];

                    lockfrm();
                    pos=id2pos(Frameid);

                    if (pos!=-1) {
                        int oldflags;
                        wsprintfA(TBcapt,"%s - h:%d, vis:%d, fl:%d, fl:(%d,%d,%d,%d),or: %d",
                                 Frames[pos].name,Frames[pos].height,Frames[pos].visible,Frames[pos].floating,
                                 Frames[pos].FloatingPos.x,Frames[pos].FloatingPos.y,
                                 Frames[pos].FloatingSize.x,Frames[pos].FloatingSize.y,
                                 Frames[pos].order
                                );

                        oldflags=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,Frames[pos].id),(LPARAM)0);
                        if (!(oldflags&F_SHOWTBTIP))
                            oldflags|=F_SHOWTBTIP;
                    }
                    ulockfrm();
                }
                if ((wParam&MK_LBUTTON)/*&&(wParam&MK_CONTROL)*/) {
                    RECT rcMiranda;
                    RECT rcwnd,rcOverlap;
                    POINT newpt,ofspt,curpt,newpos;

					lockfrm();

					pos=id2pos(Frameid);
                    if (Frames[pos].floating) {
                        GetCursorPos(&curpt);
                        rcwnd.bottom=curpt.y+5;
                        rcwnd.top=curpt.y;
                        rcwnd.left=curpt.x;
                        rcwnd.right=curpt.x+5;

                        GetWindowRect(pcli->hwndContactList, &rcMiranda );
                        if (IsWindowVisible(pcli->hwndContactList) &&IntersectRect( &rcOverlap, &rcwnd, &rcMiranda )) {
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
                        }
                    }
                    else {
                        int id=Frames[pos].id;

                        GetCursorPos(&curpt);
                        rcwnd.bottom=curpt.y+5;
                        rcwnd.top=curpt.y;
                        rcwnd.left=curpt.x;
                        rcwnd.right=curpt.x+5;

                        GetWindowRect(pcli->hwndContactList, &rcMiranda );

                        if (!IntersectRect( &rcOverlap, &rcwnd, &rcMiranda ) ) {
                            ulockfrm();
                            GetCursorPos(&curpt);
                            GetWindowRect( Frames[pos].hWnd, &rcwnd );
                            rcwnd.left=rcwnd.right-rcwnd.left;
                            rcwnd.top=rcwnd.bottom-rcwnd.top;
                            newpos.x=curpt.x;newpos.y=curpt.y;
                            if (curpt.x>=(rcMiranda.right-1)) {
                                newpos.x=curpt.x+5;
                            }
                            if (curpt.x<=(rcMiranda.left+1)) {
                                newpos.x=curpt.x-(rcwnd.left)-5;
                            }
                            if (curpt.y>=(rcMiranda.bottom-1)) {
                                newpos.y=curpt.y+5;
                            }
                            if (curpt.y<=(rcMiranda.top+1)) {
                                newpos.y=curpt.y-(rcwnd.top)-5;
                            };
                            ofspt.x=0;ofspt.y=0;
                            GetWindowRect(Frames[pos].TitleBar.hwnd,&rcwnd);
                            ofspt.x=curpt.x-ofspt.x;ofspt.y=curpt.y-ofspt.y;
                            Frames[pos].FloatingPos.x=newpos.x;
                            Frames[pos].FloatingPos.y=newpos.y;
                            CLUIFrameSetFloat(id,0);
                            lockfrm();
                            newpt.x=0;newpt.y=0;
                            ClientToScreen(Frames[pos].TitleBar.hwnd,&newpt);
                            GetWindowRect( Frames[pos].hWnd, &rcwnd );
                            SetCursorPos(newpt.x+(rcwnd.right-rcwnd.left)/2,newpt.y+(rcwnd.bottom-rcwnd.top)/2);
                            GetCursorPos(&curpt);
                            Frames[pos].TitleBar.oldpos=curpt;
                            ulockfrm();
                            return(0);
                        }
                    }
                    ulockfrm();
				}
                if (wParam&MK_LBUTTON) {
                    int newh=-1,prevold;

                    if (GetCapture()!=hwnd)
                        break;

                    lockfrm();
                    pos=id2pos(Frameid);

                    if (Frames[pos].floating) {
                        GetCursorPos(&pt);
                        if ((Frames[pos].TitleBar.oldpos.x!=pt.x)||(Frames[pos].TitleBar.oldpos.y!=pt.y)) {

                            pt2=pt;
                            ScreenToClient(hwnd,&pt2);
                            GetWindowRect(Frames[pos].ContainerWnd,&wndr);
                            {
                                int dX,dY;
                                POINT ptNew;

                                ptNew.x = pt.x;
                                ptNew.y = pt.y;

                                dX = ptNew.x - ptOld.x;
                                dY = ptNew.y - ptOld.y;

                                nLeft   += (short)dX;
                                nTop    += (short)dY;

                                if (!(wParam&MK_CONTROL)) {
                                    PositionThumb( &Frames[pos], nLeft, nTop );
                                }
                                else {

                                    SetWindowPos(   Frames[pos].ContainerWnd,
                                                    0,
                                                    nLeft,
                                                    nTop,
                                                    0,
                                                    0,
                                                    SWP_NOSIZE | SWP_NOZORDER );
                                }
                                ptOld = ptNew;
                            }
                            pt.x=nLeft;
                            pt.y=nTop;
                            Frames[pos].TitleBar.oldpos=pt;
                        }
                        ulockfrm();
                        return(0);
                    }
                    if (Frames[pos].prevvisframe!=-1) {
                        GetCursorPos(&pt);

                        if ((Frames[pos].TitleBar.oldpos.x==pt.x)&&(Frames[pos].TitleBar.oldpos.y==pt.y)) {
                            ulockfrm();
							break;
                        }

                        ypos=rect.top+pt.y;xpos=rect.left+pt.x;
                        Framemod=-1;

                        if (Frames[pos].align==alBottom) {
                            direction=-1;
                            Framemod=pos;
                        }
                        else {
                            direction=1;
                            Framemod=Frames[pos].prevvisframe;
                        }
                        if (Frames[Framemod].Locked) {
                            ulockfrm();
							break;
                        }
                        if (curdragbar!=-1&&curdragbar!=pos) {
                            ulockfrm();
							break;
                        }

                        if (lbypos==-1) {
                            curdragbar=pos;
                            lbypos=ypos;
                            oldframeheight=Frames[Framemod].height;
                            SetCapture(hwnd);
                            ulockfrm();
							break;;
                        }
                        newh=oldframeheight+direction*(ypos-lbypos);
                        if (newh>0) {
                            prevold=Frames[Framemod].height;
                            Frames[Framemod].height=newh;
                            if (!CLUIFramesFitInSize()) {
                                Frames[Framemod].height=prevold; ulockfrm();
								return TRUE;
                            }
                            Frames[Framemod].height=newh;
                            if (newh>3) Frames[Framemod].collapsed=TRUE;

                        }
                        Frames[pos].TitleBar.oldpos=pt;
                    }
                    ulockfrm();
                    if (newh>0) {
                        CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
                    }
                    break;
                }
                curdragbar=-1;lbypos=-1;oldframeheight=-1;ReleaseCapture();
            }
            break;
        case WM_NCPAINT:
            {
				if(GetWindowLong(hwnd, GWL_STYLE) & WS_BORDER) {
					HDC hdc = GetWindowDC(hwnd);
					HPEN hPenOld = SelectObject(hdc, g_hPenCLUIFrames);
					RECT rcWindow, rc;
					HBRUSH brold;

					CallWindowProc(DefWindowProc, hwnd, msg, wParam, lParam);					
					GetWindowRect(hwnd, &rcWindow);
					rc.left = rc.top = 0;
					rc.right = rcWindow.right - rcWindow.left;
					rc.bottom = rcWindow.bottom - rcWindow.top;
					brold = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
					Rectangle(hdc, 0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top);
					SelectObject(hdc, hPenOld);
					SelectObject(hdc, brold);
					ReleaseDC(hwnd, hdc);
					return 0;
				}
				break;
            }
        case WM_PRINT:
        case WM_PRINTCLIENT:
            {
                GetClientRect(hwnd,&rect);
                DrawTitleBar((HDC)wParam,rect,Frameid);
            }
        case WM_PAINT:
            {
                HDC paintDC;
                PAINTSTRUCT paintStruct;

                paintDC = BeginPaint(hwnd, &paintStruct);
                rect=paintStruct.rcPaint;
                DrawTitleBar(paintDC, rect, Frameid);
                EndPaint(hwnd, &paintStruct);
                return 0;
            }
        default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return TRUE;
}
int CLUIFrameResizeFloatingFrame(int framepos)
{

    int width,height;
    RECT rect;

    if (!Frames[framepos].floating) {
        return(0);
    };
    if (Frames[framepos].ContainerWnd==0) {
        return(0);
    };
    GetClientRect(Frames[framepos].ContainerWnd,&rect);

    width=rect.right-rect.left;
    height=rect.bottom-rect.top;

    Frames[framepos].visible?ShowWindow(Frames[framepos].ContainerWnd,SW_SHOWNOACTIVATE):ShowWindow(Frames[framepos].ContainerWnd,SW_HIDE);



    if (Frames[framepos].TitleBar.ShowTitleBar) {
        ShowWindow(Frames[framepos].TitleBar.hwnd,SW_SHOWNOACTIVATE);
        Frames[framepos].height=height-DEFAULT_TITLEBAR_HEIGHT;
        SetWindowPos(Frames[framepos].TitleBar.hwnd,HWND_TOP,0,0,width,DEFAULT_TITLEBAR_HEIGHT,SWP_SHOWWINDOW|SWP_DRAWFRAME|SWP_NOACTIVATE);
        InvalidateRect(Frames[framepos].TitleBar.hwnd, NULL, FALSE);
        SetWindowPos(Frames[framepos].hWnd,HWND_TOP,0,DEFAULT_TITLEBAR_HEIGHT,width,height-DEFAULT_TITLEBAR_HEIGHT,SWP_SHOWWINDOW|SWP_NOACTIVATE);

    }
    else {
        Frames[framepos].height=height;
        ShowWindow(Frames[framepos].TitleBar.hwnd,SW_HIDE);
        SetWindowPos(Frames[framepos].hWnd,HWND_TOP,0,0,width,height,SWP_SHOWWINDOW|SWP_NOACTIVATE);

    }
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
    switch (msg) {
        case WM_CREATE:
            {
                int framepos;
                lockfrm();
                framepos=id2pos(Frameid);
                ulockfrm();
                return(0);
            }
        case WM_GETMINMAXINFO:
            {
                int framepos;
                MINMAXINFO minmax;

                TitleBarH = g_CluiData.titleBarHeight;
                lockfrm();

                framepos=id2pos(Frameid);
                if (framepos<0||framepos>=nFramescount) {
                    ulockfrm();
					break;
                }
                if (!Frames[framepos].minmaxenabled) {
                    ulockfrm();
					break;
                }
                if (Frames[framepos].ContainerWnd==0) {
                    ulockfrm();
					break;
                }
                if (Frames[framepos].Locked) {
                    RECT rct;

                    GetWindowRect(hwnd,&rct);
                    ((LPMINMAXINFO)lParam)->ptMinTrackSize.x=rct.right-rct.left;
                    ((LPMINMAXINFO)lParam)->ptMinTrackSize.y=rct.bottom-rct.top;
                    ((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=rct.right-rct.left;
                    ((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=rct.bottom-rct.top;
                }

                memset(&minmax,0,sizeof(minmax));
                if (SendMessage(Frames[framepos].hWnd,WM_GETMINMAXINFO,(WPARAM)0,(LPARAM)&minmax)==0) {
                    RECT border;
                    int tbh=TitleBarH*btoint(Frames[framepos].TitleBar.ShowTitleBar);
                    GetBorderSize(hwnd,&border);
                    if (minmax.ptMaxTrackSize.x!=0&&minmax.ptMaxTrackSize.y!=0) {

                        ((LPMINMAXINFO)lParam)->ptMinTrackSize.x=minmax.ptMinTrackSize.x;
                        ((LPMINMAXINFO)lParam)->ptMinTrackSize.y=minmax.ptMinTrackSize.y;
                        ((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=minmax.ptMaxTrackSize.x+border.left+border.right;
                        ((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=minmax.ptMaxTrackSize.y+tbh+border.top+border.bottom;
                    }
                }
                else {
                    ulockfrm();
                    return(DefWindowProc(hwnd, msg, wParam, lParam));
                }
                ulockfrm();
            }
        case WM_MOVE:
            {
                int framepos;
                RECT rect;

                lockfrm();
                framepos=id2pos(Frameid);

                if (framepos<0||framepos>=nFramescount) {
                    ulockfrm();
					break;
                }
                if (Frames[framepos].ContainerWnd==0) {
                    ulockfrm();
					return(0);
                }
                GetWindowRect(Frames[framepos].ContainerWnd,&rect);
                Frames[framepos].FloatingPos.x=rect.left;
                Frames[framepos].FloatingPos.y=rect.top;
                Frames[framepos].FloatingSize.x=rect.right-rect.left;
                Frames[framepos].FloatingSize.y=rect.bottom-rect.top;
                CLUIFramesStoreFrameSettings(framepos);
                ulockfrm();
                return(0);
            }
        case WM_SIZE:
            {
                int framepos;
                RECT rect;
                lockfrm();
             
				framepos=id2pos(Frameid);

                if (framepos<0||framepos>=nFramescount) {
                    ulockfrm();
					break;
                }
                if (Frames[framepos].ContainerWnd==0) {
                    ulockfrm();
					return(0);
                }
                CLUIFrameResizeFloatingFrame(framepos);

                GetWindowRect(Frames[framepos].ContainerWnd,&rect);
                Frames[framepos].FloatingPos.x=rect.left;
                Frames[framepos].FloatingPos.y=rect.top;
                Frames[framepos].FloatingSize.x=rect.right-rect.left;
                Frames[framepos].FloatingSize.y=rect.bottom-rect.top;

                CLUIFramesStoreFrameSettings(framepos);
                ulockfrm();
                return(0);
            }
        case WM_CLOSE:
            {
                DestroyWindow(hwnd);
                break;
            }
        case WM_DESTROY:
            {
                return(0);
            }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static HWND CreateContainerWindow(HWND parent,int x,int y,int width,int height)
{
    return(CreateWindowA("FramesContainer","aaaa",WS_POPUP|WS_THICKFRAME,x,y,width,height,parent,0,g_hInst,0));
}


int CLUIFrameSetFloat(WPARAM wParam,LPARAM lParam)
{
    int hwndtmp,hwndtooltiptmp;

    lockfrm();
    wParam=id2pos(wParam);
    if (wParam>=0&&(int)wParam<nFramescount)
        if (Frames[wParam].floating) {
            SetParent(Frames[wParam].hWnd,pcli->hwndContactList);
            SetParent(Frames[wParam].TitleBar.hwnd,pcli->hwndContactList);
            Frames[wParam].floating=FALSE;
            DestroyWindow(Frames[wParam].ContainerWnd);
            Frames[wParam].ContainerWnd=0;
        }
        else {
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
            if (!Frames[wParam].TitleBar.ShowTitleBar) {
                recttb.top=recttb.bottom=recttb.left=recttb.right=0;
            }
            Frames[wParam].ContainerWnd=CreateContainerWindow(pcli->hwndContactList,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,10,10);

            SetParent(Frames[wParam].hWnd,Frames[wParam].ContainerWnd);
            SetParent(Frames[wParam].TitleBar.hwnd,Frames[wParam].ContainerWnd);

            GetBorderSize(Frames[wParam].ContainerWnd,&border);

            SetWindowLong(Frames[wParam].ContainerWnd,GWL_USERDATA,Frames[wParam].id);
            if ((lParam==1)) {
                if ((Frames[wParam].FloatingPos.x!=0)&&(Frames[wParam].FloatingPos.y!=0)) {
                    if (Frames[wParam].FloatingPos.x<20) {
                        Frames[wParam].FloatingPos.x=40;
                    }
                    if (Frames[wParam].FloatingPos.y<20) {
                        Frames[wParam].FloatingPos.y=40;
                    }
                    SetWindowPos(Frames[wParam].ContainerWnd,HWND_TOPMOST,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,Frames[wParam].FloatingSize.x,Frames[wParam].FloatingSize.y,SWP_HIDEWINDOW);
                }
                else {
                    SetWindowPos(Frames[wParam].ContainerWnd,HWND_TOPMOST,120,120,140,140,SWP_HIDEWINDOW);
                }
            }
            else {
                neww=rectw.right-rectw.left+border.left+border.right;
                newh=(rectw.bottom-rectw.top)+(recttb.bottom-recttb.top)+border.top+border.bottom;
                if (neww<20) {
                    neww=40;
                }
                if (newh<20) {
                    newh=40;
                }
                if (Frames[wParam].FloatingPos.x<20) {
                    Frames[wParam].FloatingPos.x=40;
                }
                if (Frames[wParam].FloatingPos.y<20) {
                    Frames[wParam].FloatingPos.y=40;
                }
                SetWindowPos(Frames[wParam].ContainerWnd,HWND_TOPMOST,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,neww,newh,SWP_HIDEWINDOW);
            }
            SetWindowText(Frames[wParam].ContainerWnd,Frames[wParam].TitleBar.tbname);
            temp=GetWindowLong(Frames[wParam].ContainerWnd,GWL_EXSTYLE);
            temp|=WS_EX_TOOLWINDOW|WS_EX_TOPMOST ;
            SetWindowLong(Frames[wParam].ContainerWnd,GWL_EXSTYLE,temp);
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
    SetWindowPos((HWND)hwndtooltiptmp, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE  );
    return 0;
}

TCHAR g_ptszEventName[100];

static int CLUIFrameOnModulesLoad(WPARAM wParam,LPARAM lParam)
{
    DWORD dwThreadID;

    mir_sntprintf(g_ptszEventName, SIZEOF(g_ptszEventName), _T("mf_update_evt_%d"), GetCurrentThreadId());
    g_hEventThread = CreateEvent(NULL, TRUE, FALSE, g_ptszEventName);
    hThreadMFUpdate = CreateThread(NULL, 16000, MF_UpdateThread, 0, 0, &dwThreadID);
    SetThreadPriority(hThreadMFUpdate, THREAD_PRIORITY_IDLE);
    CLUIFramesLoadMainMenu(0,0);
    CLUIFramesCreateMenuForFrame(-1,-1,000010000,MS_CLIST_ADDCONTEXTFRAMEMENUITEM);
    return 0;
}

static int CLUIFrameOnModulesUnload(WPARAM wParam,LPARAM lParam)
{
    mf_updatethread_running = FALSE;

    SetEvent(g_hEventThread);
    WaitForSingleObject(hThreadMFUpdate, 10000);
    CloseHandle(hThreadMFUpdate);
    CloseHandle(g_hEventThread);

    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIVisible, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMITitle, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMITBVisible, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMILock, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIColl, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIFloating, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIAlignRoot, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIAlignTop, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIAlignClient, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIAlignBottom, 0 );
    CallService( MS_CLIST_REMOVECONTEXTFRAMEMENUITEM, ( LPARAM )contMIBorder, 0 );
    return 0;
}

static int SetIconForExtraColumn(WPARAM wParam,LPARAM lParam)
{
    pIconExtraColumn piec;

    if (pcli->hwndContactTree == 0)
        return -1;

    if (wParam == 0 || lParam == 0 || IsBadCodePtr((FARPROC)lParam))
        return -1;

    piec = (pIconExtraColumn)lParam;

    if (piec->cbSize != sizeof(IconExtraColumn))
        return -1;

    if(piec->ColumnType == 7)
		piec->ColumnType = 6;

    if(g_CluiData.bMetaAvail && g_CluiData.bMetaEnabled && DBGetContactSettingByte((HANDLE)wParam, g_CluiData.szMetaName, "IsSubcontact", 0))
        PostMessage(pcli->hwndContactTree, CLM_SETEXTRAIMAGEINTMETA, wParam, MAKELONG((WORD)piec->ColumnType,  (WORD)piec->hImage));
    else
        PostMessage(pcli->hwndContactTree, CLM_SETEXTRAIMAGEINT, wParam, MAKELONG((WORD)piec->ColumnType,  (WORD)piec->hImage));
    return 0;
}

//wparam=hIcon
//return hImage on success,-1 on failure
static int AddIconToExtraImageList(WPARAM wParam,LPARAM lParam)
{
    if (himlExtraImages == 0 || wParam==0)
        return -1;

    return((int)ImageList_AddIcon(himlExtraImages, (HICON)wParam));
}

/*
static int SkinDrawBgService(WPARAM wParam, LPARAM lParam)
{
    StatusItems_t item;
    HWND hwnd;
    RECT rc;

    SKINDRAWREQUEST *sdrq = (SKINDRAWREQUEST *)wParam;

    if(wParam == 0 || IsBadCodePtr((FARPROC)wParam) || pDrawAlpha == NULL)
        return 0;

    hwnd = WindowFromDC(sdrq->hDC);
    GetClientRect(hwnd, &rc);
    if(strstr(sdrq->szObjectID, "/Background") && EqualRect(&sdrq->rcClipRect, &sdrq->rcDestRect)) {
        SkinDrawBg(hwnd, sdrq->hDC);
        GetItemByStatus(ID_EXTBKEVTAREA, &item);
        if(item.IGNORED)
            FillRect(sdrq->hDC, &(sdrq->rcClipRect), GetSysColorBrush(COLOR_3DFACE));
        else {
            DrawAlpha(sdrq->hDC, &(sdrq->rcClipRect), item.COLOR, item.ALPHA, item.COLOR2, item.COLOR2_TRANSPARENT,
                      item.GRADIENT, item.CORNER, item.BORDERSTYLE, item.imageItem);
        }
    }
    else {
        GetItemByStatus(ID_EXTBKEVTAREA, &item);
        if(item.IGNORED)
            FillRect(sdrq->hDC, &(sdrq->rcClipRect), GetSysColorBrush(COLOR_3DFACE));
        else {
            DrawAlpha(sdrq->hDC, &(sdrq->rcClipRect), item.COLOR, item.ALPHA, item.COLOR2, item.COLOR2_TRANSPARENT,
                      item.GRADIENT, item.CORNER, item.BORDERSTYLE, item.imageItem);
        }
    }
}
*/

void RegisterCLUIFrameClasses()
{
    WNDCLASS wndclass;
    WNDCLASS cntclass;

    wndclass.style         = CS_DBLCLKS;//|CS_HREDRAW|CS_VREDRAW ;
    wndclass.lpfnWndProc   = CLUIFrameTitleBarProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = g_hInst;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = CLUIFrameTitleBarClassName;
    RegisterClass(&wndclass);

    cntclass.style         = CS_DBLCLKS/*|CS_HREDRAW|CS_VREDRAW*/|( IsWinVerXPPlus()  ? CS_DROPSHADOW : 0);
    cntclass.lpfnWndProc   = CLUIFrameContainerWndProc;
    cntclass.cbClsExtra    = 0;
    cntclass.cbWndExtra    = 0;
    cntclass.hInstance     = g_hInst;
    cntclass.hIcon         = NULL;
    cntclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    cntclass.hbrBackground = NULL;
    cntclass.lpszMenuName  = NULL;
    cntclass.lpszClassName = _T("FramesContainer");
    RegisterClass(&cntclass);
}

int LoadCLUIFramesModule(void)
{
    GapBetweenFrames = g_CluiData.gapBetweenFrames;

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
    CreateServiceFunction(MS_CLIST_FRAMES_SETSKINNED,CLUIFramesSetUnSetSkinned);

    CreateServiceFunction(CLUIFRAMESSETALIGN,CLUIFramesSetAlign);
    CreateServiceFunction(CLUIFRAMESMOVEDOWN,CLUIFramesMoveDown);
    CreateServiceFunction(CLUIFRAMESMOVEUP,CLUIFramesMoveUp);

    CreateServiceFunction(CLUIFRAMESSETALIGNALTOP,CLUIFramesSetAlignalTop);
    CreateServiceFunction(CLUIFRAMESSETALIGNALCLIENT,CLUIFramesSetAlignalClient);
    CreateServiceFunction(CLUIFRAMESSETALIGNALBOTTOM,CLUIFramesSetAlignalBottom);

    CreateServiceFunction("Set_Floating",CLUIFrameSetFloat);
    hWndExplorerToolBar =FindWindowExA(0,0,"Shell_TrayWnd",NULL);
    OnFrameTitleBarBackgroundChange(0,0);

    //skin
    //CreateServiceFunction(MS_SKIN_DRAWGLYPH, SkinDrawBgService);
    //CreateServiceFunction("SkinEngine/DrawGlyph", SkinDrawBgService);

    FramesSysNotStarted=FALSE;
	g_hPenCLUIFrames = CreatePen(PS_SOLID, 1, DBGetContactSettingDword(NULL, "CLUI", "clr_frameborder", GetSysColor(COLOR_3DDKSHADOW)));
    return 0;
}

void LoadExtraIconModule()
{
    CreateServiceFunction(MS_CLIST_EXTRA_SET_ICON,SetIconForExtraColumn);
    CreateServiceFunction(MS_CLIST_EXTRA_ADD_ICON,AddIconToExtraImageList);

    hExtraImageListRebuilding=CreateHookableEvent(ME_CLIST_EXTRA_LIST_REBUILD);
    hExtraImageApplying=CreateHookableEvent(ME_CLIST_EXTRA_IMAGE_APPLY);

    hStatusBarShowToolTipEvent = CreateHookableEvent(ME_CLIST_FRAMES_SB_SHOW_TOOLTIP);
    hStatusBarHideToolTipEvent = CreateHookableEvent(ME_CLIST_FRAMES_SB_HIDE_TOOLTIP);
}

int UnLoadCLUIFramesModule(void)
{
    int i;
    CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,0);
    CLUIFramesStoreAllFrames();
	DeleteObject(g_hPenCLUIFrames);
	//RemoveDialogBoxHook();
	EnterCriticalSection(&csFrameHook);
    FramesSysNotStarted=TRUE;
    for (i=0;i<nFramescount;i++) {
        DestroyWindow(Frames[i].hWnd);
        Frames[i].hWnd=(HWND)-1;
        DestroyWindow(Frames[i].TitleBar.hwnd);
        Frames[i].TitleBar.hwnd=(HWND)-1;
        DestroyWindow(Frames[i].ContainerWnd);
        Frames[i].ContainerWnd=(HWND)-1;
        DestroyMenu(Frames[i].TitleBar.hmenu);

        if (Frames[i].name!=NULL) free(Frames[i].name);
        if (Frames[i].TitleBar.tbname!=NULL) free(Frames[i].TitleBar.tbname);
    }
    if (Frames) 
		free(Frames);
    Frames=NULL;
    nFramescount=0;
    UnregisterClass(CLUIFrameTitleBarClassName,g_hInst);
	LeaveCriticalSection(&csFrameHook);
    DeleteCriticalSection(&csFrameHook);
    UnitFramesMenu();
    return 0;
}

void ReloadExtraIcons()
{
    NotifyEventHooks(hExtraImageListRebuilding, 0, 0);
}

/*
static LRESULT CALLBACK FrameHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    MSG *msg = (MSG*)lParam;

    if (code < 0)
		return CallNextHookEx(g_hFrameHook, code, wParam, lParam);

    if(code == HC_ACTION) {
        switch(msg->message) {
            case WM_NCPAINT:
            {
				_DebugPopup(0, "wm_ncpaint");
				if(pcli && pcli->hwndContactList != 0 && GetParent(msg->hwnd) == pcli->hwndContactList) {
					HDC hdc = GetDCEx(msg->hwnd, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
					HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 255));
					HPEN hPenOld = SelectObject(hdc, hPen);
					RECT rcWindow;

					GetWindowRect(msg->hwnd, &rcWindow);
					Rectangle(hdc, 0, 0, rcWindow.right, rcWindow.bottom);
					SelectObject(hdc, hPenOld);
					DeleteObject(hPen);
					ReleaseDC(msg->hwnd, hdc);
					return 0;
				}
            }
            default:
                break;
        }
    }
	return CallNextHookEx(g_hFrameHook, code, wParam, lParam);
}
*/
