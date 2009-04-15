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
#include "commonheaders.h"
#include "m_ieview.h"

struct GlobalMessageData *g_dat=NULL;
extern HINSTANCE g_hInst;
extern PSLWA pSetLayeredWindowAttributes;

HANDLE hEventSkin2IconsChanged;

static HANDLE g_hAck = 0;
static int ackevent(WPARAM wParam, LPARAM lParam);

extern int    Chat_ModulesLoaded(WPARAM wParam,LPARAM lParam);
extern int    Chat_PreShutdown(WPARAM wParam,LPARAM lParam);

BOOL IsStaticIcon(HICON hIcon) {
	int i;
	for (i=0; i<SIZEOF(g_dat->hIcons); i++) {
		if (hIcon == g_dat->hIcons[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

void ReleaseIconSmart(HICON hIcon) {
	if (!IsStaticIcon(hIcon)) {
		DWORD result = CallService(MS_SKIN2_RELEASEICON,(WPARAM)hIcon, 0);
		if ( result == 1 || result == CALLSERVICE_NOTFOUND)
			DestroyIcon(hIcon);
	}
}

int ImageList_AddIcon_Ex(HIMAGELIST hIml, int id) {
	HICON hIcon = LoadSkinnedIcon(id);
 	int res = ImageList_AddIcon(hIml, hIcon);
 	CallService(MS_SKIN2_RELEASEICON,(WPARAM)hIcon, 0);
 	return res;
}

int ImageList_AddIcon_Ex2(HIMAGELIST hIml, HICON hIcon) {
 	int res = ImageList_AddIcon(hIml, hIcon);
 	CallService(MS_SKIN2_RELEASEICON,(WPARAM)hIcon, 0);
 	return res;
}

int ImageList_ReplaceIcon_Ex(HIMAGELIST hIml, int nIndex, int id) {
	HICON hIcon = LoadSkinnedIcon(id);
	int res = ImageList_ReplaceIcon(hIml, nIndex, hIcon);
	CallService(MS_SKIN2_RELEASEICON,(WPARAM)hIcon, 0);
	return res;
}

int ImageList_AddIcon_ProtoEx(HIMAGELIST hIml, const char* szProto, int status) {
	HICON hIcon = LoadSkinnedProtoIcon(szProto, status);
 	int res = ImageList_AddIcon(hIml, hIcon);
	CallService(MS_SKIN2_RELEASEICON,(WPARAM)hIcon, 0);
 	return res;
}

int ImageList_ReplaceIcon_ProtoEx(HIMAGELIST hIml, int nIndex, const char* szProto, int status) {
	HICON hIcon = LoadSkinnedProtoIcon(szProto, status);
	int res = ImageList_ReplaceIcon(hIml, nIndex, hIcon);
 	CallService(MS_SKIN2_RELEASEICON,(WPARAM)hIcon, 0);
 	return res;
}

static int buttonIcons[] = {SMF_ICON_CLOSEX, SMF_ICON_QUOTE, SMF_ICON_SMILEY, SMF_ICON_ADD, -1, SMF_ICON_USERDETAILS, SMF_ICON_HISTORY, SMF_ICON_CANCEL, SMF_ICON_SEND};

void LoadGlobalIcons() {
	int i;
	g_dat->hIcons[SMF_ICON_ADD] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_ADD");
	g_dat->hIcons[SMF_ICON_USERDETAILS] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_USERDETAILS");
	g_dat->hIcons[SMF_ICON_HISTORY] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_HISTORY");
	g_dat->hIcons[SMF_ICON_TYPING] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_TYPING");
	g_dat->hIcons[SMF_ICON_TYPINGOFF] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_TYPINGOFF");

	g_dat->hIcons[SMF_ICON_SEND] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_SEND");
	g_dat->hIcons[SMF_ICON_CANCEL] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_CANCEL");
	g_dat->hIcons[SMF_ICON_SMILEY] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_SMILEY");
	g_dat->hIcons[SMF_ICON_UNICODEON] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_UNICODEON");
	g_dat->hIcons[SMF_ICON_UNICODEOFF] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_UNICODEOFF");
	g_dat->hIcons[SMF_ICON_DELIVERING] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_DELIVERING");
	g_dat->hIcons[SMF_ICON_QUOTE] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_QUOTE");
	g_dat->hIcons[SMF_ICON_CLOSEX] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_CLOSEX");
	g_dat->hIcons[SMF_ICON_OVERLAY] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_OVERLAY");

	g_dat->hIcons[SMF_ICON_INCOMING] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_INCOMING");
	g_dat->hIcons[SMF_ICON_OUTGOING] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_OUTGOING");
	g_dat->hIcons[SMF_ICON_NOTICE] = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)"scriver_NOTICE");
	g_dat->hIcons[SMF_ICON_MESSAGE] = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	for (i=0; i<SIZEOF(buttonIcons); i++) {
		if (buttonIcons[i] == -1) {
			ImageList_AddIcon_ProtoEx(g_dat->hButtonIconList, NULL, ID_STATUS_OFFLINE);
		} else {
			ImageList_AddIcon(g_dat->hButtonIconList, g_dat->hIcons[buttonIcons[i]]);
		}
	}
	{
		int overlayIcon;
		ImageList_AddIcon(g_dat->hHelperIconList, g_dat->hIcons[SMF_ICON_OVERLAY]);
		overlayIcon = ImageList_AddIcon(g_dat->hHelperIconList, g_dat->hIcons[SMF_ICON_OVERLAY]);
		ImageList_SetOverlayImage(g_dat->hHelperIconList, overlayIcon, 1);
	}
	for (i=0; i<IDI_FOODNETWORK - IDI_GOOGLE + 1; i++) {
		ImageList_AddIcon(g_dat->hSearchEngineIconList, LoadImage(g_hInst,MAKEINTRESOURCE(IDI_GOOGLE + i),IMAGE_ICON,0,0,LR_SHARED));
	}
}

void ReleaseGlobalIcons() {
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_ADD");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_USERDETAILS");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_HISTORY");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_TYPING");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_SEND");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_CANCEL");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_SMILEY");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_UNICODEON");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_UNICODEOFF");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_DELIVERING");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_QUOTE");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_CLOSEX");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_OVERLAY");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_INCOMING");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_OUTGOING");
	CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)"scriver_NOTICE");

	CallService(MS_SKIN2_RELEASEICON, (WPARAM)g_dat->hIcons[SMF_ICON_MESSAGE], 0);
	ImageList_RemoveAll(g_dat->hButtonIconList);
	ImageList_RemoveAll(g_dat->hHelperIconList);
}

