// Logger Plugin.cpp : Defines the entry point for the DLL application.
//


#include "Logger Plugin.h"
#include <windows.h>
#include <stdio.h>
#include "..\..\miranda32\global.h"
#include "..\..\miranda32\pluginapi.h"
#include "resource.h"

char szTitle[MAX_PLUG_TITLE];
HWND mirhwnd;
HWND meloghwnd;
HINSTANCE mehinst;
BOOL pauselog=FALSE;

BOOL CALLBACK DlgProcLog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			mehinst=(HINSTANCE)hModule;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}



MIRANDAPLUGIN_API int __stdcall Load(HWND hwnd,HINSTANCE hinst,char *title)
{
	HMENU hmsys;
	mirhwnd=hwnd;

	strcpy(szTitle,"Logger\tCtrl+Shift+L");
	strcpy(title,szTitle);

	meloghwnd = CreateDialogParam(mehinst, MAKEINTRESOURCE(IDD_LOG), NULL, DlgProcLog,NULL);	
	hmsys=GetSystemMenu(meloghwnd,FALSE);
	AppendMenu(hmsys,MF_SEPARATOR,0,"brk");
	AppendMenu(hmsys,MF_STRING,999,"Clear Log");
	AppendMenu(hmsys,MF_STRING,998,"Pause Logging");

	RegisterHotKey(meloghwnd,0,MOD_CONTROL|MOD_SHIFT,'L');

	return TRUE; //return false if we failed and want to be unloaded
}

MIRANDAPLUGIN_API int __stdcall Unload(void)
{
	//EndDialog(meloghwnd, 0);
	DestroyWindow(meloghwnd);
	UnregisterHotKey(meloghwnd,0);
	return TRUE; //dont really matter, but better leave it t otrue
}

MIRANDAPLUGIN_API int __stdcall Notify(long msg,WPARAM wparam,LPARAM lparam)
{
	HWND itemhwnd;
	char *curtxt;
	long len;

	switch(msg)
	{
	case PM_STATUSCHANGE: //my status change

		break;
	case PM_CONTACTSTATUSCHANGE: //someone else changed
	
		break;
	case PM_SHOWOPTIONS:
		//Show 
		ShowWindow(meloghwnd, SW_SHOW);
		
		break;
	case PM_ICQDEBUGMSG:
		//ADD MSG	
		if (pauselog){break;}

		itemhwnd=GetDlgItem(meloghwnd,IDC_LOG);

		len=GetWindowTextLength(itemhwnd);
		curtxt=(char*)malloc(strlen((const char*)lparam)+3);
		curtxt[0]=0;
		strcpy(curtxt,"\r\n");
		strcat(curtxt,(const char*)lparam);
		curtxt[strlen(curtxt)-1]=0; //remove trailing \n
		
		if (len>=30000) //log is getting big, reset it
		{
			char t[20];			
			strcpy(t,"Log Reset\r\n");
			SendMessage(itemhwnd,WM_SETTEXT,0,(long)&t);
		}

		SendMessage(itemhwnd,EM_SETSEL,len,len);
		SendMessage(itemhwnd,EM_REPLACESEL,FALSE,(LPARAM)curtxt);
		
		free(curtxt);
		
		/*
		curtxt=(char*)malloc(len + strlen((const char*)lparam)+3);
		curtxt[0]=0;
		GetWindowText(itemhwnd,curtxt,len);
		
		if (curtxt[0]!=0)
			strcat(curtxt,"\r\n");


		strcat(curtxt,(const char*)lparam);
		
		
		SendMessage(itemhwnd,WM_SETTEXT,0,(long)curtxt);
		free(curtxt);*/
		break;
	}

	return 0;

}

BOOL CALLBACK DlgProcLog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND itemhwnd;
	switch (msg)
	{
	case WM_INITDIALOG:
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(mehinst, MAKEINTRESOURCE(IDI_ICO)));

		return TRUE;			
	case WM_SYSCOMMAND:
		if (wParam==999)
		{//CLEAR TXT
			itemhwnd=GetDlgItem(hwndDlg,IDC_LOG);
			SetWindowText(itemhwnd,NULL);
		}
		else if (wParam==998)
		{//PAUSE LOGGIN
			if (pauselog)
			{
				pauselog=FALSE;
				CheckMenuItem(GetSystemMenu(hwndDlg,FALSE),9,MF_BYPOSITION|MF_UNCHECKED);//MF_BYCOMMAND|MF_UNCHECKED);
			}
			else
			{
				pauselog=TRUE;
				CheckMenuItem(GetSystemMenu(hwndDlg,FALSE),9,MF_BYPOSITION|MF_CHECKED);//MF_BYCOMMAND|MF_UNCHECKED);
			}
			
			
		}
		break;
	case WM_SIZE:
		RECT rt;
		GetClientRect(hwndDlg,&rt);
		//SendDlgItemMessage(hwndDlg,IDC_LOG,WM_SIZE,wParam,lParam);
		itemhwnd=GetDlgItem(hwndDlg,IDC_LOG);
		MoveWindow(itemhwnd,0,0,rt.right,rt.bottom,true);
		break;
	/*case WM_COMMAND:
		switch ((int)wParam)
		{
			case IDOK:
				EndDialog(hwndDlg, 0);
				return TRUE;
		}*/
	case WM_CLOSE:
		ShowWindow(hwndDlg,SW_HIDE);
		break;
	case WM_HOTKEY:
		//assume its only L
		if (IsWindowVisible(hwndDlg))
		{
			if (GetForegroundWindow()==hwndDlg)
				ShowWindow(hwndDlg,SW_HIDE);
			else
				SetForegroundWindow(hwndDlg);

		}
		else
		{
			ShowWindow(hwndDlg,SW_SHOW);
			SetForegroundWindow(hwndDlg);
		}
		break;
	}
	return 0;
}