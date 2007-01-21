/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, 
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
#ifndef _CLIST_H_
#define _CLIST_H_

void LoadContactTree(void);
int ExtIconFromStatusMode(HANDLE hContact, const char *szProto,int status);
HTREEITEM GetTreeItemByHContact(HANDLE hContact);
void cliTrayIconUpdateWithImageList(int iImage,const TCHAR *szNewTip,char *szPreferredProto);
void cli_ChangeContactIcon(HANDLE hContact,int iIcon,int add);
int GetContactInfosForSort(HANDLE hContact,char **Proto,TCHAR **Name,int *Status);

typedef HMONITOR ( WINAPI *pfnMyMonitorFromPoint )(POINT,DWORD);
extern pfnMyMonitorFromPoint MyMonitorFromPoint;

typedef HMONITOR( WINAPI *pfnMyMonitorFromWindow) (HWND, DWORD);
extern pfnMyMonitorFromWindow MyMonitorFromWindow;

typedef BOOL(WINAPI *pfnMyGetMonitorInfo) (HMONITOR, LPMONITORINFO);
extern pfnMyGetMonitorInfo MyGetMonitorInfo;

typedef struct  {
	HANDLE hContact;
	TCHAR *name;
	#if defined( _UNICODE )
		char *szName;
	#endif
	TCHAR* szGroup;
	int Hidden;
	int noHiddenOffline;

	char *szProto;
	boolean protoNotExists;
	int	status;
	int HiddenSubcontact;

	int i;
	int ApparentMode;
	int NotOnList;
	int IdleTS;
	void *ClcContact;
	BYTE IsExpanded;
	boolean isUnknown;

	TCHAR *szSecondLineText;//[120-MAXEXTRACOLUMNS];
	SortedList *plSecondLineText;				// List of ClcContactTextPiece
	TCHAR *szThirdLineText;//[120-MAXEXTRACOLUMNS];
	SortedList *plThirdLineText;				// List of ClcContactTextPiece
	int iThirdLineMaxSmileyHeight;
    int iSecondLineMaxSmileyHeight;
	DWORD timezone;
    DWORD timediff;
	DWORD dwLastMsgTime;
} displayNameCacheEntry,*pdisplayNameCacheEntry, *PDNCE;

typedef struct tagEXTRASLOTINFO
{
    union
    {
        TCHAR * ptszSlotName;   // one of this string should be given
        char  * pszSlotName;
    };
    char * pszSlotID;
    BOOL fUnicode;
    BYTE iSlot;               // the slot 10-16 are available, do not use
} EXTRASLOTINFO;

#define CLVM_FILTER_PROTOS 1
#define CLVM_FILTER_GROUPS 2
#define CLVM_FILTER_STATUS 4
#define CLVM_FILTER_VARIABLES 8
#define CLVM_STICKY_CONTACTS 16
#define CLVM_FILTER_STICKYSTATUS 32
#define CLVM_FILTER_LASTMSG 64
#define CLVM_FILTER_LASTMSG_OLDERTHAN 128
#define CLVM_FILTER_LASTMSG_NEWERTHAN 256

#define CLVM_PROTOGROUP_OP 1
#define CLVM_GROUPSTATUS_OP 2
#define CLVM_AUTOCLEAR 4
#define CLVM_INCLUDED_UNGROUPED 8
#define CLVM_USELASTMSG 16

#if defined(_UNICODE)
#define CLVM_MODULE "CLVM_W"
#else
#define CLVM_MODULE "CLVM"
#endif

#endif