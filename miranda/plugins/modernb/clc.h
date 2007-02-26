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
#ifndef _CLC_H_
#define _CLC_H_
#include "image_array.h"


#define SETTING_TOOLWINDOW_DEFAULT   1
#define SETTING_SHOWMAINMENU_DEFAULT 1
#define SETTING_SHOWCAPTION_DEFAULT  1
#define SETTING_CLIENTDRAG_DEFAULT   1
#define SETTING_ONTOP_DEFAULT        0
#define SETTING_MIN2TRAY_DEFAULT     1
#define SETTING_TRAY1CLICK_DEFAULT   0
#define SETTING_HIDEOFFLINE_DEFAULT  0
#define SETTING_HIDEEMPTYGROUPS_DEFAULT  0
#define SETTING_USEGROUPS_DEFAULT    1

#define SETTING_SORTBY1_DEFAULT      2
#define SETTING_SORTBY2_DEFAULT      1
#define SETTING_SORTBY3_DEFAULT      0

#define SETTING_NOOFFLINEBOTTOM_DEFAULT 0

#define SETTING_TRANSPARENT_DEFAULT  0
#define SETTING_AUTOALPHA_DEFAULT    150
#define SETTING_CONFIRMDELETE_DEFAULT 1
#define SETTING_AUTOHIDE_DEFAULT     0
#define SETTING_HIDETIME_DEFAULT     30
#define SETTING_CYCLETIME_DEFAULT    4
#define SETTING_TRAYICON_DEFAULT     SETTING_TRAYICON_SINGLE
#define SETTING_ALWAYSSTATUS_DEFAULT 0
#define SETTING_ALWAYSMULTI_DEFAULT  0

#define SETTING_TRAYICON_SINGLE   0
#define SETTING_TRAYICON_CYCLE    1
#define SETTING_TRAYICON_MULTI    2

#define NIIF_INTERN_UNICODE 0x00000100

#define SETTING_STATE_HIDDEN      0
#define SETTING_STATE_MINIMIZED   1
#define SETTING_STATE_NORMAL      2

#define SETTING_BRINGTOFRONT_DEFAULT 0

#define SETTING_AVATAR_OVERLAY_TYPE_NORMAL 0
#define SETTING_AVATAR_OVERLAY_TYPE_PROTOCOL 1
#define SETTING_AVATAR_OVERLAY_TYPE_CONTACT 2

#define HCONTACT_ISGROUP    0x80000000
#define HCONTACT_ISINFO     0xFFFF0000

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
#define INTM_STATUSCHANGED	(WM_USER+27)
#define INTM_AVATARCHANGED	(WM_USER+28)
#define INTM_TIMEZONECHANGED	(WM_USER+29)

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
#undef CLCDEFAULT_TEXTCOLOUR
#undef CLCDEFAULT_SELTEXTCOLOUR
#define CLCDEFAULT_TEXTCOLOUR    GetSysColor(COLOR_WINDOWTEXT)
#define CLCDEFAULT_SELTEXTCOLOUR GetSysColor(COLOR_HIGHLIGHTTEXT)
#define CLCDEFAULT_HOTTEXTCOLOUR (IsWinVer98Plus()?RGB(0,0,255):GetSysColor(COLOR_HOTLIGHT))
#define CLCDEFAULT_QUICKSEARCHCOLOUR RGB(255,255,0)
#define CLCDEFAULT_LEFTMARGIN    0
#define CLCDEFAULT_RIGHTMARGIN   2
#define CLCDEFAULT_GAMMACORRECT  1
#define CLCDEFAULT_SHOWIDLE      0

#define CLM_SETEXTRACOLUMNSSPACE   (CLM_FIRST+73)   //wParam=extra space between icons

#define CLBF_TILEVTOROWHEIGHT        0x0100

#define TIMERID_RENAME         10
#define TIMERID_DRAGAUTOSCROLL 11
#define TIMERID_INFOTIP        13
#define TIMERID_REBUILDAFTER   14
#define TIMERID_DELAYEDRESORTCLC   15
#define TIMERID_SUBEXPAND		21
#define TIMERID_INVALIDATE 22

#define TIMERID_INVALIDATE_FULL 25

#define FONTID_CONTACTS    0
#define FONTID_INVIS       1
#define FONTID_OFFLINE     2
#define FONTID_NOTONLIST   3
#define FONTID_OPENGROUPS      4
#define FONTID_OPENGROUPCOUNTS 5
#define FONTID_DIVIDERS    6
#define FONTID_OFFINVIS    7
#define FONTID_SECONDLINE  8
#define FONTID_THIRDLINE   9
#define FONTID_AWAY			10
#define FONTID_DND			11
#define FONTID_NA			12
#define FONTID_OCCUPIED		13
#define FONTID_CHAT			14
#define FONTID_INVISIBLE	15
#define FONTID_PHONE		16
#define FONTID_LUNCH		17
#define FONTID_CONTACT_TIME	18
#define FONTID_CLOSEDGROUPS 19
#define FONTID_CLOSEDGROUPCOUNTS 20
#define FONTID_STATUSBAR_PROTONAME 21
#define FONTID_EVENTAREA	22
#define FONTID_VIEMODES		23
#define FONTID_MODERN_MAX 23