static BOOL CALLBACK LangAddCallback(CHAR * str) {
	int i, count;
	UINT cp;
	static struct { UINT cpId; TCHAR *cpName; } cpTable[] = {
		{	874,	_T("Thai") },
		{	932,	_T("Japanese") },
		{	936,	_T("Simplified Chinese") },
		{	949,	_T("Korean") },
		{	950,	_T("Traditional Chinese") },
		{	1250,	_T("Central European") },
		{	1251,	_T("Cyrillic") },
		{	1252,	_T("Latin I") },
		{	1253,	_T("Greek") },
		{	1254,	_T("Turkish") },
		{	1255,	_T("Hebrew") },
		{	1256,	_T("Arabic") },
		{	1257,	_T("Baltic") },
		{	1258,	_T("Vietnamese") },
		{	1361,	_T("Korean (Johab)") }
	};
    cp = atoi(str);
	count = sizeof(cpTable)/sizeof(cpTable[0]);
	for (i=0; i<count && cpTable[i].cpId!=cp; i++);
	if (i < count) {
        AppendMenu(g_dat->hMenuANSIEncoding, MF_STRING, cp, TranslateTS(cpTable[i].cpName));
	}
	return TRUE;
}

int FindBorderColour(int colour) {
	BOOL add = TRUE;
	int diff = 60;
	int r = (colour >> 16) &0xFF;
	int g = (colour >> 8) &0xFF;
	int b = colour &0xFF;
	int r2 = r - diff;
	int g2 = g - diff;
	int b2 = b - diff;
	int diff2 = (max(0, r2) - r2) + (max(0, g2) - g2) + (max(0, b2) - b2);
	if (diff2 > 0) {
		int r1 = r + diff;
		int g1 = g + diff;
		int b1 = b + diff;
		int diff1 = (r1 - min(255, r1)) + (g1 - min(255, g1)) + (b1 - min(255, b1));
		if (diff1 < diff2) {
			return (min(255, r1) << 16) | (min(255, g1) << 8) | (min(255, b1));
		}
	}
	return (max(0, r2) << 16) | (max(0, g2) << 8) | (max(0, b2));
}

