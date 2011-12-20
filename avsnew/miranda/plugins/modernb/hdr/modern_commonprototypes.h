#pragma once

#ifndef _COMMONPROTOTYPES
#define _COMMONPROTOTYPES

#ifndef commonheaders_h__
#error "hdr/modern_commonheaders.h have to be including first"
#endif

#include "modern_commonheaders.h"  //TO DO: Move contents of this file to commonheaders.h
#include "modern_clist.h"
#include "modern_cluiframes.h"
#include "modern_row.h"
#include "modern_skinengine.h"
#include "modern_skinselector.h"
#include "modern_statusbar.h"

#define SKIN  "ModernSkin"

extern PLUGININFOEX pluginInfo;
extern CLIST_INTERFACE * pcli;
extern CLIST_INTERFACE corecli;
extern struct UTF8_INTERFACE utfi;
extern struct LIST_INTERFACE li;
extern struct MM_INTERFACE   mmi;

//Global variables
extern int ON_SETALLEXTRAICON_CYCLE;
extern BOOL CLM_AUTOREBUILD_WAS_POSTED;
extern FRAMEWND *g_pfwFrames;
extern int g_nFramesCount;
extern RECT g_rcEdgeSizingRect;
extern FRAMEWND *wndFrameEventArea;
extern ROWCELL * gl_RowTabAccess[];
extern ROWCELL * gl_RowRoot;
extern HIMAGELIST hAvatarOverlays;
extern int  g_nTitleBarHeight;
extern BOOL g_bTransparentFlag;
extern HIMAGELIST g_himlCListClc;
extern HIMAGELIST hCListImages;
extern BOOL g_mutex_bSizing;
extern BOOL LOCK_RECALC_SCROLLBAR;
extern HIMAGELIST g_himlCListClc;
extern int currentDesiredStatusMode;
extern BYTE nameOrder[];
extern SortedList lContactsCache;
extern BOOL g_flag_bOnModulesLoadedCalled;
extern HIMAGELIST hCListImages;
extern SKINOBJECTSLIST g_SkinObjectList;
extern CURRWNDIMAGEDATA * g_pCachedWindow;
extern BOOL g_mutex_bLockUpdating;
extern LISTMODERNMASK *MainModernMaskList;
extern HIMAGELIST hCListImages;
extern STATUSBARDATA g_StatusBarData;
extern SKINOBJECTSLIST g_SkinObjectList;
extern CURRWNDIMAGEDATA * g_pCachedWindow;
extern char * g_szConnectingProto;
extern BOOL g_mutex_bLockUpdating;
extern BOOL g_mutex_bSetAllExtraIconsCycle;
extern int  g_mutex_nCalcRowHeightLock;
extern int  g_mutex_bOnTrayRightClick;
extern BOOL g_flag_bPostWasCanceled;
extern BOOL g_flag_bFullRepaint;
extern BOOL g_bMultiConnectionMode;
extern BYTE g_bCalledFromShowHide;
extern HICON g_hListeningToIcon;
extern BOOL glOtherSkinWasLoaded;
extern BYTE glSkinWasModified;
extern HWND g_hCLUIOptionsWnd;
extern BOOL g_bTransparentFlag;
extern HINSTANCE g_hInst;
extern HIMAGELIST hCListImages;
extern BOOL g_mutex_bChangingMode;
extern HANDLE g_hMainThread;
extern DWORD g_dwMainThreadID;
extern DWORD g_dwAwayMsgThreadID;
extern DWORD g_dwGetTextAsyncThreadID;
extern DWORD g_dwSmoothAnimationThreadID;
extern DWORD g_dwFillFontListThreadID;
extern HWND  g_hwndViewModeFrame;
extern HANDLE hSmileyAddOptionsChangedHook,hAvatarChanged,hIconChangedHook;
extern BYTE gl_TrimText;

/************************************************************************/
/*                              TYPE DEFS                               */
/************************************************************************/

typedef INT_PTR (*PSYNCCALLBACKPROC)(WPARAM,LPARAM);

/************************************************************************/
/*                              PROTOTYPES                              */
/************************************************************************/


/* CLCItems */
BOOL	CLCItems_IsShowOfflineGroup(struct ClcGroup* group);

/* CListMod */
int		CListMod_HideWindow(HWND hwndContactList, int mode);

