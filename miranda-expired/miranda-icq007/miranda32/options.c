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
#include <string.h>

#include "miranda.h"
#include "Encryption.h"
#include "global.h"
#include "plugins.h"
#include "pluginapi.h"


	#include "MSN_global.h"


char tmpfn[MAX_PATH]; //temp to hold full path for ini file

//defines
#define T_STR		0
#define T_INT		1

// types
typedef struct 
{
	enum OPT_IDX idx;
	char *name;
	int type;	
	unsigned int defInt;
	char *defChar;
} REG_OPT;

void LoadFromReg(HKEY hkey, char *postkey, enum OPT_IDX idx, void *data);
void SaveToReg(HKEY hkey, char *postkey, enum OPT_IDX idx, void *data);
HKEY GetUserRegKey(char *postkey);
HKEY GetAppRegKey(void);

void GetMirandaInt(int *val, HKEY hkey, char *name, int def);
void GetMirandaStr(char *val, long sz, HKEY hkey, char *name, char *def);
void SetMirandaInt(int val, HKEY hkey, char *name);
void SetMirandaStr(char *val, HKEY hkey, char *name);

void LoadOptions(OPTIONS *opts, char *postkey)
{
	HKEY hkey;
	HKEY hkeyapp;

	char tmp[16];
	
	if (!rto.useini)
	{
		if (!postkey) postkey = "";
		hkey = GetUserRegKey(postkey);
		hkeyapp = GetAppRegKey();
	}

	strcpy(tmpfn, rto.inifile);
	//main window pos
	GetMirandaInt(&opts->pos_mainwnd.xpos, hkey, "xpos", 50);
	GetMirandaInt(&opts->pos_mainwnd.ypos, hkey, "ypos", 50);
	GetMirandaInt(&opts->pos_mainwnd.width, hkey, "width", 150);
	GetMirandaInt(&opts->pos_mainwnd.height, hkey, "height", 260);
	//send msg window pos
	GetMirandaInt(&opts->pos_sendmsg.xpos, hkey, "xpos_sendmsg", -1);
	GetMirandaInt(&opts->pos_sendmsg.ypos, hkey, "ypos_sendmsg", -1);
	GetMirandaInt(&opts->pos_sendmsg.width, hkey, "width_sendmsg", -1);
	GetMirandaInt(&opts->pos_sendmsg.height, hkey, "height_sendmsg", -1);
	//send url window pos
	GetMirandaInt(&opts->pos_sendurl.xpos, hkey, "xpos_sendurl", -1);
	GetMirandaInt(&opts->pos_sendurl.ypos, hkey, "ypos_sendurl", -1);
	GetMirandaInt(&opts->pos_sendurl.width, hkey, "width_sendurl", -1);
	GetMirandaInt(&opts->pos_sendurl.height, hkey, "height_sendurl", -1);
	//recv msg window pos
	GetMirandaInt(&opts->pos_recvmsg.xpos, hkey, "xpos_recvmsg", -1);
	GetMirandaInt(&opts->pos_recvmsg.ypos, hkey, "ypos_recvmsg", -1);
	GetMirandaInt(&opts->pos_recvmsg.width, hkey, "width_recvmsg", -1);
	GetMirandaInt(&opts->pos_recvmsg.height, hkey, "height_recvmsg", -1);
	//recv url window pos
	GetMirandaInt(&opts->pos_recvurl.xpos, hkey, "xpos_recvurl", -1);
	GetMirandaInt(&opts->pos_recvurl.ypos, hkey, "ypos_recvurl", -1);
	GetMirandaInt(&opts->pos_recvurl.width, hkey, "width_recvurl", -1);
	GetMirandaInt(&opts->pos_recvurl.height, hkey, "height_recvurl", -1);

	
	GetMirandaInt(&opts->flags1, hkey, "flags_1", 0);
	GetMirandaInt(&opts->alpha, hkey, "alpha", 200);
	GetMirandaInt(&opts->autoalpha, hkey, "autoalpha", 150);
	GetMirandaInt(&opts->playsounds, hkey, "sounds", 1);
	GetMirandaInt(&opts->ICQ.laststatus, hkey, "defmode", -1);
	GetMirandaInt(&opts->hotkey_showhide, hkey, "hotkey_showhide", 'A');
	GetMirandaInt(&opts->hotkey_setfocus, hkey, "hotkey_setfocus", 'I');
	GetMirandaInt(&opts->hotkey_netsearch, hkey, "hotkey_netsearch", 'S');
	GetMirandaStr(opts->netsearchurl, sizeof(opts->netsearchurl), hkey, "netsearchURL", "Http://www.Google.com");
	//GetMirandaInt(&opts->newctl, hkey, "newctl", 0);

	GetMirandaInt(&opts->ICQ.enabled, hkey, "ICQ_Enabled",TRUE);
	GetMirandaInt(&opts->ICQ.sendmethod, hkey, "ICQ_SendMethod",ICQ_SEND_THRUSERVER);

	GetMirandaInt(&opts->ICQ.uin, hkey, "uin", 0);
	GetMirandaInt(&opts->ICQ.port, hkey, "port", 4000);

	GetMirandaStr(tmp, sizeof(tmp), hkey, "passwordenc", "");
	if (tmp[0]==0)
	{ //no enc password there -check for a non encryped password
		GetMirandaStr(opts->ICQ.password, sizeof(opts->ICQ.password), hkey, "password", "mc_hammer");
	}
	else
	{
		Encrypt(tmp,FALSE);
		strcpy(opts->ICQ.password,tmp);
	}
	GetMirandaStr(opts->ICQ.hostname, sizeof(opts->ICQ.hostname), hkey, "hostname", "icq.mirabilis.com");
	
	
	GetMirandaInt(&opts->ICQ.proxyuse, hkey, "ICQ_proxyuse", 0);
	GetMirandaStr(opts->ICQ.proxyhost, sizeof(opts->ICQ.proxyhost), hkey, "ICQ_proxyhost", "localhost");
	GetMirandaInt(&opts->ICQ.proxyport, hkey, "ICQ_proxyport", 1234);
	GetMirandaInt(&opts->ICQ.proxyauth, hkey, "ICQ_proxyauth", 0);
	GetMirandaStr(opts->ICQ.proxyname, sizeof(opts->ICQ.proxyname), hkey, "ICQ_proxyname", "username");
	GetMirandaStr(opts->ICQ.proxypass, sizeof(opts->ICQ.proxypass), hkey, "ICQ_proxypass", "password");
	

	GetMirandaStr(opts->contactlist, sizeof(opts->contactlist), hkey, "contactlist", "default.cn2");	
	GetMirandaStr(opts->history, sizeof(opts->history), hkey, "history", "default.hst");
	GetMirandaStr(opts->grouplist, sizeof(opts->grouplist), hkey, "group", "default.grp");


	//MSN settings
	GetMirandaInt(&opts->MSN.enabled,hkey,"MSN_Enabled",FALSE);
	GetMirandaStr(opts->MSN.uhandle,sizeof(opts->MSN.uhandle), hkey,"MSN_uhandle","");
	GetMirandaStr(opts->MSN.password,sizeof(opts->MSN.password), hkey,"MSN_passwordenc","");
	Encrypt(opts->MSN.password,FALSE);

	
	//docking
	GetMirandaInt(&opts->dockstate,hkey,"dockstate",DOCK_NONE);
	if (!rto.useini)
	{	
		RegCloseKey(hkey);
		RegCloseKey(hkeyapp);
	}
}

