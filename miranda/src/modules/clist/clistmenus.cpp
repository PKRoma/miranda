/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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
#pragma hdrstop

#include "clc.h"
#include "genmenu.h"

#define FIRSTCUSTOMMENUITEMID	30000
#define MENU_CUSTOMITEMMAIN		0x80000000
//#define MENU_CUSTOMITEMCONTEXT	0x40000000
//#define MENU_CUSTOMITEMFRAME	0x20000000

typedef struct  {
	WORD id;
	int iconId;
	CLISTMENUITEM mi;
}
	CListIntMenuItem,*lpCListIntMenuItem;

//new menu sys
int hMainMenuObject = 0;
int hContactMenuObject = 0;
int hStatusMenuObject = 0;
int UnloadMoveToGroup(void);

int statustopos(int status);
//
//HIMAGELIST hImlMenuIcons;

HANDLE hPreBuildMainMenuEvent, hStatusModeChangeEvent, hPreBuildContactMenuEvent;

static HANDLE hAckHook;

static HMENU hMainMenu,hStatusMenu = 0;
int statusModeList[ MAX_STATUS_COUNT ] =
{
	ID_STATUS_OFFLINE, ID_STATUS_ONLINE, ID_STATUS_AWAY, ID_STATUS_NA, ID_STATUS_OCCUPIED,
	ID_STATUS_DND, ID_STATUS_FREECHAT, ID_STATUS_INVISIBLE, ID_STATUS_ONTHEPHONE, ID_STATUS_OUTTOLUNCH
};

int skinIconStatusList[ MAX_STATUS_COUNT ] =
{
	SKINICON_STATUS_OFFLINE, SKINICON_STATUS_ONLINE, SKINICON_STATUS_AWAY, SKINICON_STATUS_NA, SKINICON_STATUS_OCCUPIED,
	SKINICON_STATUS_DND, SKINICON_STATUS_FREE4CHAT, SKINICON_STATUS_INVISIBLE, SKINICON_STATUS_ONTHEPHONE, SKINICON_STATUS_OUTTOLUNCH
};

int statusModePf2List[ MAX_STATUS_COUNT ] =
{
	0xFFFFFFFF, PF2_ONLINE, PF2_SHORTAWAY, PF2_LONGAWAY, PF2_LIGHTDND,
	PF2_HEAVYDND, PF2_FREECHAT, PF2_INVISIBLE, PF2_ONTHEPHONE, PF2_OUTTOLUNCH
};

PMO_IntMenuItem* hStatusMainMenuHandles;
int  hStatusMainMenuHandlesCnt;

typedef struct
{
	int protoindex;
	int protostatus[ MAX_STATUS_COUNT ];
	PMO_IntMenuItem menuhandle[ MAX_STATUS_COUNT ];
}
	tStatusMenuHandles,*lpStatusMenuHandles;

lpStatusMenuHandles hStatusMenuHandles;
int hStatusMenuHandlesCnt;

//mainmenu exec param(ownerdata)
typedef struct
{
	char *szServiceName;
	TCHAR *szMenuName;
	int Param1;
}
	MainMenuExecParam,*lpMainMenuExecParam;

//contactmenu exec param(ownerdata)
//also used in checkservice
typedef struct
{
	char *szServiceName;
	char *pszContactOwner;//for check proc
	int param;
}
	ContactMenuExecParam,*lpContactMenuExecParam;

typedef struct
{
	char *szProto;
	int isOnList;
	int isOnline;
}
	BuildContactParam;

typedef struct
{
	char *proto;            //This is unique protoname
	int protoindex;
	int status;

	BOOL custom;
	char *svc;
	HANDLE hMenuItem;
}
	StatusMenuExecParam,*lpStatusMenuExecParam;

typedef struct _MenuItemHandles
{
	HMENU OwnerMenu;
	int position;
}
	MenuItemData;

/////////////////////////////////////////////////////////////////////////////////////////
// service functions

void FreeMenuProtos( void )
{
	int i;

	if ( cli.menuProtos ) {
		for ( i=0; i < cli.menuProtoCount; i++ )
			if ( cli.menuProtos[i].szProto )
				mir_free(cli.menuProtos[i].szProto);

		mir_free( cli.menuProtos );
		cli.menuProtos = NULL;
	}
	cli.menuProtoCount = 0;
}

//////////////////////////////////////////////////////////////////////////

int GetAverageMode()
{
	int netProtoCount,i;
	int averageMode=0;
	int flags, flags2;

	for ( i=0,netProtoCount = 0; i < accounts.getCount(); i++ ) {
		PROTOACCOUNT* pa = accounts[i];
		if ( cli.pfnGetProtocolVisibility( pa->szModuleName ) == 0 )
			continue;

		netProtoCount++;

		flags = CallProtoService( pa->szModuleName, PS_GETCAPS, PFLAGNUM_2, 0 );
		flags2 = CallProtoService( pa->szModuleName, PS_GETCAPS, PFLAGNUM_5, 0 );
		if ((flags & ~flags2) == 0) continue;

		if ( averageMode == 0 )
			averageMode = CallProtoService( pa->szModuleName, PS_GETSTATUS, 0, 0 );
		else if ( averageMode != CallProtoService( pa->szModuleName, PS_GETSTATUS, 0, 0 )) {
			averageMode = -1;
			break;
	}	}

	return averageMode;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MAIN MENU

/*
wparam=handle to the menu item returned by MS_CLIST_ADDCONTACTMENUITEM
return 0 on success.
*/

static int RemoveMainMenuItem(WPARAM wParam, LPARAM)
{
	CallService(MO_REMOVEMENUITEM,wParam,0);
	return 0;
}

static int BuildMainMenu(WPARAM, LPARAM)
{
	ListParam param = { 0 };
	param.MenuObjectHandle = hMainMenuObject;

	HMENU hMenu = hMainMenu;
	NotifyEventHooks(hPreBuildMainMenuEvent,(WPARAM)0,(LPARAM)0);

	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);
	DrawMenuBar((HWND)CallService("CLUI/GetHwnd",(WPARAM)0,(LPARAM)0));
	return (int)hMenu;
}

static int AddMainMenuItem(WPARAM, LPARAM lParam)
{
	CLISTMENUITEM* mi = ( CLISTMENUITEM* )lParam;
	if ( mi->cbSize != sizeof( CLISTMENUITEM ))
		return NULL;

	TMO_MenuItem tmi = { 0 };
	tmi.cbSize   = sizeof(tmi);
	tmi.flags    = mi->flags;
	tmi.hIcon    = mi->hIcon;
	tmi.hotKey   = mi->hotKey;
	tmi.ptszName = mi->ptszName;
	tmi.position = mi->position;

	//pszPopupName for new system mean root level
	//pszPopupName for old system mean that exists popup
	tmi.root = ( HGENMENU )mi->pszPopupName;
	{
		lpMainMenuExecParam mmep;
		mmep = ( lpMainMenuExecParam )mir_alloc( sizeof( MainMenuExecParam ));
		if ( mmep == NULL )
			return 0;

		//we need just one parametr.
		mmep->szServiceName = mir_strdup(mi->pszService);
		mmep->Param1 = mi->popupPosition;
		mmep->szMenuName = tmi.ptszName;
		tmi.ownerdata=mmep;
	}

	PMO_IntMenuItem pimi = MO_AddNewMenuItem( hMainMenuObject, &tmi );
	MO_SetOptionsMenuItem( pimi, OPT_MENUITEMSETUNIQNAME, ( int )mi->pszService );
	return ( int )pimi;
}

