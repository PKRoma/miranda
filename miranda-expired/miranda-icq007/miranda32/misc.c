/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000  Roland Rabien

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

For more information, e-mail figbug@users.sourceforge.net
*/
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <stdio.h>
//#include <string.h>

#include "resource.h"
#include "miranda.h"
#include "internal.h"
#include "global.h"
#include "contact.h"
#include "pluginapi.h"
#include "plugins.h"


	#include "MSN_global.h"




VOID ChangeStatus(HWND hWnd, int newStatus)
{
	char conmsg[30];
	char tmpst[25];
	opts.ICQ.status = newStatus;

	if (newStatus == STATUS_OFFLINE)
	{
		rto.askdisconnect = TRUE;
		opts.ICQ.laststatus = -1;
		if (opts.ICQ.online) 
		{

			icq_Logout(plink);
			icq_Disconnect(plink);

			
		}
		else
		{
			KillTimer(ghwnd,TM_TIMEOUT);
			UpdateUserStatus();
		}
		
	
	}
	else
	{
		if (!opts.ICQ.online)
		{
			//connectionAlive=60;
			//set a timer for timeout
			
			GetStatusString(newStatus,TRUE,tmpst);
			sprintf(conmsg,"Connecting...[%s]",tmpst);
			SetStatusTextEx(conmsg,STATUS_SLOT_ICQ);

			SetTimer(ghwnd,TM_TIMEOUT,TIMEOUT_TIMEOUT,NULL);
			rto.net_activity = TRUE;
			if (icq_Connect(plink, opts.ICQ.hostname, opts.ICQ.port)==-1)
			{
				
				SetStatusTextEx("Error Connecting",STATUS_SLOT_ICQ);
				KillTimer(ghwnd,TM_TIMEOUT);
			}
			icq_Login(plink, newStatus);

			
		}
		else
		{
			UpdateUserStatus();
			icq_ChangeStatus(plink, newStatus);
			
		}
		opts.ICQ.laststatus = newStatus;
	}



	
}

void LoadContactList(OPTIONS *opts, OPTIONS_RT *rto)
{
	FILE *fp;
	int sz;
	char ft[5];
	if (opts->contactlist) fp = fopen(opts->contactlist, "rb");
	
	if (fp)
	{
		//if (opts->newctl)
		strcpy(ft,opts->contactlist+(strlen(opts->contactlist)-4));
		
		if (stricmp(ft,".CN2")==0)
		{
			while (fread(&sz, sizeof(int), 1, fp))
			{
				memset(&opts->clist[opts->ccount], 0, sizeof(CONTACT));
				fseek(fp, -(int)sizeof(int),	SEEK_CUR);
				fread(&opts->clist[opts->ccount], sizeof(CONTACT), 1, fp);
				opts->clist[opts->ccount].structsize = sizeof(CONTACT);
				
				opts->clist[opts->ccount].status = STATUS_OFFLINE & 0xFFFF;

				if (opts->clist[opts->ccount].id==CT_MSN)
				{
					MSN_ccount++;
					opts->clist[opts->ccount].status=MSN_INTSTATUS_OFFLINE;
				}
				if (opts->clist[opts->ccount].id==CT_ICQ)
				{
					icq_ContactAdd(plink, opts->clist[opts->ccount].uin);
				}
				
				DisplayContact(&opts->clist[opts->ccount]);

				opts->ccount++;
			}
			fclose(fp);
		}
		else
		{		
			CONTACT_OLD oldct;

			while (fread(&oldct, sizeof(CONTACT_OLD), 1, fp))
			{
				memset(&opts->clist[opts->ccount], 0, sizeof(CONTACT));
				memcpy(&opts->clist[opts->ccount].uin, &oldct, sizeof(oldct));

				opts->clist[opts->ccount].id = CT_ICQ;
				opts->clist[opts->ccount].structsize = sizeof(CONTACT);
				opts->clist[opts->ccount].status = STATUS_OFFLINE & 0xFFFF;
				
				DisplayContact(&opts->clist[opts->ccount]);				
				icq_ContactAdd(plink, opts->clist[opts->ccount].uin);
				opts->ccount++;
			}
			fclose(fp);
			//CHANGE FILE to .cnt2
			opts->contactlist[strlen(opts->contactlist)-1]='2';
			MessageBox(ghwnd,"Your Conact database (.cnt file) has been upgraded (to a .cn2 file). Your old database still exists but will not be used.",MIRANDANAME,MB_OK);
					
		}
	}
	SortContacts();
}

void SaveContactList(OPTIONS *opts, OPTIONS_RT *rto)
{
	FILE *fp;

	int i;

	//opts->newctl = TRUE;

	if (opts->contactlist) fp = fopen(opts->contactlist, "wb");

	if (fp)
	{
		for (i = 0; i < opts->ccount; i++)
		{			
			fwrite(&opts->clist[i], sizeof(CONTACT), 1, fp);
		}
		fclose(fp);
	}
}

void ChangeContactStatus(unsigned long uin, unsigned long status)
{
	unsigned long oldstatus;
	int i;

	for (i = 0; i < opts.ccount; i++)
	{
		if (uin == opts.clist[i].uin)
		{
			oldstatus=opts.clist[i].status;
			opts.clist[i].status = status & 0xFFFF;

			if (MsgQue_MsgsForUIN(uin)>0)
			{//the Unread Msg icon will be there, so dont change their icon
				
			}
			else
			{
				SetContactIcon(uin, status,CT_ICQ);
			}
			//TELL PLugins someone has changed status
			Plugin_NotifyPlugins(PM_CONTACTSTATUSCHANGE, (WPARAM)oldstatus,(LPARAM)&opts.clist[i]);//lparam pointer to contact struct
		}
	}

	
}

int GetStatusIconIndex(unsigned long status)
{
	int index;

	switch (status & 0xFFFF)
	{
		case STATUS_OFFLINE: 
		case 0xFFFF:
			index = 0;
			break;
		case STATUS_ONLINE:
			index = 1;
			break;
		case STATUS_INVISIBLE:
			index = 2;
			break;
		case STATUS_NA:
		case 0x0005:
			index = 4;
			break;
		case STATUS_FREE_CHAT:
			index = 3;
			break;
		case STATUS_OCCUPIED:
		case 0x0011:
			index = 7; //4;
			break;
		case STATUS_AWAY:
			index = 5;
			break;
		case STATUS_DND:
		case 0x0013:
			index = 6; //4;
			break;
		default:
			index = 1;
			break;
	}
	return index;
}

