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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "miranda.h"
#include "internal.h"
#include "contact.h"
#include "global.h"
#include "resource.h"
#include "msn_global.h"

BOOL CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char buf[50];
	switch (msg)
	{
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE)));
			
			sprintf(buf,"Version %s",MIRANDAVERSTR);
			SetDlgItemText(hwndDlg, IDC_VER, buf);

			return TRUE;			
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
					EndDialog(hwndDlg, 0);
					return TRUE;
			}
	}
	return 0;
}

BOOL CALLBACK DlgProcMsgRecv(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem;
	DLG_DATA *dlgdata;
	
	char buffer[128];
	char *buf;
	
	WINDOWPLACEMENT wp;//wmsize/wmmove

	unsigned long uin;

	switch (msg)
	{
		case TI_POP:
			SetForegroundWindow(hwndDlg);
			break;
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_RECVMSG)));
			
			dlgdata = (DLG_DATA *)lParam;
			buf = LookupContactNick(dlgdata->uin);
			
			if (buf)
				sprintf(buffer,"Message from %s",buf);
			else
				sprintf(buffer,"Message from %d",dlgdata->uin);

			SetWindowText(hwndDlg,buffer);
			

			
			//SetProp(hwndDlg,"MQID",dlgdata->msgid);

			hwndItem = GetDlgItem(hwndDlg, IDC_MSG);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)dlgdata->msg);
			hwndItem = GetDlgItem(hwndDlg, IDC_FROM);
			
			if (buf)
			{
				sprintf(buffer, "%s", buf);
			}
			else
			{
				sprintf(buffer, "%d", dlgdata->uin);
			}
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			hwndItem = GetDlgItem(hwndDlg, IDC_DATE);
			sprintf(buffer, "%d:%2.2d %d/%2.2d/%4.4d", dlgdata->hour, dlgdata->minute, dlgdata->day, dlgdata->month, dlgdata->year);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
			sprintf(buffer, "%d", dlgdata->uin);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			
			if (!InContactList(dlgdata->uin)) EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), TRUE);
			
			if (MsgQue_MsgsForUIN(dlgdata->uin)>1 )
			{//more messages in the que, make Close to NExt
				SetDlgItemText(hwndDlg, IDCANCEL, "&Next");
			}

			if (opts.pos_recvmsg.xpos!=-1)
				MoveWindow(hwndDlg,opts.pos_recvmsg.xpos,opts.pos_recvmsg.ypos,opts.pos_recvmsg.width,opts.pos_recvmsg.height,TRUE);

			return TRUE;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_SIZE:
		case WM_MOVE:
			wp.length = sizeof(wp);
			GetWindowPlacement(hwndDlg, &wp);
			opts.pos_recvmsg.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			opts.pos_recvmsg.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
			opts.pos_recvmsg.xpos = wp.rcNormalPosition.left;
			opts.pos_recvmsg.ypos = wp.rcNormalPosition.top;
			break;
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDCANCEL:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);

					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);

					ShowNextMsg(MsgQue_FindFirst(uin));
					return TRUE;
				case IDOK:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					buf = LookupContactNick(uin);

					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					
					ShowNextMsg(MsgQue_FindFirst(uin));
					SendIcqMessage(uin, buf);

					if (rto.hwndlist) PostMessage(rto.hwndlist->hwnd, TI_POP, 0, 0);

					return TRUE;
				case IDC_ADD:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					AddToContactList(uin);
					EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
					return TRUE;
				case IDC_DETAILS:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					DisplayUINDetails(uin);
					return TRUE;
				case IDC_HISTORY:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 1024, (LPARAM)buffer);
					uin = atol(buffer);
					DisplayHistory(hwndDlg, uin);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK DlgProcUrlRecv(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem;
	DLG_DATA *dlgdata;
	static int mqid;

	WINDOWPLACEMENT wp;//wmsize/wmmove

	char buffer[1024];
	char *buf;

	unsigned long uin;

	switch (msg)
	{
		case TI_POP:
			SetForegroundWindow(hwndDlg);
			break;
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_RECVURL)));

			dlgdata = (DLG_DATA *)lParam;

			buf = LookupContactNick(dlgdata->uin);
			
			if (buf)
				sprintf(buffer,"URL from %s",buf);
			else
				sprintf(buffer,"URL from %d",dlgdata->uin);

			SetWindowText(hwndDlg,buffer);

			mqid=dlgdata->msgid;
			hwndItem = GetDlgItem(hwndDlg, IDC_MSG);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)dlgdata->msg);
			hwndItem = GetDlgItem(hwndDlg, IDC_URL);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)dlgdata->url);
			hwndItem = GetDlgItem(hwndDlg, IDC_FROM);
			

			if (buf)
			{
				sprintf(buffer, "%s", buf);
			}
			else
			{
				sprintf(buffer, "%d", dlgdata->uin);
			}
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			hwndItem = GetDlgItem(hwndDlg, IDC_DATE);
			sprintf(buffer, "%d:%2.2d %d/%2.2d/%4.4d", dlgdata->hour, dlgdata->minute, dlgdata->day, dlgdata->month, dlgdata->year);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
			sprintf(buffer, "%d", dlgdata->uin);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);

			if (MsgQue_MsgsForUIN(dlgdata->uin)>1 )
			{//more messages in the que, make Close to NExt
				SetDlgItemText(hwndDlg, IDCANCEL, "&Next");
			}

			if (opts.pos_recvurl.xpos!=-1)
				MoveWindow(hwndDlg,opts.pos_recvurl.xpos,opts.pos_recvurl.ypos,opts.pos_recvurl.width,opts.pos_recvurl.height,TRUE);

			return TRUE;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_SIZE:
		case WM_MOVE:
			wp.length = sizeof(wp);
			GetWindowPlacement(hwndDlg, &wp);
			opts.pos_recvurl.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			opts.pos_recvurl.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
			opts.pos_recvurl.xpos = wp.rcNormalPosition.left;
			opts.pos_recvurl.ypos = wp.rcNormalPosition.top;
			break;
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					ShowNextMsg(mqid);
					return TRUE;
				case IDOK:
					GetDlgItemText(hwndDlg, IDC_URL, buffer, sizeof(buffer));
					ShellExecute(hwndDlg, "open", buffer, NULL, NULL, SW_SHOW);
					return TRUE;
				case IDC_REPLY:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					buf = LookupContactNick(uin);

					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					ShowNextMsg(mqid);
					SendIcqMessage(uin, buf);
					if (rto.hwndlist) PostMessage(rto.hwndlist->hwnd, TI_POP, 0, 0);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK DlgProcMsgSend(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem;
	DLG_DATA *dlgdata;
	
	WINDOWPLACEMENT wp;//wmsize/wmmove

	unsigned long uin;
	char buffer[1024];

	switch (msg)
	{
		case TI_POP:
			SetForegroundWindow(hwndDlg);
			break;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
			SendDlgItemMessage(hwndDlg, IDC_BODY, EM_LIMITTEXT, 450, 0);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_SENDMSG)));
			dlgdata = (DLG_DATA *)lParam;
			hwndItem = GetDlgItem(hwndDlg, IDC_NAME);			

			if (dlgdata->nick)
				sprintf(buffer,"Send Message to %s",dlgdata->nick);
			else
				sprintf(buffer,"Send Message to %d",dlgdata->uin);

			SetWindowText(hwndDlg,buffer);


			if (dlgdata->nick)
			{
				sprintf(buffer, "%s [%d]", dlgdata->nick,dlgdata->uin);
			}
			else
			{
				sprintf(buffer, "%d", dlgdata->uin);
			}
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
			sprintf(buffer, "%d", dlgdata->uin);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);

			if (opts.pos_sendmsg.xpos!=-1)
				MoveWindow(hwndDlg,opts.pos_sendmsg.xpos,opts.pos_sendmsg.ypos,opts.pos_sendmsg.width,opts.pos_sendmsg.height,TRUE);

			return TRUE;
		case WM_TIMER:
			if ((int)wParam==0)
			{//ICQ sendmsg timed out
				/*long cnt;
				cnt=GetWindowLong(hwndDlg,GWL_USERDATA);
				cnt++;
				SetWindowLong(hwndDlg, GWL_USERDATA, cnt);*/
				KillTimer(hwndDlg,0);
				
				/*if (cnt>3)
				{//3 times r up, user will have to retry them selfs
					SetWindowLong(hwndDlg, GWL_USERDATA, 0);
					break;
				}*/
				
				goto sendmsg;
				
			}
			break;
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
				{
					unsigned long *srvack;
sendmsg:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
					uin = atol(buffer);
					
					hwndItem = GetDlgItem(hwndDlg, IDC_BODY);
					SendMessage(hwndItem, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
					
					if (!IsMSN(uin))
					{//ICQ SEND MSG
						srvack = GetAckPtr(hwndDlg);
						*srvack = icq_SendMessage(plink, uin, buffer, opts.ICQ.sendmethod);
						
						//minimize the wnd
						ShowWindow(hwndDlg, SW_SHOWMINIMIZED);
						
						//create a timeout timer
						SetTimer(hwndDlg,0,TIMEOUT_MSGSEND,NULL);
						
						//(added to history on server ack)
					}
					else
					{//MSN SENDMSG
						char uhandle[MSN_UHANDLE_LEN];

						MSN_HandleFromContact(uin,uhandle);
						if (!MSN_SendUserMessage(uhandle,buffer))
						{//failed to send b/c there is no session
							//MessageBox(ghwnd,"No Session to transport message.",MIRANDANAME,MB_OK);
							
							int i;
							for (i=0;i<opts.ccount;i++)
							{
								if (opts.clist[i].id==CT_MSN && opts.clist[i].uin==uin)
								{
									opts.clistrto[i].MSN_PENDINGSEND=TRUE;
									break;
								}
							}
							
							
							MSN_RequestSBSession();
							ShowWindow(hwndDlg, SW_SHOWMINIMIZED);

						}
						else
						{//(MSN) sent ok
							AddToHistory(uin, time(NULL), HISTORY_MSGSEND, buffer);

							DeleteWindowFromList(hwndDlg);
							DestroyWindow(hwndDlg);
						}
					}

					


					return TRUE;
				}
				case IDCANCEL:
					{
						int i;
						//check if we need to cancel a pending MSN send
						hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
						SendMessage(hwndItem, WM_GETTEXT, 1024, (LPARAM)buffer);
						uin = atol(buffer);
						if (IsMSN(uin))
						{
							for (i=0;i<opts.ccount;i++)
							{
								if (opts.clist[i].id==CT_MSN && opts.clist[i].uin==uin)
								{
									opts.clistrto[i].MSN_PENDINGSEND=FALSE;
									break;
								}
							}
						}

						DeleteWindowFromList(hwndDlg);
						DestroyWindow(hwndDlg);
					}
					return TRUE;
				case IDC_HISTORY:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
					uin = atol(buffer);
					DisplayHistory(hwndDlg, uin);
					return TRUE;
			}
			break;
		case WM_SIZE:
		case WM_MOVE:
			
			wp.length = sizeof(wp);
			GetWindowPlacement(hwndDlg, &wp);
			opts.pos_sendmsg.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			opts.pos_sendmsg.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
			opts.pos_sendmsg.xpos = wp.rcNormalPosition.left;
			opts.pos_sendmsg.ypos = wp.rcNormalPosition.top;
			break;
		case TI_SRVACK:
			KillTimer(hwndDlg,0);

			hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
			SendMessage(hwndItem, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
			uin = atol(buffer);

			hwndItem = GetDlgItem(hwndDlg, IDC_BODY);
			SendMessage(hwndItem, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

			AddToHistory(uin, time(NULL), HISTORY_MSGSEND, buffer);

			DeleteWindowFromList(hwndDlg);
			DestroyWindow(hwndDlg);
			break;

		case TI_MSN_TRYAGAIN:
			goto sendmsg;
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcUrlSend(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem;
	DLG_DATA *dlgdata;

	WINDOWPLACEMENT wp;//wmsize/wmmove

	unsigned long uin;
	char msgbuf[1028];
	char urlbuf[256];

	switch (msg)
	{
		case TI_POP:
			SetForegroundWindow(hwndDlg);
			break;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{
			char buffer[2048];
			char *token;
			GetOpenURLs(buffer, sizeof(buffer));
			token = strtok(buffer, "\n");
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_SENDURL)));
			while (token)
			{
				SendDlgItemMessage(hwndDlg, IDC_URLS, CB_ADDSTRING, 0, (LPARAM)token);
				token = strtok(NULL, "\n");
			}
			SendDlgItemMessage(hwndDlg, IDC_URLS, CB_SETCURSEL, 0, 0);

			dlgdata = (DLG_DATA *)lParam;
			
			if (dlgdata->nick)
				sprintf(buffer,"Send URL to %s",dlgdata->nick);
			else
				sprintf(buffer,"Send URL to %d",dlgdata->uin);

			SetWindowText(hwndDlg,buffer);


			hwndItem = GetDlgItem(hwndDlg, IDC_NAME);			
			if (dlgdata->nick)
			{
				sprintf(buffer, "%s [%d]", dlgdata->nick,dlgdata->uin);
			}
			else
			{
				sprintf(buffer, "%d", dlgdata->uin);
			}
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
			sprintf(buffer, "%d", dlgdata->uin);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);

			if (opts.pos_sendurl.xpos!=-1)
				MoveWindow(hwndDlg,opts.pos_sendurl.xpos,opts.pos_sendurl.ypos,opts.pos_sendurl.width,opts.pos_sendurl.height,TRUE);

			return TRUE;
		}
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
				{
					unsigned long *srvack;
					char buffer[2048];

					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)urlbuf);
					uin = atol(urlbuf);
					
					hwndItem = GetDlgItem(hwndDlg, IDC_BODY);
					SendMessage(hwndItem, WM_GETTEXT, 1024, (LPARAM)msgbuf);

					hwndItem = GetDlgItem(hwndDlg, IDC_URLS);
					SendMessage(hwndItem, WM_GETTEXT, sizeof(urlbuf), (LPARAM)urlbuf);

					srvack = GetAckPtr(hwndDlg);
					*srvack = icq_SendURL(plink, uin, urlbuf, msgbuf, opts.ICQ.sendmethod);

					sprintf(buffer, "%s\r\n%s", urlbuf, msgbuf);
					AddToHistory(uin, time(NULL), HISTORY_URLSEND, buffer);
					return TRUE;
				}
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
			}
			break;
		case WM_SIZE:
		case WM_MOVE:
			wp.length = sizeof(wp);
			GetWindowPlacement(hwndDlg, &wp);
			opts.pos_sendurl.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			opts.pos_sendurl.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
			opts.pos_sendurl.xpos = wp.rcNormalPosition.left;
			opts.pos_sendurl.ypos = wp.rcNormalPosition.top;
			break;
		case TI_SRVACK:
			DeleteWindowFromList(hwndDlg);
			DestroyWindow(hwndDlg);
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcAddContact1(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem; 

	unsigned int uin;
	char buffer[128];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE)));
			return TRUE;
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);

					AddToContactList(uin);

					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK DlgProcAddContact2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem; 

	char buffer[128];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE)));
			return TRUE;
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
					hwndItem = GetDlgItem(hwndDlg, IDC_EMAIL);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);

					icq_SendSearchReq(plink, buffer, "", "", "");
					
					OpenResultWindow();

					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK DlgProcAddContact3(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char nick[128];
	char first[128];
	char last[128];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE)));
			return TRUE;
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
					GetDlgItemText(hwndDlg, IDC_NICK, nick, 128);
					GetDlgItemText(hwndDlg, IDC_FIRST, first, 128);
					GetDlgItemText(hwndDlg, IDC_LAST, last, 128);

					icq_SendSearchReq(plink, "", nick, first, last);
					
					OpenResultWindow();

					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK DlgProcPassword(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hWndChild;

	unsigned long uin;
	char buffer[32];

	switch (msg)
	{
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE)));
			return TRUE;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;			
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDCANCEL:
					EndDialog(hwndDlg, 1);
					return TRUE;
				case IDOK:
					hWndChild = GetDlgItem(hwndDlg, IDC_ICQNUM);
					SendMessage(hWndChild, WM_GETTEXT, 32, (LPARAM)buffer);
					uin = atol(buffer);
					opts.ICQ.uin = uin;
					hWndChild = GetDlgItem(hwndDlg, IDC_PASSWORD);
					SendMessage(hWndChild, WM_GETTEXT, 32, (LPARAM)buffer);
					strcpy(opts.ICQ.password, buffer);
					SaveOptions(&opts, myprofile);

					
					icq_Disconnect(plink);
					if (plink->icq_Uin!=0) //if its zero, dont bother (jsut get acces violation)
						icq_Done(plink);
						//icq_ICQLINKDelete(plink);

					icq_Init(plink, opts.ICQ.uin, opts.ICQ.password, MIRANDANAME, TRUE);
					//plink = icq_ICQLINKNew(opts.ICQ.uin, opts.ICQ.password, "nick", TRUE);

					EndDialog(hwndDlg, 0);
					return TRUE;
			}
	}
	return 0;
}