int MainMenuCheckService(WPARAM, LPARAM)
{
	return 0;
}

//called with:
//wparam - ownerdata
//lparam - lparam from winproc
int MainMenuExecService(WPARAM wParam, LPARAM lParam)
{
	lpMainMenuExecParam mmep = ( lpMainMenuExecParam )wParam;
	if ( mmep != NULL ) {
		// bug in help.c,it used wparam as parent window handle without reason.
		if ( !lstrcmpA(mmep->szServiceName,"Help/AboutCommand"))
			mmep->Param1 = 0;

		CallService(mmep->szServiceName,mmep->Param1,lParam);
	}
	return 1;
}

int FreeOwnerDataMainMenu(WPARAM, LPARAM lParam)
{
	lpMainMenuExecParam mmep = ( lpMainMenuExecParam )lParam;
	if ( mmep != NULL ) {
		FreeAndNil(( void** )&mmep->szServiceName);
		FreeAndNil(( void** )&mmep);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CONTACT MENU

static int RemoveContactMenuItem(WPARAM wParam, LPARAM)
{
	CallService(MO_REMOVEMENUITEM,wParam,0);
	return 0;
}

static int AddContactMenuItem(WPARAM, LPARAM lParam)
{
	CLISTMENUITEM *mi=(CLISTMENUITEM*)lParam;
	if ( mi->cbSize != sizeof( CLISTMENUITEM ))
		return 0;

	TMO_MenuItem tmi = { 0 };
	tmi.cbSize = sizeof(tmi);
	tmi.flags = mi->flags;
	tmi.hIcon = mi->hIcon;
	tmi.hotKey = mi->hotKey;
	tmi.position = mi->position;
	tmi.ptszName = mi->ptszName;
	tmi.root = ( HGENMENU )mi->pszPopupName;

	if ( !( mi->flags & CMIF_ROOTHANDLE )) {
		//old system
		tmi.flags |= CMIF_ROOTHANDLE;
		tmi.root = NULL;
	}

	//owner data
	lpContactMenuExecParam cmep = ( lpContactMenuExecParam )mir_calloc(sizeof(ContactMenuExecParam));
	cmep->szServiceName = mir_strdup( mi->pszService );
	if ( mi->pszContactOwner != NULL )
		cmep->pszContactOwner = mir_strdup( mi->pszContactOwner );
	cmep->param = mi->popupPosition;
	tmi.ownerdata = cmep;

	//may be need to change how UniqueName is formed?
	PMO_IntMenuItem menuHandle = MO_AddNewMenuItem( hContactMenuObject, &tmi );
	char buf[ 256 ];
	if (mi->pszService)
		sprintf( buf,"%s/%s", (mi->pszContactOwner) ? mi->pszContactOwner : "", (mi->pszService) ? mi->pszService : "" );
	else if (mi->ptszName)
	{
		if (tmi.flags&CMIF_UNICODE)
		{
			char * temp=t2a(mi->ptszName);
			sprintf( buf,"%s/NoService/%s", (mi->pszContactOwner) ? mi->pszContactOwner : "", temp );
			mir_free(temp);
		}
		else
			sprintf( buf,"%s/NoService/%s", (mi->pszContactOwner) ? mi->pszContactOwner : "", mi->ptszName );
	}
	else buf[0]='\0';
	if (buf[0]) MO_SetOptionsMenuItem( menuHandle, OPT_MENUITEMSETUNIQNAME, ( int )buf );
	return ( int )menuHandle;
}

static int BuildContactMenu(WPARAM wParam, LPARAM)
{
	HANDLE hContact = ( HANDLE )wParam;
	NotifyEventHooks(hPreBuildContactMenuEvent,(WPARAM)hContact,0);

	char *szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);

	BuildContactParam bcp;
	bcp.szProto = szProto;
	bcp.isOnList = ( DBGetContactSettingByte(hContact,"CList","NotOnList",0) == 0 );
	bcp.isOnline = ( szProto != NULL && ID_STATUS_OFFLINE != DBGetContactSettingWord(hContact,szProto,"Status",ID_STATUS_OFFLINE));

	ListParam param = { 0 };
	param.MenuObjectHandle = hContactMenuObject;
	param.wParam = (WPARAM)&bcp;

	HMENU hMenu = CreatePopupMenu();
	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);

	return (int)hMenu;
}

//called with:
//wparam - ownerdata
//lparam - lparam from winproc
int ContactMenuExecService(WPARAM wParam,LPARAM lParam)
{
	if (wParam!=0) {
		lpContactMenuExecParam cmep=(lpContactMenuExecParam)wParam;
		//call with wParam=(WPARAM)(HANDLE)hContact,lparam=popupposition
		CallService(cmep->szServiceName,lParam,cmep->param);
	}
	return 0;
}

//true - ok,false ignore
int ContactMenuCheckService(WPARAM wParam,LPARAM)
{
	PCheckProcParam pcpp = ( PCheckProcParam )wParam;
	BuildContactParam *bcp=NULL;
	lpContactMenuExecParam cmep=NULL;
	TMO_MenuItem mi;

	if ( pcpp == NULL )
		return FALSE;

	bcp = ( BuildContactParam* )pcpp->wParam;
	if ( bcp == NULL )
		return FALSE;

	cmep = ( lpContactMenuExecParam )pcpp->MenuItemOwnerData;
	if ( cmep == NULL ) //this is root...build it
		return TRUE;

	if ( cmep->pszContactOwner != NULL ) {
		if ( bcp->szProto == NULL ) return FALSE;
		if ( strcmp( cmep->pszContactOwner, bcp->szProto )) return FALSE;
	}
	if ( MO_GetMenuItem(( WPARAM )pcpp->MenuItemHandle, ( LPARAM )&mi ) == 0 ) {
		if ( mi.flags & CMIF_HIDDEN ) return FALSE;
		if ( mi.flags & CMIF_NOTONLIST  && bcp->isOnList  ) return FALSE;
		if ( mi.flags & CMIF_NOTOFFLIST && !bcp->isOnList ) return FALSE;
		if ( mi.flags & CMIF_NOTONLINE  && bcp->isOnline  ) return FALSE;
		if ( mi.flags & CMIF_NOTOFFLINE && !bcp->isOnline ) return FALSE;
	}
	return TRUE;
}

