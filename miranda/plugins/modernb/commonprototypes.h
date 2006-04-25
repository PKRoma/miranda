#include "clist.h"
#include "CLUIFRAMES\cluiframes.h"



//Global variables
extern BOOL gl_b_GDIPlusFail;
extern BOOL gl_flag_OnEdgeSizing;

extern HANDLE gl_event_hSkinLoaded;         

extern struct TList_ModernMask *MainModernMaskList;
extern wndFrame *Frames;
extern int nFramescount;

extern RECT ON_EDGE_SIZING_POS;
extern int gl_i_BehindEdge_CurrentState;
extern int BehindEdgeSettings;
extern int sortBy[3];
extern tPaintCallbackProc ClcPaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData);
extern CLIST_INTERFACE * pcli;
extern BOOL LOCK_IMAGE_UPDATING;
extern ClcProtoStatus *clcProto;
extern HIMAGELIST himlCListClc;
extern HIMAGELIST hCListImages;

//External functions 

int BltBackImage (HWND destHWND, HDC destDC, RECT * BltClientRect);
BOOL cliInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
int GetProtocolVisibility(char * ProtoName);
int GetConnectingIconService(WPARAM wParam,LPARAM lParam);
BOOL mod_TextOut(HDC hdc, int x, int y, LPCTSTR lpString, int nCount);
BOOL mod_TextOutA(HDC hdc, int x, int y, char * lpString, int nCount);
BOOL mod_DrawTextA(HDC hdc, char * lpString, int nCount, RECT * lpRect, UINT format);
BOOL mod_DrawText(HDC hdc, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format);
int InvalidateFrameImage(WPARAM wParam, LPARAM lParam);       // Post request for updating
int UpdateTimer(BYTE BringIn);
int TestCursorOnBorders();
int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);
void DrawAvatarImageWithGDIp(HDC hDestDC,int x, int y, DWORD width, DWORD height, HBITMAP hbmp, int x1, int y1, DWORD width1, DWORD height1,DWORD flag,BYTE alpha);
void TextOutWithGDIp(HDC hDestDC, int x, int y, LPCTSTR lpString, int nCount);
void InitGdiPlus(void);
void ShutdownGdiPlus(void);
BOOL _inline WildCompare(char * name, char * mask, BYTE option);
void ApplyTransluency();
int DeleteAllSettingInSection(char * SectionName);
int ImageList_ReplaceIcon_FixAlpha(HIMAGELIST himl, int i, HICON hicon);
int SizingGetWindowRect(HWND hwnd,RECT * rc);
int JustUpdateWindowImageRect(RECT * rty);
char * GetParamN(char * string, char * buf, int buflen, BYTE paramN, char Delim, BOOL SkipSpaces);
int GetSkinFolder(char * szFileName, char * t2);
BOOL mod_AlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc,BLENDFUNCTION blendFunction);
int GetProtoIndexByPos(PROTOCOLDESCRIPTOR ** proto, int protoCnt, int Pos);
int BehindEdge_Show();
int UpdateTimer(BYTE BringIn);
int ClcProtoAck(WPARAM wParam,LPARAM lParam);
BOOL SetRectAlpha_255(HDC memdc,RECT *fr);
int cliShowHide(WPARAM wParam,LPARAM lParam);
int OnShowHide(HWND hwnd, int mode);
int BehindEdge_Show();
int BehindEdge_Hide();
int LoadStatusBarData();