void LoadInfobarFonts()
{
	LOGFONT lf;
	DWORD ibColour;
	if (g_dat->hContactNameFont != NULL) {
		DeleteObject(g_dat->hContactNameFont);
	}
	if (g_dat->hContactStatusFont != NULL) {
		DeleteObject(g_dat->hContactStatusFont);
	}
	if (g_dat->hInfobarBrush != NULL) {
		DeleteObject(g_dat->hInfobarBrush);
	}
	if (g_dat->hInfobarPen != NULL) {
		DeleteObject(g_dat->hInfobarPen);
	}
	LoadMsgDlgFont(MSGFONTID_INFOBAR_NAME, &lf, &g_dat->contactNameColour);
	g_dat->hContactNameFont = CreateFontIndirect(&lf);
	LoadMsgDlgFont(MSGFONTID_INFOBAR_STATUS, &lf, &g_dat->contactStatusColour);
	g_dat->hContactStatusFont = CreateFontIndirect(&lf);
	ibColour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_INFOBARBKGCOLOUR, SRMSGDEFSET_INFOBARBKGCOLOUR);
	g_dat->hInfobarBrush = CreateSolidBrush(ibColour);
	g_dat->hInfobarPen = CreatePen(PS_SOLID, 0, FindBorderColour(ibColour));
}

void InitGlobals() {
	g_dat = (struct GlobalMessageData *)mir_alloc(sizeof(struct GlobalMessageData));
	ZeroMemory(g_dat, sizeof(struct GlobalMessageData));
	g_dat->hMessageWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	g_dat->hParentWindowList = (HANDLE) CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
	g_dat->hMenuANSIEncoding = CreatePopupMenu();
	AppendMenu(g_dat->hMenuANSIEncoding, MF_STRING, 500, TranslateT("Default codepage"));
	AppendMenuA(g_dat->hMenuANSIEncoding, MF_SEPARATOR, 0, 0);
	EnumSystemCodePagesA(LangAddCallback, CP_INSTALLED);
	g_hAck = HookEvent_Ex(ME_PROTO_ACK, ackevent);
	ReloadGlobals();
	g_dat->lastParent = NULL;
	g_dat->lastChatParent = NULL;
	g_dat->hTabIconList = NULL;
	g_dat->tabIconListUsage = NULL;
	g_dat->tabIconListUsageSize = 0;
	g_dat->hButtonIconList = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 0, 0);
	g_dat->hTabIconList = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 0, 0);
	g_dat->hHelperIconList = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 0, 0);
	g_dat->hSearchEngineIconList = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 0, 0);
	g_dat->draftList = NULL;
	LoadInfobarFonts();
}