int FreeOwnerDataContactMenu (WPARAM, LPARAM lParam)
{
	lpContactMenuExecParam cmep = ( lpContactMenuExecParam )lParam;
	if ( cmep != NULL ) {
		FreeAndNil(( void** )&cmep->szServiceName);
		FreeAndNil(( void** )&cmep->pszContactOwner);
		FreeAndNil(( void** )&cmep);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// STATUS MENU

BOOL FindMenuHandleByGlobalID(HMENU hMenu, PMO_IntMenuItem id, MenuItemData* itdat)
{
	int i;
	PMO_IntMenuItem pimi;
	MENUITEMINFO mii={0};
	BOOL inSub=FALSE;
	if (!itdat)
		return FALSE;

	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_SUBMENU | MIIM_DATA;
	for ( i = GetMenuItemCount( hMenu )-1; i >= 0; i-- ) {
		GetMenuItemInfo(hMenu,i,TRUE,&mii);
		if ( mii.fType == MFT_SEPARATOR )
			continue;
		if ( mii.hSubMenu )
			inSub = FindMenuHandleByGlobalID(mii.hSubMenu, id, itdat);
		if ( inSub )
			return inSub;

		pimi = MO_GetIntMenuItem(( HGENMENU )mii.dwItemData );
		if ( pimi != NULL ) {
			if ( pimi == id ) {
				itdat->OwnerMenu = hMenu;
				itdat->position = i;
				return TRUE;
	}	}	}

	return FALSE;
}

BOOL __inline wildcmp(char * name, char * mask)
{
	char * last='\0';
	for(;; mask++, name++)
	{
		if(*mask != '?' && *mask != *name) break;
		if(*name == '\0') return ((BOOL)!*mask);
	}
	if(*mask != '*') return FALSE;
	for(;; mask++, name++)
	{
		while(*mask == '*')
		{
			last = mask++;
			if(*mask == '\0') return ((BOOL)!*mask);   /* true */
		}
		if(*name == '\0') return ((BOOL)!*mask);      /* *mask == EOS */
		if(*mask != '?' && *mask != *name) name -= (size_t)(mask - last) - 1, mask = last;
}	}

int StatusMenuCheckService(WPARAM wParam, LPARAM)
{
	PCheckProcParam pcpp = ( PCheckProcParam )wParam;
	if ( !pcpp )
		return TRUE;

	PMO_IntMenuItem timi = MO_GetIntMenuItem( pcpp->MenuItemHandle );
	if ( !timi )
		return TRUE;

	StatusMenuExecParam *smep = ( StatusMenuExecParam* )pcpp->MenuItemOwnerData;
	if (smep && !smep->status && smep->custom ) {
		if ( wildcmp( smep->svc,"*XStatus*" )) {
			//TODO Set parent icon/text as current
			//Get parent menu ID
			BOOL check = FALSE;
			int XStatus = CallProtoService(smep->proto,"/GetXStatus",0,0);
			{
				char buf[255];
				_snprintf( buf, sizeof(buf), "*XStatus%d", XStatus );
				if ( wildcmp( smep->svc, buf ))
					check = TRUE;
			}
			BOOL reset = (wildcmp(smep->svc,"*XStatus0")) ? TRUE : FALSE;

			if ( check )
				timi->mi.flags |= CMIF_CHECKED;
			else
				timi->mi.flags &= ~CMIF_CHECKED;
			if ( timi->mi.flags & CMIF_CHECKED || reset || check ) {
				PMO_IntMenuItem timiParent = MO_GetIntMenuItem( timi->mi.root );
				if ( timiParent ) {
					CLISTMENUITEM mi2 = {0};
					mi2.cbSize = sizeof(mi2);
					mi2.flags = CMIM_NAME | CMIF_TCHAR;
					mi2.ptszName = (timi->mi.hIcon) ? timi->mi.ptszName : TranslateT("Custom status");

					timiParent = MO_GetIntMenuItem( timi->mi.root );
					{
						MENUITEMINFO mi={0};
						int res=0;
						TCHAR d[200];
						BOOL m;
						MenuItemData it={0};
						mi.cbSize = sizeof(mi);
						mi.fMask = MIIM_STRING;
						m = FindMenuHandleByGlobalID(hStatusMenu, timiParent, &it );
						if ( m ) {
							GetMenuString(it.OwnerMenu,it.position,d,100,MF_BYPOSITION);
							mi.fMask = MIIM_STRING|MIIM_BITMAP;
							mi.fType = MFT_STRING;
							mi.hbmpChecked = mi.hbmpItem = mi.hbmpUnchecked = (HBITMAP)mi2.hIcon;
							mi.dwTypeData = mi2.ptszName;
							mi.hbmpItem = HBMMENU_CALLBACK;
							res = SetMenuItemInfo(it.OwnerMenu,it.position,TRUE,&mi);
					}	}

					CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)timi->mi.root, (LPARAM)&mi2);
					timiParent->iconId = timi->iconId;
		}	}	}
	}
	else if ( smep && smep->status && !smep->custom ) {
		int curProtoStatus = ( smep->proto ) ? CallProtoService(smep->proto,PS_GETSTATUS,0,0) : GetAverageMode();
		if ( smep->status == curProtoStatus )
			timi->mi.flags |= CMIF_CHECKED;
		else
			timi->mi.flags &= ~CMIF_CHECKED;
	}
	else if (( !smep || smep->proto ) && timi->mi.pszName ) {
		int curProtoStatus=0;
		BOOL IconNeedDestroy=FALSE;
		char* prot;
		if (smep)
			prot = smep->proto;
		else
		{
			#ifdef UNICODE
				char *prn=u2a(timi->mi.ptszName);
				prot = NEWSTR_ALLOCA( prn );
				if (prn) mir_free(prn);
			#else
				prot = timi->mi.ptszName;
			#endif
		}
		if ( Proto_GetAccount( prot ) == NULL )
			return TRUE;

		if (( curProtoStatus = CallProtoService(prot,PS_GETSTATUS,0,0)) == CALLSERVICE_NOTFOUND )
			curProtoStatus = 0;

		if ( curProtoStatus >= ID_STATUS_OFFLINE && curProtoStatus < ID_STATUS_IDLE )
			timi->mi.hIcon = LoadSkinProtoIcon(prot,curProtoStatus);
		else {
			timi->mi.hIcon=(HICON)CallProtoService(prot,PS_LOADICON,PLI_PROTOCOL|PLIF_SMALL,0);
			if ( timi->mi.hIcon == (HICON)CALLSERVICE_NOTFOUND )
				timi->mi.hIcon = NULL;
			else
				IconNeedDestroy = TRUE;
		}

		if (timi->mi.hIcon) {
			timi->mi.flags |= CMIM_ICON;
			MO_ModifyMenuItem( timi, &timi->mi );
			if ( IconNeedDestroy ) {
				DestroyIcon( timi->mi.hIcon );
				timi->mi.hIcon = NULL;
			}
			else IconLib_ReleaseIcon(timi->mi.hIcon,0);
	}	}

	return TRUE;
}

