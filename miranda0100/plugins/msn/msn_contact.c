/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
//MSN Messenger Support for Miranda ICQ
//Copyright 2000, Tristan Van de Vreede

//LOGIN TO A DS Server


	#include "MSN_global.h"
//	#include "resource.h"
/*
#ifndef MODULAR
	#include "../../core/miranda.h"
	#include "../../global.h"
	#include "../../internal.h"	

	#include "../../random/oldplugins/plugins.h"
	#include "../../random/oldplugins/pluginapi.h"

    void DisplayContact(CONTACT *cnt);
    void SetContactIcon(int uin, int status,int cnttype);
#endif

	int MSN_AddContact(char* uhandle,char*nick)
	{
		int i;
		
		//first check he is not already there
		for (i=0;i<opts.ccount;i++)
		{
			if (opts.clist[i].id==CT_MSN)
			{
				if (stricmp(opts.clist[i].email,uhandle)==0)
				{ //he is already here
					return i;
				}
			}
		}
		//he doesnt exist ,add him

		
		


		opts.clist[opts.ccount].id=CT_MSN;
		strcpy(opts.clist[opts.ccount].email,uhandle);
		
		strcpy (opts.clist[opts.ccount].nick,nick);
		*//*//check for LF on nick
		if (opts.clist[opts.ccount].nick[strlen(opts.clist[opts.ccount].nick)-1]=='\r')
		{
			opts.clist[opts.ccount].nick[strlen(opts.clist[opts.ccount].nick)-1]=0;
		}*//*
		
		opts.clist[opts.ccount].uin=MSN_BASEUIN + MSN_ccount;
		opts.clist[opts.ccount].groupid=MAX_GROUPS+1;
		opts.clist[opts.ccount].status=MSN_INTSTATUS_OFFLINE;
		
		//put him on the tree view
		DisplayContact(&opts.clist[opts.ccount]);
		
		opts.ccount++;
		MSN_ccount++;

		return (opts.ccount-1);
	}

	int MSN_ContactFromHandle(char*uhandle)
	{
		int i;
		for (i=0;i<opts.ccount;i++)
		{
			if (opts.clist[i].id==CT_MSN)
			{
				if (stricmp(opts.clist[i].email,uhandle)==0)
				{
					return i;
				}
			}
		}
		
		return -1;
	}

	void MSN_HandleFromContact(unsigned long uin,char*uhandle)
	{
		int i;
		for (i=0;i<opts.ccount;i++)
		{
			if (opts.clist[i].id==CT_MSN && opts.clist[i].uin==uin)
			{
				strcpy(uhandle,opts.clist[i].email);
				return;
			}
		}
		
		uhandle=NULL;
		return ;
	}

	void MSN_ChangeContactStatus(char*uhandle,char*state)
	{
		int id;
		int oldstatus;
		//HTREEITEM hitem;
		
		id=MSN_ContactFromHandle(uhandle);
		oldstatus=opts.clist[id].status;
		opts.clist[id].status=MSN_GetIntStatus(state);
		if (id>=0)
		{
			//set icon state
			SetContactIcon(opts.clist[id].uin,opts.clist[id].status,CT_MSN);
			
			//TELL PLugins someone has changed status
			Plugin_NotifyPlugins(PM_CONTACTSTATUSCHANGE, (WPARAM)oldstatus,(LPARAM)&opts.clist[id]);//lparam pointer to contact struct
		}
		

	}

	int MSN_MSNStatetoICQState(char*state)
	{
		if (stricmp(MSN_STATUS_ONLINE,state)==0)
		{
			return STATUS_ONLINE;
		}
		else if (stricmp(MSN_STATUS_OFFLINE,state)==0)
		{
			return STATUS_OFFLINE;
		}
		else if (stricmp(MSN_STATUS_AWAY,state)==0)
		{
			return STATUS_AWAY;
		}
		else if (stricmp(MSN_STATUS_BUSY,state)==0)
		{
			return STATUS_NA;
		}

		return STATUS_NA;

	}



void MSN_AddContactByUhandle(HWND hwnd)
{
	if (opts.MSN.logedin)
	{
		HWNDLIST *node;

		node = malloc(sizeof(HWNDNODE));

		node->windowtype = WT_ADDCONT;
		node->next = rto.hwndlist;
		rto.hwndlist = node;

		node->hwnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_MSN_ADDCONTACT), ghwnd, DlgProcMSN_AddContact);
		ShowWindow(node->hwnd, SW_SHOW);	
	}
	else
	{
		MessageBox(hwnd, "You must be online to add MSN contacts", MIRANDANAME, MB_OK);
	}
}*/ 