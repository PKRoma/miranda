/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000  Roland Rabien

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

For more information, e-mail figbug@users.sourceforge.net
*/
//#define WINVER 0x0500
//#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <stdio.h>

#include "../icqlib/icq.h"
#include "miranda.h"
#include "internal.h"
#include "global.h"
#include "plugins.h"
#include "pluginapi.h"

tagPlugin Plugin[MAX_PLUGINS];

PI_CALLBACK pi_cb;

//////////////////////////////
// Plugin callback messages //
//////////////////////////////

void pcbSendMessage(int uin, char *msg)
{
	icq_SendMessage(plink, uin, msg, opts.ICQ.sendmethod);
}

void pcbSendURL(int uin, char *desc, char *url)
{
	icq_SendURL(plink, uin, url, desc, opts.ICQ.sendmethod);
}

CONTACT* pcbGetContactFromUIN(int uin)
{
	int i;

	for (i = 0; i < opts.ccount; i++)
	{
		if (opts.clist[i].uin == (unsigned int)uin) return &opts.clist[i];
	}
	return NULL;
}

void pcbChangeStatus(int status)
{
	ChangeStatus(NULL, status);
}

int pcbGetStatus(void)
{
	return opts.ICQ.status;
}

void pcbGetPluginInt(char *plugin, int *val, char *name, int def)
{
	HKEY hkey;

	char regkey[128] = "Software\\FigBug Developments\\Miranda ICQ\\";
	
	UINT ret;
	if (rto.useini)
	{
		
		ret=GetPrivateProfileInt(plugin, name, def, rto.inifile);
		def=ret;
		*val=def;
	}
	else
	{
		long sz;
		int res;

		strcat(regkey, myprofile);
		strcat(regkey, "\\");
		strcat(regkey, plugin);
		
		RegCreateKey(HKEY_CURRENT_USER, regkey, &hkey);		
		
		if (hkey)
		{
			sz = sizeof(long);
			res = RegQueryValueEx(hkey, name, 0, NULL, (BYTE *)val, &sz);
			RegCloseKey(hkey);
		}
		if (hkey == NULL || res != ERROR_SUCCESS)
		{
			*val = def;
		}		
	}
}

void pcbGetPluginStr(char *plugin, long sz, char *val, char *name, char *def)
{
	HKEY hkey;

	char regkey[128] = "Software\\FigBug Developments\\Miranda ICQ\\";

	if (rto.useini)
	{
		
		GetPrivateProfileString(plugin, name, def, val, 128, rto.inifile);
	}
	else
	{
		int res;

		strcat(regkey, myprofile);
		strcat(regkey, "\\");
		strcat(regkey, plugin);
		
		RegCreateKey(HKEY_CURRENT_USER, regkey, &hkey);		
		
		if (hkey)
		{
			res = RegQueryValueEx(hkey, name, 0, NULL, val, &sz);
			RegCloseKey(hkey);
		}
		if (hkey == NULL || res != ERROR_SUCCESS)
		{
			strcpy(val, def);
		}
	}
}

void pcbSetPluginInt(char *plugin, int val, char *name)
{
	HKEY hkey;

	char regkey[128] = "Software\\FigBug Developments\\Miranda ICQ\\";

	if (rto.useini)
	{
		char buf[64];
		sprintf(buf, "%d", val);
		WritePrivateProfileString(plugin, name, buf, rto.inifile);
	}
	else
	{
		long sz;

		strcat(regkey, myprofile);
		strcat(regkey, "\\");
		strcat(regkey, plugin);
		
		RegCreateKey(HKEY_CURRENT_USER, regkey, &hkey);		
		
		if (hkey)
		{
			sz = sizeof(long);
			RegSetValueEx(hkey, name, 0, REG_DWORD, (BYTE *)&val, sz);
			RegCloseKey(hkey);
		}
	}
}

void pcbSetPluginStr(char *plugin, char *val, char *name)
{
	HKEY hkey;

	char regkey[128] = "Software\\FigBug Developments\\Miranda ICQ\\";

	if (rto.useini)
	{
		WritePrivateProfileString(plugin, name, val, rto.inifile);
	}
	else
	{
		long sz;

		strcat(regkey, myprofile);
		strcat(regkey, "\\");
		strcat(regkey, plugin);
		
		RegCreateKey(HKEY_CURRENT_USER, regkey, &hkey);		
		
		if (hkey)
		{
			sz = strlen(val)+1;
			RegSetValueEx(hkey, name, 0, REG_SZ, val, sz);
			RegCloseKey(hkey);
		}
	}
}


///////////////////
// Plugin system //
///////////////////

void Plugin_Init()
{
	pi_cb.pChangeStatus = pcbChangeStatus;
	pi_cb.pGetContact   = pcbGetContactFromUIN;
	pi_cb.pGetStatus    = pcbGetStatus;
	pi_cb.pSendMessage  = pcbSendMessage;
	pi_cb.pSendURL	    = pcbSendURL;
	pi_cb.pGetPluginInt = pcbGetPluginInt;
	pi_cb.pGetPluginStr = pcbGetPluginStr;
	pi_cb.pSetPluginInt = pcbSetPluginInt;
	pi_cb.pSetPluginStr = pcbSetPluginStr;	
}

