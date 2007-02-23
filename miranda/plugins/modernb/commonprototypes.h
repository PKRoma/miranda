#ifndef _COMMONPROTOTYPES
#define _COMMONPROTOTYPES

#include "commonheaders.h"  //TO DO: Move contents of this file to commonheaders.h
#include "clist.h"
#include "CLUIFRAMES\cluiframes.h"
#include "modern_row.h"
#include "SkinEngine.h"
#include "mod_skin_selector.h"
#include "modern_statusbar.h"

#define SKIN  "ModernSkin"

extern PLUGININFO pluginInfo;
extern struct LIST_INTERFACE li;
extern CLIST_INTERFACE * pcli;

//Global variables
extern HANDLE g_hSkinLoadedEvent;
extern int ON_SETALLEXTRAICON_CYCLE;
extern BOOL CLM_AUTOREBUILD_WAS_POSTED;
extern wndFrame *Frames;
extern int nFramescount;
extern RECT g_rcEdgeSizingRect;
extern wndFrame *wndFrameEventArea;
extern ROWCELL * gl_RowTabAccess[];
extern ROWCELL * gl_RowRoot;
extern HIMAGELIST hAvatarOverlays;
extern int  g_nTitleBarHeight;
extern int  g_nGapBetweenTitlebar;
extern BOOL g_bTransparentFlag;
extern ClcProtoStatus *clcProto;
extern HIMAGELIST himlCListClc;
extern HIMAGELIST hCListImages;
extern BOOL g_mutex_bSizing;
extern BOOL LOCK_RECALC_SCROLLBAR;
extern HIMAGELIST himlCListClc;
extern int currentDesiredStatusMode;
extern BYTE nameOrder[];
extern SortedList lContactsCache;
extern BOOL g_flag_bOnModulesLoadedCalled;
extern HIMAGELIST hCListImages;
extern HANDLE hContactIconChangedEvent;
extern SKINOBJECTSLIST g_SkinObjectList;
extern CURRWNDIMAGEDATA * g_pCachedWindow;
extern BOOL g_mutex_bLockUpdating;
extern LISTMODERNMASK *MainModernMaskList;
extern HIMAGELIST hCListImages;
extern wndFrame *wndFrameViewMode;
extern STATUSBARDATA g_StatusBarData;
extern SKINOBJECTSLIST g_SkinObjectList;
extern void (*saveLoadCluiGlobalOpts)(void);
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
extern PLUGININFO pluginInfo;
extern HICON listening_to_icon;
extern BOOL glOtherSkinWasLoaded;
extern BYTE glSkinWasModified;
extern HWND g_hCLUIOptionsWnd;
extern BOOL g_bTransparentFlag;
extern HINSTANCE g_hInst;
extern HIMAGELIST hCListImages;
extern HFONT TitleBarFont;
extern BOOL g_mutex_bChangingMode;
extern HANDLE g_hMainThread;
extern DWORD g_dwMainThreadID;
extern DWORD g_dwAskAwayMsgThreadID;
extern DWORD g_dwGetTextThreadID;
extern DWORD g_dwSmoothAnimationThreadID;
extern DWORD g_dwFillFontListThreadID;
extern HWND  g_hwndViewModeFrame;
extern HANDLE hSmileyAddOptionsChangedHook,hAvatarChanged,hIconChangedHook;
extern BYTE gl_TrimText;



//External functions
#ifndef __cplusplus

const ROWCELL * rowAddCell(pROWCELL*, int );
void rowDeleteTree(pROWCELL cell);
BOOL rowParse(pROWCELL*cell, ROWCELL* parent, char *tbuf, int *hbuf, int *sequence, ROWCELL* *RowTabAccess );
void rowSizeWithReposition(pROWCELL* root, int width);

#endif //__cplusplus

/************************************************************************/
/*                              PROTOTYPES                              */
/************************************************************************/

/* CLCItems */
BOOL	CLCItems_IsShowOfflineGroup(struct ClcGroup* group);

/* CLCPaint */
HFONT	CLCPaint_ChangeToFont(HDC hdc,struct ClcData *dat,int id,int *fontHeight);
int		CLCPaint_GetBasicFontID(struct ClcContact * contact);
int		CLCPaint_GetRealStatus(struct ClcContact * contact, int status);
void	CLCPaint_GetTextSize(SIZE *text_size, HDC hdcMem, RECT free_row_rc, TCHAR *szText, SortedList *plText, UINT uTextFormat, int smiley_height);

/* CListMod */
int		CListMod_HideWindow(HWND hwndContactList, int mode);

/* CLUI */
void	CLUI_ChangeWindowMode();
BOOL	CLUI_CheckOwnedByClui(HWND hwnd);
void	CLUI_DisconnectAll();
int		CLUI_GetConnectingIconService(WPARAM wParam,LPARAM lParam);
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

/* CLUIServices */
int		CLUIServices_ProtocolStatusChanged(WPARAM wParam,LPARAM lParam);

/* GDIPlus */
BOOL    GDIPlus_AlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc, BLENDFUNCTION * blendFunction);
HBITMAP GDIPlus_LoadGlyphImage(char *szFileName);

