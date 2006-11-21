/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, Scott Ellis (mail@scottellis.com.au)
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
#include "module_fonts.h"
#include "m_fontservice.h"

#define CLIST_FONTID_CONTACTS    0
#define CLIST_FONTID_INVIS       1
#define CLIST_FONTID_OFFLINE     2
#define CLIST_FONTID_NOTONLIST   3
#define CLIST_FONTID_GROUPS      4
#define CLIST_FONTID_GROUPCOUNTS 5
#define CLIST_FONTID_DIVIDERS    6
#define CLIST_FONTID_OFFINVIS    7
#define CLIST_FONTID_MAX         7

// from CList
static const char *szClistFontIdDescr[CLIST_FONTID_MAX + 1] =
{ "Standard contacts", "Online contacts to whom you have a different visibility", "Offline contacts", "Contacts which are 'not on list'",
"Groups", "Group member counts", "Dividers", "Offline contacts to whom you have a different visibility" };

static int fontListOrder[CLIST_FONTID_MAX + 1] =
{ CLIST_FONTID_CONTACTS, CLIST_FONTID_INVIS, CLIST_FONTID_OFFLINE, CLIST_FONTID_OFFINVIS, 
CLIST_FONTID_NOTONLIST, CLIST_FONTID_GROUPS, CLIST_FONTID_GROUPCOUNTS, CLIST_FONTID_DIVIDERS };


// from SRMM
struct SRMMFontOptionsList
{
	char *szDescr;
	COLORREF defColour;
	char *szDefFace;
	BYTE defCharset, defStyle;
	char defSize;
	COLORREF colour;
	char szFace[LF_FACESIZE];
	BYTE charset, style;
	char size;
}
static fontOptionsList[] = {
	{"Outgoing messages", RGB(106, 106, 106), "Arial", DEFAULT_CHARSET, 0, -12},
	{"Incoming messages", RGB(0, 0, 0), "Arial", DEFAULT_CHARSET, 0, -12},
	{"Outgoing name", RGB(89, 89, 89), "Arial", DEFAULT_CHARSET, DBFONTF_BOLD, -12},
	{"Outgoing time", RGB(0, 0, 0), "Terminal", DEFAULT_CHARSET, DBFONTF_BOLD, -9},
	{"Outgoing colon", RGB(89, 89, 89), "Arial", DEFAULT_CHARSET, 0, -11},
	{"Incoming name", RGB(215, 0, 0), "Arial", DEFAULT_CHARSET, DBFONTF_BOLD, -12},
	{"Incoming time", RGB(0, 0, 0), "Terminal", DEFAULT_CHARSET, DBFONTF_BOLD, -9},
	{"Incoming colon", RGB(215, 0, 0), "Arial", DEFAULT_CHARSET, 0, -11},
	{"Message area", RGB(0, 0, 0), "Arial", DEFAULT_CHARSET, 0, -12},
	{"Notices", RGB(90, 90, 160), "Arial", DEFAULT_CHARSET, 0, -12},
};
const int msgDlgFontCount = sizeof(fontOptionsList) / sizeof(fontOptionsList[0]);

struct ChatFontOptionsList
{
	char *szDescr;
	COLORREF defColour;
	char *szDefFace;
	BYTE defCharset, defStyle;
	char defSize;
	COLORREF colour;
	char szFace[LF_FACESIZE];
	BYTE charset, style;
	char size;
}

