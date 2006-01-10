#include "clist.h"
#include "CLUIFRAMES\cluiframes.h"
extern int BltBackImage (HWND destHWND, HDC destDC, RECT * BltClientRect);
extern BOOL skinInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern int GetProtocolVisibility(char * ProtoName);
extern int GetConnectingIconService(WPARAM wParam,LPARAM lParam);
extern BOOL TextOutS(HDC hdc, int x, int y, LPCTSTR lpString, int nCount);
extern BOOL TextOutSA(HDC hdc, int x, int y, char * lpString, int nCount);
extern BOOL DrawTextSA(HDC hdc, char * lpString, int nCount, RECT * lpRect, UINT format);
extern BOOL DrawTextS(HDC hdc, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format);
extern int InvalidateFrameImage(WPARAM wParam, LPARAM lParam);       // Post request for updating
extern int UpdateTimer(BYTE BringIn);
extern int TestCursorOnBorders();
extern int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);

extern void DrawAvatarImageWithGDIp(HDC hDestDC,int x, int y, DWORD width, DWORD height, HBITMAP hbmp, int x1, int y1, DWORD width1, DWORD height1,DWORD flag,BYTE alpha);
extern void TextOutWithGDIp(HDC hDestDC, int x, int y, LPCTSTR lpString, int nCount);
extern void InitGdiPlus(void);
extern void ShutdownGdiPlus(void);
extern BYTE gdiPlusFail; 
extern BOOL _inline WildCompare(char * name, char * mask, BYTE option);
extern void ApplyTransluency();

extern int DeleteAllSettingInSection(char * SectionName);
extern int ImageList_ReplaceIcon_FixAlpha(HIMAGELIST himl, int i, HICON hicon);

extern int SizingGetWindowRect(HWND hwnd,RECT * rc);
extern int JustUpdateWindowImageRect(RECT * rty);

extern char * GetParamN(char * string, char * buf, int buflen, BYTE paramN, char Delim, BOOL SkipSpaces);
extern int GetSkinFolder(char * szFileName, char * t2);
extern BOOL MyAlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc,BLENDFUNCTION blendFunction);
extern int GetProtoIndexByPos(PROTOCOLDESCRIPTOR ** proto, int protoCnt, int Pos);
extern int BehindEdge_Show();
extern int UpdateTimer(BYTE BringIn);
extern int ClcProtoAck(WPARAM wParam,LPARAM lParam);
extern BOOL FillRect255Alpha(HDC memdc,RECT *fr);
extern int ShowHide(WPARAM wParam,LPARAM lParam);
extern int OnShowHide(HWND hwnd, int mode);
extern int BehindEdge_Show();
extern int BehindEdge_Hide();
extern int LoadStatusBarData();

extern int ReloadCLUIOptions();

//External

extern HANDLE hSkinLoaded;                        
extern struct ModernMaskList *MainModernMaskList;
extern wndFrame * FindFrameByItsHWND(HWND FrameHwnd);
extern wndFrame *Frames;
extern int nFramescount;
extern BOOL ON_EDGE_SIZING;
extern RECT ON_EDGE_SIZING_POS;

//External Sub
extern int DrawTitleBar(HDC dc,RECT rect,int Frameid);
extern int UpdateFrameImage(WPARAM wParam, LPARAM lParam);
extern int OnSkinLoad(WPARAM wParam, LPARAM lParam);
extern DWORD ModernCalcHash(char * a);
extern int  QueueAllFramesUpdating (BYTE);
extern int  SetAlpha(BYTE);
extern int  DeleteButtons();
extern int RegisterButtonByParce(char * ObjectName, char * Params);
extern int RedrawButtons(HDC hdc);
extern int SizeFramesByWindowRect(RECT *r, HDWP * PosBatch, int mode);
extern int PrepeareImageButDontUpdateIt(RECT * r);
extern int UpdateWindowImageRect(RECT * r);
extern int CheckFramesPos(RECT *wr);
extern BOOL FillRgn255Alpha(HDC memdc,HRGN hrgn);

extern int ReposButtons(HWND parent, BOOL draw, RECT * r);

extern char *DBGetStringA(HANDLE hContact,const char *szModule,const char *szSetting);
extern int CreateTimerForConnectingIcon(WPARAM,LPARAM);
extern BOOL ImageList_DrawEx_New( HIMAGELIST himl,int i,HDC hdcDst,int x,int y,int dx,int dy,COLORREF rgbBk,COLORREF rgbFg,UINT fStyle);

extern int OnMoving(HWND hwnd,RECT *lParam);
extern int CLUIFramesOnClistResize2(WPARAM wParam,LPARAM lParam, int mode);

extern BOOL skinInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern int BgStatusBarChange(WPARAM wParam,LPARAM lParam);

extern int IsInMainWindow(HWND hwnd);
extern int BgClcChange(WPARAM wParam,LPARAM lParam);
extern int BgMenuChange(WPARAM wParam,LPARAM lParam);
extern int OnFrameTitleBarBackgroundChange(WPARAM wParam,LPARAM lParam);
extern int UpdateFrameImage(WPARAM /*hWnd*/, LPARAM/*sPaintRequest*/);
extern int InvalidateFrameImage(WPARAM wParam, LPARAM lParam);

extern void CacheContactAvatar(struct ClcData *dat, struct ClcContact *contact, BOOL changed);

extern int GetStatusForContact(HANDLE hContact,char *szProto);
extern int GetContactIconC(pdisplayNameCacheEntry cacheEntry);
extern int GetContactIcon(WPARAM wParam,LPARAM lParam);

extern int TestCursorOnBorders();
extern int SizingOnBorder(POINT ,int);

extern int BehindEdge_State;
extern int BehindEdgeSettings;
extern int sortBy[3];

extern tPaintCallbackProc ClcPaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData);
extern LRESULT ( CALLBACK *saveContactListControlWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern void CheckPDNCE(pdisplayNameCacheEntry pdnce);

extern int ( *saveAddItemToGroup )( struct ClcGroup *group, int iAboveItem );
extern int ( *saveAddInfoItemToGroup )(struct ClcGroup *group,int flags,const TCHAR *pszText);

extern void ( *saveDeleteItemFromTree )(HWND hwnd, HANDLE hItem);
extern void ( *saveFreeContact )( struct ClcContact* );
extern void ( *saveFreeGroup )( struct ClcGroup* );

extern CLIST_INTERFACE * pcli;

extern struct ClcGroup* ( *saveAddGroup )(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);

//routines for managing adding/removal of items in the list, including sorting
extern BOOL skinInvalidateRect(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern int CompareContacts(const struct ClcContact *contact1,const struct ClcContact *contact2);

extern BOOL FillRect255Alpha(HDC memdc,RECT *fr);
extern BOOL LOCK_IMAGE_UPDATING;

extern int StartGDIPlus();
extern int TerminateGDIPlus();
extern BYTE gdiPlusFail;
extern int CallTest(HDC hdc, int x, int y, char * Text);
extern BOOL DrawIconExS(HDC hdc,int xLeft,int yTop,HICON hIcon,int cxWidth,int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags);

//extern int hClcProtoCount;
extern int IsInMainWindow(HWND hwnd);
extern ClcProtoStatus *clcProto;
extern HIMAGELIST himlCListClc;
extern HIMAGELIST hCListImages;
extern void InvalidateDisplayNameCacheEntryByPDNE(HANDLE hContact,pdisplayNameCacheEntry pdnce,int SettingType);
extern int TrayIconProcessMessage(WPARAM wParam,LPARAM lParam);

extern int InitCListEvents(void);
extern void UninitCListEvents(void);