int StatusMenuExecService(WPARAM wParam, LPARAM)
{
	lpStatusMenuExecParam smep = ( lpStatusMenuExecParam )wParam;
	if ( smep != NULL ) {
		if ( smep->custom ) {
			if (smep->svc && *smep->svc)
				CallService(smep->svc, 0, (LPARAM)smep->hMenuItem);
		}
		else {
			if ( smep->status == 0 && smep->protoindex !=0 && smep->proto != NULL ) {
				PMO_IntMenuItem pimi;
				char *prot = smep->proto;
				char szHumanName[64]={0};
				PROTOACCOUNT * acc = ProtoGetAccount( smep->proto );
				int i=(DBGetContactSettingByte(NULL,prot,"LockMainStatus",0)?0:1);
				DBWriteContactSettingByte(NULL,prot,"LockMainStatus",(BYTE)i);

				CallProtoService( smep->proto, PS_GETNAME, (WPARAM)SIZEOF(szHumanName), (LPARAM)szHumanName );
				pimi = MO_GetIntMenuItem(( HGENMENU )smep->protoindex );
				if ( i ) {
					TCHAR buf[256];
					pimi->mi.flags|=CMIF_CHECKED;
					if ( pimi->mi.pszName )
						mir_free( pimi->mi.pszName );
					if ( cli.bDisplayLocked ) {
						_sntprintf(buf,SIZEOF(buf),TranslateT("%s (locked)"),acc->tszAccountName);
						pimi->mi.ptszName = mir_tstrdup( buf );
					}
					else pimi->mi.ptszName =  mir_tstrdup( acc->tszAccountName );
				}
				else {
					if ( pimi->mi.pszName )
						mir_free( pimi->mi.pszName );
					pimi->mi.ptszName = mir_tstrdup( acc->tszAccountName );
					pimi->mi.flags &= ~CMIF_CHECKED;
				}
				if ( cli.hwndStatus )
					InvalidateRect( cli.hwndStatus, NULL, TRUE );
			}
			else if ( smep->proto != NULL ) {
				CallProtoService(smep->proto,PS_SETSTATUS,smep->status,0);
				NotifyEventHooks(hStatusModeChangeEvent, smep->status, (LPARAM)smep->proto);
			}
			else {
				int MenusProtoCount = 0;

				for( int i=0; i < accounts.getCount(); i++ )
					if ( cli.pfnGetProtocolVisibility( accounts[i]->szModuleName ))
						MenusProtoCount++;

				cli.currentDesiredStatusMode = smep->status;

				for ( int j=0; j < accounts.getCount(); j++ ) {
					PROTOACCOUNT* pa = accounts[j];
					if ( !cli.pfnGetProtocolVisibility( pa->szModuleName ))
						continue;
					if ( MenusProtoCount > 1 && DBGetContactSettingByte( NULL, pa->szModuleName, "LockMainStatus", 0 ))
						continue;

					CallProtoService( pa->szModuleName, PS_SETSTATUS, cli.currentDesiredStatusMode, 0 );
				}
				NotifyEventHooks( hStatusModeChangeEvent, cli.currentDesiredStatusMode, 0 );

				DBWriteContactSettingWord( NULL, "CList", "Status", ( WORD )cli.currentDesiredStatusMode );
				return 1;
	}	}	}

	return 0;
}

int FreeOwnerDataStatusMenu(WPARAM, LPARAM lParam)
{
	lpStatusMenuExecParam smep = (lpStatusMenuExecParam)lParam;
	if ( smep != NULL ) {
		FreeAndNil(( void** )&smep->proto);
		FreeAndNil(( void** )&smep->svc);
		FreeAndNil(( void** )&smep);
	}

	return (0);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Other menu functions

//wparam MenuItemHandle
static int ModifyCustomMenuItem(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM *mi=(CLISTMENUITEM*)lParam;
	TMO_MenuItem tmi;

	if ( lParam == 0 )
		return -1;
	if ( mi->cbSize != sizeof( CLISTMENUITEM ))
		return 1;

	tmi.cbSize = sizeof(tmi);
	tmi.flags = mi->flags;
	tmi.hIcon = mi->hIcon;
	tmi.hotKey = mi->hotKey;
	tmi.ptszName = mi->ptszName;
	return MO_ModifyMenuItem(( PMO_IntMenuItem )wParam, &tmi );
}

int MenuProcessCommand(WPARAM wParam,LPARAM lParam)
{
	WORD cmd = LOWORD(wParam);

	if ( HIWORD(wParam) & MPCF_MAINMENU ) {
		int hst = LOWORD( wParam );
		if ( hst >= ID_STATUS_OFFLINE && hst <= ID_STATUS_OUTTOLUNCH ) {
			int pos = statustopos( hst );
			if ( pos != -1 && hStatusMainMenuHandles != NULL )
				return MO_ProcessCommand( hStatusMainMenuHandles[ pos ], lParam );
		}	}

	if ( !( cmd >= CLISTMENUIDMIN && cmd <= CLISTMENUIDMAX   ))
		return 0; // DO NOT process ids outside from clist menu id range		v0.7.0.27+

	//process old menu sys
	if ( HIWORD(wParam) & MPCF_CONTACTMENU )
		return MO_ProcessCommandBySubMenuIdent( hContactMenuObject, LOWORD(wParam), lParam );

	//unknown old menu
	return MO_ProcessCommandByMenuIdent( LOWORD(wParam), lParam );
}

BOOL FindMenuHanleByGlobalID(HMENU hMenu, PMO_IntMenuItem id, MenuItemData* itdat)
{
	int i;
	PMO_IntMenuItem pimi;
	MENUITEMINFO mii = {0};
	BOOL inSub=FALSE;

	if ( !itdat )
		return FALSE;

	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_SUBMENU | MIIM_DATA;
	for ( i = GetMenuItemCount( hMenu )-1; i >= 0; i-- ) {
		GetMenuItemInfo( hMenu, i, TRUE, &mii );
		if ( mii.fType == MFT_SEPARATOR )
			continue;

		if ( mii.hSubMenu )
			inSub = FindMenuHanleByGlobalID( mii.hSubMenu, id, itdat );
		if (inSub)
			return inSub;

		pimi = MO_GetIntMenuItem(( HGENMENU )mii.dwItemData);
		if ( pimi != NULL ) {
			if ( pimi == id ) {
				itdat->OwnerMenu = hMenu;
				itdat->position = i;
				return TRUE;
	}	}	}

	return FALSE;
}

static int MenuProcessHotkey(WPARAM vKey, LPARAM)
{
	if ( MO_ProcessHotKeys( hStatusMenuObject, vKey ))
		return TRUE;

	if ( MO_ProcessHotKeys( hMainMenuObject, vKey ))
		return TRUE;

	return 0;
}

static int MenuIconsChanged(WPARAM, LPARAM)
{
	//just rebuild menu
	RebuildMenuOrder();
	cli.pfnCluiProtocolStatusChanged(0,0);
	return 0;
}

static int MeasureMenuItem(WPARAM, LPARAM lParam)
{
	return MO_MeasureMenuItem(( LPMEASUREITEMSTRUCT )lParam );
}

static int DrawMenuItem(WPARAM, LPARAM lParam)
{
	return MO_DrawMenuItem(( LPDRAWITEMSTRUCT )lParam );
}

int RecursiveDeleteMenu(HMENU hMenu)
{
	int i=GetMenuItemCount(hMenu);
	while (i>0)
	{
		HMENU submenu=GetSubMenu(hMenu,0);
		if (submenu)
		{
			RecursiveDeleteMenu(submenu);
			DestroyMenu(submenu);
		}
		DeleteMenu(hMenu,0,MF_BYPOSITION);
		i=GetMenuItemCount(hMenu);
	}
	return 0;
}

static int MenuGetMain(WPARAM, LPARAM)
{
	RecursiveDeleteMenu(hMainMenu);
	BuildMainMenu(0,0);
	return (int)hMainMenu;
}

static int BuildStatusMenu(WPARAM, LPARAM)
{
	ListParam param = { 0 };
	param.MenuObjectHandle = hStatusMenuObject;

	HMENU hMenu=hStatusMenu;
	RecursiveDeleteMenu(hStatusMenu);
	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);
	return (int)hMenu;
}