//remeber to put these in the Translate( ) template file too
static chatFontOptionsList[] = {
	{"Timestamp", RGB(50, 50, 240), "Terminal", DEFAULT_CHARSET, 0, -8},
	{"Others nicknames", RGB(0, 0, 0), "Courier New", DEFAULT_CHARSET, DBFONTF_BOLD, -13},
	{"Your nickname", RGB(0, 0, 0), "Courier New", DEFAULT_CHARSET, DBFONTF_BOLD, -13},
	{"User has joined", RGB(90, 160, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User has left", RGB(160, 160, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User has disconnected", RGB(160, 90, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User kicked ...", RGB(100, 100, 100), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User is now known as ...", RGB(90, 90, 160), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Notice from user", RGB(160, 130, 60), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Incoming message", RGB(90, 90, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Outgoing message", RGB(90, 90, 90), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"The topic is ...", RGB(70, 70, 160), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Information messages", RGB(130, 130, 195), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User enables status for ...", RGB(70, 150, 70), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"User disables status for ...", RGB(150, 70, 70), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Action message", RGB(160, 90, 160), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Highlighted message", RGB(180, 150, 80), "Courier New", DEFAULT_CHARSET, 0, -13},
	{"Message typing area", RGB(0, 0, 40), "MS Shell Dlg", DEFAULT_CHARSET, 0, -13},
	{"User list members", RGB(40,40, 90), "MS Shell Dlg", DEFAULT_CHARSET, 0, -11},
	{"User list statuses", RGB(0, 0, 0), "MS Shell Dlg", DEFAULT_CHARSET, DBFONTF_BOLD, -11},
};
const int chatMsgDlgFontCount = sizeof(chatFontOptionsList) / sizeof(chatFontOptionsList[0]);

#define CLCDEFAULT_BKCOLOUR      GetSysColor(COLOR_3DFACE)
#define CLCDEFAULT_SELTEXTCOLOUR GetSysColor(COLOR_HIGHLIGHTTEXT)
#define CLCDEFAULT_HOTTEXTCOLOUR (IsWinVer98Plus()?RGB(0,0,255):GetSysColor(COLOR_HOTLIGHT))
#define CLCDEFAULT_QUICKSEARCHCOLOUR RGB(255,255,0)

void RegisterCListFonts() {
	FontID fontid = {0};
	char idstr[10];
	int i;
	ColourID colourid;

	fontid.cbSize = sizeof(FontID);
	fontid.flags = FIDF_ALLOWREREGISTER | FIDF_APPENDNAME | FIDF_NOAS | FIDF_SAVEPOINTSIZE | FIDF_ALLOWEFFECTS;

	for (i = 0; i <= CLIST_FONTID_MAX; i++) {
		strncpy(fontid.dbSettingsGroup, "CLC", sizeof(fontid.dbSettingsGroup));
		strncpy(fontid.group, Translate("Contact List"), sizeof(fontid.group));
		strncpy(fontid.name, Translate(szClistFontIdDescr[fontListOrder[i]]), sizeof(fontid.name));
		sprintf(idstr, "Font%d", fontListOrder[i]);
		strncpy(fontid.prefix, idstr, sizeof(fontid.prefix));
		fontid.order = fontListOrder[i];

		CallService(MS_FONT_REGISTER, (WPARAM)&fontid, 0);
	}

	// and colours
	colourid.cbSize = sizeof(ColourID);
	strncpy(colourid.dbSettingsGroup, "CLC", sizeof(colourid.dbSettingsGroup));

	strncpy(colourid.setting, "BkColour", sizeof(colourid.setting));
	strncpy(colourid.name, Translate("Background"), sizeof(colourid.name));
	colourid.order = 0;
	strncpy(colourid.group, Translate("Contact List"), sizeof(colourid.group));
	colourid.defcolour = CLCDEFAULT_BKCOLOUR;	
	CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "SelTextColour", sizeof(colourid.setting));
	strncpy(colourid.name, Translate("Selected Text"), sizeof(colourid.name));
	colourid.order = 1;
	strncpy(colourid.group, Translate("Contact List"), sizeof(colourid.group));
	colourid.defcolour = CLCDEFAULT_SELTEXTCOLOUR;
	CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "HotTextColour", sizeof(colourid.setting));
	strncpy(colourid.name, Translate("Hottrack Text"), sizeof(colourid.name));
	colourid.order = 1;
	strncpy(colourid.group, Translate("Contact List"), sizeof(colourid.group));
	colourid.defcolour = CLCDEFAULT_HOTTEXTCOLOUR;
	CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "QuickSearchColour", sizeof(colourid.setting));
	strncpy(colourid.name, Translate("Quicksearch Text"), sizeof(colourid.name));
	colourid.order = 1;
	strncpy(colourid.group, Translate("Contact List"), sizeof(colourid.group));
	colourid.defcolour = CLCDEFAULT_QUICKSEARCHCOLOUR;
	CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);
}