#define DROPTARGET_OUTSIDE    0
#define DROPTARGET_ONSELF     1
#define DROPTARGET_ONNOTHING  2
#define DROPTARGET_ONGROUP    3
#define DROPTARGET_ONCONTACT  4
#define DROPTARGET_INSERTION  5
#define DROPTARGET_ONMETACONTACT  6
#define DROPTARGET_ONSUBCONTACT  7

struct ClcGroup;

#define CONTACTF_ONLINE    1
#define CONTACTF_INVISTO   2
#define CONTACTF_VISTO     4
#define CONTACTF_NOTONLIST 8
#define CONTACTF_CHECKED   16
#define CONTACTF_IDLE      32
//#define CONTACTF_STATUSMSG 64

#define AVATAR_POS_DONT_HAVE -1

#define TEXT_PIECE_TYPE_TEXT   0
#define TEXT_PIECE_TYPE_SMILEY 1

#define DRAGSTAGE_NOTMOVED  0
#define DRAGSTAGE_ACTIVE    1
#define DRAGSTAGEM_STAGE    0x00FF
#define DRAGSTAGEF_MAYBERENAME  0x8000
#define DRAGSTAGEF_OUTSIDE      0x4000

#define ITEM_AVATAR 0
#define ITEM_ICON 1
#define ITEM_TEXT 2
#define ITEM_EXTRA_ICONS 3
#define ITEM_CONTACT_TIME 4
#define NUM_ITEM_TYPE 5

#define TEXT_EMPTY -1
#define TEXT_STATUS 0
#define TEXT_NICKNAME 1
#define TEXT_STATUS_MESSAGE 2
#define TEXT_TEXT 3
#define TEXT_CONTACT_TIME 4
#define TEXT_LISTENING_TO 5

#define TEXT_TEXT_MAX_LENGTH 1024

#define CLUI_SetDrawerService "CLUI/SETDRAWERSERVICE"
#define CLUI_EXT_FUNC_PAINTCLC	1

//add a new hotkey so it has a default and can be changed in the options dialog
//wParam=0
//lParam=(LPARAM)(SKINHOTKEYDESC*)ssd;
//returns 0 on success, nonzero otherwise
#define MS_SKIN_ADDHOTKEY      "Skin/HotKeys/AddNew"
#define MS_SKIN_PLAYHOTKEY		"Skin/HotKeys/Run"

#define IsHContactGroup(h)  (((unsigned)(h)^HCONTACT_ISGROUP)<(HCONTACT_ISGROUP^HCONTACT_ISINFO))
#define IsHContactInfo(h)   (((unsigned)(h)&HCONTACT_ISINFO)==HCONTACT_ISINFO)
#define IsHContactContact(h) (((unsigned)(h)&HCONTACT_ISGROUP)==0)

typedef struct tagClcContactTextPiece
{
	int type;
	int len;
	union
	{
		struct
		{
			int start_pos;
		};
		struct
		{
			HICON smiley;
			int smiley_width;
			int smiley_height;
		};
	};
} ClcContactTextPiece;


struct ClcContact {
	BYTE type;
	BYTE flags;
	union {
		struct {
			int iImage;
			HANDLE hContact;
		};
		struct {
			WORD groupId;
			struct ClcGroup *group;
		};
	};
	BYTE iExtraImage[MAXEXTRACOLUMNS];
	TCHAR szText[120 - MAXEXTRACOLUMNS];
	char * proto;    // MS_PROTO_GETBASEPROTO

	struct ClcContact *subcontacts;
	BYTE SubAllocated;
	BYTE SubExpanded;
	BYTE isSubcontact;
//	int status;
	BOOL image_is_special;
	int avatar_pos;
	struct avatarCacheEntry *avatar_data;
	SortedList *plText;						// List of ClcContactTextPiece
	//TCHAR *szSecondLineText;//[120-MAXEXTRACOLUMNS];
	//SortedList *plSecondLineText;				// List of ClcContactTextPiece
	//TCHAR *szThirdLineText;//[120-MAXEXTRACOLUMNS];
	//SortedList *plThirdLineText;				// List of ClcContactTextPiece
    int iTextMaxSmileyHeight;
	//int iThirdLineMaxSmileyHeight;
    //int iSecondLineMaxSmileyHeight;
    //DWORD timezone;
    //DWORD timediff;

