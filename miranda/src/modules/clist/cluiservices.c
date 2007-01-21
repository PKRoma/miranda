/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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
#include "commonheaders.h"
#include "clc.h"

static int GetHwnd(WPARAM wParam, LPARAM lParam)
{
	return (int)cli.hwndContactList;
}

static int GetHwndTree(WPARAM wParam,LPARAM lParam)
{
	return (int)cli.hwndContactTree;
}

static int CluiProtocolStatusChanged(WPARAM wParam, LPARAM lParam)
{
	cli.pfnCluiProtocolStatusChanged( wParam, (const char*)lParam );
	return 0;
}

int SortList(WPARAM wParam, LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int GroupAdded(WPARAM wParam, LPARAM lParam)
{
	//CLC does this automatically unless it's a new group
	if (lParam) {
		HANDLE hItem;
		TCHAR szFocusClass[64];
		HWND hwndFocus = GetFocus();

		GetClassName(hwndFocus, szFocusClass, SIZEOF(szFocusClass));
		if (!lstrcmp(szFocusClass, CLISTCONTROL_CLASS)) {
			hItem = (HANDLE) SendMessage(hwndFocus, CLM_FINDGROUP, wParam, 0);
			if (hItem)
				SendMessage(hwndFocus, CLM_EDITLABEL, (WPARAM) hItem, 0);
		}
	}
	return 0;
}

static int ContactSetIcon(WPARAM wParam, LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ContactDeleted(WPARAM wParam, LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ContactAdded(WPARAM wParam, LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ListBeginRebuild(WPARAM wParam, LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ListEndRebuild(WPARAM wParam, LPARAM lParam)
{
	int rebuild = 0;
	//CLC does this automatically, but we need to force it if hideoffline or hideempty has changed
	if ((DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT) == 0) != ((GetWindowLong(cli.hwndContactTree, GWL_STYLE) & CLS_HIDEOFFLINE) == 0)) {
		if (DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT))
			SetWindowLong(cli.hwndContactTree, GWL_STYLE, GetWindowLong(cli.hwndContactTree, GWL_STYLE) | CLS_HIDEOFFLINE);
		else
			SetWindowLong(cli.hwndContactTree, GWL_STYLE, GetWindowLong(cli.hwndContactTree, GWL_STYLE) & ~CLS_HIDEOFFLINE);
		rebuild = 1;
	}
	if ((DBGetContactSettingByte(NULL, "CList", "HideEmptyGroups", SETTING_HIDEEMPTYGROUPS_DEFAULT) == 0) != ((GetWindowLong(cli.hwndContactTree, GWL_STYLE) & CLS_HIDEEMPTYGROUPS) == 0)) {
		if (DBGetContactSettingByte(NULL, "CList", "HideEmptyGroups", SETTING_HIDEEMPTYGROUPS_DEFAULT))
			SetWindowLong(cli.hwndContactTree, GWL_STYLE, GetWindowLong(cli.hwndContactTree, GWL_STYLE) | CLS_HIDEEMPTYGROUPS);
		else
			SetWindowLong(cli.hwndContactTree, GWL_STYLE, GetWindowLong(cli.hwndContactTree, GWL_STYLE) & ~CLS_HIDEEMPTYGROUPS);
		rebuild = 1;
	}
	if ((DBGetContactSettingByte(NULL, "CList", "UseGroups", SETTING_USEGROUPS_DEFAULT) == 0) != ((GetWindowLong(cli.hwndContactTree, GWL_STYLE) & CLS_USEGROUPS) == 0)) {
		if (DBGetContactSettingByte(NULL, "CList", "UseGroups", SETTING_USEGROUPS_DEFAULT))
			SetWindowLong(cli.hwndContactTree, GWL_STYLE, GetWindowLong(cli.hwndContactTree, GWL_STYLE) | CLS_USEGROUPS);
		else
			SetWindowLong(cli.hwndContactTree, GWL_STYLE, GetWindowLong(cli.hwndContactTree, GWL_STYLE) & ~CLS_USEGROUPS);
		rebuild = 1;
	}
	if (rebuild)
		SendMessage(cli.hwndContactTree, CLM_AUTOREBUILD, 0, 0);
	return 0;
}

static int ContactRenamed(WPARAM wParam, LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int GetCaps(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
	case CLUICAPS_FLAGS1:
		return CLUIF_HIDEEMPTYGROUPS | CLUIF_DISABLEGROUPS | CLUIF_HASONTOPOPTION | CLUIF_HASAUTOHIDEOPTION;
	}
	return 0;
}

int LoadCluiServices(void)
{
	CreateServiceFunction(MS_CLUI_GETHWND, GetHwnd);
	CreateServiceFunction(MS_CLUI_GETHWNDTREE,GetHwndTree);
	CreateServiceFunction(MS_CLUI_PROTOCOLSTATUSCHANGED, CluiProtocolStatusChanged);
	CreateServiceFunction(MS_CLUI_GROUPADDED, GroupAdded);
	CreateServiceFunction(MS_CLUI_CONTACTSETICON, ContactSetIcon);
	CreateServiceFunction(MS_CLUI_CONTACTADDED, ContactAdded);
	CreateServiceFunction(MS_CLUI_CONTACTDELETED, ContactDeleted);
	CreateServiceFunction(MS_CLUI_CONTACTRENAMED, ContactRenamed);
	CreateServiceFunction(MS_CLUI_LISTBEGINREBUILD, ListBeginRebuild);
	CreateServiceFunction(MS_CLUI_LISTENDREBUILD, ListEndRebuild);
	CreateServiceFunction(MS_CLUI_SORTLIST, SortList);
	CreateServiceFunction(MS_CLUI_GETCAPS, GetCaps);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// default protocol status notification handler

int fnCluiProtocolStatusChanged(int parStatus, const char* szProto)
{
	int i, *partWidths;
	int borders[3];
	int status;
	int flags = 0;

	if ( cli.menuProtoCount == 0 )
		return 0;

	SendMessage(cli.hwndStatus, SB_GETBORDERS, 0, ( LPARAM )&borders);

	partWidths = ( int* )alloca( cli.menuProtoCount * sizeof( int ));
	if ( DBGetContactSettingByte( NULL, "CLUI", "EqualSections", 0 )) {
		RECT rc;
		GetClientRect( cli.hwndStatus, &rc );
		rc.right -= borders[0] * 2 + ( DBGetContactSettingByte( NULL, "CLUI", "ShowGrip", 1 ) ? GetSystemMetrics( SM_CXVSCROLL ) : 0 );
		for ( i = 0; i < cli.menuProtoCount; i++ )
			partWidths[ i ] = ( i+1 ) * rc.right / cli.menuProtoCount - (borders[2] >> 1);
	}
	else {
		HDC hdc;
		SIZE textSize;
		BYTE showOpts = DBGetContactSettingByte(NULL, "CLUI", "SBarShow", 1);
		char szName[32];

		hdc = GetDC(NULL);
		SelectObject(hdc, (HFONT) SendMessage(cli.hwndStatus, WM_GETFONT, 0, 0));
		for ( i = 0; i < cli.menuProtoCount; i++ ) {  //count down since built in ones tend to go at the end
			int x = 2;
			if ( showOpts & 1 )
				x += g_IconWidth;
			if ( showOpts & 2 ) {
				CallProtoService( cli.menuProtos[i].szProto, PS_GETNAME, SIZEOF(szName), (LPARAM) szName);
				if ( showOpts & 4 && lstrlenA(szName) < SIZEOF(szName)-1 )
					lstrcatA( szName, " " );
				GetTextExtentPoint32A(hdc, szName, lstrlenA(szName), &textSize);
				x += textSize.cx;
				x += GetSystemMetrics(SM_CXBORDER) * 4; // The SB panel doesnt allocate enough room
			}
			if ( showOpts & 4 ) {
				TCHAR* modeDescr = ( TCHAR* )CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, CallProtoService( cli.menuProtos[i].szProto, PS_GETSTATUS, 0, 0), GCMDF_TCHAR );
				GetTextExtentPoint32(hdc, modeDescr, lstrlen(modeDescr), &textSize);
				x += textSize.cx;
				x += GetSystemMetrics(SM_CXBORDER) * 4; // The SB panel doesnt allocate enough room
			}
			partWidths[ i ] = ( i ? partWidths[ i-1] : 0 ) + x + 2;
		}
		ReleaseDC(NULL, hdc);
	}

	partWidths[ cli.menuProtoCount-1 ] = -1;
	SendMessage(cli.hwndStatus, SB_SETMINHEIGHT, g_IconHeight, 0);
	SendMessage(cli.hwndStatus, SB_SETPARTS, cli.menuProtoCount, ( LPARAM )partWidths);
	flags = SBT_OWNERDRAW;
	if ( DBGetContactSettingByte( NULL, "CLUI", "SBarBevel", 1 ) == 0 )
		flags |= SBT_NOBORDERS;
	for ( i = 0; i < cli.menuProtoCount; i++ ) {
		status = CallProtoService( cli.menuProtos[i].szProto, PS_GETSTATUS, 0, 0);
		SendMessage( cli.hwndStatus, SB_SETTEXT, i | flags, ( LPARAM )cli.menuProtos[i].szProto );
	}
	return 0;
}
