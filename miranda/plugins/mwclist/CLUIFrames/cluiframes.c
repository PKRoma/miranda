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


//not needed,now use MS_CLIST_FRAMEMENUNOTIFY service
//HANDLE hPreBuildFrameMenuEvent;//external event from clistmenus

static HWND hwndContactList=NULL;
//static HWND hwndStatus=NULL;
extern HINSTANCE g_hInst;

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
extern int InitFramesMenus(void);
extern int UnitFramesMenu();
typedef struct 
{
	int order;
	int realpos;
}SortData;

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


//============
#define CLUIFRAMESSETALIGN				"CLUIFramesSetAlign"

#define CLUIFRAMESSETALIGNALTOP				"CLUIFramesSetAlignalTop"
#define CLUIFRAMESSETALIGNALCLIENT				"CLUIFramesSetAlignalClient"
#define CLUIFRAMESSETALIGNALBOTTOM				"CLUIFramesSetAlignalBottom"

#define CLUIFRAMESMOVEUPDOWN			"CLUIFramesMoveUpDown"
typedef struct tagMenuHandles
{
	HANDLE MainMenuItem;
	HANDLE MIVisible,MITitle,MITBVisible,MILock,MIColl,MIFloating,MIAlignRoot;
	HANDLE MIAlignTop,MIAlignClient,MIAlignBottom;
	HANDLE MIBorder;
} FrameMenuHandles;

typedef struct tagFrameTitleBar{
	HWND hwnd;
	HWND TitleBarbutt;
	HWND hwndTip;

	char *tbname;
	char *tooltip;
	HMENU hmenu;
	HICON hicon;

	BOOLEAN ShowTitleBar;
	BOOLEAN ShowTitleBarTip;
	COLORREF BackColour;
	int oldstyles;
	POINT oldpos;
	RECT wndSize;
} FrameTitleBar;

typedef struct _DockOpt
{
	HWND	hwndLeft;
	HWND	hwndRight;
}
DockOpt;

typedef struct {
	int id;
	HWND hWnd ;
	RECT wndSize;
	char *name;
	int align;
	int height;
	int dwFlags;
	BOOLEAN Locked;
	BOOLEAN visible;
	BOOLEAN needhide;
	BOOLEAN collapsed;
	int prevvisframe;
	int HeightWhenCollapsed;
	FrameTitleBar TitleBar;
	FrameMenuHandles MenuHandles;
	int oldstyles;
	BOOLEAN floating;
	HWND ContainerWnd;
	POINT FloatingPos;
	POINT FloatingSize;
	BOOLEAN minmaxenabled;
	BOOLEAN UseBorder;
	int order;
	DockOpt dockOpt;
} wndFrame;

//static wndFrame Frames[MAX_FRAMES];
static wndFrame *Frames=NULL;

static int nFramescount=0;
static int alclientFrame=-1;//for fast access to frame with alclient properties
static int NextFrameId=100;

HFONT TitleBarFont;
static TitleBarH=DEFAULT_TITLEBAR_HEIGHT;
static boolean resizing=FALSE;

// menus
static HANDLE contMIVisible,contMITitle,contMITBVisible,contMILock,contMIColl,contMIFloating;
static HANDLE contMIAlignRoot;
static HANDLE contMIAlignTop,contMIAlignClient,contMIAlignBottom;
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

static void __inline lockfrm()
{
	EnterCriticalSection(&csFrameHook);
}

static void __inline ulockfrm()
{
	LeaveCriticalSection(&csFrameHook);
}

//////////screen docking,code  from "floating contacts" plugin.

static wndFrame* FindFrameByWnd( HWND hwnd )
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
	fakeMainWindow.ContainerWnd=hwndContactList;
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
					SWP_NOSIZE | SWP_NOZORDER );


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


char *DBGetString(HANDLE hContact,const char *szModule,const char *szSetting)
{
	char *str=NULL;
	DBVARIANT dbv;
	DBGetContactSetting(hContact,szModule,szSetting,&dbv);
	if(dbv.type==DBVT_ASCIIZ)
		str=strdup(dbv.pszVal);
	DBFreeVariant(&dbv);
	return str;
}

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
		wsprintf(settingname,"%s%d","Name",i);
		frmname=DBGetString(0,CLUIFrameModule,settingname);
		if(frmname==NULL) continue;
		if(strcmpi(frmname,Frames[Frameid].name)==0) {
			storpos=i;
			free(frmname);
			break;
		}
		free(frmname);
	}
	return storpos;
}

