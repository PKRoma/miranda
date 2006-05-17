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
#include "m_clui.h"
#include "clist.h"

void TrayIconSetToBase(char *szPreferredProto);
static int RemoveEvent(WPARAM wParam,LPARAM lParam);

struct CListEvent {
	int imlIconIndex;
	int flashesDone;
	CLISTEVENT cle;
};
static struct CListEvent *event;
static int eventCount;
static int disableTrayFlash;
static int disableIconFlash;


struct CListImlIcon {
	int index;
	HICON hIcon;
};
static struct CListImlIcon *imlIcon;
static int imlIconCount;
static UINT flashTimerId;
static int iconsOn;
extern HIMAGELIST hCListImages;

static int GetImlIconIndex(HICON hIcon)
{
	int i;
	
	for(i=0;i<imlIconCount;i++) {
		if(imlIcon[i].hIcon==hIcon) return imlIcon[i].index;
	}
	imlIcon=(struct CListImlIcon*)mir_realloc(imlIcon,sizeof(struct CListImlIcon)*(imlIconCount+1));
	imlIconCount++;
	imlIcon[i].hIcon=hIcon;
	imlIcon[i].index=ImageList_AddIcon(hCListImages,hIcon);
	return imlIcon[i].index;
}

static VOID CALLBACK IconFlashTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	int i,j;

	if(eventCount) {
		char *szProto;
		if(event[0].cle.hContact==NULL) szProto=NULL;
		else szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)event[0].cle.hContact,0);
		TrayIconUpdateWithImageList((iconsOn || DBGetContactSettingByte(NULL,"CList","DisableTrayFlash",0))?event[0].imlIconIndex:0,event[0].cle.pszTooltip,szProto);
	}
	for(i=0;i<eventCount;i++) {
		for(j=0;j<i;j++) if(event[j].cle.hContact==event[i].cle.hContact) break;
		if(j<i) continue;
		ChangeContactIcon(event[i].cle.hContact,iconsOn?event[i].imlIconIndex:0,0);
		if(event[i].cle.flags&CLEF_ONLYAFEW) {
			if(0==--event[i].flashesDone) RemoveEvent((WPARAM)event[i].cle.hContact,(LPARAM)event[i].cle.hDbEvent);
		}
	}
	iconsOn=!iconsOn;
}

static int AddEvent(WPARAM wParam,LPARAM lParam)
{
	CLISTEVENT *cle=(CLISTEVENT*)lParam;
	int i;
	TRACE("Called ADD event\n");
	if(cle->cbSize!=sizeof(CLISTEVENT)) return 1;
	event=(struct CListEvent*)mir_realloc(event,sizeof(struct CListEvent)*(eventCount+1));
	if(cle->flags&CLEF_URGENT) {
		for(i=0;i<eventCount;i++) if(!(cle->flags&CLEF_URGENT)) break;
	}
	else i=eventCount;
	MoveMemory(&event[i+1],&event[i],sizeof(struct CListEvent)*(eventCount-i));
	event[i].cle=*cle;
	event[i].imlIconIndex=GetImlIconIndex(event[i].cle.hIcon);
	event[i].flashesDone=12;
	event[i].cle.pszService=mir_strdup(event[i].cle.pszService);  
	event[i].cle.pszTooltip=mir_strdupT(event[i].cle.pszTooltip);//TODO convert from utf to mb
	eventCount++;
	if(eventCount==1) {
		char *szProto;
		if(event[0].cle.hContact==NULL) szProto=NULL;
		else szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)event[0].cle.hContact,0);
		iconsOn=1;
		flashTimerId=SetTimer(NULL,0,DBGetContactSettingWord(NULL,"CList","IconFlashTime",550),IconFlashTimer);
	    TrayIconUpdateWithImageList(iconsOn?event[0].imlIconIndex:0,event[0].cle.pszTooltip,szProto);
	}
	TRACE("Called ChangeContactIcon event\n");
	ChangeContactIcon(cle->hContact,event[eventCount-1].imlIconIndex,1);
	pcli->pfnSortContacts();
	return 0;
}



// Removes an event from the contact list's queue
// wParam=(WPARAM)(HANDLE)hContact
// lParam=(LPARAM)(HANDLE)hDbEvent
// Returns 0 if the event was successfully removed, or nonzero if the event was not found
static int RemoveEvent(WPARAM wParam, LPARAM lParam)
{

	int i;
	char* szProto;


	// Find the event that should be removed
	for (i = 0; i < eventCount; i++)
	{
		if ((event[i].cle.hContact == (HANDLE)wParam) &&
			(event[i].cle.hDbEvent == (HANDLE)lParam))
		{
			break;
		}
	}

	// Event was not found
	if (i == eventCount)
	{
		TRACE("---===   EVENT NOT FOUND   ===---\n");
		return 1;
	}

	// Update contact's icon
	szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	ChangeContactIcon(event[i].cle.hContact, ExtIconFromStatusMode(event[i].cle.hContact,szProto, szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord(event[i].cle.hContact,szProto,"Status",ID_STATUS_OFFLINE)),0); //by FYR
    TrayIconSetToBase((HANDLE)wParam == NULL ? NULL : szProto);

	// Free any memory allocated to the event
	if (event[i].cle.pszService != NULL)
	{ 
		mir_free(event[i].cle.pszService);
		event[i].cle.pszService = NULL;
	}
	if (event[i].cle.pszTooltip != NULL)
	{ 
		mir_free(event[i].cle.pszTooltip);
		event[i].cle.pszTooltip = NULL;
	}

	// Shift events in array
	// The array size is not adjusted until a new event is added though.
	if (eventCount > 1)
		MoveMemory(&event[i], &event[i+1], sizeof(struct CListEvent)*(eventCount-i-1));

	eventCount--;

	if (eventCount == 0)
	{
		KillTimer(NULL, flashTimerId);
		TrayIconSetToBase((HANDLE)wParam == NULL ? NULL : szProto);
	}
    else
	{
		if (event[0].cle.hContact == NULL)
			szProto = NULL;
		else
			szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)event[0].cle.hContact, 0);
		TrayIconUpdateWithImageList(iconsOn ? event[0].imlIconIndex : 0, event[0].cle.pszTooltip, szProto);
	}

	return 0;

}



