/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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
#include <io.h>

HANDLE hLoadSkinIcon, hLoadSkinProtoIcon;

struct StandardIconDescription
{
	int  id;
	char *description;
	int  resource_id;
	int  pf2;
};

struct StandardIconDescription mainIcons[] =
{
	{ SKINICON_OTHER_MIRANDA,     "Miranda IM",      -IDI_MIRANDA        },
	{ SKINICON_EVENT_MESSAGE,     "Message",         -IDI_RECVMSG        },
	{ SKINICON_EVENT_URL,         "URL",             -IDI_URL            },
	{ SKINICON_EVENT_FILE,        "File",            -IDI_FILE           },
	{ SKINICON_OTHER_USERONLINE,  "User Online",     -IDI_USERONLINE     },
	{ SKINICON_OTHER_GROUPOPEN,   "Group (Open)",    -IDI_GROUPOPEN      },
	{ SKINICON_OTHER_GROUPSHUT,   "Group (Closed)",  -IDI_GROUPSHUT      },
	{ SKINICON_OTHER_CONNECTING,  "Connecting",      -IDI_LOAD           },
	{ SKINICON_OTHER_ADDCONTACT,  "Add Contact",     -IDI_ADDCONTACT     },
	{ SKINICON_OTHER_USERDETAILS, "User Details",    -IDI_USERDETAILS    },
	{ SKINICON_OTHER_HISTORY,     "History",         -IDI_HISTORY        },
	{ SKINICON_OTHER_DOWNARROW,   "Down Arrow",      -IDI_DOWNARROW      },
	{ SKINICON_OTHER_FINDUSER,    "Find User",       -IDI_FINDUSER       },
	{ SKINICON_OTHER_OPTIONS,     "Options",         -IDI_OPTIONS        },
	{ SKINICON_OTHER_SENDEMAIL,   "Send E-mail",     -IDI_SENDEMAIL      },
	{ SKINICON_OTHER_DELETE,      "Delete",          -IDI_DELETE         },
	{ SKINICON_OTHER_RENAME,      "Rename",          -IDI_RENAME         },
	{ SKINICON_OTHER_SMS,         "SMS",             -IDI_SMS            },
	{ SKINICON_OTHER_SEARCHALL,   "Search All",      -IDI_SEARCHALL      },
	{ SKINICON_OTHER_TICK,        "Tick",            -IDI_TICK           },
	{ SKINICON_OTHER_NOTICK,      "No Tick",         -IDI_NOTICK         },
	{ SKINICON_OTHER_HELP,        "Help",            -IDI_HELP           },
	{ SKINICON_OTHER_MIRANDAWEB,  "Miranda Website", -IDI_MIRANDAWEBSITE },
	{ SKINICON_OTHER_TYPING,      "Typing",          -IDI_TYPING         },
	{ SKINICON_OTHER_SMALLDOT,    "Small Dot",       -IDI_SMALLDOT       },
	{ SKINICON_OTHER_FILLEDBLOB,  "Filled Blob",     -IDI_FILLEDBLOB     },
	{ SKINICON_OTHER_EMPTYBLOB,   "Empty Blob",      -IDI_EMPTYBLOB      },
};

struct StandardIconDescription statusIcons[] =
{
	{ ID_STATUS_OFFLINE,         "Offline",          -IDI_OFFLINE,       0xFFFFFFFF     },
	{ ID_STATUS_ONLINE,          "Online",           -IDI_ONLINE,        PF2_ONLINE     },
	{ ID_STATUS_AWAY,            "Away",             -IDI_AWAY,          PF2_SHORTAWAY  },
	{ ID_STATUS_NA,              "NA",               -IDI_NA,            PF2_LONGAWAY   },
	{ ID_STATUS_OCCUPIED,        "Occupied",         -IDI_OCCUPIED,      PF2_LIGHTDND   },
	{ ID_STATUS_DND,             "DND",              -IDI_DND,           PF2_HEAVYDND   },
	{ ID_STATUS_FREECHAT,        "Free for chat",    -IDI_FREE4CHAT,     PF2_FREECHAT   },
	{ ID_STATUS_INVISIBLE,       "Invisible",        -IDI_INVISIBLE,     PF2_INVISIBLE  },
	{ ID_STATUS_ONTHEPHONE,      "On the phone",     -IDI_ONTHEPHONE,    PF2_ONTHEPHONE },
	{ ID_STATUS_OUTTOLUNCH,      "Out to lunch",     -IDI_OUTTOLUNCH,    PF2_OUTTOLUNCH }
};