/* CLUI */
HANDLE  RegisterIcolibIconHandle(char * szIcoID, char *szSectionName,  char * szDescription, TCHAR * tszDefaultFile, int iDefaultIndex, HINSTANCE hDefaultModule, int iDefaultResource );
void	CLUI_UpdateAeroGlass();
void	CLUI_ChangeWindowMode();
BOOL	CLUI_CheckOwnedByClui(HWND hwnd);
INT_PTR	CLUI_GetConnectingIconService(WPARAM wParam,LPARAM lParam);
int		CLUI_HideBehindEdge();
int		CLUI_IconsChanged(WPARAM,LPARAM);
int		CLUI_IsInMainWindow(HWND hwnd);
HICON	CLUI_LoadIconFromExternalFile (char *filename,int i,boolean UseLibrary,boolean registerit,char *IconName,char *SectName,char *Description,int internalidx, BOOL * needFree);
int		CLUI_OnSkinLoad(WPARAM wParam, LPARAM lParam);
int		CLUI_ReloadCLUIOptions();
int		CLUI_ShowFromBehindEdge();
int		CLUI_SizingGetWindowRect(HWND hwnd,RECT * rc);
int		CLUI_SizingOnBorder(POINT ,int);
int		CLUI_SmoothAlphaTransition(HWND hwnd, BYTE GoalAlpha, BOOL wParam);
int		CLUI_TestCursorOnBorders();
int		CLUI_UpdateTimer(BYTE BringIn);
void	CLUI_UpdateLayeredMode();
UINT_PTR CLUI_SafeSetTimer(HWND hwnd, int ID, int Timeout, TIMERPROC proc);

/* CLUIServices */
INT_PTR	CLUIServices_ProtocolStatusChanged(WPARAM wParam,LPARAM lParam);

int		CLUIUnreadEmailCountChanged(WPARAM wParam,LPARAM lParam);

/* GDIPlus */
BOOL    GDIPlus_AlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc, BLENDFUNCTION * blendFunction);
HBITMAP GDIPlus_LoadGlyphImage(char *szFileName);

/* EventArea */
void	EventArea_ConfigureEventArea();

/* ExtraImage */
void	ExtraImage_SetAllExtraIcons(HWND hwndList,HANDLE hContact);

/* ModernSkinButton */
int		ModernSkinButton_AddButton(HWND parent,char * ID,char * CommandService,char * StateDefService,char * HandeService,             int Left, int Top, int Right, int Bottom, DWORD AlignedTo,TCHAR * Hint,char * DBkey,char * TypeDef,int MinWidth, int MinHeight);
int		ModernSkinButtonLoadModule();
int		ModernSkinButton_ReposButtons(HWND parent, BYTE draw, RECT * r);
int     ModernSkinButtonUnloadModule(WPARAM,LPARAM);

/* RowHeight */
int		RowHeight_CalcRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, int item);

/* SkinEngine */
BOOL	ske_AlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc,BLENDFUNCTION blendFunction);
void	ske_ApplyTransluency(void);
int		ske_BltBackImage (HWND destHWND, HDC destDC, RECT * BltClientRect);
HBITMAP ske_CreateDIB32(int cx, int cy);
HBITMAP ske_CreateDIB32Point(int cx, int cy, void ** bits);
HRGN ske_CreateOpaqueRgn(BYTE Level, bool Opaque);
HICON	ske_CreateJoinedIcon(HICON hBottom, HICON hTop,BYTE alpha);
int		ske_DrawImageAt(HDC hdc, RECT *rc);
BOOL	ske_DrawIconEx(HDC hdc,int xLeft,int yTop,HICON hIcon,int cxWidth,int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);
int     ske_DrawNonFramedObjects(BOOL Erase,RECT *r);
BOOL	ske_DrawText(HDC hdc, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format);
BOOL	ske_DrawTextA(HDC hdc, char * lpString, int nCount, RECT * lpRect, UINT format);
LPSKINOBJECTDESCRIPTOR	ske_FindObjectByName(const char * szName, BYTE objType, SKINOBJECTSLIST* Skin);
HBITMAP ske_GetCurrentWindowImage();
int		ske_GetFullFilename(char * buf, char *file, char * skinfolder,BOOL madeAbsolute);
int		ske_GetSkinFolder(char * szFileName, char * t2);
BOOL	ske_ImageList_DrawEx( HIMAGELIST himl,int i,HDC hdcDst,int x,int y,int dx,int dy,COLORREF rgbBk,COLORREF rgbFg,UINT fStyle);
HICON	ske_ImageList_GetIcon(HIMAGELIST himl, int i, UINT fStyle);
int     ske_JustUpdateWindowImageRect(RECT * rty);
HBITMAP ske_LoadGlyphImage(char * szFileName);
HRESULT SkinEngineLoadModule();
void	ske_LoadSkinFromDB(void);
int		ske_LoadSkinFromIniFile(TCHAR*, BOOL);
TCHAR*	ske_ParseText(TCHAR *stzText);
int		ske_PrepeareImageButDontUpdateIt(RECT * r);
int     ske_ReCreateBackImage(BOOL Erase,RECT *w);
int		ske_RedrawCompleteWindow();
BOOL	ske_ResetTextEffect(HDC);
BOOL	ske_SelectTextEffect(HDC hdc, BYTE EffectID, DWORD FirstColor, DWORD SecondColor);
INT_PTR	ske_Service_DrawGlyph(WPARAM wParam,LPARAM lParam);
BOOL	ske_SetRectOpaque(HDC memdc,RECT *fr, BOOL force = FALSE );
BOOL	ske_SetRgnOpaque(HDC memdc,HRGN hrgn, BOOL force = FALSE );
BOOL	ske_TextOut(HDC hdc, int x, int y, LPCTSTR lpString, int nCount);
BOOL	ske_TextOutA(HDC hdc, int x, int y, char * lpString, int nCount);
int     ske_UnloadGlyphImage(HBITMAP hbmp);
int     SkinEngineUnloadModule();
int     ske_UpdateWindowImage();
int     ske_UpdateWindowImageRect(RECT * lpRect);
int     ske_ValidateFrameImageProc(RECT * r);

