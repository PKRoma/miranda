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
#include "../../core/commonheaders.h"

#define SUMMARY     0
#define DETAIL      1
#define DM_FINDNEXT  (WM_USER+10)
#define DM_HREBUILD  (WM_USER+11)

static BOOL CALLBACK DlgProcHistory(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcHistoryFind(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static HANDLE hWindowList=0;

static int UserHistoryCommand(WPARAM wParam,LPARAM lParam)
{
    HWND hwnd;

    if(hwnd=WindowList_Find(hWindowList,(HANDLE)wParam)) {
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
		return 0;
	}
	CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_HISTORY),NULL,DlgProcHistory,wParam);
	return 0;
}

static int HistoryContactDelete(WPARAM wParam,LPARAM lParam)
{
	HWND hwnd;
	hwnd=WindowList_Find(hWindowList,(HANDLE)wParam);
	if(hwnd!=NULL) DestroyWindow(hwnd);
	return 0;
}

int PreShutdownHistoryModule(WPARAM wParam, LPARAM lParam)
{
	if (hWindowList) WindowList_BroadcastAsync(hWindowList,WM_DESTROY,0,0);
	return 0;
}

int LoadHistoryModule(void)
{
	CLISTMENUITEM mi;

	//bit of a fudge that the one service works for both global requests and
	//the contact list's menu processing stuff
	CreateServiceFunction(MS_HISTORY_SHOWCONTACTHISTORY,UserHistoryCommand);
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=1000090000;
	mi.flags=0;
	mi.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_HISTORY));
	mi.pszContactOwner=NULL;    //all contacts
	mi.pszName=Translate("View &History");
	mi.pszService=MS_HISTORY_SHOWCONTACTHISTORY;
	CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);
    hWindowList=(HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST,0,0);
    HookEvent(ME_DB_CONTACT_DELETED,HistoryContactDelete);
	HookEvent(ME_SYSTEM_PRESHUTDOWN,PreShutdownHistoryModule);
	return 0;
}

static void GetMessageDescription(int type,DBEVENTINFO *dbei,char *buf,int cbBuf)
{
	if(type==SUMMARY) {
		if(dbei->flags&DBEF_SENT) mir_snprintf(buf,cbBuf,Translate("Outgoing Message"));
		else mir_snprintf(buf,cbBuf,Translate("Incoming Message"));
	}
	else {
		strncpy(buf,dbei->pBlob,cbBuf);
		buf[cbBuf-1]=0;
	}
}

static void GetUrlDescription(int type,DBEVENTINFO *dbei,char *buf,int cbBuf)
{
	if(type==SUMMARY) {
		if(dbei->flags&DBEF_SENT) mir_snprintf(buf,cbBuf,Translate("Outgoing URL"));
		else mir_snprintf(buf,cbBuf,Translate("Incoming URL"));
	}
	else {
		CopyMemory(buf,dbei->pBlob,min((DWORD)cbBuf,dbei->cbBlob));
		buf[cbBuf-1]=0;
		if((int)strlen(buf)<cbBuf-1) {
			MoveMemory(buf+strlen(buf)+1,buf+strlen(buf),cbBuf-strlen(buf)-2);
			buf[cbBuf-1]=0;
			buf[strlen(buf)+1]='\n';
			buf[strlen(buf)]='\r';
		}
	}
}

static void GetAuthReqDescription(int type,DBEVENTINFO *dbei,char *buf,int cbBuf)
{
    buf[0]=0;
}

static void GetAddedDescription(int type,DBEVENTINFO *dbei,char *buf,int cbBuf)
{
    buf[0]=0;
}

static void GetFileDescription(int type,DBEVENTINFO *dbei,char *buf,int cbBuf)
{
	if(type==SUMMARY) {
		if(dbei->flags&DBEF_SENT) mir_snprintf(buf,cbBuf,Translate("Outgoing File"));
		else mir_snprintf(buf,cbBuf,Translate("Incoming File"));
	}
	else {
		CopyMemory(buf,dbei->pBlob+sizeof(DWORD),min((DWORD)cbBuf,dbei->cbBlob-sizeof(DWORD)));
		buf[cbBuf-1]=0;
		if((int)strlen(buf)<cbBuf-1) {
			MoveMemory(buf+strlen(buf)+1,buf+strlen(buf),cbBuf-strlen(buf)-2);
			buf[cbBuf-1]=0;
			buf[strlen(buf)+1]='\n';
			buf[strlen(buf)]='\r';
		}
	}
}

