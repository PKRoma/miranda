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
//#define WINVER 0x0500
//#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.h"
#include "miranda.h"
#include "internal.h"
#include "global.h"
#include "contact.h"
#include "plugins.h"

void CbLogged(struct icq_link *link)
{
	int i;

	rto.askdisconnect = FALSE;
	opts.ICQ.online = TRUE;

	PlaySoundEvent("MICQ_Logged");

	UpdateUserStatus();

	// make sure everybody has info
	for (i = 0; i < opts.ccount; i++)
	{
		if (opts.clist[i].nick[0] == 0)
		{
			icq_SendInfoReq(plink, opts.clist[i].uin);
		}
	}
	// send me contact list for good luck
	icq_SendContactList(plink);

	TrayIcon(ghwnd, TI_UPDATE);
}

void CbDisconnected(struct icq_link *link)
{
	int i;

	opts.ICQ.online = FALSE;
	rto.net_activity = FALSE;
	opts.ICQ.status = STATUS_OFFLINE;

	PlaySoundEvent("MICQ_Disconnected");

	UpdateUserStatus();

	// hey, put everybody offline
	for (i = 0; i < opts.ccount; i++)
	{
		if (opts.clist[i].id==CT_ICQ)
			ChangeContactStatus(opts.clist[i].uin, STATUS_OFFLINE);
	}
	TrayIcon(ghwnd, TI_UPDATE);
}

void CbRecvMsg(struct icq_link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *msg)
{
	int mqid=0;	
	
	AddToHistory(uin, MakeTime(hour, minute, day, month, year), HISTORY_MSGRECV, (char *)msg);

	if (!(GetUserFlags(uin) & UF_IGNORE))
	{
		PlaySoundEvent("MICQ_RecvMsg");

		mqid=MsgQue_Add(uin,hour,minute,day,month,year,(char *)msg,NULL,INSTMSG);
		
		SetGotMsgIcon(uin,TRUE);
		
		if (MsgQue_MsgsForUIN(uin)==1)
		{//only msg, display it
			//DisplayMessageRecv(mqid);
			
		}
		else
		{//others msgs being displayed - set Dismiss to Read Next
			SetCaptionToNext(uin);
		}				
	}	
	Plugin_NotifyPlugins(PM_GOTMESSAGE,0,(WPARAM)&msgque[mqid]);

	if (opts.ICQ.status == STATUS_ONLINE && (opts.flags1 & FG_POPUP)) SendMessage(ghwnd,WM_HOTKEY,HOTKEY_SETFOCUS,0);
}

void CbRecvUrl(struct icq_link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *url, const char *desc)
{
	int mqid=0;
	char buffer[2048];	
	
	sprintf(buffer, "%s\r\n%s", url, desc);
	AddToHistory(uin, MakeTime(hour, minute, day, month, year), HISTORY_URLRECV, buffer);

	if (!(GetUserFlags(uin) & UF_IGNORE))
	{
		PlaySoundEvent("MICQ_RecvUrl");
		mqid=MsgQue_Add(uin,hour,minute,day,month,year,(char *)desc,(char *)url,URL);
		SetGotMsgIcon(uin,TRUE);

		if (MsgQue_MsgsForUIN(uin)==1)
		{//only msg, display it
			//DisplayUrlRecv(mqid);
			
		}
		else
		{//others msgs being displayed - set Dismiss to Read Next
			SetCaptionToNext(uin);
		}
	}
	//tell plugins
	Plugin_NotifyPlugins(PM_GOTURL,0,(WPARAM)&msgque[mqid]);

	if (opts.ICQ.status == STATUS_ONLINE && (opts.flags1 & FG_POPUP)) SendMessage(ghwnd,WM_HOTKEY,HOTKEY_SETFOCUS,0);
}