int CLUIFramesLoadFrameSettings(int Frameid)
{
	int storpos,maxstored;

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
	if(alclientFrame!=-1)
		return alclientFrame;
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

		mi.pszPopupName=(char *)menuid;
		mi.popupPosition=frameid;
		mi.position=popuppos++;
		mi.pszName=Translate("&Up");
		mi.flags=CMIF_CHILDPOPUP;
		mi.pszService=CLUIFRAMESMOVEUPDOWN;
		mi.pszContactOwner=(char *)1;
		CallService(addservice,0,(LPARAM)&mi);
	
		mi.pszPopupName=(char *)menuid;
		mi.popupPosition=frameid;
		mi.position=popuppos++;
		mi.pszName=Translate("&Down");
		mi.flags=CMIF_CHILDPOPUP;
		mi.pszService=CLUIFRAMESMOVEUPDOWN;
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
	//TMO_MenuItem tmi;
	
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
			style|=WS_BORDER;
			if(flag&F_NOBORDER) {style&=(~WS_BORDER);};
			 SetWindowLong(Frames[pos].hWnd,GWL_STYLE,(LONG)style);
			ulockfrm();
			CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
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
			if (Frames[pos].floating&&(Frames[pos].TitleBar.tbname!=NULL)){SetWindowText(Frames[pos].ContainerWnd,Frames[pos].TitleBar.tbname);};
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
			retval=Frames[pos].height;
			Frames[pos].height=lParam;
			if(!CLUIFramesFitInSize()) Frames[pos].height=retval;
			retval=Frames[pos].height;
			ulockfrm();
			
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
				OutputDebugString("Wrong align option \r\n");
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
	CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
	return -1;
}

//wparam=lparam=0
static int CLUIFramesShowAll(WPARAM wParam,LPARAM lParam)
{
	int i;
	for(i=0;i<nFramescount;i++)
		Frames[i].visible=TRUE;
	CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
	return 0;
}

//wparam=lparam=0
int CLUIFramesShowAllTitleBars(WPARAM wParam,LPARAM lParam)
{
	int i;
	for(i=0;i<nFramescount;i++)
		Frames[i].TitleBar.ShowTitleBar=TRUE;
	CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
	return 0;
}

//wparam=lparam=0
int CLUIFramesHideAllTitleBars(WPARAM wParam,LPARAM lParam)
{
	int i;
	for(i=0;i<nFramescount;i++)
		Frames[i].TitleBar.ShowTitleBar=FALSE;
	CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
	return 0;
}

//wparam=frameid
int CLUIFramesShowHideFrame(WPARAM wParam,LPARAM lParam)
{
	int pos;
	
	lockfrm();
	pos=id2pos(wParam);
	if(pos>=0&&(int)pos<nFramescount)
		Frames[pos].visible=!Frames[pos].visible;
	if (Frames[pos].floating){CLUIFrameResizeFloatingFrame(pos);};
	ulockfrm();
	if (!Frames[pos].floating) CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
	return 0;
}

//wparam=frameid
int CLUIFramesShowHideFrameTitleBar(WPARAM wParam,LPARAM lParam)
{
	int pos;
	
	lockfrm();
	pos=id2pos(wParam);
	if(pos>=0&&(int)pos<nFramescount)
		Frames[pos].TitleBar.ShowTitleBar=!Frames[pos].TitleBar.ShowTitleBar;
		//if (Frames[pos].height>
	
	ulockfrm();
	
	CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);


	return 0;
}
//wparam=frameid
//lparam=-1 up ,1 down
int CLUIFramesMoveUpDown(WPARAM wParam,LPARAM lParam)
{
	int pos,i,curpos,curalign,v,tmpval;
	
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
		CLUIFramesOnClistResize((WPARAM)hwndContactList,0);		

	}
	ulockfrm();
	return(0);
};



//wparam=frameid
//lparam=alignment
int CLUIFramesSetAlign(WPARAM wParam,LPARAM lParam)
{
	CLUIFramesSetFrameOptions(MAKEWPARAM(FO_ALIGN,wParam),lParam);
	CLUIFramesOnClistResize((WPARAM)hwndContactList,0);		
	return(0);
}
int CLUIFramesSetAlignalTop(WPARAM wParam,LPARAM lParam)
{
	return CLUIFramesSetAlign(wParam,alTop);
}
int CLUIFramesSetAlignalBottom(WPARAM wParam,LPARAM lParam)
{
	return CLUIFramesSetAlign(wParam,alBottom);
}
int CLUIFramesSetAlignalClient(WPARAM wParam,LPARAM lParam)
{
	return CLUIFramesSetAlign(wParam,alClient);
}


