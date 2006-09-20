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

/************************************************************************/
/**     FILE CONTAINS HEADER PART FOR .../modernb/clui.c file          **/
/**                                                                    **/
/**         !!!  DO NOT INCLUDE IN TO OTHER FILES  !!!                 **/
/************************************************************************/

/* Definitions */

#define TM_AUTOALPHA                1
#define TM_DELAYEDSIZING            2
#define TM_BRINGOUTTIMEOUT          3
#define TM_BRINGINTIMEOUT           4
#define TM_UPDATEBRINGTIMER         5
#define TM_SMOTHALPHATRANSITION     20
#define TM_WINDOWUPDATE             100
#define TM_STATUSBARUPDATE          200

#define MENU_MIRANDAMENU            0xFFFF1234
#define MENU_STATUSMENU             0xFFFF1235

#define M_CREATECLC                 (WM_USER+1)
#define M_SETALLEXTRAICONS          (WM_USER+2)
#define UM_ALPHASUPPORT             (WM_USER+100)

#define MS_CLUI_SHOWMAINMENU    "CList/ShowMainMenu"
#define MS_CLUI_SHOWSTATUSMENU  "CList/ShowStatusMenu"

#define AC_SRC_NO_PREMULT_ALPHA     0x01
#define AC_SRC_NO_ALPHA             0x02
#define AC_DST_NO_PREMULT_ALPHA     0x10
#define AC_DST_NO_ALPHA             0x20

#define ANIMATION_STEP              40

/* Declaration of prototypes in other modules */

int  CLC_EnterDragToScroll(HWND hwnd, int Y);
HFONT CLCPaint_ChangeToFont(HDC hdc,struct ClcData *dat,int id,int *fontHeight);

int CListMod_ContactListShutdownProc(WPARAM wParam,LPARAM lParam);
int CListMod_HideWindow(HWND hwndContactList, int mode);

int CListTray_GetGlobalStatus(WPARAM wparam,LPARAM lparam);
void CListTray_TrayIconDestroy(HWND hwnd);

int CLUIFrames_ActivateSubContainers(BOOL active);
int CLUIFrames_ApplyNewSizes(int mode);
int CLUIFrames_GetTotalHeight();
int CLUIFrames_OnClistResize_mod(WPARAM wParam,LPARAM lParam, int mode);
int CLUIFrames_OnMoving(HWND hwnd,RECT *lParam);
int CLUIFrames_OnShowHide(HWND hwnd, int mode);
int CLUIFrames_RepaintSubContainers();
int CLUIFrames_SetParentForContainers(HWND parent);
void __inline CLUIFrames_UnLockFrame();
int CLUIFrames_UpdateFrame(WPARAM wParam,LPARAM lParam);

int CLUIOpt_Init(WPARAM wParam,LPARAM lParam);

int CLUIServices_LoadModule(void);
int CLUIServices_SortList(WPARAM wParam,LPARAM lParam);
int CLUIServices_ProtocolStatusChanged(WPARAM wParam,LPARAM lParam);

void Docking_GetMonitorRectFromWindow(HWND hWnd,RECT *rc);

int EventArea_Create(HWND hCluiWnd);

int ExtraImage_ExtraIDToColumnNum(int extra);
void ExtraImage_LoadModule();
void ExtraImage_ReloadExtraIcons();
void ExtraImage_SetAllExtraIcons(HWND hwndList,HANDLE hContact);

void GroupMenus_Init();

int ModernButton_LoadModule();
int ModernButton_ReposButtons(HWND parent, BOOL draw,RECT *r);

int ProtocolOrder_CheckOrder();
int ProtocolOrder_LoadModule(void);

void SkinEngine_ApplyTransluency();
HBITMAP SkinEngine_CreateDIB32(int cx, int cy);
HBITMAP SkinEngine_CreateDIB32Point(int cx, int cy, void ** bits);
int SkinEngine_JustSkinEngine_UpdateWindowImage();
void SkinEngine_LoadSkinFromDB(void);
int SkinEngine_RedrawCompleteWindow();
BOOL SkinEngine_SetRectOpaque(HDC memdc,RECT *fr);
int SkinEngine_UpdateWindowImage();
int SkinEngine_ValidateFrameImageProc(RECT * r);


int StatusBar_Create(HWND parent);

void RowHeight_InitModernRow();

int UnhookAll();


/* External variables */

extern STATUSBARDATA g_StatusBarData;

extern void (*saveLoadCluiGlobalOpts)(void);
extern CURRWNDIMAGEDATA * g_pCachedWindow;
            
extern HWND g_hwndEventFrame;
extern char * g_szConnectingProto;

extern BOOL g_mutex_bLockImageUpdating;
extern BOOL g_mutex_bLockUpdating;
extern BOOL g_mutex_bSetAllExtraIconsCycle;