static void GetObjectDescription(DBEVENTINFO *dbei,int type,char *str,int cbStr)
{
	str[0]=0;
	switch(dbei->eventType) {
		case EVENTTYPE_MESSAGE: GetMessageDescription(type,dbei,str,cbStr); break;
		case EVENTTYPE_URL: GetUrlDescription(type,dbei,str,cbStr); break;
		case EVENTTYPE_AUTHREQUEST: GetAuthReqDescription(type,dbei,str,cbStr); break;
		case EVENTTYPE_ADDED: GetAddedDescription(type,dbei,str,cbStr); break;
		case EVENTTYPE_FILE: GetFileDescription(type,dbei,str,cbStr); break;
	}
}

typedef struct {
	HANDLE hContact;
	HWND hwnd;
} THistoryThread;

static void FillHistoryThread(THistoryThread *hInfo)
{
	char str[200],eventText[256],strdatetime[64];
	HANDLE hDbEvent;
	DBEVENTINFO dbei;
	int newBlobSize,oldBlobSize,i;
	DBTIMETOSTRING dbtts;
	HWND hwndList;

	SendDlgItemMessage(hInfo->hwnd,IDC_LIST,LB_RESETCONTENT,0,0);
	i=CallService(MS_DB_EVENT_GETCOUNT,(WPARAM)hInfo->hContact,0);
	SendDlgItemMessage(hInfo->hwnd,IDC_LIST,LB_INITSTORAGE,i,i*40);

	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.pBlob=NULL;
	oldBlobSize=0;
	hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDLAST,(WPARAM)hInfo->hContact,0);
	dbtts.cbDest=sizeof(strdatetime);
	dbtts.szDest=strdatetime;
	dbtts.szFormat="d t";
	hwndList=GetDlgItem(hInfo->hwnd,IDC_LIST);
	while(hDbEvent!=NULL) {
		if (!IsWindow(hInfo->hwnd)) break;
		newBlobSize=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
		if(newBlobSize>oldBlobSize) {
			dbei.pBlob=(PBYTE)realloc(dbei.pBlob,newBlobSize);
			oldBlobSize=newBlobSize;
		}
		dbei.cbBlob=oldBlobSize;
		CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
		GetObjectDescription(&dbei,SUMMARY,str,sizeof(str));
		if(str[0]) {
			CallService(MS_DB_TIME_TIMESTAMPTOSTRING,dbei.timestamp,(LPARAM)&dbtts);
			wsprintf(eventText,"%s: %s",strdatetime,str);
			i=SendMessage(hwndList,LB_ADDSTRING,0,(LPARAM)eventText);
			SendMessage(hwndList,LB_SETITEMDATA,i,(LPARAM)hDbEvent);
		}
		hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDPREV,(WPARAM)hDbEvent,0);
	}
	if(dbei.pBlob!=NULL) free(dbei.pBlob);

	SendDlgItemMessage(hInfo->hwnd,IDC_LIST,LB_SETCURSEL,0,0);
	SendMessage(hInfo->hwnd,WM_COMMAND,MAKEWPARAM(IDC_LIST,LBN_SELCHANGE),0);
    EnableWindow(GetDlgItem(hInfo->hwnd, IDC_LIST), TRUE);
	free(hInfo);
}

