/*
Miranda IM Help Plugin
Copyright (C) 2002 Richard Hughes

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
#include <tchar.h>
#include <windows.h>
#include <newpluginapi.h>
#include <m_langpack.h>
#include "help.h"

#include "resource.h"

extern HINSTANCE hInst;
static HHOOK hMessageHook;
extern HWND hwndHelpDlg;

struct FindChildAtPointData {
	HWND hwnd;
	POINT pt;
	int bestArea;
};

BOOL CALLBACK FindChildAtPointEnumProc(HWND hwnd,LPARAM lParam)
{
	struct FindChildAtPointData *fcap=(struct FindChildAtPointData*)lParam;
	if(IsWindowVisible(hwnd)) {
		RECT rcVisible,rc,rcParent;
		GetWindowRect(hwnd,&rc);
		GetWindowRect(GetParent(hwnd),&rcParent);
		IntersectRect(&rcVisible,&rcParent,&rc);
		if(PtInRect(&rcVisible,fcap->pt)) {
			int thisArea=(rc.bottom-rc.top)*(rc.right-rc.left);
			if(thisArea && (thisArea<fcap->bestArea || fcap->bestArea==0)) {
				fcap->bestArea=thisArea;
				fcap->hwnd=hwnd;
			}
		}
	}
	return TRUE;
}

struct DlgBoxSubclassData {
	HWND hwndDlg;
	WNDPROC pfnOldWndProc;
} static *dlgBoxSubclass=NULL;
static int dlgBoxSubclassCount=0;

static LRESULT CALLBACK DialogBoxSubclassProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	struct DlgBoxSubclassData *dat;
	int i;

	for(i=0;i<dlgBoxSubclassCount;i++)
		if(dlgBoxSubclass[i].hwndDlg==hwndDlg) break;
	if(i==dlgBoxSubclassCount) return 0;
	dat=&dlgBoxSubclass[i];
	switch(message) {
		case WM_CONTEXTMENU:
		{	HMENU hMenu;
			POINT pt;
			HWND hwndCtl;
			struct FindChildAtPointData fcap={0};
			int type;

			GetCursorPos(&pt);
			//ChildWindowFromPoint() messes up with group boxes
			fcap.hwnd=NULL;
			fcap.pt=pt;
			EnumChildWindows(hwndDlg,FindChildAtPointEnumProc,(LPARAM)&fcap);
			hwndCtl=fcap.hwnd;
			if(hwndCtl==NULL) {
				ScreenToClient(hwndDlg,&pt);
				hwndCtl=ChildWindowFromPointEx(hwndDlg,pt,CWP_SKIPINVISIBLE);
				if(hwndCtl==NULL) break;
				GetCursorPos(&pt);
			}
			type=GetControlType(hwndCtl);
			if(type==CTLTYPE_LISTVIEW || type==CTLTYPE_TREEVIEW || type==CTLTYPE_DATETIME || type==CTLTYPE_STATUSBAR || type==CTLTYPE_HYPERLINK || type==CTLTYPE_CLC)
				break;
			hMenu=CreatePopupMenu();
			AppendMenu(hMenu,MF_STRING,1,TranslateTS(hwndCtl==hwndDlg ? _T("&What's This Dialog?") : _T("&What's This?")));
			switch(TrackPopupMenu(hMenu,TPM_RIGHTBUTTON|TPM_RETURNCMD,pt.x,pt.y,0,hwndDlg,NULL)) {
				case 1:
					if(!IsWindow(hwndHelpDlg))
						hwndHelpDlg=CreateDialog(hInst,MAKEINTRESOURCE(IDD_HELP),NULL,HelpDlgProc);
					SendMessage(hwndHelpDlg,M_CHANGEHELPCONTROL,0,(LPARAM)hwndCtl);
					break;
			}
			DestroyMenu(hMenu);
			return 0;
		}
		case WM_HELP:
		{	HELPINFO *hi=(HELPINFO*)lParam;
			if(hi->iContextType!=HELPINFO_WINDOW) break;
			if(!IsWindow(hwndHelpDlg))
				hwndHelpDlg=CreateDialog(hInst,MAKEINTRESOURCE(IDD_HELP),NULL,HelpDlgProc);
			SendMessage(hwndHelpDlg,M_CHANGEHELPCONTROL,0,(LPARAM)hi->hItemHandle);
			return 0;
		}
		case WM_DESTROY:
		{	WNDPROC pfnWndProc=dat->pfnOldWndProc;
			SetWindowLong(hwndDlg,GWL_WNDPROC,(LONG)dat->pfnOldWndProc);
			MoveMemory(dlgBoxSubclass+i,dlgBoxSubclass+i+1,sizeof(struct DlgBoxSubclassData)*(dlgBoxSubclassCount-i-1));
			dlgBoxSubclassCount--;
			dlgBoxSubclass=(struct DlgBoxSubclassData*)realloc(dlgBoxSubclass,sizeof(struct DlgBoxSubclassData)*dlgBoxSubclassCount);
			return CallWindowProc(pfnWndProc,hwndDlg,message,wParam,lParam);
		}
	}
	return CallWindowProc(dat->pfnOldWndProc,hwndDlg,message,wParam,lParam);
}

static LRESULT CALLBACK HelpSendMessageHookProc(int code,WPARAM wParam,LPARAM lParam)
{
	CWPSTRUCT *msg=(CWPSTRUCT*)lParam;
	if(code>=0) {
		switch(msg->message) {
			case WM_INITDIALOG:
				{	TCHAR szClassName[32];
					GetClassName(msg->hwnd,szClassName,sizeof(szClassName));
					if(lstrcmp(szClassName,_T("#32770"))) break;
				}
				dlgBoxSubclass=(struct DlgBoxSubclassData*)realloc(dlgBoxSubclass,sizeof(struct DlgBoxSubclassData)*(dlgBoxSubclassCount+1));
				dlgBoxSubclass[dlgBoxSubclassCount].hwndDlg=msg->hwnd;
				dlgBoxSubclass[dlgBoxSubclassCount].pfnOldWndProc=(WNDPROC)SetWindowLong(msg->hwnd,GWL_WNDPROC,(LONG)DialogBoxSubclassProc);
				dlgBoxSubclassCount++;
				{	DWORD style=GetWindowLong(msg->hwnd,GWL_STYLE);
					DWORD exStyle=GetWindowLong(msg->hwnd,GWL_EXSTYLE);
					if(!(style&(WS_MINIMIZEBOX|WS_MAXIMIZEBOX)))
						SetWindowLong(msg->hwnd,GWL_EXSTYLE,exStyle|WS_EX_CONTEXTHELP);
				}
				break;
		}
	}
	return CallNextHookEx(hMessageHook,code,wParam,lParam);
}

int InstallDialogBoxHook(void)
{
	hMessageHook=SetWindowsHookEx(WH_CALLWNDPROC,(HOOKPROC)HelpSendMessageHookProc,NULL,GetCurrentThreadId());
	return hMessageHook==NULL;
}

int RemoveDialogBoxHook(void)
{
	int i;

	UnhookWindowsHookEx(hMessageHook);
	for(i=0;i<dlgBoxSubclassCount;i++)
		SetWindowLong(dlgBoxSubclass[i].hwndDlg,GWL_WNDPROC,(LONG)dlgBoxSubclass[i].pfnOldWndProc);
	if(dlgBoxSubclass) free(dlgBoxSubclass);
	return 0;
}
