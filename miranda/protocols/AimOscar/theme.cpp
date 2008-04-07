#include "aim.h"
#include "theme.h"

HMODULE  themeAPIHandle = NULL; // handle to uxtheme.dll
HANDLE   (WINAPI *MyOpenThemeData)(HWND,LPCWSTR) = 0;
HRESULT  (WINAPI *MyCloseThemeData)(HANDLE) = 0;
HRESULT  (WINAPI *MyDrawThemeBackground)(HANDLE,HDC,int,int,const RECT *,const RECT *) = 0;
void ThemeSupport()
{
	if (IsWinVerXPPlus()) {
		if (!themeAPIHandle) {
			themeAPIHandle = GetModuleHandleA("uxtheme");
			if (themeAPIHandle)
			{
				MyOpenThemeData = (HANDLE (WINAPI *)(HWND,LPCWSTR))MGPROC("OpenThemeData");
				MyCloseThemeData = (HRESULT (WINAPI *)(HANDLE))MGPROC("CloseThemeData");
				MyDrawThemeBackground = (HRESULT (WINAPI *)(HANDLE,HDC,int,int,const RECT *,const RECT *))MGPROC("DrawThemeBackground");
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Icons init

struct _tag_iconList
{
	char*  szDescr;
	char*  szName;
	int    defIconID;
	char*  szSection;
	HANDLE hIconLibItem;
}
static iconList[] =
{
	{	"ICQ",                    "icq",         IDI_ICQ             },
	{	"Add",                    "add",         IDI_ADD             },
	{	"Profile",                "profile",     IDI_PROFILE         },
	{	"AOL Mail",               "mail",        IDI_MAIL            },
	{	"AIM Icon",               "aim",         IDI_AIM             },     
	{	"Hiptop",                 "hiptop",      IDI_HIPTOP          },
	{	"AOL Bot",                "bot",         IDI_BOT             },
	{	"Admin",                  "admin",       IDI_ADMIN           },
	{	"Confirmed",              "confirm",     IDI_CONFIRMED       },
	{	"Not Confirmed",          "uconfirm",    IDI_UNCONFIRMED     },
	{	"Blocked list",           "away",        IDI_AWAY            },
	{	"Idle",                   "idle",        IDI_IDLE            },
	{	"AOL",                    "aol",         IDI_AOL             },

	{	"Foreground Color",       "foreclr",     IDI_FOREGROUNDCOLOR, "Profile Editor" },
	{	"Background Color",       "backclr",     IDI_BACKGROUNDCOLOR, "Profile Editor" },
	{	"Bold",                   "bold",        IDI_BOLD,            "Profile Editor" },
	{	"Not Bold",               "nbold",       IDI_NBOLD,           "Profile Editor" },
	{	"Italic",                 "italic",      IDI_ITALIC,          "Profile Editor" },
	{	"Not Italic",             "nitalic",     IDI_NITALIC,         "Profile Editor" },
	{	"Underline",              "undrln",      IDI_UNDERLINE,       "Profile Editor" },
	{	"Not Underline",          "nundrln",     IDI_NUNDERLINE,      "Profile Editor" },
	{	"Subscript",              "sub_scrpt",   IDI_SUBSCRIPT,       "Profile Editor" },
	{	"Not Subscript",          "nsub_scrpt",  IDI_NSUBSCRIPT,      "Profile Editor" },
	{	"Superscript",            "sup_scrpt",   IDI_SUPERSCRIPT,     "Profile Editor" },
	{	"Not Superscript",        "nsup_scrpt",  IDI_NSUPERSCRIPT,    "Profile Editor" },
	{	"Normal Script",          "norm_scrpt",  IDI_NORMALSCRIPT,    "Profile Editor" },
	{	"Not Normal Script",      "nnorm_scrpt", IDI_NNORMALSCRIPT,   "Profile Editor" },
};

static const size_t icolstsz = sizeof(iconList)/sizeof(iconList[0]); 

void CAimProto::InitIcons(void)
{
	char szFile[MAX_PATH];
	GetModuleFileNameA(hInstance, szFile, MAX_PATH);

	char szSettingName[100];

	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszDefaultFile = szFile;
	sid.cx = sid.cy = 16;
	sid.pszName = szSettingName;

	for ( int i = 0; i < icolstsz; i++ ) {
		mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", m_szModuleName, iconList[i].szName );

		char szSectionName[100];
		if (iconList[i].szSection)
		{
			mir_snprintf( szSectionName, sizeof( szSectionName ), "%s/%s", m_szModuleName, iconList[i].szSection );
			sid.pszSection = Translate(szSectionName);
		}
		else
			sid.pszSection = Translate( m_szModuleName );

		sid.pszDescription = Translate( iconList[i].szDescr );
		sid.iDefaultIndex = -iconList[i].defIconID;
		iconList[i].hIconLibItem = ( HANDLE )CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
	}	
}

HICON CAimProto::LoadIconEx(const char* name)
{
	char szSettingName[100];
	mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", m_szModuleName, name );
	return ( HICON )CallService( MS_SKIN2_GETICON, 0, (LPARAM)szSettingName );
}

HANDLE CAimProto::GetIconHandle(const char* name)
{
	for (unsigned i=0; i < icolstsz; i++)
		if (strcmp(iconList[i].szName, name) == 0)
			return iconList[i].hIconLibItem;
	return NULL;
}

void CAimProto::ReleaseIconEx(const char* name)
{
	char szSettingName[100];
	mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", m_szModuleName, name );
	CallService( MS_SKIN2_RELEASEICON, 0, (LPARAM)szSettingName );
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnPreBuildContactMenu

int ExtraIconsRebuild(WPARAM wParam, LPARAM lParam);
int ExtraIconsApply(WPARAM wParam, LPARAM lParam);

int CAimProto::OnPreBuildContactMenu(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM mi;
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	//see if we should add the html away message context menu items
	if ( getWord((HANDLE)wParam, "Status", ID_STATUS_OFFLINE ) == ID_STATUS_AWAY )
		mi.flags=CMIM_FLAGS|CMIF_NOTOFFLINE;
	else
		mi.flags=CMIM_FLAGS|CMIF_NOTOFFLINE|CMIF_HIDDEN;

	CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)hHTMLAwayContextMenuItem,(LPARAM)&mi);
	char* item= new char[lstrlenA(AIM_KEY_BI)+10];
	mir_snprintf(item,lstrlenA(AIM_KEY_BI)+10,AIM_KEY_BI"%d",1);
	if ( !getWord((HANDLE)wParam, item, 0 ) && state == 1 )
		mi.flags=CMIM_FLAGS|CMIF_NOTONLINE;
	else
		mi.flags=CMIM_FLAGS|CMIF_NOTONLINE|CMIF_HIDDEN;

	delete[] item;
	CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)hAddToServerListContextMenuItem,(LPARAM)&mi);
	return 0;
}

void CAimProto::InitMenus()
{
	//Do not put any services below HTML get away message!!!
	char service_name[200];

	CLISTMENUITEM mi = {0};
	mi.cbSize = sizeof( mi );
	mi.popupPosition = 500090000;
	mi.position = 500090000;
	mi.pszContactOwner = m_szModuleName;
	mi.flags = CMIF_ROOTPOPUP | CMIF_ICONFROMICOLIB | CMIF_TCHAR;
	mi.icolibItem = GetIconHandle("aim");
	mi.ptszName = m_tszUserName;
	mi.pszPopupName = (char *)-1;
	hMenuRoot = (HANDLE)CallService( MS_CLIST_ADDMAINMENUITEM,  (WPARAM)0, (LPARAM)&mi);

	mi.pszService = service_name;
	mi.pszPopupName = (char *)hMenuRoot;
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_CHILDPOPUP;
	mir_snprintf(service_name, sizeof(service_name), "%s%s", m_szModuleName, "/ManageAccount");
	CreateProtoService("/ManageAccount",&CAimProto::ManageAccount);
	mi.icolibItem = GetIconHandle("aim");
	mi.pszName = LPGEN( "Manage Account" );
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

	mir_snprintf(service_name, sizeof(service_name), "%s%s", m_szModuleName, "/CheckMail");
	CreateProtoService("/CheckMail",&CAimProto::CheckMail);
	mi.icolibItem = GetIconHandle("mail");
	mi.pszName = LPGEN( "Check Mail" );
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

	mir_snprintf(service_name, sizeof(service_name), "%s%s", m_szModuleName, "/InstantIdle");
	CreateProtoService("/InstantIdle",&CAimProto::InstantIdle);
	mi.icolibItem = GetIconHandle("idle");
	mi.pszName = LPGEN( "Instant Idle" );
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

	mir_snprintf(service_name, sizeof(service_name), "%s%s", m_szModuleName, "/GetHTMLAwayMsg");
	CreateProtoService("/GetHTMLAwayMsg",&CAimProto::GetHTMLAwayMsg);
	mi.pszPopupName=Translate("Read &HTML Away Message");
	mi.popupPosition=-2000006000;
	mi.position=-2000006000;
	mi.icolibItem = GetIconHandle("away");
	mi.pszName = LPGEN("Read &HTML Away Message");
	mi.flags=CMIF_NOTOFFLINE|CMIF_HIDDEN|CMIF_ICONFROMICOLIB;
	hHTMLAwayContextMenuItem=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);

	mir_snprintf(service_name, sizeof(service_name), "%s%s", m_szModuleName, "/GetProfile");
	CreateProtoService("/GetProfile",&CAimProto::GetProfile);
	mi.pszPopupName=Translate("Read Profile");
	mi.popupPosition=-2000006500;
	mi.position=-2000006500;
	mi.icolibItem = GetIconHandle("profile");
	mi.pszName = LPGEN("Read Profile");
	mi.flags=CMIF_NOTOFFLINE|CMIF_ICONFROMICOLIB;
	hReadProfileMenuItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);

	mir_snprintf(service_name, sizeof(service_name), "%s%s", m_szModuleName, "/AddToServerList");
	CreateProtoService("/AddToServerList",&CAimProto::AddToServerList);
	mi.pszPopupName=Translate("Add To Server List");
	mi.popupPosition=-2000006500;
	mi.position=-2000006500;
	mi.icolibItem = GetIconHandle("add");
	mi.pszName = LPGEN("Add To Server List");
	mi.flags=CMIF_NOTONLINE|CMIF_HIDDEN|CMIF_ICONFROMICOLIB;
	hAddToServerListContextMenuItem=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);
}
