// P_StatusNotify.cpp : Defines the entry point for the DLL application.
//


#include "stdafx.h"
#include <winsock.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <malloc.h>

#include "..\..\miranda32\miranda.h"
#include "..\..\miranda32\msgque.h"
#include "..\..\miranda32\pluginapi.h"
#include "resource.h"

#define TITLE "Status Notify"
#define CLASSNAME "MIRANDA_STATUSNOTIFY"

BOOL multiwnd=TRUE;//more than one wnd at a time?
int maxwnd=6;//how many wnds at one time
HWND slot[6];

PI_CALLBACK			picb;

BOOL enabled=TRUE; //inuse?
BOOL enabledndocc=FALSE;
BOOL enablena=FALSE;

HWND tmplastact;

int ver=6; //is miranda v0061 or hihger?
char gmsg[200];
int wndcnt=0;

//RECT mwnd;

HWND parhwnd;
HINSTANCE parhinst;
HWND ghwnd;
HINSTANCE ghinst;
time_t onlinetime;

long gmystatus=STATUS_OFFLINE;

BOOL CALLBACK DlgProcStatus(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void ShowStatus(char *msg);

char *statToString(int status); //taken from Talk Plugin

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			ghinst=(HINSTANCE)hModule;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

MIRANDAPLUGIN_API int __stdcall Load(HWND hwnd, HINSTANCE hinst, char *title)
{	
	
	strcpy(title, TITLE);

	parhwnd=hwnd;
	parhinst=hinst;

	//REG class
	/*WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= ghinst;
	wcex.hIcon			= NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_MEMORY);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;//(LPCSTR)IDC_MEMORY;
	wcex.lpszClassName	= CLASSNAME;
	wcex.hIconSm		= NULL;//LoadIcon(wcex.hInstance, (LPCTSTR)IDI_MEMORY);
	
	if (!RegisterClassEx(&wcex))
	{
		MessageBox(parhwnd,"Register class failed",TITLE,MB_OK);
		return FALSE;
	}
	//make wnd

	ghwnd=CreateWindowEx(WS_EX_TOPMOST,CLASSNAME,TITLE,WS_POPUP,0,0,100,50,NULL,NULL,ghinst,NULL);
	 //  ghwnd = CreateWindow(CLASSNAME, TITLE, WS_POPUP,
      // 0, 0, 5, 5, NULL, NULL, ghinst, NULL);

 	if (!ghwnd)
	{
		DWORD err;
		err=GetLastError();
		MessageBox(parhwnd,"CreateWindow Failed",TITLE,MB_OK);
		return FALSE;
	}
	*/
	if (!multiwnd)
		ghwnd = CreateDialogParam(ghinst, MAKEINTRESOURCE(IDD_STATUS), NULL, DlgProcStatus,NULL);	
		
	//ShowWindow(ghwnd,SW_SHOW);
	return TRUE;
}

MIRANDAPLUGIN_API int __stdcall Unload(void)
{
		
	return TRUE;
}

MIRANDAPLUGIN_API int __stdcall Notify(long msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case PM_VERSION:
			//char *verstr;
			//verstr=(char*)lParam;

			ver=7;//its higher than 0061 (which atm could only b 007)
		break;
		case PM_SHOWOPTIONS:
			
			DialogBox(ghinst, MAKEINTRESOURCE(IDD_OPTIONS), ghwnd,(DLGPROC) DlgProcOptions);
			break;
		case PM_STATUSCHANGE:
			if (gmystatus==STATUS_OFFLINE || wParam!=STATUS_OFFLINE)
			{time(&onlinetime);}//we were offline, now online

			gmystatus=wParam;
			
			break;
		case PM_CONTACTSTATUSCHANGE:
			{
				CONTACT *c;
				char strstat[15];
				if (ver==6)
				{//its version 006 of miranda, using old Contact struct
					CONTACT_OLD *co;
					co=(CONTACT_OLD*)lParam;
					c=(CONTACT*)malloc(sizeof(CONTACT));
					c->uin=co->uin;
					strcpy(c->nick,co->nick);
					c->status=co->status;
				}
				else
				{//v007 +
					c=(CONTACT*)lParam;
				}
				time_t curtime;
				
				if (c->status==wParam)
				{
					return 0;//havent changed
				}
				if (!enabled){return 0;}
				if (gmystatus==STATUS_DND || gmystatus==STATUS_OCCUPIED)
				{
					if (!enabledndocc)
						return 0;
				}
				if (gmystatus==STATUS_OFFLINE){return 0;}
				if (gmystatus==STATUS_NA)
				{
					if (!enablena)
						return 0;
				}

				time(&curtime);
				if ((curtime-onlinetime)<=10) {return 0;} //if its within 10 seconds of being online then dont bother

				
				strcpy(strstat,statToString(c->status));
				
				//if (c->status==STATUS_ONLINE)
				//{//came on online (from offline)
				char msg[50];
				if (c->nick[0]==0)
				{//no nick
					sprintf(msg,"%d is %s",c->uin,strstat);
				}
				else
				{
					sprintf(msg,"%s is %s",c->nick ,strstat);
				}
				ShowStatus(msg);
				//}

				if (ver==6)
				{
					free(c);
				}
			}
			break;
		case PM_GOTMESSAGE:
			break;
		case PM_GOTURL:
			break;
		case PM_ICQDEBUGMSG:
			//ShowStatus((char*)lParam);
			//ShowStatus("Tristan is Online(NA)");
			break;
		case PM_REGCALLBACK:
			memcpy(&picb, (void*)wParam, sizeof(picb));
			break;
		case PM_START:
			picb.pGetPluginInt("StatusNotify", (int*)&enabled,  "enable",  TRUE);
			picb.pGetPluginInt("StatusNotify", (int*)&enabledndocc,  "enable_dndocc",  FALSE);
			picb.pGetPluginInt("StatusNotify", (int*)&enablena,  "enable_na",  FALSE);

			break;
		case PM_SAVENOW:
			picb.pSetPluginInt("StatusNotify", (int)enabled,  "enable");
			picb.pSetPluginInt("StatusNotify", (int)enabledndocc,  "enable_dndocc");
			picb.pSetPluginInt("StatusNotify", (int)enablena,  "enable_na");
			break;
		case PM_PROPSHEET:
			{
				PROPSHEETPAGE *t;
				int *cnt;
				cnt=(int*)wParam;
				
				t=(PROPSHEETPAGE*)lParam;
				t[*cnt].dwSize = sizeof(PROPSHEETPAGE);
				t[*cnt].dwFlags = PSP_DEFAULT;
				t[*cnt].hInstance = ghinst;
				t[*cnt].pszTitle = "Status Notify Options";
				t[*cnt].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS);
				t[*cnt].pfnDlgProc = DlgProcOptions;
				t[*cnt].lParam=TRUE;
				(*cnt)++;
				
				
				
				
			}
		break;
	}
	return FALSE;
}