const char* mainIconsFmt   = "core_main_";
const char* statusIconsFmt = "core_status_";
const char* protoIconsFmt  = "%s Icons";

#define PROTOCOLS_PREFIX "Status Icons/"
#define GLOBAL_PROTO_NAME "*"

__inline int IcoLib_GetIcon(WPARAM wParam, LPARAM lParam)
{
	return CallService( MS_SKIN2_GETICON, wParam, lParam );
}

__inline int IcoLib_AddNewIcon(WPARAM wParam, LPARAM lParam)
{
	return CallService( MS_SKIN2_ADDICON, wParam, lParam );
}

// load small icon (shared) it's not need to be destroyed

static HICON LoadSmallIconShared(HINSTANCE hInstance, LPCTSTR lpIconName)
{
	int cx = GetSystemMetrics(SM_CXSMICON);
	return LoadImage( hInstance, lpIconName, IMAGE_ICON,cx, cx, LR_DEFAULTCOLOR | LR_SHARED );
}

// load small icon (not shared) it IS NEED to be destroyed
static HICON LoadSmallIcon(HINSTANCE hInstance, LPCTSTR lpIconName)
{
	HICON hIcon = NULL;				  // icon handle 
	int index = -( int )lpIconName;
	TCHAR filename[MAX_PATH] = {0};
	GetModuleFileName( hInstance, filename, MAX_PATH );
	ExtractIconEx( filename, index, NULL, &hIcon, 1 );
	return hIcon;
}

// load small icon from hInstance
HICON LoadIconEx(HINSTANCE hInstance, LPCTSTR lpIconName, BOOL bShared)
{
	HICON hResIcon=bShared?LoadSmallIcon(hInstance,lpIconName):LoadSmallIconShared(hInstance,lpIconName);
	if (!hResIcon) //Icon not found in hInstance lets try to load it from core
	{
		HINSTANCE hCoreInstance=GetModuleHandle(NULL);
		if (hCoreInstance!=hInstance)
			hResIcon=bShared?LoadSmallIcon(hCoreInstance,lpIconName):LoadSmallIconShared(hCoreInstance,lpIconName);
	}
	return hResIcon;
}

int ImageList_AddIcon_NotShared(HIMAGELIST hIml, LPCTSTR szResource) 
{   
	HICON hTempIcon=LoadIconEx( GetModuleHandle(NULL), szResource, 0);
	int res=ImageList_AddIcon(hIml, hTempIcon);
	Safe_DestroyIcon(hTempIcon); 
	return res;
}

int ImageList_AddIcon_IconLibLoaded(HIMAGELIST hIml, int iconId) 
{  
	HICON hIcon = LoadSkinnedIcon( iconId );
	int res=ImageList_AddIcon(hIml, hIcon);
	IconLib_ReleaseIcon(hIcon,0);
	return res;
}

int ImageList_AddIcon_ProtoIconLibLoaded(HIMAGELIST hIml, const char* szProto, int iconId)
{  
	HICON hIcon = LoadSkinnedProtoIcon( szProto, iconId );
	int res=ImageList_AddIcon(hIml, hIcon);
	IconLib_ReleaseIcon(hIcon,0);
	return res;
}

int ImageList_ReplaceIcon_NotShared(HIMAGELIST hIml, int iIndex, HINSTANCE hInstance, LPCTSTR szResource) 
{   
	HICON hTempIcon=LoadIconEx(hInstance, szResource, 0);
	int res=ImageList_ReplaceIcon(hIml, iIndex, hTempIcon);
	Safe_DestroyIcon(hTempIcon); 
	return res;
}

int ImageList_ReplaceIcon_IconLibLoaded(HIMAGELIST hIml, int nIndex, HICON hIcon) 
{   
	int res=ImageList_ReplaceIcon(hIml,nIndex, hIcon);
	IconLib_ReleaseIcon(hIcon,0);
	return res;
}

