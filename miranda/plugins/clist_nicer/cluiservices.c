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

UNICODE done

*/
#include "commonheaders.h"
#include "cluiframes/cluiframes.h"
#include <m_icq.h>

extern HIMAGELIST hCListImages, himlExtraImages;;
extern struct CluiData g_CluiData;
extern int g_shutDown;
extern PLUGININFOEX pluginInfo;

extern ButtonItem *g_ButtonItems;

static int GetClistVersion(WPARAM wParam, LPARAM lParam)
{
	static char g_szVersionString[256];

	mir_snprintf(g_szVersionString, 256, "%s, %d.%d.%d.%d", pluginInfo.shortName, HIBYTE(HIWORD(pluginInfo.version)), LOBYTE(HIWORD(pluginInfo.version)), HIBYTE(LOWORD(pluginInfo.version)), LOBYTE(LOBYTE(pluginInfo.version)));
	if(!IsBadWritePtr((LPVOID)lParam, 4))
		*((DWORD *)lParam) = pluginInfo.version;

	return (int)g_szVersionString;
}


void FreeProtocolData( void )
{
	//free protocol data
	int nPanel;
	int nParts=SendMessage(pcli->hwndStatus,SB_GETPARTS,0,0);
	for (nPanel=0;nPanel<nParts;nPanel++)
	{
		ProtocolData *PD;
		PD=(ProtocolData *)SendMessage(pcli->hwndStatus,SB_GETTEXT,(WPARAM)nPanel,(LPARAM)0);
		if (PD!=NULL&&!IsBadCodePtr((void *)PD))
		{
			SendMessage(pcli->hwndStatus,SB_SETTEXT,(WPARAM)nPanel|SBT_OWNERDRAW,(LPARAM)0);
			if (PD->RealName) mir_free(PD->RealName);
			if (PD) mir_free(PD);
}	}	}

int g_maxStatus = ID_STATUS_OFFLINE;
char g_maxProto[100] = "";