void ShowStatus(char *msg)
{
	int i;
	RECT mwnd;
	HWND curhwnd;

	tmplastact=GetForegroundWindow();		


	if (wndcnt>maxwnd){return;}

	if (IsWindowVisible(ghwnd) && !multiwnd)
	{

	}
	else
	{
		if (multiwnd)
		{//make a new wnd
			
			for (i=0;i<maxwnd;i++)
			{
				if (slot[i]==NULL)
				{
					curhwnd = CreateDialogParam(ghinst, MAKEINTRESOURCE(IDD_STATUS), NULL, DlgProcStatus,i);
					
					slot[i]=curhwnd;
					goto cont;
				}

			}
			//no free slots
			return;
		}
		else
		{
			curhwnd=ghwnd;
		}
cont:
		//wndcnt++;

		GetClientRect(curhwnd,&mwnd);
		
		mwnd.left=GetSystemMetrics(SM_CXSCREEN)-60-((i+1)*mwnd.right);
		
	
		mwnd.top=-mwnd.bottom;
		
		
		strcpy(gmsg,msg);

		SetDlgItemText(curhwnd, IDC_MSG, gmsg);

		
		SetWindowPos(curhwnd,HWND_TOPMOST,mwnd.left,mwnd.top,mwnd.right,mwnd.bottom,SWP_NOACTIVATE);
		ShowWindow(curhwnd,SW_SHOWNOACTIVATE);
		//hack to try and stop the wnd from stealing the foreground
		if (GetForegroundWindow()==curhwnd && curhwnd!=tmplastact)
		{
			SetForegroundWindow(tmplastact);
		}

		SetTimer(curhwnd,1,100,NULL);
		
	}
	


}