void Window_SetIcon_IcoLib(HWND hWnd, int iconId)
{
	HICON hIcon = LoadSkinnedIcon( iconId );
	SendMessage(hWnd, WM_SETICON, ICON_BIG, ( LPARAM )hIcon);
}

void Window_SetProtoIcon_IcoLib(HWND hWnd, const char* szProto, int iconId)
{
	HICON hIcon = LoadSkinnedProtoIcon( szProto, iconId );
	SendMessage(hWnd, WM_SETICON, ICON_BIG, ( LPARAM )hIcon);
}

void Window_FreeIcon_IcoLib(HWND hWnd)
{
	HICON hIcon = ( HICON )SendMessage(hWnd, WM_SETICON, ICON_BIG, ( LPARAM )NULL);
	IconLib_ReleaseIcon(hIcon, 0);
}

void Button_SetIcon_IcoLib(HWND hwndDlg, int itemId, int iconId, const char* tooltip)
{
	HWND hWnd = GetDlgItem( hwndDlg, itemId );
	SendMessage( hWnd, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinnedIcon( iconId ));
	SendMessage( hWnd, BUTTONSETASFLATBTN, 0, 0 );
	SendMessage( hWnd, BUTTONADDTOOLTIP, (WPARAM)Translate(tooltip), 0);
}

void Button_FreeIcon_IcoLib(HWND hwndDlg, int itemId)
{
	HICON hIcon = ( HICON )SendDlgItemMessage(hwndDlg, itemId, BM_SETIMAGE, IMAGE_ICON, 0 );
	IconLib_ReleaseIcon(hIcon,0);
}