static int SetStatusMode(WPARAM wParam, LPARAM)
{
	MenuProcessCommand(MAKEWPARAM(LOWORD(wParam), MPCF_MAINMENU), 0);
	return 0;
}

int fnGetProtocolVisibility(const char* accName)
{
	if ( accName ) {
		PROTOACCOUNT* pa = Proto_GetAccount( accName );
		if ( pa )
			return ( pa->bIsVisible && IsAccountEnabled( pa )) ? TRUE : FALSE;
	}

	return FALSE;
}

int fnGetProtoIndexByPos(PROTOCOLDESCRIPTOR ** proto, int protoCnt, int Pos)
{
	int p;
	char buf[10];
	DBVARIANT dbv;

	_itoa( Pos, buf, 10 );
	if ( !DBGetContactSetting( NULL, "Protocols", buf, &dbv )) {
		for ( p=0; p < protoCnt; p++ ) {
			if ( lstrcmpA( proto[p]->szName, dbv.pszVal ) == 0 ) {
				DBFreeVariant( &dbv );
				return p;
		}	}

		DBFreeVariant( &dbv );
	}

	return -1;
}

int fnGetAccountIndexByPos(int Pos)
{
	int i;
	for ( i=0; i < accounts.getCount(); i++ )
		if ( accounts[i]->iOrder == Pos )
			return i;

	return -1;
}