/* CLUIFrames.c PROXIED */

int CLUIFrames_ActivateSubContainers(BOOL wParam);
int CLUIFrames_OnClistResize_mod(WPARAM wParam,LPARAM lParam);
int CLUIFrames_OnMoving( HWND, RECT * );
int CLUIFrames_OnShowHide( HWND hwnd, int mode );
int CLUIFrames_SetLayeredMode( BOOL fLayeredMode, HWND hwnd );
int CLUIFrames_SetParentForContainers( HWND parent );
int CLUIFramesOnClistResize(WPARAM wParam,LPARAM lParam);

FRAMEWND * FindFrameByItsHWND(HWND FrameHwnd);						//cluiframes.c

//int callProxied_DrawTitleBar(HDC hdcMem2,RECT * rect,int Frameid);
int DrawTitleBar(HDC hdcMem2,RECT * rect,int Frameid);

int FindFrameID(HWND FrameHwnd);
int SetAlpha(BYTE Alpha);


/* others TODO: move above */
int Docking_ProcessWindowMessage(WPARAM wParam,LPARAM lParam);
void	DrawBackGround(HWND hwnd,HDC mhdc, HBITMAP hBmpBackground, COLORREF bkColour, DWORD backgroundBmpUse );
HRESULT		BackgroundsLoadModule();
int		BackgroundsUnloadModule();
BOOL    wildcmp(const char * name, const char * mask, BYTE option);										//mod_skin_selector.c
BOOL	wildcmpi(char * name, char * mask);													//mod_skin_selector.c
BOOL	wildcmpi(WCHAR* name, WCHAR* mask);													//mod_skin_selector.c
INT_PTR	CALLBACK DlgSkinEditorOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);	//SkinEditor.c
INT_PTR	CALLBACK DlgTmplEditorOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);	//RowTemplate.c
BOOL	FindMenuHanleByGlobalID(HMENU hMenu, int globalID, struct _MenuItemHandles * dat);	//GenMenu.c
BOOL	MatchMask(char * name, char * mask);												//mod_skin_selector.c
char*   DBGetStringA(HANDLE hContact,const char *szModule,const char *szSetting);			//commonheaders.c
char*   GetContactCachedProtocol(HANDLE hContact);											//clistsettings.c
char*   GetParamN(char * string, char * buf, int buflen, BYTE paramN, char Delim, BOOL SkipSpaces);  //mod_skin_selector.c
DWORD	CompareContacts2_getLMTime(HANDLE u);												//contact.c
DWORD	mod_CalcHash(const char * a);														//mod_skin_selector.c
HICON	cliGetIconFromStatusMode(HANDLE hContact, const char *szProto,int status);				//clistmod.c
HICON	GetMainStatusOverlay(int STATUS);													//clc.c
int	__fastcall	CLVM_GetContactHiddenStatus(HANDLE hContact, char *szStatus, struct ClcData *dat);  //clcitems.c
int		BgStatusBarChange(WPARAM wParam,LPARAM lParam);										//clcopts.c
int		ClcDoProtoAck(HANDLE wParam,ACKDATA * ack);											//clc.c
int		ModernSkinButtonDeleteAll();																	//modernbutton.c
int		GetAverageMode( void );																	//clisttray.c
int		GetContactCachedStatus(HANDLE hContact);											//clistsettings.c
INT_PTR	GetContactIcon(WPARAM wParam,LPARAM lParam);										//clistmod.c
int		GetContactIconC(pdisplayNameCacheEntry cacheEntry);									//clistmod.c
int		GetContactIndex(struct ClcGroup *group,struct ClcContact *contact);					//clcidents.c
int		GetStatusForContact(HANDLE hContact,char *szProto);									//clistsettings.c
int		InitCustomMenus(void);																//clistmenus.c
int		InitFramesMenus(void);																//framesmenus.c
int		LoadMoveToGroup();																	//movetogroup.c
int		LoadPositionsFromDB(BYTE * OrderPos);												//clistopts.c
int		LoadStatusBarData();																//modern_statusbar.c
int		MenuModulesLoaded(WPARAM wParam,LPARAM lParam);										//clistmenu.c
int		MenuModulesShutdown(WPARAM wParam,LPARAM lParam);									//clistmenu.c
int		MenuProcessCommand(WPARAM wParam,LPARAM lParam);									//clistmenu.c
int		ModifyMenuItemProxy(WPARAM wParam,LPARAM lParam);									//framesmenu.c
int		OnFrameTitleBarBackgroundChange(WPARAM wParam,LPARAM lParam);						//cluiframes.c
int		ProcessCommandProxy(WPARAM wParam,LPARAM lParam);									//framesmenu.c
int		QueueAllFramesUpdating (BYTE);														//cluiframes.c
int		RecursiveDeleteMenu(HMENU hMenu);													//clistmenus.c
int		ModernSkinButtonRedrawAll(HDC hdc);																//modern_button.c
int		RegisterButtonByParce(char * ObjectName, char * Params);							//mod_skin_selector.c
int		RestoreAllContactData(struct ClcData *dat);											//cache_funcs.c

