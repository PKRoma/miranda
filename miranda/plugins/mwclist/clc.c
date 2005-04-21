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
#include "clc.h"
#include "clist.h"

static LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HIMAGELIST himlCListClc;
static HANDLE hClcWindowList;
static HANDLE hShowInfoTipEvent;
HANDLE hHideInfoTipEvent;
static HANDLE hAckHook;

static HANDLE hSettingChanged1;
static HANDLE hSettingChanged2;
extern void InitDisplayNameCache(SortedList *list);
extern pdisplayNameCacheEntry GetDisplayNameCacheEntry(HANDLE hContact);

extern int BgStatusBarChange(WPARAM wParam,LPARAM lParam);

extern int BgClcChange(WPARAM wParam,LPARAM lParam);


int hClcProtoCount = 0;
ClcProtoStatus *clcProto = NULL;
extern int sortByStatus;
struct ClcContact * hitcontact=NULL;

static int stopStatusUpdater = 0;
void StatusUpdaterThread(HWND hwndDlg)
{
	int i,curdelay,lastcheck=0;
	char szServiceName[256];
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST);
		
	while (!stopStatusUpdater) {
	
	curdelay=DBGetContactSettingByte(hContact,"CList","StatusMsgAutoDelay",15000);
	if (curdelay<5000) curdelay=5000;
	
	if ((int)(GetTickCount()-lastcheck)>curdelay)
	{
		lastcheck=GetTickCount();
		if (DBGetContactSettingByte(hContact,"CList","StatusMsgAuto",0)) {
				for (i=0; i<5; i++) {
					if (hContact!=NULL) {
					pdisplayNameCacheEntry pdnce =(pdisplayNameCacheEntry)GetDisplayNameCacheEntry((HANDLE)hContact);
						if (pdnce!=NULL&&pdnce->protoNotExists==FALSE&&pdnce->szProto!=NULL)
						{			
							char *szProto =pdnce->szProto; //(char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
							_snprintf(szServiceName, sizeof(szServiceName), "%s%s", szProto, PSS_GETAWAYMSG);
							if (ServiceExists(szServiceName)) {
								strncpy(szServiceName, PSS_GETAWAYMSG, sizeof(szServiceName));
								CallContactService(hContact, szServiceName, 0, 0);
							}
						};
						hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
					}
					if (hContact==NULL) {
						hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
						if (hContact==NULL) break;
					}
				Sleep(500);
				}
			}
	};
	//Sleep(DBGetContactSettingByte(hContact,"CList","StatusMsgAutoDelay",100));
	Sleep(200);
	}
}



int GetProtocolVisibility(char * ProtoName)
{
    int i;
    int res=0;
    DBVARIANT dbv;
    char buf2[10];
    int count;
	count=(int)DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
	if (count==-1) return (1);
    for (i=0; i<count; i++)
    {
        itoa(i,buf2,10);
        if (!DBGetContactSetting(NULL,"Protocols",buf2,&dbv))
        {
            if (strcmp(ProtoName,dbv.pszVal)==0)
            {
                mir_free(dbv.pszVal);
                itoa(i+400,buf2,10);
                res= DBGetContactSettingDword(NULL,"Protocols",buf2,0);
                return res;
            }
            mir_free(dbv.pszVal);
        }
    }
    return 0;
}

void ClcOptionsChanged(void)
{
	WindowList_Broadcast(hClcWindowList,INTM_RELOADOPTIONS,0,0);
}
void SortClcByTimer (HWND hwnd)
{
	KillTimer(hwnd,TIMERID_DELAYEDRESORTCLC);
	SetTimer(hwnd,TIMERID_DELAYEDRESORTCLC,DBGetContactSettingByte(NULL,"CLUI","DELAYEDTIMER",200),NULL);
}

static int ClcSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;

	if((HANDLE)wParam!=NULL&&!strcmp(cws->szModule,"MetaContacts")&&!strcmp(cws->szSetting,"Handle"))
	{
		WindowList_Broadcast(hClcWindowList,INTM_NAMEORDERCHANGED,0,0);	
	};
	
	if((HANDLE)wParam!=NULL&&!strcmp(cws->szModule,"CList")) {
		if((cws->value.type==DBVT_ASCIIZ||cws->value.type==DBVT_DELETED)&&!strcmp(cws->szSetting,"MyHandle"))
			WindowList_Broadcast(hClcWindowList,INTM_NAMECHANGED,wParam,lParam);
		else if(cws->value.type==DBVT_ASCIIZ&&!strcmp(cws->szSetting,"Group"))
			WindowList_Broadcast(hClcWindowList,INTM_GROUPCHANGED,wParam,lParam);
		else if(!strcmp(cws->szSetting,"Hidden"))
			WindowList_Broadcast(hClcWindowList,INTM_HIDDENCHANGED,wParam,lParam);
		else if(!strcmp(cws->szSetting,"noOffline"))
			WindowList_Broadcast(hClcWindowList,INTM_NAMEORDERCHANGED,wParam,lParam);
		else if(cws->value.type==DBVT_BYTE&&!strcmp(cws->szSetting,"NotOnList"))
			WindowList_Broadcast(hClcWindowList,INTM_NOTONLISTCHANGED,wParam,lParam);
		else if(cws->value.type==DBVT_WORD&&!strcmp(cws->szSetting,"Status"))
			WindowList_Broadcast(hClcWindowList,INTM_INVALIDATE,0,0);
		else if(cws->value.type==DBVT_BLOB&&!strcmp(cws->szSetting,"NameOrder"))
			WindowList_Broadcast(hClcWindowList,INTM_NAMEORDERCHANGED,0,0);
		else if(!strcmp(cws->szSetting,"StatusMsg")) 
			WindowList_Broadcast(hClcWindowList,INTM_STATUSMSGCHANGED,wParam,lParam);

	}
	else if(!strcmp(cws->szModule,"CListGroups")) {
		WindowList_Broadcast(hClcWindowList,INTM_GROUPSCHANGED,wParam,lParam);
	}
	else {
		if ((HANDLE)wParam!=NULL)
		{
		pdisplayNameCacheEntry pdnce =(pdisplayNameCacheEntry)GetDisplayNameCacheEntry((HANDLE)wParam);
			if (pdnce!=NULL)
			{					
					if(pdnce->szProto==NULL || strcmp(pdnce->szProto,cws->szModule)) return 0;

					if(cws->value.type==DBVT_DWORD&&(!strcmp(cws->szSetting,"UIN")) )
						WindowList_Broadcast(hClcWindowList,INTM_NAMECHANGED,wParam,lParam);
					else if(cws->value.type==DBVT_ASCIIZ&&(!strcmp(cws->szSetting,"Nick") || !strcmp(cws->szSetting,"FirstName") || !strcmp(cws->szSetting,"e-mail") || !strcmp(cws->szSetting,"LastName") || !strcmp(cws->szSetting,"JID")) )
						WindowList_Broadcast(hClcWindowList,INTM_NAMECHANGED,wParam,lParam);
					else if(!strcmp(cws->szSetting,"ApparentMode"))
						WindowList_Broadcast(hClcWindowList,INTM_APPARENTMODECHANGED,wParam,lParam);
					else if(!strcmp(cws->szSetting,"IdleTS"))
						WindowList_Broadcast(hClcWindowList,INTM_IDLECHANGED,wParam,lParam);
			};
		};
	}
	return 0;
}