void FreeGlobals() {
	if (g_dat) {
		if (g_dat->hContactNameFont != NULL) {
			DeleteObject(g_dat->hContactNameFont);
		}
		if (g_dat->hContactStatusFont != NULL) {
			DeleteObject(g_dat->hContactStatusFont);
		}
		if (g_dat->hInfobarBrush != NULL) {
			DeleteObject(g_dat->hInfobarBrush);
		}
		if (g_dat->hInfobarPen != NULL) {
			DeleteObject(g_dat->hInfobarPen);
		}
		if (g_dat->draftList != NULL) tcmdlist_free(g_dat->draftList);
		ReleaseGlobalIcons();
		if (g_dat->hTabIconList)
			ImageList_Destroy(g_dat->hTabIconList);
		if (g_dat->hButtonIconList)
			ImageList_Destroy(g_dat->hButtonIconList);
		if (g_dat->hHelperIconList)
			ImageList_Destroy(g_dat->hHelperIconList);
		if (g_dat->hSearchEngineIconList)
			ImageList_Destroy(g_dat->hSearchEngineIconList);
		mir_free(g_dat->tabIconListUsage);
		mir_free(g_dat);
		g_dat = NULL;
	}
}

void ReloadGlobals() {
	g_dat->avatarServiceInstalled = ServiceExists(MS_AV_GETAVATARBITMAP);
	g_dat->smileyAddInstalled =  ServiceExists(MS_SMILEYADD_SHOWSELECTION);
	g_dat->popupInstalled =  ServiceExists(MS_POPUP_ADDPOPUPEX);
	g_dat->ieviewInstalled =  ServiceExists(MS_IEVIEW_WINDOW);
	g_dat->flags = 0;
	g_dat->flags2 = 0;
//	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDBUTTON, SRMSGDEFSET_SENDBUTTON))
//		g_dat->flags |= SMF_SENDBTN;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AVATARENABLE, SRMSGDEFSET_AVATARENABLE)) {
		g_dat->flags |= SMF_AVATAR;
	}
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWPROGRESS, SRMSGDEFSET_SHOWPROGRESS))
		g_dat->flags |= SMF_SHOWPROGRESS;

	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWLOGICONS, SRMSGDEFSET_SHOWLOGICONS))
		g_dat->flags |= SMF_SHOWICONS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTIME, SRMSGDEFSET_SHOWTIME))
		g_dat->flags |= SMF_SHOWTIME;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSECONDS, SRMSGDEFSET_SHOWSECONDS))
		g_dat->flags |= SMF_SHOWSECONDS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWDATE, SRMSGDEFSET_SHOWDATE))
		g_dat->flags |= SMF_SHOWDATE;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USELONGDATE, SRMSGDEFSET_USELONGDATE))
		g_dat->flags |= SMF_LONGDATE;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USERELATIVEDATE, SRMSGDEFSET_USERELATIVEDATE))
		g_dat->flags |= SMF_RELATIVEDATE;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_GROUPMESSAGES, SRMSGDEFSET_GROUPMESSAGES))
		g_dat->flags |= SMF_GROUPMESSAGES;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_MARKFOLLOWUPS, SRMSGDEFSET_MARKFOLLOWUPS))
		g_dat->flags |= SMF_MARKFOLLOWUPS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_MESSAGEONNEWLINE, SRMSGDEFSET_MESSAGEONNEWLINE))
		g_dat->flags |= SMF_MSGONNEWLINE;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_DRAWLINES, SRMSGDEFSET_DRAWLINES))
		g_dat->flags |= SMF_DRAWLINES;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDENAMES, SRMSGDEFSET_HIDENAMES))
		g_dat->flags |= SMF_HIDENAMES;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP))
		g_dat->flags |= SMF_AUTOPOPUP;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_STAYMINIMIZED, SRMSGDEFSET_STAYMINIMIZED))
		g_dat->flags |= SMF_STAYMINIMIZED;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVEDRAFTS, SRMSGDEFSET_SAVEDRAFTS))
		g_dat->flags |= SMF_SAVEDRAFTS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTORESIZE, SRMSGDEFSET_AUTORESIZE))
		g_dat->flags |= SMF_AUTORESIZE;

	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_DELTEMP, SRMSGDEFSET_DELTEMP))
		g_dat->flags |= SMF_DELTEMP;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER))
		g_dat->flags |= SMF_SENDONENTER;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONDBLENTER, SRMSGDEFSET_SENDONDBLENTER))
		g_dat->flags |= SMF_SENDONDBLENTER;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_STATUSICON, SRMSGDEFSET_STATUSICON))
		g_dat->flags |= SMF_STATUSICON;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_INDENTTEXT, SRMSGDEFSET_INDENTTEXT))
		g_dat->flags |= SMF_INDENTTEXT;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_DONOTSTEALFOCUS, SRMSGDEFSET_DONOTSTEALFOCUS))
		g_dat->flags |= SMF_DONOTSTEALFOCUS;

	g_dat->openFlags = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_POPFLAGS, SRMSGDEFSET_POPFLAGS);
	g_dat->indentSize = DBGetContactSettingWord(NULL, SRMMMOD, SRMSGSET_INDENTSIZE, SRMSGDEFSET_INDENTSIZE);

	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USETABS, SRMSGDEFSET_USETABS))
		g_dat->flags2 |= SMF2_USETABS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TABSATBOTTOM, SRMSGDEFSET_TABSATBOTTOM))
		g_dat->flags2 |= SMF2_TABSATBOTTOM;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SWITCHTOACTIVE, SRMSGDEFSET_SWITCHTOACTIVE))
		g_dat->flags2 |= SMF2_SWITCHTOACTIVE;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITNAMES, SRMSGDEFSET_LIMITNAMES))
		g_dat->flags2 |= SMF2_LIMITNAMES;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDEONETAB, SRMSGDEFSET_HIDEONETAB))
		g_dat->flags2 |= SMF2_HIDEONETAB;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SEPARATECHATSCONTAINERS, SRMSGDEFSET_SEPARATECHATSCONTAINERS))
		g_dat->flags2 |= SMF2_SEPARATECHATSCONTAINERS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TABCLOSEBUTTON, SRMSGDEFSET_TABCLOSEBUTTON))
		g_dat->flags2 |= SMF2_TABCLOSEBUTTON;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITTABS, SRMSGDEFSET_LIMITTABS))
		g_dat->flags2 |= SMF2_LIMITTABS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITCHATSTABS, SRMSGDEFSET_LIMITCHATSTABS))
		g_dat->flags2 |= SMF2_LIMITCHATSTABS;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDECONTAINERS, SRMSGDEFSET_HIDECONTAINERS))
		g_dat->flags2 |= SMF2_HIDECONTAINERS;

	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSTATUSBAR, SRMSGDEFSET_SHOWSTATUSBAR))
		g_dat->flags2 |= SMF2_SHOWSTATUSBAR;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTITLEBAR, SRMSGDEFSET_SHOWTITLEBAR))
		g_dat->flags2 |= SMF2_SHOWTITLEBAR;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE))
		g_dat->flags2 |= SMF2_SHOWTOOLBAR;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWINFOBAR, SRMSGDEFSET_SHOWINFOBAR))
		g_dat->flags2 |= SMF2_SHOWINFOBAR;

	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPING, SRMSGDEFSET_SHOWTYPING))
		g_dat->flags2 |= SMF2_SHOWTYPING;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN))
		g_dat->flags2 |= SMF2_SHOWTYPINGWIN;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGNOWIN, SRMSGDEFSET_SHOWTYPINGNOWIN))
		g_dat->flags2 |= SMF2_SHOWTYPINGTRAY;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST))
		g_dat->flags2 |= SMF2_SHOWTYPINGCLIST;
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGSWITCH, SRMSGDEFSET_SHOWTYPINGSWITCH))
		g_dat->flags2 |= SMF2_SHOWTYPINGSWITCH;

	if (LOBYTE(LOWORD(GetVersion())) >= 5  && pSetLayeredWindowAttributes != NULL) {
		if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USETRANSPARENCY, SRMSGDEFSET_USETRANSPARENCY))
			g_dat->flags2 |= SMF2_USETRANSPARENCY;
		g_dat->activeAlpha = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_ACTIVEALPHA, SRMSGDEFSET_ACTIVEALPHA);
		g_dat->inactiveAlpha = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_INACTIVEALPHA, SRMSGDEFSET_INACTIVEALPHA);
	}
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USEIEVIEW, SRMSGDEFSET_USEIEVIEW))
		g_dat->flags |= SMF_USEIEVIEW;

	g_dat->buttonVisibility = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_BUTTONVISIBILITY, SRMSGDEFSET_BUTTONVISIBILITY);

	g_dat->limitNamesLength = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_LIMITNAMESLEN, SRMSGDEFSET_LIMITNAMESLEN);
	g_dat->limitTabsNum = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_LIMITTABSNUM, SRMSGDEFSET_LIMITTABSNUM);
	g_dat->limitChatsTabsNum = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_LIMITCHATSTABSNUM, SRMSGDEFSET_LIMITCHATSTABSNUM);

}