int		SkinSelector_DeleteMask(MODERNMASK * mm);											//mod_skin_selector.c
int		StoreAllContactData(struct ClcData *dat);											//cache_func.c
INT_PTR	ToggleHideOffline(WPARAM wParam,LPARAM lParam);										//contact.c
INT_PTR	ToggleGroups(WPARAM wParam,LPARAM lParam);											//contact.c
INT_PTR	SetUseGroups(WPARAM wParam,LPARAM lParam);											//contact.c
INT_PTR	ToggleSounds(WPARAM wParam,LPARAM lParam);											//contact.c
int		UnitFramesMenu();																	//framesmenu.c
void	ClcOptionsChanged();																//clc.c
void	Docking_GetMonitorRectFromWindow(HWND hWnd,RECT *rc);								//Docking.c
void	DrawAvatarImageWithGDIp(HDC hDestDC,int x, int y, DWORD width, DWORD height, HBITMAP hbmp, int x1, int y1, DWORD width1, DWORD height1,DWORD flag,BYTE alpha);	//gdiplus.cpp
void	FreeRowCell();																		//RowHeight
void	InitGdiPlus();																		//gdiplus.cpp
void	InitTray();																			//clisttray.c
void	InvalidateDNCEbyPointer(HANDLE hContact,pdisplayNameCacheEntry pdnce,int SettingType);  //clistsettings.c
void	ReAssignExtraIcons();																//extraimage.c
void	ShutdownGdiPlus();																	//gdiplus.cpp
void	TextOutWithGDIp(HDC hDestDC, int x, int y, LPCTSTR lpString, int nCount);			//gdiplus.cpp
void	UninitCustomMenus();																//clistmenus.c
void	UnloadAvatarOverlayIcon();															//clc.c
void	UnLoadContactListModule();															//clistmod.c
void	UpdateAllAvatars(struct ClcData *dat);												//cache_func.c
											//cluiframes.c
void gtaRenewText(HANDLE hContact);
int ExtraImage_ExtraIDToColumnNum(int extra);
int ExtraImage_ColumnNumToExtraID(int column);

int		LoadSkinButtonModule();
void	UninitSkinHotKeys();
void	GetDefaultFontSetting(int i,LOGFONTA *lf,COLORREF *colour);
int		CLUI_OnSkinLoad(WPARAM wParam, LPARAM lParam);
HRESULT CluiLoadModule();
HRESULT PreLoadContactListModule();
HRESULT ClcLoadModule();
HRESULT ToolbarLoadModule();
HRESULT ToolbarButtonLoadModule();

