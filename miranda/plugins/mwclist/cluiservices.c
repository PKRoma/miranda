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
#include "m_clc.h"
#include "m_clui.h"

extern HWND hwndContactTree,hwndContactList,hwndStatus;

static int GetHwnd(WPARAM wParam,LPARAM lParam)
{
	return (int)hwndContactList;
}

int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam)
{
	int protoCount,i;
	PROTOCOLDESCRIPTOR **proto;
	int *partWidths,partCount;
	int borders[3];
	int status;
	int flags=0;

	SendMessage(hwndStatus,SB_GETBORDERS,0,(LPARAM)&borders);
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	if(protoCount==0) return 0;
	partWidths=(int*)mir_alloc(protoCount*sizeof(int));
	if(DBGetContactSettingByte(NULL,"CLUI","EqualSections",0)) {
		RECT rc;
		int part;
		GetClientRect(hwndStatus,&rc);
		rc.right-=borders[0]*2+GetSystemMetrics(SM_CXVSCROLL);
		for(partCount=0,i=protoCount-1;i>=0;i--)
			if(proto[i]->type==PROTOTYPE_PROTOCOL && CallProtoService(proto[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)!=0) partCount++;
		for(part=0,i=0;i<protoCount;i++) {
			if(proto[i]->type!=PROTOTYPE_PROTOCOL) continue;
			partWidths[part]=(part+1)*rc.right/partCount-(borders[2]>>1);
			part++;
		}
	}
	else {
		char *modeDescr;
		HDC hdc;
		SIZE textSize;
		BYTE showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",1);
		int x;
		char szName[32];

		hdc=GetDC(NULL);
		SelectObject(hdc,(HFONT)SendMessage(hwndStatus,WM_GETFONT,0,0));
		for(partCount=0,i=protoCount-1;i>=0;i--) {      //count down since built in ones tend to go at the end
			if(proto[i]->type!=PROTOTYPE_PROTOCOL || CallProtoService(proto[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)==0) continue;
			x=2;
			if(showOpts&1) x+=GetSystemMetrics(SM_CXSMICON);
			if(showOpts&2) {
				CallProtoService(proto[i]->szName,PS_GETNAME,sizeof(szName),(LPARAM)szName);
				if(showOpts&4 && lstrlen(szName)<sizeof(szName)-1) lstrcat(szName," ");
				GetTextExtentPoint32(hdc,szName,lstrlen(szName),&textSize);				
				x+=textSize.cx;				
				x+=GetSystemMetrics(SM_CXBORDER)*4; // The SB panel doesnt allocate enough room
			}
			if(showOpts&4) {
				modeDescr=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,CallProtoService(proto[i]->szName,PS_GETSTATUS,0,0),0);
				GetTextExtentPoint32(hdc,modeDescr,lstrlen(modeDescr),&textSize);
				x+=textSize.cx;
				x+=GetSystemMetrics(SM_CXBORDER)*4; // The SB panel doesnt allocate enough room
			}			
			partWidths[partCount]=(partCount?partWidths[partCount-1]:0)+x+2;
			partCount++;
		}
		ReleaseDC(NULL,hdc);
	}
	if(partCount==0) {
		mir_free(partWidths);
		return 0;
	}
	partWidths[partCount-1]=-1;
	SendMessage(hwndStatus,SB_SETMINHEIGHT,GetSystemMetrics(SM_CYSMICON),0);
	SendMessage(hwndStatus,SB_SETPARTS,partCount,(LPARAM)partWidths);
	mir_free(partWidths);
	flags = SBT_OWNERDRAW; 
	if ( DBGetContactSettingByte(NULL,"CLUI","SBarBevel", 1)==0 ) flags |= SBT_NOBORDERS;
	for(partCount=0, i=protoCount-1; i >= 0; i--) {      //count down since built in ones tend to go at the end
		// okay, so it was a bug ;)
		if(proto[i]->type!=PROTOTYPE_PROTOCOL || (CallProtoService(proto[i]->szName, PS_GETCAPS, PFLAGNUM_2, 0)==0)) continue;
		status=CallProtoService(proto[i]->szName,PS_GETSTATUS,0,0);
		SendMessage(hwndStatus,SB_SETTEXT,partCount|flags,(LPARAM)proto[i]->szName);
		partCount++;
	}
	return 0;
}

int SortList(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int GroupAdded(WPARAM wParam,LPARAM lParam)
{
	//CLC does this automatically unless it's a new group
	if(lParam) {
		HANDLE hItem;
		char szFocusClass[64];
		HWND hwndFocus=GetFocus();

		GetClassName(hwndFocus,szFocusClass,sizeof(szFocusClass));
		if(!lstrcmp(szFocusClass,CLISTCONTROL_CLASS)) {
			hItem=(HANDLE)SendMessage(hwndFocus,CLM_FINDGROUP,wParam,0);
			if(hItem) SendMessage(hwndFocus,CLM_EDITLABEL,(WPARAM)hItem,0);
		}
	}
	return 0;
}

static int ContactSetIcon(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ContactDeleted(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ContactAdded(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ListBeginRebuild(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int ListEndRebuild(WPARAM wParam,LPARAM lParam)
{
	int rebuild=0;
	//CLC does this automatically, but we need to force it if hideoffline or hideempty has changed
	if((DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT)==0)!=((GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_HIDEOFFLINE)==0)) {
		if(DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT))
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)|CLS_HIDEOFFLINE);
		else
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)&~CLS_HIDEOFFLINE);
		rebuild=1;
	}
	if((DBGetContactSettingByte(NULL,"CList","HideEmptyGroups",SETTING_HIDEEMPTYGROUPS_DEFAULT)==0)!=((GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_HIDEEMPTYGROUPS)==0)) {
		if(DBGetContactSettingByte(NULL,"CList","HideEmptyGroups",SETTING_HIDEEMPTYGROUPS_DEFAULT))
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)|CLS_HIDEEMPTYGROUPS);
		else
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)&~CLS_HIDEEMPTYGROUPS);
		rebuild=1;
	}
	if((DBGetContactSettingByte(NULL,"CList","UseGroups",SETTING_USEGROUPS_DEFAULT)==0)!=((GetWindowLong(hwndContactTree,GWL_STYLE)&CLS_USEGROUPS)==0)) {
		if(DBGetContactSettingByte(NULL,"CList","UseGroups",SETTING_USEGROUPS_DEFAULT))
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)|CLS_USEGROUPS);
		else
			SetWindowLong(hwndContactTree,GWL_STYLE,GetWindowLong(hwndContactTree,GWL_STYLE)&~CLS_USEGROUPS);
		rebuild=1;
	}
	if(rebuild) SendMessage(hwndContactTree,CLM_AUTOREBUILD,0,0);
	return 0;
}