void RegisterSRMMFonts() {
	FontID fontid = {0};
	char idstr[10];
	int index = 0, i;
	ColourID colourid;

	fontid.cbSize = sizeof(FontID);
	fontid.flags = FIDF_ALLOWREREGISTER | FIDF_DEFAULTVALID;
	
	for(i = 0; i < msgDlgFontCount; i++, index++) {
		strncpy(fontid.dbSettingsGroup, "SRMM", sizeof(fontid.dbSettingsGroup));
		strncpy(fontid.group, Translate("Message Log"), sizeof(fontid.group));
		strncpy(fontid.name, Translate(fontOptionsList[i].szDescr), sizeof(fontid.name));
		sprintf(idstr, "SRMFont%d", index);
		strncpy(fontid.prefix, idstr, sizeof(fontid.prefix));
		fontid.order = index;

		fontid.deffontsettings.charset = fontOptionsList[i].defCharset;
		fontid.deffontsettings.colour = fontOptionsList[i].defColour;
		fontid.deffontsettings.size = fontOptionsList[i].defSize;
		fontid.deffontsettings.style = fontOptionsList[i].defStyle;
		strncpy(fontid.deffontsettings.szFace, fontOptionsList[i].szDefFace, sizeof(fontid.deffontsettings.szFace));

		CallService(MS_FONT_REGISTER, (WPARAM)&fontid, 0);
	}

	colourid.cbSize = sizeof(ColourID);
	strncpy(colourid.dbSettingsGroup, "SRMM", sizeof(colourid.dbSettingsGroup));
	strncpy(colourid.setting, "BkgColour", sizeof(colourid.setting));
	strncpy(colourid.name, Translate("Background"), sizeof(colourid.name));
	colourid.order = 0;
	strncpy(colourid.group, Translate("Message Log"), sizeof(colourid.group));

	CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);
}

void RegisterChatFonts() {

	FontID fontid = {0};
	ColourID colourid;
	char idstr[10];
	int index = 0, i;

	fontid.cbSize = sizeof(FontID);
	fontid.flags = FIDF_ALLOWREREGISTER | FIDF_DEFAULTVALID | FIDF_NEEDRESTART;
	for(i = 0; i < chatMsgDlgFontCount; i++, index++) {
		strncpy(fontid.dbSettingsGroup, "ChatFonts", sizeof(fontid.dbSettingsGroup));
		strncpy(fontid.group, Translate("Chat Module"), sizeof(fontid.group));
		strncpy(fontid.name, Translate(chatFontOptionsList[i].szDescr), sizeof(fontid.name));
		sprintf(idstr, "Font%d", index);
		strncpy(fontid.prefix, idstr, sizeof(fontid.prefix));
		fontid.order = index;

		fontid.deffontsettings.charset = chatFontOptionsList[i].defCharset;
		fontid.deffontsettings.colour = chatFontOptionsList[i].defColour;
		fontid.deffontsettings.size = chatFontOptionsList[i].defSize;
		fontid.deffontsettings.style = chatFontOptionsList[i].defStyle;
		strncpy(fontid.deffontsettings.szFace, chatFontOptionsList[i].szDefFace, sizeof(fontid.deffontsettings.szFace));

		CallService(MS_FONT_REGISTER, (WPARAM)&fontid, 0);
	}

	colourid.cbSize = sizeof(ColourID);
	strncpy(colourid.dbSettingsGroup, "Chat", sizeof(colourid.dbSettingsGroup));

	strncpy(colourid.setting, "ColorLogBG", sizeof(colourid.setting));
	strncpy(colourid.name, Translate("Background"), sizeof(colourid.name));
	colourid.order = 0;
	strncpy(colourid.group, Translate("Chat Module"), sizeof(colourid.group));
	colourid.defcolour = GetSysColor(COLOR_WINDOW);
	CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "ColorMessageBG", sizeof(colourid.setting));
	strncpy(colourid.name, Translate("Message Background"), sizeof(colourid.name));
	colourid.order = 0;
	strncpy(colourid.group, Translate("Chat Module"), sizeof(colourid.group));
	colourid.defcolour = GetSysColor(COLOR_WINDOW);
	CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "ColorNicklistBG", sizeof(colourid.setting));
	strncpy(colourid.name, Translate("Userlist Background"), sizeof(colourid.name));
	colourid.order = 0;
	strncpy(colourid.group, Translate("Chat Module"), sizeof(colourid.group));
	colourid.defcolour = GetSysColor(COLOR_WINDOW);
	CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "ColorNicklistLines", sizeof(colourid.setting));
	strncpy(colourid.name, Translate("Userlist Lines"), sizeof(colourid.name));
	colourid.order = 0;
	strncpy(colourid.group, Translate("Chat Module"), sizeof(colourid.group));
	colourid.defcolour = GetSysColor(COLOR_INACTIVEBORDER);
	CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);
}
