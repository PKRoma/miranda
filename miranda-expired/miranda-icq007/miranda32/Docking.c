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

#include <windows.h>
#include "global.h"
#include "docking.h"
#include "resource.h"

APPBARDATA apd;

void Docking_Init(HWND hwnd)
{
	if (apd.hWnd!=hwnd)
	{
		apd.cbSize=sizeof(apd);
		apd.hWnd=hwnd;
		apd.lParam=NULL;
		apd.uCallbackMessage=WM_DOCKCB; //WM_SHAPPBAR;
		SHAppBarMessage(ABM_NEW,&apd);	
	}

	Docking_UpdateSize(hwnd);
	Docking_ResizeWndToDock();
}

/*void Docking_UpdateSize(HWND hwnd)
{
	RECT mywnd;
	int iWidth;

	GetClientRect(hwnd,&mywnd);
	
    

	if (opts.dockstate==DOCK_RIGHT)
	{
		apd.uEdge=ABE_RIGHT;
		apd.rc.left=GetSystemMetrics(SM_CXSCREEN)-mywnd.right;
		
	}
	else
	{
		apd.uEdge=ABE_LEFT;
		apd.rc.left=0;
	}

	apd.rc.top=0;
	apd.rc.bottom=GetSystemMetrics(SM_CYSCREEN);
	
	
	//apd.rc.left=mywnd.left;
	
	
	apd.rc.right=mywnd.right;

	//iWidth=apd.rc.right - apd.rc.left;

	// Query the system for an approved size and position. 
    //SHAppBarMessage(ABM_QUERYPOS, &apd); 
 
///
	
	switch(apd.uEdge)
	{
	case ABE_LEFT:
		apd.rc.left=0;
		apd.rc.right=apd.rc.left+iWidth;
		
		break;
	case ABE_RIGHT:
		
		apd.rc.left=apd.rc.right-iWidth;
		break;
	}
///
	SHAppBarMessage(ABM_SETPOS,&apd);

}*/


void Docking_UpdateSize(HWND hwnd)
{
	RECT mywnd;
	int iWidth;

	GetClientRect(hwnd,&mywnd);
	
    

	if (opts.dockstate==DOCK_RIGHT)
		apd.uEdge=ABE_RIGHT;
	else
		apd.uEdge=ABE_LEFT;

	apd.rc.top=0;
	apd.rc.left=0;
	apd.rc.bottom=GetSystemMetrics(SM_CYSCREEN);
	apd.rc.right=GetSystemMetrics(SM_CXSCREEN);
	


	// Query the system for an approved size and position. 
    SHAppBarMessage(ABM_QUERYPOS, &apd); 
 
 
	
	switch(apd.uEdge)
	{
	case ABE_LEFT:
		apd.rc.left=0;
		apd.rc.right=apd.rc.left+mywnd.right;
		
		break;
	case ABE_RIGHT:
		
		apd.rc.left=apd.rc.right-mywnd.right;
		break;
	}

	SHAppBarMessage(ABM_SETPOS,&apd);

}

void Docking_ResizeWndToDock()
{
	//SHAppBarMessage(ABM_QUERYPOS,&apd);
	//SetWindowPos(apd.hWnd,HWND_TOPMOST,apd.rc.left,apd.rc.top,apd.rc.right,apd.rc.bottom,0);
	
	MoveWindow(apd.hWnd,apd.rc.left,apd.rc.top,apd.rc.right-apd.rc.left,apd.rc.bottom-apd.rc.top,FALSE);
}

void Docking_Kill(HWND hwnd)
{
	long flag;
	
	if (!(opts.flags1 & FG_ONTOP))
	{
		flag=HWND_NOTOPMOST;
	}
	else
	{
		flag=HWND_TOPMOST;
	}

	
	SHAppBarMessage(ABM_REMOVE,&apd);
	apd.hWnd=NULL;

	SetWindowPos(hwnd,flag,opts.pos_mainwnd.xpos,opts.pos_mainwnd.ypos,opts.pos_mainwnd.width,opts.pos_mainwnd.height,0);
	
}


void Docking_UpdateMenu(HWND hwnd)
{
	HMENU hmenu;
	
	hmenu = GetMenu(hwnd);
	hmenu = GetSubMenu(hmenu, 0);
	hmenu = GetSubMenu(hmenu,2);	
	CheckMenuItem(hmenu, ID_DOCK_NONE, MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_DOCK_LEFT, MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_DOCK_RIGHT, MF_UNCHECKED);

	switch(opts.dockstate)
	{
	case DOCK_NONE:
		CheckMenuItem(hmenu, ID_DOCK_NONE, MF_CHECKED);
		break;
	case DOCK_RIGHT:
		CheckMenuItem(hmenu, ID_DOCK_RIGHT, MF_CHECKED);
		break;
	case DOCK_LEFT:
		CheckMenuItem(hmenu, ID_DOCK_LEFT, MF_CHECKED);
		break;
	}
}