BOOL CALLBACK DlgProcStatus(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT curwnd;
	
	switch (msg)
	{
	
	case WM_LBUTTONUP:
		goto closewnd;
		break;
	case WM_TIMER:
		if ((int)wParam==1)
		{//scroll in timer
			GetWindowRect(hwndDlg,&curwnd);
			curwnd.bottom=curwnd.bottom-curwnd.top;
			curwnd.right=curwnd.right-curwnd.left;


			curwnd.top=curwnd.top+5;
			
			if (curwnd.top>=0)
			{
				curwnd.top=0;
				KillTimer(hwndDlg,1);
				SetTimer(hwndDlg,2,5000,NULL);
			}
			MoveWindow(hwndDlg,curwnd.left,curwnd.top,curwnd.right,curwnd.bottom,TRUE);

			
		}
		else if ((int)wParam==2)
		{
closewnd:
			KillTimer(hwndDlg,2);
			SetTimer(hwndDlg,3,100,NULL);
			
		}
		else if ((int)wParam==3)
		{
			GetWindowRect(hwndDlg,&curwnd);
			curwnd.bottom=curwnd.bottom-curwnd.top;
			curwnd.right=curwnd.right-curwnd.left;


			curwnd.top=curwnd.top-5;

			MoveWindow(hwndDlg,curwnd.left,curwnd.top,curwnd.right,curwnd.bottom,TRUE);

			if (curwnd.top<=-curwnd.bottom)
			{
				KillTimer(hwndDlg,3);
				
				ShowWindow(hwndDlg,SW_HIDE);
				
				if (multiwnd)
				{//kill this wnd
					int id;
					for (id=0;id<maxwnd;id++)
					{
						if (slot[id]==hwndDlg)
						{
							slot[id]=NULL;
							break;
						}
					}
					
					DestroyWindow(hwndDlg);
				}
			}

		}
		break;
	/*case WM_PAINT:
		PAINTSTRUCT ps;
		
		RECT r;
		HBRUSH forec;
		BeginPaint(hwndDlg,&ps);
		hdc=GetDC(hwndDlg);

		GetClientRect(hwndDlg,&r);

		FillRect(hdc,&r,CreateSolidBrush(0));

		forec=CreateSolidBrush(0xffffff);
		SelectObject(hdc,forec);
		//DrawText(hdc,gmsg,strlen(gmsg),&r,DT_LEFT);
		TextOut(hdc,0,0,gmsg,strlen(gmsg));


		DeleteObject(forec);
		ReleaseDC(hwndDlg,hdc);
		EndPaint(hwndDlg,&ps);
		break;*/
	case WM_INITDIALOG:
		//SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(mehinst, MAKEINTRESOURCE(IDI_ICO)));
		
		hdc=GetDC(hwndDlg);
		SetBkColor(hdc,0);
		ReleaseDC(hwndDlg,hdc);
		return TRUE;			

	case WM_CLOSE:
		ShowWindow(hwndDlg,SW_HIDE);
		break;
	}
	return 0;
}


BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	
	
	switch (msg)
	{
		case WM_INITDIALOG:
			
			SendDlgItemMessage(hwndDlg, IDC_ENABLED, BM_SETCHECK, (enabled) ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, IDC_ENABLEDNDOCC, BM_SETCHECK, (enabledndocc) ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, IDC_ENABLENA, BM_SETCHECK, (enablena) ? BST_CHECKED : BST_UNCHECKED, 0);
			break;
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
					enabled=SendDlgItemMessage(hwndDlg, IDC_ENABLED, BM_GETCHECK,0,0);
					enabledndocc=SendDlgItemMessage(hwndDlg, IDC_ENABLEDNDOCC, BM_GETCHECK,0,0);
					enablena=SendDlgItemMessage(hwndDlg, IDC_ENABLENA, BM_GETCHECK,0,0);
					//drop thru
				case IDCANCEL:
					EndDialog(hwndDlg, 0);
					return TRUE;
			}
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				
				case PSN_APPLY:
					
					enabled=SendDlgItemMessage(hwndDlg, IDC_ENABLED, BM_GETCHECK,0,0);
					enabledndocc=SendDlgItemMessage(hwndDlg, IDC_ENABLEDNDOCC, BM_GETCHECK,0,0);
					enablena=SendDlgItemMessage(hwndDlg, IDC_ENABLENA, BM_GETCHECK,0,0);

					return TRUE;
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
				case PSN_SETACTIVE:
					{
						HWND thwnd;
						thwnd=GetDlgItem(hwndDlg,IDOK);
						
						ShowWindow(thwnd,SW_HIDE);

						thwnd=GetDlgItem(hwndDlg,IDCANCEL);
						ShowWindow(thwnd,SW_HIDE);
					}
				break;
			}
			break;

	}
	return 0;
}
//ripped from talk plugin
char *statToString(int status)
{
	if (status == 0x0000) return "online";
	if (status == 0x0001) return "away";
	if (status == 0x0002) return "DND";
	if (status == 0x0013) return "DND";
	if (status == 0x0004) return "NA";
	if (status == 0x0005) return "NA";
	if (status == 0x0010) return "occupied";
	if (status == 0x0011) return "occupied";
	if (status == 0x0020) return "free for chat";
	if (status == 0x0100) return "invisible";

	return "offline";
}

