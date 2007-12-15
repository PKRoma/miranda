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
	{	"Foreground Color",       "foreclr",     IDI_FOREGROUNDCOLOR },
	{	"Background Color",       "backclr",     IDI_BACKGROUNDCOLOR },
	{	"Bold",                   "bold",        IDI_BOLD            },
	{	"Not Bold",               "nbold",       IDI_NBOLD           },
	{	"Italic",                 "italic",      IDI_ITALIC          },
	{	"Not Italic",             "nitalic",     IDI_NITALIC         },
	{	"Underline",              "undrln",      IDI_UNDERLINE       },
	{	"Not Underline",          "nundrln",     IDI_NUNDERLINE      },
	{	"Subscript",              "sub_scrpt",   IDI_SUBSCRIPT       },
	{	"Not Subscript",          "nsub_scrpt",  IDI_NSUBSCRIPT      },
	{	"Superscript",            "sup_scrpt",   IDI_SUPERSCRIPT     },
	{	"Not Superscript",        "nsup_scrpt",  IDI_NSUPERSCRIPT    },
	{	"Normal Script",          "norm_scrpt",  IDI_NORMALSCRIPT    },
	{	"Not Normal Script",      "nnorm_scrpt", IDI_NNORMALSCRIPT   },
};

static const size_t icolstsz = sizeof(iconList)/sizeof(iconList[0]); 

void InitIcons(void)
{
	char szFile[MAX_PATH];
	GetModuleFileNameA(conn.hInstance, szFile, MAX_PATH);

	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszDefaultFile = szFile;
	sid.cx = sid.cy = 16;
	sid.pszSection = Translate( AIM_PROTOCOL_NAME );

	for ( int i = 0; i < icolstsz; i++ ) {
		char szSettingName[100];
		mir_snprintf( szSettingName, sizeof( szSettingName ), "%s_%s", AIM_PROTOCOL_NAME, iconList[i].szName );
		sid.pszName = szSettingName;
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