void RebuildMenuOrder( void )
{
	int i,j,s;
	int visnetworkProtoCount=0;
	DWORD flags,flags2;
	TMenuParam tmp;
	int pos=0;
	PROTOACCOUNT* pa;
	BYTE bHideStatusMenu = DBGetContactSettingByte( NULL, "CLUI", "DontHideStatusMenu", 0 ); // cool perversion, though

	//clear statusmenu
	while ( GetMenuItemCount(hStatusMenu) > 0 )
		DeleteMenu(hStatusMenu,0,MF_BYPOSITION);

	//status menu
	if ( hStatusMenuObject != 0 ) {
		CallService(MO_REMOVEMENUOBJECT,hStatusMenuObject,0);
		if ( hStatusMainMenuHandles != NULL)
			mir_free( hStatusMainMenuHandles );

		if ( hStatusMenuHandles != NULL )
			mir_free( hStatusMenuHandles );
	}
	memset(&tmp,0,sizeof(tmp));
	tmp.cbSize = sizeof(tmp);
	tmp.ExecService = "StatusMenuExecService";
	tmp.CheckService = "StatusMenuCheckService";
	tmp.name = "StatusMenu";

	hStatusMenuObject=(int)CallService(MO_CREATENEWMENUOBJECT,(WPARAM)0,(LPARAM)&tmp);
	MO_SetOptionsMenuObject( hStatusMenuObject, OPT_MENUOBJECT_SET_FREE_SERVICE, (int)"CLISTMENUS/FreeOwnerDataStatusMenu" );

	hStatusMainMenuHandles = ( PMO_IntMenuItem* )mir_alloc( SIZEOF(statusModeList) * sizeof( int ));
	hStatusMainMenuHandlesCnt = SIZEOF(statusModeList);
	memset( hStatusMainMenuHandles, 0, SIZEOF(statusModeList) * sizeof( int ));

	hStatusMenuHandles = ( tStatusMenuHandles* )mir_alloc(sizeof(tStatusMenuHandles)*accounts.getCount());
	hStatusMenuHandlesCnt = accounts.getCount();
	memset( hStatusMenuHandles, 0, sizeof(tStatusMenuHandles)*accounts.getCount() );

	for ( i=0; i < accounts.getCount(); i++ ) {
		pa = accounts[i];
		if ( CallProtoService( pa->szModuleName, PS_GETCAPS, PFLAGNUM_2, 0 ) == 0 )
			continue;

		if ( !bHideStatusMenu && cli.pfnGetProtocolVisibility( pa->szModuleName ))
			visnetworkProtoCount++;
	}

	FreeMenuProtos();

	for ( s=0; s < accounts.getCount(); s++ ) {
		i = cli.pfnGetAccountIndexByPos( s );
		if ( i == -1 )
			continue;

		pa = accounts[i];
		pos = 0;
		if ( !bHideStatusMenu && !cli.pfnGetProtocolVisibility( pa->szModuleName ))
			continue;

		flags = CallProtoService( pa->szModuleName, PS_GETCAPS,PFLAGNUM_2, 0 );
		flags2 = CallProtoService( pa->szModuleName, PS_GETCAPS,PFLAGNUM_5, 0 );
		flags &= ~flags2;
		if (flags == 0)
			continue;

		int j;
		HICON ic;

		//adding root
		TMO_MenuItem tmi = { 0 };
		tmi.cbSize = sizeof(tmi);
		tmi.flags = CMIF_TCHAR | CMIF_ROOTHANDLE;
		tmi.position = pos++;
		tmi.hIcon=(HICON)CallProtoService( pa->szModuleName, PS_LOADICON, PLI_PROTOCOL | PLIF_SMALL, 0 );
		ic = tmi.hIcon;
		tmi.root = NULL;
		tmi.ptszName = pa->tszAccountName;
		{
			//owner data
			lpStatusMenuExecParam smep = ( lpStatusMenuExecParam )mir_calloc( sizeof( StatusMenuExecParam ));
			smep->proto = mir_strdup(pa->szModuleName);
			tmi.ownerdata = smep;
		}
		PMO_IntMenuItem rootmenu = MO_AddNewMenuItem( hStatusMenuObject, &tmi );

		memset(&tmi,0,sizeof(tmi));
		tmi.cbSize = sizeof(tmi);
		tmi.flags = CMIF_TCHAR | CMIF_ROOTHANDLE;
		tmi.root = rootmenu;
		tmi.position = pos++;
		tmi.hIcon = ic;
		{
			//owner data
			lpStatusMenuExecParam smep = ( lpStatusMenuExecParam )mir_alloc( sizeof( StatusMenuExecParam ));
			memset( smep, 0, sizeof( *smep ));
			smep->proto = mir_strdup(pa->szModuleName);
			tmi.ownerdata = smep;
		}

		if ( DBGetContactSettingByte( NULL, pa->szModuleName, "LockMainStatus", 0 ))
			tmi.flags |= CMIF_CHECKED;

		if (( tmi.flags & CMIF_CHECKED ) && cli.bDisplayLocked )
		{
			TCHAR buf[256];
			_sntprintf( buf, SIZEOF(buf), TranslateT("%s (locked)"), pa->tszAccountName );
			tmi.ptszName = buf;
		} else {
			tmi.ptszName = pa->tszAccountName;
		}

		PMO_IntMenuItem menuHandle = MO_AddNewMenuItem( hStatusMenuObject, &tmi );
		((lpStatusMenuExecParam)tmi.ownerdata)->protoindex = ( int )menuHandle;
		MO_ModifyMenuItem( menuHandle, &tmi );

		if (cli.menuProtos)
			cli.menuProtos=(MenuProto*)mir_realloc(cli.menuProtos,sizeof(MenuProto)*(cli.menuProtoCount+1));
		else
			cli.menuProtos=(MenuProto*)mir_alloc(sizeof(MenuProto));
		memset(&(cli.menuProtos[cli.menuProtoCount]),0,sizeof(MenuProto));
		cli.menuProtos[cli.menuProtoCount].pMenu = rootmenu;
		cli.menuProtos[cli.menuProtoCount].szProto = mir_strdup(pa->szModuleName);

		cli.menuProtoCount++;
		{
			char buf[256];
			sprintf( buf, "RootProtocolIcon_%s", pa->szModuleName );
			MO_SetOptionsMenuItem( menuHandle, OPT_MENUITEMSETUNIQNAME, ( int )buf );
		}
		DestroyIcon(tmi.hIcon);
		pos += 100000;

		for ( j=0; j < SIZEOF(statusModeList); j++ ) {
			if ( !( flags & statusModePf2List[j] ))
				continue;

			//adding
			memset( &tmi, 0, sizeof( tmi ));
			tmi.cbSize = sizeof(tmi);
			tmi.flags = CMIF_ROOTHANDLE | CMIF_TCHAR;
			if ( statusModeList[j] == ID_STATUS_OFFLINE )
				tmi.flags |= CMIF_CHECKED;
			tmi.root = rootmenu;
			tmi.position = pos++;
			tmi.ptszName = cli.pfnGetStatusModeDescription( statusModeList[j], GSMDF_UNTRANSLATED );
			tmi.hIcon = LoadSkinProtoIcon( pa->szModuleName, statusModeList[j] );
			{
				//owner data
				lpStatusMenuExecParam smep = ( lpStatusMenuExecParam )mir_calloc( sizeof( StatusMenuExecParam ));
				smep->custom = FALSE;
				smep->status = statusModeList[j];
				smep->protoindex = i;
				smep->proto = mir_strdup(pa->szModuleName);
				tmi.ownerdata = smep;
			}

			hStatusMenuHandles[i].protoindex = i;
			hStatusMenuHandles[i].protostatus[j] = statusModeList[j];
			hStatusMenuHandles[i].menuhandle[j] = MO_AddNewMenuItem( hStatusMenuObject, &tmi );
			{
				char buf[ 256 ];
				sprintf(buf,"ProtocolIcon_%s_%s",pa->szModuleName,tmi.pszName);
				MO_SetOptionsMenuItem( hStatusMenuHandles[i].menuhandle[j], OPT_MENUITEMSETUNIQNAME, ( int )buf );
			}
			IconLib_ReleaseIcon(tmi.hIcon,0);
	}	}

	NotifyEventHooks(cli.hPreBuildStatusMenuEvent, 0, 0);
	pos = 200000;

	//add to root menu
	for ( j=0; j < SIZEOF(statusModeList); j++ ) {
		for ( i=0; i < accounts.getCount(); i++ ) {
			pa = accounts[i];
			if ( !bHideStatusMenu && !cli.pfnGetProtocolVisibility( pa->szModuleName ))
				continue;

			flags = CallProtoService(pa->szModuleName,PS_GETCAPS,PFLAGNUM_2,0);
			flags2 = CallProtoService(pa->szModuleName,PS_GETCAPS,PFLAGNUM_5,0);
			flags &= ~flags2;
			if ( !( flags & statusModePf2List[j] ))
				continue;

			TMO_MenuItem tmi = { 0 };
			tmi.cbSize = sizeof( tmi );
			tmi.flags = CMIF_ROOTHANDLE | CMIF_TCHAR;
			if ( statusModeList[j] == ID_STATUS_OFFLINE )
				tmi.flags |= CMIF_CHECKED;

			tmi.hIcon = LoadSkinIcon( skinIconStatusList[j] );
			tmi.position = pos++;
			tmi.hotKey = MAKELPARAM(MOD_CONTROL,'0'+j);
			{
				//owner data
				lpStatusMenuExecParam smep = ( lpStatusMenuExecParam )mir_alloc( sizeof( StatusMenuExecParam ));
				smep->custom = FALSE;
				smep->status = statusModeList[j];
				smep->proto = NULL;
				smep->svc = NULL;
				tmi.ownerdata = smep;
			}
			{
				TCHAR buf[ 256 ];
				mir_sntprintf( buf, SIZEOF( buf ), _T("%s\tCtrl+%c"),
					cli.pfnGetStatusModeDescription( statusModeList[j], 0 ), '0'+j );
				tmi.ptszName = buf;
				hStatusMainMenuHandles[j] = MO_AddNewMenuItem( hStatusMenuObject, &tmi );
			}
			{
				char buf[ 256 ];
				mir_snprintf( buf, sizeof( buf ), "Root2ProtocolIcon_%s_%s", pa->szModuleName, tmi.pszName );
				MO_SetOptionsMenuItem( hStatusMainMenuHandles[j], OPT_MENUITEMSETUNIQNAME, ( int )buf );
			}
			IconLib_ReleaseIcon( tmi.hIcon, 0 );
			break;
	}	}

	BuildStatusMenu(0,0);
}

int statustopos(int status)
{
	int j;
	for ( j = 0; j < SIZEOF(statusModeList); j++ )
		if ( status == statusModeList[j] )
			return j;

	return -1;
}

