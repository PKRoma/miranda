/*
Change ICQ Details plugin for Miranda IM

Copyright © 2001-2005 Richard Hughes, Martin Öberg

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



#include <windows.h>
#include <commctrl.h>
#include <newpluginapi.h>
#include <m_database.h>
#include <m_langpack.h>
#include "changeinfo.h"
#include "resource.h"

#ifndef AW_SLIDE
#define SPI_GETCOMBOBOXANIMATION 0x1004
#define AW_SLIDE			0x40000
#define AW_ACTIVATE			0x20000
#define AW_VER_POSITIVE		0x4
#define UDM_SETPOS32            (WM_USER+113)
#define UDM_GETPOS32            (WM_USER+114)
#endif

static HWND hwndListEdit=NULL;
extern HWND hwndList;
extern HFONT hListFont;
extern HINSTANCE hInst;
extern int iEditItem;
static BOOL (WINAPI *MyAnimateWindow)(HWND,DWORD,DWORD);
static WNDPROC OldListEditProc;

static LRESULT CALLBACK ListEditSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) {
		case WM_LBUTTONUP:
			CallWindowProc(OldListEditProc,hwnd,msg,wParam,lParam);
			{	POINT pt;
				pt.x=(short)LOWORD(lParam);
				pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwnd,&pt);
				if(SendMessage(hwnd,WM_NCHITTEST,0,MAKELPARAM(pt.x,pt.y))==HTVSCROLL) break;
			}
			{	int i;
				i=SendMessage(hwnd,LB_GETCURSEL,0,0);
				EndListEdit(i!=LB_ERR);
			}
			return 0;
		case WM_CHAR:
			if(wParam!='\r') break;
			{	int i;
				i=SendMessage(hwnd,LB_GETCURSEL,0,0);
				EndListEdit(i!=LB_ERR);
			}
			return 0;
		case WM_KILLFOCUS:
			EndListEdit(1);
			return 0;
	}
	return CallWindowProc(OldListEditProc,hwnd,msg,wParam,lParam);
}

void BeginListEdit(int iItem,RECT *rc,int i,WORD wVKey)
{
	int j,n;
	POINT pt;
	int itemHeight;

	EndListEdit(0);
	pt.x=pt.y=0;
	ClientToScreen(hwndList,&pt);
	OffsetRect(rc,pt.x,pt.y);
	InflateRect(rc,-2,-2);
	rc->left-=2;
	iEditItem=iItem;
	ListView_RedrawItems(hwndList,iEditItem,iEditItem);
	UpdateWindow(hwndList);

	hwndListEdit=CreateWindowEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,"LISTBOX","",WS_POPUP|WS_BORDER|WS_VSCROLL,rc->left,rc->bottom,rc->right-rc->left,150,NULL,NULL,hInst,NULL);
	SendMessage(hwndListEdit,WM_SETFONT,(WPARAM)hListFont,0);
	itemHeight=SendMessage(hwndListEdit,LB_GETITEMHEIGHT,0,0);
	for(j=0;j<setting[i].listCount;j++) {
		n=SendMessage(hwndListEdit,LB_ADDSTRING,0,(LPARAM)Translate(((ListTypeDataItem*)setting[i].pList)[j].szValue));
		SendMessage(hwndListEdit,LB_SETITEMDATA,n,((ListTypeDataItem*)setting[i].pList)[j].id);
		if((setting[i].dbType==DBVT_ASCIIZ && (!lstrcmp((char*)setting[i].value,((ListTypeDataItem*)setting[i].pList)[j].szValue))
											   || ((char*)setting[i].value==NULL && ((ListTypeDataItem*)setting[i].pList)[j].id==0))
		   || (setting[i].dbType!=DBVT_ASCIIZ && setting[i].value==((ListTypeDataItem*)setting[i].pList)[j].id))
			SendMessage(hwndListEdit,LB_SETCURSEL,n,0);
	}
	SendMessage(hwndListEdit,LB_SETTOPINDEX,SendMessage(hwndListEdit,LB_GETCURSEL,0,0)-3,0);
	if(itemHeight*setting[i].listCount<150)
		SetWindowPos(hwndListEdit,0,0,0,rc->right-rc->left,itemHeight*setting[i].listCount+GetSystemMetrics(SM_CYBORDER)*2,SWP_NOZORDER|SWP_NOMOVE);
	OldListEditProc=(WNDPROC)SetWindowLong(hwndListEdit,GWL_WNDPROC,(LONG)ListEditSubclassProc);
	if(MyAnimateWindow=(BOOL (WINAPI*)(HWND,DWORD,DWORD))GetProcAddress(GetModuleHandle("user32"),"AnimateWindow")) {
		BOOL enabled;
		SystemParametersInfo(SPI_GETCOMBOBOXANIMATION,0,&enabled,FALSE);
		if(enabled) MyAnimateWindow(hwndListEdit,200,AW_SLIDE|AW_ACTIVATE|AW_VER_POSITIVE);
	}
	ShowWindow(hwndListEdit,SW_SHOW);
	SetFocus(hwndListEdit);
	if(wVKey)
		PostMessage(hwndListEdit,WM_KEYDOWN,wVKey,0);
}

void EndListEdit(int save)
{
	if(hwndListEdit==NULL || iEditItem==-1) return;
	if(save) {
		int i;
		LPARAM newValue;
		i=SendMessage(hwndListEdit,LB_GETCURSEL,0,0);
		newValue=SendMessage(hwndListEdit,LB_GETITEMDATA,i,0);
		if(setting[iEditItem].dbType==DBVT_ASCIIZ) {
			char szNewValue[256];
			SendMessage(hwndListEdit,LB_GETTEXT,i,(LPARAM)szNewValue);
			if(newValue || setting[iEditItem].displayType&LIF_ZEROISVALID) {
				setting[iEditItem].changed=lstrcmp(szNewValue,(char*)setting[iEditItem].value);
				if((char*)setting[iEditItem].value) free((char*)setting[iEditItem].value);
				setting[iEditItem].value=(LPARAM)_strdup(szNewValue);
			}
			else {
				setting[iEditItem].changed=(char*)setting[iEditItem].value!=NULL;
				if((char*)setting[iEditItem].value) free((char*)setting[iEditItem].value);
				setting[iEditItem].value=(LPARAM)(char*)NULL;
			}
		}
		else {
			setting[iEditItem].changed=newValue!=setting[iEditItem].value;
			setting[iEditItem].value=newValue;
		}
		if(setting[iEditItem].changed) EnableWindow(GetDlgItem(GetParent(hwndList),IDC_SAVE),TRUE);
	}
	ListView_RedrawItems(hwndList,iEditItem,iEditItem);
	iEditItem=-1;
	DestroyWindow(hwndListEdit);
	hwndListEdit=NULL;
}

int IsListEditWindow(HWND hwnd)
{
	if(hwnd==hwndListEdit) return 1;
	return 0;
}