//void DisplayMessageRecv(unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, char *msg)
void DisplayMessageRecv(int msgid)
{
	//msg stays in array untill the close btn has pressed(not when the window is opend)
	HWNDLIST *node;
	DLG_DATA dlg_data;

	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_RECVMSG;
	node->next = rto.hwndlist;
	node->uin = msgque[msgid].uin;
	rto.hwndlist = node;

	dlg_data.uin = msgque[msgid].uin;
	dlg_data.hour = msgque[msgid].hour;
	dlg_data.minute = msgque[msgid].minute;
	dlg_data.day = msgque[msgid].day;
	dlg_data.month = msgque[msgid].month;
	dlg_data.year = msgque[msgid].year;
	dlg_data.msg = msgque[msgid].msg;
	
	dlg_data.msgid=msgid;
	msgque[msgid].beingread=TRUE;

	node->hwnd = CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_MSGRECV), NULL, DlgProcMsgRecv, (LPARAM)&dlg_data);
	//ShowWindow(node->hwnd, (opts.flags1 & FG_POPUP) ? SW_SHOW : SW_SHOWMINIMIZED);
	ShowWindow(node->hwnd,  SW_SHOW);
	SetForegroundWindow(node->hwnd);
	PostMessage(node->hwnd, TI_POP, 0, 0);
}

void DisplayUrlRecv(int msgid)
{
	HWNDLIST *node;
	DLG_DATA dlg_data;

	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_RECVURL;
	node->next = rto.hwndlist;
	node->uin = msgque[msgid].uin;
	rto.hwndlist = node;

	dlg_data.uin = msgque[msgid].uin;
	dlg_data.hour = msgque[msgid].hour;
	dlg_data.minute = msgque[msgid].minute;
	dlg_data.day = msgque[msgid].day;
	dlg_data.month = msgque[msgid].month;
	dlg_data.year = msgque[msgid].year;
	
	dlg_data.descr = msgque[msgid].msg;
	dlg_data.msg = msgque[msgid].msg;

	dlg_data.url = msgque[msgid].url;
	dlg_data.msgid=msgid;
	
	msgque[msgid].beingread=TRUE;
	
	node->hwnd = CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_URLRECV), NULL, DlgProcUrlRecv, (LPARAM)&dlg_data);
	//ShowWindow(node->hwnd, (opts.flags1 & FG_POPUP) ? SW_SHOW : SW_SHOWMINIMIZED);
	ShowWindow(node->hwnd,  SW_SHOW);
	SetForegroundWindow(node->hwnd);
}

int GetModelessType(HWND hwnd)
{
	HWNDLIST *node;

	int type;

	node = rto.hwndlist;
	while (node)
	{
		if (hwnd == node->hwnd)
		{
			type = node->windowtype;
			hwnd = NULL;
		}
		else
		{
			node = node->next;
		}
	}
	return type;
}

unsigned int *GetAckPtr(HWND hwnd)
{
	HWNDLIST *node = rto.hwndlist;
	while (node)
	{
		if (hwnd == node->hwnd)
		{
			return &(node->ack);
		}
		node = node->next;
	}
	return NULL;
}

void DeleteWindowFromList(HWND hwnd)
{
	HWNDLIST *node;
	HWNDLIST *curr;

	node = rto.hwndlist;
	if (node->hwnd == hwnd)
	{
		curr = node;

		rto.hwndlist = curr->next;

		free(curr);
		return;
	}
	else
	{
		while (node)
		{
			if (hwnd == node->next->hwnd)
			{
				curr = node->next;
				node->next = curr->next;

				free(curr);
				return;
			}
			node = node->next;
		}
	}
}

char *LookupContactNick(unsigned long uin)
{
	int i;

	for (i = 0; i < opts.ccount; i++)
	{
		if (uin == opts.clist[i].uin)
		{
			if (*opts.clist[i].nick == 0)
			{
				return NULL;
			}
			else
			{
				return opts.clist[i].nick;
			}
		}
	}
	return NULL;
}

void UpdateUserStatus(void)
{
	HMENU hmenu;

	char buffer[128];
	unsigned long status;

	hmenu = GetMenu(ghwnd);
	hmenu = GetSubMenu(hmenu, 1);

	status = opts.ICQ.status;

	CheckMenuItem(hmenu, ID_ICQ_STATUS_OFFLINE, MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_ICQ_STATUS_ONLINE, MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_ICQ_STATUS_INVISIBLE, MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_ICQ_STATUS_NA, MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_ICQ_STATUS_FREECHAT, MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_ICQ_STATUS_OCCUPIED, MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_ICQ_STATUS_AWAY, MF_UNCHECKED);
	CheckMenuItem(hmenu, ID_ICQ_STATUS_DND, MF_UNCHECKED);

	/*switch (status)
	{
		case STATUS_OFFLINE:
			sprintf(buffer, "%s", "Offline");
			CheckMenuItem(hmenu, ID_ICQ_STATUS_OFFLINE, MF_CHECKED);
			break;
		case STATUS_ONLINE:
			sprintf(buffer, "%s", "Online");
			CheckMenuItem(hmenu, ID_ICQ_STATUS_ONLINE, MF_CHECKED);
			break;
		case STATUS_INVISIBLE:
//		case STATUS_INVISIBLE_2:
			sprintf(buffer, "%s", "Online: Invisible");
			CheckMenuItem(hmenu, ID_ICQ_STATUS_INVISIBLE, MF_CHECKED);
			break;
		case STATUS_NA:
			sprintf(buffer, "%s", "Online: NA");
			CheckMenuItem(hmenu, ID_ICQ_STATUS_NA, MF_CHECKED);
			break;
		case STATUS_FREE_CHAT:
			sprintf(buffer, "%s", "Online: Free Chat");
			CheckMenuItem(hmenu, ID_ICQ_STATUS_FREECHAT, MF_CHECKED);
			break;
		case STATUS_OCCUPIED:
			sprintf(buffer, "%s", "Online: Occupied");
			CheckMenuItem(hmenu, ID_ICQ_STATUS_OCCUPIED, MF_CHECKED);
			break;
		case STATUS_AWAY:
			sprintf(buffer, "%s", "Online: Away");
			CheckMenuItem(hmenu, ID_ICQ_STATUS_AWAY, MF_CHECKED);
			break;
		case STATUS_DND:
			sprintf(buffer, "%s", "Online: DND");
			CheckMenuItem(hmenu, ID_ICQ_STATUS_DND, MF_CHECKED);
			break;
	}	*/
	GetStatusString(status,TRUE,buffer);
	
	SetStatusTextEx(buffer,STATUS_SLOT_ICQ);

	TrayIcon(ghwnd, TI_UPDATE);

	//TELL PLugins we have changed oour status
	Plugin_NotifyPlugins(PM_STATUSCHANGE,(WPARAM)status,0);//wparam, new status
}