static int MenuProtoAck(WPARAM, LPARAM lParam)
{
	int i,networkProtoCount;
	ACKDATA* ack=(ACKDATA*)lParam;
	int overallStatus = 0,thisStatus;
	TMO_MenuItem tmi;

	if ( ack->type != ACKTYPE_STATUS ) return 0;
	if ( ack->result != ACKRESULT_SUCCESS ) return 0;
	if ( hStatusMainMenuHandles == NULL ) return 0;

	networkProtoCount = 0;
	for ( i=0; i < accounts.getCount(); i++ ) {
		PROTOACCOUNT* pa = accounts[i];
		int flags = CallProtoService( pa->szModuleName, PS_GETCAPS,PFLAGNUM_2, 0 );
		int flags2 = CallProtoService( pa->szModuleName, PS_GETCAPS,PFLAGNUM_5, 0 );
		if ((flags & ~flags2) == 0)
			continue;

		thisStatus = CallProtoService( pa->szModuleName, PS_GETSTATUS, 0, 0 );
		if ( overallStatus == 0 )
			overallStatus = thisStatus;
		else if ( overallStatus != thisStatus )
			overallStatus = -1;
		networkProtoCount++;
	}

	memset(&tmi,0,sizeof(tmi));
	tmi.cbSize=sizeof(tmi);
	if (overallStatus>ID_STATUS_CONNECTING) {
		int pos = statustopos(cli.currentStatusMenuItem);
		if (pos==-1) pos=0;
		{   // reset all current possible checked statuses
			int pos2;
			for (pos2=0; pos2<hStatusMainMenuHandlesCnt; pos2++)
			{
				if (pos2>=0 && pos2 < hStatusMainMenuHandlesCnt)
				{
					tmi.flags = CMIM_FLAGS | CMIF_ROOTHANDLE;
					MO_ModifyMenuItem( hStatusMainMenuHandles[pos2], &tmi );
		}	}	}

		cli.currentStatusMenuItem=overallStatus;
		pos = statustopos(cli.currentStatusMenuItem);
		if (pos>=0 && pos < hStatusMainMenuHandlesCnt) {
			tmi.flags = CMIM_FLAGS | CMIF_ROOTHANDLE | CMIF_CHECKED;
			MO_ModifyMenuItem( hStatusMainMenuHandles[pos], &tmi );
		}
		cli.currentDesiredStatusMode = cli.currentStatusMenuItem;
	}
	else {
		int pos = statustopos( cli.currentStatusMenuItem );
		if ( pos == -1 ) pos=0;
		if ( pos >= 0 && pos < hStatusMainMenuHandlesCnt ) {
			tmi.flags = CMIM_FLAGS | CMIF_ROOTHANDLE;
			MO_ModifyMenuItem( hStatusMainMenuHandles[pos], &tmi );
		}
		//SetMenuDefaultItem(hStatusMenu,-1,FALSE);
		cli.currentStatusMenuItem=0;
	}
	if ( networkProtoCount <= 1 )
		return 0;

	for ( i=0; i < accounts.getCount(); i++ ) {
		if ( !lstrcmpA( accounts[i]->szModuleName, ack->szModule )) {
			//hProcess is previous mode, lParam is new mode
			if ((( int )ack->hProcess >= ID_STATUS_OFFLINE || ( int )ack->hProcess == 0 ) && ( int )ack->hProcess < ID_STATUS_OFFLINE + SIZEOF(statusModeList)) {
				int pos = statustopos(( int )ack->hProcess);
				if ( pos == -1 )
					pos = 0;
				for ( pos = 0; pos < SIZEOF(statusModeList); pos++ ) {
					tmi.flags = CMIM_FLAGS | CMIF_ROOTHANDLE;
					MO_ModifyMenuItem( hStatusMenuHandles[i].menuhandle[pos], &tmi );
			}	}

			if ( ack->lParam >= ID_STATUS_OFFLINE && ack->lParam < ID_STATUS_OFFLINE + SIZEOF(statusModeList)) {
				int pos = statustopos(( int )ack->lParam );
				if ( pos >= 0 && pos < SIZEOF(statusModeList)) {
					tmi.flags = CMIM_FLAGS | CMIF_ROOTHANDLE | CMIF_CHECKED;
					MO_ModifyMenuItem( hStatusMenuHandles[i].menuhandle[pos], &tmi );
			}	}
			break;
	}	}

	//BuildStatusMenu(0,0);
	return 0;
}

static MenuProto* FindProtocolMenu( const char* proto )
{
	for (int i=0; i < cli.menuProtoCount; i++)
		if ( cli.menuProtos[i].pMenu && !lstrcmpiA( cli.menuProtos[i].szProto, proto ))
			return &cli.menuProtos[i];

	if ( cli.menuProtoCount == 1 )
		if ( !lstrcmpiA( cli.menuProtos[0].szProto, proto ))
			return &cli.menuProtos[0];

	return NULL;
}

HGENMENU fnGetProtocolMenu( const char* proto )
{
	MenuProto* mp = FindProtocolMenu( proto );
	if ( mp )
		return mp->pMenu;

	return NULL;
}

static int AddStatusMenuItem(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM *mi = ( CLISTMENUITEM* )lParam;
	if ( mi->cbSize != sizeof( CLISTMENUITEM ))
		return 0;

	PMO_IntMenuItem pRoot = NULL;
	lpStatusMenuExecParam smep = NULL;

	TMO_MenuItem tmi = { 0 };
	tmi.cbSize = sizeof(tmi);
	tmi.hIcon = mi->hIcon;
	tmi.hotKey = mi->hotKey;
	tmi.position = mi->position;
	tmi.pszName = mi->pszName;
	tmi.flags = mi->flags;
	tmi.root = ( HGENMENU )mi->pszPopupName;

	// for new style menus the pszPopupName contains the root menu handle
	if ( mi->flags & CMIF_ROOTHANDLE )
		pRoot = MO_GetIntMenuItem( mi->hParentMenu );

	// for old style menus the pszPopupName really means the popup name
	else {
		MenuProto* mp = FindProtocolMenu( mi->pszContactOwner );
		if ( mp && mi->pszPopupName ) {
			if ( mp->pMenu ) {
				#if defined _UNICODE
					TCHAR* ptszName = ( mi->flags & CMIF_UNICODE ) ? mir_tstrdup(mi->ptszPopupName) : mir_a2t(mi->pszPopupName);
					pRoot = MO_RecursiveWalkMenu( mp->pMenu->submenu.first, FindRoot, ptszName );
					mir_free( ptszName );
				#else
					pRoot = MO_RecursiveWalkMenu( mp->pMenu->submenu.first, FindRoot, mi->pszPopupName );
				#endif
			}
			if ( pRoot == NULL ) {
				TMO_MenuItem tmi = { 0 };
				tmi.cbSize = sizeof(tmi);
				tmi.flags = (mi->flags & CMIF_UNICODE) | CMIF_ROOTHANDLE;
				tmi.position = 1001;
				tmi.root = mp->pMenu;
				tmi.hIcon = NULL;
				tmi.pszName = mi->pszPopupName;
				pRoot = MO_AddNewMenuItem( hStatusMenuObject, &tmi );
			}

			tmi.flags |= CMIF_ROOTHANDLE;
			tmi.root = pRoot;
	}	}

	if (wParam) {
		int * res=(int*)wParam;
		*res = ( int )pRoot;
	}

	//owner data
	if ( mi->pszService ) {
		smep = ( lpStatusMenuExecParam )mir_calloc(sizeof(StatusMenuExecParam));
		smep->custom = TRUE;
		smep->svc=mir_strdup(mi->pszService);
		{
			char *buf=mir_strdup(mi->pszService);
			int i=0;
			while(buf[i]!='\0' && buf[i]!='/') i++;
			buf[i]='\0';
				smep->proto=mir_strdup(buf);
			mir_free(buf);
		}
		tmi.ownerdata = smep;
	}
	PMO_IntMenuItem menuHandle = MO_AddNewMenuItem( hStatusMenuObject, &tmi );
	if ( smep )
		smep->hMenuItem = menuHandle;

	char buf[MAX_PATH+64];
	#if defined( _UNICODE )
	{
		char* p = ( pRoot ) ? mir_t2a( pRoot->mi.ptszName ) : NULL;
		mir_snprintf( buf, SIZEOF(buf), "%s/%s", ( p ) ? p : "", mi->pszService ? mi->pszService : "" );
		mir_free( p );
	}
	#else
		mir_snprintf( buf, SIZEOF(buf), "%s/%s", pRoot ? pRoot->mi.ptszName : _T(""), mi->pszService ? mi->pszService : "" );
	#endif
	MO_SetOptionsMenuItem( menuHandle, OPT_MENUITEMSETUNIQNAME, ( int )buf );

	return ( int )menuHandle;
}