void CbUserOnline(struct icq_link *link, unsigned long uin, unsigned long status, unsigned long ip, unsigned short port, unsigned long real_ip, unsigned char tcp_flag)
{	
	int i=0;
	PlaySoundEvent("MICQ_UserOnline");
	//if (status > 0xFFFF) status >>= 16;
	ChangeContactStatus(uin, status);
	for (i = 0; i < opts.ccount; i++)
	{
		if (uin==opts.clist[i].uin)
		{
			opts.clistrto[i].IP=ip;
			opts.clistrto[i].REALIP=real_ip;
			break;
		}
	}
	

	SortContacts();
}

void CbUserOffline(struct icq_link *link, unsigned long uin)
{
	PlaySoundEvent("MICQ_UserOffline");
	ChangeContactStatus(uin, STATUS_OFFLINE);

	if (opts.flags1 & FG_HIDEOFFLINE)
	{//hide em
		HTREEITEM t;
		t=UinToTree(uin);

		if (t)
			TreeView_DeleteItem(rto.hwndContact, t);		

	}
	SortContacts();
}

void CbUserStatusUpdate(struct icq_link *link, unsigned long uin, unsigned long status)
{
	ChangeContactStatus(uin, status);

	SortContacts();
}

void CbInfoReply(struct icq_link *link, unsigned long uin, const char *nick, const char *first, const char *last, const char *email, char auth)
{
	INFOREPLY inforeply;
	HWNDLIST *node;

	int i;
	int ri = -1;
	// cha cha cha
	inforeply.uin = uin;
	inforeply.nick = (char *)nick;
	inforeply.first = (char *)first;
	inforeply.last = (char *)last;
	inforeply.email = (char *)email;

	node = rto.hwndlist;
	while (node)
	{
		if (node->windowtype == WT_DETAILS)
		{
			SendMessage(node->hwnd, TI_INFOREPLY, 0, (LPARAM)&inforeply);
		}
		node = node->next;
	}
	// update the info in the contact list
	for (i = 0; i < opts.ccount; i++)
	{
		if (opts.clist[i].uin == uin)
		{
			if (!*(opts.clist[i].first)) 
				strcpy(opts.clist[i].first, first);
			if (!*(opts.clist[i].last)) 
				strcpy(opts.clist[i].last, last);
			if (!*(opts.clist[i].email)) 
				strcpy(opts.clist[i].email, email);
			if (!*(opts.clist[i].nick)) 
				strcpy(opts.clist[i].nick, nick);
			SortContacts();
			ri=i;
		}
	}
	// update the info on the list view
	
	if (ri != -1 && opts.clist[ri].nick[0])
	{
		HTREEITEM hitem;
		hitem = UinToTree(uin);

		if (hitem)
		{
			TVITEM item;
			item.hItem = hitem;
			item.mask = TVIF_TEXT;
			item.pszText = _strdup(opts.clist[ri].nick);

			TreeView_SetItem(rto.hwndContact, &item);
		}
	}
}

void CbExtInfoReply(struct icq_link *link, unsigned long uin, const char *city, unsigned short country_code, char country_stat, const char *state, unsigned short age, char gender, const char *phone, const char *hp, const char *about)
{
	HWNDLIST *node;
	EXTINFOREPLY extinforeply;

	extinforeply.uin = uin;
	extinforeply.city = (char *)city;
	extinforeply.country_code = country_code;
	extinforeply.state = (char *)state;
	extinforeply.age = age;
	extinforeply.gender = gender;
	extinforeply.phone = (char *)phone;
	extinforeply.hp = (char *)hp;
	extinforeply.about = (char *)about;

	node = rto.hwndlist;
	while (node)
	{
		if (node->windowtype == WT_DETAILS)
		{
			SendMessage(node->hwnd, TI_EXTINFOREPLY, 0, (LPARAM)&extinforeply);
		}
		node = node->next;
	}
}

void CbSrvAck(struct icq_link *link, unsigned short seq)
{
	HWNDLIST *node;
	node = rto.hwndlist;

	//connectionAlive=450; // for alivecheck in timer function (wndproc)

	while (node)
	{
		if (seq == node->ack) 
		{
			SendMessage(node->hwnd, TI_SRVACK, 0, 0);
			return;
		}
		node = node->next;
	}
}