//
//  wParam = szProto
//  lParam = status
//
static int LoadSkinProtoIcon(WPARAM wParam,LPARAM lParam)
{
	char* szProto = (char*)wParam;
	int i, statusIndx = -1;
	char iconName[MAX_PATH];
	HICON hIcon;
	int suffIndx;
	DWORD caps2 = ( szProto == NULL ) ? ( DWORD )-1 : CallProtoService(szProto,PS_GETCAPS,PFLAGNUM_2,0);

	if ( lParam >= ID_STATUS_CONNECTING && lParam < ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES ) {
		sprintf( iconName, "%s%d", mainIconsFmt, 7 );
		return IcoLib_GetIcon( 0, ( LPARAM )iconName );
	}

	for ( i = 0; i < SIZEOF(statusIcons); i++ ) {
		if ( statusIcons[i].id == lParam ) {
			statusIndx = i; 
			break;
	}	}

	if ( statusIndx == -1 )
		return (int)NULL;

	if ( !szProto ) {
		PROTOCOLDESCRIPTOR **proto;
		DWORD protoCount;

		CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
		// Only return a protocol specific icon if there is only one protocol
		// Otherwise return the global icon. This affects the global status menu mainly.
		if ( protoCount == 1 ) {
			HICON hIcon;

			// format: core_status_%proto%statusindex
			strcpy(iconName, statusIconsFmt);
			strcat(iconName, proto[0]->szName);
			itoa(statusIndx, iconName + strlen(iconName), 10);

			hIcon = (HICON)IcoLib_GetIcon(0, (LPARAM)iconName);
			if (hIcon) return (int)hIcon;
		}

		// format: core_status_%s%d
		strcpy(iconName, statusIconsFmt);
		strcat(iconName, GLOBAL_PROTO_NAME);
		itoa(statusIndx, iconName + strlen(iconName), 10);
		return (int)IcoLib_GetIcon(0, (LPARAM)iconName);
	}

	// format: core_status_%s%d
	mir_snprintf(iconName, SIZEOF(iconName), "%s%s%d", statusIconsFmt, szProto, statusIndx);
	hIcon = (HICON)IcoLib_GetIcon(0, (LPARAM)iconName);
	if ( hIcon == NULL && ( caps2 == 0 || ( caps2 & statusIcons[statusIndx].pf2 ))) {
		char szPath[MAX_PATH], szFullPath[MAX_PATH],*str;
		char iconId[MAX_PATH];
		SKINICONDESC sid = { 0 };
		//
		//  Queried protocol isn't in list, adding
		//
		strcpy(iconName, PROTOCOLS_PREFIX); suffIndx = strlen(iconName);
		CallProtoService(szProto,PS_GETNAME,sizeof(iconName)-suffIndx,(LPARAM)iconName+suffIndx);

		sid.cbSize = sizeof(sid);
		sid.cx = GetSystemMetrics(SM_CXSMICON);
		sid.cy = GetSystemMetrics(SM_CYSMICON);

		GetModuleFileNameA(GetModuleHandle(NULL), szPath, MAX_PATH);
		str = strrchr( szPath, '\\' );
		if ( str != NULL ) *str = 0;
		mir_snprintf( szFullPath, SIZEOF(szFullPath), "%s\\Icons\\proto_%s.dll", szPath, szProto );
		if ( GetFileAttributesA( szFullPath ) != INVALID_FILE_ATTRIBUTES )
			sid.pszDefaultFile = szFullPath;
		else {
			mir_snprintf( szFullPath, SIZEOF(szFullPath), "%s\\Plugins\\%s.dll", szPath, szProto );
			if (( int )ExtractIconExA( szFullPath, statusIcons[i].resource_id, NULL, &hIcon, 1 ) > 0 ) {
				DestroyIcon( hIcon );
				sid.pszDefaultFile = szFullPath;
				hIcon = NULL;
			}

			if ( sid.pszDefaultFile == NULL ) {
				if ( str != NULL )
					*str = '\\';
				sid.pszDefaultFile = szPath;
		}	}

		//
		// Add global icons to list
		//
		sid.pszSection = iconName;
		strcpy(iconId, statusIconsFmt);
		strcat(iconId, szProto);
		suffIndx = strlen(iconId);
		{
			int lowidx, highidx;
			if ( caps2 == 0 )
				lowidx = statusIndx, highidx = statusIndx+1;
			else
				lowidx = 0, highidx = SIZEOF(statusIcons);

			for ( i = lowidx; i < highidx; i++ ) {
				if ( caps2 == 0 || ( caps2 & statusIcons[i].pf2 )) {
					// format: core_%s%d
					itoa(i, iconId + suffIndx, 10);
					sid.pszName = iconId;
					sid.pszDescription = (char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,statusIcons[i].id,0);
					//statusIcons[i].description;
					sid.iDefaultIndex = statusIcons[i].resource_id;
					IcoLib_AddNewIcon(0, (LPARAM)&sid);
		}	}	}

		// format: core_status_%s%d
		strcpy(iconName, statusIconsFmt);
		strcat(iconName, szProto);
		itoa(statusIndx, iconName + strlen(iconName), 10);
		hIcon = (HICON) IcoLib_GetIcon(0, (LPARAM)iconName);
		if ( hIcon ) 
			return ( int )hIcon;
	}

	if ( hIcon == NULL ) {
		mir_snprintf( iconName, SIZEOF(iconName), "%s%s%d", statusIconsFmt, GLOBAL_PROTO_NAME, statusIndx );
		hIcon = (HICON)IcoLib_GetIcon(0, (LPARAM)iconName);
	}
 
	return (int)hIcon;
}