BOOL CALLBACK DlgProcGPL(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	FILE *fp;

	char *buf;

	switch (msg)
	{
		case WM_INITDIALOG:			
			{
				char fn[MAX_PATH];
				SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE)));

				GetModuleFileName(ghInstance, fn, sizeof(fn));
				strcpy(strrchr(fn, '\\'), "\\gpl.txt");

				fp = fopen(fn, "rb");
				if (fp)
				{
					buf = calloc(32 * 1024, 1);

					fread(buf, 1, 32 * 1024, fp);

					SendDlgItemMessage(hwndDlg, IDC_TEXT, WM_SETTEXT, 0, (LPARAM)buf);
					free(buf);
					fclose(fp);
				}
			}
			return TRUE;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;			
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDCANCEL:
					EndDialog(hwndDlg, 0);
					return TRUE;
				case IDOK:
					EndDialog(hwndDlg, 3);
					return TRUE;
			}
	}
	return 0;
}

BOOL CALLBACK DlgProcSound(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char buf[MAX_PATH];

	switch (msg)
	{
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE)));

			if (opts.playsounds) 
				SendDlgItemMessage(hwndDlg, IDC_USESOUND, BM_SETCHECK, BST_CHECKED, 0);
			
			if (LoadSound("MICQ_Loged", buf))
				SetDlgItemText(hwndDlg, IDC_SOUND1, buf);			
			if (LoadSound("MICQ_Disconnected", buf))
				SetDlgItemText(hwndDlg, IDC_SOUND2, buf);
			if (LoadSound("MICQ_RecvMsg", buf))
				SetDlgItemText(hwndDlg, IDC_SOUND3, buf);
			if (LoadSound("MICQ_RecvUrl", buf))
				SetDlgItemText(hwndDlg, IDC_SOUND4, buf);
			if (LoadSound("MICQ_UserOnline", buf))
				SetDlgItemText(hwndDlg, IDC_SOUND5, buf);
			if (LoadSound("MICQ_UserOffline", buf))
				SetDlgItemText(hwndDlg, IDC_SOUND6, buf);
			if (LoadSound("MICQ_Added", buf))
				SetDlgItemText(hwndDlg, IDC_SOUND7, buf);
			if (LoadSound("MICQ_AuthReq", buf))
				SetDlgItemText(hwndDlg, IDC_SOUND8, buf);
			if (LoadSound("MICQ_Error", buf))
				SetDlgItemText(hwndDlg, IDC_SOUND9, buf);

			return TRUE;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
				case PSN_APPLY:
					opts.playsounds = (BST_CHECKED == SendDlgItemMessage(hwndDlg, IDC_USESOUND, BM_GETCHECK, 0, 0)) ? 1 : 0;
					GetDlgItemText(hwndDlg, IDC_SOUND1, buf, MAX_PATH);
					SaveSound("MICQ_Loged", buf);
					GetDlgItemText(hwndDlg, IDC_SOUND2, buf, MAX_PATH);
					SaveSound("MICQ_Disconnected", buf);
					GetDlgItemText(hwndDlg, IDC_SOUND3, buf, MAX_PATH);
					SaveSound("MICQ_RecvMsg", buf);
					GetDlgItemText(hwndDlg, IDC_SOUND4, buf, MAX_PATH);
					SaveSound("MICQ_RecvUrl", buf);
					GetDlgItemText(hwndDlg, IDC_SOUND5, buf, MAX_PATH);
					SaveSound("MICQ_UserOnline", buf);
					GetDlgItemText(hwndDlg, IDC_SOUND6, buf, MAX_PATH);
					SaveSound("MICQ_UserOffline", buf);
					GetDlgItemText(hwndDlg, IDC_SOUND7, buf, MAX_PATH);
					SaveSound("MICQ_Added", buf);
					GetDlgItemText(hwndDlg, IDC_SOUND8, buf, MAX_PATH);
					SaveSound("MICQ_AuthReq", buf);
					GetDlgItemText(hwndDlg, IDC_SOUND9, buf, MAX_PATH);
					SaveSound("MICQ_Error", buf);
					SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
					return TRUE;
			}
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 1, 0);
			switch (LOWORD(wParam))
			{
				case IDC_B1:
					BrowseForWave(GetDlgItem(hwndDlg, IDC_SOUND1));
					break;
				case IDC_B2:
					BrowseForWave(GetDlgItem(hwndDlg, IDC_SOUND2));
					break;
				case IDC_B3:
					BrowseForWave(GetDlgItem(hwndDlg, IDC_SOUND3));
					break;
				case IDC_B4:
					BrowseForWave(GetDlgItem(hwndDlg, IDC_SOUND4));
					break;
				case IDC_B5:
					BrowseForWave(GetDlgItem(hwndDlg, IDC_SOUND5));
					break;
				case IDC_B6:
					BrowseForWave(GetDlgItem(hwndDlg, IDC_SOUND6));
					break;
				case IDC_B7:
					BrowseForWave(GetDlgItem(hwndDlg, IDC_SOUND7));
					break;
				case IDC_B8:
					BrowseForWave(GetDlgItem(hwndDlg, IDC_SOUND8));
					break;
				case IDC_B9:
					BrowseForWave(GetDlgItem(hwndDlg, IDC_SOUND9));
					break;
			}
	}
	return 0;
}

