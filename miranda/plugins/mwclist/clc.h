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
#ifndef _CLC_H_
#define _CLC_H_
#include "dblists.h"

#define HCONTACT_ISGROUP    0x80000000
#define HCONTACT_ISINFO     0xFFFF0000
#define IsHContactGroup(h)  (((unsigned)(h)^HCONTACT_ISGROUP)<(HCONTACT_ISGROUP^HCONTACT_ISINFO))
#define IsHContactInfo(h)   (((unsigned)(h)&HCONTACT_ISINFO)==HCONTACT_ISINFO)
#define IsHContactContact(h) (((unsigned)(h)&HCONTACT_ISGROUP)==0)
#define MAXEXTRACOLUMNS     16
#define MAXSTATUSMSGLEN		256

#define INTM_NAMECHANGED     (WM_USER+10)
#define INTM_ICONCHANGED     (WM_USER+11)
#define INTM_GROUPCHANGED    (WM_USER+12)
#define INTM_GROUPSCHANGED   (WM_USER+13)
#define INTM_CONTACTADDED    (WM_USER+14)
#define INTM_CONTACTDELETED  (WM_USER+15)
#define INTM_HIDDENCHANGED   (WM_USER+16)
#define INTM_INVALIDATE      (WM_USER+17)
#define INTM_APPARENTMODECHANGED (WM_USER+18)
#define INTM_SETINFOTIPHOVERTIME (WM_USER+19)
#define INTM_NOTONLISTCHANGED   (WM_USER+20)
#define INTM_RELOADOPTIONS   (WM_USER+21)
#define INTM_NAMEORDERCHANGED (WM_USER+22)
#define INTM_IDLECHANGED         (WM_USER+23)
#define INTM_SCROLLBARCHANGED (WM_USER+24)
#define INTM_PROTOCHANGED (WM_USER+25)
#define INTM_STATUSMSGCHANGED	(WM_USER+26)

#define TIMERID_RENAME         10
#define TIMERID_DRAGAUTOSCROLL 11
#define TIMERID_INFOTIP        13
#define TIMERID_REBUILDAFTER   14
#define TIMERID_DELAYEDRESORTCLC   15
#define TIMERID_DELAYEDREPAINT   16
#define TIMERID_SUBEXPAND 21

struct ClcGroup;

#define CONTACTF_ONLINE    1
#define CONTACTF_INVISTO   2
#define CONTACTF_VISTO     4
#define CONTACTF_NOTONLIST 8
#define CONTACTF_CHECKED   16
#define CONTACTF_IDLE      32
#define CONTACTF_STATUSMSG 64

struct ClcContact {
	BYTE type;
	BYTE flags;
	struct ClcContact *subcontacts;
	BYTE SubAllocated;
	BYTE SubExpanded;
	BYTE isSubcontact;
	union {
		struct {
			WORD iImage;
			HANDLE hContact;
		};
		struct {
			WORD groupId;
			struct ClcGroup *group;
		};
	};
	BYTE iExtraImage[MAXEXTRACOLUMNS];
	char szText[120-MAXEXTRACOLUMNS];
	char * proto;	// MS_PROTO_GETBASEPROTO
	char szStatusMsg[MAXSTATUSMSGLEN];
};

#define GROUP_ALLOCATE_STEP  8
struct ClcGroup {
	int contactCount,allocedCount;
	struct ClcContact *contact;
	int expanded,hideOffline,groupId;
	struct ClcGroup *parent;
	int scanIndex;
	int totalMembers;
};

struct ClcFontInfo {
	HFONT hFont;
	int fontHeight,changed;
	COLORREF colour;
};

typedef struct {
	char *szProto;
	DWORD dwStatus;	
} ClcProtoStatus;