void GetStatusString(int status,BOOL shortmsg,char*buffer)
{
	
	switch (status)
	{
		case STATUS_OFFLINE:
			sprintf(buffer, "%s", "Offline");
			break;
		case STATUS_ONLINE:
			sprintf(buffer, "%s", "Online");
			break;
		case STATUS_INVISIBLE:
//		case STATUS_INVISIBLE_2:
			if (shortmsg)
				sprintf(buffer, "%s", "Invis");
			else
				sprintf(buffer, "%s", "Online: Invisible");
			
			break;
		case STATUS_NA:
			if (shortmsg)
				sprintf(buffer, "%s", "NA");
			else
				sprintf(buffer, "%s", "Online: NA");

			break;
		case STATUS_FREE_CHAT:
			if (shortmsg)
				sprintf(buffer, "%s", "Free Chat");
			else
				sprintf(buffer, "%s", "Online: Free Chat");

			break;
		case STATUS_OCCUPIED:
			if (shortmsg)
				sprintf(buffer, "%s", "Occupied");
			else
				sprintf(buffer, "%s", "Online: Occupied");

			break;
		case STATUS_AWAY:
			if (shortmsg)
				sprintf(buffer, "%s", "Away");
			else
				sprintf(buffer, "%s", "Online: Away");

			break;
		case STATUS_DND:
			if (shortmsg)
				sprintf(buffer, "%s", "DND");
			else
				sprintf(buffer, "%s", "Online: DND");
			break;
	}	

	
	
}
void ContactDoubleClicked(HWND hwnd)
{
	int mqid;
	unsigned long uin;
	char *name;

	
	if (opts.ICQ.online || opts.MSN.logedin)
	{
		HTREEITEM hitem;
		hitem = TreeView_GetSelection(rto.hwndContact);
		if (hitem)
		{
			uin = TreeToUin(hitem);
			
			

			if (CheckForDupWindow(uin,WT_RECVMSG)!=NULL || CheckForDupWindow(uin,WT_RECVURL)!=NULL || MsgQue_MsgsForUIN(uin)==0)
			{ //no msgs waiting, or its alreadt being displyed, so a send msg instead
				//if ((IsMSN(uin) && MSN.logedin) || (!IsMSN(uin) && rto.online))
				//{
					name = LookupContactNick(uin);
					SendIcqMessage(uin, name);
				//}
			}
			else
			{//read msg/url
				
				mqid=MsgQue_FindFirst(uin);
				if (msgque[mqid].type==URL)
				{
					DisplayUrlRecv(mqid);
				}
				else if (msgque[mqid].type==INSTMSG)
				{
					DisplayMessageRecv(mqid);
				}
			}
		}
	}
}

unsigned int GetSelectedUIN(HWND hwnd)
{
	HTREEITEM hitem;
	TVITEM item;

	unsigned int uin = 0;

	hitem = TreeView_GetSelection(hwnd);
	if (hitem)
	{
		item.mask = TVIF_PARAM;
		item.hItem = hitem;	
		TreeView_GetItem(hwnd, &item);
		uin = item.lParam;
	}
	return uin;
}

void SendIcqMessage(unsigned long uin, char *name)
{
	HWNDLIST *node;
	DLG_DATA dlg_data;

	HWND wnd;
	if ((wnd=CheckForDupWindow(uin,WT_SENDMSG))!=NULL)
	{
		SetForegroundWindow(wnd);
		return;
	}


	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_SENDMSG;
	node->next = rto.hwndlist;
	node->uin = uin;
	rto.hwndlist = node;

	dlg_data.uin = uin;
	dlg_data.nick = name;
	
	node->hwnd = CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_MSGSEND), NULL, DlgProcMsgSend, (LPARAM)&dlg_data);
	ShowWindow(node->hwnd, SW_SHOW);
}

void SendIcqUrl(unsigned long uin, char *name)
{
	HWNDLIST *node;
	DLG_DATA dlg_data;

	if (IsMSN(uin))
	{
		MessageBox(ghwnd,"You can't send URLs to MSN Contacts.",MIRANDANAME,MB_OK);
		return;
	}
	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_SENDURL;
	node->next = rto.hwndlist;
	node->uin = uin;
	rto.hwndlist = node;

	dlg_data.uin = uin;
	dlg_data.nick = name;
	
	node->hwnd = CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_SENDURL), NULL, DlgProcUrlSend, (LPARAM)&dlg_data);
	ShowWindow(node->hwnd, SW_SHOW);
}

void AddContactbyIcq(HWND hwnd)
{
	HWNDLIST *node;

	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_ADDCONT;
	node->next = rto.hwndlist;
	rto.hwndlist = node;

	node->hwnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_ADDCONTACT1), ghwnd, DlgProcAddContact1);
	ShowWindow(node->hwnd, SW_SHOW);	
}

void AddContactbyEmail(HWND hwnd)
{
	if (opts.ICQ.online)
	{
		HWNDLIST *node;

		node = malloc(sizeof(HWNDNODE));

		node->windowtype = WT_ADDCONT;
		node->next = rto.hwndlist;
		rto.hwndlist = node;

		node->hwnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_ADDCONTACT2), ghwnd, DlgProcAddContact2);
		ShowWindow(node->hwnd, SW_SHOW);	
	}
	else
	{
		MessageBox(hwnd, "You must be online to search for contacts", MIRANDANAME, MB_OK);
	}
}

void AddContactbyName(HWND hwnd)
{
	if (opts.ICQ.online)
	{
		HWNDLIST *node;

		node = malloc(sizeof(HWNDNODE));

		node->windowtype = WT_ADDCONT;
		node->next = rto.hwndlist;
		rto.hwndlist = node;

		node->hwnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_ADDCONTACT3), ghwnd, DlgProcAddContact3);
		ShowWindow(node->hwnd, SW_SHOW);	
	}
	else
	{
		MessageBox(hwnd, "You must be online to search for contacts", MIRANDANAME, MB_OK);
	}
}

void OpenResultWindow(void)
{
	HWNDLIST *node;

	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_RESULT;
	node->next = rto.hwndlist;
	rto.hwndlist = node;

	node->hwnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_RESULTS), ghwnd, DlgProcResult);
	ShowWindow(node->hwnd, SW_SHOW);
}

int FirstTime(char *lpCmdLine)
{
	// display the gpl
	if (DialogBox(ghInstance, MAKEINTRESOURCE(IDD_GNUPL), NULL, DlgProcGPL) != 3)
	{
		return 1;
	}

	// get the UIN and password
	if (DialogBox(ghInstance, MAKEINTRESOURCE(IDD_PASSWORD), NULL, DlgProcPassword) == 1)
	{
		return 1;
	}
	

	sprintf(opts.contactlist, "%d.cn2", opts.ICQ.uin);
	sprintf(opts.history, "%d.hst", opts.ICQ.uin);
	sprintf(opts.grouplist, "%d.grp", opts.ICQ.uin);

	return 0;
}