void SaveOptions(OPTIONS *opts, char *postkey)
{
	HKEY hkey;
	HKEY hkeyapp;
	
	char *h;
	char *c;
	char *g;

	Plugin_NotifyPlugins(PM_SAVENOW, 0, 0);

	if (!rto.useini)
	{
		if (!postkey) postkey = "";
		hkey = GetUserRegKey(postkey);
		hkeyapp = GetAppRegKey();
	}

	strcpy(tmpfn, rto.inifile);
	//main window pso
	SetMirandaInt(opts->pos_mainwnd.xpos, hkey, "xpos");
	SetMirandaInt(opts->pos_mainwnd.ypos, hkey, "ypos");
	SetMirandaInt(opts->pos_mainwnd.width, hkey, "width");
	SetMirandaInt(opts->pos_mainwnd.height, hkey, "height");
	//send msg window pos
	SetMirandaInt(opts->pos_sendmsg.xpos, hkey, "xpos_sendmsg");
	SetMirandaInt(opts->pos_sendmsg.ypos, hkey, "ypos_sendmsg");
	SetMirandaInt(opts->pos_sendmsg.width, hkey, "width_sendmsg");
	SetMirandaInt(opts->pos_sendmsg.height, hkey, "height_sendmsg");
	//send URL window pos
	SetMirandaInt(opts->pos_sendurl.xpos, hkey, "xpos_sendurl");
	SetMirandaInt(opts->pos_sendurl.ypos, hkey, "ypos_sendurl");
	SetMirandaInt(opts->pos_sendurl.width, hkey, "width_sendurl");
	SetMirandaInt(opts->pos_sendurl.height, hkey, "height_sendurl");
	//recv msg window pos
	SetMirandaInt(opts->pos_recvmsg.xpos, hkey, "xpos_recvmsg");
	SetMirandaInt(opts->pos_recvmsg.ypos, hkey, "ypos_recvmsg");
	SetMirandaInt(opts->pos_recvmsg.width, hkey, "width_recvmsg");
	SetMirandaInt(opts->pos_recvmsg.height, hkey, "height_recvmsg");
	//recv URL window pos
	SetMirandaInt(opts->pos_recvurl.xpos, hkey, "xpos_recvurl");
	SetMirandaInt(opts->pos_recvurl.ypos, hkey, "ypos_recvurl");
	SetMirandaInt(opts->pos_recvurl.width, hkey, "width_recvurl");
	SetMirandaInt(opts->pos_recvurl.height, hkey, "height_recvurl");

	
	SetMirandaInt(opts->flags1, hkey, "flags_1");
	SetMirandaInt(opts->alpha, hkey, "alpha");
	SetMirandaInt(opts->autoalpha, hkey, "autoalpha");
	SetMirandaInt(opts->playsounds, hkey, "sounds");
	SetMirandaInt(opts->ICQ.laststatus, hkey, "defmode");
	SetMirandaInt(opts->hotkey_showhide, hkey, "hotkey_showhide");
	SetMirandaInt(opts->hotkey_setfocus, hkey, "hotkey_setfocus");
	SetMirandaInt(opts->hotkey_netsearch, hkey, "hotkey_netsearch");
	SetMirandaStr(opts->netsearchurl, hkey, "netsearchURL");
	//SetMirandaInt(opts->newctl, hkey, "newctl");

	SetMirandaInt(opts->ICQ.enabled, hkey, "ICQ_Enabled");

	SetMirandaInt(opts->ICQ.sendmethod, hkey, "ICQ_SendMethod");

	SetMirandaInt(opts->ICQ.uin, hkey, "uin");
	SetMirandaInt(opts->ICQ.port, hkey, "port");

	//NO LONGER save it as a non-encryped password, only as a encytped password
	//delte the "password" key
	RegDeleteValue(hkey,"password");

	Encrypt(opts->ICQ.password,TRUE);
	SetMirandaStr(opts->ICQ.password, hkey, "passwordenc");
	Encrypt(opts->ICQ.password,FALSE);

	c = strrchr(opts->contactlist, '\\');
	if (c)
		c++;
	else
		c = opts->contactlist;

	h = strrchr(opts->history, '\\');
	if (h)
		h++;
	else
		h = opts->history;

	g = strrchr(opts->grouplist, '\\');
	if (g)
		g++;
	else
		g = opts->grouplist;

	SetMirandaStr(opts->ICQ.hostname, hkey, "hostname");
	SetMirandaInt(opts->ICQ.proxyuse, hkey, "ICQ_proxyuse");
	SetMirandaStr(opts->ICQ.proxyhost, hkey, "ICQ_proxyhost");
	SetMirandaInt(opts->ICQ.proxyport, hkey, "ICQ_proxyport");
	SetMirandaInt(opts->ICQ.proxyauth, hkey, "ICQ_proxyauth");
	SetMirandaStr(opts->ICQ.proxyname, hkey, "ICQ_proxyname");
	SetMirandaStr(opts->ICQ.proxypass, hkey, "ICQ_proxypass");



	SetMirandaStr(c, hkey, "contactlist");
	SetMirandaStr(h, hkey, "history");
	SetMirandaStr(g, hkey, "group");

	
	//MSN settings	
	SetMirandaInt(opts->MSN.enabled,hkey,"MSN_Enabled");
	SetMirandaStr(opts->MSN.uhandle,hkey,"MSN_uhandle");
	Encrypt(opts->MSN.password,TRUE);
	SetMirandaStr(opts->MSN.password,hkey,"MSN_passwordenc");
	Encrypt(opts->MSN.password,FALSE);
	
	//docking
	SetMirandaInt(opts->dockstate, hkey, "dockstate");

	if (!rto.useini)
	{
		RegCloseKey(hkey);
		RegCloseKey(hkeyapp);
	}
}