static int ContactRenamed(WPARAM wParam,LPARAM lParam)
{
	//unnecessary: CLC does this automatically
	return 0;
}

static int GetCaps(WPARAM wParam,LPARAM lParam)
{
	switch(wParam) {
		case CLUICAPS_FLAGS1:
			return CLUIF_HIDEEMPTYGROUPS|CLUIF_DISABLEGROUPS|CLUIF_HASONTOPOPTION|CLUIF_HASAUTOHIDEOPTION;
	}
	return 0;
}

int LoadCluiServices(void)
{
	CreateServiceFunction(MS_CLUI_GETHWND,GetHwnd);
	CreateServiceFunction(MS_CLUI_PROTOCOLSTATUSCHANGED,CluiProtocolStatusChanged);
	CreateServiceFunction(MS_CLUI_GROUPADDED,GroupAdded);
	CreateServiceFunction(MS_CLUI_CONTACTSETICON,ContactSetIcon);
	CreateServiceFunction(MS_CLUI_CONTACTADDED,ContactAdded);
	CreateServiceFunction(MS_CLUI_CONTACTDELETED,ContactDeleted);
	CreateServiceFunction(MS_CLUI_CONTACTRENAMED,ContactRenamed);
	CreateServiceFunction(MS_CLUI_LISTBEGINREBUILD,ListBeginRebuild);
	CreateServiceFunction(MS_CLUI_LISTENDREBUILD,ListEndRebuild);
	CreateServiceFunction(MS_CLUI_SORTLIST,SortList);
	CreateServiceFunction(MS_CLUI_GETCAPS,GetCaps);
	return 0;
}