/* EventArea */
void	EventArea_ConfigureEventArea();

/* ExtraImage */
void	ExtraImage_SetAllExtraIcons(HWND hwndList,HANDLE hContact);

/* ModernButton */
int		ModernButton_AddButton(HWND parent,char * ID,char * CommandService,char * StateDefService,char * HandeService,             int Left, int Top, int Right, int Bottom, DWORD AlignedTo,TCHAR * Hint,char * DBkey,char * TypeDef,int MinWidth, int MinHeight);
int		ModernButton_LoadModule();
int		ModernButton_ReposButtons(HWND parent, BOOL draw, RECT * r);
int     ModernButton_UnloadModule(WPARAM,LPARAM);

/* RowHeight */
int		RowHeight_CalcRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, int item);

/* SkinEngine */
BOOL	SkinEngine_AlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc,BLENDFUNCTION blendFunction);
void	SkinEngine_ApplyTransluency(void);
int		SkinEngine_BltBackImage (HWND destHWND, HDC destDC, RECT * BltClientRect);
HBITMAP SkinEngine_CreateDIB32(int cx, int cy);
HBITMAP SkinEngine_CreateDIB32Point(int cx, int cy, void ** bits);
HICON	SkinEngine_CreateJoinedIcon(HICON hBottom, HICON hTop,BYTE alpha);
int		SkinEngine_DrawImageAt(HDC hdc, RECT *rc);
BOOL	SkinEngine_DrawIconEx(HDC hdc,int xLeft,int yTop,HICON hIcon,int cxWidth,int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);
int     SkinEngine_DrawNonFramedObjects(BOOL Erase,RECT *r);
BOOL	SkinEngine_DrawText(HDC hdc, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format);
BOOL	SkinEngine_DrawTextA(HDC hdc, char * lpString, int nCount, RECT * lpRect, UINT format);
LPSKINOBJECTDESCRIPTOR	SkinEngine_FindObjectByName(const char * szName, BYTE objType, SKINOBJECTSLIST* Skin);
HBITMAP SkinEngine_GetCurrentWindowImage();
int		SkinEngine_GetFullFilename(char * buf, char *file, char * skinfolder,BOOL madeAbsolute);
int		SkinEngine_GetSkinFolder(char * szFileName, char * t2);
BOOL	SkinEngine_ImageList_DrawEx( HIMAGELIST himl,int i,HDC hdcDst,int x,int y,int dx,int dy,COLORREF rgbBk,COLORREF rgbFg,UINT fStyle);
HICON	SkinEngine_ImageList_GetIcon(HIMAGELIST himl, int i, UINT fStyle);
int     SkinEngine_JustUpdateWindowImageRect(RECT * rty);
HBITMAP SkinEngine_LoadGlyphImage(char * szFileName);
int     SkinEngine_LoadModule();
void	SkinEngine_LoadSkinFromDB(void);
int		SkinEngine_LoadSkinFromIniFile(char*);
TCHAR*	SkinEngine_ParseText(TCHAR *stzText);
int		SkinEngine_PrepeareImageButDontUpdateIt(RECT * r);
int     SkinEngine_ReCreateBackImage(BOOL Erase,RECT *w);
int		SkinEngine_RedrawCompleteWindow();
BOOL	SkinEngine_ResetTextEffect(HDC);
BOOL	SkinEngine_SelectTextEffect(HDC hdc, BYTE EffectID, DWORD FirstColor, DWORD SecondColor);
int		SkinEngine_Service_DrawGlyph(WPARAM wParam,LPARAM lParam);
BOOL	SkinEngine_SetRectOpaque(HDC memdc,RECT *fr);
BOOL	SkinEngine_SetRgnOpaque(HDC memdc,HRGN hrgn);
BOOL	SkinEngine_TextOut(HDC hdc, int x, int y, LPCTSTR lpString, int nCount);
BOOL	SkinEngine_TextOutA(HDC hdc, int x, int y, char * lpString, int nCount);
int     SkinEngine_UnloadGlyphImage(HBITMAP hbmp);
int     SkinEngine_UnloadModule();
int     SkinEngine_UpdateWindowImage();
int     SkinEngine_UpdateWindowImageRect(RECT * lpRect);
int     SkinEngine_ValidateFrameImageProc(RECT * r);

/* CLUIFrames.c PROXIED */

int callProxied_OnFrameTitleBarBackgroundChange(WPARAM wParam,LPARAM lParam);
int callProxied_CLUIFrames_ActivateSubContainers(BOOL wParam);
int callProxied_CLUIFrames_OnClistResize_mod(WPARAM wParam,LPARAM lParam);
int callProxied_CLUIFrames_OnMoving(HWND hwnd,RECT *lParam);
int callProxied_CLUIFrames_OnShowHide(HWND hwnd, int mode);
int callProxied_CLUIFrames_SetLayeredMode(BOOL fLayeredMode,HWND hwnd);
int callProxied_CLUIFrames_SetParentForContainers(HWND parent);
int callProxied_CLUIFramesOnClistResize(WPARAM wParam,LPARAM lParam);
int callProxied_DrawTitleBar(HDC hdcMem2,RECT * rect,int Frameid);
int callProxied_FindFrameID(HWND FrameHwnd);
int callProxied_QueueAllFramesUpdating(BYTE queue);
int callProxied_SetAlpha(BYTE Alpha);
int callProxied_SizeFramesByWindowRect(RECT *r, HDWP * PosBatch, int mode);