static int GetEvent(WPARAM wParam,LPARAM lParam)
{
	int i;

	if((HANDLE)wParam==INVALID_HANDLE_VALUE) {
		if(lParam>=eventCount) return (int)(CLISTEVENT*)NULL;
		return (int)&event[lParam].cle;
	}
	for(i=0;i<eventCount;i++)
		if(event[i].cle.hContact==(HANDLE)wParam)
			if(lParam--==0)
				return (int)&event[i].cle;
	return (int)(CLISTEVENT*)NULL;
}

int EventsProcessContactDoubleClick(HANDLE hContact)
{
	int i;
	HANDLE hDbEvent;

	for(i=0;i<eventCount;i++) {
		if(event[i].cle.hContact==hContact) {
			hDbEvent=event[i].cle.hDbEvent;
			CallService(event[i].cle.pszService,(WPARAM)(HWND)NULL,(LPARAM)&event[i].cle);
			RemoveEvent((WPARAM)hContact,(LPARAM)hDbEvent);
			return 0;
		}
	}
	return 1;
}

int EventsProcessTrayDoubleClick(void)
{
	if(eventCount) {
		HANDLE hContact,hDbEvent;
		hContact=event[0].cle.hContact;
		hDbEvent=event[0].cle.hDbEvent;
		CallService(event[0].cle.pszService,(WPARAM)NULL,(LPARAM)&event[0].cle);
		RemoveEvent((WPARAM)hContact,(LPARAM)hDbEvent);
		return 0;
	}
	return 1;
}

static int RemoveEventsForContact(WPARAM wParam, LPARAM lParam)
{
	int j, hit;

	/*
	the for(;;) loop is used here since the eventCount can not be relied upon to take us
	thru the event[] array without suffering from shortsightedness about how many unseen
	events remain, e.g. three events, we remove the first, we're left with 2, the event
	loop exits at 2 and we never see the real new 2.
	*/

	for (;eventCount>0;) {
		for (hit=0, j=0; j<eventCount; j++) {
			if (event[j].cle.hContact==(HANDLE)wParam) {
				RemoveEvent(wParam,(LPARAM)event[j].cle.hDbEvent);
				hit=1;
			}			
		}
		if (j==eventCount && hit==0) return 0; /* got to the end of the array and didnt remove anything */
	}
	
	return 0;
}
static int CListEventSettingsChanged(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact=(HANDLE)wParam;
	DBCONTACTWRITESETTING * cws = (DBCONTACTWRITESETTING *)lParam;
	if ( hContact == NULL && cws && cws->szModule && cws->szSetting 
		&& MyStrCmp(cws->szModule,"CList")==0 ) {
		if ( MyStrCmp(cws->szSetting,"DisableTrayFlash") == 0 )disableTrayFlash = (int) cws->value.bVal;
		else if ( MyStrCmp(cws->szSetting,"NoIconBlink") == 0 )disableIconFlash = (int) cws->value.bVal;
	}
	return 0;
}



int InitCListEvents(void)
{
	event=NULL;
	eventCount=0;
	CreateServiceFunction(MS_CLIST_ADDEVENT,AddEvent);
	CreateServiceFunction(MS_CLIST_REMOVEEVENT,RemoveEvent);
	CreateServiceFunction(MS_CLIST_GETEVENT,GetEvent);
	HookEvent(ME_DB_CONTACT_DELETED,RemoveEventsForContact);

	disableTrayFlash=DBGetContactSettingByte(NULL,"CList","DisableTrayFlash",0);
	disableIconFlash=DBGetContactSettingByte(NULL,"CList","NoIconBlink",0);
	HookEvent(ME_DB_CONTACT_SETTINGCHANGED,CListEventSettingsChanged);


	return 0;
}

void UninitCListEvents(void)
{
	int i;

	for(i=0;i<eventCount;i++) {
		if(event[i].cle.pszService) mir_free(event[i].cle.pszService);
		if(event[i].cle.pszTooltip) mir_free(event[i].cle.pszTooltip);
	}
	if(event!=NULL) mir_free(event);
	if(imlIcon!=NULL) mir_free(imlIcon);
}