BOOL CALLBACK DlgProcAdded(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem;
	DLG_DATA *dlgdata;

	unsigned long uin;
	char buffer[128];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE)));
			dlgdata = (DLG_DATA *)lParam;
			hwndItem = GetDlgItem(hwndDlg, IDC_NAME);			
			if (dlgdata->nick)
			{
				sprintf(buffer, "%s", dlgdata->nick);
			}
			else
			{
				sprintf(buffer, "%d", dlgdata->uin);
			}
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
			sprintf(buffer, "%d", dlgdata->uin);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			if (InContactList(dlgdata->uin)) EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
			return TRUE;
		}
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					AddToContactList(uin);
					
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);

					return TRUE;
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcAuthReq(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem;
	DLG_DATA *dlgdata;

	unsigned long uin;
	char buffer[128];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_MIRANDA)));
			dlgdata = (DLG_DATA *)lParam;
			if (dlgdata->nick)
			{
				sprintf(buffer, "%s", dlgdata->nick);
			}
			else
			{
				sprintf(buffer, "%d", dlgdata->uin);
			}
			hwndItem = GetDlgItem(hwndDlg, IDC_NAME);			
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
			sprintf(buffer, "%d", dlgdata->uin);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);
			hwndItem = GetDlgItem(hwndDlg, IDC_REASON);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)dlgdata->msg);

			if (InContactList(dlgdata->uin)) EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
			return TRUE;
		}
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDC_ADD:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					AddToContactList(uin);
					EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
					return TRUE;
				case IDC_DETAILS:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					DisplayUINDetails(uin);
					return TRUE;
				case IDOK:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					icq_SendAuthMsg(plink, uin);
					
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);

					return TRUE;
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcResult(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{
			HWND hwnd;
			LVCOLUMN		col;

			hwnd = GetDlgItem(hwndDlg, IDC_CONTACTS);

			col.mask = LVCF_TEXT | LVCF_WIDTH;
			col.pszText = "UIN";
			col.cx = 100;
			ListView_InsertColumn(hwnd, 0, &col);

			col.mask = LVCF_TEXT | LVCF_WIDTH;
			col.pszText = "Nickname";
			col.cx = 100;
			ListView_InsertColumn(hwnd, 1, &col);

			col.mask = LVCF_TEXT | LVCF_WIDTH;
			col.pszText = "First Name";
			col.cx = 100;
			ListView_InsertColumn(hwnd, 2, &col);

			col.mask = LVCF_TEXT | LVCF_WIDTH;
			col.pszText = "Last Name";
			col.cx = 100;
			ListView_InsertColumn(hwnd, 3, &col);
			return TRUE;
		}
		case TI_USERFOUND:
		{
			char buffer[128];
			HWND hwnd;
			int count;

			LV_ITEM item;
			DLG_DATA *dlg_data = (DLG_DATA *)lParam;

			hwnd = GetDlgItem(hwndDlg, IDC_CONTACTS);				
			count = ListView_GetItemCount(hwnd);

			ZeroMemory(&item, sizeof(item));
			item.mask = LVIF_TEXT | LVIF_PARAM;
			item.iSubItem = 0;
			item.lParam = dlg_data->uin;
			sprintf(buffer, "%d", dlg_data->uin);
			item.pszText = buffer;
			count = ListView_InsertItem(hwnd, &item);			

			if (dlg_data->nick)
			{
				ListView_SetItemText(hwnd, count, 1, (char *)dlg_data->nick);
			}
			if (dlg_data->first)
			{
				ListView_SetItemText(hwnd, count, 2, (char *)dlg_data->first);
			}
			if (dlg_data->last)
			{
				ListView_SetItemText(hwnd, count, 3, (char *)dlg_data->last);
			}
			return TRUE;
		}
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
				{
					int idx;
					HWND hwnd;
					LVITEM item;
					hwnd = GetDlgItem(hwndDlg, IDC_CONTACTS);

					idx = ListView_GetNextItem(hwnd, -1, LVNI_ALL | LVNI_SELECTED);
					if (idx != -1)
					{
						item.mask = LVIF_PARAM;
						item.iItem = idx;

						ListView_GetItem(hwnd, &item);
						AddToContactList(item.lParam);
					}					
					return TRUE;
				}
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcDetails(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char buffer[128];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{
			int i=0;
			char sip[16];
			char srealip[16];
			char smsg[50];
			SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			
			//IP
			for (i = 0; i < opts.ccount; i++)
			{
				if (lParam == (long)opts.clist[i].uin)
				{
					if (opts.clistrto[i].IP == 0 || opts.clistrto[i].REALIP == 0)
					{
						sprintf(smsg,"<NA>");
					}
					else
					{
						IPtoString(opts.clistrto[i].IP,sip);
						IPtoString(opts.clistrto[i].REALIP,srealip);
						if (_stricmp(srealip,sip)!=0)
							sprintf(smsg,"%s [Real IP:%s]",sip,srealip);
						else
							sprintf(smsg,"%s",sip);

					}
					SetDlgItemText(hwndDlg,IDC_IP,smsg);
					break;
				}
			}

			sprintf(smsg,"%d",lParam);
			SetDlgItemText(hwndDlg, IDC_UIN, smsg);

			return TRUE;
		}
		case TI_INFOREPLY:
		{
			INFOREPLY *inforeply = (INFOREPLY *)lParam;
			
			if (inforeply->uin == (unsigned int)GetWindowLong(hwndDlg, GWL_USERDATA))
			{
				sprintf(buffer, "User Details (%d)", inforeply->uin);
				SetWindowText(hwndDlg, buffer);
				SetDlgItemText(hwndDlg, IDC_FIRST, inforeply->first);
				SetDlgItemText(hwndDlg, IDC_LAST, inforeply->last);
				SetDlgItemText(hwndDlg, IDC_NICK, inforeply->nick);
				SetDlgItemText(hwndDlg, IDC_EMAIL, inforeply->email);
			}
			break;
		}
		case TI_EXTINFOREPLY:
		{
			EXTINFOREPLY *extinforeply = (EXTINFOREPLY *)lParam;
			if (extinforeply->uin == (unsigned int)GetWindowLong(hwndDlg, GWL_USERDATA))
			{
				SetDlgItemText(hwndDlg, IDC_HP, extinforeply->hp);
				SetDlgItemText(hwndDlg, IDC_ABOUT, extinforeply->about);
				SetDlgItemText(hwndDlg, IDC_CITY, extinforeply->city);
				SetDlgItemText(hwndDlg, IDC_PHONE, extinforeply->phone);
				SetDlgItemText(hwndDlg, IDC_STATE, extinforeply->state);
				SetDlgItemText(hwndDlg, IDC_COUNTRY, icq_GetCountryName(extinforeply->country_code));

				if (extinforeply->gender == 2) SetDlgItemText(hwndDlg, IDC_GENDER, "M");
				if (extinforeply->gender == 1) SetDlgItemText(hwndDlg, IDC_GENDER, "F");

				sprintf(buffer, "%d", extinforeply->age);
				SetDlgItemText(hwndDlg, IDC_AGE, buffer);
			}
			break;
		}
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDC_GO:
					GetDlgItemText(hwndDlg, IDC_HP, buffer, 128);
					if (*buffer)
					{
						if (strncmp(buffer, "http", 4) && strncmp(buffer, "HTTP", 4))
						{
							memmove(buffer + 7, buffer, strlen(buffer));
							strncpy(buffer, "http://", 7);
						}
						ShellExecute(hwndDlg, "open", buffer, NULL, NULL, SW_SHOW);
					}
					return TRUE;
				case IDC_GO2:
					strcpy(buffer, "mailto:");
					GetDlgItemText(hwndDlg, IDC_EMAIL, buffer + strlen(buffer), 128 - strlen(buffer));
					if (*(buffer+7)) ShellExecute(hwndDlg, "open", buffer, NULL, NULL, SW_SHOW);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcProxy(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{
			SendDlgItemMessage(hwndDlg, IDC_USE,  BM_SETCHECK, opts.ICQ.proxyuse  ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, IDC_AUTH, BM_SETCHECK, opts.ICQ.proxyauth ? BST_CHECKED : BST_UNCHECKED, 0);

			SetDlgItemText(hwndDlg, IDC_HOST, opts.ICQ.proxyhost);
			SetDlgItemInt(hwndDlg, IDC_PORT, opts.ICQ.proxyport, FALSE);
			SetDlgItemText(hwndDlg, IDC_NAME, opts.ICQ.proxyname);
			SetDlgItemText(hwndDlg, IDC_PASS, opts.ICQ.proxypass);

			return TRUE;
		}
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				case PSN_APPLY:
					if (SendDlgItemMessage(hwndDlg, IDC_AUTH, BM_GETCHECK, 0, 0)==BST_CHECKED)
						opts.ICQ.proxyauth=1;
					else
						opts.ICQ.proxyauth=0;
					

					GetDlgItemText(hwndDlg, IDC_HOST, opts.ICQ.proxyhost, sizeof(opts.ICQ.hostname));
					GetDlgItemText(hwndDlg, IDC_NAME, opts.ICQ.proxyname, sizeof(opts.ICQ.proxyname));
					GetDlgItemText(hwndDlg, IDC_PASS, opts.ICQ.proxypass, sizeof(opts.ICQ.proxypass));
					opts.ICQ.proxyport = GetDlgItemInt(hwndDlg, IDC_PORT, NULL, FALSE);

					if ((SendDlgItemMessage(hwndDlg, IDC_USE, BM_GETCHECK, 0, 0) == BST_CHECKED) && !opts.ICQ.proxyuse)
					{
						icq_SetProxy(plink, opts.ICQ.proxyhost, (unsigned short)opts.ICQ.proxyport, opts.ICQ.proxyauth, opts.ICQ.proxyname, opts.ICQ.proxypass);
					}
					if (!(SendDlgItemMessage(hwndDlg, IDC_USE, BM_GETCHECK, 0, 0) == BST_CHECKED) && opts.ICQ.proxyuse)
					{
						icq_UnsetProxy(plink);
					}
					opts.ICQ.proxyuse  = SendDlgItemMessage(hwndDlg, IDC_USE, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;



					if (opts.ICQ.proxyuse)
						icq_SetProxy(plink,opts.ICQ.proxyhost,opts.ICQ.proxyport,opts.ICQ.proxyauth,opts.ICQ.proxyname,opts.ICQ.proxypass);
					else
						icq_UnsetProxy(plink);

					SaveOptions(&opts, myprofile);
					//SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
					//DestroyWindow(hwndDlg);
					return TRUE;
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
			}
			break;
	}
	return FALSE;
}



BOOL CALLBACK DlgProcGenOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	long tmp;

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{			
			SendDlgItemMessage(hwndDlg, IDC_MIN2TRAY, BM_SETCHECK, (opts.flags1 & FG_MIN2TRAY) ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, IDC_ONTOP, BM_SETCHECK, (opts.flags1 & FG_ONTOP) ? BST_CHECKED : BST_UNCHECKED, 0);			
			//SendDlgItemMessage(hwndDlg, IDC_TRANSP, BM_SETCHECK, (opts.flags1 & FG_TRANSP) ? BST_CHECKED : BST_UNCHECKED, 0);

			SendDlgItemMessage(hwndDlg, IDC_POPUP, BM_SETCHECK, (opts.flags1 & FG_POPUP) ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, IDC_TOOLWND, BM_SETCHECK, (opts.flags1 & FG_TOOLWND) ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, IDC_ONECLK, BM_SETCHECK, (opts.flags1 & FG_ONECLK) ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, IDC_HIDEOFFLINE, BM_SETCHECK, (opts.flags1 & FG_HIDEOFFLINE) ? BST_CHECKED : BST_UNCHECKED, 0);
			
			SendDlgItemMessage(hwndDlg, IDC_USEINI, BM_SETCHECK, rto.useini ? BST_CHECKED : BST_UNCHECKED, 0);
			return TRUE;
		}
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				case PSN_APPLY:

					tmp=opts.flags1 & FG_HIDEOFFLINE;//used for hide offline users
					opts.flags1 = 0;
					opts.flags1 |= SendDlgItemMessage(hwndDlg, IDC_MIN2TRAY, BM_GETCHECK, 0, 0) ? FG_MIN2TRAY : 0;
					opts.flags1 |= SendDlgItemMessage(hwndDlg, IDC_ONTOP, BM_GETCHECK, 0, 0) ? FG_ONTOP : 0;					
					//opts.flags1 |= SendDlgItemMessage(hwndDlg, IDC_TRANSP, BM_GETCHECK, 0, 0) ? FG_TRANSP : 0;
					opts.flags1 |= SendDlgItemMessage(hwndDlg, IDC_POPUP, BM_GETCHECK, 0, 0) ? FG_POPUP : 0;
					opts.flags1 |= SendDlgItemMessage(hwndDlg, IDC_TOOLWND, BM_GETCHECK, 0, 0) ? FG_TOOLWND : 0;
					opts.flags1 |= SendDlgItemMessage(hwndDlg, IDC_ONECLK, BM_GETCHECK, 0, 0) ? FG_ONECLK : 0;
					
					
					opts.flags1 |= SendDlgItemMessage(hwndDlg, IDC_HIDEOFFLINE, BM_GETCHECK, 0, 0) ? FG_HIDEOFFLINE : 0;

					//UPDATE HIDE OFFLINE USERS 
					if ((opts.flags1 & FG_HIDEOFFLINE)==FG_HIDEOFFLINE && !tmp)
					{//use to be showing, now hiding
						//kill off all offline guys
						int i;
						for (i=0;i<opts.ccount;i++)
						{
							if (opts.clist[i].status==STATUS_OFFLINE || opts.clist[i].status==(STATUS_OFFLINE & 0xFFFF))
							{
								HTREEITEM t;
								t=UinToTree(opts.clist[i].uin);
								TreeView_DeleteItem(rto.hwndContact, t);
							}
						}
					}
					else if ((opts.flags1 & FG_HIDEOFFLINE)==0 && tmp)
					{//use to have hide, now show all
						//add all the hiden ppl
						int i;
						for (i=0;i<opts.ccount;i++)
						{
							if (opts.clist[i].status==STATUS_OFFLINE || opts.clist[i].status==(STATUS_OFFLINE & 0xFFFF))
							{
								DisplayContact(&opts.clist[i]);
							}
						}
						SortContacts();
					}
					


					rto.useini = SendDlgItemMessage(hwndDlg, IDC_USEINI, BM_GETCHECK, 0, 0) ? TRUE : FALSE;
					if (!rto.useini) remove(rto.inifile);

					SetWindowPos(ghwnd, (opts.flags1 & FG_ONTOP) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
					
					if (opts.flags1 & FG_TOOLWND)
					{
						ShowWindow(ghwnd, SW_HIDE);
						SetWindowLong(ghwnd, GWL_EXSTYLE, GetWindowLong(ghwnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
						ShowWindow(ghwnd, SW_NORMAL);
					}
					else
					{
						ShowWindow(ghwnd, SW_HIDE);
						SetWindowLong(ghwnd, GWL_EXSTYLE, GetWindowLong(ghwnd, GWL_EXSTYLE) & ~WS_EX_TOOLWINDOW);
						ShowWindow(ghwnd, SW_NORMAL);
					}
					SaveOptions(&opts, myprofile);
					SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);

					RefreshContacts();
					return TRUE;
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcOpsTransparency(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//char buf[MAX_PATH];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{			
			long t;
			SendDlgItemMessage(hwndDlg, IDC_ENABLE, BM_SETCHECK, (opts.flags1 & FG_TRANSP) ? BST_CHECKED : BST_UNCHECKED, 0);		

			
			SendDlgItemMessage(hwndDlg,IDC_TRANSLEVEL,TBM_SETRANGEMAX,TRUE,0);
			SendDlgItemMessage(hwndDlg,IDC_TRANSLEVEL,TBM_SETRANGEMIN,TRUE,-255);
			
			SendDlgItemMessage(hwndDlg,IDC_TRANSLEVEL2,TBM_SETRANGEMAX,TRUE,0);
			SendDlgItemMessage(hwndDlg,IDC_TRANSLEVEL2,TBM_SETRANGEMIN,TRUE,-255);

			if (opts.autoalpha==-1)
			{t=opts.alpha;}
			else
			{t=opts.autoalpha;}

			SendDlgItemMessage(hwndDlg,IDC_TRANSLEVEL,TBM_SETPOS,TRUE,-(t));
			SendDlgItemMessage(hwndDlg,IDC_TRANSLEVEL2,TBM_SETPOS,TRUE,-(opts.alpha));

			return TRUE;
		}
		case WM_HSCROLL://drop thru
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				
				case PSN_APPLY:
					opts.flags1=opts.flags1 & ~FG_TRANSP;//clear fg_trans bit

					opts.flags1 |= SendDlgItemMessage(hwndDlg, IDC_ENABLE, BM_GETCHECK, 0, 0) ? FG_TRANSP : 0;
					
					opts.alpha= -SendDlgItemMessage(hwndDlg,IDC_TRANSLEVEL2,TBM_GETPOS,0,0);
					opts.autoalpha= -SendDlgItemMessage(hwndDlg,IDC_TRANSLEVEL,TBM_GETPOS,0,0);
					
					if (opts.alpha==opts.autoalpha){opts.autoalpha=-1;}

					//update transparencey levels
					if (opts.flags1 & FG_TRANSP)
					{
						SetWindowLong(ghwnd, GWL_EXSTYLE, GetWindowLong(ghwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
						if (mySetLayeredWindowAttributes) mySetLayeredWindowAttributes(ghwnd, RGB(0,0,0), (BYTE)opts.alpha, LWA_ALPHA);
						RedrawWindow(ghwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
					}
					else
					{
						SetWindowLong(ghwnd, GWL_EXSTYLE, GetWindowLong(ghwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
						RedrawWindow(ghwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
					}
					
					
					
					return TRUE;
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
			}
			break;
	}
	return FALSE;
}


BOOL CALLBACK DlgProcOpsAdvanced(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//char buf[MAX_PATH];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{			
			
			//SendDlgItemMessage(hwndDlg,IDC_TRANSLEVEL,TBM_SETPOS,TRUE,-(t));
			
			if (opts.ICQ.sendmethod==ICQ_SEND_THRUSERVER)
				SendDlgItemMessage(hwndDlg,IDC_THRUSERVER,BM_SETCHECK,BST_CHECKED,0);

			if (opts.ICQ.sendmethod==ICQ_SEND_DIRECT)
				SendDlgItemMessage(hwndDlg,IDC_DIRECT,BM_SETCHECK,BST_CHECKED,0);
			
			if (opts.ICQ.sendmethod==ICQ_SEND_BESTWAY)
				SendDlgItemMessage(hwndDlg,IDC_BESTWAY,BM_SETCHECK,BST_CHECKED,0);
			
			return TRUE;
		}
		//case WM_HSCROLL://drop thru
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				
				case PSN_APPLY:
					if (SendDlgItemMessage(hwndDlg,IDC_THRUSERVER,BM_GETCHECK,0,0))
						opts.ICQ.sendmethod=ICQ_SEND_THRUSERVER;

					if (SendDlgItemMessage(hwndDlg,IDC_DIRECT,BM_GETCHECK,0,0))
						opts.ICQ.sendmethod=ICQ_SEND_DIRECT;

					if (SendDlgItemMessage(hwndDlg,IDC_BESTWAY,BM_GETCHECK,0,0))
						opts.ICQ.sendmethod=ICQ_SEND_BESTWAY;
					
					
					return TRUE;
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
			}
			break;
	}
	return FALSE;
}


BOOL CALLBACK DlgProcOpsHotkey(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char buf[MAX_PATH];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{						
			SendDlgItemMessage(hwndDlg, IDC_SHOWHIDE, BM_SETCHECK, (opts.hotkey_showhide!=-1) ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, IDC_READMSG, BM_SETCHECK, (opts.hotkey_setfocus!=-1) ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwndDlg, IDC_NETSEARCH, BM_SETCHECK, (opts.hotkey_netsearch!=-1) ? BST_CHECKED : BST_UNCHECKED, 0);
			
			buf[0]=opts.hotkey_showhide;buf[1]=0;
			SetDlgItemText(hwndDlg, IDC_TXTSHOWHIDE, buf);
			
			buf[0]=opts.hotkey_setfocus;buf[1]=0;
			SetDlgItemText(hwndDlg, IDC_TXTREADMSG, buf);

			buf[0]=opts.hotkey_netsearch;buf[1]=0;
			SetDlgItemText(hwndDlg, IDC_TXTNETSEARCH, buf);

			strcpy(buf,opts.netsearchurl);
			SetDlgItemText(hwndDlg, IDC_TXTURL, buf);

			return TRUE;
		}
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				
				case PSN_APPLY:
					//SHOWHIDE
					if (SendDlgItemMessage(hwndDlg, IDC_SHOWHIDE, BM_GETCHECK, 0, 0))
					{
						GetDlgItemText(hwndDlg, IDC_TXTSHOWHIDE, buf, MAX_PATH);
						if (buf[0])
							opts.hotkey_showhide=buf[0];
					}
					else
					{
						opts.hotkey_showhide=-1;
					}
					//SETFOCYUS / READMSG
					if (SendDlgItemMessage(hwndDlg, IDC_READMSG, BM_GETCHECK, 0, 0))
					{
						GetDlgItemText(hwndDlg, IDC_TXTREADMSG, buf, MAX_PATH);
						if (buf[0])
							opts.hotkey_setfocus=buf[0];
					}
					else
					{
						opts.hotkey_setfocus=-1;
					}
					//NETSEARCH
					if (SendDlgItemMessage(hwndDlg, IDC_NETSEARCH, BM_GETCHECK, 0, 0))
					{
						GetDlgItemText(hwndDlg, IDC_TXTNETSEARCH, buf, MAX_PATH);
						if (buf[0])
							opts.hotkey_netsearch=buf[0];
					}
					else
					{
						opts.hotkey_netsearch=-1;
					}

					GetDlgItemText(hwndDlg, IDC_TXTURL, buf, MAX_PATH);
					strcpy(opts.netsearchurl,buf);
					
					return TRUE;
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcIcqNew(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{
			SetDlgItemText(hwndDlg, IDC_MSG, "Enter personal details");
			ChangeStatus(hwndDlg, STATUS_OFFLINE);
			icq_Connect(plink, opts.ICQ.hostname, opts.ICQ.port);
		}
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 0, 0);
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					char password[128];
					link.icq_NewUIN = CbNewUIN;
					GetDlgItemText(hwndDlg, IDC_PASS, password, sizeof(password));
					icq_RegNewUser(plink, password);

					break;
				}
				case IDCANCEL:
					EndDialog(hwndDlg, 0);
					break;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcICQOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char buf[MAX_PATH];
	BOOL oldstate;
	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{
			SendDlgItemMessage(hwndDlg, IDC_USEICQ, BM_SETCHECK, (opts.ICQ.enabled) ? BST_CHECKED : BST_UNCHECKED, 0);

			sprintf(buf,"%d",opts.ICQ.uin);
			SetDlgItemText(hwndDlg, IDC_ICQNUM, buf);
			SetDlgItemText(hwndDlg, IDC_PASSWORD, opts.ICQ.password);
			return TRUE;
		}
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 0, 0);
			switch (LOWORD(wParam))
			{
				case IDC_NEW:
				{
						HWNDNODE *node;
						node = malloc(sizeof(HWNDNODE));

						node->windowtype = WT_ADDED;
						node->next = rto.hwndlist;
						rto.hwndlist = node;

						node->hwnd = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_ICQNEW), NULL, DlgProcIcqNew);
						ShowWindow(node->hwnd, SW_SHOW);
				}
			}
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				
				case PSN_APPLY:
					GetDlgItemText(hwndDlg, IDC_ICQNUM, buf, sizeof(buf));
					opts.ICQ.uin=atoi(buf);
					GetDlgItemText(hwndDlg, IDC_PASSWORD, opts.ICQ.password, sizeof(opts.ICQ.password));
					
					oldstate=opts.ICQ.enabled;

					opts.ICQ.enabled=SendDlgItemMessage(hwndDlg, IDC_USEICQ, BM_GETCHECK, 0, 0);
					if (oldstate!=opts.ICQ.enabled)
						MessageBox(hwndDlg,"You will have to restart Miranda for your ICQ changes to take affect.",MIRANDANAME,MB_OK);

					return TRUE;
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcMSNOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL oldstate;
	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{		
			SendDlgItemMessage(hwndDlg, IDC_USEMSN, BM_SETCHECK, (opts.MSN.enabled) ? BST_CHECKED : BST_UNCHECKED, 0);		
			SetDlgItemText(hwndDlg, IDC_UHANDLE, opts.MSN.uhandle);
			SetDlgItemText(hwndDlg, IDC_PASSWORD, opts.MSN.password);
			
			return TRUE;
		}
		case WM_COMMAND:
			SendMessage(hProp, PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				
				case PSN_APPLY:
						oldstate=opts.MSN.enabled;
						GetDlgItemText(hwndDlg, IDC_UHANDLE, opts.MSN.uhandle, MSN_UHANDLE_LEN);
						GetDlgItemText(hwndDlg, IDC_PASSWORD, opts.MSN.password, MSN_PASSWORD_LEN);
					
						opts.MSN.enabled=SendDlgItemMessage(hwndDlg, IDC_USEMSN, BM_GETCHECK, 0, 0);
						
						if (oldstate!=opts.MSN.enabled)
							MessageBox(hwndDlg,"You will have to restart Miranda for your MSN changes to take affect.",MIRANDANAME,MB_OK);

					return TRUE;
				case PSN_QUERYCANCEL:
					SetWindowLong(hwndDlg, DWL_MSGRESULT, FALSE);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcHistory(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{
			int i = 0;
			int sz;
			char buf[4096];
			char uinb[32];
			FILE *fp;
			HISTORY *hs;
			
			sprintf(uinb, "%d", lParam);
			if (lParam==0)
			{//all users
				SendMessage(hwndDlg,WM_SETTEXT,0,"History for All Users");
			}
			else
			{
				sprintf(buf,"History for %d",lParam);
				//check if they have a nick name
				for (i=0;i<opts.ccount;i++)
				{
					if (opts.clist[i].uin==(unsigned long)lParam)
					{
						sprintf(buf,"History for %s (%d)",opts.clist[i].nick,lParam);		
						break;
					}
				}
				i=0;//reset i
				
				SendMessage(hwndDlg,WM_SETTEXT,0,buf);
				
			}

			

			hs = malloc(sizeof(HISTORY));
			hs->bodys = malloc(sizeof(char *) * 100);
			hs->max = 100;
			hs->count = 0;
			SetWindowLong(hwndDlg, GWL_USERDATA, (long)hs);
			if ((fp = fopen(opts.history, "rb")) == NULL) return TRUE;
			while (fgets(buf, sizeof(buf), fp))
			{
				//a uin of 0 will show ALL records
				if (!strncmp(buf, uinb, strlen(uinb)) || lParam==0)
				{
					buf[strlen(buf)-4] = 0;
					SendDlgItemMessage(hwndDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)(strchr(buf, '#')+1));
					*buf = 0;
					sz = strlen(buf);
					while (fgets(buf + sz, sizeof(buf) - sz, fp) && strncmp(buf + sz, "оооо", 4))
					{
						sz = strlen(buf);
					}
					buf[strlen(buf)-6] = 0;
					if (i == hs->max)
					{
						hs->max *= 2;
						hs->bodys = realloc(hs->bodys, sizeof(char *) * hs->max);
					}
					hs->bodys[i++] = _strdup(buf);
				}
				else
				{
					while(fgets(buf, sizeof(buf), fp) && strncmp(buf, "оооо", 4));
				}
			}
			hs->count = i;
			if (i)
			{
				SendDlgItemMessage(hwndDlg, IDC_LIST, LB_SETCURSEL, i - 1, 0);
				SetDlgItemText(hwndDlg, IDC_EDIT, hs->bodys[i-1]);
			}
			fclose(fp);
			return TRUE;
		}
		case WM_DESTROY:
		{
			HISTORY *hs;
			int i;
			hs = (HISTORY *)GetWindowLong(hwndDlg, GWL_USERDATA);
			for (i = 0; i < hs->count; i++)
			{
				free(hs->bodys[i]);
			}
			free(hs->bodys);
			free(hs);
			return FALSE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDC_FIND:
					ShowWindow(CreateDialogParam(ghInstance, MAKEINTRESOURCE(IDD_HISTORY_FIND), hwndDlg, DlgProcHistoryFind, hwndDlg), SW_SHOW);
					return TRUE;
				case IDC_LIST:
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{
						int i;
						HISTORY *hs;
						hs = (HISTORY *)GetWindowLong(hwndDlg, GWL_USERDATA);
						i = SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
						SetDlgItemText(hwndDlg, IDC_EDIT, hs->bodys[i]);
					}
					return TRUE;
			}
			break;
	}
	return FALSE;
}