void Plugin_Load()
{

	WIN32_FIND_DATA ffdata;
	
	HANDLE ff;
	char dir[MAX_PATH];
	char curfile[MAX_PATH];
	char wildcard[MAX_PATH];
	
	
	Plugin_GetDir(dir);
	strcpy(wildcard,dir);
	strcat(wildcard,"*.dll");


	if ((ff=FindFirstFile(wildcard,&ffdata))==INVALID_HANDLE_VALUE)
	{ //error
		//MessageBox(NULL,"Can't Find Plugins",szTitle,MB_OK);
		return;
	}
	goto loadfile;
	for(;;)
	{	
		if ((FindNextFile(ff,&ffdata))==0)
		{	//error
			if (GetLastError()==ERROR_NO_MORE_FILES)
			{
				break;
			}
		}
	loadfile:
		strcpy(curfile,dir);
		strcat(curfile,ffdata.cFileName);
		if ((Plugin[plugincnt].hinst=LoadLibrary(curfile))!=NULL)
		{ //loaded ok, get proc addresses
			Plugin[plugincnt].loadfunc=(FARPROC_LOAD)GetProcAddress(Plugin[plugincnt].hinst,"Load");
			Plugin[plugincnt].closefunc=(FARPROC_CLOSE)GetProcAddress(Plugin[plugincnt].hinst,"Unload");
			Plugin[plugincnt].notifyfunc=(FARPROC_NOTIFY)GetProcAddress(Plugin[plugincnt].hinst,"Notify");
			
			
			plugincnt++;
				
		}
		
	}

	FindClose(ff);
}

/*returns <app path>\plugins\ */

void Plugin_GetDir(char*plugpath)
{
	char path[260];
	strcpy(path, mydir);
	if (path[strlen(path)-1]!='\\')
	{ // add '\'
		strcat(path,"\\");
	}
	strcat(path,"plugins\\");
	strcpy(plugpath,path);
}



void Plugin_LoadPlugins(HWND hwnd,HINSTANCE hinstance)
{	
	char *verstr;
	HMENU hmen;
	int i;

	if (plugincnt)
	{
		HMENU hmen;
		hmen = GetMenu(ghwnd);
		hmen = GetSubMenu(hmen, 0);
		hmen = GetSubMenu(hmen, 1);

		RemoveMenu(hmen, 0, MF_BYPOSITION);
	}

	for (i=0;i<plugincnt;i++)
	{
		if (Plugin[i].loadfunc==NULL) 
		{//dll isnt working prop (didnt export a Load func)
			goto dlldidntload;
		}
		if ((*Plugin[i].loadfunc)(hwnd,hinstance,Plugin[i].Title)==FALSE)
		{
			//failed to load, kill the DLL
dlldidntload:
			FreeLibrary(Plugin[i].hinst);
			Plugin[i].hinst=NULL;
		}
		else
		{ //DLL loaded ok, add to menu (for options call)
			
			
			

			hmen=GetMenu(ghwnd);
			hmen=GetSubMenu(hmen,0);
			
			//NOTE: this needs to be changed if the menu structure changes
			hmen=GetSubMenu(hmen,1);
			AppendMenu(hmen,MF_STRING,IDC_PLUGMNUBASE+i,Plugin[i].Title);
		}		
	}
	//send PM_VERSION..has to be the first msg the plugin gets
	verstr=(char*)malloc(strlen(MIRANDAVERSTR)+1);
	strcpy(verstr,MIRANDAVERSTR);
	Plugin_NotifyPlugins(PM_VERSION,(WPARAM)strlen(MIRANDAVERSTR) , (LPARAM)verstr);
	free(verstr);

	Plugin_NotifyPlugins(PM_REGCALLBACK, (WPARAM)&pi_cb, 0);
	Plugin_NotifyPlugins(PM_START, 0, 0);
}
void Plugin_ClosePlugins()
{
	int i;
	
	__try
	{
		for (i=0;i<plugincnt;i++)
		{
			if (Plugin[i].hinst!=NULL)
			{
				(*Plugin[i].closefunc)();
				//free the lib
				FreeLibrary(Plugin[i].hinst);
			}

		}
	}
	__except (1)
	{
		char tmp[MAX_PLUG_TITLE+20];
		sprintf(tmp,"Plugin \"%s\" had errors (On Close).",Plugin[i].Title);
		MessageBox(ghwnd, tmp, MIRANDANAME, MB_OK);
	}
}

int Plugin_NotifyPlugins(long msg,WPARAM wParam,LPARAM lParam)
{
	int i;
	int ret = 0;

	__try
	{
		for (i=0;i<plugincnt;i++)
		{
			if (Plugin[i].hinst!=NULL)
				ret |= (*Plugin[i].notifyfunc)(msg,wParam,lParam);
		}
	}
	__except (1)
	{
		char tmp[MAX_PLUG_TITLE+20];
		sprintf(tmp,"Plugin \"%s\" had errors (On Notify,%d).",Plugin[i].Title,msg);
		MessageBox(ghwnd, tmp, MIRANDANAME, MB_OK);
	}
	return ret;
}

int Plugin_NotifyPlugin(int id,long msg,WPARAM wParam,LPARAM lParam)
{
	int ret;

	__try
	{
		if (Plugin[id].hinst!=NULL)
			ret = (*Plugin[id].notifyfunc)(msg,wParam,lParam);
	}
	__except (1)
	{
		char tmp[MAX_PLUG_TITLE+20];
		sprintf(tmp,"Plugin \"%s\" had errors (On Notify,%d).",Plugin[id].Title,msg);
		MessageBox(ghwnd, tmp, MIRANDANAME, MB_OK);
	}
	return ret;
}
