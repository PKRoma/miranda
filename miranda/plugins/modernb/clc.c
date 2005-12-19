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
#include "m_skin.h"
#include "commonprototypes.h"

int DefaultImageListColorDepth=ILC_COLOR32;

HIMAGELIST himlCListClc;
static HANDLE hShowInfoTipEvent;
HANDLE hHideInfoTipEvent;
static HANDLE hAckHook, hAvatarChanged;
extern int BehindEdgeSettings;

static HANDLE hSettingChanged1;
static HANDLE hSettingChanged2;
extern BOOL InvalidateRectZ(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern int BgStatusBarChange(WPARAM wParam,LPARAM lParam);

extern int IsInMainWindow(HWND hwnd);
extern int BgClcChange(WPARAM wParam,LPARAM lParam);
extern int BgMenuChange(WPARAM wParam,LPARAM lParam);
extern int OnFrameTitleBarBackgroundChange(WPARAM wParam,LPARAM lParam);
extern int UpdateFrameImage(WPARAM /*hWnd*/, LPARAM/*sPaintRequest*/);
extern int InvalidateFrameImage(WPARAM wParam, LPARAM lParam);

extern void CacheContactAvatar(struct ClcData *dat, struct ClcContact *contact, BOOL changed);

extern int GetStatusForContact(HANDLE hContact,char *szProto);
extern int GetContactIconC(pdisplayNameCacheEntry cacheEntry);
extern int GetContactIcon(WPARAM wParam,LPARAM lParam);

extern int TestCursorOnBorders();
extern int SizingOnBorder(POINT ,int);

extern int BehindEdge_State;

int hClcProtoCount = 0;
ClcProtoStatus *clcProto = NULL;
extern int sortBy[3];
struct ClcContact * hitcontact=NULL;
static POINT HitPoint;
static int mUpped;
BYTE LOCK_REPAINTING=0;
#define UNLOCK_REPAINT 13315

BYTE IsDragToScrollMode=0;
int StartDragPos=0;
int StartScrollPos=0;


struct AvatarOverlayIconConfig 
{
	char *name;
	char *description;
	int id;
	HICON icon;
} avatar_overlay_icons[ID_STATUS_OUTTOLUNCH - ID_STATUS_OFFLINE + 1] = 
{
	{ "AVATAR_OVERLAY_OFFLINE", "Offline", IDI_AVATAR_OVERLAY_OFFLINE, NULL},
	{ "AVATAR_OVERLAY_ONLINE", "Online", IDI_AVATAR_OVERLAY_ONLINE, NULL},
	{ "AVATAR_OVERLAY_AWAY", "Away", IDI_AVATAR_OVERLAY_AWAY, NULL},
	{ "AVATAR_OVERLAY_DND", "DND", IDI_AVATAR_OVERLAY_DND, NULL},
	{ "AVATAR_OVERLAY_NA", "NA", IDI_AVATAR_OVERLAY_NA, NULL},
	{ "AVATAR_OVERLAY_OCCUPIED", "Occupied", IDI_AVATAR_OVERLAY_OCCUPIED, NULL},
	{ "AVATAR_OVERLAY_CHAT", "Free for chat", IDI_AVATAR_OVERLAY_CHAT, NULL},
	{ "AVATAR_OVERLAY_INVISIBLE", "Invisible", IDI_AVATAR_OVERLAY_INVISIBLE, NULL},
	{ "AVATAR_OVERLAY_PHONE", "On the phone", IDI_AVATAR_OVERLAY_PHONE, NULL},
	{ "AVATAR_OVERLAY_LUNCH", "Out to lunch", IDI_AVATAR_OVERLAY_LUNCH, NULL}
};


int ExitDragToScroll(void);

extern tPaintCallbackProc ClcPaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData);


int EnterDragToScroll(HWND hwnd, int Y)
{
	struct ClcData * dat;
	if (IsDragToScrollMode) return 0;
	dat=(struct ClcData*)GetWindowLong(hwnd,0);
	if (!dat) return 0;
	StartDragPos=Y;
	StartScrollPos=dat->yScroll;
	IsDragToScrollMode=1;
	SetCapture(hwnd);
	return 1;
}
int ProceedDragToScroll(HWND hwnd, int Y)
{
	int pos,dy;
	if (!IsDragToScrollMode) return 0;
	if(GetCapture()!=hwnd) ExitDragToScroll();
	dy=StartDragPos-Y;
	pos=StartScrollPos+dy;
	if (pos<0) 
		pos=0;
	SendMessage(hwnd, WM_VSCROLL,MAKEWPARAM(SB_THUMBTRACK,pos),0);
	return 1;
}

int ExitDragToScroll()
{
	if (!IsDragToScrollMode) return 0;
	IsDragToScrollMode=0;
	ReleaseCapture();
	return 1;
}

int GetProtocolVisibility(char * ProtoName)
//{
//	char buf[255];
//	return DBGetContactSettingByte(0,"Protocols",buf,0);
//}
{
	int i;
	int res=0;
	DBVARIANT dbv={0};
	char buf2[15];
	int count;
	if (!ProtoName) return 0;
	count=(int)DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
	if (count==-1) return 1;
	for (i=0; i<count; i++)
	{
		itoa(i,buf2,10);
		if (!DBGetContactSetting(NULL,"Protocols",buf2,&dbv))
		{
			if (MyStrCmp(ProtoName,dbv.pszVal)==0)
			{
				mir_free(dbv.pszVal);
				DBFreeVariant(&dbv);
				itoa(i+400,buf2,10);
				res= DBGetContactSettingDword(NULL,"Protocols",buf2,0);
				return res;
			}
			mir_free(dbv.pszVal);
			DBFreeVariant(&dbv);
		}
	}
	return 0;
}

int SmileyAddOptionsChanged(WPARAM wParam,LPARAM lParam)
{
	pcli->pfnClcBroadcast( CLM_AUTOREBUILD,0,0);
	pcli->pfnClcBroadcast( INTM_INVALIDATE,0,0);
	return 0;
}

void ClcOptionsChanged(void)
{
	pcli->pfnClcBroadcast( INTM_RELOADOPTIONS,0,0);
	pcli->pfnClcBroadcast( INTM_INVALIDATE,0,0);
}

void SortClcByTimer (HWND hwnd)
{
	KillTimer(hwnd,TIMERID_DELAYEDRESORTCLC);
	SetTimer(hwnd,TIMERID_DELAYEDRESORTCLC,DBGetContactSettingByte(NULL,"CLUI","DELAYEDTIMER",10),NULL);
}