//wparam=frameid
int CLUIFramesLockUnlockFrame(WPARAM wParam,LPARAM lParam)
{
	int pos;
	
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
//		CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
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
	
	lockfrm();
	FrameId=id2pos(wParam);
	if(FrameId>=0&&FrameId<nFramescount)
	{
		int oldHeight;

		// do not collapse/uncollapse client/locked/invisible frames
		if(Frames[FrameId].align==alClient) return 0;
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
					  SetWindowPos(Frames[FrameId].ContainerWnd,HWND_TOP,0,0,Frames[FrameId].wndSize.right-Frames[FrameId].wndSize.left+6,Frames[FrameId].height+DEFAULT_TITLEBAR_HEIGHT+4,SWP_SHOWWINDOW|SWP_NOMOVE);		  
					};
				
				
				ulockfrm();return -1;};//redraw not needed
		}
		};//floating test
		ulockfrm();
		//CLUIFramesOnClistResize((WPARAM)hwndContactList,0);		
		if (!Frames[FrameId].floating)
		{
		CLUIFramesOnClistResize((WPARAM)hwndContactList,0);		
		}
		else
		{
		  //SetWindowPos(Frames[FrameId].hWnd,HWND_TOP,0,0,Frames[FrameId].wndSize.right-Frames[FrameId].wndSize.left,Frames[FrameId].height,SWP_SHOWWINDOW|SWP_NOMOVE);		  
		  RECT contwnd;
		  GetWindowRect(Frames[FrameId].ContainerWnd,&contwnd);
		  contwnd.top=contwnd.bottom-contwnd.top;//height
		  contwnd.left=contwnd.right-contwnd.left;//width
				
  		  contwnd.top-=(oldHeight-Frames[FrameId].height);//newheight
		  SetWindowPos(Frames[FrameId].ContainerWnd,HWND_TOP,0,0,contwnd.left,contwnd.top,SWP_SHOWWINDOW|SWP_NOMOVE);		  
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
	
	if (!(ServiceExists(MS_CLIST_REMOVEMAINMENUITEM)))
	{
		//hmm new menu system not used..so display only two items and warning message
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	// create "show all frames" menu
	mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_MIRANDA));
	mi.flags=CMIF_GRAYED;
	mi.position=10000000;
	mi.pszPopupName=Translate("Frames");
	mi.pszName=Translate("New Menu System not Found...");
	mi.pszService="";
	CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
	
	// create "show all frames" menu
	mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_MIRANDA));
	mi.flags=0;
	mi.position=10100000;
	mi.pszPopupName=Translate("Frames");
	mi.pszName=Translate("Show All Frames");
	mi.pszService=MS_CLIST_FRAMES_SHOWALLFRAMES;
	CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
	
	mi.hIcon=NULL;//LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_HELP));
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
		
		return(SendMessage(Frames[framepos].TitleBar.hwndTip,TTM_UPDATETIPTEXT ,(WPARAM)0,(LPARAM)&ti));
		}	

};

//wparam=(CLISTFrame*)clfrm
int CLUIFramesAddFrame(WPARAM wParam,LPARAM lParam)
{
	int style,retval;
	CLISTFrame *clfrm=(CLISTFrame *)wParam;

	if(hwndContactList==0) return -1;
	if(clfrm->cbSize!=sizeof(CLISTFrame)) return -1;
	if(!(TitleBarFont)) TitleBarFont=(HFONT)CLUILoadTitleBarFont();

	lockfrm();
	if(nFramescount>=MAX_FRAMES) { ulockfrm(); return -1;}
	Frames=(wndFrame*)realloc(Frames,sizeof(wndFrame)*(nFramescount+1));
	
	memset(&Frames[nFramescount],0,sizeof(wndFrame));
	Frames[nFramescount].id=NextFrameId++;
	Frames[nFramescount].align=clfrm->align;
	Frames[nFramescount].hWnd=clfrm->hWnd;
	Frames[nFramescount].height=clfrm->height;
	Frames[nFramescount].TitleBar.hicon=clfrm->hIcon;
	Frames[nFramescount].TitleBar.BackColour;
	Frames[nFramescount].floating=FALSE;
	

	//override tbbtip
	//clfrm->Flags|=F_SHOWTBTIP;
	//
	if (DBGetContactSettingByte(0,CLUIFrameModule,"RemoveAllBorders",0)==1)
	{
		clfrm->Flags|=F_NOBORDER;
	};
	Frames[nFramescount].dwFlags=clfrm->Flags;

	if(clfrm->name==NULL||strlen(clfrm->name)==0) {
		Frames[nFramescount].name=(char *)malloc(255);
		GetClassName(Frames[nFramescount].hWnd,Frames[nFramescount].name,255);
	}
	else
		Frames[nFramescount].name=_strdup(clfrm->name);		
	if(clfrm->TBname==NULL||strlen(clfrm->TBname)==0)
		Frames[nFramescount].TitleBar.tbname=_strdup(Frames[nFramescount].name);
	else
		Frames[nFramescount].TitleBar.tbname=_strdup(clfrm->TBname);
	Frames[nFramescount].needhide=FALSE;
	Frames[nFramescount].TitleBar.ShowTitleBar=(clfrm->Flags&F_SHOWTB?TRUE:FALSE);
	Frames[nFramescount].TitleBar.ShowTitleBarTip=(clfrm->Flags&F_SHOWTBTIP?TRUE:FALSE);

	Frames[nFramescount].collapsed = clfrm->Flags&F_UNCOLLAPSED?FALSE:TRUE;
	Frames[nFramescount].Locked = clfrm->Flags&F_LOCKED?TRUE:FALSE;
	Frames[nFramescount].visible = clfrm->Flags&F_VISIBLE?TRUE:FALSE;

	Frames[nFramescount].UseBorder=(clfrm->Flags&F_NOBORDER)?FALSE:TRUE;


	// create frame
	Frames[nFramescount].TitleBar.hwnd
		=CreateWindow(CLUIFrameTitleBarClassName,Frames[nFramescount].name,
					WS_BORDER|WS_CHILD|WS_CLIPCHILDREN|
					(Frames[nFramescount].TitleBar.ShowTitleBar?WS_VISIBLE:0)|
					WS_CLIPCHILDREN,
					0,0,0,0,hwndContactList,NULL,g_hInst,NULL);
	SetWindowLong(Frames[nFramescount].TitleBar.hwnd,GWL_USERDATA,Frames[nFramescount].id);
	
 Frames[nFramescount].TitleBar.hwndTip 
	 =CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
                            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            hwndContactList, NULL, g_hInst,
                            NULL);