void CbRequestNotify(struct icq_link *link, unsigned long id, int type, int arg, void *data)
{
	HWNDLIST *node;
	node = rto.hwndlist;

	//connectionAlive=520; // for alivecheck in timer function (wndproc)

	while (node)
	{
		if (id == node->ack) 
		{
			SendMessage(node->hwnd, TI_SRVACK, 0, 0);
			return;
		}
		node = node->next;
	}
}

void CbRecvAdded(struct icq_link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *nick, const char *first, const char *last, const char *email)
{
	HWNDLIST *node;
	DLG_DATA dlg_data;

	PlaySoundEvent("MICQ_Added");
	//AddToHistory(uin, MakeTime(hour, minute, day, month, year), HISTORY_ADDED, NULL);

	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_ADDED;
	node->next = rto.hwndlist;
	node->uin = uin;
	rto.hwndlist = node;

	dlg_data.nick = (char *)nick;
	dlg_data.uin = uin;
	dlg_data.hour = hour;
	dlg_data.minute = minute;
	dlg_data.day = day;
	dlg_data.month = month;
	dlg_data.year = year;

	node->hwnd = CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_ADDED), NULL, DlgProcAdded, (LPARAM)&dlg_data);
	//ShowWindow(node->hwnd, (opts.flags1 & FG_POPUP) ? SW_SHOW : SW_SHOWMINIMIZED);
	ShowWindow(node->hwnd, SW_SHOW);
}

void CbRecvAuthReq(struct icq_link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *nick, const char *first, const char *last, const char *email, const char *reason)
{
	HWNDLIST *node;
	DLG_DATA dlg_data;

	PlaySoundEvent("MICQ_AuthReq");

	AddToHistory(uin, MakeTime(hour, minute, day, month, year), HISTORY_AUTHREQ, (char *)reason);
	
	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_AUTHREQ;
	node->next = rto.hwndlist;
	node->uin = uin;
	rto.hwndlist = node;

	dlg_data.msg = reason;
	dlg_data.nick = nick;
	dlg_data.uin = uin;
	dlg_data.hour = hour;
	dlg_data.minute = minute;
	dlg_data.day = day;
	dlg_data.month = month;
	dlg_data.year = year;

	node->hwnd = CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_AUTHREQ), NULL, DlgProcAuthReq, (LPARAM)&dlg_data);

	ShowWindow(node->hwnd, SW_SHOW);
}

void CbUserFound(struct icq_link *link, unsigned long uin, const char *nick, const char *first, const char *last, const char *email, char auth)
{
	HWNDLIST *node;
	DLG_DATA dlg_data;

	dlg_data.uin = uin;
	dlg_data.nick = (char *)nick;
	dlg_data.first = (char *)first;
	dlg_data.last = (char *)last;

	node = rto.hwndlist;
	while (node)
	{
		if (node->windowtype == WT_RESULT)
		{
			SendMessage(node->hwnd, TI_USERFOUND, 0, (LPARAM)&dlg_data);
		}
		node = node->next;
	}	
}

void CbSearchDone(struct icq_link *link)
{
	HWNDLIST *node;

	node = rto.hwndlist;
	while (node)
	{
		if (node->windowtype == WT_RESULT)
		{
			SendMessage(node->hwnd, WM_SETTEXT, 0,  (LPARAM)"Search done.");
		}
		node = node->next;
	}
}

void CbWrongPassword(struct icq_link *link)
{
	PlaySoundEvent("MICQ_Error");
	MessageBox(ghwnd, "Your password is incorrect", MIRANDANAME, MB_OK);
}

void CbInvalidUIN(struct icq_link *link)
{
	PlaySoundEvent("MICQ_Error");
	MessageBox(ghwnd, "Miranda made a request with an invalid UIN", MIRANDANAME, MB_OK);
}