#define DRAGSTAGE_NOTMOVED  0
#define DRAGSTAGE_ACTIVE    1
#define DRAGSTAGEM_STAGE    0x00FF
#define DRAGSTAGEF_MAYBERENAME  0x8000
#define DRAGSTAGEF_OUTSIDE      0x4000
struct ClcData {
	struct ClcGroup list;
	int rowHeight;
	int yScroll;
	int selection;
	struct ClcFontInfo fontInfo[FONTID_MAX+1];
	int scrollTime;
	HIMAGELIST himlHighlight;
	int groupIndent;
	char szQuickSearch[128];
	int iconXSpace;
	HWND hwndRenameEdit;
	COLORREF bkColour,selBkColour,selTextColour,hotTextColour,quickSearchColour;
	int iDragItem,iInsertionMark;
	int dragStage;
	POINT ptDragStart;
	int dragAutoScrolling;
	int dragAutoScrollHeight;
	int leftMargin;
	int insertionMarkHitHeight;
	HBITMAP hBmpBackground;
	int backgroundBmpUse,bkChanged;
	int iHotTrack;
	int gammaCorrection;
	DWORD greyoutFlags;			  //see m_clc.h
	DWORD offlineModes;
	DWORD exStyle;
	DWORD style;
	POINT ptInfoTip;
	int infoTipTimeout;
	HANDLE hInfoTipItem;
	HIMAGELIST himlExtraColumns;
	int extraColumnsCount;
	int extraColumnSpacing;
	int checkboxSize;
	int showSelAlways;
	int showIdle;
	int noVScrollbar;
	int NeedResort;
	SortedList lCLCContactsCache;
	BYTE HiLightMode;
	BYTE doubleClickExpand;
	int MetaIgnoreEmptyExtra;
};

//clc.c
void ClcOptionsChanged(void);

//clcidents.c
int GetRowsPriorTo(struct ClcGroup *group,struct ClcGroup *subgroup,int contactIndex);
int FindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible);
int GetRowByIndex(struct ClcData *dat,int testindex,struct ClcContact **contact,struct ClcGroup **subgroup);
HANDLE ContactToHItem(struct ClcContact *contact);
HANDLE ContactToItemHandle(struct ClcContact *contact,DWORD *nmFlags);
void ClearRowByIndexCache();

//clcitems.c
struct ClcGroup *AddGroup(HWND hwnd,struct ClcData *dat,const char *szName,DWORD flags,int groupId,int calcTotalMembers);
void FreeGroup(struct ClcGroup *group);
int AddInfoItemToGroup(struct ClcGroup *group,int flags,const char *pszText);
void RebuildEntireList(HWND hwnd,struct ClcData *dat);
struct ClcGroup *RemoveItemFromGroup(HWND hwnd,struct ClcGroup *group,struct ClcContact *contact,int updateTotalCount);
void DeleteItemFromTree(HWND hwnd,HANDLE hItem);
void AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);
void SortCLC(HWND hwnd,struct ClcData *dat,int useInsertionSort);
int GetGroupContentsCount(struct ClcGroup *group,int visibleOnly);
int GetNewSelection(struct ClcGroup *group,int selection, int direction);
void SaveStateAndRebuildList(HWND hwnd,struct ClcData *dat);

//clcmsgs.c
LRESULT ProcessExternalMessages(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);

//clcutils.c
void EnsureVisible(HWND hwnd,struct ClcData *dat,int iItem,int partialOk);
void RecalcScrollBar(HWND hwnd,struct ClcData *dat);
void SetGroupExpand(HWND hwnd,struct ClcData *dat,struct ClcGroup *group,int newState);
void DoSelectionDefaultAction(HWND hwnd,struct ClcData *dat);
int FindRowByText(HWND hwnd,struct ClcData *dat,const char *text,int prefixOk);
void EndRename(HWND hwnd,struct ClcData *dat,int save);
void DeleteFromContactList(HWND hwnd,struct ClcData *dat);
void BeginRenameSelection(HWND hwnd,struct ClcData *dat);
char *GetGroupCountsText(struct ClcData *dat,struct ClcContact *contact);
int HitTest(HWND hwnd,struct ClcData *dat,int testx,int testy,struct ClcContact **contact,struct ClcGroup **group,DWORD *flags);
void ScrollTo(HWND hwnd,struct ClcData *dat,int desty,int noSmooth);
#define DROPTARGET_OUTSIDE    0
#define DROPTARGET_ONSELF     1
#define DROPTARGET_ONNOTHING  2
#define DROPTARGET_ONGROUP    3
#define DROPTARGET_ONCONTACT  4
#define DROPTARGET_INSERTION  5
int GetDropTargetInformation(HWND hwnd,struct ClcData *dat,POINT pt);
int ClcStatusToPf2(int status);
int IsHiddenMode(struct ClcData *dat,int status);
void HideInfoTip(HWND hwnd,struct ClcData *dat);
void NotifyNewContact(HWND hwnd,HANDLE hContact);
void LoadClcOptions(HWND hwnd,struct ClcData *dat);
void RecalculateGroupCheckboxes(HWND hwnd,struct ClcData *dat);
void SetGroupChildCheckboxes(struct ClcGroup *group,int checked);
void InvalidateItem(HWND hwnd,struct ClcData *dat,int iItem);

