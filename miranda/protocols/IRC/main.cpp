/*
IRC plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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

#include "irc.h"

CIrcSession			g_ircSession=NULL;			// Representation of the IRC-connection
CMyMonitor			*monitor;					// Object that processes data from the IRC server
char *				IRCPROTONAME = NULL; 
char *				ALTIRCPROTONAME = NULL;
char *				pszServerFile = NULL;
char *				pszPerformFile = NULL;
char *				pszIgnoreFile = NULL;
char				mirandapath[MAX_PATH];
CRITICAL_SECTION	cs;
PLUGINLINK *		pluginLink;
HINSTANCE			g_hInstance = NULL;	
PREFERENCES			* prefs;	
long				lWrongVersion;

//static HMODULE	m_libeay32;
HMODULE				m_ssleay32 = NULL;
	

PLUGININFO			pluginInfo=
{						// Information about the plugin
						sizeof( PLUGININFO ),
						"IRC Protocol",
						PLUGIN_MAKE_VERSION( 0,5,0,2 ),
						"IRC protocol for Miranda IM.",
						"MatriX ' m3x",
						"i_am_matrix@users.sourceforge.net",
						"© 2004 Jörgen Persson",
						"http://members.chello.se/matrix/",
						0,	
						0
}; 



BOOL APIENTRY DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	g_hInstance=hinstDLL;

	return TRUE;
}



extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if( mirandaVersion<PLUGIN_MAKE_VERSION( 0, 3, 4 ,3 ) ) 
	{
		if(IDYES == MessageBoxA(0,Translate("The IRC protocol could not be loaded as it is dependant on features in newer versions of Miranda IM.\n\nDo you want to download an update of Miranda IM now?."),Translate("Information"),MB_YESNO|MB_ICONINFORMATION))
			CallService(MS_UTILS_OPENURL, 1, (LPARAM) "http://miranda-im.org/");
		return NULL;
	}

	lWrongVersion = 20041010;

	return &pluginInfo;
}



static void GetModuleName( void )	 // ripped from msn
{
	char * p = NULL;
	char * p1 = NULL;

	GetModuleFileName(g_hInstance, mirandapath, MAX_PATH);
	p = strrchr( mirandapath, '\\' );
	if(p)
	{
		*p = '\0';
		p++;
		p1 = strrchr( p, '.' );
		*p1 = '\0';
		IRCPROTONAME = strdup( p );
		ALTIRCPROTONAME = new char[lstrlen( IRCPROTONAME ) + 7 ];
		CharUpper(IRCPROTONAME);

		if (lstrcmpi(IRCPROTONAME, "IRC"))
			_snprintf(ALTIRCPROTONAME, lstrlen( IRCPROTONAME ) + 7 , "IRC (%s)", IRCPROTONAME);
		else
			_snprintf(ALTIRCPROTONAME, lstrlen( IRCPROTONAME ) + 7 , "%s", IRCPROTONAME);
	}
}



static void RegisterProtocol( void )
{
	PROTOCOLDESCRIPTOR pd;
	ZeroMemory( &pd, sizeof( pd ) );
	pd.cbSize = sizeof( pd );
	pd.szName = IRCPROTONAME;
	pd.type = PROTOTYPE_PROTOCOL;
	CallService( MS_PROTO_REGISTERMODULE, 0, (LPARAM)&pd );
}



extern "C" int __declspec(dllexport) Load( PLUGINLINK *link )
{
	#ifndef NDEBUG //mem leak detector :-) Thanks Tornado!
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	_CrtSetDbgFlag(flag); // Set flag to the new value
	#endif

	pluginLink=link;

	if (ServiceExists(MS_SYSTEM_GETBUILDSTRING))
	{
// should be reenabled later when this service works
/*
		extern long lWrongVersion;
		char szTemp[40];
		CallService(MS_SYSTEM_GETBUILDSTRING, 39, (LPARAM)szTemp);
		char * stuff = strdup (szTemp);
		MessageBox(NULL, szTemp, "version", 0);
		long date = atoi(szTemp);
		if (date < lWrongVersion * 1000000)
*/
	}
	else
	{

		if(IDYES == MessageBoxA(0,Translate("The IRC protocol requires features found in newer versions of Miranda IM.\n\nDo you want to download it from the Miranda IM web site now?"),Translate("IRC Error"),MB_YESNO|MB_ICONERROR))
			CallService(MS_UTILS_OPENURL, 1, (LPARAM) "http://www.miranda-im.org/");
		return 1;
	}

	InitializeCriticalSection(&cs);

#ifdef IRC_SSL
	m_ssleay32 = LoadLibrary("ssleay32.dll");
#endif

	monitor = new CMyMonitor;
	g_ircSession.AddIrcMonitor(monitor);

	GetModuleName();
	RegisterProtocol();
	HookEvents();
	CreateServiceFunctions();
	InitPrefs();
	CList_SetAllOffline(true);

	return 0;
}



extern "C" int __declspec(dllexport) Unload(void)
{

	CList_SetAllOffline(TRUE);

	DeleteCriticalSection(&cs);

	if(m_ssleay32)
		FreeLibrary(m_ssleay32);

	UnhookEvents();
	UnInitOptions();
	delete [] IRCPROTONAME;
	delete [] ALTIRCPROTONAME;
	delete monitor;

	return 0;
}



extern "C" int __declspec(dllexport) UninstallEx(PLUGINUNINSTALLPARAMS* ppup)
{
	if (ppup && ppup->bDoDeleteSettings) 
	{
		char path[MAX_PATH];
		char * dllname;
		char * fend;
		
		GetModuleFileName( g_hInstance, path, MAX_PATH);
		dllname = strrchr(path,'\\');
		dllname++;
		fend = strrchr(path,'.');
		if(fend)
			*fend = '\0';

		if (ppup->bIsMirandaRunning )
		{ 
			HANDLE hContact;
			char *szProto;

			hContact = (HANDLE) PUICallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
			while (hContact) 
			{
				szProto = (char *) PUICallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
				if (szProto != NULL && !lstrcmpi(szProto, dllname)) 
				{
					DBCONTACTWRITESETTING cws;
					cws.szModule="CList";
					cws.szSetting="NotOnList";
					cws.value.type=DBVT_BYTE;
					cws.value.bVal=1;
					PUICallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)hContact,(LPARAM)&cws);
				}
				hContact = (HANDLE) PUICallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
			}
			PUIRemoveDbModule(dllname);
		}
		char szFileName[MAX_PATH]; 
		_snprintf(szFileName, sizeof(szFileName), "%s%s_servers.ini", ppup->pszPluginsPath, dllname);
		DeleteFile(szFileName); 
		_snprintf(szFileName, sizeof(szFileName), "%s%s_ignore.ini",  ppup->pszPluginsPath, dllname);
		DeleteFile(szFileName); 
		_snprintf(szFileName, sizeof(szFileName), "%s%s_perform.ini",  ppup->pszPluginsPath, dllname);
		DeleteFile(szFileName); 
		_snprintf(szFileName, sizeof(szFileName), "%sIRC_license.txt",  ppup->pszPluginsPath);
		DeleteFile(szFileName); 
		_snprintf(szFileName, sizeof(szFileName), "%sIRC_Readme.txt",  ppup->pszPluginsPath);
		DeleteFile(szFileName); 
		_snprintf(szFileName, sizeof(szFileName), "%sIRC_Translate.txt",  ppup->pszPluginsPath);
		DeleteFile(szFileName); 
	}
	return 0; 
}