static int ClcSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;

	if ((HANDLE)wParam==NULL)
	{
		if (!MyStrCmp(cws->szModule,"MetaContacts"))
		{
			if (!MyStrCmp(cws->szSetting,"Enabled"))
				pcli->pfnClcBroadcast( INTM_RELOADOPTIONS,wParam,lParam);
		}
		else if (!MyStrCmp(cws->szModule,"CListGroups")) 
		{
			pcli->pfnClcBroadcast( INTM_GROUPSCHANGED,wParam,lParam);
		}
	}
	else // (HANDLE)wParam != NULL
	{
		if (!strcmp(cws->szSetting,"TickTS"))
		{
			pcli->pfnClcBroadcast( INTM_STATUSCHANGED,wParam,0);
		}
		else if (!strcmp(cws->szModule,"MetaContacts") && !strcmp(cws->szSetting,"Handle"))
		{
			pcli->pfnClcBroadcast( INTM_NAMEORDERCHANGED,0,0);	
		}
		else if (!strcmp(cws->szModule,"UserInfo"))
		{
			if (!strcmp(cws->szSetting,"Timezone"))
				pcli->pfnClcBroadcast( INTM_TIMEZONECHANGED,wParam,0);	
		}
		else if (!strcmp(cws->szModule,"CList")) 
		{
			if(!strcmp(cws->szSetting,"MyHandle"))
				pcli->pfnClcBroadcast( INTM_NAMECHANGED,wParam,lParam);
			else if(!strcmp(cws->szSetting,"Group"))
				pcli->pfnClcBroadcast( INTM_GROUPCHANGED,wParam,lParam);
			else if(!strcmp(cws->szSetting,"Hidden"))
				pcli->pfnClcBroadcast( INTM_HIDDENCHANGED,wParam,lParam);
			else if(!strcmp(cws->szSetting,"noOffline"))
				pcli->pfnClcBroadcast( INTM_NAMEORDERCHANGED,wParam,lParam);
			else if(!strcmp(cws->szSetting,"NotOnList"))
				pcli->pfnClcBroadcast( INTM_NOTONLISTCHANGED,wParam,lParam);
			else if(!strcmp(cws->szSetting,"Status"))
				pcli->pfnClcBroadcast( INTM_STATUSCHANGED,wParam,0);
			else if(!strcmp(cws->szSetting,"NameOrder"))
				pcli->pfnClcBroadcast( INTM_NAMEORDERCHANGED,0,0);
			else if(!strcmp(cws->szSetting,"StatusMsg")) 
				pcli->pfnClcBroadcast( INTM_STATUSMSGCHANGED,wParam,0);    
		}
		else if(!strcmp(cws->szModule,"ContactPhoto")) 
		{
			if (!strcmp(cws->szSetting,"File")) 
				pcli->pfnClcBroadcast( INTM_AVATARCHANGED,wParam,0);
		}
		else 
		{
			pdisplayNameCacheEntry pdnce =(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry((HANDLE)wParam);

			if (pdnce!=NULL)
			{					
				if(pdnce->szProto==NULL || MyStrCmp(pdnce->szProto,cws->szModule)) return 0;

				if (!strcmp(cws->szSetting,"UIN"))
					pcli->pfnClcBroadcast( INTM_NAMECHANGED,wParam,lParam);
				else if (!strcmp(cws->szSetting,"Nick") || !strcmp(cws->szSetting,"FirstName") 
					|| !strcmp(cws->szSetting,"e-mail") || !strcmp(cws->szSetting,"LastName") 
					|| !strcmp(cws->szSetting,"JID"))
					pcli->pfnClcBroadcast( INTM_NAMECHANGED,wParam,lParam);
				else if (!strcmp(cws->szSetting,"ApparentMode"))
					pcli->pfnClcBroadcast( INTM_APPARENTMODECHANGED,wParam,lParam);
				else if (!strcmp(cws->szSetting,"IdleTS"))
					pcli->pfnClcBroadcast( INTM_IDLECHANGED,wParam,lParam);
				else if (!strcmp(cws->szSetting,"XStatusMsg"))
					pcli->pfnClcBroadcast( INTM_STATUSMSGCHANGED,wParam,0);
				else if (!strcmp(cws->szSetting,"Status") || !strcmp(cws->szSetting,"XStatusId") || !strcmp(cws->szSetting,"XStatusName"))
					pcli->pfnClcBroadcast( INTM_STATUSCHANGED,wParam,0);
				else if (!strcmp(cws->szSetting,"Timezone"))
					pcli->pfnClcBroadcast( INTM_TIMEZONECHANGED,wParam,0);
			}
		}
	}
	return 0;
}

// IcoLib hook to handle icon changes
static int ReloadAvatarOverlayIcons(WPARAM wParam, LPARAM lParam) 
{
	int i;
	for (i = 0 ; i < MAX_REGS(avatar_overlay_icons) ; i++)
	{
		avatar_overlay_icons[i].icon = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)avatar_overlay_icons[i].name);
	}

	pcli->pfnClcBroadcast( INTM_INVALIDATE,0,0);

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

	// Get icons
	if(ServiceExists(MS_SKIN2_ADDICON)) 
	{
		SKINICONDESC2 sid;
		char szMyPath[MAX_PATH];
		int i;

		GetModuleFileNameA(g_hInst, szMyPath, MAX_PATH);

		sid.cbSize = sizeof(sid);
		sid.pszSection = Translate("Contact List/Avatar Overlay");
		sid.pszDefaultFile = szMyPath;


		for (i = 0 ; i < MAX_REGS(avatar_overlay_icons) ; i++)
		{
			sid.pszDescription = Translate(avatar_overlay_icons[i].description);
			sid.pszName = avatar_overlay_icons[i].name;
			sid.iDefaultIndex = - avatar_overlay_icons[i].id;
			CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		}

		ReloadAvatarOverlayIcons(0,0);

		HookEvent(ME_SKIN2_ICONSCHANGED, ReloadAvatarOverlayIcons);
	}
	else 
	{
		int i;
		for (i = 0 ; i < MAX_REGS(avatar_overlay_icons) ; i++)
		{
			avatar_overlay_icons[i].icon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(avatar_overlay_icons[i].id), IMAGE_ICON, 0, 0, 0);
		}
	}

	// Register smiley category
	if (ServiceExists(MS_SMILEYADD_REGISTERCATEGORY))
	{
		SMADD_REGCAT rc;

		rc.cbSize = sizeof(rc);
		rc.name = "clist";
		rc.dispname = Translate("Contact List smileys");

		CallService(MS_SMILEYADD_REGISTERCATEGORY, 0, (LPARAM)&rc);

		HookEvent(ME_SMILEYADD_OPTIONSCHANGED,SmileyAddOptionsChanged);
	}

	//CallService(MS_BACKGROUNDCONFIG_REGISTER,(WPARAM)"StatusBar Background/StatusBar",0);
	//CallService(MS_BACKGROUNDCONFIG_REGISTER,(WPARAM)"List Background/CLC",0);
	//CallService(MS_BACKGROUNDCONFIG_REGISTER,(WPARAM)"Frames TitleBar BackGround/FrameTitleBar",0);
	//CallService(MS_BACKGROUNDCONFIG_REGISTER,(WPARAM)"Main Menu/Menu",0);
	HookEvent(ME_BACKGROUNDCONFIG_CHANGED,BgClcChange);
	HookEvent(ME_BACKGROUNDCONFIG_CHANGED,BgMenuChange);
	HookEvent(ME_BACKGROUNDCONFIG_CHANGED,BgStatusBarChange);
	HookEvent(ME_BACKGROUNDCONFIG_CHANGED,OnFrameTitleBarBackgroundChange);


	return 0;
}

int ClcProtoAck(WPARAM wParam,LPARAM lParam)
{
	ACKDATA *ack=(ACKDATA*)lParam;

	if (ack->type==ACKTYPE_AWAYMSG)
	{
		if (ack->result==ACKRESULT_SUCCESS && ack->lParam) {
			{//Do not change DB if it is IRC protocol    
				if (ack->szModule!= NULL) 
					if(DBGetContactSettingByte(ack->hContact, ack->szModule, "ChatRoom", 0) != 0) return 0;
			}
			DBWriteContactSettingString(ack->hContact,"CList","StatusMsg",(const char *)ack->lParam);
			//pcli->pfnClcBroadcast( INTM_STATUSMSGCHANGED,(WPARAM)ack->hContact,(LPARAM)ack->lParam);      

		} 
		else
		{
			//DBDeleteContactSetting(ack->hContact,"CList","StatusMsg");
			//char a='\0';
			{//Do not change DB if it is IRC protocol    
				if (ack->szModule!= NULL) 
					if(DBGetContactSettingByte(ack->hContact, ack->szModule, "ChatRoom", 0) != 0) return 0;
			}
			DBWriteContactSettingString(ack->hContact,"CList","StatusMsg","");
			//pcli->pfnClcBroadcast( INTM_STATUSMSGCHANGED,(WPARAM)ack->hContact,&a);              
		}
	}
	else if (ack->type==ACKTYPE_AVATAR) 
	{
		if (ack->result==ACKRESULT_SUCCESS) 
		{
			PROTO_AVATAR_INFORMATION *pai = (PROTO_AVATAR_INFORMATION *) ack->hProcess;

			if (pai != NULL && pai->hContact != NULL)
			{
				pcli->pfnClcBroadcast( INTM_AVATARCHANGED,(WPARAM)pai->hContact,0);
			}
		}
	}
	return 0;
}

