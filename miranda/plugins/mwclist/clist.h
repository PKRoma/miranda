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
#ifndef _CLIST_H_
#define _CLIST_H_

void LoadContactTree(void);
int IconFromStatusMode(const char *szProto,int status);
HTREEITEM GetTreeItemByHContact(HANDLE hContact);
void TrayIconUpdateWithImageList(int iImage,const char *szNewTip,char *szPreferredProto);
void SortContacts(void);
void ChangeContactIcon(HANDLE hContact,int iIcon,int add);
int GetContactInfosForSort(HANDLE hContact,char **Proto,char **Name,int *Status);


typedef struct  {
	HANDLE hContact;
	char *name;
	int NameHash;
	char *szProto;
	boolean protoNotExists;
	int ProtoHash;
	int	  status;
	int Hidden;
	char *szGroup;
	int i;
	int ApparentMode;
	int NotOnList;
	int IdleTS;
	void *ClcContact;
	boolean isUnknown;

} displayNameCacheEntry,*pdisplayNameCacheEntry;

pdisplayNameCacheEntry GetContactFullCacheEntry(HANDLE hContact);

#endif