void CluiProtocolStatusChanged( int parStatus, const char* szProto )
{
	int protoCount,i;
	PROTOCOLDESCRIPTOR **proto;
	PROTOCOLDESCRIPTOR *curprotocol;
	int *partWidths,partCount;
	int borders[3];
	int status;
	int storedcount;
	char *szStoredName;
	char buf[10];
	int toshow;
	char *szStatus = NULL;
	char *szMaxProto = NULL;
	int maxOnline = 0, onlineness = 0;
	WORD maxStatus = ID_STATUS_OFFLINE, wStatus;
	DBVARIANT dbv = {0};
	int iIcon = 0;
	HICON hIcon = 0;
	int rdelta = g_CluiData.bCLeft + g_CluiData.bCRight;
	BYTE windowStyle;

	if (pcli->hwndStatus == 0 || g_shutDown)
		return;

	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&protoCount, (LPARAM)&proto);
	if (protoCount == 0)
		return;

	storedcount=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
	if (storedcount==-1)
		return;

	FreeProtocolData();
	g_maxStatus = ID_STATUS_OFFLINE;
	g_maxProto[0] = 0;

	SendMessage(pcli->hwndStatus,SB_GETBORDERS,0,(LPARAM)&borders);

	partWidths=(int*)_alloca((storedcount+1)*sizeof(int));

	if (g_CluiData.bEqualSections) {
		RECT rc;
		int part;
		//SendMessage(pcli->hwndStatus,WM_SIZE,0,0);        // XXX fix (may break status bar geometry)
		GetClientRect(pcli->hwndStatus,&rc);
		rc.right-=borders[0]*2;
		toshow=0;
		for (i=0;i<storedcount;i++) {
			_itoa(OFFSET_VISIBLE+i,(char *)&buf,10);
			if (DBGetContactSettingDword(0,"Protocols",(char *)&buf,1)==0)
				continue;
			toshow++;
		};

		if (toshow>0) {
			for (part=0,i=0;i<storedcount;i++) {

				_itoa(OFFSET_VISIBLE+i,(char *)&buf,10);
				if (DBGetContactSettingDword(0,"Protocols",(char *)&buf,1)==0)
					continue;

				partWidths[part]=((rc.right-rc.left-rdelta)/toshow)*(part+1) + g_CluiData.bCLeft;
				if(part == storedcount - 1)
					partWidths[part] += g_CluiData.bCRight;
				part++;
			}
		}
		partCount=toshow;
	}
	else {
		HDC hdc;
		SIZE textSize;
		BYTE showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",1);
		int x;
		char szName[32];
		HFONT hofont;

		hdc=GetDC(NULL);
		hofont=SelectObject(hdc,(HFONT)SendMessage(pcli->hwndStatus,WM_GETFONT,0,0));

		for (partCount=0,i=0;i<storedcount;i++) {      //count down since built in ones tend to go at the end

			_itoa(OFFSET_VISIBLE+i,(char *)&buf,10);
			//show this protocol ?
			if (DBGetContactSettingDword(0,"Protocols",(char *)&buf,1)==0)
				continue;

			_itoa(i,buf,10);
			szStoredName=DBGetString(NULL,"Protocols",buf);
			if (szStoredName==NULL)
				continue;
			curprotocol=(PROTOCOLDESCRIPTOR*)CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)szStoredName);
			if (curprotocol==0)
				continue;
			mir_free(szStoredName);
			x=2;
			if (showOpts & 1)
				x += 16;
			if (showOpts & 2) {
				CallProtoService(curprotocol->szName,PS_GETNAME,sizeof(szName),(LPARAM)szName);
				if (showOpts&4 && lstrlenA(szName)<sizeof(szName)-1) lstrcatA(szName," ");
				GetTextExtentPoint32A(hdc,szName,lstrlenA(szName),&textSize);
				x += textSize.cx + GetSystemMetrics(SM_CXBORDER) * 4; // The SB panel doesnt allocate enough room
			}
			if (showOpts & 4) {
				TCHAR* modeDescr = pcli->pfnGetStatusModeDescription( CallProtoService(curprotocol->szName,PS_GETSTATUS,0,0 ), 0 );
				GetTextExtentPoint32(hdc, modeDescr, lstrlen(modeDescr), &textSize );
				x += textSize.cx + GetSystemMetrics(SM_CXBORDER) * 4; // The SB panel doesnt allocate enough room
			}
			partWidths[partCount]=(partCount?partWidths[partCount-1]:g_CluiData.bCLeft)+ x + 2;
			partCount++;
		}
		SelectObject(hdc,hofont);
		ReleaseDC(NULL,hdc);
	}
	if (partCount==0) {
		SendMessage(pcli->hwndStatus,SB_SIMPLE,TRUE,0);
		return;
	}
	SendMessage(pcli->hwndStatus,SB_SIMPLE,FALSE,0);

	partWidths[partCount-1]=-1;
	windowStyle = DBGetContactSettingByte(NULL, "CLUI", "WindowStyle", 0);
	SendMessage(pcli->hwndStatus,SB_SETMINHEIGHT, 18 + g_CluiData.bClipBorder + ((windowStyle == SETTING_WINDOWSTYLE_THINBORDER || windowStyle == SETTING_WINDOWSTYLE_NOBORDER) ? 3 : 0), 0);
	SendMessage(pcli->hwndStatus, SB_SETPARTS, partCount, (LPARAM)partWidths);

	for (partCount=0,i=0;i<storedcount;i++) {      //count down since built in ones tend to go at the end
		ProtocolData    *PD;
		int caps1, caps2;

		_itoa(OFFSET_VISIBLE+i,(char *)&buf,10);
		//show this protocol ?
		if (DBGetContactSettingDword(0,"Protocols",(char *)&buf,1)==0)
			continue;

		_itoa(i,(char *)&buf,10);
		szStoredName=DBGetString(NULL,"Protocols",buf);
		if (szStoredName==NULL)
			continue;
		curprotocol=(PROTOCOLDESCRIPTOR*)CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)szStoredName);
		mir_free(szStoredName);
		if (curprotocol==0)
			continue;
		status=CallProtoService(curprotocol->szName,PS_GETSTATUS,0,0);
		PD=(ProtocolData*)mir_alloc(sizeof(ProtocolData));
		PD->RealName=mir_strdup(curprotocol->szName);
		PD->statusbarpos = partCount;
		_itoa(OFFSET_PROTOPOS+i,(char *)&buf,10);
		PD->protopos=DBGetContactSettingDword(NULL,"Protocols",(char *)&buf,-1);
		{
			int flags;
			flags = SBT_OWNERDRAW;
			if ( DBGetContactSettingByte(NULL,"CLUI","SBarBevel", 1)==0 ) flags |= SBT_NOBORDERS;
			SendMessageA(pcli->hwndStatus,SB_SETTEXTA,partCount|flags,(LPARAM)PD);
		}
		caps2 = CallProtoService(curprotocol->szName, PS_GETCAPS, PFLAGNUM_2, 0);
		caps1 = CallProtoService(curprotocol->szName, PS_GETCAPS, PFLAGNUM_1, 0);
		if((caps1 & PF1_IM) && (caps2 & (PF2_LONGAWAY | PF2_SHORTAWAY))) {
			onlineness = GetStatusOnlineness(status);
			if(onlineness > maxOnline) {
				maxStatus = status;
				maxOnline = onlineness;
				szMaxProto = curprotocol->szName;
			}
		}
