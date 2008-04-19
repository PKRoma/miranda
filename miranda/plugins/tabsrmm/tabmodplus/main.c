#include "tabmodplus.h"

HINSTANCE hinstance;

WNDPROC mainProc;

static HANDLE hEventCBButtonPressed,hEventCBInit, hEventDbWindowEvent,hEventDbSettingChanged, hEventDbOptionsInit, hEventDbPluginsLoaded,
hEventDbPreShutdown,hIcon1,hIcon0,hib1,hib0 ;	

int g_bShutDown=0;   	
int g_bStartup=0; 
BOOL bWOpened=FALSE;


struct MM_INTERFACE mmi;																					   


int OptionsInit(WPARAM,LPARAM);
PLUGINLINK *pluginLink;

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	MODULENAME,
	PLUGIN_MAKE_VERSION(0,0,0,7),
	"",
	"MaD_CLuSTeR",
	"daniok@yandex.ru",
	"� 2008 Danil Mozhar",
	"",
	UNICODE_AWARE,
	0,	
#ifdef _UNICODE
	// {A4975736-E73B-4823-B657-E68691180845}
		{ 0xa4975736, 0xe73b, 0x4823, { 0xb6, 0x57, 0xe6, 0x86, 0x91, 0x18, 0x8, 0x45 } }
#else
	// {D9DDEC14-7B5A-46c4-A4D9-20237C9E7440} 
		{ 0xd9ddec14, 0x7b5a, 0x46c4, { 0xa4, 0xd9, 0x20, 0x23, 0x7c, 0x9e, 0x74, 0x40 } }
#endif
	};


char* getMirVer(HANDLE hContact) 
	{	  
	char * szProto=NULL;
	char* msg=NULL;
	DBVARIANT dbv = {0};

	szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0); 
	if(!szProto) return NULL; 

	if(!DBGetContactSetting(hContact, szProto, "MirVer", &dbv)) 
		{
		msg=mir_strdup(dbv.pszVal);
		DBFreeVariant(&dbv);
		}
	 return msg;	
	}

TCHAR* getMenuEntry(int i) 
	{	  
	TCHAR* msg=NULL;
	char MEntry[256]={'\0'};
	DBVARIANT dbv = {0};
	mir_snprintf(MEntry,255,"MenuEntry_%u",i);
	if(!DBGetContactSettingTString(NULL, "tabmodplus",MEntry, &dbv)) 
		{
		msg=mir_tstrdup(dbv.ptszVal);
		DBFreeVariant(&dbv);
		}
	return msg; 
	}

int ChangeClientIconInStatusBar(WPARAM wparam,LPARAM lparam)
	{
	int i=0;
	HICON hIcon=NULL;
	char* msg = getMirVer((HANDLE)wparam);
	if(!msg) return 1;

	hIcon=(HICON)CallService(MS_FP_GETCLIENTICON,(WPARAM)msg,(LPARAM)0);
	if(!hIcon) return 1;

	if(ServiceExists(MS_MSG_MODIFYICON))
		{
		StatusIconData sid = {0}; 
		sid.cbSize = sizeof(sid); 
		sid.szModule = "tabmodplus";
		sid.hIcon=sid.hIconDisabled=hIcon;
		sid.dwId = 1;
		sid.szTooltip=msg;
		sid.flags=MBF_OWNERSTATE;
		CallService(MS_MSG_MODIFYICON,(WPARAM)wparam, (LPARAM)&sid); 
		}
	mir_free(msg);
	return 0;
	}


int PreShutdown(WPARAM wparam,LPARAM lparam)
	{	 

	g_bShutDown=1;
	if(hEventCBButtonPressed) UnhookEvent(hEventCBButtonPressed);
	if(hEventCBInit) UnhookEvent(hEventCBInit);
	UnhookEvent(hEventDbPluginsLoaded);
	UnhookEvent(hEventDbOptionsInit);
	UnhookEvent(hEventDbPreShutdown);

	UnhookEvent(hEventDbSettingChanged);
	UnhookEvent(hEventDbWindowEvent);

	return 0;
	}


static int GetContactHandle(WPARAM wparam,LPARAM lParam)
	{
	MessageWindowEventData *MWeventdata = (MessageWindowEventData*)lParam;

	if(g_bShutDown||!g_bClientInStatusBar)
		return 0;
	if(MWeventdata->uType == MSG_WINDOW_EVT_OPENING&&MWeventdata->hContact)
		{
		bWOpened=TRUE;
		ChangeClientIconInStatusBar((WPARAM)MWeventdata->hContact,0);
		}
	return 0;
	}