BOOL CALLBACK DlgProcHistoryFind(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char buffer[128];
	char *tmp;
	static int startmsg=0;
	int i;
	HISTORY *hs;
	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
		{//make oour userdata point to the history wnd hs
			hs = (HISTORY *)GetWindowLong((HWND)lParam, GWL_USERDATA);
			SetWindowLong(hwndDlg, GWL_USERDATA, (long)hs);	
		}
		case WM_DESTROY:
		{
			
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_FINDWHAT:
					//a text box has changed
					if (HIWORD(wParam)==EN_CHANGE)
							startmsg=0;
					break;
				case IDOK://find Next
					hs = (HISTORY *)GetWindowLong(hwndDlg, GWL_USERDATA);
					GetDlgItemText(hwndDlg, IDC_FINDWHAT, buffer, sizeof(buffer));
					strcpy(buffer,strupr(buffer));//make it upper case
					for (i=startmsg;i<hs->count;i++)
					{
						startmsg=i+1;
						//get an upper case copy going
						tmp=(char*)malloc(strlen(hs->bodys[i])+1);
						strcpy(tmp,hs->bodys[i]);
						tmp=strupr(tmp);

						if (strstr(tmp,buffer))
						{//found it
							
							SendDlgItemMessage(GetParent(hwndDlg), IDC_LIST, LB_SETCURSEL, i , 0);
							SendMessage(GetParent(hwndDlg),WM_COMMAND,MAKEWPARAM(IDC_LIST,LBN_SELCHANGE),0);
							free(tmp);
							break;
						}
						free(tmp);
						
					}
					if (i==hs->count)
					{
						startmsg=0;
						MessageBox(hwndDlg,"The end has been reached, further searches will start from the top.",MIRANDANAME,MB_OK);
					}
					return TRUE;
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
				
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcMSN_AddContact(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem; 

	char buffer[128];

	switch (msg)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAYONLINE)));
			return TRUE;
		case WM_COMMAND:
			switch ((int)wParam)
			{
				case IDOK:
					hwndItem = GetDlgItem(hwndDlg, IDC_EMAIL);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);

					if (opts.MSN.logedin)
					{
						char out[MSN_UHANDLE_LEN+5];
						
						sprintf(out,"FL %s nickname",buffer);
						MSN_SendPacket(opts.MSN.sNS,"ADD",out);
					}
					else
					{
						MessageBox(ghwnd,"You must be logged on to add MSN Contacts.",MIRANDANAME,MB_OK);
					}

					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDCANCEL:
					DeleteWindowFromList(hwndDlg);
					DestroyWindow(hwndDlg);
					return TRUE;
			}
	}
	return FALSE;
}