void InitCustomMenus(void)
{
	CreateServiceFunction("MainMenuExecService",MainMenuExecService);

	CreateServiceFunction("ContactMenuExecService",ContactMenuExecService);
	CreateServiceFunction("ContactMenuCheckService",ContactMenuCheckService);

	CreateServiceFunction("StatusMenuExecService",StatusMenuExecService);
	CreateServiceFunction("StatusMenuCheckService",StatusMenuCheckService);

	//free services
	CreateServiceFunction("CLISTMENUS/FreeOwnerDataMainMenu",FreeOwnerDataMainMenu);
	CreateServiceFunction("CLISTMENUS/FreeOwnerDataContactMenu",FreeOwnerDataContactMenu);
	CreateServiceFunction("CLISTMENUS/FreeOwnerDataStatusMenu",FreeOwnerDataStatusMenu);

	CreateServiceFunction(MS_CLIST_SETSTATUSMODE, SetStatusMode);

	CreateServiceFunction(MS_CLIST_ADDMAINMENUITEM,AddMainMenuItem);
	CreateServiceFunction(MS_CLIST_ADDSTATUSMENUITEM,AddStatusMenuItem);
	CreateServiceFunction(MS_CLIST_MENUGETMAIN,MenuGetMain);
	CreateServiceFunction(MS_CLIST_REMOVEMAINMENUITEM,RemoveMainMenuItem);
	CreateServiceFunction(MS_CLIST_MENUBUILDMAIN,BuildMainMenu);

	CreateServiceFunction(MS_CLIST_ADDCONTACTMENUITEM,AddContactMenuItem);
	CreateServiceFunction(MS_CLIST_MENUBUILDCONTACT,BuildContactMenu);
	CreateServiceFunction(MS_CLIST_REMOVECONTACTMENUITEM,RemoveContactMenuItem);

	CreateServiceFunction(MS_CLIST_MODIFYMENUITEM,ModifyCustomMenuItem);
	CreateServiceFunction(MS_CLIST_MENUMEASUREITEM,MeasureMenuItem);
	CreateServiceFunction(MS_CLIST_MENUDRAWITEM,DrawMenuItem);

	CreateServiceFunction(MS_CLIST_MENUGETSTATUS,BuildStatusMenu);
	CreateServiceFunction(MS_CLIST_MENUPROCESSCOMMAND,MenuProcessCommand);
	CreateServiceFunction(MS_CLIST_MENUPROCESSHOTKEY,MenuProcessHotkey);

	hPreBuildContactMenuEvent=CreateHookableEvent(ME_CLIST_PREBUILDCONTACTMENU);
	hPreBuildMainMenuEvent=CreateHookableEvent(ME_CLIST_PREBUILDMAINMENU);
	cli.hPreBuildStatusMenuEvent=CreateHookableEvent(ME_CLIST_PREBUILDSTATUSMENU);
	hStatusModeChangeEvent = CreateHookableEvent( ME_CLIST_STATUSMODECHANGE );

	hAckHook=(HANDLE)HookEvent(ME_PROTO_ACK,MenuProtoAck);

	hMainMenu=GetSubMenu(LoadMenu(hMirandaInst,MAKEINTRESOURCE(IDR_CLISTMENU)),0);
	hStatusMenu=GetSubMenu(LoadMenu(hMirandaInst,MAKEINTRESOURCE(IDR_CLISTMENU)),1);
	CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hMainMenu,0);
	CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hStatusMenu,0);

	hStatusMainMenuHandles=NULL;
	hStatusMainMenuHandlesCnt=0;

	hStatusMenuHandles=NULL;
	hStatusMenuHandlesCnt=0;

	//new menu sys
	InitGenMenu();

	//main menu
	{
		TMenuParam tmp = { 0 };
		tmp.cbSize=sizeof(tmp);
		tmp.CheckService=NULL;
		tmp.ExecService="MainMenuExecService";
		tmp.name="MainMenu";
		hMainMenuObject=CallService(MO_CREATENEWMENUOBJECT,(WPARAM)0,(LPARAM)&tmp);
	}

	MO_SetOptionsMenuObject( hMainMenuObject, OPT_USERDEFINEDITEMS, TRUE );
	MO_SetOptionsMenuObject( hMainMenuObject, OPT_MENUOBJECT_SET_FREE_SERVICE, (int)"CLISTMENUS/FreeOwnerDataMainMenu" );

	//contact menu
	{
		TMenuParam tmp = { 0 };
		tmp.cbSize=sizeof(tmp);
		tmp.CheckService="ContactMenuCheckService";
		tmp.ExecService="ContactMenuExecService";
		tmp.name="ContactMenu";
		hContactMenuObject=CallService(MO_CREATENEWMENUOBJECT,(WPARAM)0,(LPARAM)&tmp);
	}

	MO_SetOptionsMenuObject( hContactMenuObject, OPT_USERDEFINEDITEMS, TRUE );
	MO_SetOptionsMenuObject( hContactMenuObject, OPT_MENUOBJECT_SET_FREE_SERVICE, (int)"CLISTMENUS/FreeOwnerDataContactMenu" );

   // add exit command to menu
	{
		CLISTMENUITEM mi = { 0 };
		mi.cbSize = sizeof( mi );
		mi.position = 0x7fffffff;
		mi.flags = CMIF_ICONFROMICOLIB;
		mi.pszService = "CloseAction";
		mi.pszName = LPGEN("E&xit");
        mi.icolibItem = GetSkinIconHandle( SKINICON_OTHER_EXIT );
		AddMainMenuItem( 0, ( LPARAM )&mi );
	}

	cli.currentStatusMenuItem=ID_STATUS_OFFLINE;
	cli.currentDesiredStatusMode=ID_STATUS_OFFLINE;

	if ( IsWinVer98Plus() )
		HookEvent(ME_SKIN_ICONSCHANGED, MenuIconsChanged );
}

void UninitCustomMenus(void)
{
	if (hStatusMainMenuHandles!=NULL)
		mir_free(hStatusMainMenuHandles);

	hStatusMainMenuHandles = NULL;

	if ( hStatusMenuHandles != NULL )
		mir_free( hStatusMenuHandles );
	hStatusMenuHandles = NULL;

	if ( hMainMenuObject   ) CallService( MO_REMOVEMENUOBJECT, hMainMenuObject, 0 );
	if ( hStatusMenuObject ) CallService( MO_REMOVEMENUOBJECT, hMainMenuObject, 0 );

	UnloadMoveToGroup();
	FreeMenuProtos();

	RecursiveDeleteMenu(hStatusMenu);
	RecursiveDeleteMenu(hMainMenu);
	DestroyMenu(hMainMenu);
	DestroyMenu(hStatusMenu);
	UnhookEvent(hAckHook);
}