void CbLog(struct icq_link *link, time_t time, unsigned char level, const char *str)
{
	#ifdef _DEBUG
		OutputDebugString(str);
	#endif
	//send the msg to the plugins
	Plugin_NotifyPlugins(PM_ICQDEBUGMSG,(WPARAM)strlen(str),(LPARAM)str);
}

void CbRecvFileReq(struct icq_link *link, unsigned long uin,
       unsigned char hour, unsigned char minute, unsigned char day,
       unsigned char month, unsigned short year, const char *descr,
       const char *filename, unsigned long filesize, unsigned long seq)
{

	HWNDLIST *node;
	DLG_DATA dlg_data;
	
	
	node = malloc(sizeof(HWNDNODE));

	node->windowtype = WT_RECVFILE;
	node->next = rto.hwndlist;
	node->uin = uin;
	rto.hwndlist = node;

	dlg_data.uin = uin;
	dlg_data.hour = hour;
	dlg_data.minute = minute;
	dlg_data.day = day;
	dlg_data.month = month;
	dlg_data.year = year;
	dlg_data.msg = descr;
	dlg_data.seq = seq;
	dlg_data.url = filename;
	
	ShowWindow(CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_FILERECV), NULL, DlgProcRecvFile,(LPARAM)&dlg_data), SW_SHOW);
	
	PostMessage(node->hwnd, TI_POP, 0, 0);

}

void CbMetaUserFound(struct icq_link *link, unsigned short seq2, unsigned long uin, const char *nick, const char *first, const char *last, const char *email, char auth)
{
}

void CbMetaUserInfo(struct icq_link *link, unsigned short seq2, const char *nick, const char *first, const char *last, const char *pri_eml, const char *sec_eml, const char *old_eml, const char *city, const char *state, const char *phone, const char *fax, const char *street, const char *cellular, unsigned long zip, unsigned short country, unsigned char timezone, unsigned char auth, unsigned char webaware, unsigned char hideip)
{
}

void CbMetaUserWork(struct icq_link *link, unsigned short seq2, const char *wcity, const char *wstate, const char *wphone, const char *wfax, const char *waddress, unsigned long wzip, unsigned short wcountry, const char *company, const char *department, const char *job, unsigned short occupation, const char *whomepage)
{
}

void CbMetaUserMore(struct icq_link *link, unsigned short seq2, unsigned short age, unsigned char gender, const char *homepage, unsigned char byear, unsigned char bmonth, unsigned char bday, unsigned char lang1, unsigned char lang2, unsigned char lang3)
{
}

void CbMetaUserAbout(struct icq_link *link, unsigned short seq2, const char *about)
{
}

void CbMetaUserInterests(struct icq_link *link, unsigned short seq2, unsigned char num, unsigned short icat1, const char *int1, unsigned short icat2, const char *int2, unsigned short icat3, const char *int3, unsigned short icat4, const char *int4)
{
}

void CbMetaUserAffiliations(struct icq_link *link, unsigned short seq2, unsigned char anum, unsigned short acat1, const char *aff1, unsigned short acat2, const char *aff2, unsigned short acat3, const char *aff3, unsigned short acat4, const char *aff4, unsigned char bnum, unsigned short bcat1, const char *back1, unsigned short bcat2, const char *back2, unsigned short bcat3, const char *back3, unsigned short bcat4, const char *back4)
{
}

void CbMetaUserHomePageCategory(struct icq_link *link, unsigned short seq2, unsigned char num, unsigned short hcat1, const char *htext1)
{
}

void CbNewUIN(struct icq_link *link, unsigned long uin)
{
	HWNDLIST *node;

	node = rto.hwndlist;
	while (node)
	{
		if (node->windowtype == WT_NEWICQ)
		{
			SetDlgItemInt(node->hwnd, IDC_NUMBER, uin, FALSE);
			opts.ICQ.uin = uin;
			GetWindowText(node->hwnd, opts.ICQ.password, sizeof(opts.ICQ.password));
		}
		node = node->next;
	}
}

void CbSetTimeout(struct icq_link *link, long interval)
{
}