static int ClcShutdown(WPARAM wParam,LPARAM lParam)
{
	UnhookEvent(hAckHook);
	UnhookEvent(hSettingChanged1);
	if(clcProto) mir_free(clcProto);
	return 0;
}

int LoadCLCModule(void)
{
	himlCListClc=(HIMAGELIST)CallService(MS_CLIST_GETICONSIMAGELIST,0,0);

	hSettingChanged1=HookEvent(ME_DB_CONTACT_SETTINGCHANGED,ClcSettingChanged);
	HookEvent(ME_OPT_INITIALISE,ClcOptInit);
	hAckHook=(HANDLE)HookEvent(ME_PROTO_ACK,ClcProtoAck);
	HookEvent(ME_SYSTEM_SHUTDOWN,ClcShutdown);
	return 0;
}

int AvatarChanged(WPARAM wParam, LPARAM lParam)
{
	pcli->pfnClcBroadcast(  INTM_AVATARCHANGED, wParam, lParam);
	return 0;
}

extern LRESULT ( CALLBACK *saveContactListControlWndProc )(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{     
	struct ClcData *dat;

	dat=(struct ClcData*)GetWindowLong(hwnd,0);

	if(msg>=CLM_FIRST && msg<CLM_LAST) return ProcessExternalMessages(hwnd,dat,msg,wParam,lParam);
	switch (msg) {
	case WM_CREATE:
		{
			TRACE("Create New ClistControl BEGIN\r\n");
			dat=(struct ClcData*)mir_calloc(1,sizeof(struct ClcData));
			SetWindowLong(hwnd,0,(LONG)dat);

			dat->use_avatar_service = ServiceExists(MS_AV_GETAVATARBITMAP);
			if (dat->use_avatar_service)
			{
				if (!hAvatarChanged)
					hAvatarChanged=HookEvent(ME_AV_AVATARCHANGED, AvatarChanged);
			}
			else
			{
				ImageArray_Initialize(&dat->avatar_cache, FALSE, 20);
			}

			RowHeights_Initialize(dat);

			dat->NeedResort=1;
			dat->MetaIgnoreEmptyExtra=DBGetContactSettingByte(NULL,"CLC","MetaIgnoreEmptyExtra",1);
			dat->IsMetaContactsEnabled=
				DBGetContactSettingByte(NULL,"MetaContacts","Enabled",1) && ServiceExists(MS_MC_GETDEFAULTCONTACT);
			dat->expandMeta=DBGetContactSettingByte(NULL,"CLC","MetaExpanding",1);
			InitDisplayNameCache(&dat->lCLCContactsCache);

			LoadClcOptions(hwnd,dat);
			RebuildEntireList(hwnd,dat);

			TRACE("Create New ClistControl END\r\n");
			SetTimer(hwnd,TIMERID_INVALIDATE,2000,NULL);
			break;
		}
	case WM_NCHITTEST:
		{

			break;//return 0;
		}

	case WM_SIZE:
		pcli->pfnEndRename(hwnd,dat,1);
		KillTimer(hwnd,TIMERID_INFOTIP);
		KillTimer(hwnd,TIMERID_RENAME);
		RecalcScrollBar(hwnd,dat);
		return 0;

	case INTM_ICONCHANGED:
		{
			struct ClcContact *contact=NULL;
			struct ClcGroup *group=NULL;
			int recalcScrollBar=0,shouldShow;
			int moveToGroup=0;
			BOOL image_is_special;
			pdisplayNameCacheEntry cacheEntry;

			WORD status;
			char *szProto;
			int NeedResort=0;
			BOOL curisspecial=FALSE;
			dat->NeedResort=0;
			cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry((HANDLE)wParam);

			//szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
			//szProto=GetContactCachedProtocol((HANDLE)wParam);
			szProto=cacheEntry->szProto;
			if(szProto==NULL) status=ID_STATUS_OFFLINE;
			else status=cacheEntry->status;

			if (GetContactIconC(cacheEntry) != lParam)
			{
				image_is_special = TRUE;
				NeedResort=0;
			}
			else
			{
				image_is_special = FALSE;
				NeedResort=1;
			}
			shouldShow=(GetWindowLong(hwnd,GWL_STYLE)&CLS_SHOWHIDDEN
				|| !cacheEntry->Hidden)
				&& (!pcli->pfnIsHiddenMode(dat,status)||cacheEntry->noHiddenOffline
				|| image_is_special);	//this means an offline msg is flashing, so the contact should be shown

			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,&group,NULL,FALSE)) 
			{
				if(shouldShow) 
				{
					if (cacheEntry->HiddenSubcontact)
					{
						dat->NeedResort=1;
						NeedResort=1;
						recalcScrollBar=1;
					}
					else
					{
						AddContactToTree(hwnd,dat,(HANDLE)wParam,0,0);
						dat->NeedResort=1;
						recalcScrollBar=1;
						FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE);
						if (contact) 
						{
							contact->iImage=(WORD)lParam;
							pcli->pfnNotifyNewContact(hwnd,(HANDLE)wParam);
							dat->NeedResort=1;
							NeedResort=1;

							// Propagate icon to parent
							if (0&&contact->isSubcontact)
							{
								if (image_is_special)
								{
									PostMessage(hwnd, INTM_ICONCHANGED, (WPARAM) contact->/*by_fyr_*/subcontacts/*by_fyr_*/->hContact, 
										lParam);
								}
								else
								{
									PostMessage(hwnd, INTM_ICONCHANGED, (WPARAM) contact->/*by_fyr_*/subcontacts/*by_fyr_*/->hContact, 
										(LPARAM) GetContactIcon((WPARAM) contact->/*by_fyr_*/subcontacts/*by_fyr_*/->hContact, 0));
								}
							}
						}
					}				
				}
			}
			else 
			{
				//item in list already
				DWORD style=GetWindowLong(hwnd,GWL_STYLE);				

				if(contact->iImage==(WORD)lParam) break;				

				if ((sortBy[0]==1 || sortBy[1]==1 || sortBy[2]==1)||
					(style&CLS_NOHIDEOFFLINE) || (style&CLS_HIDEOFFLINE && ! group->hideOffline) || 
					(!DBGetContactSettingByte(NULL,"CList","NoOfflineBottom",SETTING_NOOFFLINEBOTTOM_DEFAULT))
					)
				{
					if (!image_is_special || contact->image_is_special!=image_is_special)
					{
						dat->NeedResort=1;
						NeedResort=1;
					}
				}

				if(!shouldShow && !(style&CLS_NOHIDEOFFLINE) && (style&CLS_HIDEOFFLINE || group->hideOffline)) 
				{
					HANDLE hSelItem;
					struct ClcContact *selcontact;
					struct ClcGroup *selgroup;
					if(GetRowByIndex(dat,dat->selection,&selcontact,NULL)==-1) hSelItem=NULL;
					else hSelItem=ContactToHItem(selcontact);
					pcli->pfnRemoveItemFromGroup(hwnd,group,contact,0);
					contact=NULL;
					if(hSelItem)
						if(FindItem(hwnd,dat,hSelItem,&selcontact,&selgroup,NULL,FALSE))
							dat->selection=GetRowsPriorTo(&dat->list,selgroup,selcontact-selgroup->cl.items[0]);
					recalcScrollBar=1;
					dat->NeedResort=1;
					NeedResort=1;

				}
				else 
				{
					int oldflags;
					contact->iImage=(WORD)lParam;

					oldflags=contact->flags;
					if(!pcli->pfnIsHiddenMode(dat,status)||cacheEntry->noHiddenOffline) contact->flags|=CONTACTF_ONLINE;
					else contact->flags&=~CONTACTF_ONLINE;

					if (oldflags!=contact->flags)
					{
						dat->NeedResort=1;
						NeedResort=1;
						moveToGroup=2;
					}

					//Propagate icon to parent
					if (contact->isSubcontact)
					{
						if (image_is_special)
						{
							SendMessage(hwnd, INTM_ICONCHANGED, (WPARAM) contact->/*by_fyr_*/subcontacts/*by_fyr_*/->hContact, 
								lParam);
						}
						else
						{
							SendMessage(hwnd, INTM_ICONCHANGED, (WPARAM) contact->/*by_fyr_*/subcontacts/*by_fyr_*/->hContact, 
								(LPARAM) GetContactIcon((WPARAM) contact->/*by_fyr_*/subcontacts/*by_fyr_*/->hContact, 0));
						}
					}
				}
			}

			if (contact != NULL)
			{
				if (!IsBadWritePtr(contact, sizeof(struct ClcContact)))
				{
					contact->image_is_special = image_is_special;
					contact->status = status;
					if (DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",0) && moveToGroup)
					{
						if (contact->hContact)
							pcli->pfnClcBroadcast( INTM_GROUPCHANGED,(WPARAM)contact->hContact,0);
					}
				}
#ifdef _DEBUG
				else
				{
					log1("IsBadWritePtr(contact, sizeof(ClcContact)) | INTM_ICONCHANGED  [%08x]", contact);
					break;
				}
#endif
			}
			if (NeedResort)// || image_is_special)
			{
				pcli->pfnSortContacts();
				//SortCLC(hwnd,dat,1);
				//RecalcScrollBar(hwnd,dat);             
			}
			if(recalcScrollBar) 
			{
				RecalcScrollBar(hwnd,dat); 	
			}

			PostMessage(hwnd,INTM_INVALIDATE,0,0);

			//InvalidateItemByHandle(hwnd,(HANDLE)wParam);
			break;
		}

	case INTM_AVATARCHANGED:
		{
			struct ClcContact *contact;
			if (!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) 
				break;

			//ShowTracePopup("INTM_AVATARCHANGED");

			Cache_GetAvatar(dat, contact); 
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		}

	case INTM_TIMEZONECHANGED:
		{
			struct ClcContact *contact;

			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) break;

			if (!IsBadWritePtr(contact, sizeof(struct ClcContact)))
			{
				Cache_GetTimezone(dat,contact);
				Cache_GetText(dat, contact);

				RecalcScrollBar(hwnd,dat);
			}
#ifdef _DEBUG
			else
			{
				log1("IsBadWritePtr(contact, sizeof(ClcContact)) | INTM_TIMEZONECHANGED  [%x]", contact);
			}
#endif

			break;
		}

	case INTM_NAMECHANGED:
		{
			struct ClcContact *contact;
			pcli->pfnInvalidateDisplayNameCacheEntry((HANDLE)wParam);
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,FALSE)) break;

			lstrcpyn(contact->szText, pcli->pfnGetContactDisplayName((HANDLE)wParam,0),sizeof(contact->szText));

			if (!IsBadWritePtr(contact, sizeof(struct ClcContact)))
			{
				Cache_GetText(dat,contact);

				RecalcScrollBar(hwnd,dat);
			}