static int RegisterCustomButton(WPARAM wParam,LPARAM lParam)
	{
	if  (ServiceExists(MS_BB_ADDBUTTON))
		{
		BBButton bbd={0};
		bbd.cbSize=sizeof(BBButton);
		bbd.bbbFlags=BBBF_ISIMBUTTON|BBBF_ISLSIDEBUTTON|BBBF_ISPUSHBUTTON;
		bbd.dwButtonID=1;
		bbd.dwDefPos=200;
		bbd.hIcon=hib0;
		bbd.pszModuleName="Tabmodplus";
		bbd.ptszTooltip=_T("Insert [img] tag / surround selected text with [img][/img]");

		return CallService(MS_BB_ADDBUTTON, 0, (LPARAM)&bbd);
		}
	return 1;
	}

static int CustomButtonPressed(WPARAM wParam,LPARAM lParam)
	{
	CustomButtonClickData *cbcd=(CustomButtonClickData *)lParam;

	CHARRANGE cr;
	TCHAR* pszMenu[256]={0};//=NULL;
	int i=0;
	TCHAR* pszText=_T("");
	TCHAR* pszFormatedText=NULL;
	UINT textlenght=0;
	BBButton bbd={0};

	int state=0;

	if(strcmp(cbcd->pszModule,"Tabmodplus")||cbcd->dwButtonId!=1) return 0;

	bbd.cbSize=sizeof(BBButton);
	bbd.dwButtonID=1;
	bbd.pszModuleName="Tabmodplus";
	CallService(MS_BB_GETBUTTONSTATE, wParam, (LPARAM)&bbd);

	cr.cpMin = cr.cpMax = 0;
	SendDlgItemMessage(cbcd->hwndFrom,IDC_MESSAGE, EM_EXGETSEL, 0, (LPARAM)&cr); 
	textlenght=cr.cpMax-cr.cpMin;
	if(textlenght)
		{
		pszText=mir_alloc((textlenght+1)*sizeof(TCHAR));
		ZeroMemory(pszText,(textlenght+1)*sizeof(TCHAR));
		SendDlgItemMessage(cbcd->hwndFrom, IDC_MESSAGE,EM_GETSELTEXT, 0, (LPARAM)pszText);
		}

	if(cbcd->flags&BBCF_RIGHTBUTTON)
		state=1;
	else if(textlenght)
		state=2;
	else if(bbd.bbbFlags&BBSF_PUSHED)
		state=3;
	else 
		state=4;
	
	switch(state)
		{
		case 1:
			{
			int res=0;
			int menunum;
			int menulimit;
			HMENU hMenu=NULL;

			menulimit=DBGetContactSettingByte(NULL, "tabmodplus","MenuCount", 0);
			if(menulimit){
				hMenu = CreatePopupMenu();
				//pszMenu=malloc(menulimit*sizeof(TCHAR*));
				}
			else break;
			for(menunum=0;menunum<menulimit;menunum++)
				{
				pszMenu[menunum]=getMenuEntry(menunum);
				AppendMenu(hMenu, MF_STRING,menunum+1,	pszMenu[menunum]);
				}
			res = TrackPopupMenu(hMenu, TPM_RETURNCMD, cbcd->pt.x, cbcd->pt.y, 0, cbcd->hwndFrom, NULL);
			if(res==0) break;

			pszFormatedText=mir_alloc((textlenght+_tcslen(pszMenu[res-1])+2)*sizeof(TCHAR));
			ZeroMemory(pszFormatedText,(textlenght+_tcslen(pszMenu[res-1])+2)*sizeof(TCHAR));

			mir_sntprintf(pszFormatedText,(textlenght+_tcslen(pszMenu[res-1])+2)*sizeof(TCHAR),pszMenu[res-1],pszText);

			}break;
		case 2:
			{
			pszFormatedText=mir_alloc((textlenght+12)*sizeof(TCHAR));
			ZeroMemory(pszFormatedText,(textlenght+12)*sizeof(TCHAR));

			SendDlgItemMessage(cbcd->hwndFrom, IDC_MESSAGE,EM_GETSELTEXT, 0, (LPARAM)pszText);
			mir_sntprintf(pszFormatedText,(textlenght+12)*sizeof(TCHAR),_T("[img]%s[/img]"),pszText);

			bbd.ptszTooltip=0;
			bbd.hIcon=0;
			bbd.bbbFlags=BBSF_RELEASED;
			CallService(MS_BB_SETBUTTONSTATE, wParam, (LPARAM)&bbd);
			}break;
			
		case 3:
			{ 
			pszFormatedText=mir_alloc(6*sizeof(TCHAR));
			ZeroMemory(pszFormatedText,6*sizeof(TCHAR));

			_sntprintf(pszFormatedText,6*sizeof(TCHAR),_T("%s"),_T("[img]"));

			bbd.ptszTooltip=_T("Insert [/img] tag");
			bbd.hIcon=hib1;
			CallService(MS_BB_SETBUTTONSTATE, wParam, (LPARAM)&bbd);

			}break;
		case 4:
			{
			
				pszFormatedText=mir_alloc(7*sizeof(TCHAR));
				ZeroMemory(pszFormatedText,7*sizeof(TCHAR)); 
				_sntprintf(pszFormatedText,7*sizeof(TCHAR),_T("%s"),_T("[/img]"));

				bbd.ptszTooltip=_T("Insert [img] tag / surround selected text with [img][/img]");
				bbd.hIcon=hib0;
				CallService(MS_BB_SETBUTTONSTATE, wParam, (LPARAM)&bbd);

			}break;
		}

	while(pszMenu[i])
		{
		  mir_free(pszMenu[i]);
		  i++;
		}

	if(pszFormatedText) SendDlgItemMessage(cbcd->hwndFrom, IDC_MESSAGE, EM_REPLACESEL, TRUE, (LPARAM)pszFormatedText);
	
	if(textlenght) mir_free(pszText);
	if(pszFormatedText) mir_free(pszFormatedText);
	return 1;

	}