/* others TODO: move above */
BOOL    wildcmp(char * name, char * mask, BYTE option);										//mod_skin_selector.c
BOOL	wildcmpi(char * name, char * mask);													//mod_skin_selector.c
BOOL	CALLBACK DlgSkinEditorOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);	//SkinEditor.c
BOOL	CALLBACK DlgProcHotKeyOpts2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);	//Keyboard.c
BOOL	CALLBACK DlgTmplEditorOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);	//RowTemplate.c
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
int		ClcProtoAck(WPARAM wParam,LPARAM lParam);											//clc.c
int		DeleteButtons();																	//modernbutton.c
int		GetAverageMode( void );																	//clisttray.c
int		GetContactCachedStatus(HANDLE hContact);											//clistsettings.c
int		GetContactIcon(WPARAM wParam,LPARAM lParam);										//clistmod.c
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
int		RedrawButtons(HDC hdc);																//modern_button.c
int		RegisterButtonByParce(char * ObjectName, char * Params);							//mod_skin_selector.c
int		RestoreAllContactData(struct ClcData *dat);											//cache_funcs.c
int		callProxied_SetAlpha(BYTE bAlpha);																//cluiframes.c

int		SkinSelector_DeleteMask(MODERNMASK * mm);											//mod_skin_selector.c
int		StoreAllContactData(struct ClcData *dat);											//cache_func.c
int		ToggleHideOffline(WPARAM wParam,LPARAM lParam);										//contact.c
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

// INTERFACES
int    cliShowHide(WPARAM wParam,LPARAM lParam);
BOOL   CLUI__cliInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
int    cliCompareContacts(const struct ClcContact *contact1,const struct ClcContact *contact2);
int    cliFindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible);
void   cliTrayIconUpdateBase(const char *szChangedProto);
void   cliCluiProtocolStatusChanged(int status,const unsigned char * proto);
HMENU  cliBuildGroupPopupMenu(struct ClcGroup *group);
void   cliInvalidateDisplayNameCacheEntry(HANDLE hContact);
void   cliCheckCacheItem(pdisplayNameCacheEntry pdnce);
void   cli_SaveStateAndRebuildList(HWND hwnd, struct ClcData *dat);
void   CLUI_cli_LoadCluiGlobalOpts(void);
int    cli_TrayIconProcessMessage(WPARAM wParam,LPARAM lParam);
BOOL   CLUI__cliInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );

struct ClcContact* cliCreateClcContact( void );
ClcCacheEntryBase* cliCreateCacheItem(HANDLE hContact);
ClcCacheEntryBase* cliGetCacheEntry(HANDLE hContact);

// FUNCTION POINTERS

extern struct CListEvent* ( *saveAddEvent )( CLISTEVENT *cle );
extern struct ClcGroup* ( *saveAddGroup )(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);

extern void    ( *saveAddContactToTree )( HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);
extern int     ( *saveAddItemToGroup )( struct ClcGroup *group, int iAboveItem );
extern int     ( *saveAddInfoItemToGroup )( struct ClcGroup *group,int flags,const TCHAR *pszText);
extern void    ( *saveChangeContactIcon )( HANDLE hContact,int iIcon,int add );
extern void    ( *saveDeleteItemFromTree )( HWND hwnd, HANDLE hItem);
extern void    ( *saveFreeContact )( struct ClcContact* );
extern void    ( *saveFreeGroup )( struct ClcGroup* );
extern char*   ( *saveGetGroupCountsText )( struct ClcData *dat, struct ClcContact *contact);
extern int     ( *saveIconFromStatusMode )( const char *szProto,int nStatus, HANDLE hContact);
extern LRESULT ( *saveProcessExternalMessages )(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);
extern int     ( *saveRemoveEvent )( HANDLE hContact, HANDLE hDbEvent );
extern void    ( *saveSortCLC )(HWND hwnd, struct ClcData *dat, int useInsertionSort );
extern int     ( *saveTrayIconProcessMessage )( WPARAM wParam, LPARAM lParam );

extern BOOL (WINAPI *g_proc_UpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
extern BOOL (WINAPI *g_proc_SetLayeredWindowAttributesNew)(HWND,COLORREF,BYTE,DWORD);

extern tPaintCallbackProc CLCPaint_PaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData);
extern LRESULT ( CALLBACK *saveContactListWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL (WINAPI *pfEnableThemeDialogTexture)(HANDLE, DWORD);
extern BOOL (WINAPI *MySetProcessWorkingSetSize)(HANDLE,SIZE_T,SIZE_T);
extern LRESULT ( CALLBACK *saveContactListControlWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