void TrayIcon(HWND hwnd, int action)
{
	NOTIFYICONDATA nid;
	static int fc=8;
	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	nid.uID = 69;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = TI_CALLBACK;
	nid.hIcon = NULL;
	if (opts.ICQ.online)
	{
		//nid.hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE));
		nid.hIcon=ImageList_GetIcon(rto.himlIcon,GetStatusIconIndex(opts.ICQ.status),ILD_NORMAL);
	}
	else
	{
		//nid.hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYOFFLINE));
		nid.hIcon=ImageList_GetIcon(rto.himlIcon,GetStatusIconIndex(STATUS_OFFLINE),ILD_NORMAL);
	}
	if (msgquecnt>0 && MsgQue_ReadCount()!=msgquecnt) //if the readcnt=msgquecnyt then all the msgs are being read
	{//have unread msgs, show msg icon in sys
	
			if (nid.hIcon) DestroyIcon(nid.hIcon);
			nid.hIcon=ImageList_GetIcon(rto.himlIcon,fc,ILD_NORMAL);
			if (fc==8)
				fc=9; //unread(2) flash
			else
				fc=8; //unread

			SetTimer(ghwnd,TM_MSGFLASH,TIMEOUT_MSGFLASH,NULL);
	}
	else
	{
		KillTimer(ghwnd,TM_MSGFLASH);
	}

	if (opts.ICQ.uin)
	{
		sprintf(nid.szTip, "Miranda ICQ: %d", opts.ICQ.uin);
	}
	else
	{
		strcpy(nid.szTip, "Miranda ICQ");
	}
	switch (action)
	{
		case TI_CREATE:
			Shell_NotifyIcon(NIM_ADD, &nid);
			break;
		case TI_DESTROY:
			Shell_NotifyIcon(NIM_DELETE, &nid);
			break;
		case TI_UPDATE:
			nid.uFlags = NIF_ICON;
			Shell_NotifyIcon(NIM_MODIFY, &nid);
			break;
	}
	DestroyIcon(nid.hIcon);
}

unsigned long GetDlgUIN(HWND hwnd)
{
	HWNDLIST *node;

	node = rto.hwndlist;
	while (node)
	{
		if (hwnd == node->hwnd)
		{
			return node->uin;
		}
		else
		{
			node = node->next;
		}
	}
	return 0;
}

void AddToContactList(unsigned int uin)
{
	if (uin)
	{
		// add the guy to the contact list
		opts.clist[opts.ccount].uin = uin;
		opts.clist[opts.ccount].id=CT_ICQ; //default to ICQ contact
		strcpy(opts.clist[opts.ccount].nick, "");
		strcpy(opts.clist[opts.ccount].first, "");
		strcpy(opts.clist[opts.ccount].last, "");
		strcpy(opts.clist[opts.ccount].email, "");
		opts.clist[opts.ccount].status = STATUS_OFFLINE & 0xFFFF;
		// add the guy to the list view
		DisplayContact(&opts.clist[opts.ccount]);

		// do network stuff
		icq_ContactAdd(plink, opts.clist[opts.ccount].uin);
		if (opts.ICQ.online)
		{
			icq_SendContactList(plink);
			icq_SendInfoReq(plink, uin);
		}
		opts.ccount++;
	}
	SaveContactList(&opts, &rto);
	SortContacts();
}

void RemoveContact(HWND hwnd)
{
	HTREEITEM hitem;
	int i;
	unsigned int uin;

	hitem = TreeView_GetSelection(hwnd);
	if (hitem)
	{
		uin = TreeToUin(hitem);
		if (IsGroup(uin))
		{
			if (MessageBox(ghwnd,"Do you really want to delete this group?",MIRANDANAME,MB_YESNO)==IDNO){return;}
			DeleteGroup(hwnd);
			return;
		}
		
		if (MessageBox(ghwnd,"Do you really want to delete this contact?",MIRANDANAME,MB_YESNO)==IDNO){return;}
		
		TreeView_DeleteItem(rto.hwndContact, hitem);
		
		for (i = 0; i < opts.ccount; i++)
		{
			if (uin == opts.clist[i].uin)
			{
				if (IsMSN(uin))
				{//tell the MSN server we have killed the person off
					MSN_RemoveContact(opts.clist[i].email);
				}
				opts.clist[i] = opts.clist[--opts.ccount];
				return;
			}
		}
				
	}
	SortContacts();
}

void DisplayDetails(HWND hwnd)
{
	HTREEITEM hitem;

	hitem = TreeView_GetSelection(hwnd);
	if (hitem)
	{
		DisplayUINDetails(TreeToUin(hitem));
	}
}

void DisplayUINDetails(unsigned int uin)
{
	HWNDLIST *node;

	if (IsMSN(uin))
	{	
		MessageBox(ghwnd,"No Details for MSN Contacts.",MIRANDANAME,MB_OK);
		return;
	}
	icq_SendInfoReq(plink, uin);
	icq_SendExtInfoReq(plink, uin);	
	icq_SendMetaInfoReq(plink, uin);

	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_DETAILS;
	node->next = rto.hwndlist;
	rto.hwndlist = node;

	node->hwnd = CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_DETAILS), ghwnd, DlgProcDetails, uin);
	ShowWindow(node->hwnd, SW_SHOW);
}

void RenameContact(HWND hwnd)
{
	HTREEITEM hitem;

	hitem = TreeView_GetSelection(hwnd);
	if (hitem)
	{
		SetFocus(hwnd);
		TreeView_EditLabel(hwnd, hitem);
	}
}