static int ackevent(WPARAM wParam, LPARAM lParam) {
	ACKDATA *pAck = (ACKDATA *)lParam;

	if (!pAck) return 0;
	else if (pAck->type==ACKTYPE_AVATAR) {
		HWND h = WindowList_Find(g_dat->hMessageWindowList, (HANDLE)pAck->hContact);
		if(h) SendMessage(h, HM_AVATARACK, wParam, lParam);
	}
	else if (pAck->type==ACKTYPE_MESSAGE) {
		ACKDATA *ack = (ACKDATA *) lParam;
		DBEVENTINFO dbei = { 0 };
		HANDLE hNewEvent;
		MessageSendQueueItem * item;
		HWND hwndSender;

		item = FindSendQueueItem((HANDLE)pAck->hContact, (HANDLE)pAck->hProcess);
		if (item != NULL) {
			hwndSender = item->hwndSender;
			if (ack->result == ACKRESULT_FAILED) {
				if (item->hwndErrorDlg != NULL) {
					item = FindOldestPendingSendQueueItem(hwndSender, (HANDLE)pAck->hContact);
				}
				if (item != NULL && item->hwndErrorDlg == NULL) {
					if (hwndSender != NULL) {
						ErrorWindowData *ewd = (ErrorWindowData *) mir_alloc(sizeof(ErrorWindowData));
						ewd->szName = GetNickname(item->hContact, item->proto);
						ewd->szDescription = a2t((char *) ack->lParam);
						ewd->szText = GetSendBufferMsg(item);
						ewd->hwndParent = hwndSender;
						ewd->queueItem = item;
						SendMessage(hwndSender, DM_STOPMESSAGESENDING, 0, 0);
						SendMessage(hwndSender, DM_SHOWERRORMESSAGE, 0, (LPARAM)ewd);
					} else {
						RemoveSendQueueItem(item);
					}
				}
				return 0;
			}

			dbei.cbSize = sizeof(dbei);
			dbei.eventType = EVENTTYPE_MESSAGE;
			dbei.flags = DBEF_SENT | (( item->flags & PREF_RTL) ? DBEF_RTL : 0 );
			if ( item->flags & PREF_UTF )
				dbei.flags |= DBEF_UTF;
			dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) item->hContact, 0);
			dbei.timestamp = time(NULL);
			dbei.cbBlob = lstrlenA(item->sendBuffer) + 1;
	#if defined( _UNICODE )
			if ( !( item->flags & PREF_UTF ))
				dbei.cbBlob *= sizeof(TCHAR) + 1;
	#endif
			dbei.pBlob = (PBYTE) item->sendBuffer;
			hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) item->hContact, (LPARAM) & dbei);

			if (item->hwndErrorDlg != NULL) {
				DestroyWindow(item->hwndErrorDlg);
			}

			if (RemoveSendQueueItem(item) && DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOCLOSE, SRMSGDEFSET_AUTOCLOSE)) {
				if (hwndSender != NULL) {
					DestroyWindow(hwndSender);
				}
			} else if (hwndSender != NULL) {
				SendMessage(hwndSender, DM_STOPMESSAGESENDING, 0, 0);
				SkinPlaySound("SendMsg");
			}
		}
	}
	return 0;
}