SetWindowPos(Frames[nFramescount].TitleBar.hwndTip, HWND_TOPMOST,0, 0, 0, 0,
             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		{
		TOOLINFO ti;
		int res;
		
		ZeroMemory(&ti,sizeof(ti));
		ti.cbSize=sizeof(ti);
		ti.lpszText="";
		ti.hinst=g_hInst;
		ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS ;
		ti.uId=(UINT)Frames[nFramescount].TitleBar.hwnd;
		res=SendMessage(Frames[nFramescount].TitleBar.hwndTip,TTM_ADDTOOL,(WPARAM)0,(LPARAM)&ti);
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
		style&=(~WS_BORDER);
		style|=((Frames[nFramescount-1].UseBorder)?WS_BORDER:0);
		SetWindowLong(Frames[nFramescount-1].hWnd,GWL_STYLE,style);

	
	if (Frames[nFramescount-1].order==0){Frames[nFramescount-1].order=nFramescount;};
	ulockfrm();

	
	alclientFrame=-1;//recalc it
	CLUIFramesOnClistResize((WPARAM)hwndContactList,0);
	
	if (Frames[nFramescount-1].floating)
	{
		
		Frames[nFramescount-1].floating=FALSE;
		//SetWindowPos(Frames[nFramescount-1].hw
		CLUIFrameSetFloat(retval,1);//lparam=1 use stored width and height
	};
	
	return retval;
}

static int CLUIFramesRemoveFrame(WPARAM wParam,LPARAM lParam)
{
	int pos;
	lockfrm();
	pos=id2pos(wParam);
	if (pos<0||pos>nFramescount){ulockfrm();return(-1);};
	
	if (Frames[pos].name!=NULL) free(Frames[pos].name);
	if (Frames[pos].TitleBar.tbname!=NULL) free(Frames[pos].TitleBar.tbname);
	if (Frames[pos].TitleBar.tooltip!=NULL) free(Frames[pos].TitleBar.tooltip);

		DestroyWindow(Frames[pos].hWnd);
		Frames[pos].hWnd=(HWND)-1;
		DestroyWindow(Frames[pos].TitleBar.hwnd);
		Frames[pos].TitleBar.hwnd=(HWND)-1;
		DestroyWindow(Frames[pos].ContainerWnd);
		Frames[pos].ContainerWnd=(HWND)-1;
		DestroyMenu(Frames[pos].TitleBar.hmenu);

	RemoveItemFromList(pos,&Frames,&nFramescount);

	ulockfrm();
	CLUIFramesOnClistResize((WPARAM)hwndContactList,0);
	
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
	
	// we need to show or hide the frame?
	if(Frame->visible&&(!Frame->needhide)) {
		ShowWindow(Frame->hWnd,SW_SHOW);
		ShowWindow(Frame->TitleBar.hwnd,Frame->TitleBar.ShowTitleBar==TRUE?SW_SHOW:SW_HIDE);
	}
	else {
		ShowWindow(Frame->hWnd,SW_HIDE);
		ShowWindow(Frame->TitleBar.hwnd,SW_HIDE);
		return(0);
	}

	// set frame position
	SetWindowPos(Frame->hWnd,NULL,Frame->wndSize.left,Frame->wndSize.top,
					Frame->wndSize.right-Frame->wndSize.left,
					Frame->wndSize.bottom-Frame->wndSize.top,SWP_NOZORDER|SWP_NOREDRAW);
	// set titlebar position
	if(Frame->TitleBar.ShowTitleBar) {
		SetWindowPos(Frame->TitleBar.hwnd,NULL,Frame->wndSize.left,Frame->wndSize.top-TitleBarH-1,
					Frame->wndSize.right-Frame->wndSize.left,
					TitleBarH,SWP_NOZORDER|SWP_NOREDRAW	);
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
			sumheight+=(Frames[i].height)+(TitleBarH*btoint(Frames[i].TitleBar.ShowTitleBar))+2;
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
	if(hwndContactList==NULL) return 0; 
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
	GetBorderSize(hwndContactList,&border);
	
	//GetWindowRect(hwndContactList,&winrect);
	//GetClientRect(hwndContactList,&clirect);
//	clirect.bottom-=clirect.top;
//	clirect.bottom+=border.top+border.bottom;
	//allbord=(winrect.bottom-winrect.top)-(clirect.bottom-clirect.top);
	return (sumheight+border.top+border.bottom+allbord+tbh+3);
}


int CLUIFramesResize(const RECT newsize)
{
	int sumheight=9999999,newheight;
	int prevframe,prevframebottomline;
	int tbh,curfrmtbh;
	int drawitems;
	int clientfrm;
	int i,j;
	int sepw=GapBetweenFrames;
	SortData *sdarray;

	if(nFramescount<1) return 0; 

	newheight=newsize.bottom-newsize.top;

	
	// search for alClient frame and get the titlebar's height
	tbh=0;
	clientfrm=CLUIFramesGetalClientFrame();
	if(clientfrm!=-1)
		tbh=(TitleBarH+1)*btoint(Frames[clientfrm].TitleBar.ShowTitleBar);
	
	for(i=0;i<nFramescount;i++)
	{
		if (!Frames[i].floating)
		{
		Frames[i].needhide=FALSE;
		Frames[i].wndSize.left=0;
		Frames[i].wndSize.right=newsize.right-0;
		};
	};
	{
		//sorting stuff
		sdarray=(SortData*)malloc(sizeof(SortData)*nFramescount);
		if (sdarray==NULL){return(-1);};
		for(i=0;i<nFramescount;i++)
		{	sdarray[i].order=Frames[i].order;
			sdarray[i].realpos=i;
		};
		qsort(sdarray,nFramescount,sizeof(SortData),sortfunc);

	}

	drawitems=nFramescount;
	
	while(sumheight>(newheight-tbh)&&drawitems>0) {
		sumheight=0;
		drawitems=0;
		for(i=0;i<nFramescount;i++)	{
			if(((Frames[i].align!=alClient))&&(!Frames[i].floating)&&(Frames[i].visible)&&(!Frames[i].needhide)) {
				drawitems++;
				curfrmtbh=(TitleBarH+1)*btoint(Frames[i].TitleBar.ShowTitleBar);
				sumheight+=(Frames[i].height)+curfrmtbh+sepw;
				if(sumheight>newheight-tbh) {
					sumheight-=(Frames[i].height)+curfrmtbh+sepw;
					Frames[i].needhide=TRUE;
					drawitems--;
					break;
				}
			}
		}
	}

	prevframe=-1;
	prevframebottomline=0;
	for(j=0;j<nFramescount;j++) {
		//move all alTop frames
		i=sdarray[j].realpos;
		if((!Frames[i].needhide)&&(!Frames[i].floating)&&(Frames[i].visible)&&(Frames[i].align==alTop)) {
			curfrmtbh=(TitleBarH+1)*btoint(Frames[i].TitleBar.ShowTitleBar);
			Frames[i].wndSize.top=prevframebottomline+sepw+(curfrmtbh);
			Frames[i].wndSize.bottom=Frames[i].height+Frames[i].wndSize.top;
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
		for(j=0;j<nFramescount;j++)	{
			//move alClient frame
			i=sdarray[j].realpos;
			if((!Frames[i].needhide)&&(!Frames[i].floating)&&(Frames[i].visible)&&(Frames[i].align==alClient)) {			
				Frames[i].wndSize.top=prevframebottomline+sepw+(tbh);
				Frames[i].wndSize.bottom=Frames[i].wndSize.top+newheight-sumheight-tbh-sepw;

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
	prevframebottomline=newheight+sepw;
	//prevframe=-1;
	for(j=nFramescount-1;j>=0;j--) {
		//move all alBottom frames
		i=sdarray[j].realpos;
		if((Frames[i].visible)&&(!Frames[i].floating)&&(!Frames[i].needhide)&&(Frames[i].align==alBottom)) {
			curfrmtbh=(TitleBarH+1)*btoint(Frames[i].TitleBar.ShowTitleBar);

			Frames[i].wndSize.bottom=prevframebottomline-sepw;
			Frames[i].wndSize.top=Frames[i].wndSize.bottom-Frames[i].height;
			Frames[i].prevvisframe=prevframe;
			prevframe=i;
			prevframebottomline=Frames[i].wndSize.top/*-1*/-curfrmtbh;
			if(prevframebottomline>newheight) {
			
			}
		}
	}

	if (sdarray!=NULL){free(sdarray);sdarray=NULL;};

	for(i=0;i<nFramescount;i++){
		
		if (Frames[i].floating){
			CLUIFrameResizeFloatingFrame(i);
		}else
		{
			CLUIFrameMoveResize(&Frames[i]);
		};

	}
	return 0;
}

int CLUIFramesUpdateFrame(WPARAM wParam,LPARAM lParam)
{
	if(wParam==-1) { CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0); return 0;}
	if(lParam&FU_FMPOS)	CLUIFramesOnClistResize((WPARAM)hwndContactList,1);
	lockfrm();
	wParam=id2pos(wParam);
	if(wParam<0||(int)wParam>=nFramescount) { ulockfrm(); return -1;}
	if(lParam&FU_TBREDRAW)	CLUIFramesForceUpdateTB(&Frames[wParam]);
	if(lParam&FU_FMREDRAW)	CLUIFramesForceUpdateFrame(&Frames[wParam]);
	//if (){};
	ulockfrm();
	
	return 0;
}

int CLUIFramesOnClistResize(WPARAM wParam,LPARAM lParam)
{
	RECT nRect,rcStatus;
	int tick,i;
	
	lockfrm();
	//if (resizing){return(0);};
	//resizing=TRUE;
	GetClientRect(hwndContactList,&nRect);
	
	//if(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1)) GetWindowRect(hwndStatus,&rcStatus);
	//else rcStatus.top=rcStatus.bottom=0;
	rcStatus.top=rcStatus.bottom=0;
	
	
	nRect.bottom-=nRect.top;
	nRect.bottom-=(rcStatus.bottom-rcStatus.top);
	nRect.right-=nRect.left;
	nRect.left=0;
	nRect.top=0;
	ContactListHeight=nRect.bottom;
	
	tick=GetTickCount();


		CLUIFramesResize(nRect);

	for (i=0;i<nFramescount;i++)
	{
		CLUIFramesForceUpdateFrame(&Frames[i]);
		CLUIFramesForceUpdateTB(&Frames[i]);

	};

	//resizing=FALSE;	
	ulockfrm();
	tick=GetTickCount()-tick;

	if (hwndContactList!=0) InvalidateRect(hwndContactList,NULL,TRUE);
	if (hwndContactList!=0) UpdateWindow(hwndContactList);

	//if(lParam==2) RedrawWindow(hwndContactList,NULL,NULL,RDW_UPDATENOW|RDW_ALLCHILDREN|RDW_ERASE|RDW_INVALIDATE);
	

	Sleep(0);
	
	//dont save to database too many times
	if(GetTickCount()-LastStoreTick>1000){ CLUIFramesStoreAllFrames();LastStoreTick=GetTickCount();};
	
	return 0;
}

static int DrawTitleBar(HDC dc,RECT rect,int Frameid)
{
					HDC hdcMem;
					HBITMAP hBmpOsb;
					HBRUSH hBack;
					HDC paintDC;
					int pos;

					//GetClientRect(hwnd,&rect);	
					paintDC=dc;
					
//					paintDC = BeginPaint(hwnd, &paintStruct);
					//rect=paintStruct.rcPaint;


					hdcMem=CreateCompatibleDC(paintDC);
					hBmpOsb=CreateBitmap(rect.right,rect.bottom,1,GetDeviceCaps(paintDC,BITSPIXEL),NULL);
					SelectObject(hdcMem,hBmpOsb);
		
					SelectObject(hdcMem,TitleBarFont);
					SetBkMode(hdcMem,TRANSPARENT);
					
					hBack=GetSysColorBrush(COLOR_3DFACE);
					SelectObject(hdcMem,hBack);

					FillRect(hdcMem,&rect,hBack);
					lockfrm();
					pos=id2pos(Frameid);

					if (pos>=0&&pos<nFramescount)
					{
					GetClientRect(Frames[pos].TitleBar.hwnd,&Frames[pos].TitleBar.wndSize);
					{
						//set font charset
						HFONT hf=GetStockObject(DEFAULT_GUI_FONT);
						SelectObject(hdcMem,hf);

					
						if(Frames[pos].TitleBar.hicon!=NULL)	{
							//(TitleBarH>>1)-(GetSystemMetrics(SM_CXSMICON)>>1)
							DrawIconEx(hdcMem,2,((TitleBarH>>1)-(GetSystemMetrics(SM_CYSMICON)>>1)),Frames[pos].TitleBar.hicon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
							TextOut(hdcMem,GetSystemMetrics(SM_CYSMICON)+4,2,Frames[pos].TitleBar.tbname,strlen(Frames[pos].TitleBar.tbname));
						}
						else
							TextOut(hdcMem,2,2,Frames[pos].TitleBar.tbname,strlen(Frames[pos].TitleBar.tbname));

						DrawIconEx(hdcMem,Frames[pos].TitleBar.wndSize.right-GetSystemMetrics(SM_CXSMICON)-2,((TitleBarH>>1)-(GetSystemMetrics(SM_CXSMICON)>>1)),Frames[pos].collapsed?LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN):LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT),GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
						DeleteObject(hf);
						}
					};
					ulockfrm();

					BitBlt(paintDC,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,hdcMem,rect.left,rect.top,SRCCOPY);
					DeleteDC(hdcMem);
					DeleteObject(hBack);
					DeleteObject(hBmpOsb);
					//EndPaint(hwnd, &paintStruct);
					return 0;

};

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

		case WM_ENABLE:
			 if (hwnd!=0) InvalidateRect(hwnd,NULL,FALSE);
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
				CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
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
				AppendMenu(hmenu,MF_STRING|MF_DISABLED|MF_GRAYED,15,Frames[framepos].name);
				AppendMenu(hmenu,MF_SEPARATOR,16,"");

				if (Frames[framepos].Locked)
				{AppendMenu(hmenu,MF_STRING|MF_CHECKED,frame_menu_lock,Translate("Lock Frame"));}
				else{AppendMenu(hmenu,MF_STRING,frame_menu_lock,Translate("Lock Frame"));};

				if (Frames[framepos].visible)
				{AppendMenu(hmenu,MF_STRING|MF_CHECKED,frame_menu_visible,Translate("Visible"));}
				else{AppendMenu(hmenu,MF_STRING,frame_menu_visible,Translate("Visible") );};
				
				if (Frames[framepos].TitleBar.ShowTitleBar)
				{AppendMenu(hmenu,MF_STRING|MF_CHECKED,frame_menu_showtitlebar,Translate("Show TitleBar") );}
				else{AppendMenu(hmenu,MF_STRING,frame_menu_showtitlebar,Translate("Show TitleBar") );};

				if (Frames[framepos].floating)
				{AppendMenu(hmenu,MF_STRING|MF_CHECKED,frame_menu_floating,Translate("Floating") );}
				else{AppendMenu(hmenu,MF_STRING,frame_menu_floating,Translate("Floating") );};
				
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
					//pt=nm->pt;
					GetCursorPos(&pt);
					return SendMessage(GetParent(hwnd), WM_SYSCOMMAND, SC_MOVE|HTCAPTION,MAKELPARAM(pt.x,pt.y));
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
			wsprintf(TBcapt,"%s - h:%d, vis:%d, fl:%d, fl:(%d,%d,%d,%d),or: %d",
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

						GetWindowRect(hwndContactList, &rcMiranda );
						//GetWindowRect( Frames[pos].ContainerWnd, &rcwnd );
						//IntersectRect( &rcOverlap, &rcwnd, &rcMiranda )
						if (IsWindowVisible(hwndContactList) &&IntersectRect( &rcOverlap, &rcwnd, &rcMiranda ))
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

						GetWindowRect(hwndContactList, &rcMiranda );
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
								SWP_NOSIZE | SWP_NOZORDER );
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
					if (newh>0){CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);};
					break;
				}
				curdragbar=-1;lbypos=-1;oldframeheight=-1;ReleaseCapture();
			}
			break;
		case WM_PRINTCLIENT:
			{
				if (lParam&PRF_CLIENT)
				{
				GetClientRect(hwnd,&rect);
				DrawTitleBar((HDC)wParam,rect,Frameid);
				}
			}
		case WM_PAINT:	
			{
					HDC paintDC;
					PAINTSTRUCT paintStruct;

					//GetClientRect(hwnd,&rect);	
					paintDC = BeginPaint(hwnd, &paintStruct);
					rect=paintStruct.rcPaint;
					DrawTitleBar(paintDC,rect,Frameid);
					EndPaint(hwnd, &paintStruct);					
					return 0;
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
			
			Frames[framepos].visible?ShowWindow(Frames[framepos].ContainerWnd,SW_SHOW):ShowWindow(Frames[framepos].ContainerWnd,SW_HIDE);
			
			

			if (Frames[framepos].TitleBar.ShowTitleBar)
			{
			ShowWindow(Frames[framepos].TitleBar.hwnd,SW_SHOW);
			//if (Frames[framepos].Locked){return(0);};
			Frames[framepos].height=height-DEFAULT_TITLEBAR_HEIGHT;
						
			SetWindowPos(Frames[framepos].TitleBar.hwnd,HWND_TOP,0,0,width,DEFAULT_TITLEBAR_HEIGHT,SWP_SHOWWINDOW|SWP_DRAWFRAME);
			SetWindowPos(Frames[framepos].hWnd,HWND_TOP,0,DEFAULT_TITLEBAR_HEIGHT,width,height-DEFAULT_TITLEBAR_HEIGHT,SWP_SHOWWINDOW);
			
			}
			else
			{
			//SetWindowPos(Frames[framepos].TitleBar.hwnd,HWND_TOP,0,0,width,DEFAULT_TITLEBAR_HEIGHT,SWP_SHOWWINDOW|SWP_NOMOVE);
			//if (Frames[framepos].Locked){return(0);};
			Frames[framepos].height=height;
			ShowWindow(Frames[framepos].TitleBar.hwnd,SW_HIDE);
			SetWindowPos(Frames[framepos].hWnd,HWND_TOP,0,0,width,height,SWP_SHOWWINDOW);		
			
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
		return(SendMessage(hwndContactList,msg,wParam,lParam));
		*/
			

	};
	return DefWindowProc(hwnd, msg, wParam, lParam);
};
static HWND CreateContainerWindow(HWND parent,int x,int y,int width,int height)
{

	return(CreateWindow("FramesContainer","aaaa",WS_POPUP|WS_THICKFRAME,x,y,width,height,parent,0,g_hInst,0));
	
};


int CLUIFrameSetFloat(WPARAM wParam,LPARAM lParam)
{	
	int hwndtmp,hwndtooltiptmp;
	
	lockfrm();
	wParam=id2pos(wParam);
	if(wParam>=0&&(int)wParam<nFramescount)
	
	//parent=GetParent(Frames[wParam].hWnd);
	if (Frames[wParam].floating)
	{
		//SetWindowLong(Frames[wParam].hWnd,GWL_STYLE,Frames[wParam].oldstyles);
		//SetWindowLong(Frames[wParam].TitleBar.hwnd,GWL_STYLE,Frames[wParam].TitleBar.oldstyles);
		SetParent(Frames[wParam].hWnd,hwndContactList);
		SetParent(Frames[wParam].TitleBar.hwnd,hwndContactList);
		Frames[wParam].floating=FALSE;
		DestroyWindow(Frames[wParam].ContainerWnd);
		Frames[wParam].ContainerWnd=0;
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

		Frames[wParam].ContainerWnd=CreateContainerWindow(hwndContactList,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,10,10);

		
		

		SetParent(Frames[wParam].hWnd,Frames[wParam].ContainerWnd);
		SetParent(Frames[wParam].TitleBar.hwnd,Frames[wParam].ContainerWnd);
		
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

			SetWindowPos(Frames[wParam].ContainerWnd,HWND_TOPMOST,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,Frames[wParam].FloatingSize.x,Frames[wParam].FloatingSize.y,SWP_HIDEWINDOW);
			}else
			{
				SetWindowPos(Frames[wParam].ContainerWnd,HWND_TOPMOST,120,120,140,140,SWP_HIDEWINDOW);
			};
		}
		else
		{
			neww=rectw.right-rectw.left+border.left+border.right;
			newh=(rectw.bottom-rectw.top)+(recttb.bottom-recttb.top)+border.top+border.bottom;
			if (neww<20){neww=40;};
			if (newh<20){newh=40;};
			if (Frames[wParam].FloatingPos.x<20){Frames[wParam].FloatingPos.x=40;};
			if (Frames[wParam].FloatingPos.y<20){Frames[wParam].FloatingPos.y=40;};

			SetWindowPos(Frames[wParam].ContainerWnd,HWND_TOPMOST,Frames[wParam].FloatingPos.x,Frames[wParam].FloatingPos.y,neww,newh,SWP_HIDEWINDOW);
		};
		
		
		SetWindowText(Frames[wParam].ContainerWnd,Frames[wParam].TitleBar.tbname);

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
	CLUIFramesOnClistResize((WPARAM)hwndContactList,(LPARAM)0);
	SendMessage((HWND)hwndtmp,WM_SIZE,0,0);

	
	SetWindowPos((HWND)hwndtooltiptmp, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE  );
	
	return 0;
}


static CLUIFrameOnModulesLoad(WPARAM wParam,LPARAM lParam)
{
	CLUIFramesLoadMainMenu(0,0);
	CLUIFramesCreateMenuForFrame(-1,-1,000010000,MS_CLIST_ADDCONTEXTFRAMEMENUITEM);
}

int LoadCLUIFramesModule(void)
{
	WNDCLASS wndclass;
	WNDCLASS cntclass;

    wndclass.style         = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW ;
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

	hwndContactList=(HWND)CallService(MS_CLUI_GETHWND,0,0);
	//hwndStatus=FindWindowEx(hwndContactList,NULL,"msctls_statusbar32",NULL);
	//container helper
	
    cntclass.style         = CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|( IsWinVerXPPlus()  ? CS_DROPSHADOW : 0);
    cntclass.lpfnWndProc   = CLUIFrameContainerWndProc;
    cntclass.cbClsExtra    = 0;
    cntclass.cbWndExtra    = 0;
    cntclass.hInstance     = g_hInst;
    cntclass.hIcon         = NULL;
    cntclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    cntclass.hbrBackground = NULL;
    cntclass.lpszMenuName  = NULL;
    cntclass.lpszClassName = "FramesContainer";
	RegisterClass(&cntclass);
	//end container helper

	GapBetweenFrames=DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1);

	nFramescount=0;
	InitializeCriticalSection(&csFrameHook);
	InitFramesMenus();

	HookEvent(ME_SYSTEM_MODULESLOADED,CLUIFrameOnModulesLoad);
	HookEvent(ME_CLIST_PREBUILDFRAMEMENU,CLUIFramesModifyContextMenuForFrame);
	HookEvent(ME_CLIST_PREBUILDMAINMENU,CLUIFrameOnMainMenuBuild);

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
	hWndExplorerToolBar	=FindWindowEx(0,0,"Shell_TrayWnd",NULL);
	return 0;
}

int UnLoadCLUIFramesModule(void)
{
	int i;
	CLUIFramesOnClistResize((WPARAM)hwndContactList,0);
	CLUIFramesStoreAllFrames();
	lockfrm();
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
	nFramescount=0;
	UnregisterClass(CLUIFrameTitleBarClassName,g_hInst);
	DeleteObject(TitleBarFont);
	ulockfrm();
	DeleteCriticalSection(&csFrameHook);
	UnitFramesMenu();
	return 0;
}