int InContactList(unsigned int uin)
{
	int i;

	for (i = 0; i < opts.ccount; i++)
	{
		if (opts.clist[i].uin == uin) return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK ChildControlProc(HWND hwnd, LPARAM lParam)
{
	int sz;
	int res;
	char buf[128];
	
	res = SendMessage(hwnd, WM_GETTEXT, 128, (LPARAM)buf);
	buf[res] = 0;
	
	if (res && (!strncmp(buf, "http", 4) || !strncmp(buf, "ftp", 3)))   {
		sz = strlen(buf) + strlen((char *)(lParam + sizeof(int))) + 1;     if (sz <= *((int *)lParam) && !strstr((char *)(lParam + sizeof(int)),buf))
		{
			strcat((char *)(lParam + sizeof(int)), buf);
			strcat((char *)(lParam + sizeof(int)), "\n");
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK BrowserWindowProc(HWND hwnd, LPARAM lParam)
{
	char buf[128];
	
	SendMessage(hwnd, WM_GETTEXT, 128, (LPARAM)buf);
	
	if (strstr(buf, "Internet Explorer") ||
		strstr(buf, "Netscape") ||
		strstr(buf, "Opera") ||
		strstr(buf, "Mozilla"))
	{
		EnumChildWindows(hwnd, ChildControlProc, lParam);
	}
	return TRUE;
}

// parameters are a string to get the data and the length of the string // urls are '\n' delimited.
// returns 0 on error or the length of the string
int GetOpenURLs(char *buffer, int sz)
{
	if (sz > (sizeof(int) + 1))
	{
		*((int *)buffer) = sz;
		buffer[sizeof(int)] = 0;
		
		EnumDesktopWindows(NULL, BrowserWindowProc, (LPARAM)buffer); 
		
		memmove(buffer, buffer + sizeof(int), strlen(buffer + sizeof(int))+1); 
		
		return strlen(buffer);
	}
	else
	{
		*buffer = '\0';
		return 0;
	}
}

unsigned int GetUserFlags(unsigned int uin)
{
	int i;
	for (i = 0; i < opts.ccount; i++)
	{
		if (uin == opts.clist[i].uin) return opts.clist[i].flags;
	}
	return 0;
}

void SetUserFlags(unsigned int uin, unsigned int flg)
{
	int i;
	for (i = 0; i < opts.ccount; i++)
	{
		if (uin == opts.clist[i].uin) opts.clist[i].flags |= flg;
	}
}

void ClrUserFlags(unsigned int uin, unsigned int flg)
{
	int i;
	for (i = 0; i < opts.ccount; i++)
	{
		if (uin == opts.clist[i].uin) opts.clist[i].flags &= ~flg;
	}
}

void AddToHistory(unsigned int uin, time_t evt_t, int dir, char *msg)
{
	FILE *fp;
	struct tm *et;
	char dirstr[30];
	et = localtime(&evt_t);
	fp = fopen(opts.history, "ab");
	
	/*if (dir == '<') dirstr = "Incoming message from";
	if (dir == '>') dirstr = "Outgoing message to";*/

	switch (dir)
	{
	case HISTORY_MSGRECV:
		strcpy(dirstr,"Incoming message from");
		break;
	case HISTORY_MSGSEND:
		strcpy(dirstr,"Outgoing message to");
		break;
	case HISTORY_URLRECV:
		strcpy(dirstr,"Incoming URL from");
		break;
	case HISTORY_URLSEND:
		strcpy(dirstr,"Outgoing URL to");
		break;
	case HISTORY_AUTHREQ:
		strcpy(dirstr,"Auth Request from");
		break;
	case HISTORY_ADDED:
		strcpy(dirstr,"You were added by");
		break;
	}
	
	fprintf(fp, "%d#%s %s at %d:%2.2d on %2.2d/%2.2d/%4.4d %c\r\n", uin, dirstr, LookupContactNick(uin), et->tm_hour, et->tm_min, et->tm_mday, et->tm_mon + 1, et->tm_year + 1900, dir);
	fprintf(fp, "%s\r\n", msg);
	fprintf(fp, "%c%c%c%c\r\n", 0xEE, 0xEE, 0xEE, 0xEE);	

	fclose(fp);
}

void DisplayHistory(HWND hwnd, unsigned int uin)
{
	ShowWindow(CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_HISTORY), hwnd, DlgProcHistory, uin), SW_SHOW);
}

time_t MakeTime(int hour, int minute, int day, int month, int year)
{
	struct tm et;
	memset(&et, 0, sizeof(et));
	et.tm_hour = hour - 1;
	et.tm_min = minute;
	et.tm_mday = day;
	et.tm_mon = month - 1;
	et.tm_year = year - 1900;

	return mktime(&et);
}

void PlaySoundEvent(char *event)
{
	char sound[MAX_PATH];
	 
	if (opts.playsounds && LoadSound(event, sound) && *sound)
	{
		PlaySound(sound, NULL, SND_ASYNC | SND_FILENAME);
	}
}

int LoadSound(char *key, char *sound)
{
	HKEY hkey;
	char szKey[MAX_PATH];
	DWORD cbSound;
	int res;
	sound[0]=0; //just in case we dont get anything from the reg

	res = FALSE;	
	cbSound = MAX_PATH;
		
	sprintf(szKey, "AppEvents\\Schemes\\Apps\\MirandaICQ\\%s\\.current", key);
		
	if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_READ, &hkey) == ERROR_SUCCESS) 
	{
		if(RegQueryValueEx(hkey, NULL, 0, NULL, sound, &cbSound) ==	ERROR_SUCCESS)
		{
			res = TRUE;
		}
		RegCloseKey(hkey);
	}
	return res;
}

int SaveSound(char *key, char *sound)
{
	HKEY hkey;
	char szKey[MAX_PATH];
	DWORD cbSound;
	DWORD dsp;
	int res;

	res = FALSE;	
	cbSound = MAX_PATH;
		
	sprintf(szKey, "AppEvents\\Schemes\\Apps\\MirandaICQ\\%s\\.current", key);
		
	if (RegCreateKeyEx(HKEY_CURRENT_USER, szKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &dsp) == ERROR_SUCCESS) 
	{
		cbSound = strlen(sound);
		if(RegSetValueEx(hkey, NULL, 0, REG_SZ, sound, cbSound) ==	ERROR_SUCCESS)
		{
			res = TRUE;
		}
		RegCloseKey(hkey);
	}
	return res;
}

int BrowseForWave(HWND hwnd)
{
	char buf[MAX_PATH] = {0};

	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetParent(hwnd);
	ofn.hInstance = NULL;
	ofn.lpstrFilter = "Wave Files\0*.WAV\0";
	ofn.lpstrFile = buf;
	ofn.Flags = OFN_FILEMUSTEXIST;
	ofn.nMaxFile = sizeof(buf);
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = "wav";

	if (GetOpenFileName(&ofn))
		SetWindowText(hwnd, buf);

	return 0;
}


void ShowHide()
{
	if (rto.hidden)
	{
		ShowWindow(ghwnd, SW_RESTORE);
		rto.hidden = FALSE;
		SetWindowPos(ghwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		if (!(opts.flags1 & FG_ONTOP)) SetWindowPos(ghwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		SetForegroundWindow(ghwnd);
	}
	else
	{
		if (opts.flags1 & FG_ONTOP)
		{
			ShowWindow(ghwnd, SW_HIDE);
			rto.hidden = TRUE;
		}
		else
		{
			RECT crect;
			RECT wrect;

			GetClipBox(GetDC(ghwnd), &crect);
			GetClientRect(ghwnd, &wrect);

			if (crect.bottom == wrect.bottom &&
				crect.top == wrect.top &&
				crect.left == wrect.left &&
				crect.right == wrect.right)
			{
				ShowWindow(ghwnd, SW_HIDE);
				rto.hidden = TRUE;				
			}
			else
			{
				SetWindowPos(ghwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				SetWindowPos(ghwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);			
			}
		}
	}
}


HWND CheckForDupWindow(unsigned long uin,long wintype)
{
	HWNDLIST *node;

	node = rto.hwndlist;
	if (node==NULL){return FALSE;}
	if (node->uin == uin && node->windowtype==wintype)
	{
		
	return node->hwnd;
		
	}
	else
	{
		while (node)
		{
			if (node->next==NULL){return FALSE;}
			if (uin == node->next->uin && node->next->windowtype==wintype)
			{
				return node->next->hwnd;
			}
			node = node->next;
		}
	}

	return NULL;

}

void ShowNextMsg(int mqid) //shows next msg if it exists , also removes old msg from msgque array
{//DONT rearrange, remove on or both fo the msgque_remove ,their positioning (mainly the top one) is important
	unsigned long uin;
	int newmqid;
	
	if (MsgQue_MsgsForUIN(msgque[mqid].uin)>1)
	{
		uin=msgque[mqid].uin;
		MsgQue_Remove(mqid);

		newmqid=MsgQue_FindFirst(uin);
		
		if (msgque[mqid].type==URL)
		{//URL
			DisplayUrlRecv(newmqid);
		}
		else if (msgque[mqid].type==INSTMSG)
		{//msg
			DisplayMessageRecv(newmqid);
		}
		
	}
	else
	{
		uin=msgque[mqid].uin;
		MsgQue_Remove(mqid); //remove must be bore setgotmsgicon func
		SetGotMsgIcon(uin,FALSE);
		
	}

}


void SetCaptionToNext(unsigned long uin) //sets the cancel btn to Next (used when a msg is qued up)
{
	HWND msgwnd;
	msgwnd=CheckForDupWindow(uin,WT_RECVMSG);
	if (msgwnd)
	{	
		SetDlgItemText(msgwnd, IDCANCEL, "&Next");
	}
	
	msgwnd=CheckForDupWindow(uin,WT_RECVURL);
	if (msgwnd)
	{	
		SetDlgItemText(msgwnd, IDCANCEL, "&Next");
	}			

}


void SetGotMsgIcon(unsigned long uin,BOOL gotmsg)
{
	HTREEITEM hitem;
	TVITEM item;
	
	int i;

	hitem = UinToTree(uin);
	if (hitem)
	{
		item.mask = TVIF_IMAGE |TVIF_SELECTEDIMAGE ;
		item.hItem = hitem;
		if (gotmsg)
		{
			item.iImage = 8; //IMAGE ID (9th pos -1)
			
		}
		else
		{
			for (i = 0; i < opts.ccount; i++)
			{
				if (uin == opts.clist[i].uin)
				{
					if (opts.clist[i].id==CT_MSN)
						item.iImage=opts.clist[i].status;
					else
						item.iImage=GetStatusIconIndex(opts.clist[i].status);

					break;
				}
			}
			
		}
		item.iSelectedImage=item.iImage;

		TreeView_SetItem(rto.hwndContact, &item);
	}

	
	TrayIcon(ghwnd, TI_UPDATE);

}

void SendFiles(HWND hwnd, unsigned long uin)
{
	char buf[MAX_PATH];

	int cnt;
	char **files;

	OPENFILENAME ofn;
	buf[0]=NULL;

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = ghInstance;
	ofn.lpstrFilter = "All Files\0*.*\0";
	ofn.lpstrInitialDir=NULL;
	ofn.lpstrFile = buf;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	ofn.nMaxFile = sizeof(buf);
	
	if (GetOpenFileName(&ofn))
	{
		files = calloc(20, sizeof(char*));
		if (GetFileAttributes(buf) == FILE_ATTRIBUTE_DIRECTORY)
		{
			char *p;

			cnt = 0;
			p = buf + strlen(buf) + 1;

			while (*p && cnt < 20)
			{
				files[cnt] = calloc(MAX_PATH, sizeof(char));

				strcpy(files[cnt], buf);
				strcat(files[cnt], "\\");
				strcat(files[cnt], p);
				p += strlen(p) + 1;
				cnt++;
			}
		}
		else
		{
			cnt = 1;
			files[0] = calloc(MAX_PATH, sizeof(char));
			strcpy(files[0], buf);
		}
		icq_SendFileRequest(plink, uin, "Here are some files", files);
	}
	else
	{
		int i;
		//i = GetLastError();
		i=CommDlgExtendedError();
	}

}

void tcp_engage(void)
{
	WSADATA wsadata;

	WSAStartup(MAKEWORD(1,1),&wsadata);

	memset(&link, 0, sizeof(link));
	plink = &link;

	icq_LogLevel = ICQ_LOG_MESSAGE;
}

void tcp_disengage(void)
{	
	WSACleanup();
}

void IPtoString(unsigned long IP,char* strIP)
{//convert a 4 byte long IP into a string IP
	
	char tmp[15];
	sprintf(tmp,"%d.%d.%d.%d",((IP & 0xFF000000)>>24),((IP & 0x00FF0000)>>16),((IP & 0x0000FF00)>>8),(IP & 0x000000FF));
	strcpy(strIP,tmp);
}

void InitMenuIcons()
{
// customize menus
	HMENU hMenu;
	
	MENUITEMINFO lpmii;
	
	
	lpmii.fMask = MIIM_BITMAP | MIIM_DATA;
	lpmii.hbmpItem = HBMMENU_CALLBACK;			//LoadBitmap(ghInstance,MAKEINTRESOURCE(IDB_OCCUPIED));
	lpmii.dwItemData=MFT_OWNERDRAW;				//DRAW_ICONONLY;
	lpmii.cbSize = sizeof(lpmii);
	
	//STATUS MNU ICONS

	hMenu=GetSubMenu(GetMenu(ghwnd),1);

	SetMenuItemInfo (hMenu,ID_ICQ_STATUS_OFFLINE,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_ICQ_STATUS_ONLINE,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_ICQ_STATUS_AWAY,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_ICQ_STATUS_DND,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_ICQ_STATUS_NA,FALSE,&lpmii);	
	SetMenuItemInfo (hMenu,ID_ICQ_STATUS_OCCUPIED,FALSE,&lpmii);	
	SetMenuItemInfo (hMenu,ID_ICQ_STATUS_FREECHAT,FALSE,&lpmii);	
	SetMenuItemInfo (hMenu,ID_ICQ_STATUS_INVISIBLE,FALSE,&lpmii);
	
	lpmii.fMask=MIIM_BITMAP | MIIM_ID | MIIM_DATA;
	lpmii.wID=ID_ICQ_ADDCONTACT;
	SetMenuItemInfo (hMenu,0,TRUE,&lpmii);

	lpmii.fMask = MIIM_BITMAP | MIIM_DATA;
	SetMenuItemInfo(hMenu,ID_ICQ_VIEWDETAILS,FALSE,&lpmii);

	hMenu=GetSubMenu(hMenu,0);
	SetMenuItemInfo(hMenu,ID_ICQ_ADDCONTACT_BYICQ,FALSE,&lpmii);
	SetMenuItemInfo(hMenu,ID_ICQ_ADDCONTACT_BYNAME,FALSE,&lpmii);
	SetMenuItemInfo(hMenu,ID_ICQ_ADDCONTACT_BYEMAIL,FALSE,&lpmii);
	SetMenuItemInfo(hMenu,ID_ICQ_ADDCONTACT_IMPORT,FALSE,&lpmii);


	//ICQ MNU ICONS
	hMenu=GetSubMenu(GetMenu(ghwnd),0);
	//ADD Submenu (parent ICQ)
	
	//GIVE NAMES to mnu items that are popups (show a submenu)	
	lpmii.fMask=MIIM_BITMAP | MIIM_ID | MIIM_DATA;
	lpmii.wID=ID_ICQ_OPTIONS_PLUGINS;
	SetMenuItemInfo(hMenu,1,TRUE,&lpmii);	

	lpmii.fMask = MIIM_BITMAP | MIIM_DATA; //put back to normal
		
	SetMenuItemInfo(hMenu,ID_ICQ_OPTIONS,FALSE,&lpmii);	
	
	SetMenuItemInfo(hMenu,ID_ICQ_GLOBALHISTORY,FALSE,&lpmii);	
		
	//HLP MNU ICONS
	hMenu=GetSubMenu(GetMenu(ghwnd),3);
	
	SetMenuItemInfo (hMenu,ID_HELP_CKECKFORUPGRADES,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_HELP_WINDEX,FALSE,&lpmii);	
	SetMenuItemInfo (hMenu,ID_HELP_MIRANDAWEBSITE,FALSE,&lpmii);

	//MSN mnu
	//HLP MNU ICONS
	hMenu=GetSubMenu(GetMenu(ghwnd),2);
	
	SetMenuItemInfo (hMenu,ID_MSN_OFFLINE,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_MSN_ONLINE,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_MSN_BUSY,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_MSN_AWAY,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_MSN_BERIGHTBACK,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_MSN_ONTHEPHONE,FALSE,&lpmii);
	SetMenuItemInfo (hMenu,ID_MSN_OUTTOLUNCH,FALSE,&lpmii);
}

int GetMenuIconNo(long ID, BOOL *isCheck)
{
int i;
	switch (ID)
	{
	case ID_ICQ_STATUS_ONLINE:
		i=GetStatusIconIndex(STATUS_ONLINE);
		*isCheck=TRUE;
		break;
	case ID_ICQ_STATUS_OFFLINE:
		i=GetStatusIconIndex(STATUS_OFFLINE);
		*isCheck=TRUE;
		break;
	case ID_ICQ_STATUS_OCCUPIED:
		i=GetStatusIconIndex(STATUS_OCCUPIED);
		*isCheck=TRUE;
		break;
	case ID_ICQ_STATUS_AWAY:
		i=GetStatusIconIndex(STATUS_AWAY);
		*isCheck=TRUE;
		break;
	case ID_ICQ_STATUS_INVISIBLE:
		i=GetStatusIconIndex(STATUS_INVISIBLE);
		*isCheck=TRUE;
		break;
	case ID_ICQ_STATUS_NA:
		i=GetStatusIconIndex(STATUS_NA);
		*isCheck=TRUE;
		break;
	case ID_ICQ_STATUS_DND:
		i=GetStatusIconIndex(STATUS_DND);
		*isCheck=TRUE;
		break;
	case ID_ICQ_STATUS_FREECHAT:
		i=GetStatusIconIndex(STATUS_FREE_CHAT);
		*isCheck=TRUE;
		break;
	case ID_ICQ_OPTIONS:
		i=10;
		break;
	case ID_ICQ_ADDCONTACT:
	case ID_ICQ_ADDCONTACT_BYICQ:
	case ID_ICQ_ADDCONTACT_BYEMAIL:
	case ID_ICQ_ADDCONTACT_BYNAME:
		i=12;
		break;
	case ID_ICQ_VIEWDETAILS:
		i=11;
		break;
	case ID_HELP_WINDEX: 
	case ID_HELP_ABOUT:
		i=13;
		break;
	case ID_HELP_CKECKFORUPGRADES:
		i=14;
		break;
	case ID_ICQ_OPTIONS_PLUGINS:
		i=17;
		break;
	//case ID_ICQ_OPTIONS_PROXYSETTINGS:
	//	i=15;
	//	break;
	/*case ID_ICQ_OPTIONS_SOUNDOPTIONS:
		i=16;
		break;	
	
	case ID_ICQ_OPTIONS_GENERALOPTIONS:
		i=18;
		break;*/
	case ID_ICQ_OPTIONS_ICQPASSWORD:
		i=19;
		break;
	case ID_HELP_MIRANDAWEBSITE:
		i=20;
		break;
	case POPUP_SENDMSG: //USER POPUP MENU ICONS
		i=21;
		break;
	case POPUP_SENDURL:
		i=22;
		break;
	case POPUP_SENDEMAIL:
		i=23;
		break;
	case ID_ICQ_GLOBALHISTORY: //drop thru
	case POPUP_HISTORY:
		i=24;
		break;
	case POPUP_IGNORE:
		i=25;
		*isCheck=TRUE;
		break;
	case POPUP_USERDETAILS:
		i=11;
		
		break;
	case POPUP_RENAME:
		i=25;
		break;
	case POPUP_DELETE:
		i=26;
		break;
	case ID_ICQ_ADDCONTACT_IMPORT:
		i=28;
		break;
	case ID_MSN_OFFLINE:
		i=MSN_INTSTATUS_OFFLINE;
		break;
	case ID_MSN_ONLINE:
		i=MSN_INTSTATUS_ONLINE;
		break;
	case ID_MSN_BUSY:
		i=MSN_INTSTATUS_BUSY;
		break;
	case ID_MSN_AWAY:
		i=MSN_INTSTATUS_AWAY;
		break;
	case ID_MSN_BERIGHTBACK:
		i=MSN_INTSTATUS_BRB;
		break;
	case ID_MSN_ONTHEPHONE:
		i=MSN_INTSTATUS_PHONE;
		break;
	case ID_MSN_OUTTOLUNCH:
		i=MSN_INTSTATUS_LUNCH;
		break;

	/*case ID_POPUP_SENDURL:
		i=17;
		break;		
	case ID_POPUP_SENDMESSAGE:
		i=12;
		break;
	case ID_POPUP_IGNOREUSER:
		i=14;
		*isCheck=TRUE;
		break;
	case ID_POPUP_HISTORY:
		i=13;
		break;
	case ID_POPUP_DELETE:
		i=8;
		break;
	case ID_POPUP_RENAME:
		i=15;
		break;
	
	case ID_POPUP_USERDETAILS:
	case ID_ICQ_VIEWDETAILS:
		i=10;
		break;
	case ID_ICQ_STATUS:
		i=GetStatusIconIndex(rto.desiredStatus);
		break;
	
	
	default:
		i=10;
		break;*/
	}
	return i;
}

void parseCmdLine(char *p)
{
	char *token;

	strcpy(myprofile, "default");
	//GetCurrentDirectory(sizeof(mydir), mydir);
	GetModuleFileName(ghInstance, mydir, sizeof(mydir));
	*strrchr(mydir, '\\') = 0;
	strcat(mydir, "\\");

	token = strtok(p, " \t");
	while (token)
	{
		if (token[0] == '-' || token[0] == '/')
		{
			switch (token[1])
			{
				case 'd':
				case 'D':
					token += 2;
					memset(mydir, 0, sizeof(mydir));
					strcpy(mydir, token);
					if (mydir[strlen(mydir)-1]!='\\')
					{
						mydir[strlen(mydir)]   = '\\';
						mydir[strlen(mydir)+1] = '\0';
					}
					SetCurrentDirectory(mydir);
					break;
			}
		}
		else
		{
			strcpy(myprofile, token);
		}
		token = strtok(NULL, " \t");
	}
	free(p);
}

void OptionsWindow(HWND hwnd)
{
	PROPSHEETHEADER psh;
	int pages = 0;
	//int i;
	
	ppsp = calloc(sizeof(PROPSHEETPAGE), 8 + plugincnt);

	ppsp[pages].dwSize = sizeof(PROPSHEETPAGE);
	ppsp[pages].dwFlags = PSP_DEFAULT;
	ppsp[pages].hInstance = ghInstance;
	ppsp[pages].pszTitle = "ICQ Options";
	ppsp[pages].pszTemplate = MAKEINTRESOURCE(IDD_ICQOPT);
	ppsp[pages].pfnDlgProc = DlgProcICQOpts;
	pages++;

	ppsp[pages].dwSize = sizeof(PROPSHEETPAGE);
	ppsp[pages].dwFlags = PSP_DEFAULT;
	ppsp[pages].hInstance = ghInstance;
	ppsp[pages].pszTitle = "MSN Options";
	ppsp[pages].pszTemplate = MAKEINTRESOURCE(IDD_MSNOPT);
	ppsp[pages].pfnDlgProc = DlgProcMSNOpts;
	pages++;

	ppsp[pages].dwSize = sizeof(PROPSHEETPAGE);
	ppsp[pages].dwFlags = PSP_DEFAULT;
	ppsp[pages].hInstance = ghInstance;
	ppsp[pages].pszTitle = "General Options";
	ppsp[pages].pszTemplate = MAKEINTRESOURCE(IDD_GENERALOPT);
	ppsp[pages].pfnDlgProc = DlgProcGenOpts;
	pages++;

	ppsp[pages].dwSize = sizeof(PROPSHEETPAGE);
	ppsp[pages].dwFlags = PSP_DEFAULT;
	ppsp[pages].hInstance = ghInstance;
	ppsp[pages].pszTitle = "Hotkey Options";
	ppsp[pages].pszTemplate = MAKEINTRESOURCE(IDD_OPS_HOTKEY);
	ppsp[pages].pfnDlgProc = DlgProcOpsHotkey;
	pages++;

	ppsp[pages].dwSize = sizeof(PROPSHEETPAGE);
	ppsp[pages].dwFlags = PSP_DEFAULT;
	ppsp[pages].hInstance = ghInstance;
	ppsp[pages].pszTitle = "Sound Options";
	ppsp[pages].pszTemplate = MAKEINTRESOURCE(IDD_SOUND);
	ppsp[pages].pfnDlgProc = DlgProcSound;	
	pages++;

	
	ppsp[pages].dwSize = sizeof(PROPSHEETPAGE);
	ppsp[pages].dwFlags = PSP_DEFAULT;
	ppsp[pages].hInstance = ghInstance;
	ppsp[pages].pszTitle = "Transparency Options";
	ppsp[pages].pszTemplate = MAKEINTRESOURCE(IDD_OPS_TRANSPARENT);
	ppsp[pages].pfnDlgProc = DlgProcOpsTransparency;	
	pages++;

	ppsp[pages].dwSize = sizeof(PROPSHEETPAGE);
	ppsp[pages].dwFlags = PSP_DEFAULT;
	ppsp[pages].hInstance = ghInstance;
	ppsp[pages].pszTitle = "Proxy Options";
	ppsp[pages].pszTemplate = MAKEINTRESOURCE(IDD_PROXY);
	ppsp[pages].pfnDlgProc = DlgProcProxy;	
	pages++;
	

	ppsp[pages].dwSize = sizeof(PROPSHEETPAGE);
	ppsp[pages].dwFlags = PSP_DEFAULT;
	ppsp[pages].hInstance = ghInstance;
	ppsp[pages].pszTitle = "Advanced Options";
	ppsp[pages].pszTemplate = MAKEINTRESOURCE(IDD_OPS_ADVANCED);
	ppsp[pages].pfnDlgProc = DlgProcOpsAdvanced;	
	pages++;
	

	Plugin_NotifyPlugins(PM_PROPSHEET, (WPARAM)&pages, (LPARAM)ppsp);
	

	memset(&psh, 0, sizeof(psh));

	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_DEFAULT  | PSH_MODELESS | PSH_PROPTITLE | PSH_PROPSHEETPAGE;
	psh.hwndParent = hwnd;
	psh.hInstance = ghInstance;
	psh.nPages = pages;
	psh.pStartPage = 0;
	psh.pszCaption = "Miranda ICQ";	
	psh.ppsp = ppsp;

	hProp = (HWND)PropertySheet(&psh);
}

void SetStatusText(char *buff)
{
	SendMessage(rto.hwndStatus, WM_SETTEXT, 0, (LPARAM)buff);
}

void SetStatusTextEx(char*buff,int slot)
{
	static char sslot[2] [50];
	int cnt=0;
	int i=0;
	char tmp[101];

	strcpy(sslot[slot],buff);

	tmp[0]=0;

	for (i=0;i<2;i++)
	{
		if (sslot[i][0]!=0)
		{
			if(cnt>0)
			{
				strcat(tmp," | ");
			}

			strcat(tmp,sslot[i]);
			cnt++;
		}
	}
	
	
	SetStatusText(tmp);

}