void GetMirandaInt(int *val, HKEY hkey, char *name, int def)
{	
	UINT ret;
	if (rto.useini)
	{
		
		ret=GetPrivateProfileInt("Miranda ICQ", name, def, tmpfn);
		def=ret;
		*val=def;
	}
	else
	{
		long sz;
		int res;

		if (hkey)
		{
			sz = sizeof(long);
			res = RegQueryValueEx(hkey, name, 0, NULL, (BYTE *)val, &sz);
		}
		if (hkey == NULL || res != ERROR_SUCCESS)
		{
			*val = def;
		}

	}
}

void GetMirandaStr(char *val, long sz, HKEY hkey, char *name, char *def)
{
	if (rto.useini)
	{

		GetPrivateProfileString("Miranda ICQ", name, def, val, 128, tmpfn);
	}
	else
	{
		int res;

		if (hkey)
		{
			res = RegQueryValueEx(hkey, name, 0, NULL, val, &sz);
		}
		if (hkey == NULL || res != ERROR_SUCCESS)
		{
			strcpy(val, def);
		}
	}
}

void SetMirandaInt(int val, HKEY hkey, char *name)
{
	if (rto.useini)
	{
		char buf[64];
		sprintf(buf, "%d", val);
		WritePrivateProfileString("Miranda ICQ", name, buf, tmpfn);
	}
	else
	{
		long sz;
		if (hkey)
		{
			sz = sizeof(long);
			RegSetValueEx(hkey, name, 0, REG_DWORD, (BYTE *)&val, sz);
		}
	}
}

