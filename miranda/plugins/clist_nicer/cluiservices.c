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

extern HWND hwndContactTree, hwndContactList, hwndStatus;
extern HIMAGELIST hCListImages, himlExtraImages;;
extern struct CluiData g_CluiData;
extern int g_shutDown;
extern PLUGININFO pluginInfo;
extern HANDLE hStatusModeChangeEvent;
extern int currentDesiredStatusMode;

extern protoMenu *protoMenus;

int g_isConnecting = 0;

static int GetHwndTree(WPARAM wParam,LPARAM lParam)
{
	return (int)hwndContactTree;
}

static int GetClistVersion(WPARAM wParam, LPARAM lParam)
{
    static char g_szVersionString[256];

    mir_snprintf(g_szVersionString, 256, "%s, %d.%d.%d.%d", pluginInfo.shortName, HIBYTE(HIWORD(pluginInfo.version)), LOBYTE(HIWORD(pluginInfo.version)), HIBYTE(LOWORD(pluginInfo.version)), LOBYTE(LOBYTE(pluginInfo.version)));
    if(!IsBadWritePtr((LPVOID)lParam, 4))
        *((DWORD *)lParam) = pluginInfo.version;
    
    return (int)g_szVersionString;
}

static int GetHwnd(WPARAM wParam, LPARAM lParam)
{
    return(int) hwndContactList;
}

static char *xStatusNames_ansi[] = { ("Angry"), ("Duck"), ("Tired"), ("Party"), ("Beer"), ("Thinking"), ("Eating"), ("TV"), ("Friends"), ("Coffee"),
                         ("Music"), ("Business"), ("Camera"), ("Funny"), ("Phone"), ("Games"), ("College"), ("Shopping"), ("Sick"), ("Sleeping"),
                         ("Surfing"), ("@Internet"), ("Engineering"), ("Typing")};

