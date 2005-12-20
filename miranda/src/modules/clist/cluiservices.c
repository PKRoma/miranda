/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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
	cli.pfnCluiProtocolStatusChanged();
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

int fnCluiProtocolStatusChanged(void)
{
	int protoCount, i;
	PROTOCOLDESCRIPTOR **proto;
	int *partWidths, partCount;
	int borders[3];
	int status;
	int flags = 0;

	SendMessage(cli.hwndStatus, SB_GETBORDERS, 0, (LPARAM) & borders);
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM) & protoCount, (LPARAM) & proto);
	if (protoCount == 0)
		return 0;
	partWidths = (int *) malloc(protoCount * sizeof(int));
	if (DBGetContactSettingByte(NULL, "CLUI", "EqualSections", 0)) {
		RECT rc;
		int part;
		GetClientRect(cli.hwndStatus, &rc);
		rc.right -= borders[0] * 2 + (DBGetContactSettingByte(NULL, "CLUI", "ShowGrip", 1) ? GetSystemMetrics(SM_CXVSCROLL) : 0);
		for (partCount = 0, i = protoCount - 1; i >= 0; i--)
			if (proto[i]->type == PROTOTYPE_PROTOCOL && CallProtoService(proto[i]->szName, PS_GETCAPS, PFLAGNUM_2, 0) != 0)
				partCount++;
		for (part = 0, i = 0; i < protoCount; i++) {
			if (proto[i]->type != PROTOTYPE_PROTOCOL)
				continue;
			partWidths[part] = (part + 1) * rc.right / partCount - (borders[2] >> 1);
			part++;
		}
	}
	else {
		char *modeDescr;
		HDC hdc;
		SIZE textSize;
		BYTE showOpts = DBGetContactSettingByte(NULL, "CLUI", "SBarShow", 1);
		int x;
		char szName[32];

		hdc = GetDC(NULL);
		SelectObject(hdc, (HFONT) SendMessage(cli.hwndStatus, WM_GETFONT, 0, 0));
		for (partCount = 0, i = protoCount - 1; i >= 0; i--) {  //count down since built in ones tend to go at the end
			if (proto[i]->type != PROTOTYPE_PROTOCOL || CallProtoService(proto[i]->szName, PS_GETCAPS, PFLAGNUM_2, 0) == 0)
				continue;
			x = 2;
			if (showOpts & 1)
				x += g_IconWidth;
			if (showOpts & 2) {
				CallProtoService(proto[i]->szName, PS_GETNAME, SIZEOF(szName), (LPARAM) szName);
				if (showOpts & 4 && lstrlenA(szName) < SIZEOF(szName) - 1)
					lstrcatA(szName, " ");
				GetTextExtentPoint32A(hdc, szName, lstrlenA(szName), &textSize);
				x += textSize.cx;
				x += GetSystemMetrics(SM_CXBORDER) * 4; // The SB panel doesnt allocate enough room
			}
			if (showOpts & 4) {
				modeDescr = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, CallProtoService(proto[i]->szName, PS_GETSTATUS, 0, 0), 0);
				GetTextExtentPoint32A(hdc, modeDescr, lstrlenA(modeDescr), &textSize);
				x += textSize.cx;
				x += GetSystemMetrics(SM_CXBORDER) * 4; // The SB panel doesnt allocate enough room
			}
			partWidths[partCount] = (partCount ? partWidths[partCount - 1] : 0) + x + 2;
			partCount++;
		}
		ReleaseDC(NULL, hdc);
	}
	if (partCount == 0) {
		free(partWidths);
		return 0;
	}
	partWidths[partCount - 1] = -1;
	SendMessage(cli.hwndStatus, SB_SETMINHEIGHT, g_IconHeight, 0);
	SendMessage(cli.hwndStatus, SB_SETPARTS, partCount, (LPARAM) partWidths);
	free(partWidths);
	flags = SBT_OWNERDRAW;
	if (DBGetContactSettingByte(NULL, "CLUI", "SBarBevel", 1) == 0)
		flags |= SBT_NOBORDERS;
	for (partCount = 0, i = protoCount - 1; i >= 0; i--) {      //count down since built in ones tend to go at the end
		// okay, so it was a bug ;)
		if (proto[i]->type != PROTOTYPE_PROTOCOL || (CallProtoService(proto[i]->szName, PS_GETCAPS, PFLAGNUM_2, 0) == 0))
			continue;
		status = CallProtoService(proto[i]->szName, PS_GETSTATUS, 0, 0);
		SendMessage(cli.hwndStatus, SB_SETTEXT, partCount | flags, (LPARAM) proto[i]->szName);
		partCount++;
	}
	return 0;
}