static int ClcModulesLoaded(WPARAM wParam,LPARAM lParam) {
	PROTOCOLDESCRIPTOR **proto;
	int protoCount,i;

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	for(i=0;i<protoCount;i++) {
		if(proto[i]->type!=PROTOTYPE_PROTOCOL) continue;
		clcProto=(ClcProtoStatus*)mir_realloc(clcProto,sizeof(ClcProtoStatus)*(hClcProtoCount+1));
		clcProto[hClcProtoCount].szProto = proto[i]->szName;
		clcProto[hClcProtoCount].dwStatus = ID_STATUS_OFFLINE;
		hClcProtoCount++;
	}
	CallService(MS_BACKGROUNDCONFIG_REGISTER,(WPARAM)"StatusBar Background/StatusBar",0);
	CallService(MS_BACKGROUNDCONFIG_REGISTER,(WPARAM)"List Background/CLC",0);
	HookEvent(ME_BACKGROUNDCONFIG_CHANGED,BgClcChange);
	HookEvent(ME_BACKGROUNDCONFIG_CHANGED,BgStatusBarChange);
	
	
	
	
	return 0;
}

static int ClcProtoAck(WPARAM wParam,LPARAM lParam)
{
	ACKDATA *ack=(ACKDATA*)lParam;
	int i;

	if(ack->type==ACKTYPE_STATUS) {
		WindowList_BroadcastAsync(hClcWindowList,INTM_INVALIDATE,0,0);
		if (ack->result==ACKRESULT_SUCCESS) {
			for (i=0;i<hClcProtoCount;i++) {
				if (!lstrcmp(clcProto[i].szProto,ack->szModule)) {
					clcProto[i].dwStatus = (WORD)ack->lParam;
					break;
				}
			}
		}
	}
	return 0;
}

static int ClcContactAdded(WPARAM wParam,LPARAM lParam)
{
	WindowList_BroadcastAsync(hClcWindowList,INTM_CONTACTADDED,wParam,lParam);
	return 0;
}

static int ClcContactDeleted(WPARAM wParam,LPARAM lParam)
{
	WindowList_BroadcastAsync(hClcWindowList,INTM_CONTACTDELETED,wParam,lParam);
	return 0;
}

static int ClcContactIconChanged(WPARAM wParam,LPARAM lParam)
{
	WindowList_BroadcastAsync(hClcWindowList,INTM_ICONCHANGED,wParam,lParam);
	return 0;
}

static int ClcIconsChanged(WPARAM wParam,LPARAM lParam)
{
	WindowList_BroadcastAsync(hClcWindowList,INTM_INVALIDATE,0,0);
	return 0;
}

static int SetInfoTipHoverTime(WPARAM wParam,LPARAM lParam)
{
	DBWriteContactSettingWord(NULL,"CLC","InfoTipHoverTime",(WORD)wParam);
	WindowList_Broadcast(hClcWindowList,INTM_SETINFOTIPHOVERTIME,wParam,0);
	return 0;
}

static int GetInfoTipHoverTime(WPARAM wParam,LPARAM lParam)
{
	return DBGetContactSettingWord(NULL,"CLC","InfoTipHoverTime",750);
}

static int ClcShutdown(WPARAM wParam,LPARAM lParam)
{
	UnhookEvent(hAckHook);
	UnhookEvent(hSettingChanged1);
	mir_free(clcProto);
	FreeFileDropping();
	return 0;
}

int LoadCLCModule(void)
{
	WNDCLASS wndclass;

	himlCListClc=(HIMAGELIST)CallService(MS_CLIST_GETICONSIMAGELIST,0,0);
	hClcWindowList=(HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST,0,0);
	hShowInfoTipEvent=CreateHookableEvent(ME_CLC_SHOWINFOTIP);
	hHideInfoTipEvent=CreateHookableEvent(ME_CLC_HIDEINFOTIP);
	CreateServiceFunction(MS_CLC_SETINFOTIPHOVERTIME,SetInfoTipHoverTime);
	CreateServiceFunction(MS_CLC_GETINFOTIPHOVERTIME,GetInfoTipHoverTime);

    wndclass.style         = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS|CS_GLOBALCLASS;
    wndclass.lpfnWndProc   = ContactListControlWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(void*);
    wndclass.hInstance     = g_hInst;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = CLISTCONTROL_CLASS;
	RegisterClass(&wndclass);

	InitFileDropping();
	
	HookEvent(ME_SYSTEM_MODULESLOADED,ClcModulesLoaded);
	hSettingChanged1=HookEvent(ME_DB_CONTACT_SETTINGCHANGED,ClcSettingChanged);
	HookEvent(ME_DB_CONTACT_ADDED,ClcContactAdded);
	HookEvent(ME_DB_CONTACT_DELETED,ClcContactDeleted);
	HookEvent(ME_CLIST_CONTACTICONCHANGED,ClcContactIconChanged);
	HookEvent(ME_SKIN_ICONSCHANGED,ClcIconsChanged);
	HookEvent(ME_OPT_INITIALISE,ClcOptInit);
	hAckHook=(HANDLE)HookEvent(ME_PROTO_ACK,ClcProtoAck);
	HookEvent(ME_SYSTEM_SHUTDOWN,ClcShutdown);



	return 0;
}

static LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{     
	struct ClcData *dat;

	dat=(struct ClcData*)GetWindowLong(hwnd,0);
	if(msg>=CLM_FIRST && msg<CLM_LAST) return ProcessExternalMessages(hwnd,dat,msg,wParam,lParam);
    switch (msg)
    {
		case WM_CREATE:
			WindowList_Add(hClcWindowList,hwnd,NULL);
			RegisterFileDropping(hwnd);
			dat=(struct ClcData*)mir_alloc(sizeof(struct ClcData));
			SetWindowLong(hwnd,0,(LONG)dat);

			{	int i;
				for(i=0;i<=FONTID_MAX;i++) dat->fontInfo[i].changed=1;
			}
			dat->yScroll=0;
			dat->selection=-1;
			dat->iconXSpace=20;
			dat->checkboxSize=13;
			dat->dragAutoScrollHeight=30;
			dat->himlHighlight=NULL;
			dat->hBmpBackground=NULL;
			dat->hwndRenameEdit=NULL;
			dat->iDragItem=-1;
			dat->iInsertionMark=-1;
			dat->insertionMarkHitHeight=5;
			dat->szQuickSearch[0]=0;
			dat->bkChanged=0;
			dat->iHotTrack=-1;
			dat->infoTipTimeout=DBGetContactSettingWord(NULL,"CLC","InfoTipHoverTime",750);
			dat->hInfoTipItem=NULL;
			dat->himlExtraColumns=NULL;
			dat->extraColumnsCount=0;
			dat->extraColumnSpacing=20;
			dat->showSelAlways=0;
			dat->list.contactCount=0;
			dat->list.allocedCount=0;
			dat->list.contact=NULL;
			dat->list.parent=NULL;
			dat->list.hideOffline=0;
			dat->NeedResort=1;
			dat->MetaIgnoreEmptyExtra=DBGetContactSettingByte(NULL,"CLC","MetaIgnoreEmptyExtra",1);
			//dat->lCLCContactsCache=
			InitDisplayNameCache(&dat->lCLCContactsCache);

			LoadClcOptions(hwnd,dat);
			if (!IsWindowVisible(hwnd)) 
			{
				SetTimer(hwnd,TIMERID_REBUILDAFTER,10,NULL);
			}else
			{
				RebuildEntireList(hwnd,dat);
			}
			

			{	NMCLISTCONTROL nm;
				nm.hdr.code=CLN_LISTREBUILT;
				nm.hdr.hwndFrom=hwnd;
				nm.hdr.idFrom=GetDlgCtrlID(hwnd);
				SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
			}
			OutputDebugString("Create New ClistControl END\r\n");
			forkthread(StatusUpdaterThread,0,0);

			break;

		case INTM_RELOADOPTIONS:
			LoadClcOptions(hwnd,dat);
			SaveStateAndRebuildList(hwnd,dat);
			break;
		case WM_THEMECHANGED:
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		case INTM_SCROLLBARCHANGED:
		{
			if ( GetWindowLong(hwnd,GWL_STYLE)&CLS_CONTACTLIST ) {
				if ( dat->noVScrollbar ) ShowScrollBar(hwnd,SB_VERT,FALSE);
				else RecalcScrollBar(hwnd,dat);
			}
			break;
		}

		case WM_SIZE:
			EndRename(hwnd,dat,1);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			RecalcScrollBar(hwnd,dat);
			{	//creating imagelist containing blue line for highlight
				HBITMAP hBmp,hBmpMask,hoBmp;
				HDC hdc,hdcMem;
				RECT rc;
				int depth;
				HBRUSH hBrush;

				GetClientRect(hwnd,&rc);
				if(rc.right==0) break;
				rc.bottom=dat->rowHeight;
				hdc=GetDC(hwnd);
				depth=GetDeviceCaps(hdc,BITSPIXEL);
				if(depth<16) depth=16;
				hBmp=CreateBitmap(rc.right,rc.bottom,1,depth,NULL);
				hBmpMask=CreateBitmap(rc.right,rc.bottom,1,1,NULL);
				hdcMem=CreateCompatibleDC(hdc);
				hoBmp=(HBITMAP)SelectObject(hdcMem,hBmp);
				hBrush=CreateSolidBrush(dat->selBkColour);
				FillRect(hdcMem,&rc,hBrush);
				DeleteObject(hBrush);

				SelectObject(hdcMem,hBmpMask);
				FillRect(hdcMem,&rc,GetStockObject(BLACK_BRUSH));
				SelectObject(hdcMem,hoBmp);
				DeleteDC(hdcMem);
				ReleaseDC(hwnd,hdc);
				if(dat->himlHighlight)
					ImageList_Destroy(dat->himlHighlight);
				dat->himlHighlight=ImageList_Create(rc.right,rc.bottom,ILC_COLOR32|ILC_MASK,1,1);
				ImageList_Add(dat->himlHighlight,hBmp,hBmpMask);
				DeleteObject(hBmpMask);
				DeleteObject(hBmp);
			}			
			break;

		case WM_SYSCOLORCHANGE:
			SendMessage(hwnd,WM_SIZE,0,0);
			break;

		case WM_GETDLGCODE:
			if(lParam) {
				MSG *msg=(MSG*)lParam;
				if(msg->message==WM_KEYDOWN) {
					if(msg->wParam==VK_TAB) return 0;
					if(msg->wParam==VK_ESCAPE && dat->hwndRenameEdit==NULL && dat->szQuickSearch[0]==0) return 0;
				}
				if(msg->message==WM_CHAR) {
					if(msg->wParam=='\t') return 0;
					if(msg->wParam==27 && dat->hwndRenameEdit==NULL && dat->szQuickSearch[0]==0) return 0;
				}
			}
			return DLGC_WANTMESSAGE;

		case WM_KILLFOCUS:
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
		case WM_SETFOCUS:
		case WM_ENABLE:
			InvalidateRect(hwnd,NULL,FALSE);
			break;

		case WM_GETFONT:
			return (LRESULT)dat->fontInfo[FONTID_CONTACTS].hFont;

		case INTM_GROUPSCHANGED:
		{	DBCONTACTWRITESETTING *dbcws=(DBCONTACTWRITESETTING*)lParam;
			if(dbcws->value.type==DBVT_ASCIIZ) {
				int groupId=atoi(dbcws->szSetting)+1;
				struct ClcContact *contact;
				struct ClcGroup *group;
				char szFullName[512];
				int i,nameLen;
				//check name of group and ignore message if just being expanded/collapsed
				if(FindItem(hwnd,dat,(HANDLE)(groupId|HCONTACT_ISGROUP),&contact,&group,NULL)) {
					lstrcpy(szFullName,contact->szText);
					while(group->parent) {
						for(i=0;i<group->parent->contactCount;i++)
							if(group->parent->contact[i].group==group) break;
						if(i==group->parent->contactCount) {szFullName[0]='\0'; break;}
						group=group->parent;
						nameLen=lstrlen(group->contact[i].szText);
						if(lstrlen(szFullName)+1+nameLen>sizeof(szFullName)) {szFullName[0]='\0'; break;}
						memmove(szFullName+1+nameLen,szFullName,lstrlen(szFullName)+1);
						memcpy(szFullName,group->contact[i].szText,nameLen);
						szFullName[nameLen]='\\';
					}
					if(!lstrcmp(szFullName,dbcws->value.pszVal+1) && (contact->group->hideOffline!=0)==((dbcws->value.pszVal[0]&GROUPF_HIDEOFFLINE)!=0))
						break;  //only expanded has changed: no action reqd
				}
			}
			//SaveStateAndRebuildList(hwnd,dat);
			SetTimer(hwnd,TIMERID_REBUILDAFTER,1,NULL);
			break;
		}

		case INTM_NAMEORDERCHANGED:
			PostMessage(hwnd,CLM_AUTOREBUILD,0,0);
			break;

		case INTM_CONTACTADDED:
			AddContactToTree(hwnd,dat,(HANDLE)wParam,1,1);
			NotifyNewContact(hwnd,(HANDLE)wParam);
			//RecalcScrollBar(hwnd,dat);
			//SortCLC(hwnd,dat,1);
			SortClcByTimer(hwnd);
			break;

		case INTM_CONTACTDELETED:
			DeleteItemFromTree(hwnd,(HANDLE)wParam);
			//SortCLC(hwnd,dat,1);
			SortClcByTimer(hwnd);
			//RecalcScrollBar(hwnd,dat);
			break;

		case INTM_HIDDENCHANGED:
		{	DBCONTACTWRITESETTING *dbcws=(DBCONTACTWRITESETTING*)lParam;
			if (lParam!=0)
			{
			
			if(GetWindowLong(hwnd,GWL_STYLE)&CLS_SHOWHIDDEN) break;
			if(dbcws->value.type==DBVT_DELETED || dbcws->value.bVal==0) {
				if(FindItem(hwnd,dat,(HANDLE)wParam,NULL,NULL,NULL)) break;
				AddContactToTree(hwnd,dat,(HANDLE)wParam,1,1);
				NotifyNewContact(hwnd,(HANDLE)wParam);
			}
			else
				DeleteItemFromTree(hwnd,(HANDLE)wParam);
			};
			dat->NeedResort=1;
			//SortCLC(hwnd,dat,1);
			SortClcByTimer(hwnd);
			//RecalcScrollBar(hwnd,dat);
			break;
		}

		case INTM_GROUPCHANGED:
		{	struct ClcContact *contact;
			BYTE iExtraImage[MAXEXTRACOLUMNS];
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL))
				memset(iExtraImage,0xFF,sizeof(iExtraImage));
			else CopyMemory(iExtraImage,contact->iExtraImage,sizeof(iExtraImage));
			DeleteItemFromTree(hwnd,(HANDLE)wParam);
			if(GetWindowLong(hwnd,GWL_STYLE)&CLS_SHOWHIDDEN || !DBGetContactSettingByte((HANDLE)wParam,"CList","Hidden",0)) {
				NMCLISTCONTROL nm;
				AddContactToTree(hwnd,dat,(HANDLE)wParam,1,1);
				if(FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL))
					CopyMemory(contact->iExtraImage,iExtraImage,sizeof(iExtraImage));
				nm.hdr.code=CLN_CONTACTMOVED;
				nm.hdr.hwndFrom=hwnd;
				nm.hdr.idFrom=GetDlgCtrlID(hwnd);
				nm.flags=0;
				nm.hItem=(HANDLE)wParam;
				SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
				dat->NeedResort=1;
			}
			//SortCLC(hwnd,dat,1);
			SortClcByTimer(hwnd);
			//RecalcScrollBar(hwnd,dat);
			break;
		}

		case INTM_ICONCHANGED:
		{	struct ClcContact *contact=NULL;
			struct ClcGroup *group=NULL;
			int recalcScrollBar=0,shouldShow;
			pdisplayNameCacheEntry cacheEntry;

			WORD status;
			char *szProto;
			int NeedResort=0;

			cacheEntry=GetContactFullCacheEntry((HANDLE)wParam);

			//szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
			//szProto=GetContactCachedProtocol((HANDLE)wParam);
			szProto=cacheEntry->szProto;
			if(szProto==NULL) status=ID_STATUS_OFFLINE;
			else status=cacheEntry->status;
			
			shouldShow=(GetWindowLong(hwnd,GWL_STYLE)&CLS_SHOWHIDDEN
			            || !cacheEntry->Hidden)
					&& (!IsHiddenMode(dat,status)||cacheEntry->noHiddenOffline
					    || CallService(MS_CLIST_GETCONTACTICON,wParam,0)!=lParam);	//this means an offline msg is flashing, so the contact should be shown
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,&group,NULL)) {				
				if(shouldShow) {					
					AddContactToTree(hwnd,dat,(HANDLE)wParam,0,0);
					NeedResort=1;
					recalcScrollBar=1;					
					FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL);
					if (contact) {						
						contact->iImage=(WORD)lParam;
						NotifyNewContact(hwnd,(HANDLE)wParam);
						dat->NeedResort=1;
					}
				}				
			}
			else {	  //item in list already
				DWORD style=GetWindowLong(hwnd,GWL_STYLE);				
				if(contact->iImage==(WORD)lParam) break;				
				if (sortByStatus) dat->NeedResort=1;

				if(!shouldShow && !(style&CLS_NOHIDEOFFLINE) && (style&CLS_HIDEOFFLINE || group->hideOffline)) {
					HANDLE hSelItem;
					struct ClcContact *selcontact;
					struct ClcGroup *selgroup;
					if(GetRowByIndex(dat,dat->selection,&selcontact,NULL)==-1) hSelItem=NULL;
					else hSelItem=ContactToHItem(selcontact);
					RemoveItemFromGroup(hwnd,group,contact,0);
					if(hSelItem)
						if(FindItem(hwnd,dat,hSelItem,&selcontact,&selgroup,NULL))
							dat->selection=GetRowsPriorTo(&dat->list,selgroup,selcontact-selgroup->contact);
					recalcScrollBar=1;
					dat->NeedResort=1;
				}
				else {
					int oldflags;
					contact->iImage=(WORD)lParam;
					oldflags=contact->flags;
					if(!IsHiddenMode(dat,status)||cacheEntry->noHiddenOffline) contact->flags|=CONTACTF_ONLINE;
					else contact->flags&=~CONTACTF_ONLINE;
					if (oldflags!=contact->flags)
					{
						dat->NeedResort=1;
					}else
					{
						//OutputDebugString("No Status Change\r\n");	
					}

				}
			}
			//if (NeedResort) SortCLC(hwnd,dat,1);
			//is sorts list if only icon changed !!! very bad
			//SortCLC(hwnd,dat,1); 
			//SortContacts();
			SortClcByTimer(hwnd);
			if(recalcScrollBar) RecalcScrollBar(hwnd,dat);			
			break;
		}

		case INTM_NAMECHANGED:
		{	struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL)) break;
				lstrcpyn(contact->szText,(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,wParam,0),sizeof(contact->szText));
				dat->NeedResort=1;
				//SortCLC(hwnd,dat,1);		
				SortClcByTimer(hwnd);
			break;
		}

		case INTM_STATUSMSGCHANGED:
		{	struct ClcContact *contact=NULL;
			struct ClcGroup *group=NULL;
			DBVARIANT dbv;
			DWORD style=GetWindowLong(hwnd,GWL_STYLE);

			if (!(style&CLS_SHOWSTATUSMESSAGES)) break;
			if(FindItem(hwnd,dat,(HANDLE)wParam,&contact,&group,NULL) && contact!=NULL) {
				contact->flags &= ~CONTACTF_STATUSMSG;
				if (!DBGetContactSetting((HANDLE)wParam, "CList", "StatusMsg", &dbv)) {
					int j;
					if (dbv.pszVal==NULL||strlen(dbv.pszVal)==0) break;
					lstrcpyn(contact->szStatusMsg, dbv.pszVal, sizeof(contact->szStatusMsg));
					for (j=strlen(contact->szStatusMsg)-1;j>=0;j--) {
						if (contact->szStatusMsg[j]=='\r' || contact->szStatusMsg[j]=='\n' || contact->szStatusMsg[j]=='\t') {
							contact->szStatusMsg[j] = ' ';
						}
					}
					DBFreeVariant(&dbv);
					if (strlen(contact->szStatusMsg)>0) {
						contact->flags |= CONTACTF_STATUSMSG;
						dat->NeedResort=TRUE;
					}
				}
			}

			InvalidateRect(hwnd,NULL,FALSE);
			
			SortClcByTimer(hwnd);
			RecalcScrollBar(hwnd,dat);
			break;
		}

		case INTM_NOTONLISTCHANGED:
		{	DBCONTACTWRITESETTING *dbcws=(DBCONTACTWRITESETTING*)lParam;
			struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL)) break;
			if(contact->type!=CLCIT_CONTACT) break;
			if(dbcws->value.type==DBVT_DELETED || dbcws->value.bVal==0) contact->flags&=~CONTACTF_NOTONLIST;
			else contact->flags|=CONTACTF_NOTONLIST;
			InvalidateRect(hwnd,NULL,FALSE);
			break;
		}

		case INTM_INVALIDATE:
			InvalidateRect(hwnd,NULL,FALSE);
			break;

		case INTM_APPARENTMODECHANGED:
		{	WORD apparentMode;
			char *szProto;
			struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL)) break;
			szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
			if(szProto==NULL) break;
			apparentMode=DBGetContactSettingWord((HANDLE)wParam,szProto,"ApparentMode",0);
			contact->flags&=~(CONTACTF_INVISTO|CONTACTF_VISTO);
			if(apparentMode==ID_STATUS_OFFLINE)	contact->flags|=CONTACTF_INVISTO;
			else if(apparentMode==ID_STATUS_ONLINE) contact->flags|=CONTACTF_VISTO;
			else if(apparentMode) contact->flags|=CONTACTF_VISTO|CONTACTF_INVISTO;
			InvalidateRect(hwnd,NULL,FALSE);
			break;
		}

		case INTM_SETINFOTIPHOVERTIME:
			dat->infoTipTimeout=wParam;
			break;
		
		case INTM_IDLECHANGED:
		{
			char *szProto;
			struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL)) break;
			szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
			if(szProto==NULL) break;
			contact->flags&=~CONTACTF_IDLE;
			if (DBGetContactSettingDword((HANDLE)wParam,szProto,"IdleTS",0)) {
				contact->flags|=CONTACTF_IDLE;
			}
			InvalidateRect(hwnd,NULL,FALSE);
			break;
		}

		case WM_PRINTCLIENT:
			PaintClc(hwnd,dat,(HDC)wParam,NULL);
			break;

		case WM_NCPAINT:
			if(wParam==1) break;
			{	POINT ptTopLeft={0,0};
				HRGN hClientRgn;
				ClientToScreen(hwnd,&ptTopLeft);
				hClientRgn=CreateRectRgn(0,0,1,1);
				CombineRgn(hClientRgn,(HRGN)wParam,NULL,RGN_COPY);
				OffsetRgn(hClientRgn,-ptTopLeft.x,-ptTopLeft.y);
				InvalidateRgn(hwnd,hClientRgn,FALSE);
				DeleteObject(hClientRgn);
				UpdateWindow(hwnd);
			}
			break;

		case WM_PAINT:
		{	HDC hdc;
			PAINTSTRUCT ps;
			hdc=BeginPaint(hwnd,&ps);
			/* we get so many InvalidateRect()'s that there is no point painting,
			Windows in theory shouldn't queue up WM_PAINTs in this case but it does so
			we'll just ignore them */
			if (IsWindowVisible(hwnd)) PaintClc(hwnd,dat,hdc,&ps.rcPaint);
			EndPaint(hwnd,&ps);
			break;
		}

		case WM_VSCROLL:
		{	int desty;
			RECT clRect;
			int noSmooth=0;

			EndRename(hwnd,dat,1);
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			desty=dat->yScroll;
			GetClientRect(hwnd,&clRect);
			switch(LOWORD(wParam)) {
				case SB_LINEUP: desty-=dat->rowHeight; break;
				case SB_LINEDOWN: desty+=dat->rowHeight; break;
				case SB_PAGEUP: desty-=clRect.bottom-dat->rowHeight; break;
				case SB_PAGEDOWN: desty+=clRect.bottom-dat->rowHeight; break;
				case SB_BOTTOM: desty=0x7FFFFFFF; break;
				case SB_TOP: desty=0; break;
				case SB_THUMBTRACK: desty=HIWORD(wParam); noSmooth=1; break;    //noone has more than 4000 contacts, right?
				default: return 0;
			}
			ScrollTo(hwnd,dat,desty,noSmooth);
			break;
		}

		case WM_MOUSEWHEEL:
		{	UINT scrollLines;
			EndRename(hwnd,dat,1);
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			if(!SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0,&scrollLines,FALSE))
				scrollLines=3;
			ScrollTo(hwnd,dat,dat->yScroll-(short)HIWORD(wParam)*dat->rowHeight*(signed)scrollLines/WHEEL_DELTA,0);
			return 0;
		}

		case WM_KEYDOWN:
		{	int selMoved=0;
			int changeGroupExpand=0;
			int pageSize;
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			if(CallService(MS_CLIST_MENUPROCESSHOTKEY,wParam,MPCF_CONTACTMENU)) break;
			{	RECT clRect;
				GetClientRect(hwnd,&clRect);
				pageSize=clRect.bottom/dat->rowHeight;
			}
			switch(wParam) {
				case VK_DOWN: dat->selection = GetNewSelection(&dat->list, dat->selection+1, 1); selMoved=1; break;
				case VK_UP: dat->selection = GetNewSelection(&dat->list, dat->selection-1, 0); selMoved=1; break;
				case VK_PRIOR: dat->selection = GetNewSelection(&dat->list, dat->selection-pageSize, 0); selMoved=1; break;
				case VK_NEXT: dat->selection = GetNewSelection(&dat->list, dat->selection+pageSize, 1); selMoved=1; break;
//				case VK_DOWN: dat->selection++; selMoved=1; break;
//				case VK_UP: dat->selection--; selMoved=1; break;
//				case VK_PRIOR: dat->selection-=pageSize; selMoved=1; break;
//				case VK_NEXT: dat->selection+=pageSize; selMoved=1; break;
				case VK_HOME: dat->selection=0; selMoved=1; break;
				case VK_END: dat->selection=GetNewSelection(&dat->list, dat->selection+99999, 1); selMoved=1; break;
//				case VK_END: dat->selection=GetGroupContentsCount(&dat->list,1)-1; selMoved=1; break;
				case VK_LEFT: changeGroupExpand=1; break;
				case VK_RIGHT: changeGroupExpand=2; break;
				case VK_RETURN: DoSelectionDefaultAction(hwnd,dat); return 0;
				case VK_F2: BeginRenameSelection(hwnd,dat); return 0;
				case VK_DELETE: DeleteFromContactList(hwnd,dat); return 0;
				default:
				{	NMKEY nmkey;
					nmkey.hdr.hwndFrom=hwnd;
					nmkey.hdr.idFrom=GetDlgCtrlID(hwnd);
					nmkey.hdr.code=NM_KEYDOWN;
					nmkey.nVKey=wParam;
					nmkey.uFlags=HIWORD(lParam);
					if(SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nmkey)) return 0;
				}
			}
			if(changeGroupExpand) {
				int hit;
				struct ClcContact *contact;
				struct ClcGroup *group;
				dat->szQuickSearch[0]=0;
				hit=GetRowByIndex(dat,dat->selection,&contact,&group);
				if(hit!=-1) {
					if(changeGroupExpand==1 && contact->type==CLCIT_CONTACT) {
						if(group==&dat->list) return 0;
						dat->selection=GetRowsPriorTo(&dat->list,group,-1);
						selMoved=1;
					}
					else {
						if(contact->type==CLCIT_GROUP)
							SetGroupExpand(hwnd,dat,contact->group,changeGroupExpand==2);
						return 0;
					}
				}
				else return 0; 
			}
			if(selMoved) {
				dat->szQuickSearch[0]=0;
				if(dat->selection>=GetGroupContentsCount(&dat->list,1))
					dat->selection=GetGroupContentsCount(&dat->list,1)-1;
				if(dat->selection<0) dat->selection=0;
				InvalidateRect(hwnd,NULL,FALSE);
				EnsureVisible(hwnd,dat,dat->selection,0);
				UpdateWindow(hwnd);
				return 0;
			}
			break;
		}

		case WM_CHAR:
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			if(wParam==27)   //escape
				dat->szQuickSearch[0]=0;
			else if(wParam=='\b' && dat->szQuickSearch[0])
				dat->szQuickSearch[lstrlen(dat->szQuickSearch)-1]='\0';
			else if(wParam<' ') break;
			else if(wParam==' ' && dat->szQuickSearch[0]=='\0' && GetWindowLong(hwnd,GWL_STYLE)&CLS_CHECKBOXES) {
				struct ClcContact *contact;
				NMCLISTCONTROL nm;
				if(GetRowByIndex(dat,dat->selection,&contact,NULL)==-1) break;
				if(contact->type!=CLCIT_CONTACT) break;
				contact->flags^=CONTACTF_CHECKED;
				if(contact->type==CLCIT_GROUP) SetGroupChildCheckboxes(contact->group,contact->flags&CONTACTF_CHECKED);
				RecalculateGroupCheckboxes(hwnd,dat);
				InvalidateRect(hwnd,NULL,FALSE);
				nm.hdr.code=CLN_CHECKCHANGED;
				nm.hdr.hwndFrom=hwnd;
				nm.hdr.idFrom=GetDlgCtrlID(hwnd);
				nm.flags=0;
				nm.hItem=ContactToItemHandle(contact,&nm.flags);
				SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
			}
			else {
				char szNew[2];
				szNew[0]=(char)wParam; szNew[1]='\0';
				if(lstrlen(dat->szQuickSearch)>=sizeof(dat->szQuickSearch)-1) {
					MessageBeep(MB_OK);
					break;
				}
				strcat(dat->szQuickSearch,szNew);
			}
			if(dat->szQuickSearch[0]) {
				int index;
				index=FindRowByText(hwnd,dat,dat->szQuickSearch,1);
				if(index!=-1) dat->selection=index;
				else {
					MessageBeep(MB_OK);
					dat->szQuickSearch[lstrlen(dat->szQuickSearch)-1]='\0';
				}
				InvalidateRect(hwnd,NULL,FALSE);
				EnsureVisible(hwnd,dat,dat->selection,0);
			}
			else InvalidateRect(hwnd,NULL,FALSE);
			break;

		case WM_SYSKEYDOWN:
			EndRename(hwnd,dat,1);
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			dat->iHotTrack=-1;
			InvalidateRect(hwnd,NULL,FALSE);
			ReleaseCapture();
			if(wParam==VK_F10 && GetKeyState(VK_SHIFT)&0x8000) break;
			SendMessage(GetParent(hwnd),msg,wParam,lParam);
			return 0;

		case WM_TIMER:
			if (wParam==TIMERID_REBUILDAFTER)
			{
				KillTimer(hwnd,TIMERID_REBUILDAFTER);
#ifdef _DEBUG
OutputDebugString("Delayed REBUILDAFTER\r\n");
#endif
				InvalidateRect(hwnd,NULL,FALSE);
				SaveStateAndRebuildList(hwnd,dat);
				break;
			}
			if (wParam==TIMERID_DELAYEDRESORTCLC)
			{
				KillTimer(hwnd,TIMERID_DELAYEDRESORTCLC);
#ifdef _DEBUG
OutputDebugString("Delayed Sort CLC\r\n");
#endif
				InvalidateRect(hwnd,NULL,FALSE);
				SortCLC(hwnd,dat,1);
				RecalcScrollBar(hwnd,dat);
				break;
			}

			if (wParam==TIMERID_SUBEXPAND) 
			{		
				KillTimer(hwnd,TIMERID_SUBEXPAND);
				if (hitcontact)
				{
					if (hitcontact->SubExpanded) hitcontact->SubExpanded=0; else hitcontact->SubExpanded=1;
					DBWriteContactSettingByte(hitcontact->hContact,"CList","Expanded",hitcontact->SubExpanded);
				}
				hitcontact=NULL;
				dat->NeedResort=1;
				SortCLC(hwnd,dat,1);		
				RecalcScrollBar(hwnd,dat);
				break;
			}			
			
			if(wParam==TIMERID_RENAME)
				BeginRenameSelection(hwnd,dat);
			else if(wParam==TIMERID_DRAGAUTOSCROLL)
				ScrollTo(hwnd,dat,dat->yScroll+dat->dragAutoScrolling*dat->rowHeight*2,0);
			else if(wParam==TIMERID_INFOTIP) {
				CLCINFOTIP it;
				struct ClcContact *contact;
				int hit;
				RECT clRect;
				POINT ptClientOffset={0};

				KillTimer(hwnd,wParam);
				GetCursorPos(&it.ptCursor);
				ScreenToClient(hwnd,&it.ptCursor);
				if(it.ptCursor.x!=dat->ptInfoTip.x || it.ptCursor.y!=dat->ptInfoTip.y) break;
				GetClientRect(hwnd,&clRect);
				it.rcItem.left=0; it.rcItem.right=clRect.right;
				hit=HitTest(hwnd,dat,it.ptCursor.x,it.ptCursor.y,&contact,NULL,NULL);
				if(hit==-1) break;
				if(contact->type!=CLCIT_GROUP && contact->type!=CLCIT_CONTACT) break;
				ClientToScreen(hwnd,&it.ptCursor);
				ClientToScreen(hwnd,&ptClientOffset);
				it.isTreeFocused=GetFocus()==hwnd;
				it.rcItem.top=hit*dat->rowHeight-dat->yScroll;
				it.rcItem.bottom=it.rcItem.top+dat->rowHeight;
				OffsetRect(&it.rcItem,ptClientOffset.x,ptClientOffset.y);
				it.isGroup=contact->type==CLCIT_GROUP;
				it.hItem=contact->type==CLCIT_GROUP?(HANDLE)contact->groupId:contact->hContact;
				it.cbSize=sizeof(it);
				dat->hInfoTipItem=ContactToHItem(contact);				
				NotifyEventHooks(hShowInfoTipEvent,0,(LPARAM)&it);
			}
			break;

		case WM_LBUTTONDOWN:
		{	struct ClcContact *contact;
			struct ClcGroup *group;
			int hit;
			DWORD hitFlags;

			if(GetFocus()!=hwnd) SetFocus(hwnd);
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			KillTimer(hwnd,TIMERID_SUBEXPAND);

			EndRename(hwnd,dat,1);
			dat->ptDragStart.x=(short)LOWORD(lParam);
			dat->ptDragStart.y=(short)HIWORD(lParam);
			dat->szQuickSearch[0]=0;
			hit=HitTest(hwnd,dat,(short)LOWORD(lParam),(short)HIWORD(lParam),&contact,&group,&hitFlags);
			if(hit!=-1) {
				if(hit==dat->selection && hitFlags&CLCHT_ONITEMLABEL && dat->exStyle&CLS_EX_EDITLABELS) {
					SetCapture(hwnd);
					dat->iDragItem=dat->selection;
					dat->dragStage=DRAGSTAGE_NOTMOVED|DRAGSTAGEF_MAYBERENAME;
					dat->dragAutoScrolling=0;
					break;
				}
			}
			if(hit!=-1 && contact->type==CLCIT_CONTACT && contact->SubAllocated)
				if(hitFlags&CLCHT_ONITEMICON) {
					BYTE doubleClickExpand=DBGetContactSettingByte(NULL,"CLC","MetaDoubleClick",0);
					hitcontact=contact;				
					SetTimer(hwnd,TIMERID_SUBEXPAND,GetDoubleClickTime()*doubleClickExpand,NULL);
					break;
				}
				else
					hitcontact=NULL;




			if(hit!=-1 && contact->type==CLCIT_GROUP)
				if(hitFlags&CLCHT_ONITEMICON) {
					struct ClcGroup *selgroup;
					struct ClcContact *selcontact;
					dat->selection=GetRowByIndex(dat,dat->selection,&selcontact,&selgroup);
					SetGroupExpand(hwnd,dat,contact->group,-1);
					if(dat->selection!=-1) {
						dat->selection=GetRowsPriorTo(&dat->list,selgroup,((unsigned)selcontact-(unsigned)selgroup->contact)/sizeof(struct ClcContact));
						if(dat->selection==-1) dat->selection=GetRowsPriorTo(&dat->list,contact->group,-1);
					}
					InvalidateRect(hwnd,NULL,FALSE);
					UpdateWindow(hwnd);
					break;
				}
			if(hit!=-1 && hitFlags&CLCHT_ONITEMCHECK) {
				NMCLISTCONTROL nm;
				contact->flags^=CONTACTF_CHECKED;
				if(contact->type==CLCIT_GROUP) SetGroupChildCheckboxes(contact->group,contact->flags&CONTACTF_CHECKED);
				RecalculateGroupCheckboxes(hwnd,dat);
				InvalidateRect(hwnd,NULL,FALSE);
				nm.hdr.code=CLN_CHECKCHANGED;
				nm.hdr.hwndFrom=hwnd;
				nm.hdr.idFrom=GetDlgCtrlID(hwnd);
				nm.flags=0;
				nm.hItem=ContactToItemHandle(contact,&nm.flags);
				SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
			}
			if(!(hitFlags&(CLCHT_ONITEMICON|CLCHT_ONITEMLABEL|CLCHT_ONITEMCHECK))) {
				NMCLISTCONTROL nm;
				nm.hdr.code=NM_CLICK;
				nm.hdr.hwndFrom=hwnd;
				nm.hdr.idFrom=GetDlgCtrlID(hwnd);
				nm.flags=0;
				if(hit==-1) nm.hItem=NULL;
				else nm.hItem=ContactToItemHandle(contact,&nm.flags);
				nm.iColumn=hitFlags&CLCHT_ONITEMEXTRA?HIBYTE(HIWORD(hitFlags)):-1;
				nm.pt=dat->ptDragStart;
				SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
			}
			if(hitFlags&(CLCHT_ONITEMCHECK|CLCHT_ONITEMEXTRA)) break;
			dat->selection=hit;
			InvalidateRect(hwnd,NULL,FALSE);
			if(dat->selection!=-1) EnsureVisible(hwnd,dat,hit,0);
			UpdateWindow(hwnd);
			if(dat->selection!=-1 && (contact->type==CLCIT_CONTACT || contact->type==CLCIT_GROUP) && !(hitFlags&(CLCHT_ONITEMEXTRA|CLCHT_ONITEMCHECK))) {
				SetCapture(hwnd);
				dat->iDragItem=dat->selection;
				dat->dragStage=DRAGSTAGE_NOTMOVED;
				dat->dragAutoScrolling=0;
			}
			break;
		}


		case WM_MOUSEMOVE:
			if(dat->iDragItem==-1) {
				int iOldHotTrack=dat->iHotTrack;
				if(dat->hwndRenameEdit!=NULL) break;
				if(GetKeyState(VK_MENU)&0x8000 || GetKeyState(VK_F10)&0x8000) break;
				dat->iHotTrack=HitTest(hwnd,dat,(short)LOWORD(lParam),(short)HIWORD(lParam),NULL,NULL,NULL);
				if(iOldHotTrack!=dat->iHotTrack) {
					if(iOldHotTrack==-1) SetCapture(hwnd);
					else if(dat->iHotTrack==-1) ReleaseCapture();
					if(dat->exStyle&CLS_EX_TRACKSELECT) {
						InvalidateItem(hwnd,dat,iOldHotTrack);
						InvalidateItem(hwnd,dat,dat->iHotTrack);
					}
					HideInfoTip(hwnd,dat);
				}
				KillTimer(hwnd,TIMERID_INFOTIP);
				if(wParam==0 && dat->hInfoTipItem==NULL) {
					dat->ptInfoTip.x=(short)LOWORD(lParam);
					dat->ptInfoTip.y=(short)HIWORD(lParam);
					SetTimer(hwnd,TIMERID_INFOTIP,dat->infoTipTimeout,NULL);
				}
				break;
			}
			if((dat->dragStage&DRAGSTAGEM_STAGE)==DRAGSTAGE_NOTMOVED && !(dat->exStyle&CLS_EX_DISABLEDRAGDROP)) {
				if(abs((short)LOWORD(lParam)-dat->ptDragStart.x)>=GetSystemMetrics(SM_CXDRAG) || abs((short)HIWORD(lParam)-dat->ptDragStart.y)>=GetSystemMetrics(SM_CYDRAG))
					dat->dragStage=(dat->dragStage&~DRAGSTAGEM_STAGE)|DRAGSTAGE_ACTIVE;
			}
			if((dat->dragStage&DRAGSTAGEM_STAGE)==DRAGSTAGE_ACTIVE) {
				HCURSOR hNewCursor;
				RECT clRect;
				POINT pt;
				int target;

				GetClientRect(hwnd,&clRect);
				pt.x=(short)LOWORD(lParam); pt.y=(short)HIWORD(lParam);
				hNewCursor=LoadCursor(NULL, IDC_NO);
				InvalidateRect(hwnd,NULL,FALSE);
				if(dat->dragAutoScrolling)
					{KillTimer(hwnd,TIMERID_DRAGAUTOSCROLL); dat->dragAutoScrolling=0;}
				target=GetDropTargetInformation(hwnd,dat,pt);
				if(dat->dragStage&DRAGSTAGEF_OUTSIDE && target!=DROPTARGET_OUTSIDE) {
					NMCLISTCONTROL nm;
					struct ClcContact *contact;
					GetRowByIndex(dat,dat->iDragItem,&contact,NULL);
					nm.hdr.code=CLN_DRAGSTOP;
					nm.hdr.hwndFrom=hwnd;
					nm.hdr.idFrom=GetDlgCtrlID(hwnd);
					nm.flags=0;
					nm.hItem=ContactToItemHandle(contact,&nm.flags);
					SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
					dat->dragStage&=~DRAGSTAGEF_OUTSIDE;
				}
				switch(target) {
					case DROPTARGET_ONSELF:
					case DROPTARGET_ONCONTACT:
						break;
					case DROPTARGET_ONGROUP:
						hNewCursor=LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER));
						break;
					case DROPTARGET_INSERTION:
						hNewCursor=LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER));
						break;
					case DROPTARGET_OUTSIDE:
					{	NMCLISTCONTROL nm;
						struct ClcContact *contact;

						if(pt.x>=0 && pt.x<clRect.right && ((pt.y<0 && pt.y>-dat->dragAutoScrollHeight) || (pt.y>=clRect.bottom && pt.y<clRect.bottom+dat->dragAutoScrollHeight))) {
							if(!dat->dragAutoScrolling) {
								if(pt.y<0) dat->dragAutoScrolling=-1;
								else dat->dragAutoScrolling=1;
								SetTimer(hwnd,TIMERID_DRAGAUTOSCROLL,dat->scrollTime,NULL);
							}
							SendMessage(hwnd,WM_TIMER,TIMERID_DRAGAUTOSCROLL,0);
						}

						dat->dragStage|=DRAGSTAGEF_OUTSIDE;
						GetRowByIndex(dat,dat->iDragItem,&contact,NULL);
						nm.hdr.code=CLN_DRAGGING;
						nm.hdr.hwndFrom=hwnd;
						nm.hdr.idFrom=GetDlgCtrlID(hwnd);
						nm.flags=0;
						nm.hItem=ContactToItemHandle(contact,&nm.flags);
						nm.pt=pt;
						if(SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm))
							return 0;
						break;
					}
					default:
					{	struct ClcGroup *group;
						GetRowByIndex(dat,dat->iDragItem,NULL,&group);
						if(group->parent) hNewCursor=LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_DROPUSER));
						break;
					}
				}
				SetCursor(hNewCursor);
			}
			break;

		case WM_LBUTTONUP:
			if(dat->iDragItem==-1) break;
			SetCursor((HCURSOR)GetClassLong(hwnd,GCL_HCURSOR));
			if(dat->exStyle&CLS_EX_TRACKSELECT) {
				dat->iHotTrack=HitTest(hwnd,dat,(short)LOWORD(lParam),(short)HIWORD(lParam),NULL,NULL,NULL);
				if(dat->iHotTrack==-1) ReleaseCapture();
			}
			else ReleaseCapture();
			KillTimer(hwnd,TIMERID_DRAGAUTOSCROLL);
			if(dat->dragStage==(DRAGSTAGE_NOTMOVED|DRAGSTAGEF_MAYBERENAME))
				SetTimer(hwnd,TIMERID_RENAME,GetDoubleClickTime(),NULL);
			else if((dat->dragStage&DRAGSTAGEM_STAGE)==DRAGSTAGE_ACTIVE) {
				POINT pt;
				int target;

				pt.x=(short)LOWORD(lParam); pt.y=(short)HIWORD(lParam);
				target=GetDropTargetInformation(hwnd,dat,pt);
				switch(target) {
					case DROPTARGET_ONSELF:
						break;
					case DROPTARGET_ONCONTACT:
						break;
					case DROPTARGET_ONGROUP:
					{	struct ClcContact *contact;
						char *szGroup;
						GetRowByIndex(dat,dat->selection,&contact,NULL);
						szGroup=(char*)CallService(MS_CLIST_GROUPGETNAME2,contact->groupId,(LPARAM)(int*)NULL);
						GetRowByIndex(dat,dat->iDragItem,&contact,NULL);
						if(contact->type==CLCIT_CONTACT)	 //dropee is a contact
							DBWriteContactSettingString(contact->hContact,"CList","Group",szGroup);
						else if(contact->type==CLCIT_GROUP) { //dropee is a group
							char szNewName[120];
							_snprintf(szNewName,sizeof(szNewName),"%s\\%s",szGroup,contact->szText);
							CallService(MS_CLIST_GROUPRENAME,contact->groupId,(LPARAM)szNewName);
						}
						break;
					}
					case DROPTARGET_INSERTION:
					{	struct ClcContact *contact,*destcontact;
						struct ClcGroup *destgroup;
						GetRowByIndex(dat,dat->iDragItem,&contact,NULL);
						if(GetRowByIndex(dat,dat->iInsertionMark,&destcontact,&destgroup)==-1 || destgroup!=contact->group->parent)
							CallService(MS_CLIST_GROUPMOVEBEFORE,contact->groupId,0);
						else {
							if(destcontact->type==CLCIT_GROUP) destgroup=destcontact->group;
							else destgroup=destgroup;
							CallService(MS_CLIST_GROUPMOVEBEFORE,contact->groupId,destgroup->groupId);
						}
						break;
					}
					case DROPTARGET_OUTSIDE:
					{	NMCLISTCONTROL nm;
						struct ClcContact *contact;
						GetRowByIndex(dat,dat->iDragItem,&contact,NULL);
						nm.hdr.code=CLN_DROPPED;
						nm.hdr.hwndFrom=hwnd;
						nm.hdr.idFrom=GetDlgCtrlID(hwnd);
						nm.flags=0;
						nm.hItem=ContactToItemHandle(contact,&nm.flags);
						nm.pt=pt;
						SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
						break;
					}
					default:
					{	struct ClcGroup *group;
						struct ClcContact *contact;
						GetRowByIndex(dat,dat->iDragItem,&contact,&group);
						if(group->parent) {	 //move to root
							if(contact->type==CLCIT_CONTACT)	 //dropee is a contact
								DBDeleteContactSetting(contact->hContact,"CList","Group");
							else if(contact->type==CLCIT_GROUP) { //dropee is a group
								char szNewName[120];
								lstrcpyn(szNewName,contact->szText,sizeof(szNewName));
								CallService(MS_CLIST_GROUPRENAME,contact->groupId,(LPARAM)szNewName);
							}
						}
						break;
					}
				}
			}
			InvalidateRect(hwnd,NULL,FALSE);
			dat->iDragItem=-1;
			dat->iInsertionMark=-1;
			break;

		case WM_LBUTTONDBLCLK:
		{	struct ClcContact *contact;
			DWORD hitFlags;
			ReleaseCapture();
			dat->iHotTrack=-1;
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_RENAME);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_SUBEXPAND);
			dat->szQuickSearch[0]=0;
			dat->selection=HitTest(hwnd,dat,(short)LOWORD(lParam),(short)HIWORD(lParam),&contact,NULL,&hitFlags);
			InvalidateRect(hwnd,NULL,FALSE);
			if(dat->selection!=-1) EnsureVisible(hwnd,dat,dat->selection,0);
			if(!(hitFlags&(CLCHT_ONITEMICON|CLCHT_ONITEMLABEL))) break;
			UpdateWindow(hwnd);
			DoSelectionDefaultAction(hwnd,dat);
			break;
		}

		case WM_CONTEXTMENU:
		{	struct ClcContact *contact;
			HMENU hMenu=NULL;
			POINT pt;
			DWORD hitFlags;

			EndRename(hwnd,dat,1);
			HideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_RENAME);
			KillTimer(hwnd,TIMERID_INFOTIP);
			if(GetFocus()!=hwnd) SetFocus(hwnd);
			dat->iHotTrack=-1;
			dat->szQuickSearch[0]=0;
			pt.x=(short)LOWORD(lParam);
			pt.y=(short)HIWORD(lParam);
			if(pt.x==-1 && pt.y==-1) {
				dat->selection=GetRowByIndex(dat,dat->selection,&contact,NULL);
				if(dat->selection!=-1) EnsureVisible(hwnd,dat,dat->selection,0);
				pt.x=dat->iconXSpace+15;
				pt.y=dat->selection*dat->rowHeight-dat->yScroll+(int)(dat->rowHeight*.7);
				hitFlags=dat->selection==-1?CLCHT_NOWHERE:CLCHT_ONITEMLABEL;
			}
			else {
				ScreenToClient(hwnd,&pt);
				dat->selection=HitTest(hwnd,dat,pt.x,pt.y,&contact,NULL,&hitFlags);
			}
			InvalidateRect(hwnd,NULL,FALSE);
			if(dat->selection!=-1) EnsureVisible(hwnd,dat,dat->selection,0);
			UpdateWindow(hwnd);

			if(dat->selection!=-1 && hitFlags&(CLCHT_ONITEMICON|CLCHT_ONITEMCHECK|CLCHT_ONITEMLABEL)) {
				if(contact->type==CLCIT_GROUP) {
					hMenu=(HMENU)CallService(MS_CLIST_MENUBUILDSUBGROUP,(WPARAM)contact->group,0);
					ClientToScreen(hwnd,&pt);
					TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,(HWND)CallService(MS_CLUI_GETHWND,0,0),NULL);
					return 0;
					//CheckMenuItem(hMenu,POPUP_GROUPHIDEOFFLINE,contact->group->hideOffline?MF_CHECKED:MF_UNCHECKED);
				}
				else if(contact->type==CLCIT_CONTACT)
					hMenu=(HMENU)CallService(MS_CLIST_MENUBUILDCONTACT,(WPARAM)contact->hContact,0);
			}
			else
			{
				//call parent for new group/hide offline menu
				PostMessage(GetParent(hwnd),WM_CONTEXTMENU,wParam,lParam);
				return 0;
			}
			if(hMenu!=NULL) {
				ClientToScreen(hwnd,&pt);
				TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL);
				DestroyMenu(hMenu);
			}
			return 0;
		}

		case WM_MEASUREITEM:
			return CallService(MS_CLIST_MENUMEASUREITEM,wParam,lParam);
		case WM_DRAWITEM:
			return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);

		case WM_COMMAND:
		{	int hit;
			struct ClcContact *contact;
			hit=GetRowByIndex(dat,dat->selection,&contact,NULL);
			if(hit==-1) break;
			if(contact->type==CLCIT_CONTACT)
				if(CallService(MS_CLIST_MENUPROCESSCOMMAND,MAKEWPARAM(LOWORD(wParam),MPCF_CONTACTMENU),(LPARAM)contact->hContact)) break;
			switch(LOWORD(wParam)) {
				case POPUP_NEWSUBGROUP:
					if(contact->type!=CLCIT_GROUP) break;
					SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)&~CLS_HIDEEMPTYGROUPS);
					CallService(MS_CLIST_GROUPCREATE,contact->groupId,0);
					break;
				case POPUP_RENAMEGROUP:
					BeginRenameSelection(hwnd,dat);
					break;
				case POPUP_DELETEGROUP:
					if(contact->type!=CLCIT_GROUP) break;
					CallService(MS_CLIST_GROUPDELETE,contact->groupId,0);
					break;
				case POPUP_GROUPHIDEOFFLINE:
					if(contact->type!=CLCIT_GROUP) break;
					CallService(MS_CLIST_GROUPSETFLAGS,contact->groupId,MAKELPARAM(contact->group->hideOffline?0:GROUPF_HIDEOFFLINE,GROUPF_HIDEOFFLINE));
					break;
			}
			break;
		}
			
		case WM_DESTROY:
			stopStatusUpdater = 1;
			HideInfoTip(hwnd,dat);
			{	int i;
				for(i=0;i<=FONTID_MAX;i++)
					if(!dat->fontInfo[i].changed) DeleteObject(dat->fontInfo[i].hFont);
			}
			if(dat->himlHighlight)
				ImageList_Destroy(dat->himlHighlight);
			if(dat->hwndRenameEdit) DestroyWindow(dat->hwndRenameEdit);
			if(!dat->bkChanged && dat->hBmpBackground) DeleteObject(dat->hBmpBackground);
			FreeGroup(&dat->list);
			mir_free(dat);
			UnregisterFileDropping(hwnd);
			WindowList_Remove(hClcWindowList,hwnd);

	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}