int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam)
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
    
    if (hwndStatus == 0 || g_shutDown) 
        return 0;


    CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&protoCount, (LPARAM)&proto);
    if (protoCount == 0) 
        return 0;

    storedcount=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
    if (storedcount==-1)
        return 0;

    {
        //free protocol data
        int nPanel;
        int nParts = SendMessage(hwndStatus, SB_GETPARTS, 0, 0);
        for (nPanel = 0; nPanel < nParts; nPanel++) {
            ProtocolData *PD;
            PD = (ProtocolData *)SendMessageA(hwndStatus, SB_GETTEXTA, (WPARAM)nPanel, (LPARAM)0);
            if(PD != NULL && !IsBadCodePtr((void *)PD)) {
                SendMessageA(hwndStatus, SB_SETTEXTA, (WPARAM)nPanel|SBT_OWNERDRAW, (LPARAM)0);
                if(!IsBadCodePtr((FARPROC)PD->RealName)) mir_free(PD->RealName);
                if(PD) mir_free(PD);
            }
        }
    }

    SendMessage(hwndStatus,SB_GETBORDERS,0,(LPARAM)&borders); 

    partWidths=(int*)malloc((storedcount+1)*sizeof(int));

    if (g_CluiData.bEqualSections) {
        RECT rc;
        int part;
        SendMessage(hwndStatus,WM_SIZE,0,0);
        GetClientRect(hwndStatus,&rc);
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
        char *modeDescr;
        HDC hdc;
        SIZE textSize;
        BYTE showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",1);
        int x;
        char szName[32];
        HFONT hofont;

        hdc=GetDC(NULL);
        hofont=SelectObject(hdc,(HFONT)SendMessage(hwndStatus,WM_GETFONT,0,0));

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
                x += textSize.cx;
            }
            if (showOpts & 4) {
                modeDescr=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,CallProtoService(curprotocol->szName,PS_GETSTATUS,0,0),0);
                GetTextExtentPoint32A(hdc,modeDescr,lstrlenA(modeDescr),&textSize);
                x+=textSize.cx;
            }
            partWidths[partCount]=(partCount?partWidths[partCount-1]:g_CluiData.bCLeft)+ x + 2;
            partCount++;
        }
        SelectObject(hdc,hofont);
        ReleaseDC(NULL,hdc);
    }
    if (partCount==0) {
        SendMessage(hwndStatus,SB_SIMPLE,TRUE,0);
        free(partWidths);
        return 0;
    }
    SendMessage(hwndStatus,SB_SIMPLE,FALSE,0);

    partWidths[partCount-1]=-1;
    windowStyle = DBGetContactSettingByte(NULL, "CLUI", "WindowStyle", 0);
    SendMessage(hwndStatus,SB_SETMINHEIGHT, 18 + g_CluiData.bClipBorder + ((windowStyle == SETTING_WINDOWSTYLE_THINBORDER || windowStyle == SETTING_WINDOWSTYLE_NOBORDER) ? 3 : 0), 0);
    SendMessage(hwndStatus, SB_SETPARTS, partCount, (LPARAM)partWidths);
    free(partWidths);

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
            SendMessageA(hwndStatus,SB_SETTEXTA,partCount|flags,(LPARAM)PD);
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
        if(protoMenus != NULL) {
            int i;

            for(i = 0; protoMenus[i].protoName[0]; i++) {
                if(!strcmp(protoMenus[i].protoName, curprotocol->szName) && protoMenus[i].menuID != 0 && protoMenus[i].added != 0) {
                    BYTE xStatus = DBGetContactSettingByte(NULL, curprotocol->szName, "XStatusId", -1);
                    CLISTMENUITEM mi = {0};
                    mi.cbSize = sizeof(mi);
                    mi.flags = CMIM_FLAGS|CMIM_NAME|CMIM_ICON;

                    if(protoMenus[i].hIcon)
                        DestroyIcon(protoMenus[i].hIcon);
                    
                    if(xStatus > 0 && xStatus <= 24) {
                        mi.hIcon = ImageList_ExtractIcon(0, himlExtraImages, (int)xStatus + 3);
                        protoMenus[i].hIcon = mi.hIcon;
                        mi.pszName = xStatusNames_ansi[xStatus - 1];
                    }
                    else {
                        mi.pszName = "None";
                        mi.flags |= CMIF_CHECKED;
                        mi.hIcon = 0;
                        protoMenus[i].hIcon = 0;
                    }
                    CallService(MS_CLIST_MODIFYMENUITEM, protoMenus[i].menuID, (LPARAM)&mi);
                    break;
                }
            }
        }
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
    }
    /*
     * this is used globally (actually, by the clist control only) to determine if
     * any protocol is "in connection" state. If true, then the clist discards redraws
     * and uses timer based sort and redraw handling. This can improve performance
     * when connecting multiple protocols significantly.
     */
    g_isConnecting = (wStatus >= ID_STATUS_CONNECTING && wStatus < ID_STATUS_OFFLINE);
    szStatus = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM) wStatus, 0);

	/*
	 * set the global status icon and display the global (most online) status mode on the
	 * status mode button
	 */

	if (szStatus) {
        if(hwndContactList && IsWindow(GetDlgItem(hwndContactList, IDC_TBGLOBALSTATUS)) && IsWindow(GetDlgItem(hwndContactList, IDC_TBTOPSTATUS))) {
            SendMessageA(GetDlgItem(hwndContactList, IDC_TBGLOBALSTATUS), WM_SETTEXT, 0, (LPARAM) szStatus);
            if(!hIcon) {
                SendMessage(GetDlgItem(hwndContactList, IDC_TBGLOBALSTATUS), BM_SETIMLICON, (WPARAM) hCListImages, (LPARAM) iIcon);
                SendMessage(GetDlgItem(hwndContactList, IDC_TBTOPSTATUS), BM_SETIMLICON, (WPARAM) hCListImages, (LPARAM) iIcon);
            }
            else {
                SendMessage(GetDlgItem(hwndContactList, IDC_TBGLOBALSTATUS), BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
                SendMessage(GetDlgItem(hwndContactList, IDC_TBTOPSTATUS), BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
            }
            InvalidateRect(GetDlgItem(hwndContactList, IDC_TBGLOBALSTATUS), NULL, TRUE);
            InvalidateRect(GetDlgItem(hwndContactList, IDC_TBTOPSTATUS), NULL, TRUE);
			SFL_Update(hIcon, iIcon, hCListImages, szStatus, TRUE);
        }
    }
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

        GetClassName(hwndFocus, szFocusClass, sizeof(szFocusClass));
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
    if ((DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT) == 0) != ((GetWindowLong(hwndContactTree, GWL_STYLE) & CLS_HIDEOFFLINE) == 0)) {
        if (DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT))
            SetWindowLong(hwndContactTree, GWL_STYLE, GetWindowLong(hwndContactTree, GWL_STYLE) | CLS_HIDEOFFLINE);
        else
            SetWindowLong(hwndContactTree, GWL_STYLE, GetWindowLong(hwndContactTree, GWL_STYLE) & ~CLS_HIDEOFFLINE);
        rebuild = 1;
    }
    if ((DBGetContactSettingByte(NULL, "CList", "HideEmptyGroups", SETTING_HIDEEMPTYGROUPS_DEFAULT) == 0) != ((GetWindowLong(hwndContactTree, GWL_STYLE) & CLS_HIDEEMPTYGROUPS) == 0)) {
        if (DBGetContactSettingByte(NULL, "CList", "HideEmptyGroups", SETTING_HIDEEMPTYGROUPS_DEFAULT))
            SetWindowLong(hwndContactTree, GWL_STYLE, GetWindowLong(hwndContactTree, GWL_STYLE) | CLS_HIDEEMPTYGROUPS);
        else
            SetWindowLong(hwndContactTree, GWL_STYLE, GetWindowLong(hwndContactTree, GWL_STYLE) & ~CLS_HIDEEMPTYGROUPS);
        rebuild = 1;
    }
    if ((DBGetContactSettingByte(NULL, "CList", "UseGroups", SETTING_USEGROUPS_DEFAULT) == 0) != ((GetWindowLong(hwndContactTree, GWL_STYLE) & CLS_USEGROUPS) == 0)) {
        if (DBGetContactSettingByte(NULL, "CList", "UseGroups", SETTING_USEGROUPS_DEFAULT))
            SetWindowLong(hwndContactTree, GWL_STYLE, GetWindowLong(hwndContactTree, GWL_STYLE) | CLS_USEGROUPS);
        else
            SetWindowLong(hwndContactTree, GWL_STYLE, GetWindowLong(hwndContactTree, GWL_STYLE) & ~CLS_USEGROUPS);
        rebuild = 1;
    }
    if (rebuild)
        SendMessage(hwndContactTree, CLM_AUTOREBUILD, 0, 0);
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
    CreateServiceFunction(MS_CLUI_GETVERSION, GetClistVersion);
    return 0;
}