	// For hittest
	int pos_indent;
	RECT pos_check;
	RECT pos_avatar;
	RECT pos_icon;
	RECT pos_label;
	RECT pos_rename_rect;
	RECT pos_contact_time;
	RECT pos_extra[MAXEXTRACOLUMNS];
    DWORD lastPaintCounter;
    BYTE bContactRate;
};



struct ClcModernFontInfo {
	HFONT hFont;
	int fontHeight,changed;
	COLORREF colour;
  BYTE effect;
  COLORREF effectColour1;
  COLORREF effectColour2;
};

struct ClcData {
	struct ClcGroup list;
	int max_row_height;

	int yScroll;
	int selection;
	struct ClcFontInfo fontInfo[FONTID_MAX+1];
	int scrollTime;
	HIMAGELIST himlHighlight;
	int groupIndent;
	TCHAR szQuickSearch[128];
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
	int useWindowsColours;
	int NeedResort;
	SortedList lCLCContactsCache;
	BYTE HiLightMode;
	BYTE doubleClickExpand;
	int MetaIgnoreEmptyExtra;
	BYTE expandMeta;
	BYTE IsMetaContactsEnabled;
	time_t last_tick_time;
	BOOL force_in_dialog;
	int subIndent;
	int rightMargin;
	HBITMAP hMenuBackground;
	DWORD MenuBkColor, MenuBkHiColor, MenuTextColor, MenuTextHiColor;
	int MenuBmpUse;

	// Row height
	int *row_heights;
	int row_heights_size;
	int row_heights_allocated;

	// Avatar cache
	int use_avatar_service;
	IMAGE_ARRAY_DATA avatar_cache;

	// Row
	int row_min_heigh;
	int row_border;
	int row_before_group_space;

	BOOL row_variable_height;
	BOOL row_align_left_items_to_left;
	BOOL row_align_right_items_to_right;
	int row_items[NUM_ITEM_TYPE];
	BOOL row_hide_group_icon;
	BYTE row_align_group_mode;

	// Avatar
	BOOL avatars_show;
	BOOL avatars_draw_border;
	COLORREF avatars_border_color;
	BOOL avatars_round_corners;
	BOOL avatars_use_custom_corner_size;
	int avatars_custom_corner_size;
	BOOL avatars_ignore_size_for_row_height;
	BOOL avatars_draw_overlay;
	int avatars_overlay_type;
	
	int avatars_maxheight_size;
	int avatars_maxwidth_size;

	// Icon
	BOOL icon_hide_on_avatar;
	BOOL icon_draw_on_avatar_space;
	BOOL icon_ignore_size_for_row_height;

	// Contact time
	BOOL contact_time_show;
	BOOL contact_time_show_only_if_different;
	DWORD local_gmt_diff;
	DWORD local_gmt_diff_dst;

	// Text
	BOOL text_rtl;
	BOOL text_align_right;
	BOOL text_replace_smileys;
	BOOL text_resize_smileys;
	int text_smiley_height;
	BOOL text_use_protocol_smileys;
	BOOL text_ignore_size_for_row_height;

	// First line
	BOOL first_line_draw_smileys;
	BOOL first_line_append_nick;

	// Second line
	BOOL second_line_show;
	int second_line_top_space;
	BOOL second_line_draw_smileys;
	int second_line_type;
	TCHAR second_line_text[TEXT_TEXT_MAX_LENGTH];
	BOOL second_line_xstatus_has_priority;
	BOOL second_line_show_status_if_no_away;
	BOOL second_line_show_listening_if_no_away;
	BOOL second_line_use_name_and_message_for_xstatus;

	// Third line
	BOOL third_line_show;
	int third_line_top_space;
	BOOL third_line_draw_smileys;
	int third_line_type;
	TCHAR third_line_text[TEXT_TEXT_MAX_LENGTH];
	BOOL third_line_xstatus_has_priority;
	BOOL third_line_show_status_if_no_away;
	BOOL third_line_show_listening_if_no_away;
	BOOL third_line_use_name_and_message_for_xstatus;
	struct ClcModernFontInfo fontModernInfo[FONTID_MODERN_MAX+1];
	HWND hWnd;
	BYTE menuOwnerType;
	int menuOwnerID;
    DWORD m_paintCouter; //range is enoght to 49 days if painting will occure each one millisecond
    BYTE useMetaIcon;
    BYTE drawOverlayedStatus;
    int nInsertionLevel;

	BYTE dbbMetaHideExtra;
	BYTE dbbBlendInActiveState;
	BYTE dbbBlend25;
};

struct SHORTDATA
{
    HWND    hWnd;
    BOOL    contact_time_show_only_if_different;
    int     text_smiley_height;
    BOOL    text_replace_smileys;
    BOOL    text_use_protocol_smileys;