static int ContactSettingChanged(WPARAM wParam,LPARAM lParam)
	{
	DBCONTACTWRITESETTING *dbcws = (DBCONTACTWRITESETTING *) lParam;

	if(g_bShutDown||!g_bClientInStatusBar||!wParam||!bWOpened)
		return 0;	

	if (dbcws&&!strcmp(dbcws->szSetting,"MirVer")) 
		ChangeClientIconInStatusBar(wParam,0);

	return 0;
	}


int AddIcon(HICON icon, char *name, char *description)
	{
	SKINICONDESC sid = {0};
	sid.cbSize = sizeof(SKINICONDESC);
	sid.pszSection = "TabSRMM/Toolbar";
	sid.cx = sid.cy = 16;
	sid.pszDescription = description;
	sid.pszName = name;
	sid.hDefaultIcon = icon;

	return CallService(MS_SKIN2_ADDICON, 0, (LPARAM) &sid);
	}

#define MBF_OWNERSTATE        0x04
static int PluginInit(WPARAM wparam,LPARAM lparam)
	{
	g_bStartup=1;

	g_bIMGtagButton=DBGetContactSettingByte(NULL,"tabmodplus","IMGtagButton",1);	
	g_bClientInStatusBar=DBGetContactSettingByte(NULL,"tabmodplus","ClientIconInStatusBar",1);

	hEventDbOptionsInit=HookEvent(ME_OPT_INITIALISE,OptionsInit);
	hEventDbSettingChanged= HookEvent(ME_DB_CONTACT_SETTINGCHANGED,ContactSettingChanged);
	hEventDbWindowEvent=HookEvent(ME_MSG_WINDOWEVENT,GetContactHandle);


	if(g_bIMGtagButton)
		{
		hEventCBButtonPressed=HookEvent(ME_MSG_BUTTONPRESSED,CustomButtonPressed); 
		hEventCBInit=HookEvent(ME_MSG_TOOLBARLOADED,RegisterCustomButton);
		}
	if (g_bClientInStatusBar&&ServiceExists(MS_MSG_ADDICON)) 
		{ 
		StatusIconData sid = {0}; 
		sid.cbSize = sizeof(sid); 
		sid.szModule = "tabmodplus"; 
		sid.flags = MBF_OWNERSTATE|MBF_HIDDEN; 
		sid.dwId = 1;
		sid.szTooltip = 0; 
		sid.hIcon = sid.hIconDisabled = 0; 
		CallService(MS_MSG_ADDICON, 0, (LPARAM)&sid); 
		}
	if(TRUE){
		hIcon1	   = LoadIcon(hinstance, MAKEINTRESOURCE(IDI_IMGCLOSE	));
		hIcon0     = LoadIcon(hinstance, MAKEINTRESOURCE(IDI_IMGOPEN));
		hib1		=(HANDLE)AddIcon(hIcon1, "tabmodplus1", "[/img]");
		hib0		=(HANDLE)AddIcon(hIcon0, "tabmodplus0", "[img]");
		}

	g_bStartup=0;
	return 0;
	}

// {B78AFD78-0AD5-452d-A4D1-C02D01BAC2F9}
static const MUUID interfaces[] = {{ 0xb78afd78, 0xad5, 0x452d, { 0xa4, 0xd1, 0xc0, 0x2d, 0x1, 0xba, 0xc2, 0xf9 } }, MIID_LAST};
const  __declspec(dllexport) MUUID* MirandaPluginInterfaces(void){
	return interfaces;}

__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
	{

	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 7, 0, 0))
		return NULL;

	return &pluginInfo;
	}

__declspec(dllexport)int Unload(void)
	{
	return 0;
	}

BOOL WINAPI DllMain(HINSTANCE hinst,DWORD fdwReason,LPVOID lpvReserved)
	{
	hinstance=hinst;
	return 1;
	}

int __declspec(dllexport)Load(PLUGINLINK *link)
	{	

	pluginLink=link;
	mir_getMMI(&mmi);

	hEventDbPluginsLoaded=HookEvent(ME_SYSTEM_MODULESLOADED,PluginInit);
	hEventDbPreShutdown=HookEvent(ME_SYSTEM_PRESHUTDOWN,PreShutdown);
	return 0;
	}