// INTERFACES

void	cliCheckCacheItem(pdisplayNameCacheEntry pdnce);
void	cliFreeCacheItem( pdisplayNameCacheEntry p );
void	cliRebuildEntireList(HWND hwnd,struct ClcData *dat);
void	cliRecalcScrollBar(HWND hwnd,struct ClcData *dat);
void	CLUI_cliOnCreateClc(void);
int		cli_AddItemToGroup(struct ClcGroup *group, int iAboveItem);
int		cli_AddInfoItemToGroup(struct ClcGroup *group,int flags,const TCHAR *pszText);
int		cliGetGroupContentsCount(struct ClcGroup *group, int visibleOnly);
int		cliFindRowByText(HWND hwnd, struct ClcData *dat, const TCHAR *text, int prefixOk);
int		cliGetRowsPriorTo(struct ClcGroup *group,struct ClcGroup *subgroup,int contactIndex);
int		cli_IconFromStatusMode(const char *szProto,int nStatus, HANDLE hContact);
int		cli_RemoveEvent(HANDLE hContact, HANDLE hDbEvent);
void	cli_AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);
void	cli_DeleteItemFromTree(HWND hwnd, HANDLE hItem);
void	cli_FreeContact( struct ClcContact* );
void	cli_FreeGroup( struct ClcGroup* );
char*	cli_GetGroupCountsText(struct ClcData *dat, struct ClcContact *contact);
void	cli_ChangeContactIcon(HANDLE hContact,int iIcon,int add);
LRESULT cli_ProcessExternalMessages(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);
struct CListEvent* cliCreateEvent( void );
struct CListEvent* cli_AddEvent(CLISTEVENT *cle);
LRESULT CALLBACK cli_ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int		cliShowHide(WPARAM wParam,LPARAM lParam);
BOOL   CLUI__cliInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
int    cliCompareContacts(const struct ClcContact *contact1,const struct ClcContact *contact2);
int    cliFindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible);
void   cliTrayIconUpdateBase(const char *szChangedProto);
void   cliCluiProtocolStatusChanged(int status,const char * proto);
HMENU  cliBuildGroupPopupMenu(struct ClcGroup *group);
void   cliInvalidateDisplayNameCacheEntry(HANDLE hContact);
void   cliCheckCacheItem(pdisplayNameCacheEntry pdnce);
void   cli_SaveStateAndRebuildList(HWND hwnd, struct ClcData *dat);
void   CLUI_cli_LoadCluiGlobalOpts(void);
INT_PTR cli_TrayIconProcessMessage(WPARAM wParam,LPARAM lParam);
BOOL   CLUI__cliInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );

struct ClcContact* cliCreateClcContact( void );
ClcCacheEntryBase* cliCreateCacheItem(HANDLE hContact);
ClcCacheEntryBase* cliGetCacheEntry(HANDLE hContact);

// FUNCTION POINTERS
extern BOOL (WINAPI *g_proc_UpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
extern BOOL (WINAPI *g_proc_SetLayeredWindowAttributesNew)(HWND,COLORREF,BYTE,DWORD);

#define WM_DWMCOMPOSITIONCHANGED  0x031E

#define DWM_BB_ENABLE					0x00000001
#define DWM_BB_BLURREGION				0x00000002
#define DWM_BB_TRANSITIONONMAXIMIZED	0x00000004
struct DWM_BLURBEHIND
{
	DWORD dwFlags;
	BOOL fEnable;
	HRGN hRgnBlur;
	BOOL fTransitionOnMaximized;
};
extern HRESULT (WINAPI *g_proc_DWMEnableBlurBehindWindow)(HWND hWnd, DWM_BLURBEHIND *pBlurBehind);

extern tPaintCallbackProc CLCPaint_PaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData);
extern BOOL (WINAPI *MySetProcessWorkingSetSize)(HANDLE,SIZE_T,SIZE_T);

/* SkinEngine.c */


BYTE SkinDBGetContactSettingByte(HANDLE hContact, const char* szSection, const char*szKey, BYTE bDefault);

extern OVERLAYICONINFO g_pAvatarOverlayIcons[ID_STATUS_OUTTOLUNCH - ID_STATUS_OFFLINE + 1];
extern OVERLAYICONINFO g_pStatusOverlayIcons[ID_STATUS_OUTTOLUNCH - ID_STATUS_OFFLINE + 1];


#endif