BOOL CALLBACK DlgProcRecvFile(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndItem;
	DLG_DATA *dlgdata;
	char buffer[128];
	char *buf;
	unsigned long uin;
	icq_FileSession *fs;

	switch (msg)
	{
		case TI_POP:
			SetForegroundWindow(hwndDlg);
			break;
		case WM_INITDIALOG:
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(ghIcons, MAKEINTRESOURCE(IDI_RECVMSG)));

			dlgdata = (DLG_DATA *)lParam;
			buf = LookupContactNick(dlgdata->uin);
			
			hwndItem = GetDlgItem(hwndDlg, IDC_FROM);
			if (buf)
			{
				sprintf(buffer, "%s", buf);
			}
			else
			{
				sprintf(buffer, "%d", dlgdata->uin);
			}
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);

			hwndItem = GetDlgItem(hwndDlg, IDC_DATE);
			sprintf(buffer, "%d:%2.2d %d/%2.2d/%4.4d", dlgdata->hour, dlgdata->minute, dlgdata->day, dlgdata->month, dlgdata->year);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);

			if (!InContactList(dlgdata->uin)) EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), TRUE);

			hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
			sprintf(buffer, "%d", dlgdata->uin);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)buffer);

			hwndItem = GetDlgItem(hwndDlg, IDC_FILE);
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)dlgdata->url);


			hwndItem = GetDlgItem(hwndDlg, IDC_MSG);
			
			SendMessage(hwndItem, WM_SETTEXT, 0, (LPARAM)dlgdata->msg);

			SetWindowLong(hwndDlg,GWL_USERDATA,dlgdata->seq);
						
			return TRUE;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				hwndModeless = hwndDlg;
			}
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				hwndModeless = NULL;
			}
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
					uin = atol(buffer);

					fs=icq_AcceptFileRequest(plink,uin,GetWindowLong(hwndDlg,GWL_USERDATA));
					
					//set RECV dir
					sprintf(buffer,"%sRecv\\%d\\",mydir,uin);
					icq_FileSessionSetWorkingDir(fs,buffer);

					DestroyWindow(hwndDlg);
					return TRUE;
				case IDCANCEL:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);

					icq_RefuseFileRequest(plink,uin,GetWindowLong(hwndDlg,GWL_USERDATA),"");
	
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDC_ADD:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					AddToContactList(uin);
					EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
					return TRUE;
				case IDC_DETAILS:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 128, (LPARAM)buffer);
					uin = atol(buffer);
					DisplayUINDetails(uin);
					return TRUE;
				case IDC_HISTORY:
					hwndItem = GetDlgItem(hwndDlg, IDC_ICQ);
					SendMessage(hwndItem, WM_GETTEXT, 1024, (LPARAM)buffer);
					uin = atol(buffer);
					DisplayHistory(hwndDlg, uin);
					return TRUE;
			}
	}

	return FALSE;
}
