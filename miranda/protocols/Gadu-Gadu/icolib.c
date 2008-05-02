////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2007 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"

struct
{
	char*  szDescr;
	char*  szName;
	int    defIconID;
	HANDLE hIconLibItem;
}
static iconList[] =
{
	{ LPGEN("Protocol icon"),				"main",			IDI_GG				},
	{ LPGEN("Import list from server"),		"importserver",	IDI_IMPORT_SERVER	},
	{ LPGEN("Import list from text file"),	"importtext",	IDI_IMPORT_TEXT 	},
	{ LPGEN("Remove list from server"),		"removeserver",	IDI_REMOVE_SERVER	},
	{ LPGEN("Export list to server"),		"exportserver",	IDI_EXPORT_SERVER	},
	{ LPGEN("Export list to text file"),	"exporttext",	IDI_EXPORT_TEXT 	},
	{ LPGEN("Account settings"),			"settings",		IDI_SETTINGS		},
	{ LPGEN("Blocked to this contact"),		"blocked",		IDI_STOP			},
	{ LPGEN("Previous image"),				"previous",		IDI_PREV			},
	{ LPGEN("Next image"),					"next",			IDI_NEXT			},
	{ LPGEN("Send image"), 					"image",		IDI_IMAGE			},
	{ LPGEN("Save image"),					"save",			IDI_SAVE			},
	{ LPGEN("Delete image"),				"delete",		IDI_DELETE			},
	{ LPGEN("Open new conference"),			"conference",	IDI_CONFERENCE		}
};

static int skinIconStatusToResourceId[] = 	{IDI_OFFLINE,	IDI_ONLINE,	IDI_AWAY,	IDI_INVISIBLE};
static int skinStatusToGGStatus[] = 		{0,				1,			2,			3};

void gg_icolib_init()
{
	SKINICONDESC sid = {0};
	char szFile[MAX_PATH], szSection[MAX_PATH];
	int i;

	GetModuleFileNameA(hInstance, szFile, MAX_PATH);

	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszDefaultFile = szFile;
	sid.cx = sid.cy = 16;
    mir_snprintf(szSection, sizeof(szSection), "%s/%s", Translate("Protocols"), Translate(GG_PROTONAME));
	sid.pszSection = szSection;

	for(i = 0; i < sizeof(iconList) / sizeof(iconList[0]); i++) {
		char szSettingName[100];
		mir_snprintf(szSettingName, sizeof(szSettingName), "%s_%s", GG_PROTO, iconList[i].szName);
		sid.pszName = szSettingName;
		sid.pszDescription = Translate(iconList[i].szDescr);
		sid.iDefaultIndex = -iconList[i].defIconID;
		iconList[i].hIconLibItem = (HANDLE) CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
	}
}

HICON LoadIconEx(int iconId)
{
	int i;
	for(i = 0; i < sizeof(iconList) / sizeof(iconList[0]); i++)
		if(iconList[i].defIconID == iconId)
			return (HICON) CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)iconList[i].hIconLibItem);

	return NULL;
}

HANDLE GetIconHandle(int iconId)
{
	int i;
	for(i = 0; i < sizeof(iconList) / sizeof(iconList[0]); i++)
		if (iconList[i].defIconID == iconId)
			return iconList[i].hIconLibItem;
	return NULL;
}

void gg_refreshblockedicon()
{
	// Store blocked icon
	char strFmt1[MAX_PATH];
	DBVARIANT dbv;
	GetModuleFileName(hInstance, strFmt1, sizeof(strFmt1));
	mir_snprintf(strFmt1, sizeof(strFmt1), "%s_blocked", GG_PROTO, ID_STATUS_DND);
	if(!DBGetContactSettingString(NULL, "SkinIcons", strFmt1, &dbv))
	{
		DBWriteContactSettingString(NULL, "Icons", strFmt1, dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else 
	{
		char strFmt2[MAX_PATH];
		mir_snprintf(strFmt2, sizeof(strFmt2), "%s,-%d", strFmt1, IDI_STOP);
		DBWriteContactSettingString(NULL, "Icons", strFmt1, strFmt2);
	}
}

int gg_iconschanged(WPARAM wParam, LPARAM lParam)
{
	gg_refreshblockedicon();

	return 0;
}