//clcpaint.c
void PaintClc(HWND hwnd,struct ClcData *dat,HDC hdc,RECT *rcPaint);

//clcopts.c
int ClcOptInit(WPARAM wParam,LPARAM lParam);
DWORD GetDefaultExStyle(void);
void GetFontSetting(int i,LOGFONT *lf,COLORREF *colour);

//clcfiledrop.c
void InitFileDropping(void);
void FreeFileDropping(void);
void RegisterFileDropping(HWND hwnd);
void UnregisterFileDropping(HWND hwnd);


int GetContactCachedStatus(HANDLE hContact);
char *GetContactCachedProtocol(HANDLE hContact);

#define CLCDEFAULT_ROWHEIGHT     17
#define CLCDEFAULT_EXSTYLE       (CLS_EX_EDITLABELS|CLS_EX_TRACKSELECT|CLS_EX_SHOWGROUPCOUNTS|CLS_EX_HIDECOUNTSWHENEMPTY|CLS_EX_TRACKSELECT|CLS_EX_NOTRANSLUCENTSEL)  //plus CLS_EX_NOSMOOTHSCROLL is got from the system
#define CLCDEFAULT_SCROLLTIME    150
#define CLCDEFAULT_GROUPINDENT   5
#define CLCDEFAULT_BKCOLOUR      GetSysColor(COLOR_3DFACE)
#define CLCDEFAULT_USEBITMAP     0
#define CLCDEFAULT_BKBMPUSE      CLB_STRETCH
#define CLCDEFAULT_OFFLINEMODES  MODEF_OFFLINE
#define CLCDEFAULT_GREYOUTFLAGS  0
#define CLCDEFAULT_FULLGREYOUTFLAGS  (MODEF_OFFLINE|PF2_INVISIBLE|GREYF_UNFOCUS)
#define CLCDEFAULT_SELBKCOLOUR   GetSysColor(COLOR_HIGHLIGHT)
#define CLCDEFAULT_SELTEXTCOLOUR GetSysColor(COLOR_HIGHLIGHTTEXT)
#define CLCDEFAULT_HOTTEXTCOLOUR (IsWinVer98Plus()?RGB(0,0,255):GetSysColor(COLOR_HOTLIGHT))
#define CLCDEFAULT_QUICKSEARCHCOLOUR RGB(255,255,0)
#define CLCDEFAULT_LEFTMARGIN    0
#define CLCDEFAULT_GAMMACORRECT  1
#define CLCDEFAULT_SHOWIDLE      0

#define CLUI_SetDrawerService "CLUI/SETDRAWERSERVICE"
typedef struct {
	int cbSize;
	char *PluginName;
	char *Comments;
	char *GetDrawFuncsServiceName;

} DrawerServiceStruct,*pDrawerServiceStruct ;

#define CLUI_EXT_FUNC_PAINTCLC	1

typedef struct {
	int cbSize;
	void (*PaintClc)(HWND,struct ClcData *,HDC,RECT *,int ,ClcProtoStatus *,HIMAGELIST);

} ExternDrawer,*pExternDrawer ;

ExternDrawer SED;




#endif _CLC_H_