	// Second line
	BOOL    second_line_show;
	BOOL    second_line_draw_smileys;
	int     second_line_type;
	TCHAR   second_line_text[TEXT_TEXT_MAX_LENGTH];
	BOOL    second_line_xstatus_has_priority;
	BOOL    second_line_show_status_if_no_away;
	BOOL    second_line_show_listening_if_no_away;
	BOOL    second_line_use_name_and_message_for_xstatus;

	// Third line
	BOOL    third_line_show;
	BOOL    third_line_draw_smileys;
	int     third_line_type;
	TCHAR   third_line_text[TEXT_TEXT_MAX_LENGTH];
	BOOL    third_line_xstatus_has_priority;
	BOOL    third_line_show_status_if_no_away;
	BOOL    third_line_show_listening_if_no_away;
	BOOL    third_line_use_name_and_message_for_xstatus;
};


typedef struct tagOVERLAYICONINFO 
{
    char *name;
    char *description;
    int id;
    int listID;
} OVERLAYICONINFO;

typedef struct {
	int cbSize;
	char *PluginName;
	char *Comments;
	char *GetDrawFuncsServiceName;

} DrawerServiceStruct,*pDrawerServiceStruct ;

typedef struct {
	int cbSize;
	void (*PaintClc)(HWND,struct ClcData *,HDC,RECT *,int ,ClcProtoStatus *,HIMAGELIST);

} ExternDrawer,*pExternDrawer ;

typedef struct {
	int cbSize;
	const char *pszName;		   //name to refer to sound when playing and in db
	const char *pszDescription;	   //description for options dialog
//    const char *pszDefaultFile;    //default sound file to use
    const char *pszSection;        //section name used to group sounds (NULL is acceptable)
	const char *pszService;        //Service to call when HotKey Pressed

	int DefHotKey; //default hot key for action
} SKINHOTKEYDESCEX;

//clc.c
void    ClcOptionsChanged(void);

//clcidents.c
int     cliGetRowsPriorTo(struct ClcGroup *group,struct ClcGroup *subgroup,int contactIndex);
int     FindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible, BOOL isIgnoreSubcontacts );
int     cliGetRowByIndex(struct ClcData *dat,int testindex,struct ClcContact **contact,struct ClcGroup **subgroup);
HANDLE  ContactToHItem(struct ClcContact *contact);
HANDLE  ContactToItemHandle(struct ClcContact *contact,DWORD *nmFlags);
void    ClearRowByIndexCache();

//clcitems.c
struct ClcGroup *cli_AddGroup(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);
void    cli_FreeGroup(struct ClcGroup *group);
int     cli_AddInfoItemToGroup(struct ClcGroup *group,int flags,const TCHAR *pszText);
void    cliRebuildEntireList(HWND hwnd,struct ClcData *dat);
void    cli_DeleteItemFromTree(HWND hwnd,HANDLE hItem);
void    cli_AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);
void    cli_SortCLC(HWND hwnd,struct ClcData *dat,int useInsertionSort);
int     GetNewSelection(struct ClcGroup *group,int selection, int direction);

//clcmsgs.c
LRESULT cli_ProcessExternalMessages(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);

//clcutils.c
void    cliRecalcScrollBar(HWND hwnd,struct ClcData *dat);
void    cliBeginRenameSelection(HWND hwnd,struct ClcData *dat);
int     cliHitTest(HWND hwnd,struct ClcData *dat,int testx,int testy,struct ClcContact **contact,struct ClcGroup **group,DWORD *flags);
void    cliScrollTo(HWND hwnd,struct ClcData *dat,int desty,int noSmooth);
int     GetDropTargetInformation(HWND hwnd,struct ClcData *dat,POINT pt);
void    LoadCLCOptions(HWND hwnd,struct ClcData *dat);


//clcpaint.c
void    CLCPaint_cliPaintClc(HWND hwnd,struct ClcData *dat,HDC hdc,RECT *rcPaint);

//clcopts.c
int     ClcOptInit(WPARAM wParam,LPARAM lParam);
DWORD   GetDefaultExStyle(void);
void    GetFontSetting(int i,LOGFONTA *lf,COLORREF *colour,BYTE *effect, COLORREF *eColour1,COLORREF *eColour2);

//clistsettings.c
TCHAR * GetContactDisplayNameW( HANDLE hContact, int mode );
char*   u2a( wchar_t* src );
wchar_t* a2u( char* src );

#ifdef _UNICODE
#define t2a(src) u2a(src)
#else
#define t2a(src) mir_strdup(src)
#endif

//groups.c
TCHAR*  GetGroupNameTS( int idx, DWORD* pdwFlags );
int     RenameGroupT(WPARAM groupID, LPARAM newName);

int     GetContactCachedStatus(HANDLE hContact);
char   *GetContactCachedProtocol(HANDLE hContact);

ExternDrawer SED;

#endif _CLC_H_