/* // Joe @ Whale: The core handles this by itself now, do not mess with it // 0.7.0.8+
    if(pcli->menuProtos != NULL) {
			int i;

			for(i = 0; i < pcli->menuProtoCount; i++) {
				if(!strcmp(pcli->menuProtos[i].szProto, curprotocol->szName) && pcli->menuProtos[i].menuID != 0 && pcli->menuProtos[i].hasAdded !=0) {
					CLISTMENUITEM mi = {0};
					char szServiceName[128];
					TCHAR xStatusName[128];
					int xStatus = 0;
					int xStatusCount;
					ICQ_CUSTOM_STATUS cst = {0};
					mi.cbSize = sizeof(mi);
					mi.flags = CMIM_FLAGS | CMIM_NAME | CMIM_ICON | CMIF_TCHAR;

					mir_snprintf(szServiceName, 128, "%s%s", curprotocol->szName, PS_ICQ_GETCUSTOMSTATUSEX);

					cst.cbSize = sizeof(ICQ_CUSTOM_STATUS);
					cst.flags = CSSF_MASK_STATUS | CSSF_STATUSES_COUNT;
					cst.wParam = &xStatusCount;
					cst.status = &xStatus;
					if(ServiceExists(szServiceName) && !CallService(szServiceName, 0, (LPARAM)&cst)) {
						if(xStatus > 0 && xStatus <= xStatusCount) {
							mi.hIcon = (HICON)CallProtoService(curprotocol->szName, PS_ICQ_GETCUSTOMSTATUSICON, 0, LR_SHARED);	// get OWN xStatus icon (if set)

							cst.flags = CSSF_MASK_NAME | CSSF_DEFAULT_NAME | CSSF_TCHAR;
							cst.wParam = &xStatus;
							cst.ptszName = xStatusName; 
							if(!CallService(szServiceName, 0, (LPARAM)&cst))
								mi.ptszName = xStatusName;
							else
								mi.ptszName = _T("Custom Status");
						}	
						else {
							mi.ptszName = _T("None");
							mi.flags |= CMIF_CHECKED;
							mi.hIcon = 0;
						}
						CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)pcli->menuProtos[i].hasAdded, (LPARAM)&mi);
					}
					break;
				}
			}
		}*/
		partCount++;
	}
	// update the clui button

	if (!DBGetContactSetting(NULL, "CList", "PrimaryStatus", &dbv)) {
		if (dbv.type == DBVT_ASCIIZ && lstrlenA(dbv.pszVal) > 1) {
			wStatus = (WORD) CallProtoService(dbv.pszVal, PS_GETSTATUS, 0, 0);
			iIcon = IconFromStatusMode(dbv.pszVal, (int) wStatus, 0, &hIcon);
		}
		mir_free(dbv.pszVal);
	} else {
		wStatus = maxStatus;
		iIcon = IconFromStatusMode((wStatus >= ID_STATUS_CONNECTING && wStatus < ID_STATUS_OFFLINE) ? szMaxProto : NULL, (int) wStatus, 0, &hIcon);
		g_maxStatus = (int)wStatus;
		if(szMaxProto) {
			lstrcpynA(g_maxProto, szMaxProto, 100);
			g_maxProto[99] = 0;
		}
	}
	/*
	* this is used globally (actually, by the clist control only) to determine if
	* any protocol is "in connection" state. If true, then the clist discards redraws
	* and uses timer based sort and redraw handling. This can improve performance
	* when connecting multiple protocols significantly.
	*/
	//g_isConnecting = (wStatus >= ID_STATUS_CONNECTING && wStatus < ID_STATUS_OFFLINE);
	szStatus = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM) wStatus, 0);

	/*
	* set the global status icon and display the global (most online) status mode on the
	* status mode button
	*/

	if (szStatus) {
		if(pcli->hwndContactList && IsWindow(GetDlgItem(pcli->hwndContactList, IDC_TBGLOBALSTATUS)) && IsWindow(GetDlgItem(pcli->hwndContactList, IDC_TBTOPSTATUS))) {
			SendMessageA(GetDlgItem(pcli->hwndContactList, IDC_TBGLOBALSTATUS), WM_SETTEXT, 0, (LPARAM) szStatus);
			if(!hIcon) {
				SendMessage(GetDlgItem(pcli->hwndContactList, IDC_TBGLOBALSTATUS), BM_SETIMLICON, (WPARAM) hCListImages, (LPARAM) iIcon);
                if(g_ButtonItems == NULL)
                    SendMessage(GetDlgItem(pcli->hwndContactList, IDC_TBTOPSTATUS), BM_SETIMLICON, (WPARAM) hCListImages, (LPARAM) iIcon);
			}
			else {
				SendMessage(GetDlgItem(pcli->hwndContactList, IDC_TBGLOBALSTATUS), BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
                if(g_ButtonItems == NULL)
                    SendMessage(GetDlgItem(pcli->hwndContactList, IDC_TBTOPSTATUS), BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
			}
			InvalidateRect(GetDlgItem(pcli->hwndContactList, IDC_TBGLOBALSTATUS), NULL, TRUE);
			InvalidateRect(GetDlgItem(pcli->hwndContactList, IDC_TBTOPSTATUS), NULL, TRUE);
			SFL_Update(hIcon, iIcon, hCListImages, szStatus, TRUE);
	}	}
    return;
}
