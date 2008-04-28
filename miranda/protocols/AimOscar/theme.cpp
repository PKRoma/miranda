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

void InitIcons(void)
{
	char szFile[MAX_PATH];
	GetModuleFileNameA(conn.hInstance, szFile, MAX_PATH);

	char szSettingName[100];

	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszDefaultFile = szFile;
	sid.cx = sid.cy = 16;
	sid.pszName = szSettingName;

	for ( int i = 0; i < icolstsz; i++ ) 
	{
		mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", AIM_PROTOCOL_NAME, iconList[i].szName );

		char szSectionName[100];
		if (iconList[i].szSection)
		{
			mir_snprintf( szSectionName, sizeof( szSectionName ), "%s/%s", AIM_PROTOCOL_NAME, iconList[i].szSection );
			sid.pszSection = Translate(szSectionName);
		}
		else
			sid.pszSection = Translate( AIM_PROTOCOL_NAME );

		sid.pszDescription = Translate( iconList[i].szDescr );
		sid.iDefaultIndex = -iconList[i].defIconID;
		iconList[i].hIconLibItem = ( HANDLE )CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
	}	
}

HICON  LoadIconEx(const char* name)
{
	char szSettingName[100];
	mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", AIM_PROTOCOL_NAME, name );
	return ( HICON )CallService( MS_SKIN2_GETICON, 0, (LPARAM)szSettingName );
}

HANDLE  GetIconHandle(const char* name)
{
	for (unsigned i=0; i < icolstsz; i++)
		if (strcmp(iconList[i].szName, name) == 0)
			return iconList[i].hIconLibItem;
	return NULL;
}

void  ReleaseIconEx(const char* name)
{
	char szSettingName[100];
	mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", AIM_PROTOCOL_NAME, name );
	CallService( MS_SKIN2_RELEASEICON, 0, (LPARAM)szSettingName );
}

