/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-2  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
#include "msn_global.h"

struct MsnContact {
	int list;
	char *email,*nick;
};

static int count;
static struct MsnContact *lists;
static CRITICAL_SECTION csLists;

void Lists_Init(void)
{
	lists=NULL;
	count=0;
	InitializeCriticalSection(&csLists);
}

void Lists_Uninit(void)
{
	int i;
	for(i=0;i<count;i++) {
		free(lists[i].email);
		free(lists[i].nick);
	}
	if(lists!=NULL) free(lists);
	DeleteCriticalSection(&csLists);
}

int Lists_NameToCode(const char *name)
{
	if(name[2]) return 0;
	switch(*(PWORD)name) {
		case 'LA': return LIST_AL;
		case 'LB': return LIST_BL;
		case 'LR': return LIST_RL;
		case 'LF': return LIST_FL;
	}
	return 0;
}

void Lists_Wipe(void)
{
	int i;
	EnterCriticalSection(&csLists);
	for(i=0;i<count;i++) {
		free(lists[i].email);
		free(lists[i].nick);
	}
	if(lists!=NULL) {
		free(lists);
		lists=NULL;
	}
	count=0;
	LeaveCriticalSection(&csLists);
}

int Lists_IsInList(int list,const char *email)
{
	int i;
	EnterCriticalSection(&csLists);
	for(i=0;i<count;i++)
		if(lists[i].list==list && !strcmp(lists[i].email,email)) {
		  	LeaveCriticalSection(&csLists);
			return i+1;
		}
	LeaveCriticalSection(&csLists);
	return 0;
}

void Lists_Add(int list,const char *email,const char *nick)
{
	EnterCriticalSection(&csLists);
	if(Lists_IsInList(list,email)) {
		LeaveCriticalSection(&csLists);
		return;
	}
	lists=(struct MsnContact*)realloc(lists,sizeof(struct MsnContact)*(count+1));
	lists[count].list=list;
	lists[count].email=_strdup(email);
	lists[count].nick=_strdup(nick);
	count++;
	LeaveCriticalSection(&csLists);
}

void Lists_Remove(int list,const char *email)
{
	int i;
	EnterCriticalSection(&csLists);
	i=Lists_IsInList(list,email);
	if(!i) {
		LeaveCriticalSection(&csLists);
		return;
	}
	i--;
	free(lists[i].email);
	free(lists[i].nick);
	count--;
	memmove(lists+i,lists+i+1,sizeof(struct MsnContact)*(count-i));
	lists=(struct MsnContact*)realloc(lists,sizeof(struct MsnContact)*count);
	LeaveCriticalSection(&csLists);
}