static int LoadSkinIcon(WPARAM wParam, LPARAM lParam)
{
	int i;
	//
	//  Query for global status icons
	//
	if ( wParam < SKINICON_EVENT_MESSAGE ) {
		if ( wParam >= SIZEOF( statusIcons ))
			return (int)(HICON)NULL;

		return (int)LoadSkinProtoIcon((WPARAM)(char*)NULL,statusIcons[wParam].id);
	}

	for (i = 0; i < SIZEOF(mainIcons); i++) {
		if (wParam == (WPARAM)mainIcons[i].id) {
			char iconName[64];
			strcpy(iconName, mainIconsFmt);
			itoa(i, iconName + strlen(iconName), 10);
			return (int)IcoLib_GetIcon(0, (LPARAM)iconName);
	}	}

	return (int)NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Initializes the icon skin module

static void convertOneProtocol( char* moduleName, char* iconName )
{
	char* pm = moduleName + strlen( moduleName );
	char* pi = iconName + strlen( iconName );
	DBVARIANT dbv;
	int i;

	for ( i = 0; i < SIZEOF(statusIcons); i++ ) {
		itoa( statusIcons[i].id, pm, 10 );

		if ( !DBGetContactSettingTString( NULL, "Icons", moduleName, &dbv ) ) {
			itoa( i, pi, 10 );

			DBWriteContactSettingTString( NULL, "SkinIcons", iconName, dbv.ptszVal );
			DBFreeVariant( &dbv );

			DBDeleteContactSetting( NULL, "Icons", moduleName );
}	}	}

int InitSkinIcons(void)
{
	SKINICONDESC sid;
	int i, j = 0;
	char iconName[MAX_PATH];
	char moduleName[MAX_PATH];
	int iconNameSuffIndx;
	DBVARIANT dbv;

	//
	//  Perform "1st-time running import"

	for ( i = 0; i < SIZEOF(mainIcons); i++ ) {
		itoa( mainIcons[i].id, moduleName, 10 );
		if ( DBGetContactSettingTString( NULL, "Icons", moduleName, &dbv ))
			break;

		strcpy(iconName, mainIconsFmt);
		itoa(i, iconName + strlen(iconName), 10);

		DBWriteContactSettingTString( NULL, "SkinIcons", iconName, dbv.ptszVal );
		DBFreeVariant( &dbv );

		DBDeleteContactSetting( NULL, "Icons", moduleName );
	}

	while ( TRUE ) {
		// get the next protocol name
		moduleName[0] = 'p';
		moduleName[1] = 0;
		itoa( j++, moduleName+1, 100 );
		if ( DBGetContactSettingTString( NULL, "Icons", moduleName, &dbv ))
			break;

		DBDeleteContactSetting( NULL, "Icons", moduleName );

		// make old skinicons' prefix
		mir_snprintf( moduleName, SIZEOF(moduleName), TCHAR_STR_PARAM, dbv.ptszVal );
		// make IcoLib's prefix
		mir_snprintf( iconName, SIZEOF(iconName), "%s" TCHAR_STR_PARAM, statusIconsFmt, dbv.ptszVal );

		convertOneProtocol( moduleName, iconName );
		DBFreeVariant( &dbv );
	}
	moduleName[0] = 0;
	strcpy(iconName, "core_status_" GLOBAL_PROTO_NAME);
	convertOneProtocol( moduleName, iconName );

	hLoadSkinIcon = CreateServiceFunction(MS_SKIN_LOADICON,LoadSkinIcon);
	hLoadSkinProtoIcon = CreateServiceFunction(MS_SKIN_LOADPROTOICON,LoadSkinProtoIcon);

	ZeroMemory( &sid, sizeof(sid) );
	sid.cbSize = sizeof(sid);
	sid.cx = GetSystemMetrics(SM_CXSMICON);
	sid.cy = GetSystemMetrics(SM_CYSMICON);
	GetModuleFileNameA(NULL, moduleName, sizeof(moduleName));
	sid.pszDefaultFile = moduleName;
	//
	//  Add main icons to list
	//
	sid.pszSection = "Main Icons";
	strcpy(iconName, mainIconsFmt);

	iconNameSuffIndx = strlen(iconName);
	for ( i = 0; i < SIZEOF(mainIcons); i++ ) {
		itoa(i, iconName + iconNameSuffIndx, 10);
		sid.pszName = iconName;
		sid.pszDescription = mainIcons[i].description;
		sid.iDefaultIndex = mainIcons[i].resource_id;
		IcoLib_AddNewIcon(0, (LPARAM)&sid);
	}
	//
	// Add global icons to list
	//
	sid.pszSection = PROTOCOLS_PREFIX "Global";
	//
	// Asterisk is used, to avoid conflict with proto-plugins
	// 'coz users can't rename it to name with '*'
	strcpy(iconName, "core_status_" GLOBAL_PROTO_NAME);
	iconNameSuffIndx = strlen(iconName);
	for ( i = 0; i < SIZEOF(statusIcons); i++ ) {
		itoa(i, iconName + iconNameSuffIndx, 10);
		sid.pszName = iconName;
		sid.pszDescription = statusIcons[i].description;
		sid.iDefaultIndex = statusIcons[i].resource_id;
		IcoLib_AddNewIcon(0, (LPARAM)&sid);
	}
	return 0;
}