extern int  g_mutex_uPreventDockMoving;
extern int  g_mutex_nPaintLock;
extern int  g_mutex_nCalcRowHeightLock;
extern int  g_mutex_bOnTrayRightClick;

extern BOOL g_flag_bUpdateQueued;
extern BOOL g_flag_bPostWasCanceled;
extern BOOL g_flag_bFullRepaint;   

extern BOOL g_bLayered;
extern BOOL g_bDocked;
extern BOOL g_bMultiConnectionMode;

extern BYTE g_bCalledFromShowHide;

extern PLUGININFO pluginInfo;   

extern tPaintCallbackProc CLC_PaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData);
extern BOOL (WINAPI *g_proc_UpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
extern LRESULT ( CALLBACK *saveContactListWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Global variables */

struct CluiData g_CluiData={0};
BYTE    g_bSTATE=STATE_NORMAL;
HANDLE  g_hSkinLoadedEvent;

HANDLE  g_hMainThreadID=NULL,
        g_hAskAwayMsgThreadID=NULL,
        g_hGetTextThreadID=NULL,
        g_hSmoothAnimationThreadID=NULL,
        g_hFillFontListThreadID=NULL;
        
HMENU   g_hMenuMain;
BOOL    g_bTransparentFlag=FALSE;
int     g_nBehindEdgeState=FALSE;
int     g_nBehindEdgeSettings;

BOOL    g_bSmoothAnimation;

BOOL    g_mutex_bChangingMode=FALSE,
        g_mutex_bSizing=FALSE,
        g_mutex_bOnEdgeSizing=FALSE;
        
BOOL    g_flag_bOnModulesLoadedCalled=FALSE;


RECT    g_rcEdgeSizingRect={0};
BYTE    g_bStatusBarShowOptions;
BYTE    g_bCurrentAlpha;
BOOL    g_bOnDesktop=FALSE;

BOOL (WINAPI *g_proc_SetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
BOOL (WINAPI *g_proc_SetLayeredWindowAttributesNew)(HWND,COLORREF,BYTE,DWORD);
BOOL (WINAPI *g_proc_AnimateWindow)(HWND hWnd,DWORD dwTime,DWORD dwFlags);

/* Module function prototypes */

int CLUI_IsInMainWindow(HWND hwnd);
int CLUI_SizingOnBorder(POINT pt, int size);
int CLUI_SmoothAlphaTransition(HWND hwnd, BYTE GoalAlpha, BOOL wParam);
int CLUI_TestCursorOnBorders();

static int CLUI_CreateTimerForConnectingIcon(WPARAM wParam,LPARAM lParam);
static int CLUI_SmoothAlphaThreadTransition(HWND hwnd);

/*      structs         */

typedef struct tagPROTOTICKS{
	int     nIconsCount;
	int     nCycleStartTick;
	char  * szProto;
	int     nIndex;
	BOOL    bTimerCreated;
	BOOL    bGlobal;
	HIMAGELIST himlIconList;
} PROTOTICKS,*LPPROTOTICKS;

typedef struct tagCHECKFILLING
{
	HDC hDC;
	RECT rcRect;
} CHECKFILLING;

/* Module global variables */

static BYTE bAlphaEnd;
static BYTE bOldHideOffline;

static WORD wBehindEdgeShowDelay,
            wBehindEdgeHideDelay,
            wBehindEdgeBorderSize;

static BOOL mutex_bAnimationInProgress=FALSE,
            mutex_bShowHideCalledFromAnimation=FALSE,
            mutex_bIgnoreActivation=FALSE,
            mutex_bDisableAutoUpdate=TRUE,
            mutex_bDuringSizing=FALSE,
            mutex_bDelayedSizing=FALSE;  //TBC is it need?
            
static BOOL flag_bFirstTimeCall=FALSE;

static BOOL bTransparentFocus=TRUE,
            bNeedFixSizingRect=FALSE,
            bShowEventStarted=FALSE;

static HANDLE hContactDraggingEvent,
              hContactDroppedEvent,
              hContactDragStopEvent;

static HANDLE hRenameMenuItem,
              hShowAvatarMenuItem,
              hHideAvatarMenuItem;


static HANDLE hSettingChangedHook=NULL;

static UINT uMsgGetProfile=0;
static UINT uMsgProcessProfile;

static HMODULE hUserDll;
static HIMAGELIST himlMirandaIcon;

static int nLastRequiredHeight=0,
           nRequiredHeight=0,
           nMirMenuState=0,
           nStatusMenuState=0;
 
static RECT rcNewWindowRect={0},
            rcOldWindowRect ={0},
            rcSizingRect={0},
            rcCorrectSizeRect={0};

static HANDLE hFrameContactTree;

static PROTOTICKS CycleStartTick[64]={0};//max 64 protocols 

static int nAnimatedIconStep=100;

