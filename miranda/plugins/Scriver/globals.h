/*
Scriver

Copyright 2000-2005 Miranda ICQ/IM project,
Copyright 2005 Piotr Piastucki

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
#ifndef SRMM_GLOBALS_H
#define SRMM_GLOBALS_H

#include "cmdlist.h"
#include "msgs.h"

#define SMF_AUTOPOPUP       		0x00000001
#define SMF_STAYMINIMIZED			0x00000002
#define SMF_CLOSEONSEND				0x00000004
#define SMF_MINIMIZEONSEND			0x00000008
#define SMF_AUTORESIZE				0x00000010
#define SMF_SAVEDRAFTS				0x00000040
#define SMF_DELTEMP					0x00000080
#define SMF_SENDONENTER				0x00000100
#define SMF_SENDONDBLENTER			0x00000200
#define SMF_SHOWPROGRESS			0x00000400
#define SMF_AVATAR          		0x00000800
#define SMF_STATUSICON				0x00002000
#define SMF_RTL						0x00004000
#define SMF_DISABLE_UNICODE			0x00008000
#define SMF_USEIEVIEW				0x00010000
#define SMF_SHOWICONS       		0x00020000
#define SMF_HIDENAMES       		0x00040000
#define SMF_SHOWTIME        		0x00080000
#define SMF_SHOWDATE        		0x00100000
#define SMF_LONGDATE				0x00200000
#define SMF_RELATIVEDATE			0x00400000
#define SMF_SHOWSECONDS				0x00800000
#define SMF_GROUPMESSAGES			0x01000000
#define SMF_MARKFOLLOWUPS			0x02000000
#define SMF_MSGONNEWLINE			0x04000000
#define SMF_DRAWLINES				0x08000000
#define SMF_INDENTTEXT				0x10000000
#define SMF_ORIGINALAVATARH			0x20000000

#define SMF2_USETABS					0x00000001
#define SMF2_HIDEONETAB					0x00000002
#define SMF2_TABSATBOTTOM				0x00000004
#define SMF2_LIMITNAMES					0x00000008
#define SMF2_SWITCHTOACTIVE  			0x00000010
#define SMF2_SEPARATECHATSCONTAINERS 	0x00000020
#define SMF2_TABCLOSEBUTTON  			0x00000040
#define SMF2_LIMITTABS					0x00000080
#define SMF2_LIMITCHATSTABS				0x00000100
#define SMF2_HIDECONTAINERS         	0x00000200
#define SMF2_SHOWINFOBAR				0x00000400
#define SMF2_SHOWSTATUSBAR				0x00010000
#define SMF2_SHOWTITLEBAR				0x00020000
#define SMF2_SHOWTOOLBAR				0x00040000
#define SMF2_USETRANSPARENCY 			0x00080000
#define SMF2_SHOWTYPING      			0x01000000
#define SMF2_SHOWTYPINGWIN   			0x02000000
#define SMF2_SHOWTYPINGTRAY  			0x04000000
#define SMF2_SHOWTYPINGCLIST 			0x08000000
#define SMF2_SHOWTYPINGSWITCH			0x10000000

#define SMF_ICON_ADD         	0
#define SMF_ICON_USERDETAILS 	1
#define SMF_ICON_HISTORY     	2
#define SMF_ICON_SEND		3
#define SMF_ICON_CANCEL		4
#define SMF_ICON_SMILEY		5
#define SMF_ICON_TYPING      	6
#define SMF_ICON_UNICODEON	7
#define SMF_ICON_UNICODEOFF	8
#define SMF_ICON_DELIVERING	9
#define SMF_ICON_QUOTE		10
#define SMF_ICON_INCOMING	11
#define SMF_ICON_OUTGOING	12
#define SMF_ICON_NOTICE		13
#define SMF_ICON_CLOSEX		14
#define SMF_ICON_OVERLAY    15
#define SMF_ICON_TYPINGOFF  16
#define SMF_ICON_COUNT		17

#define SMF_CHAT_ICON_ACTION				0
#define SMF_CHAT_ICON_ADDSTATUS			1
#define SMF_CHAT_ICON_HIGHLIGHT			2
#define SMF_CHAT_ICON_INFO				3
#define SMF_CHAT_ICON_JOIN				4
#define SMF_CHAT_ICON_KICK				5
#define SMF_CHAT_ICON_MESSAGE			6
#define SMF_CHAT_ICON_MESSAGEOUT			7
#define SMF_CHAT_ICON_NICK				8
#define SMF_CHAT_ICON_NOTICE				9
#define SMF_CHAT_ICON_PART				10
#define SMF_CHAT_ICON_QUIT				11
#define SMF_CHAT_ICON_REMSTATUS			12
#define SMF_CHAT_ICON_TOPIC				13
#define SMF_CHAT_ICON_STATUS1			14
#define SMF_CHAT_ICON_STATUS2			15
#define SMF_CHAT_ICON_STATUS3			16
#define SMF_CHAT_ICON_STATUS4			17
#define SMF_CHAT_ICON_STATUS0			18
#define SMF_CHAT_ICON_STATUS5			19
#define SMF_CHAT_ICON_WINDOW				20
#define SMF_CHAT_ICON_OVERLAY			21
#define SMF_CHAT_ICON_COUNT_GROUP		22

typedef struct ImageListUsageEntry_tag
{
	int		index;
	int		used;
} ImageListUsageEntry;


struct GlobalMessageData
{
	unsigned int flags;
	unsigned int flags2;
	HANDLE hMessageWindowList;
	DWORD openFlags;
	HANDLE hParentWindowList;
	ParentWindowData *lastParent;
	ParentWindowData *lastChatParent;
	int		limitNamesLength;
	int		activeAlpha;
	int		inactiveAlpha;
	HMENU	hMenuANSIEncoding;
	int     tabIconListUsageSize;
	ImageListUsageEntry     *tabIconListUsage;
	TCmdList *draftList;
	int		avatarServiceInstalled;
	int		smileyAddInstalled;
	int 	popupInstalled;
	int		ieviewInstalled;
	int		buttonVisibility;
	int		chatBbuttonVisibility;
	int		limitTabsNum;
	int		limitChatsTabsNum;
	int		indentSize;
	HIMAGELIST hTabIconList;
	HIMAGELIST hButtonIconList;
	HIMAGELIST hChatButtonIconList;
	HIMAGELIST hHelperIconList;
	HIMAGELIST hSearchEngineIconList;
	HBRUSH	   hInfobarBrush;
	int		   toolbarPosition;
	int        splitterY;
	HWND       hFocusWnd;
    DWORD      logLineColour;
};

int IconsChanged(WPARAM wParam, LPARAM lParam);
int SmileySettingsChanged(WPARAM wParam, LPARAM lParam);
void InitGlobals();
void FreeGlobals();
void ReloadGlobals();
void RegisterIcons();
void ReleaseIcons();
void LoadGlobalIcons();
HICON GetCachedIcon(const char *name);
void RegisterFontServiceFonts();
int ScriverRestoreWindowPosition(HWND hwnd,HANDLE hContact,const char *szModule,const char *szNamePrefix, int flags, int showCmd);

int ImageList_AddIcon_Ex(HIMAGELIST hIml, int id);
int ImageList_AddIcon_Ex2(HIMAGELIST hIml, HICON hIcon);
int ImageList_ReplaceIcon_Ex(HIMAGELIST hIml, int nIndex, int id);
int ImageList_AddIcon_ProtoEx(HIMAGELIST hIml, const char* szProto, int status);
int ImageList_ReplaceIcon_ProtoEx(HIMAGELIST hIml, int nIndex, const char* szProto, int status);
void ReleaseIconSmart(HICON hIcon);

extern struct GlobalMessageData *g_dat;
void StreamInTestEvents(HWND hEditWnd, struct GlobalMessageData *gdat);

#endif
