// TestPlug.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <stdio.h>
#include "testplug.h"
#include "..\..\miranda32\global.h"
#include "..\..\miranda32\pluginapi.h"

void TellInfoDLL(char *msg);


//RIPPED FROM ICQLIB..who cares, cheap dll ne way
#define STATUS_OFFLINE     (-1L)
#define STATUS_ONLINE      0x0000L
#define STATUS_AWAY        0x0001L
#define STATUS_DND         0x0002L /* 0x13L */
#define STATUS_NA          0x0004L /* 0x05L */
#define STATUS_OCCUPIED    0x0010L /* 0x11L */
#define STATUS_FREE_CHAT   0x0020L
#define STATUS_INVISIBLE   0x0100L

char szTitle[MAX_PLUG_TITLE];
HWND mirhwnd;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}


MIRANDAPLUGIN_API int __stdcall Load(HWND hwnd,HINSTANCE hinst,char *title)
{
	strcpy(szTitle,"Info Plugin");
	strcpy(title,szTitle);
	mirhwnd=hwnd;
	return TRUE; //return false if we failed and want to be unloaded
}

MIRANDAPLUGIN_API int __stdcall Unload(void)
{
	return TRUE; //dont really matter, but better leave it t otrue
}

MIRANDAPLUGIN_API int __stdcall Notify(long msg,WPARAM wparam,LPARAM lparam)
{
	CONTACT *c;
	switch(msg)
	{
	case PM_STATUSCHANGE: //my status change

		break;
	case PM_CONTACTSTATUSCHANGE: //someone else changed
			c=(CONTACT*)lparam;
			//OutputDebugString(c->nick);
			char msg[50];
			char strstat[15];
			if (wparam==c->status){return 0;} //if they havent changed then ignore it
			
			switch( c->status & 0xFFFF)
			{
			case STATUS_ONLINE:
				strcpy(strstat,"Online");
				break;
			case 0xFFFF: //fall thru
			case STATUS_OFFLINE:
				strcpy(strstat,"Offline");
				break;
			default:
				return 0;
			}
			sprintf(msg,"ICQ:%s is %s\0x0",c->nick,strstat);
			TellInfoDLL (msg);
		break;
		case PM_SHOWOPTIONS:
				MessageBox(mirhwnd,"No Options",szTitle,MB_OK);
				break;
		case PM_ICQDEBUGMSG:

			break;
	}

	return 0;

}

void TellInfoDLL(char *msg)
{
	HWND TMP;
	HWND HWNDTO;
	TMP = FindWindow("Shell_TrayWnd", "");
	TMP = FindWindowEx(TMP, 0, "ReBarWindow32", "");
	TMP = FindWindowEx(TMP, 0, "MSTaskSwWClass", "");
	TMP = FindWindowEx(TMP, 0, "MINITASKBAR", "Mini Taskbar");
	HWNDTO = FindWindowEx(TMP, 0, "INFOPLUGIN", "Info Plugin");
	if (HWNDTO==NULL)
	{ //info dll not running
		return;
	}
	COPYDATASTRUCT cd;
	cd.lpData=msg;
	cd.cbData=strlen(msg);
	cd.dwData=0;
	SendMessage(HWNDTO,WM_COPYDATA,0,(LPARAM)&cd);

}