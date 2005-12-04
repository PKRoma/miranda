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

CLIST_INTERFACE cli = { 0 };

/* clc.h */
void   fnClcOptionsChanged( void );
void   fnClcBroadcast( int msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK fnContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* clcfiledrop.c */
void   fnRegisterFileDropping ( HWND hwnd );
void   fnUnregisterFileDropping ( HWND hwnd );

/* clcidents.c */
int    fnGetRowsPriorTo ( struct ClcGroup *group, struct ClcGroup *subgroup, int contactIndex );
int    fnFindItem ( HWND hwnd, struct ClcData *dat, HANDLE hItem, struct ClcContact **contact, struct ClcGroup **subgroup, int *isVisible );
int    fnGetRowByIndex ( struct ClcData *dat, int testindex, struct ClcContact **contact, struct ClcGroup **subgroup );
HANDLE fnContactToHItem ( struct ClcContact* contact );
HANDLE fnContactToItemHandle ( struct ClcContact * contact, DWORD * nmFlags );

/* clcitems.c */
struct ClcGroup* fnAddGroup ( HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers );
struct ClcGroup* fnRemoveItemFromGroup (HWND hwnd, struct ClcGroup *group, struct ClcContact *contact, int updateTotalCount);

void fnFreeGroup ( struct ClcGroup *group );
int  fnAddInfoItemToGroup (struct ClcGroup *group, int flags, const TCHAR *pszText);
int  fnAddItemToGroup( struct ClcGroup *group,int iAboveItem );
void fnAddContactToTree ( HWND hwnd, struct ClcData *dat, HANDLE hContact, int updateTotalCount, int checkHideOffline);
void fnDeleteItemFromTree ( HWND hwnd, HANDLE hItem );
void fnRebuildEntireList ( HWND hwnd, struct ClcData *dat );
int  fnGetGroupContentsCount ( struct ClcGroup *group, int visibleOnly );
void fnSortCLC ( HWND hwnd, struct ClcData *dat, int useInsertionSort );
void fnSaveStateAndRebuildList (HWND hwnd, struct ClcData *dat);

/* clcmsgs.c */
LRESULT fnProcessExternalMessages (HWND hwnd, struct ClcData *dat, UINT msg, WPARAM wParam, LPARAM lParam );

/* clcpaint.c */
void  fnPaintClc( HWND hwnd, struct ClcData *dat, HDC hdc, RECT * rcPaint ) {}

/* clcutils.c */
char* fnGetGroupCountsText (struct ClcData *dat, struct ClcContact *contact );
int   fnHitTest ( HWND hwnd, struct ClcData *dat, int testx, int testy, struct ClcContact **contact, struct ClcGroup **group, DWORD * flags );
void  fnScrollTo ( HWND hwnd, struct ClcData *dat, int desty, int noSmooth );
void  fnEnsureVisible (HWND hwnd, struct ClcData *dat, int iItem, int partialOk );
void  fnRecalcScrollBar ( HWND hwnd, struct ClcData *dat );
void  fnSetGroupExpand ( HWND hwnd, struct ClcData *dat, struct ClcGroup *group, int newState );
void  fnDoSelectionDefaultAction ( HWND hwnd, struct ClcData *dat );
int   fnFindRowByText (HWND hwnd, struct ClcData *dat, const TCHAR *text, int prefixOk );
void  fnEndRename (HWND hwnd, struct ClcData *dat, int save );
void  fnDeleteFromContactList ( HWND hwnd, struct ClcData *dat );
void  fnBeginRenameSelection ( HWND hwnd, struct ClcData *dat );
int   fnGetDropTargetInformation ( HWND hwnd, struct ClcData *dat, POINT pt );
int   fnClcStatusToPf2 ( int status );
int   fnIsHiddenMode ( struct ClcData *dat, int status );
void  fnHideInfoTip ( HWND hwnd, struct ClcData *dat );
void  fnNotifyNewContact ( HWND hwnd, HANDLE hContact );
DWORD fnGetDefaultExStyle ( void );
void  fnGetSetting ( int i, LOGFONT* lf, COLORREF* colour );
void  fnGetFontSetting ( int i, LOGFONT* lf, COLORREF* colour );
void  fnLoadClcOptions ( HWND hwnd, struct ClcData *dat );
void  fnRecalculateGroupCheckboxes ( HWND hwnd, struct ClcData *dat );
void  fnSetGroupChildCheckboxes ( struct ClcGroup *group, int checked );
void  fnInvalidateItem ( HWND hwnd, struct ClcData *dat, int iItem );

/* clistevents.c */
int   fnEventsProcessContactDoubleClick ( HANDLE hContact );
int   fnEventsProcessTrayDoubleClick ( void );

/* clistmod.c */
int   fnIconFromStatusMode(const char *szProto, int status);
int   fnShowHide( WPARAM wParam, LPARAM lParam );

/* clistsettings.c */
TCHAR* fnGetContactDisplayName ( HANDLE hContact, int mode );
void   fnGetDefaultFontSetting(int i, LOGFONT* lf, COLORREF * colour);
void   fnInvalidateDisplayNameCacheEntry(HANDLE hContact);

ClcCacheEntryBase* fnGetCacheEntry( HANDLE hContact );
ClcCacheEntryBase* fnCreateCacheItem ( HANDLE hContact );
void fnCheckCacheItem( ClcCacheEntryBase* p );
void fnFreeCacheItem( ClcCacheEntryBase* p );

/* clisttray.c */
void fnTrayIconUpdateWithImageList ( int iImage, const char *szNewTip, char *szPreferredProto );
void fnTrayIconUpdateBase ( const char *szChangedProto );
void fnTrayIconSetToBase ( char *szPreferredProto );
void fnTrayIconIconsChanged ( void );
int  fnTrayIconPauseAutoHide ( WPARAM wParam, LPARAM lParam );
int  fnTrayIconProcessMessage ( WPARAM wParam, LPARAM lParam );

/* clui.c */
LRESULT CALLBACK fnContactListWndProc ( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
void fnLoadCluiGlobalOpts( void );
int  fnCluiProtocolStatusChanged(void);

/* contact.c */
void fnChangeContactIcon ( HANDLE hContact, int iIcon, int add );
void fnLoadContactTree ( void );
int  fnCompareContacts ( WPARAM wParam, LPARAM lParam );
void fnSortContacts ( void );
int  fnSetHideOffline ( WPARAM wParam, LPARAM lParam );

/* docking.c */
int fnDocking_ProcessWindowMessage ( WPARAM wParam, LPARAM lParam );

/* group.c */
TCHAR* fnGetGroupName ( int idx, DWORD* pdwFlags );
int    fnRenameGroup ( int groupID, TCHAR* newName );

/* keyboard.c */
int  fnHotKeysRegister ( HWND hwnd );
void fnHotKeysUnregister ( HWND hwnd );
int  fnHotKeysProcess ( HWND hwnd, WPARAM wParam, LPARAM lParam );
int  fnHotkeysProcessMessage ( WPARAM wParam, LPARAM lParam );

static int interfaceInited = 0;

static struct ClcContact* fnCreateClcContact( void )
{
	return ( struct ClcContact* )calloc( sizeof( struct ClcContact ), 1 );
}

static int srvRetrieveInterface( WPARAM wParam, LPARAM lParam )
{
	int rc;

	if ( interfaceInited == 0 ) {
		cli.version = 1;

		cli.pfnClcOptionsChanged               = fnClcOptionsChanged;
		cli.pfnClcBroadcast                    = fnClcBroadcast;
		cli.pfnContactListControlWndProc       = fnContactListControlWndProc;

		cli.pfnRegisterFileDropping            = fnRegisterFileDropping;
		cli.pfnUnregisterFileDropping          = fnUnregisterFileDropping;
															 								  
		cli.pfnGetRowsPriorTo                  = fnGetRowsPriorTo;
		cli.pfnFindItem						      = fnFindItem;
		cli.pfnGetRowByIndex                   = fnGetRowByIndex;
		cli.pfnContactToHItem				      = fnContactToHItem;
		cli.pfnContactToItemHandle             = fnContactToItemHandle;
															 								       
		cli.pfnAddGroup                        = fnAddGroup;
		cli.pfnAddItemToGroup                  = fnAddItemToGroup;
		cli.pfnCreateClcContact                = fnCreateClcContact;
		cli.pfnRemoveItemFromGroup		         = fnRemoveItemFromGroup;
		cli.pfnFreeGroup					         = fnFreeGroup;
		cli.pfnAddInfoItemToGroup		         = fnAddInfoItemToGroup;
		cli.pfnAddContactToTree                = fnAddContactToTree;
		cli.pfnDeleteItemFromTree		         = fnDeleteItemFromTree;
		cli.pfnRebuildEntireList			      = fnRebuildEntireList;
		cli.pfnGetGroupContentsCount	         = fnGetGroupContentsCount;
		cli.pfnSortCLC						         = fnSortCLC;
		cli.pfnSaveStateAndRebuildList         = fnSaveStateAndRebuildList;
															 
		cli.pfnProcessExternalMessages	      = fnProcessExternalMessages;

		cli.pfnPaintClc                        = fnPaintClc;

		cli.pfnGetGroupCountsText		         = fnGetGroupCountsText;
		cli.pfnHitTest                         = fnHitTest;
		cli.pfnScrollTo						      = fnScrollTo;
		cli.pfnEnsureVisible				         = fnEnsureVisible;
		cli.pfnRecalcScrollBar			         = fnRecalcScrollBar;
		cli.pfnSetGroupExpand				      = fnSetGroupExpand;
		cli.pfnDoSelectionDefaultAction        = fnDoSelectionDefaultAction;
		cli.pfnFindRowByText				         = fnFindRowByText;
		cli.pfnEndRename					         = fnEndRename;
		cli.pfnDeleteFromContactList	         = fnDeleteFromContactList;
		cli.pfnBeginRenameSelection		      = fnBeginRenameSelection;
		cli.pfnGetDropTargetInformation        = fnGetDropTargetInformation;
		cli.pfnClcStatusToPf2				      = fnClcStatusToPf2;
		cli.pfnIsHiddenMode				         = fnIsHiddenMode;
		cli.pfnHideInfoTip					      = fnHideInfoTip;
		cli.pfnNotifyNewContact			         = fnNotifyNewContact;
		cli.pfnGetDefaultExStyle               = fnGetDefaultExStyle;
		cli.pfnGetFontSetting					   = fnGetFontSetting;
		cli.pfnLoadClcOptions					   = fnLoadClcOptions;
		cli.pfnRecalculateGroupCheckboxes	   = fnRecalculateGroupCheckboxes;
		cli.pfnSetGroupChildCheckboxes		   = fnSetGroupChildCheckboxes;
		cli.pfnInvalidateItem					   = fnInvalidateItem;
															 
		cli.pfnEventsProcessContactDoubleClick = fnEventsProcessContactDoubleClick;
		cli.pfnEventsProcessTrayDoubleClick	   = fnEventsProcessTrayDoubleClick;
															 
		cli.pfnGetContactDisplayName			   = fnGetContactDisplayName;
		cli.pfnInvalidateDisplayNameCacheEntry = fnInvalidateDisplayNameCacheEntry;
		cli.pfnCreateCacheItem                 = fnCreateCacheItem;
		cli.pfnCheckCacheItem                  = fnCheckCacheItem;
		cli.pfnFreeCacheItem                   = fnFreeCacheItem;
		cli.pfnGetCacheEntry                   = fnGetCacheEntry;
															 
		cli.pfnTrayIconUpdateWithImageList	   = fnTrayIconUpdateWithImageList;
		cli.pfnTrayIconUpdateBase				   = fnTrayIconUpdateBase;
		cli.pfnTrayIconSetToBase					= fnTrayIconSetToBase;
		cli.pfnTrayIconIconsChanged            = fnTrayIconIconsChanged;
		cli.pfnTrayIconPauseAutoHide			   = fnTrayIconPauseAutoHide;
		cli.pfnTrayIconProcessMessage			   = fnTrayIconProcessMessage;
															 
		cli.pfnContactListWndProc				   = fnContactListWndProc;
		cli.pfnLoadCluiGlobalOpts              = fnLoadCluiGlobalOpts;
		cli.pfnCluiProtocolStatusChanged       = fnCluiProtocolStatusChanged;
															 
		cli.pfnChangeContactIcon					= fnChangeContactIcon;
		cli.pfnLoadContactTree					   = fnLoadContactTree;
		cli.pfnCompareContacts					   = fnCompareContacts;
		cli.pfnSortContacts						   = fnSortContacts;
		cli.pfnSetHideOffline					   = fnSetHideOffline;
															 
		cli.pfnDocking_ProcessWindowMessage	   = fnDocking_ProcessWindowMessage;
								
		cli.pfnIconFromStatusMode              = fnIconFromStatusMode;
		cli.pfnShowHide                        = fnShowHide;

		cli.pfnGetGroupName						   = fnGetGroupName;
		cli.pfnRenameGroup                     = fnRenameGroup;
															 
		cli.pfnHotKeysRegister					   = fnHotKeysRegister;
		cli.pfnHotKeysUnregister					= fnHotKeysUnregister;
		cli.pfnHotKeysProcess						= fnHotKeysProcess;
		cli.pfnHotkeysProcessMessage			   = fnHotkeysProcessMessage;

		cli.hInst = ( HMODULE )lParam;

		rc = LoadContactListModule2();
		if (rc == 0)
			rc = LoadCLCModule();
		interfaceInited = 1;						
	}													
														
	return ( int )( LPARAM )&cli;				
}														

int LoadContactListModule()
{
	CreateServiceFunction( MS_CLIST_RETRIEVE_INTERFACE, srvRetrieveInterface );
	return 0;
}