#ifdef _DEBUG
			else
			{
				log1("IsBadWritePtr(contact, sizeof(ClcContact)) | INTM_NAMECHANGED  [%x]", contact);
			}
#endif

			dat->NeedResort=1;
			//SortCLC(hwnd,dat,1);		
			pcli->pfnSortContacts(); /*SortClcByTimer(hwnd);*/

			break;
		}

	case INTM_STATUSMSGCHANGED:
		{
			struct ClcContact *contact;
			HANDLE hContact = (HANDLE)wParam;

			if (hContact == NULL || IsHContactInfo(hContact) || IsHContactGroup(hContact))
				break;

			if (!FindItem(hwnd,dat,hContact,&contact,NULL,NULL,FALSE)) 
				break;
			//  }

			//ShowTracePopup("INTM_STATUSMSGCHANGED");
			///	case INTM_STATUSMSGCHANGED:
			if (!IsBadWritePtr(contact, sizeof(struct ClcContact)))
			{
				Cache_GetText(dat,contact);
				RecalcScrollBar(hwnd,dat);
				PostMessage(hwnd,INTM_INVALIDATE,0,0);
			}
#ifdef _DEBUG
			else
			{
				log1("IsBadWritePtr(contact, sizeof(ClcContact)) | INTM_STATUSMSGCHANGED  [%08x]", contact);
			}
#endif

			break;
		}

	case INTM_NOTONLISTCHANGED:
		{
			DBCONTACTWRITESETTING *dbcws=(DBCONTACTWRITESETTING*)lParam;
			struct ClcContact *contact;
			if(!FindItem(hwnd,dat,(HANDLE)wParam,&contact,NULL,NULL,TRUE)) break;
			if(contact->type!=CLCIT_CONTACT) break;
			if(dbcws->value.type==DBVT_DELETED || dbcws->value.bVal==0) contact->flags&=~CONTACTF_NOTONLIST;
			else contact->flags|=CONTACTF_NOTONLIST;
			InvalidateRectZ(hwnd,NULL,FALSE);
			break;
		}

	case INTM_STATUSCHANGED:
		TRACE("INTM_STATUSCHANGED\n");

		if (wParam != 0)
		{
			pdisplayNameCacheEntry pdnce = (pdisplayNameCacheEntry)pcli->pfnGetCacheEntry((HANDLE)wParam);
			if (pdnce && pdnce->szProto)
			{
				struct ClcContact *contact;
				pdnce->status = GetStatusForContact(pdnce->hContact,pdnce->szProto);

				//if (!pdnce->ClcContact)
				{

					int *isv={0};
					void *z={0};

					int ret;
					ret=FindItem(hwnd,dat,pdnce->hContact,(struct ClcContact ** )&z,(struct  ClcGroup** )&isv,NULL,0);
					if (ret==0)  
					{
						contact=NULL;
						TRACE("INTM_STATUSCHANGED: OBJRCT NOT FOUND\n");
						pdnce->ClcContact=NULL;
					} //(Not in list yet. or hidden) //strange
					else 
					{
						contact=z;
						pdnce->ClcContact=(void *)z;
					}		
				}
				if (contact) 
				{
					/*if (dat->seco)*/
					//if (pdnce->status!=ID_STATUS_OFFLINE && !lParam && (dat->second_line_type==TEXT_STATUS_MESSAGE || dat->third_line_type==TEXT_STATUS_MESSAGE))
					//	ReAskStatusMessage((HANDLE)wParam);
					contact->status = pdnce->status;
					Cache_GetText(dat,contact);		
				}
			}
		}
		pcli->pfnSortContacts();
		PostMessage(hwnd,INTM_INVALIDATE,0,0);
		break;

	case WM_NCPAINT:
		return 0;

	case WM_PAINT:
		{	HDC hdc;
			PAINTSTRUCT ps;          
			/* we get so many InvalidateRectZ()'s that there is no point painting,
			Windows in theory shouldn't queue up WM_PAINTs in this case but it does so
			we'll just ignore them */

			if (IsWindowVisible(hwnd) && !LOCK_REPAINTING) 
			{
				HWND h;
				h=GetParent(hwnd);  
				if (h!=pcli->hwndContactList || !LayeredFlag)
				{       
					hdc=BeginPaint(hwnd,&ps);
					PaintClc(hwnd,dat,ps.hdc,&ps.rcPaint);
					EndPaint(hwnd,&ps);
				}
				else InvalidateFrameImage((WPARAM)hwnd,0);
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);           
		}
	case WM_KEYDOWN:
		{	
			int selMoved=0;
			int changeGroupExpand=0;
			int pageSize;
			pcli->pfnHideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			if(CallService(MS_CLIST_MENUPROCESSHOTKEY,wParam,MPCF_CONTACTMENU)) break;
			{	RECT clRect;
				GetClientRect(hwnd,&clRect);
				pageSize=clRect.bottom/dat->max_row_height;
			}
			switch(wParam) {
			case VK_DOWN: dat->selection++; selMoved=1; break;
			case VK_UP: dat->selection--; selMoved=1; break;
			case VK_PRIOR: dat->selection-=pageSize; selMoved=1; break;
			case VK_NEXT: dat->selection+=pageSize; selMoved=1; break;
			case VK_HOME: dat->selection=0; selMoved=1; break;
			case VK_END: dat->selection=pcli->pfnGetGroupContentsCount(&dat->list,1)-1; selMoved=1; break;
			case VK_LEFT: changeGroupExpand=1; break;
			case VK_RIGHT: changeGroupExpand=2; break;
			case VK_RETURN: pcli->pfnDoSelectionDefaultAction(hwnd,dat); SetCapture(hwnd); return 0;
			case VK_F2: BeginRenameSelection(hwnd,dat); SetCapture(hwnd);return 0;
			case VK_DELETE: pcli->pfnDeleteFromContactList(hwnd,dat); SetCapture(hwnd);return 0;
			default:
				{	NMKEY nmkey;
					nmkey.hdr.hwndFrom=hwnd;
					nmkey.hdr.idFrom=GetDlgCtrlID(hwnd);
					nmkey.hdr.code=NM_KEYDOWN;
					nmkey.nVKey=wParam;
					nmkey.uFlags=HIWORD(lParam);
					if(SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nmkey)) {SetCapture(hwnd); return 0;}
				}
			}
			if(changeGroupExpand) 
			{
				int hit;
				struct ClcContact *contact;
				struct ClcGroup *group;
				dat->szQuickSearch[0]=0;
				hit=GetRowByIndex(dat,dat->selection,&contact,&group);
				if(hit!=-1) 
				{
					if (contact->type==CLCIT_CONTACT &&(contact->isSubcontact || contact->SubAllocated>0))
					{
						if (contact->isSubcontact && changeGroupExpand==1)
						{
							dat->selection-=contact->isSubcontact;
							selMoved=1;
						}
						else if (!contact->isSubcontact && contact->SubAllocated>0)
						{
							if (changeGroupExpand==1 && !contact->SubExpanded) 
							{
								dat->selection=GetRowsPriorTo(&dat->list,group,-1);
								selMoved=1;   
							}
							else if (changeGroupExpand==1 && contact->SubExpanded)
							{
								//Contract       
								struct ClcContact * ht=NULL;
								KillTimer(hwnd,TIMERID_SUBEXPAND);
								contact->SubExpanded=0;
								DBWriteContactSettingByte(contact->hContact,"CList","Expanded",0);
								ht=contact;
								dat->NeedResort=1;
								SortCLC(hwnd,dat,1);		
								RecalcScrollBar(hwnd,dat);
								hitcontact=NULL;	
							}
							else if (changeGroupExpand==2 && contact->SubExpanded)
							{
								dat->selection++;
								selMoved=1;
							}
							else if (changeGroupExpand==2 && !contact->SubExpanded && dat->expandMeta)
							{
								struct ClcContact * ht=NULL;
								KillTimer(hwnd,TIMERID_SUBEXPAND);
								contact->SubExpanded=1;
								DBWriteContactSettingByte(contact->hContact,"CList","Expanded",1);
								ht=contact;
								dat->NeedResort=1;
								SortCLC(hwnd,dat,1);		
								RecalcScrollBar(hwnd,dat);
								if (ht) 
								{
									int i=0;
									struct ClcContact *contact2;
									struct ClcGroup *group2;
									int k=sizeof(struct ClcContact);
									if(FindItem(hwnd,dat,contact->hContact,&contact2,&group2,NULL,FALSE))
									{
										i=GetRowsPriorTo(&dat->list,group2,((unsigned)contact2-(unsigned)group2->cl.items)/sizeof(struct ClcContact));
										pcli->pfnEnsureVisible(hwnd,dat,i+contact->SubAllocated,0);
									}
								}
								hitcontact=NULL;
							}		
						}
					}

					else
					{
						if(changeGroupExpand==1 && contact->type==CLCIT_CONTACT) {
							if(group==&dat->list) {SetCapture(hwnd); return 0;}
							dat->selection=GetRowsPriorTo(&dat->list,group,-1);
							selMoved=1;
						}
						else {
							if(contact->type==CLCIT_GROUP)
							{
								if (changeGroupExpand==1)
								{
									if (!contact->group->expanded)
									{
										dat->selection--;
										selMoved=1;
									}
									else
									{ 
										pcli->pfnSetGroupExpand(hwnd,dat,contact->group,0);
									}
								}
								else if (changeGroupExpand==2)
								{ 
									pcli->pfnSetGroupExpand(hwnd,dat,contact->group,1);
									dat->selection++;
									selMoved=1;
								}
								else {SetCapture(hwnd);return 0;}
							}//
							//						
						}

					}
				}
				else {SetCapture(hwnd);return 0; }
			}
			if(selMoved) {
				dat->szQuickSearch[0]=0;
				if(dat->selection>=pcli->pfnGetGroupContentsCount(&dat->list,1))
					dat->selection=pcli->pfnGetGroupContentsCount(&dat->list,1)-1;
				if(dat->selection<0) dat->selection=0;
				InvalidateRectZ(hwnd,NULL,FALSE);
				pcli->pfnEnsureVisible(hwnd,dat,dat->selection,0);
				UpdateWindow(hwnd);
				SetCapture(hwnd);
				return 0;
			}
			SetCapture(hwnd);
			break;

		}

	case WM_TIMER:
		{
			if (wParam==TIMERID_REBUILDAFTER)
			{                                                                           
				KillTimer(hwnd,TIMERID_REBUILDAFTER);
#ifdef _DEBUG
				TRACE("Delayed REBUILDAFTER\r\n");
#endif
				//InvalidateRectZ(hwnd,NULL,FALSE);
				SaveStateAndRebuildList(hwnd,dat);
				break;
			}
			else if (wParam==TIMERID_INVALIDATE)
			{
				time_t cur_time=(time(NULL)/60);
				if (cur_time!=dat->last_tick_time)
				{
					InvalidateRectZ(hwnd,NULL,FALSE);
					dat->last_tick_time=cur_time;
				}
				break;
			}
			else if (wParam==TIMERID_DELAYEDRESORTCLC)
			{
				if (!dat) break;
				KillTimer(hwnd,TIMERID_DELAYEDRESORTCLC);
#ifdef _DEBUG
				TRACE("Delayed Sort CLC\r\n");
#endif
				//SetTimer(hwnd,UNLOCK_REPAINT,100,NULL);
				SortCLC(hwnd,dat,1);
				InvalidateRectZ(hwnd,NULL,FALSE);
				//RecalcScrollBar(hwnd,dat);
				break;
			}

			if (wParam==TIMERID_SUBEXPAND) 
			{		
				struct ClcContact * ht=NULL;
				KillTimer(hwnd,TIMERID_SUBEXPAND);
				if (hitcontact && dat->expandMeta)
				{
					if (hitcontact->SubExpanded) hitcontact->SubExpanded=0; else hitcontact->SubExpanded=1;
					DBWriteContactSettingByte(hitcontact->hContact,"CList","Expanded",hitcontact->SubExpanded);
					if (hitcontact->SubExpanded)
					{
						ht=&(hitcontact->subcontacts[hitcontact->SubAllocated-1]);
					}
				}

				dat->NeedResort=1;
				SortCLC(hwnd,dat,1);		
				RecalcScrollBar(hwnd,dat);
				if (ht) 
				{
					int i=0;
					struct ClcContact *contact;
					struct ClcGroup *group;
					int k=sizeof(struct ClcContact);
					if(FindItem(hwnd,dat,hitcontact->hContact,&contact,&group,NULL,FALSE))
					{
						i=GetRowsPriorTo(&dat->list,group,((unsigned)contact-(unsigned)group->cl.items)/sizeof(struct ClcContact));
						pcli->pfnEnsureVisible(hwnd,dat,i+hitcontact->SubAllocated,0);
					}
				}
				hitcontact=NULL;
				break;
			}			

			if(wParam==TIMERID_RENAME)
				BeginRenameSelection(hwnd,dat);
			else if(wParam==TIMERID_DRAGAUTOSCROLL)
				ScrollTo(hwnd,dat,dat->yScroll+dat->dragAutoScrolling*dat->max_row_height*2,0);
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
				it.rcItem.top=RowHeights_GetItemTopY(dat,hit)-dat->yScroll;
				it.rcItem.bottom=it.rcItem.top+dat->row_heights[hit];
				OffsetRect(&it.rcItem,ptClientOffset.x,ptClientOffset.y);
				it.isGroup=contact->type==CLCIT_GROUP;
				it.hItem=contact->type==CLCIT_GROUP?(HANDLE)contact->groupId:contact->hContact;
				it.cbSize=sizeof(it);
				dat->hInfoTipItem=ContactToHItem(contact);				
				NotifyEventHooks(hShowInfoTipEvent,0,(LPARAM)&it);
			}
			break;
		}
	case WM_SETCURSOR: 
		{ 
			int k; 
			if (!IsInMainWindow(hwnd)) return DefWindowProc(hwnd,msg,wParam,lParam);
			if (BehindEdge_State>0)  BehindEdge_Show();
			if (BehindEdgeSettings) UpdateTimer(0);
			k=TestCursorOnBorders();     
			return k?k:DefWindowProc(hwnd,msg,wParam,lParam);
		}
	case WM_LBUTTONDOWN:
		{
			{   
				POINT pt;
				int k=0;
				pt.x = LOWORD(lParam); 
				pt.y = HIWORD(lParam); 
				ClientToScreen(hwnd,&pt);
				k=SizingOnBorder(pt,0);
				if (k) 
				{         
					int io=dat->iHotTrack;
					dat->iHotTrack=0;
					if(dat->exStyle&CLS_EX_TRACKSELECT) 
					{
						pcli->pfnInvalidateItem(hwnd,dat,io);
					}
					if (k && GetCapture()==hwnd) 
					{
						SendMessage(GetParent(hwnd),WM_PARENTNOTIFY,WM_LBUTTONDOWN,lParam);
					}
					return 0;
				}
			}
			//original
			//    case: WM_LBUTTONDOWN:
			{	struct ClcContact *contact;
			struct ClcGroup *group;
			int hit;
			DWORD hitFlags;
			mUpped=0;
			if(GetFocus()!=hwnd) SetFocus(hwnd);
			pcli->pfnHideInfoTip(hwnd,dat);
			KillTimer(hwnd,TIMERID_INFOTIP);
			KillTimer(hwnd,TIMERID_RENAME);
			KillTimer(hwnd,TIMERID_SUBEXPAND);

			pcli->pfnEndRename(hwnd,dat,1);
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
			if(hit!=-1 && contact->type==CLCIT_CONTACT && contact->SubAllocated && !contact->isSubcontact)
				if(hitFlags&CLCHT_ONITEMICON && dat->expandMeta) {
					BYTE doubleClickExpand=DBGetContactSettingByte(NULL,"CLC","MetaDoubleClick",0);

					hitcontact=contact;				
					HitPoint.x= (short)LOWORD(lParam);
					HitPoint.y= (short)HIWORD(lParam);
					mUpped=0;
					// SetCapture(hwnd);
					if ((GetKeyState(VK_SHIFT)&0x8000)||(GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_MENU)&0x8000))
					{
						mUpped=1;
						hitcontact=contact;	
						KillTimer(hwnd,TIMERID_SUBEXPAND);
						SetTimer(hwnd,TIMERID_SUBEXPAND,0,NULL);
					}
					//break;
				}
				else
					hitcontact=NULL;




			if(hit!=-1 && contact->type==CLCIT_GROUP)
				if(hitFlags&CLCHT_ONITEMICON) {
					struct ClcGroup *selgroup;
					struct ClcContact *selcontact;
					dat->selection=GetRowByIndex(dat,dat->selection,&selcontact,&selgroup);
					pcli->pfnSetGroupExpand(hwnd,dat,contact->group,-1);
					if(dat->selection!=-1) {
						dat->selection=GetRowsPriorTo(&dat->list,selgroup,((unsigned)selcontact-(unsigned)selgroup->cl.items)/sizeof(struct ClcContact));
						if(dat->selection==-1) dat->selection=GetRowsPriorTo(&dat->list,contact->group,-1);
					}
					InvalidateRectZ(hwnd,NULL,FALSE);
					UpdateWindow(hwnd);
					break;
				}
				if(hit!=-1 && hitFlags&CLCHT_ONITEMCHECK) {
					NMCLISTCONTROL nm;
					contact->flags^=CONTACTF_CHECKED;
					if(contact->type==CLCIT_GROUP) pcli->pfnSetGroupChildCheckboxes(contact->group,contact->flags&CONTACTF_CHECKED);
					pcli->pfnRecalculateGroupCheckboxes(hwnd,dat);
					InvalidateRectZ(hwnd,NULL,FALSE);
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
				InvalidateRectZ(hwnd,NULL,FALSE);
				if(dat->selection!=-1) pcli->pfnEnsureVisible(hwnd,dat,hit,0);
				UpdateWindow(hwnd);
				if(dat->selection!=-1 && (contact->type==CLCIT_CONTACT || contact->type==CLCIT_GROUP) && !(hitFlags&(CLCHT_ONITEMEXTRA|CLCHT_ONITEMCHECK))) {
					SetCapture(hwnd);
					dat->iDragItem=dat->selection;
					dat->dragStage=DRAGSTAGE_NOTMOVED;
					dat->dragAutoScrolling=0;
				}
				break;
			}
		}		
	case WM_CAPTURECHANGED:
		{
			if ((HWND)lParam!=hwnd)
			{
				if (dat->iHotTrack!=-1)
				{
					int i;
					i=dat->iHotTrack;
					dat->iHotTrack=-1;
					pcli->pfnInvalidateItem(hwnd,dat,i); 
					pcli->pfnHideInfoTip(hwnd,dat);
				}
			}
			break;

		}

	case WM_MOUSEMOVE:
		{
			if (IsInMainWindow(hwnd))
			{
				if (BehindEdgeSettings) UpdateTimer(0);
				TestCursorOnBorders();
				// return k?k:DefWindowProc(hwnd,msg,wParam,lParam);
			}
			if (ProceedDragToScroll(hwnd, (short)HIWORD(lParam))) return 0;
			if(hitcontact!=NULL)
			{
				int x,y,xm,ym;
				x= (short)LOWORD(lParam);
				y= (short)HIWORD(lParam);
				xm=GetSystemMetrics(SM_CXDOUBLECLK);
				ym=GetSystemMetrics(SM_CYDOUBLECLK);
				if(abs(HitPoint.x-x)>xm || abs(HitPoint.y-y)>ym)
				{
					// ReleaseCapture();
					if (mUpped==1)
					{
						KillTimer(hwnd,TIMERID_SUBEXPAND);
						SetTimer(hwnd,TIMERID_SUBEXPAND,0,NULL);
						mUpped=0;
					}
					else                                                   {
						KillTimer(hwnd,TIMERID_SUBEXPAND);
						hitcontact=NULL;
						//SetTimer(hwnd,TIMERID_SUBEXPAND,0,NULL);
						mUpped=0;
					}
				}
			}


			if(dat->iDragItem==-1) {
				int iOldHotTrack=dat->iHotTrack;
				if(dat->hwndRenameEdit!=NULL) break;
				if(GetKeyState(VK_MENU)&0x8000 || GetKeyState(VK_F10)&0x8000) break;
				dat->iHotTrack=HitTest(hwnd,dat,(short)LOWORD(lParam),(short)HIWORD(lParam),NULL,NULL,NULL);
				if(iOldHotTrack!=dat->iHotTrack) {
					if(iOldHotTrack==-1) SetCapture(hwnd);
					if (dat->iHotTrack==-1) ReleaseCapture();
					if(dat->exStyle&CLS_EX_TRACKSELECT) {
						pcli->pfnInvalidateItem(hwnd,dat,iOldHotTrack);
						pcli->pfnInvalidateItem(hwnd,dat,dat->iHotTrack);
					}
					pcli->pfnHideInfoTip(hwnd,dat);
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
				InvalidateRectZ(hwnd,NULL,FALSE);
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
		break;
	case DROPTARGET_ONCONTACT:

		if (ServiceExists(MS_MC_ADDTOMETA))
		{
			struct ClcContact *contSour;
			GetRowByIndex(dat,dat->iDragItem,&contSour,NULL);
			if (contSour->type==CLCIT_CONTACT && MyStrCmp(contSour->proto,"MetaContacts"))
			{
				if (!contSour->isSubcontact)
					hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_DROPUSER));  /// Add to meta
				else
					hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_DROPMETA));
			}

		}
		break;
	case DROPTARGET_ONMETACONTACT:
		if (ServiceExists(MS_MC_ADDTOMETA)) 
		{
			struct ClcContact *contSour,*contDest;
			GetRowByIndex(dat,dat->selection,&contDest,NULL);  
			GetRowByIndex(dat,dat->iDragItem,&contSour,NULL);
			if (contSour->type==CLCIT_CONTACT && MyStrCmp(contSour->proto,"MetaContacts"))
			{
				if (!contSour->isSubcontact)
					hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_DROPUSER));  /// Add to meta
				else
					if  (contSour->subcontacts==contDest)
						hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_DEFAULTSUB)); ///MakeDefault
					else hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_REGROUP));
			}
		}
		break;
	case DROPTARGET_ONSUBCONTACT:
		if (ServiceExists(MS_MC_ADDTOMETA))
		{
			struct ClcContact *contSour,*contDest;
			GetRowByIndex(dat,dat->selection,&contDest,NULL);  
			GetRowByIndex(dat,dat->iDragItem,&contSour,NULL);
			if (contSour->type==CLCIT_CONTACT && MyStrCmp(contSour->proto,"MetaContacts"))
			{
				if (!contSour->isSubcontact)
					hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_DROPUSER));  /// Add to meta
				else
					if (contDest->subcontacts==contSour->subcontacts)
						break;
					else  hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_REGROUP));
			}
		}
		break;
	case DROPTARGET_ONGROUP:
		{
			hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_DROPUSER));
		}				
		break;
	case DROPTARGET_INSERTION:

		hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_DROPUSER));
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
					if(group->parent) 
					{
						struct ClcContact *contSour;
						GetRowByIndex(dat,dat->iDragItem,&contSour,NULL);
						if (!contSour->isSubcontact)
							hNewCursor=LoadCursorA(g_hInst, MAKEINTRESOURCEA(IDC_DROPUSER));
					}
					break;
					}
				}
				SetCursor(hNewCursor);
			}
			break;
		}
	case WM_LBUTTONUP:         
		{    
			if (ExitDragToScroll()) return 0;
			mUpped=1;
			if (hitcontact!=NULL && dat->expandMeta)
			{ 
				BYTE doubleClickExpand=DBGetContactSettingByte(NULL,"CLC","MetaDoubleClick",0);
				SetTimer(hwnd,TIMERID_SUBEXPAND,GetDoubleClickTime()*doubleClickExpand,NULL);
			}
			else
			{
				ReleaseCapture();
			}
			if(dat->iDragItem==-1) break;       
			SetCursor((HCURSOR)GetClassLong(hwnd,GCL_HCURSOR));
			if(dat->exStyle&CLS_EX_TRACKSELECT) {
				dat->iHotTrack=HitTest(hwnd,dat,(short)LOWORD(lParam),(short)HIWORD(lParam),NULL,NULL,NULL);
				if(dat->iHotTrack==-1) ReleaseCapture();
			}
			else 
				if (hitcontact==NULL) ReleaseCapture();
			KillTimer(hwnd,TIMERID_DRAGAUTOSCROLL);
			if(dat->dragStage==(DRAGSTAGE_NOTMOVED|DRAGSTAGEF_MAYBERENAME))
				SetTimer(hwnd,TIMERID_RENAME,GetDoubleClickTime(),NULL);
			else if((dat->dragStage&DRAGSTAGEM_STAGE)==DRAGSTAGE_ACTIVE) {
				POINT pt;
				int target;         
				TCHAR Wording[500];
				int res=0;
				pt.x=(short)LOWORD(lParam); pt.y=(short)HIWORD(lParam);
				target=GetDropTargetInformation(hwnd,dat,pt);
				switch(target) {
				case DROPTARGET_ONSELF:
					break;
				case DROPTARGET_ONCONTACT:
					if (ServiceExists(MS_MC_ADDTOMETA))
					{
						struct ClcContact *contDest, *contSour;
						int res;
						HANDLE handle,hcontact;

						GetRowByIndex(dat,dat->iDragItem,&contSour,NULL);
						GetRowByIndex(dat,dat->selection,&contDest,NULL);
						hcontact=contSour->hContact;
						if (contSour->type==CLCIT_CONTACT)
						{

							if (MyStrCmp(contSour->proto,"MetaContacts"))
							{
								if (!contSour->isSubcontact)
								{
									HANDLE hDest=contDest->hContact;
									_sntprintf(Wording,sizeof(Wording),TranslateT("Do You want contact '%s' to be converted to MetaContact and '%s' be added to it?"),contDest->szText, contSour->szText);
									res=MessageBox(hwnd,Wording,TranslateT("Converting to MetaContact"),MB_OKCANCEL|MB_ICONQUESTION);
									if (res==1)
									{
										handle=(HANDLE)CallService(MS_MC_CONVERTTOMETA,(WPARAM)hDest,0);
										if(!handle) break;
										CallService(MS_MC_ADDTOMETA,(WPARAM)hcontact,(LPARAM)handle);                            
									}
								}
								else
								{
									HANDLE handle,hcontact,hfrom,hdest;
									hcontact=contSour->hContact;
									hfrom=contSour->subcontacts->hContact;
									hdest=contDest->hContact;
									_sntprintf(Wording,sizeof(Wording),TranslateT("Do You want contact '%s' to be converted to MetaContact and '%s' be added to it (remove it from '%s')?"), contDest->szText,contSour->szText,contSour->subcontacts->szText);
									res=MessageBox(hwnd,Wording,TranslateT("Converting to MetaContact (Moving)"),MB_OKCANCEL|MB_ICONQUESTION);
									if (res==1)
									{

										handle=(HANDLE)CallService(MS_MC_CONVERTTOMETA,(WPARAM)hdest,0);
										if(!handle) break;

										CallService(MS_MC_REMOVEFROMMETA,(WPARAM)0,(LPARAM)hcontact);                            
										CallService(MS_MC_ADDTOMETA,(WPARAM)hcontact,(LPARAM)handle);                            
									}
								}
							}

						}
					}
					break;
				case DROPTARGET_ONMETACONTACT:
					{
						struct ClcContact *contDest, *contSour;
						int res;
						GetRowByIndex(dat,dat->iDragItem,&contSour,NULL);
						GetRowByIndex(dat,dat->selection,&contDest,NULL);  
						if (contSour->type==CLCIT_CONTACT)
						{

							if (strcmp(contSour->proto,"MetaContacts"))
							{
								if (!contSour->isSubcontact)
								{   
									HANDLE handle,hcontact;
									hcontact=contSour->hContact;
									handle=contDest->hContact;
									_sntprintf(Wording,sizeof(Wording),TranslateT("Do you want to contact '%s' be added to metacontact '%s'?"),contSour->szText, contDest->szText);
									res=MessageBox(hwnd,Wording,TranslateT("Adding contact to MetaContact"),MB_OKCANCEL|MB_ICONQUESTION);
									if (res==1)
									{

										if(!handle) break;                   
										CallService(MS_MC_ADDTOMETA,(WPARAM)hcontact,(LPARAM)handle);                            
									}
								}
								else
								{
									if (contSour->subcontacts==contDest)
									{   
										HANDLE hsour;
										hsour=contSour->hContact;
										_sntprintf(Wording,sizeof(Wording),TranslateT("Do You want contact '%s' to be default ?"),contSour->szText);
										res=MessageBox(hwnd,Wording,TranslateT("Set default contact"),MB_OKCANCEL|MB_ICONQUESTION);

										if (res==1)
										{
											CallService(MS_MC_SETDEFAULTCONTACT,(WPARAM)contDest->hContact,(LPARAM)hsour);
										}
									}
									else
									{   
										HANDLE handle,hcontact,hfrom;
										hcontact=contSour->hContact;
										hfrom=contSour->subcontacts->hContact;
										handle=contDest->hContact;
										_sntprintf(Wording,sizeof(Wording),TranslateT("Do You want contact '%s' to be removed from MetaContact '%s' and added to '%s'?"), contSour->szText,contSour->subcontacts->szText,contDest->szText);
										res=MessageBox(hwnd,Wording,TranslateT("Changing MetaContacts (Moving)"),MB_OKCANCEL|MB_ICONQUESTION);
										if (res==1)
										{

											if(!handle) break;

											CallService(MS_MC_REMOVEFROMMETA,(WPARAM)0,(LPARAM)hcontact);                            
											CallService(MS_MC_ADDTOMETA,(WPARAM)hcontact,(LPARAM)handle);                            
										}
									}
								}
							}
						}
					}
					break;
				case DROPTARGET_ONSUBCONTACT:
					{
						struct ClcContact *contDest, *contSour;
						int res;
						GetRowByIndex(dat,dat->iDragItem,&contSour,NULL);
						GetRowByIndex(dat,dat->selection,&contDest,NULL);  
						if (contSour->type==CLCIT_CONTACT)
						{
							if (strcmp(contSour->proto,"MetaContacts"))
							{
								if (!contSour->isSubcontact)
								{
									HANDLE handle,hcontact;
									hcontact=contSour->hContact;
									handle=contDest->subcontacts->hContact;
									_sntprintf(Wording,sizeof(Wording),TranslateT("Do You want contact '%s' to be added to MetaContact '%s'?"), contSour->szText,contDest->subcontacts->szText);
									res=MessageBox(hwnd,Wording,TranslateT("Changing MetaContacts (Moving)"),MB_OKCANCEL|MB_ICONQUESTION);
									if (res==1)
									{

										if(!handle) break;                   
										CallService(MS_MC_ADDTOMETA,(WPARAM)hcontact,(LPARAM)handle);                            
									}
								}
								else
								{
									if (contSour->subcontacts!=contDest->subcontacts)
									{    
										HANDLE handle,hcontact,hfrom;
										hcontact=contSour->hContact;
										hfrom=contSour->subcontacts->hContact;
										handle=contDest->subcontacts->hContact;                                     
										_sntprintf(Wording,sizeof(Wording),TranslateT("Do You want contact '%s' to be removed from MetaContact '%s' and added to '%s'?"), contSour->szText,contSour->subcontacts->szText,contDest->subcontacts->szText);
										res=MessageBox(hwnd,Wording,TranslateT("Changing MetaContacts (Moving)"),MB_OKCANCEL|MB_ICONQUESTION);
										if (res==1)
										{

											if(!handle) break;

											CallService(MS_MC_REMOVEFROMMETA,(WPARAM)0,(LPARAM)hcontact);                            
											CallService(MS_MC_ADDTOMETA,(WPARAM)hcontact,(LPARAM)handle); 
										}
									}
								}
							}
						}
					}
					break;
				case DROPTARGET_ONGROUP:
					{	struct ClcContact *contact;
						TCHAR *szGroup;
						GetRowByIndex(dat,dat->selection,&contact,NULL);
						szGroup=(TCHAR*)pcli->pfnGetGroupName(contact->groupId,NULL);
						GetRowByIndex(dat,dat->iDragItem,&contact,NULL);
						if(contact->type==CLCIT_CONTACT)	 //drop is a contact
							if (!contact->isSubcontact || !ServiceExists(MS_MC_ADDTOMETA))
								DBWriteContactSettingTString(contact->hContact,"CList","Group",szGroup);
							else
							{
								HANDLE hcontact,hfrom;
								hcontact=contact->hContact;
								hfrom=contact->subcontacts->hContact;
								_sntprintf(Wording,sizeof(Wording),TranslateT("Do You want contact '%s' to be removed from MetaContact '%s' to group '%s'?"), contact->szText,contact->subcontacts->szText,szGroup);
								res=MessageBox(hwnd,Wording,TranslateT("Changing MetaContact (Removing)"),MB_OKCANCEL|MB_ICONQUESTION);
								if (res==1)
								{

									DBDeleteContactSetting(hcontact,"MetaContacts","OldCListGroup");
									CallService(MS_MC_REMOVEFROMMETA,(WPARAM)0,(LPARAM)hcontact);    
									DBWriteContactSettingTString(hcontact,"CList","Group",szGroup);
								}
							}
						else if(contact->type==CLCIT_GROUP) { //dropee is a group
							TCHAR szNewName[120];
							mir_sntprintf(szNewName,SIZEOF(szNewName),_T("%s\\%s"),szGroup,contact->szText);
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
						if(group->parent) 
						{	 //move to root
							if(contact->type==CLCIT_CONTACT)	
							{
								if (!contact->isSubcontact|| !ServiceExists(MS_MC_ADDTOMETA))
								{
									//dropee is a contact
									DBDeleteContactSetting(contact->hContact,"CList","Group");
									SendMessage(hwnd,CLM_AUTOREBUILD,0,0);
								}
								else 
								{ 
									HANDLE hcontact,hfrom;
									hcontact=contact->hContact;
									hfrom=contact->subcontacts->hContact;
									_sntprintf(Wording,sizeof(Wording),TranslateT("Do You want contact '%s' to be removed from MetaContact '%s'?"), contact->szText,contact->subcontacts->szText);
									res=MessageBox(hwnd,Wording,TranslateT("Changing MetaContact (Removing)"),MB_OKCANCEL|MB_ICONQUESTION);
									if (res==1)
									{

										DBDeleteContactSetting(hcontact,"MetaContacts","OldCListGroup"); 
										CallService(MS_MC_REMOVEFROMMETA,(WPARAM)0,(LPARAM)hcontact);                                                                     
									}
								}
							}
							else if(contact->type==CLCIT_GROUP) { //dropee is a group
								TCHAR szNewName[120];
								lstrcpyn(szNewName,contact->szText,sizeof(szNewName));
								CallService(MS_CLIST_GROUPRENAME,contact->groupId,(LPARAM)szNewName); //TODO: UNICODE
							}
						}
						break;
					}
				}
			}

			InvalidateRectZ(hwnd,NULL,FALSE);
			dat->iDragItem=-1;
			dat->iInsertionMark=-1;
			break;
		}
	case WM_LBUTTONDBLCLK:
		KillTimer(hwnd,TIMERID_SUBEXPAND);
		hitcontact=NULL;
		break;

	case WM_DESTROY:
		FreeDisplayNameCache(&dat->lCLCContactsCache);
		if (!dat->use_avatar_service)
			ImageArray_Free(&dat->avatar_cache, FALSE);

		RowHeights_Free(dat);
		break;
	}

	return saveContactListControlWndProc(hwnd, msg, wParam, lParam);
}
