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
MM_INTERFACE		mmi = {0};					// structure which keeps pointers to mirandas alloc, free and realloc
char *				IRCPROTONAME = NULL; 
char *				ALTIRCPROTONAME = NULL;
char *				pszServerFile = NULL;
char *				pszPerformFile = NULL;
char *				pszIgnoreFile = NULL;
char				mirandapath[MAX_PATH];
DWORD				mirVersion = NULL;
CRITICAL_SECTION	cs;
CRITICAL_SECTION	m_gchook;
PLUGINLINK *		pluginLink;
HINSTANCE			g_hInstance = NULL;	
PREFERENCES			* prefs;	

//static HMODULE	m_libeay32;
HMODULE				m_ssleay32 = NULL;
	

PLUGININFO			pluginInfo=
{						// Information about the plugin
						sizeof( PLUGININFO ),
						"IRC Protocol",
						PLUGIN_MAKE_VERSION( 0,6,3,7 ),
						"IRC protocol for Miranda IM.",
						"MatriX",
						"i_am_matrix@users.sourceforge.net",
						"© 2003 - 2005 Jörgen Persson",
						"http://members.chello.se/matrix/",
						0,	
						0
}; 



extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	g_hInstance=hinstDLL;

	return TRUE;
}



extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	mirVersion = mirandaVersion;
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
		char * p2;
		*p = '\0';
		p++;
		p1 = strrchr( p, '.' );
		*p1 = '\0';
		p2 = p;
		while( *p2 )
		{
			if(*p2 == ' ')
				*p2 = '_';
			p2++;
		}
		IRCPROTONAME = strdup( p );
		ALTIRCPROTONAME = new char[lstrlen( IRCPROTONAME ) + 7 ];
		CharUpper(IRCPROTONAME);

		if (lstrcmpi(IRCPROTONAME, "IRC"))
			mir_snprintf(ALTIRCPROTONAME, lstrlen( IRCPROTONAME ) + 7 , "IRC (%s)", IRCPROTONAME);
		else
			mir_snprintf(ALTIRCPROTONAME, lstrlen( IRCPROTONAME ) + 7 , "%s", IRCPROTONAME);
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

	if( !mirVersion || mirVersion<PLUGIN_MAKE_VERSION( 0, 4, 0 ,0 ) ) 
	{
		char szVersion[] = "0.4"; // minimum required version
		char szText[] = "The IRC protocol could not be loaded as it is dependant on Miranda IM version %s or later.\n\nDo you want to download an update from the Miranda website now?";
		char * szTemp = new char[lstrlen (szVersion) + lstrlen(szText) + 10];
		mir_snprintf(szTemp, lstrlen (szVersion) + lstrlen(szText) + 10, szText, szVersion);
		if(IDYES == MessageBoxA(0,Translate(szTemp),Translate("Information"),MB_YESNO|MB_ICONINFORMATION))
			CallService(MS_UTILS_OPENURL, 1, (LPARAM) "http://miranda-im.org/");
		delete[] szTemp;
		return 1;
	}

	GetModuleName();
	UpgradeCheck();
	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM) &mmi);
	
	InitializeCriticalSection(&cs);
	InitializeCriticalSection(&m_gchook);

//#ifdef IRC_SSL
	m_ssleay32 = LoadLibrary("ssleay32.dll");
//#endif

	monitor = new CMyMonitor;
	g_ircSession.AddIrcMonitor(monitor);

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
	DeleteCriticalSection(&m_gchook);

	if(m_ssleay32)
		FreeLibrary(m_ssleay32);

	UnhookEvents();
	UnInitOptions();
	free( IRCPROTONAME );
	delete [] ALTIRCPROTONAME;
	delete monitor;

	return 0;
}


void UpgradeCheck(void)
{
	DWORD dwVersion = DBGetContactSettingDword(NULL, IRCPROTONAME, "OldVersion", PLUGIN_MAKE_VERSION(0,6,0,0));
	if(	pluginInfo.version > dwVersion)
	{
		if(dwVersion < PLUGIN_MAKE_VERSION(0,6,1,0))
		{
			DBDeleteContactSetting(NULL, IRCPROTONAME,	"OnlineNotificationTime");
			DBDeleteContactSetting(NULL, IRCPROTONAME,	"AutoOnlineNotifTempAlso");
			
		}
		if(dwVersion < PLUGIN_MAKE_VERSION(0,6,3,7))
		{
			DBVARIANT dbv;
			char pw[600] = {0};
			if(!DBGetContactSetting(NULL, IRCPROTONAME, "Password", &dbv) && dbv.type==DBVT_ASCIIZ)
			{
				lstrcpyn(pw, dbv.pszVal, 599);
				DBFreeVariant(&dbv);
			}
			if(lstrlenA(pw) > 0)
			{
				CallService(MS_DB_CRYPT_ENCODESTRING, 499, (LPARAM)pw);
				DBWriteContactSettingString(NULL, IRCPROTONAME, "Password", pw);
				MessageBoxA(NULL, Translate("To increase security the saved password for your\n default network is now encrypted."), IRCPROTONAME, MB_OK|MB_ICONINFORMATION);			
			}
		}		
	}
	DBWriteContactSettingDword(NULL, IRCPROTONAME, "OldVersion", pluginInfo.version);
	return;
}