static int HistoryDlgResizer(HWND hwndDlg,LPARAM lParam,UTILRESIZECONTROL *urc) {
	switch(urc->wId) {
		case IDC_LIST:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
		case IDC_EDIT:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_BOTTOM;
		case IDC_FIND:
		case IDC_DELETEHISTORY:
			return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
		case IDOK:
			return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static BOOL CALLBACK DlgProcHistory(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact;

	hContact=(HANDLE)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (msg)
	{
		case WM_INITDIALOG:
		{   char *contactName;
			char str[200];

			TranslateDialogDefault(hwndDlg);
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);
			hContact = (HANDLE)lParam;
            WindowList_Add(hWindowList,hwndDlg,hContact);			
			Utils_RestoreWindowPosition(hwndDlg,hContact,"History","");
			contactName=(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0);
			mir_snprintf(str,sizeof(str),Translate("History for %s"),contactName);
			SetWindowText(hwndDlg,str);
			SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_HISTORY)));
			SendMessage(hwndDlg,DM_HREBUILD,0,0);
			return TRUE;
		}
		case DM_HREBUILD:
		{
			THistoryThread *hInfo = (THistoryThread*)malloc(sizeof(THistoryThread));
            EnableWindow(GetDlgItem(hwndDlg, IDC_LIST), FALSE);
			hInfo->hContact = hContact;
			hInfo->hwnd = hwndDlg;			
			forkthread(FillHistoryThread, 0, hInfo);
			return TRUE;
		}
		case WM_DESTROY:
		{
			Utils_SaveWindowPosition(hwndDlg,hContact,"History","");
			WindowList_Remove(hWindowList,hwndDlg);
			return TRUE;
		}
        case WM_GETMINMAXINFO:
        {
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x=300; 
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y=230;
        }
		case WM_SIZE:
		{
			UTILRESIZEDIALOG urd={0};
			urd.cbSize=sizeof(urd);
			urd.hwndDlg=hwndDlg;
			urd.hInstance=GetModuleHandle(NULL);
			urd.lpTemplate=MAKEINTRESOURCE(IDD_HISTORY);
			urd.lParam=(LPARAM)NULL;
			urd.pfnResizer=HistoryDlgResizer;
			CallService(MS_UTILS_RESIZEDIALOG,0,(LPARAM)&urd);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDC_FIND:
					ShowWindow(CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HISTORY_FIND), hwndDlg, DlgProcHistoryFind, (LPARAM)hwndDlg), SW_SHOW);
					return TRUE;
				case IDC_DELETEHISTORY:
				{
					HANDLE hDbevent;
					int index;
					index = SendDlgItemMessage(hwndDlg,IDC_LIST,LB_GETCURSEL,0,0);
					if(index==LB_ERR) break;
					if (MessageBox(hwndDlg,Translate("Are you sure you want to delete this history item?"),Translate("Delete History"),MB_YESNO|MB_ICONQUESTION)==IDYES) {
						hDbevent = (HANDLE)SendDlgItemMessage(hwndDlg,IDC_LIST,LB_GETITEMDATA,index,0);
						CallService(MS_DB_EVENT_DELETE,(WPARAM)hContact,(LPARAM)hDbevent);
						SendMessage(hwndDlg,DM_HREBUILD,0,0);
					}
					return TRUE;
				}
				case IDC_LIST:
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{	char str[8192],*contactName;
						HANDLE hDbEvent;
						DBEVENTINFO dbei;
						int sel;
						sel=SendDlgItemMessage(hwndDlg,IDC_LIST,LB_GETCURSEL,0,0);
						if(sel==LB_ERR) { EnableWindow(GetDlgItem(hwndDlg,IDC_DELETEHISTORY),FALSE); break; }
						EnableWindow(GetDlgItem(hwndDlg,IDC_DELETEHISTORY),TRUE);
						contactName=(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0);
						hDbEvent=(HANDLE)SendDlgItemMessage(hwndDlg,IDC_LIST,LB_GETITEMDATA,sel,0);
						ZeroMemory(&dbei,sizeof(dbei));
						dbei.cbSize=sizeof(dbei);
						dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
						dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
						CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
						GetObjectDescription(&dbei,DETAIL,str,sizeof(str));
						free(dbei.pBlob);
						if(str[0]) SetDlgItemText(hwndDlg, IDC_EDIT, str);
					}
					return TRUE;
			}
			break;
		case DM_FINDNEXT:
		{	char str[1024],*contactName;
			HANDLE hDbEvent,hDbEventStart;
			DBEVENTINFO dbei;
			int newBlobSize,oldBlobSize,index;

			contactName=(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0);
			index=SendDlgItemMessage(hwndDlg,IDC_LIST,LB_GETCURSEL,0,0);
			if(index==LB_ERR) break;
			hDbEventStart=(HANDLE)SendDlgItemMessage(hwndDlg,IDC_LIST,LB_GETITEMDATA,index,0);
			hDbEvent=hDbEventStart;
			ZeroMemory(&dbei,sizeof(dbei));
			dbei.cbSize=sizeof(dbei);
			dbei.pBlob=NULL;
			oldBlobSize=0;
			for(;;) {
				hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDPREV,(WPARAM)hDbEvent,0);
				index++;
				if(hDbEvent==NULL) {
					hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDLAST,(WPARAM)hContact,0);
					index=0;
				}
				if(hDbEvent==hDbEventStart) break;
				newBlobSize=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDbEvent,0);
				if(newBlobSize>oldBlobSize) {
					dbei.pBlob=(PBYTE)realloc(dbei.pBlob,newBlobSize);
					oldBlobSize=newBlobSize;
				}
				dbei.cbBlob=oldBlobSize;
				CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
				GetObjectDescription(&dbei,DETAIL,str,sizeof(str));
				if(str[0]) {
					CharUpperBuff(str,lstrlen(str));
					if(strstr(str,(const char*)lParam)!=NULL) {
						SendDlgItemMessage(hwndDlg,IDC_LIST,LB_SETCURSEL,index,0);
						SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_LIST,LBN_SELCHANGE),0);
						break;
					}
				}
			}
			free(dbei.pBlob);
			break;
		}
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcHistoryFind(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(hwndDlg, GWL_USERDATA, lParam);	
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK://find Next
				{	HWND hwndParent;
					char str[128];

					hwndParent=(HWND)GetWindowLong(hwndDlg, GWL_USERDATA);
					GetDlgItemText(hwndDlg, IDC_FINDWHAT, str, sizeof(str));
					CharUpperBuff(str,lstrlen(str));
					SendMessage(hwndParent,DM_FINDNEXT,0,(LPARAM)str);
					return TRUE;
				}
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
				
			}
			break;
	}
	return FALSE;
}