void SetMirandaStr(char *val, HKEY hkey, char *name)
{
	if (rto.useini)
	{
		WritePrivateProfileString("Miranda ICQ", name, val, tmpfn);
	}
	else
	{
		long sz;
		if (hkey)
		{
			sz = strlen(val)+1;
			RegSetValueEx(hkey, name, 0, REG_SZ, val, sz);
		}
	}
}

HKEY GetAppRegKey(void)
{
	HKEY hkey;

	char regkey[128] = "Software\\FigBug Developments\\Miranda ICQ";

	if (RegCreateKey(HKEY_CURRENT_USER, regkey, &hkey) == ERROR_SUCCESS)
	{
		return hkey;
	}
	else
	{
		return NULL;
	}	
}

HKEY GetUserRegKey(char *postkey)
{
	HKEY hkey;

	char regkey[128] = "Software\\FigBug Developments\\Miranda ICQ\\";

	if (!strcmp(postkey, ""))
	{
		strcat(regkey, "default");
	}
	else
	{
		strncat(regkey, postkey, 12);
		strcat(regkey, "");
	}

	if (RegCreateKey(HKEY_CURRENT_USER, regkey, &hkey) == ERROR_SUCCESS)
	{
		return hkey;
	}
	else
	{
		return NULL;
	}	
}