int ReloadCLUIOptions();
wndFrame * FindFrameByItsHWND(HWND FrameHwnd);
//int CallTest(HDC hdc, int x, int y, char * Text);
BOOL mod_DrawIconEx(HDC hdc,int xLeft,int yTop,HICON hIcon,int cxWidth,int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);
int StartGDIPlus();
int TerminateGDIPlus();
BOOL SetRectAlpha_255(HDC memdc,RECT *fr);
int DrawTitleBar(HDC hdcMem2,RECT rect,int Frameid);
int UpdateFrameImage(WPARAM wParam, LPARAM lParam);
int OnSkinLoad(WPARAM wParam, LPARAM lParam);
DWORD mod_CalcHash(char * a);
int  QueueAllFramesUpdating (BYTE);
int  SetAlpha(BYTE);
int  DeleteButtons();
int RegisterButtonByParce(char * ObjectName, char * Params);
int RedrawButtons(HDC hdc);
int SizeFramesByWindowRect(RECT *r, HDWP * PosBatch, int mode);
int PrepeareImageButDontUpdateIt(RECT * r);
int UpdateWindowImageRect(RECT * r);
int CheckFramesPos(RECT *wr);
BOOL SetRgnAlpha_255(HDC memdc,HRGN hrgn);
int ReposButtons(HWND parent, BOOL draw, RECT * r);
char *DBGetStringA(HANDLE hContact,const char *szModule,const char *szSetting);
int CreateTimerForConnectingIcon(WPARAM,LPARAM);
BOOL ImageList_DrawEx_New( HIMAGELIST himl,int i,HDC hdcDst,int x,int y,int dx,int dy,COLORREF rgbBk,COLORREF rgbFg,UINT fStyle);
int OnMoving(HWND hwnd,RECT *lParam);
int CLUIFramesOnClistResize2(WPARAM wParam,LPARAM lParam, int mode);
BOOL cliInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
int BgStatusBarChange(WPARAM wParam,LPARAM lParam);
int IsInMainWindow(HWND hwnd);
int BgClcChange(WPARAM wParam,LPARAM lParam);
int BgMenuChange(WPARAM wParam,LPARAM lParam);
int OnFrameTitleBarBackgroundChange(WPARAM wParam,LPARAM lParam);
int UpdateFrameImage(WPARAM /*hWnd*/, LPARAM/*sPaintRequest*/);
int InvalidateFrameImage(WPARAM wParam, LPARAM lParam);
//void CacheContactAvatar(struct ClcData *dat, struct ClcContact *contact, BOOL changed);
int GetStatusForContact(HANDLE hContact,char *szProto);
int GetContactIconC(pdisplayNameCacheEntry cacheEntry);
int GetContactIcon(WPARAM wParam,LPARAM lParam);
int TestCursorOnBorders();
int SizingOnBorder(POINT ,int);

void InvalidateDNCEbyPointer(HANDLE hContact,pdisplayNameCacheEntry pdnce,int SettingType);
int InitCListEvents(void);
void UninitCListEvents(void);
int IsInMainWindow(HWND hwnd);
//external functions

//INTERFACES
BOOL	cliInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
int		cliCompareContacts(const struct ClcContact *contact1,const struct ClcContact *contact2);
int		cliCListTrayNotify(MIRANDASYSTRAYNOTIFY *msn);
int		cliFindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible);
void	cliTrayIconUpdateBase(char *szChangedProto);
void	cliTrayIconSetToBase(char *szPreferredProto);
void	cliTrayIconIconsChanged(void);
void	cliCluiProtocolStatusChanged(int status,const unsigned char * proto);
HMENU	cliBuildGroupPopupMenu(struct ClcGroup *group);
void	cliInvalidateDisplayNameCacheEntry(HANDLE hContact);
void	cliCheckCacheItem(pdisplayNameCacheEntry pdnce);
ClcCacheEntryBase* cliGetCacheEntry(HANDLE hContact);
void	cli_SaveStateAndRebuildList(HWND hwnd, struct ClcData *dat);
void	cli_LoadCluiGlobalOpts(void);
int		cli_TrayIconProcessMessage(WPARAM wParam,LPARAM lParam);
struct ClcContact* cliCreateClcContact( void );
ClcCacheEntryBase* cliCreateCacheItem(HANDLE hContact);

extern struct ClcGroup* ( *saveAddGroup )(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);
extern int ( *saveAddItemToGroup )( struct ClcGroup *group, int iAboveItem );
extern int ( *saveAddInfoItemToGroup )(struct ClcGroup *group,int flags,const TCHAR *pszText);
extern void ( *saveDeleteItemFromTree )(HWND hwnd, HANDLE hItem);
extern void ( *saveFreeContact )( struct ClcContact* );
extern void ( *saveFreeGroup )( struct ClcGroup* );
extern LRESULT ( CALLBACK *saveContactListControlWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern char* (*saveGetGroupCountsText)(struct ClcData *dat, struct ClcContact *contact);

int LoadMoveToGroup();
int GetContactIcon(WPARAM wParam,LPARAM lParam);
TCHAR *parseText(TCHAR *stzText);
int  LoadModernButtonModule();
void UnloadAvatarOverlayIcon();
void FreeRowCell ();
void UninitCustomMenus(void);
