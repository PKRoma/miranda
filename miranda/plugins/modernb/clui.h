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


#define MS_CLUI_SHOWMAINMENU    "CList/ShowMainMenu"
#define MS_CLUI_SHOWSTATUSMENU  "CList/ShowStatusMenu"

#define AC_SRC_NO_PREMULT_ALPHA     0x01
#define AC_SRC_NO_ALPHA             0x02
#define AC_DST_NO_PREMULT_ALPHA     0x10
#define AC_DST_NO_ALPHA             0x20

#define ANIMATION_STEP              40

/* Declaration of prototypes in other modules */
int CLC_GetShortData(struct ClcData* pData, struct SHORTDATA *pShortData);
int  CLC_EnterDragToScroll(HWND hwnd, int Y);

HFONT CLCPaint_ChangeToFont(HDC hdc,struct ClcData *dat,int id,int *fontHeight);
void CLCPaint_FillQuickHash();

int CListMod_ContactListShutdownProc(WPARAM wParam,LPARAM lParam);
int CListMod_HideWindow(HWND hwndContactList, int mode);

int CListSettings_GetCopyFromCache(pdisplayNameCacheEntry pDest);
int CListSettings_SetToCache(pdisplayNameCacheEntry pSrc);

int CListTray_GetGlobalStatus(WPARAM wparam,LPARAM lparam);

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

void SkinEngine_ApplyTransluency();
HBITMAP SkinEngine_CreateDIB32(int cx, int cy);
HBITMAP SkinEngine_CreateDIB32Point(int cx, int cy, void ** bits);
int SkinEngine_JustUpdateWindowImage();
void SkinEngine_LoadSkin(void);
int SkinEngine_RedrawCompleteWindow();
BOOL SkinEngine_SetRectOpaque(HDC memdc,RECT *fr);
int SkinEngine_UpdateWindowImage();
int SkinEngine_ValidateFrameImageProc(RECT * r);

int StatusBar_Create(HWND parent);

void RowHeight_InitModernRow();

int UnhookAll();


/* External variables */


/* Global variables */


HANDLE  g_hSkinLoadedEvent;

HANDLE  g_hMainThread=NULL;

DWORD   g_dwMainThreadID=0,
        g_dwAskAwayMsgThreadID=0,
        g_dwGetTextThreadID=0,
        g_dwSmoothAnimationThreadID=0,
        g_dwFillFontListThreadID=0;
        
HMENU   g_hMenuMain;
BOOL    g_bTransparentFlag=FALSE;

BOOL    g_mutex_bChangingMode=FALSE,
        g_mutex_bSizing=FALSE;        
        
BOOL    g_flag_bOnModulesLoadedCalled=FALSE;

RECT    g_rcEdgeSizingRect={0};

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

