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
#include "msgs.h"

#define TIMERID_MSGSEND      0
#define TIMERID_FLASHWND     1
#define TIMERID_ANTIBOMB     2
#define TIMEOUT_FLASHWND     900
#define TIMEOUT_MSGSEND      9000		  //ms
#define TIMEOUT_ANTIBOMB     4000  //multiple-send bombproofing: send max 5 messages every 4 seconds
#define ANTIBOMB_COUNT       5

extern HCURSOR hCurSplitNS,hCurSplitWE,hCurHyperlinkHand;
extern HANDLE hMessageWindowList,hMessageSendList;
extern struct CREOleCallback reOleCallback;

static WNDPROC OldMessageEditProc,OldSplitterProc;
static const UINT infoLineControls[]={IDC_PROTOCOL,IDC_NAME};
static const UINT buttonLineControls[]={IDC_ADD,IDC_USERMENU,IDC_DETAILS,IDC_HISTORY,IDC_TIME,IDC_MULTIPLE};
static const UINT sendControls[]={IDC_ST_ENTERMSG,IDC_MESSAGE};
static const UINT recvControls[]={IDC_LOG,IDC_QUOTE,IDC_READNEXT};

static void ShowMultipleControls(HWND hwndDlg,const UINT *controls,int cControls,int state)
{
	int i;
	for(i=0;i<cControls;i++) ShowWindow(GetDlgItem(hwndDlg,controls[i]),state);
}

static void SetDialogToType(HWND hwndDlg)
{
	struct MessageWindowData *dat;
	WINDOWPLACEMENT pl={0};
	int isSplit=DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT);

	dat=(struct MessageWindowData*)GetWindowLong(hwndDlg,GWL_USERDATA);
	if(dat->hContact) {
		ShowMultipleControls(hwndDlg,infoLineControls,sizeof(infoLineControls)/sizeof(infoLineControls[0]),DBGetContactSettingByte(NULL,"SRMsg","ShowInfoLine",SRMSGDEFSET_SHOWINFOLINE)?SW_SHOW:SW_HIDE);
	}
	else ShowMultipleControls(hwndDlg,infoLineControls,sizeof(infoLineControls)/sizeof(infoLineControls[0]),SW_HIDE);
	if(dat->hContact) {
		ShowMultipleControls(hwndDlg,buttonLineControls,sizeof(buttonLineControls)/sizeof(buttonLineControls[0]),DBGetContactSettingByte(NULL,"SRMsg","ShowButtonLine",SRMSGDEFSET_SHOWBUTTONLINE)?SW_SHOW:SW_HIDE);
		if(!DBGetContactSettingByte(dat->hContact,"CList","NotOnList",0)) ShowWindow(GetDlgItem(hwndDlg, IDC_ADD),SW_HIDE);
	}
	else {
		ShowMultipleControls(hwndDlg,buttonLineControls,sizeof(buttonLineControls)/sizeof(buttonLineControls[0]),SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg,IDC_MULTIPLE),DBGetContactSettingByte(NULL,"SRMsg","ShowButtonLine",SRMSGDEFSET_SHOWBUTTONLINE)?SW_SHOW:SW_HIDE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_MULTIPLE),FALSE);
	}
	ShowMultipleControls(hwndDlg,sendControls,sizeof(sendControls)/sizeof(sendControls[0]),isSplit || dat->isSend?SW_SHOW:SW_HIDE);
	ShowMultipleControls(hwndDlg,recvControls,sizeof(recvControls)/sizeof(recvControls[0]),isSplit || !dat->isSend?SW_SHOW:SW_HIDE);
	if(!isSplit) {
		if(dat->isSend) ShowWindow(GetDlgItem(hwndDlg,IDC_TIME),SW_HIDE);
		else {
			ShowWindow(GetDlgItem(hwndDlg,IDC_MULTIPLE),SW_HIDE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MULTIPLE),FALSE);
		}
	}
	if(!DBGetContactSettingByte(NULL,"SRMsg","ShowQuote",SRMSGDEFSET_SHOWQUOTE)) ShowWindow(GetDlgItem(hwndDlg,IDC_QUOTE),SW_HIDE);
	if(!DBGetContactSettingByte(NULL,"SRMsg","ShowReadNext",SRMSGDEFSET_SHOWREADNEXT)) ShowWindow(GetDlgItem(hwndDlg,IDC_READNEXT),SW_HIDE);
	if(isSplit || dat->isSend) {
		SetDlgItemText(hwndDlg,IDC_QUOTE,Translate("&Quote"));
		SetDlgItemText(hwndDlg,IDOK,Translate("&Send"));
	}
	else {
		SetDlgItemText(hwndDlg,IDC_QUOTE,Translate("Reply &Quoted"));
		SetDlgItemText(hwndDlg,IDOK,Translate("&Reply"));
	}
	ShowWindow(GetDlgItem(hwndDlg,IDC_SPLITTER),isSplit?SW_SHOW:SW_HIDE);
	if(!isSplit && !dat->isSend) {
		ShowWindow(GetDlgItem(hwndDlg,IDC_MULTISPLITTER),SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg,IDC_CLIST),SW_HIDE);
		dat->multiple=0;
		CheckDlgButton(hwndDlg,IDC_MULTIPLE,BST_UNCHECKED);
	}
	else {
		ShowWindow(GetDlgItem(hwndDlg,IDC_MULTISPLITTER),dat->multiple?SW_SHOW:SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg,IDC_CLIST),dat->multiple?SW_SHOW:SW_HIDE);
	}
	if(isSplit || dat->isSend) EnableWindow(GetDlgItem(hwndDlg,IDOK),GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE))!=0);
	else EnableWindow(GetDlgItem(hwndDlg,IDOK),TRUE);
	SendMessage(hwndDlg,DM_UPDATETITLE,0,0);
	SendMessage(hwndDlg,WM_SIZE,0,0);
	pl.length=sizeof(pl);
	GetWindowPlacement(hwndDlg,&pl);
	if(!IsWindowVisible(hwndDlg)) pl.showCmd=SW_HIDE;
	SetWindowPlacement(hwndDlg,&pl);	//in case size is smaller than new minimum
}

static void UpdateReadNextButtonText(HWND hwndBtn,HANDLE hDbEventViewing)
{
	int count;
	HANDLE hDbEvent;
	DBEVENTINFO dbei={0};
	char str[64];

	count=0;
	if(hDbEventViewing) {
		dbei.cbSize=sizeof(dbei);
		hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDNEXT,(WPARAM)hDbEventViewing,0);
		while(hDbEvent) {
			dbei.cbBlob=0;
			CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
			if(DbEventIsShown(&dbei)) count++;
			hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDNEXT,(WPARAM)hDbEvent,0);
		}
	}
	wsprintf(str,Translate("Read &Next (%u)"),count);
	SetWindowText(hwndBtn,str);
	EnableWindow(hwndBtn,count);
}

struct SavedMessageData {
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD keyStates;  //use MOD_ defines from RegisterHotKey()
};
struct MsgEditSubclassData {
	DWORD lastEnterTime;
	struct SavedMessageData *keyboardMsgQueue;
	int msgQueueCount;
};

static void SaveKeyboardMessage(struct MsgEditSubclassData *dat,UINT message,WPARAM wParam,LPARAM lParam)
{
	dat->keyboardMsgQueue=(struct SavedMessageData*)realloc(dat->keyboardMsgQueue,sizeof(struct SavedMessageData)*(dat->msgQueueCount+1));
	dat->keyboardMsgQueue[dat->msgQueueCount].message=message;
	dat->keyboardMsgQueue[dat->msgQueueCount].wParam=wParam;
	dat->keyboardMsgQueue[dat->msgQueueCount].lParam=lParam;
	dat->keyboardMsgQueue[dat->msgQueueCount].keyStates=(GetKeyState(VK_SHIFT)&0x8000?MOD_SHIFT:0)|(GetKeyState(VK_CONTROL)&0x8000?MOD_CONTROL:0)|(GetKeyState(VK_MENU)&0x8000?MOD_ALT:0);
	dat->msgQueueCount++;
}

#define EM_REPLAYSAVEDKEYSTROKES  (WM_USER+0x100)
#define EM_SUBCLASSED             (WM_USER+0x101)
#define EM_UNSUBCLASSED           (WM_USER+0x102)
#define ENTERCLICKTIME   1000    //max time in ms during which a double-tap on enter will cause a send
#define EDITMSGQUEUE_PASSTHRUCLIPBOARD  //if set the typing queue won't capture ctrl-C etc because people might want to use them on the read only text
						  //todo: decide if this should be set or not
static LRESULT CALLBACK MessageEditSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	struct MsgEditSubclassData *dat;

	dat=(struct MsgEditSubclassData*)GetWindowLong(hwnd,GWL_USERDATA);
	switch(msg) {
		case EM_SUBCLASSED:
			dat=(struct MsgEditSubclassData*)malloc(sizeof(struct MsgEditSubclassData));
			SetWindowLong(hwnd,GWL_USERDATA,(LONG)dat);
			dat->lastEnterTime=0;
			dat->keyboardMsgQueue=NULL;
			dat->msgQueueCount=0;
			return 0;
		case EM_SETREADONLY:
			if(wParam) {
				if(dat->keyboardMsgQueue) free(dat->keyboardMsgQueue);
				dat->keyboardMsgQueue=NULL;
				dat->msgQueueCount=0;
			}
			break;
		case EM_REPLAYSAVEDKEYSTROKES:
		{	int i;
			BYTE keyStateArray[256],originalKeyStateArray[256];
			MSG msg;

			while(PeekMessage(&msg,hwnd,0,0,PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			GetKeyboardState(originalKeyStateArray);
			GetKeyboardState(keyStateArray);
			for(i=0;i<dat->msgQueueCount;i++) {
				keyStateArray[VK_SHIFT]=dat->keyboardMsgQueue[i].keyStates&MOD_SHIFT?0x80:0;
				keyStateArray[VK_CONTROL]=dat->keyboardMsgQueue[i].keyStates&MOD_CONTROL?0x80:0;
				keyStateArray[VK_MENU]=dat->keyboardMsgQueue[i].keyStates&MOD_ALT?0x80:0;
				SetKeyboardState(keyStateArray);
				PostMessage(hwnd,dat->keyboardMsgQueue[i].message,dat->keyboardMsgQueue[i].wParam,dat->keyboardMsgQueue[i].lParam);
				while(PeekMessage(&msg,hwnd,0,0,PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			if(dat->keyboardMsgQueue) free(dat->keyboardMsgQueue);
			dat->keyboardMsgQueue=NULL;
			dat->msgQueueCount=0;
			SetKeyboardState(originalKeyStateArray);
			return 0;
		}
		case WM_CHAR:
			if(GetWindowLong(hwnd,GWL_STYLE)&ES_READONLY) break;
			//for saved msg queue the keyup/keydowns generate wm_chars themselves
			if(wParam=='\n' || wParam=='\r') {
				if(((GetKeyState(VK_CONTROL)&0x8000)!=0)^(0!=DBGetContactSettingByte(NULL,"SRMsg","SendOnEnter",SRMSGDEFSET_SENDONENTER))) {
					PostMessage(GetParent(hwnd),WM_COMMAND,IDOK,0);
					return 0;
				}
				if(DBGetContactSettingByte(NULL,"SRMsg","SendOnDblEnter",SRMSGDEFSET_SENDONDBLENTER)) {
					if(dat->lastEnterTime+ENTERCLICKTIME<GetTickCount()) dat->lastEnterTime=GetTickCount();
					else {
						SendMessage(hwnd,WM_CHAR,'\b',0);
						PostMessage(GetParent(hwnd),WM_COMMAND,IDOK,0);
						return 0;
					}
				}
			}
			else dat->lastEnterTime=0;
			if(wParam==1 && GetKeyState(VK_CONTROL)&0x8000) {	//ctrl-a
				SendMessage(hwnd,EM_SETSEL,0,-1);
				return 0;
			}
			if(wParam==127 && GetKeyState(VK_CONTROL)&0x8000) {	 //ctrl-backspace
				DWORD start,end;
				char *text;
				int textLen;
				SendMessage(hwnd,EM_GETSEL,(WPARAM)&end,(LPARAM)(PDWORD)NULL);
				SendMessage(hwnd,WM_KEYDOWN,VK_LEFT,0);
				SendMessage(hwnd,EM_GETSEL,(WPARAM)&start,(LPARAM)(PDWORD)NULL);
				textLen=GetWindowTextLength(hwnd);
				text=(char*)malloc(textLen+1);
				GetWindowText(hwnd,text,textLen+1);
				MoveMemory(text+start,text+end,textLen+1-end);
				SetWindowText(hwnd,text);
				free(text);
				SendMessage(hwnd,EM_SETSEL,start,start);
				SendMessage(GetParent(hwnd),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(hwnd),EN_CHANGE),(LPARAM)hwnd);
				return 0;
			}
			break;
		case WM_KEYUP:
			if(GetWindowLong(hwnd,GWL_STYLE)&ES_READONLY) {
				int i;
				//mustn't allow keyups for which there is no keydown
				for(i=0;i<dat->msgQueueCount;i++)
					if(dat->keyboardMsgQueue[i].message==WM_KEYDOWN && dat->keyboardMsgQueue[i].wParam==wParam)
						break;
				if(i==dat->msgQueueCount) break;
#ifdef EDITMSGQUEUE_PASSTHRUCLIPBOARD
				if(GetKeyState(VK_CONTROL)&0x8000) {
					if(wParam=='X' || wParam=='C' || wParam=='V' || wParam==VK_INSERT) break;
				}
				if(GetKeyState(VK_SHIFT)&0x8000) {
					if(wParam==VK_INSERT || wParam==VK_DELETE) break;
				}
#endif
				SaveKeyboardMessage(dat,msg,wParam,lParam);
				return 0;
			}
			break;
		case WM_KEYDOWN:
			if(GetWindowLong(hwnd,GWL_STYLE)&ES_READONLY) {
#ifdef EDITMSGQUEUE_PASSTHRUCLIPBOARD
				if(GetKeyState(VK_CONTROL)&0x8000) {
					if(wParam=='X' || wParam=='C' || wParam=='V' || wParam==VK_INSERT) break;
				}
				if(GetKeyState(VK_SHIFT)&0x8000) {
					if(wParam==VK_INSERT || wParam==VK_DELETE) break;
				}
#endif
				SaveKeyboardMessage(dat,msg,wParam,lParam);
				return 0;
			}
			if(wParam==VK_RETURN) break;
			//fall through
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_MOUSEWHEEL:
		case WM_KILLFOCUS:
			dat->lastEnterTime=0;
			break;
		case WM_SYSCHAR:
			dat->lastEnterTime=0;
			if((wParam=='s' || wParam=='S') && GetKeyState(VK_MENU)&0x8000) {
				if(GetWindowLong(hwnd,GWL_STYLE)&ES_READONLY)
					SaveKeyboardMessage(dat,msg,wParam,lParam);
				else
					PostMessage(GetParent(hwnd),WM_COMMAND,IDOK,0);
				return 0;
			}
			break;
		case EM_UNSUBCLASSED:
			if(dat->keyboardMsgQueue) free(dat->keyboardMsgQueue);
			free(dat);
			return 0;
	}
	return CallWindowProc(OldMessageEditProc,hwnd,msg,wParam,lParam);
}

static LRESULT CALLBACK SplitterSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) {
		case WM_NCHITTEST:
			return HTCLIENT;
		case WM_SETCURSOR:
		{	RECT rc;
			GetClientRect(hwnd,&rc);
			SetCursor(rc.right>rc.bottom?hCurSplitNS:hCurSplitWE);
			return TRUE;
		}
		case WM_LBUTTONDOWN:
			SetCapture(hwnd);
			return 0;
		case WM_MOUSEMOVE:
			if(GetCapture()==hwnd) {
				RECT rc;
				GetClientRect(hwnd,&rc);
				SendMessage(GetParent(hwnd),DM_SPLITTERMOVED,rc.right>rc.bottom?(short)HIWORD(GetMessagePos())+rc.bottom/2:(short)LOWORD(GetMessagePos())+rc.right/2,(LPARAM)hwnd);
			}
			return 0;
		case WM_LBUTTONUP:
			ReleaseCapture();
			return 0;
	}
	return CallWindowProc(OldSplitterProc,hwnd,msg,wParam,lParam);
}

static int MessageDialogResize(HWND hwndDlg,LPARAM lParam,UTILRESIZECONTROL *urc)
{
	struct MessageWindowData *dat=(struct MessageWindowData*)lParam;
	int showInfo,showButton,split;

	showInfo=DBGetContactSettingByte(NULL,"SRMsg","ShowInfoLine",SRMSGDEFSET_SHOWINFOLINE);
	showButton=DBGetContactSettingByte(NULL,"SRMsg","ShowButtonLine",SRMSGDEFSET_SHOWBUTTONLINE);
	split=DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT);
	if(!showInfo&&!showButton) {
		int i;
		for(i=0;i<sizeof(buttonLineControls)/sizeof(buttonLineControls[0]);i++)
			if(buttonLineControls[i]==urc->wId)
				OffsetRect(&urc->rcItem,0,-dat->lineHeight);
	}
	switch(urc->wId) {
		case IDC_NAME:
		{
			int len;
			HWND h;
			
			h = GetDlgItem(hwndDlg, IDC_NAME);
			len = GetWindowTextLength(h);
			if (len>0) {
				HDC hdc;
				SIZE textSize;
				RECT rc;
				char buf[256];

				GetWindowText(h, buf, sizeof(buf));
				GetClientRect(h, &rc);
				hdc = GetDC(h);
				SelectObject(hdc, (HFONT)SendMessage(h, WM_GETFONT, 0, 0));
				GetTextExtentPoint32(hdc,buf,lstrlen(buf),&textSize);
				urc->rcItem.right = urc->rcItem.left + textSize.cx;
				ReleaseDC(h,hdc);
			}
		}
		case IDC_PROTOCOL:
			return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
		case IDC_ADD:
		case IDC_USERMENU:
		case IDC_DETAILS:
		case IDC_HISTORY:
		case IDC_TIME:
		case IDC_MULTIPLE:
			if(dat->multiple) OffsetRect(&urc->rcItem,-dat->multiSplitterX,0);
			return RD_ANCHORX_RIGHT|RD_ANCHORY_TOP;
		case IDC_LOG:
			if(!showInfo&&!showButton) urc->rcItem.top-=dat->lineHeight;
			if(dat->multiple) urc->rcItem.right-=dat->multiSplitterX;
			if(!DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT))
				return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
			urc->rcItem.bottom-=dat->splitterY-dat->originalSplitterY;
			return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
		case IDC_SPLITTER:
			if(dat->multiple) urc->rcItem.right-=dat->multiSplitterX;
			urc->rcItem.top-=dat->splitterY-dat->originalSplitterY;
			urc->rcItem.bottom-=dat->splitterY-dat->originalSplitterY;
			return RD_ANCHORX_WIDTH|RD_ANCHORY_BOTTOM;
		case IDC_ST_ENTERMSG:
			if(!split && dat->isSend) {
				if(!showInfo&&!showButton) OffsetRect(&urc->rcItem,0,-dat->lineHeight);
				return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
			}
			urc->rcItem.top-=dat->splitterY-dat->originalSplitterY;
			urc->rcItem.bottom-=dat->splitterY-dat->originalSplitterY;
			return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
		case IDC_MESSAGE:
			if(!split) {
				if(!showInfo&&!showButton) urc->rcItem.top-=dat->lineHeight;
			}
			if(dat->multiple) urc->rcItem.right-=dat->multiSplitterX;
			if(!split)
				return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
			urc->rcItem.top-=dat->splitterY-dat->originalSplitterY;
			return RD_ANCHORX_WIDTH|RD_ANCHORY_BOTTOM;
		case IDC_QUOTE:
			return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
		case IDCANCEL:
		case IDOK:
			if(dat->multiple) {
				urc->rcItem.left-=dat->multiSplitterX/2;
				urc->rcItem.right-=dat->multiSplitterX/2;
			}
			return RD_ANCHORX_CENTRE|RD_ANCHORY_BOTTOM;
		case IDC_READNEXT:
			if(dat->multiple) OffsetRect(&urc->rcItem,-dat->multiSplitterX,0);
			return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
		case IDC_MULTISPLITTER:
			urc->rcItem.left-=dat->multiSplitterX;
			urc->rcItem.right-=dat->multiSplitterX;
			return RD_ANCHORX_RIGHT|RD_ANCHORY_HEIGHT;
		case IDC_CLIST:
			urc->rcItem.left=urc->dlgNewSize.cx-dat->multiSplitterX;
			urc->rcItem.right=urc->dlgNewSize.cx-8;
			return RD_ANCHORX_CUSTOM|RD_ANCHORY_HEIGHT;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static void UpdateReadChars(HWND hwndDlg) {
	char buf[128];
	char tmp[64];

	int len=GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
	_snprintf(tmp, sizeof(tmp), Translate("%d characters typed"), len);
	_snprintf(buf, sizeof(buf), "%s  %s", Translate("Enter message:"), len==0?"":tmp);
	SetDlgItemText(hwndDlg, IDC_ST_ENTERMSG, buf);
}

BOOL CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct MessageWindowData *dat;

	dat=(struct MessageWindowData*)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			struct NewMessageWindowLParam *newData=(struct NewMessageWindowLParam*)lParam;
			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
			dat=(struct MessageWindowData*)malloc(sizeof(struct MessageWindowData));
			SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)dat);
			{
				dat->hContact=newData->hContact;
				dat->isSend=newData->isSend;
				if(newData->szInitialText) {
					int len;
					SetDlgItemText(hwndDlg,IDC_MESSAGE,newData->szInitialText);
					len=GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
					PostMessage(GetDlgItem(hwndDlg,IDC_MESSAGE),EM_SETSEL,len,len);
				}
			}			
			dat->hAckEvent=NULL;
			dat->sendInfo=NULL;
			if(dat->isSend) dat->hNewEvent=NULL;
			else dat->hNewEvent=HookEventMessage(ME_DB_EVENT_ADDED,hwndDlg,HM_DBEVENTADDED);
			dat->hBkgBrush=NULL;
			dat->hDbEventFirst=NULL;
			dat->sendBuffer=NULL;
			dat->splitterY=(int)DBGetContactSettingDword(NULL,"SRMsg","splitsplity",(DWORD)-1);
			dat->multiSplitterX=(int)DBGetContactSettingDword(NULL,"SRMsg","multisplit",150);
			dat->multiple=0;
			dat->windowWasCascaded=0;
			dat->nFlash=0;
			dat->nFlashMax=DBGetContactSettingByte(NULL,"SRMsg","FlashMax",4);
			ShowWindow(GetDlgItem(hwndDlg,IDC_MULTISPLITTER),SW_HIDE);
			ShowWindow(GetDlgItem(hwndDlg,IDC_CLIST),SW_HIDE);
			{
				RECT rc;
				POINT pt;
				GetWindowRect(GetDlgItem(hwndDlg,IDC_SPLITTER),&rc);
				pt.y=(rc.top+rc.bottom)/2; pt.x=0;
				ScreenToClient(hwndDlg,&pt);
				dat->originalSplitterY=pt.y;
				if(dat->splitterY==-1) dat->splitterY=dat->originalSplitterY+60;
			}
			{	RECT rc;
				int isSplit=DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT);
				GetWindowRect(GetDlgItem(hwndDlg,IDC_MESSAGE),&rc);
				dat->minEditBoxSize.cx=rc.right-rc.left;
				dat->minEditBoxSize.cy=rc.bottom-rc.top;

				GetWindowRect(GetDlgItem(hwndDlg,IDC_ADD),&rc);
				dat->lineHeight=rc.bottom-rc.top+3;				
			}

			WindowList_Add(dat->isSend?hMessageSendList:hMessageWindowList,hwndDlg,dat->hContact);

			dat->hIcons[0]=(HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ADDCONTACT),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
			dat->hIcons[1]=(HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_USERDETAILS),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
			dat->hIcons[2]=(HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_HISTORY),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
			dat->hIcons[3]=(HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_TIMESTAMP),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
			dat->hIcons[4]=(HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MULTISEND),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
			dat->hIcons[5]=(HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_DOWNARROW),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
			SendDlgItemMessage(hwndDlg,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)dat->hIcons[0]);
			SendDlgItemMessage(hwndDlg,IDC_DETAILS,BM_SETIMAGE,IMAGE_ICON,(LPARAM)dat->hIcons[1]);
			SendDlgItemMessage(hwndDlg,IDC_HISTORY,BM_SETIMAGE,IMAGE_ICON,(LPARAM)dat->hIcons[2]);
			SendDlgItemMessage(hwndDlg,IDC_TIME,BM_SETIMAGE,IMAGE_ICON,(LPARAM)dat->hIcons[3]);
			SendDlgItemMessage(hwndDlg,IDC_MULTIPLE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)dat->hIcons[4]);
			SendDlgItemMessage(hwndDlg,IDC_USERMENU,BM_SETIMAGE,IMAGE_ICON,(LPARAM)dat->hIcons[5]);
			// Make them pushbuttons
			SendDlgItemMessage(hwndDlg,IDC_TIME,BUTTONSETASPUSHBTN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_MULTIPLE,BUTTONSETASPUSHBTN,0,0);
			dat->hwndToolTips=CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,"",WS_POPUP,0,0,0,0,NULL,NULL,GetModuleHandle(NULL),NULL);
			{	TOOLINFO ti;
				ZeroMemory(&ti,sizeof(ti));
				ti.cbSize=sizeof(ti);
				ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS;
				ti.uId=(UINT)GetDlgItem(hwndDlg,IDC_USERMENU);
				ti.lpszText=Translate("User Menu");
				SendMessage(dat->hwndToolTips,TTM_ADDTOOL,0,(LPARAM)&ti);
				ti.uId=(UINT)GetDlgItem(hwndDlg,IDC_ADD);
				ti.lpszText=Translate("Add Contact Permanently to List");
				SendMessage(dat->hwndToolTips,TTM_ADDTOOL,0,(LPARAM)&ti);
				ti.uId=(UINT)GetDlgItem(hwndDlg,IDC_DETAILS);
				ti.lpszText=Translate("View User's Details");
				SendMessage(dat->hwndToolTips,TTM_ADDTOOL,0,(LPARAM)&ti);
				ti.uId=(UINT)GetDlgItem(hwndDlg,IDC_HISTORY);
				ti.lpszText=Translate("View User's History");
				SendMessage(dat->hwndToolTips,TTM_ADDTOOL,0,(LPARAM)&ti);
				ti.uId=(UINT)GetDlgItem(hwndDlg,IDC_TIME);
				ti.lpszText=Translate("Display Time in Message Log");
				SendMessage(dat->hwndToolTips,TTM_ADDTOOL,0,(LPARAM)&ti);
				ti.uId=(UINT)GetDlgItem(hwndDlg,IDC_MULTIPLE);
				ti.lpszText=Translate("Send Message to Multiple Users");
				SendMessage(dat->hwndToolTips,TTM_ADDTOOL,0,(LPARAM)&ti);
			}

			SendDlgItemMessage(hwndDlg,IDC_LOG,EM_SETOLECALLBACK,0,(LPARAM)&reOleCallback);
			SendDlgItemMessage(hwndDlg,IDC_LOG,EM_SETEVENTMASK,0,ENM_MOUSEEVENTS|ENM_LINK);
			if (dat->hContact) {				
				char *szProto;
				szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)dat->hContact,0);
				if (szProto) {
					int nMax;
					nMax=CallProtoService(szProto,PS_GETCAPS,PFLAG_MAXLENOFMESSAGE,(LPARAM)dat->hContact);
					if (nMax) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_LIMITTEXT, (WPARAM)nMax, 0);
				}			
			}
			OldMessageEditProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_MESSAGE),GWL_WNDPROC,(LONG)MessageEditSubclassProc);
			SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_SUBCLASSED,0,0);
			if(dat->hContact) {
				HANDLE hItem;
				hItem=(HANDLE)SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_FINDCONTACT,(WPARAM)dat->hContact,0);
				if(hItem) SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_SETCHECKMARK,(WPARAM)hItem,1);
			}

			OldSplitterProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_SPLITTER),GWL_WNDPROC,(LONG)SplitterSubclassProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_MULTISPLITTER),GWL_WNDPROC,(LONG)SplitterSubclassProc);

			if(!dat->isSend && dat->hContact) {
				// This finds the first message to display, it works like shit
				dat->hDbEventFirst=(HANDLE)CallService(MS_DB_EVENT_FINDFIRSTUNREAD,(WPARAM)dat->hContact,0);
				switch(DBGetContactSettingByte(NULL,"SRMsg","LoadHistory",SRMSGDEFSET_LOADHISTORY)) {
					case LOADHISTORY_COUNT:
					{	int i;
						HANDLE hPrevEvent;
						DBEVENTINFO dbei={0};
						dbei.cbSize=sizeof(dbei);
						for(i=DBGetContactSettingWord(NULL,"SRMsg","LoadCount",SRMSGDEFSET_LOADCOUNT);i>0;i--) {
							if(dat->hDbEventFirst==NULL) hPrevEvent=(HANDLE)CallService(MS_DB_EVENT_FINDLAST,(WPARAM)dat->hContact,0);
							else hPrevEvent=(HANDLE)CallService(MS_DB_EVENT_FINDPREV,(WPARAM)dat->hDbEventFirst,0);
							if(hPrevEvent==NULL) break;
							dbei.cbBlob=0;
							dat->hDbEventFirst=hPrevEvent;
							CallService(MS_DB_EVENT_GET,(WPARAM)dat->hDbEventFirst,(LPARAM)&dbei);
							if(!DbEventIsShown(&dbei)) i++;
						}
						break;
					}
					case LOADHISTORY_TIME:
					{	HANDLE hPrevEvent;
						DBEVENTINFO dbei={0};
						DWORD firstTime;

						dbei.cbSize=sizeof(dbei);
						if(dat->hDbEventFirst==NULL) dbei.timestamp=time(NULL);
						else CallService(MS_DB_EVENT_GET,(WPARAM)dat->hDbEventFirst,(LPARAM)&dbei);
						firstTime=dbei.timestamp-60*DBGetContactSettingWord(NULL,"SRMsg","LoadTime",SRMSGDEFSET_LOADTIME);
						for(;;) {
							if(dat->hDbEventFirst==NULL) hPrevEvent=(HANDLE)CallService(MS_DB_EVENT_FINDLAST,(WPARAM)dat->hContact,0);
							else hPrevEvent=(HANDLE)CallService(MS_DB_EVENT_FINDPREV,(WPARAM)dat->hDbEventFirst,0);
							if(hPrevEvent==NULL) break;
							dbei.cbBlob=0;
							CallService(MS_DB_EVENT_GET,(WPARAM)hPrevEvent,(LPARAM)&dbei);
							if(dbei.timestamp<firstTime) break;
							dat->hDbEventFirst=hPrevEvent;
						}
						break;
					}
				}
			}
			SendMessage(hwndDlg,DM_OPTIONSAPPLIED,0,0);

			{	int savePerContact=DBGetContactSettingByte(NULL,"SRMsg","SavePerContact",SRMSGDEFSET_SAVEPERCONTACT);
				if(Utils_RestoreWindowPosition(hwndDlg,savePerContact?dat->hContact:NULL,"SRMsg",dat->isSend?"send":"split")) {
					if(savePerContact) {
						if(Utils_RestoreWindowPositionNoMove(hwndDlg,NULL,"SRMsg",dat->isSend?"send":"split"))
							SetWindowPos(hwndDlg,0,0,0,450,300,SWP_NOZORDER|SWP_NOMOVE);
					}
					else SetWindowPos(hwndDlg,0,0,0,450,300,SWP_NOZORDER|SWP_NOMOVE);
				}
				if(!savePerContact && DBGetContactSettingByte(NULL,"SRMsg","Cascade",SRMSGDEFSET_CASCADE))
					WindowList_Broadcast(dat->isSend?hMessageSendList:hMessageWindowList,DM_CASCADENEWWINDOW,(WPARAM)hwndDlg,(LPARAM)&dat->windowWasCascaded);
			}
			if(dat->hContact==NULL) {
				CheckDlgButton(hwndDlg,IDC_MULTIPLE,BST_CHECKED);
				SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_MULTIPLE,BN_CLICKED),0);
			}
			ShowWindow(hwndDlg,SW_SHOWNORMAL);
			return TRUE;
		}
		case DM_OPTIONSAPPLIED:
			if(dat->isSend && DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT)) {
				DestroyWindow(hwndDlg);
				break;
			}
			SetDialogToType(hwndDlg);
			if(dat->hBkgBrush) DeleteObject(dat->hBkgBrush);
			{	COLORREF colour=DBGetContactSettingDword(NULL,"SRMsg","BkgColour",SRMSGDEFSET_BKGCOLOUR);
				dat->hBkgBrush=CreateSolidBrush(colour);
				SendDlgItemMessage(hwndDlg,IDC_LOG,EM_SETBKGNDCOLOR,0,colour);
			}
			dat->showTime=DBGetContactSettingByte(NULL,"SRMsg","ShowTime",SRMSGDEFSET_SHOWTIME);
			SendDlgItemMessage(hwndDlg,IDC_TIME,BUTTONSETASPUSHBTN,0,0);
			CheckDlgButton(hwndDlg,IDC_TIME,dat->showTime?BST_CHECKED:BST_UNCHECKED);
			InvalidateRect(GetDlgItem(hwndDlg,IDC_MESSAGE),NULL,FALSE);
			{	HFONT hFont;
				LOGFONT lf;
				hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_MESSAGE,WM_GETFONT,0,0);
				if(hFont!=NULL && hFont!=(HFONT)SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0)) DeleteObject(hFont);
				LoadMsgDlgFont(MSGFONTID_MYMSG,&lf,NULL);
				hFont=CreateFontIndirect(&lf);
				SendDlgItemMessage(hwndDlg,IDC_MESSAGE,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
			}
			SendMessage(hwndDlg,DM_REMAKELOG,0,0);
			break;
		case DM_UPDATETITLE:
		{	char newtitle[256],oldtitle[256];
			char *szStatus,*contactName,*pszNewTitleEnd;
			char *szProto;
			DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *)wParam;

			if(DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT))
				pszNewTitleEnd="Message Session";
			else {
				if(dat->isSend) pszNewTitleEnd="Send Message";
				else pszNewTitleEnd="Message Received";
			}
			if(dat->hContact) {
				szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)dat->hContact,0);
				if (szProto) {
					CONTACTINFO ci;
					int hasName = 0;
					char buf[128], buf2[128];

					contactName=(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)dat->hContact,0);
					ZeroMemory(&ci,sizeof(ci));
					ci.cbSize = sizeof(ci);
					ci.hContact = dat->hContact;
					ci.szProto = szProto;
					ci.dwFlag = CNF_UNIQUEID;
					if (!CallService(MS_CONTACT_GETCONTACTINFO,0,(LPARAM)&ci)) {
						switch(ci.type) {
							case CNFT_ASCIIZ:
								hasName = 1;
								if (stricmp(contactName, ci.pszVal)) 
									_snprintf(buf, sizeof(buf), "%s (%s)", contactName, ci.pszVal);
								else
									_snprintf(buf, sizeof(buf), "%s", ci.pszVal);
								free(ci.pszVal);
								break;
							case CNFT_DWORD:
								hasName = 1;
								_snprintf(buf2, sizeof(buf2),"%u",ci.dVal);
								if (strcmp(contactName, buf2)) 
									_snprintf(buf, sizeof(buf), "%s (%s)", contactName, buf2);
								else
									_snprintf(buf, sizeof(buf), "%s", buf2);
								break;
						}
					}
					SetDlgItemText(hwndDlg,IDC_NAME,hasName?buf:contactName);
					szStatus=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord(dat->hContact,szProto,"Status",ID_STATUS_OFFLINE),0);
					_snprintf(newtitle,sizeof(newtitle),"%s (%s): %s",contactName,szStatus,Translate(pszNewTitleEnd));
					if (!cws || (!strcmp(cws->szModule,szProto) && !strcmp(cws->szSetting, "Status"))) 
						InvalidateRect(GetDlgItem(hwndDlg,IDC_PROTOCOL),NULL,TRUE);
				}
			}
			else lstrcpyn(newtitle,pszNewTitleEnd,sizeof(newtitle));
			GetWindowText(hwndDlg,oldtitle,sizeof(oldtitle));
			if(lstrcmp(newtitle,oldtitle)) {   //swt() flickers even if the title hasn't actually changed
				SetWindowText(hwndDlg,newtitle);
				SendMessage(hwndDlg,WM_SIZE,0,0);
			}
			break;
		}
		case DM_CASCADENEWWINDOW:
			if((HWND)wParam==hwndDlg) break;
			{	RECT rcThis,rcNew;
				GetWindowRect(hwndDlg,&rcThis);
				GetWindowRect((HWND)wParam,&rcNew);
				if(abs(rcThis.left-rcNew.left)<3 && abs(rcThis.top-rcNew.top)<3) {
					int offset=GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFRAME);
					SetWindowPos((HWND)wParam,0,rcNew.left+offset,rcNew.top+offset,0,0,SWP_NOZORDER|SWP_NOSIZE);
					*(int*)lParam=1;
				}
			}
			break;
		case WM_SETFOCUS:
			if(DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT) || !dat->isSend)
				SetFocus(GetDlgItem(hwndDlg,IDC_MESSAGE));
			else
				SetFocus(GetDlgItem(hwndDlg,IDOK));
			break;
		case WM_ACTIVATE:
			if(LOWORD(wParam)!=WA_ACTIVE) break;
			//fall through
		case WM_MOUSEACTIVATE:
			if(KillTimer(hwndDlg,TIMERID_FLASHWND))
				FlashWindow(hwndDlg,FALSE);
			break;
		case WM_GETMINMAXINFO:
		{	MINMAXINFO *mmi=(MINMAXINFO*)lParam;
			RECT rcWindow,rcLog;
			GetWindowRect(hwndDlg,&rcWindow);
			if(dat->isSend) GetWindowRect(GetDlgItem(hwndDlg,IDC_MESSAGE),&rcLog);
			else GetWindowRect(GetDlgItem(hwndDlg,IDC_LOG),&rcLog);
			mmi->ptMinTrackSize.x=rcWindow.right-rcWindow.left-((rcLog.right-rcLog.left)-dat->minEditBoxSize.cx);
			mmi->ptMinTrackSize.y=rcWindow.bottom-rcWindow.top-((rcLog.bottom-rcLog.top)-dat->minEditBoxSize.cy);
			return 0;
		}
		case WM_SIZE:
		{	UTILRESIZEDIALOG urd;
			if(IsIconic(hwndDlg)) break;
			ZeroMemory(&urd,sizeof(urd));
			urd.cbSize=sizeof(urd);
			urd.hInstance=GetModuleHandle(NULL);
			urd.hwndDlg=hwndDlg;
			urd.lParam=(LPARAM)dat;
			if(DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT))
				urd.lpTemplate=MAKEINTRESOURCE(IDD_MSGSPLIT);
			else if(dat->isSend)
				urd.lpTemplate=MAKEINTRESOURCE(IDD_MSGSEND);
			else
				urd.lpTemplate=MAKEINTRESOURCE(IDD_MSGSINGLE);
			urd.pfnResizer=MessageDialogResize;
			CallService(MS_UTILS_RESIZEDIALOG,0,(LPARAM)&urd);
			break;
		}
		case DM_SPLITTERMOVED:
		{	POINT pt;
			RECT rc;
			RECT rcLog;

			if(DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT))
				GetWindowRect(GetDlgItem(hwndDlg,IDC_LOG),&rcLog);
			else
				GetWindowRect(GetDlgItem(hwndDlg,IDC_MESSAGE),&rcLog);
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_MULTISPLITTER)) {
				int oldSplitterX;
				GetClientRect(hwndDlg,&rc);
				pt.x=wParam; pt.y=0;
				ScreenToClient(hwndDlg,&pt);

				oldSplitterX=dat->multiSplitterX;
				dat->multiSplitterX=rc.right-pt.x;
				if(dat->multiSplitterX<25) dat->multiSplitterX=25;
				if(rcLog.right-rcLog.left-dat->multiSplitterX+oldSplitterX<dat->minEditBoxSize.cx)
					dat->multiSplitterX=oldSplitterX-dat->minEditBoxSize.cx+(rcLog.right-rcLog.left);
			}
			else if((HWND)lParam==GetDlgItem(hwndDlg,IDC_SPLITTER)) {
				int oldSplitterY;
				GetClientRect(hwndDlg,&rc);
				pt.x=0; pt.y=wParam;
				ScreenToClient(hwndDlg,&pt);

				oldSplitterY=dat->splitterY;
				dat->splitterY=rc.bottom-pt.y+3;
				GetWindowRect(GetDlgItem(hwndDlg,IDC_MESSAGE),&rc);
				if(rc.bottom-rc.top+(dat->splitterY-oldSplitterY)<dat->minEditBoxSize.cy)
					dat->splitterY=oldSplitterY+dat->minEditBoxSize.cy-(rc.bottom-rc.top);
				if(rcLog.bottom-rcLog.top-(dat->splitterY-oldSplitterY)<dat->minEditBoxSize.cy)
					dat->splitterY=oldSplitterY-dat->minEditBoxSize.cy+(rcLog.bottom-rcLog.top);
			}
			SendMessage(hwndDlg,WM_SIZE,0,0);
			break;
		}
		case DM_REMAKELOG:
			if(dat->isSend) break;
			if(DBGetContactSettingByte(NULL,"SRMsg","ShowReadNext",SRMSGDEFSET_SHOWREADNEXT)) {
				StreamInEvents(hwndDlg,dat->hDbEventFirst,1,0);
				UpdateReadNextButtonText(GetDlgItem(hwndDlg,IDC_READNEXT),dat->hDbEventFirst);
			}
			else StreamInEvents(hwndDlg,dat->hDbEventFirst,-1,0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_QUOTE),dat->hDbEventLast!=NULL);
			break;
		case DM_APPENDTOLOG:   //takes wParam=hDbEvent
			if(dat->isSend) break;
			if(DBGetContactSettingByte(NULL,"SRMsg","ShowReadNext",SRMSGDEFSET_SHOWREADNEXT))
				UpdateReadNextButtonText(GetDlgItem(hwndDlg,IDC_READNEXT),dat->hDbEventFirst);
			else StreamInEvents(hwndDlg,(HANDLE)wParam,1,1);
			EnableWindow(GetDlgItem(hwndDlg,IDC_QUOTE),dat->hDbEventLast!=NULL);
			break;
		case DM_SCROLLLOGTOBOTTOM:
		{	SCROLLINFO si={0};
			if((GetWindowLong(GetDlgItem(hwndDlg,IDC_LOG),GWL_STYLE)&WS_VSCROLL)==0) break;
			si.cbSize=sizeof(si);
			si.fMask=SIF_PAGE|SIF_RANGE;
			GetScrollInfo(GetDlgItem(hwndDlg,IDC_LOG),SB_VERT,&si);
			si.fMask=SIF_POS;
			si.nPos=si.nMax-si.nPage+1;
			SetScrollInfo(GetDlgItem(hwndDlg,IDC_LOG),SB_VERT,&si,TRUE);
			PostMessage(GetDlgItem(hwndDlg,IDC_LOG),WM_VSCROLL,MAKEWPARAM(SB_THUMBPOSITION,si.nPos),(LPARAM)(HWND)NULL);
			break;
		}
		case HM_DBEVENTADDED:
			if((HANDLE)wParam!=dat->hContact) break;
			if(dat->hContact==NULL) break;
		{	DBEVENTINFO dbei={0};

			dbei.cbSize=sizeof(dbei);
			dbei.cbBlob=0;
			CallService(MS_DB_EVENT_GET,lParam,(LPARAM)&dbei);
			if(dat->hDbEventFirst==NULL) dat->hDbEventFirst=(HANDLE)lParam;
			if(dbei.flags&DBEF_READ) break;
			if(DbEventIsShown(&dbei)) {
				if((HANDLE)lParam!=dat->hDbEventFirst && (HANDLE)CallService(MS_DB_EVENT_FINDNEXT,lParam,0)==NULL)
					SendMessage(hwndDlg,DM_APPENDTOLOG,lParam,0);
				else SendMessage(hwndDlg,DM_REMAKELOG,0,0);
				if((GetActiveWindow()!=hwndDlg || GetForegroundWindow() != hwndDlg) && !(dbei.flags&DBEF_SENT)) 
				{
					SetTimer(hwndDlg,TIMERID_FLASHWND,TIMEOUT_FLASHWND,NULL);
				} //if
			}
			break;
		}
		case WM_TIMER:
			if(wParam==TIMERID_MSGSEND) {
				KillTimer(hwndDlg,wParam);
				ShowWindow(hwndDlg,SW_SHOWNORMAL);
				EnableWindow(hwndDlg,FALSE);
				CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_MSGSENDERROR),hwndDlg,ErrorDlgProc,(LPARAM)Translate("The message send timed out."));
			}
			else if(wParam==TIMERID_FLASHWND) {
				FlashWindow(hwndDlg,TRUE);
				if (dat->nFlash > dat->nFlashMax) {
					KillTimer(hwndDlg,TIMERID_FLASHWND);
					FlashWindow(hwndDlg,FALSE);
					dat->nFlash=0;
				} 
				dat->nFlash++;
			}
			else if(wParam==TIMERID_ANTIBOMB) {
				int sentSoFar=0,i;
				KillTimer(hwndDlg,wParam);
				for(i=0;i<dat->sendCount;i++) {
					if(dat->sendInfo[i].hContact==NULL) continue;
					if(sentSoFar>=ANTIBOMB_COUNT) {
						KillTimer(hwndDlg,TIMERID_MSGSEND);
						SetTimer(hwndDlg,TIMERID_MSGSEND,DBGetContactSettingDword(NULL,"SRMsg","MessageTimeout",TIMEOUT_MSGSEND),NULL);
						SetTimer(hwndDlg,TIMERID_ANTIBOMB,TIMEOUT_ANTIBOMB,NULL);
						break;
					}
					dat->sendInfo[i].hSendId=(HANDLE)CallContactService(dat->sendInfo[i].hContact,PSS_MESSAGE,0,(LPARAM)dat->sendBuffer);
					sentSoFar++;
				}
			}
			break;
		case DM_ERRORDECIDED:
			EnableWindow(hwndDlg,TRUE);
			switch(wParam) {
				case MSGERROR_CANCEL:
					EnableWindow(GetDlgItem(hwndDlg,IDOK),TRUE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_CLIST),TRUE);
					SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_SETREADONLY,FALSE,0);
					SetFocus(GetDlgItem(hwndDlg,IDC_MESSAGE));
					if(dat->hAckEvent) {
						UnhookEvent(dat->hAckEvent);
						dat->hAckEvent=NULL;
					}
					break;
				case MSGERROR_RETRY:
					{	int i;
						for(i=0;i<dat->sendCount;i++) {
							if(dat->sendInfo[i].hSendId==NULL && dat->sendInfo[i].hContact==NULL) continue;
							dat->sendInfo[i].hSendId=(HANDLE)CallContactService(dat->sendInfo[i].hContact,PSS_MESSAGE,0,(LPARAM)dat->sendBuffer);
						}
					}
					SetTimer(hwndDlg,TIMERID_MSGSEND,DBGetContactSettingDword(NULL,"SRMsg","MessageTimeout",TIMEOUT_MSGSEND),NULL);
					break;
			}
			break;
		case WM_CTLCOLOREDIT:
		{	COLORREF colour;
			if((HWND)lParam!=GetDlgItem(hwndDlg,IDC_MESSAGE)) break;
			LoadMsgDlgFont(MSGFONTID_MYMSG,NULL,&colour);
			SetTextColor((HDC)wParam,colour);
			SetBkColor((HDC)wParam,DBGetContactSettingDword(NULL,"SRMsg","BkgColour",SRMSGDEFSET_BKGCOLOUR));
			return (BOOL)dat->hBkgBrush;
		}
		case WM_MEASUREITEM:
			return CallService(MS_CLIST_MENUMEASUREITEM,wParam,lParam);
		case WM_DRAWITEM:
		{	LPDRAWITEMSTRUCT dis=(LPDRAWITEMSTRUCT)lParam;
			if(dis->hwndItem==GetDlgItem(hwndDlg, IDC_PROTOCOL)) {
				char *szProto;

				szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)dat->hContact,0);
				if (szProto) {
					HICON hIcon;
					int dwStatus;

					dwStatus=DBGetContactSettingWord(dat->hContact,szProto,"Status",ID_STATUS_OFFLINE);
					hIcon=LoadSkinnedProtoIcon(szProto,dwStatus);
					if (hIcon) DrawIconEx(dis->hDC,dis->rcItem.left,dis->rcItem.top,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
				}
			}
			return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);
		}
		case WM_COMMAND:
			if(CallService(MS_CLIST_MENUPROCESSCOMMAND,MAKEWPARAM(LOWORD(wParam),MPCF_CONTACTMENU),(LPARAM)dat->hContact))
				break;
			switch (LOWORD(wParam))
			{
				case IDOK:
					if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDOK))) break;
					if(!DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT) && !dat->isSend) {
						//this is a 'reply' button
						HWND hwnd;
						struct NewMessageWindowLParam newData={0};

						if(hwnd=WindowList_Find(hMessageSendList,dat->hContact)) {
							SetForegroundWindow(hwnd);
							SetFocus(hwnd);
							ShowWindow(hwnd,SW_SHOWNORMAL);
						}
						else {
							newData.hContact=dat->hContact;
							newData.isSend=1;
							CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_MSGSPLIT),NULL,DlgProcMessage,(LPARAM)&newData);
						}
						if(DBGetContactSettingByte(NULL,"SRMsg","CloseOnReply",SRMSGDEFSET_CLOSEONREPLY) && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_READNEXT)))
							DestroyWindow(hwndDlg);
						break;
					}
					else {
						//this is a 'send' button
						int bufSize;

						bufSize=GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE))+1;
						dat->sendBuffer=(char*)realloc(dat->sendBuffer,bufSize);
						GetDlgItemText(hwndDlg,IDC_MESSAGE,dat->sendBuffer,bufSize);
						if(dat->sendBuffer[0]==0) break;
						
						dat->hAckEvent=HookEventMessage(ME_PROTO_ACK,hwndDlg,HM_EVENTSENT);
						if(dat->multiple) {
							HANDLE hContact,hItem;
							int sentSoFar=0;
							dat->sendCount=0;
							hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
							do {
								hItem=(HANDLE)SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_FINDCONTACT,(WPARAM)hContact,0);
								if(hItem && SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_GETCHECKMARK,(WPARAM)hItem,0)) {
									dat->sendCount++;
									dat->sendInfo=(struct MessageSendInfo*)realloc(dat->sendInfo,sizeof(struct MessageSendInfo)*dat->sendCount);
									dat->sendInfo[dat->sendCount-1].hContact=hContact;
									if(sentSoFar<ANTIBOMB_COUNT) {
										dat->sendInfo[dat->sendCount-1].hSendId=(HANDLE)CallContactService(hContact,PSS_MESSAGE,0,(LPARAM)dat->sendBuffer);
										sentSoFar++;
									}
									else dat->sendInfo[dat->sendCount-1].hSendId=NULL;
								}
							} while(hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0));
							if(dat->sendCount==0) {
								MessageBox(hwndDlg,Translate("You haven't selected any contacts from the list. Click the white box next to a name to send the message to that person."),Translate("Send Message"),MB_OK);
								break;
							}
							if(sentSoFar<dat->sendCount) SetTimer(hwndDlg,TIMERID_ANTIBOMB,TIMEOUT_ANTIBOMB,NULL);
						}
						else {
							if(dat->hContact==NULL) break;	//never happens
							dat->sendCount=1;
							dat->sendInfo=(struct MessageSendInfo*)realloc(dat->sendInfo,sizeof(struct MessageSendInfo)*dat->sendCount);
							dat->sendInfo[0].hSendId=(HANDLE)CallContactService(dat->hContact,PSS_MESSAGE,0,(LPARAM)dat->sendBuffer);
							dat->sendInfo[0].hContact=dat->hContact;
						}

						EnableWindow(GetDlgItem(hwndDlg,IDOK),FALSE);
						EnableWindow(GetDlgItem(hwndDlg,IDC_CLIST),FALSE);
						SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_SETREADONLY,TRUE,0);

						//create a timeout timer
						SetTimer(hwndDlg,TIMERID_MSGSEND,DBGetContactSettingDword(NULL,"SRMsg","MessageTimeout",TIMEOUT_MSGSEND),NULL);
						if(DBGetContactSettingByte(NULL,"SRMsg","AutoMin",SRMSGDEFSET_AUTOMIN)) ShowWindow(hwndDlg,SW_MINIMIZE);
					}
					return TRUE;
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					return TRUE;
				case IDC_QUOTE:
				{	CHARRANGE sel;
					char *szText,*szQuoted;

					if(dat->hDbEventLast==NULL) break;
					SendDlgItemMessage(hwndDlg,IDC_LOG,EM_EXGETSEL,0,(LPARAM)&sel);
					if(sel.cpMin==sel.cpMax) {
						DBEVENTINFO dbei={0};
						int iDescr;
						dbei.cbSize=sizeof(dbei);
						dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)dat->hDbEventLast,0);
						szText=(char*)malloc(dbei.cbBlob+1);   //URLs are made one char bigger for crlf
						dbei.pBlob=szText;
						CallService(MS_DB_EVENT_GET,(WPARAM)dat->hDbEventLast,(LPARAM)&dbei);
						if(dbei.eventType==EVENTTYPE_URL) {
							iDescr=lstrlen(szText);
							MoveMemory(szText+iDescr+2,szText+iDescr+1,dbei.cbBlob-iDescr-1);
							szText[iDescr]='\r'; szText[iDescr+1]='\n';
						}
						if(dbei.eventType==EVENTTYPE_FILE) {
							iDescr=lstrlen(szText+sizeof(DWORD));
							MoveMemory(szText,szText+sizeof(DWORD),iDescr);
							MoveMemory(szText+iDescr+2,szText+sizeof(DWORD)+iDescr,dbei.cbBlob-iDescr-sizeof(DWORD)-1);
							szText[iDescr]='\r'; szText[iDescr+1]='\n';
						}
					}
					else {
						szText=(char*)malloc(sel.cpMax-sel.cpMin+1);
						SendDlgItemMessage(hwndDlg,IDC_LOG,EM_GETSELTEXT,0,(LPARAM)szText);
					}
					szQuoted=QuoteText(szText,64,1);
					free(szText);
					if(DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT)) {
						int len;
						SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_REPLACESEL,TRUE,(LPARAM)szQuoted);
						len=GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
						SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_SETSEL,len,len);
						SetFocus(GetDlgItem(hwndDlg,IDC_MESSAGE));
					}
					else {
						//this is a 'reply quoted' button
						HWND hwnd;
						struct NewMessageWindowLParam newData={0};

						if(hwnd=WindowList_Find(hMessageSendList,dat->hContact)) {
							int len;
							SetForegroundWindow(hwnd);
							ShowWindow(hwnd,SW_SHOWNORMAL);
							SendDlgItemMessage(hwnd,IDC_MESSAGE,EM_REPLACESEL,TRUE,(LPARAM)szQuoted);
							len=GetWindowTextLength(GetDlgItem(hwnd,IDC_MESSAGE));
							SendDlgItemMessage(hwnd,IDC_MESSAGE,EM_SETSEL,len,len);
							SetFocus(GetDlgItem(hwnd,IDC_MESSAGE));
						}
						else {
							newData.hContact=dat->hContact;
							newData.isSend=1;
							newData.szInitialText=szQuoted;
							CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_MSGSPLIT),NULL,DlgProcMessage,(LPARAM)&newData);
						}
						if(DBGetContactSettingByte(NULL,"SRMsg","CloseOnReply",SRMSGDEFSET_CLOSEONREPLY) && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_READNEXT)))
							DestroyWindow(hwndDlg);
					}
					free(szQuoted);
					break;
				}
				case IDC_READNEXT:
				{	DBEVENTINFO dbei={0};

					dbei.cbSize=sizeof(dbei);
					for(;;) {
						dat->hDbEventFirst=(HANDLE)CallService(MS_DB_EVENT_FINDNEXT,(WPARAM)dat->hDbEventFirst,0);
						if(dat->hDbEventFirst==NULL) break;
						dbei.cbBlob=0;
						CallService(MS_DB_EVENT_GET,(WPARAM)dat->hDbEventFirst,(LPARAM)&dbei);
						if(DbEventIsShown(&dbei)) break;
					}
					SendMessage(hwndDlg,DM_REMAKELOG,0,0);
					break;
				}
				case IDC_USERMENU:
					{	RECT rc;
						HMENU hMenu=(HMENU)CallService(MS_CLIST_MENUBUILDCONTACT,(WPARAM)dat->hContact,0);
						GetWindowRect(GetDlgItem(hwndDlg,IDC_USERMENU),&rc);
						TrackPopupMenu(hMenu,0,rc.left,rc.bottom,0,hwndDlg,NULL);
						DestroyMenu(hMenu);
					}
					break;
				case IDC_HISTORY:
					CallService(MS_HISTORY_SHOWCONTACTHISTORY,(WPARAM)dat->hContact,0);
					break;
				case IDC_DETAILS:
					CallService(MS_USERINFO_SHOWDIALOG,(WPARAM)dat->hContact,0);
					break;
				case IDC_TIME:
					dat->showTime^=1;
					DBWriteContactSettingByte(NULL,"SRMsg","ShowTime",(BYTE)dat->showTime);
					SendMessage(hwndDlg,DM_REMAKELOG,0,0);
					break;
				case IDC_MULTIPLE:
					dat->multiple=IsDlgButtonChecked(hwndDlg,IDC_MULTIPLE);
					ShowWindow(GetDlgItem(hwndDlg,IDC_MULTISPLITTER),dat->multiple?SW_SHOW:SW_HIDE);
					ShowWindow(GetDlgItem(hwndDlg,IDC_CLIST),dat->multiple?SW_SHOW:SW_HIDE);
					{	RECT rc;
						WINDOWPLACEMENT wp;

						SendMessage(hwndDlg,WM_SIZE,0,0);
						wp.length = sizeof(WINDOWPLACEMENT);
						GetWindowPlacement(hwndDlg,&wp);
						if (wp.showCmd!=SW_MAXIMIZE) {
							GetWindowRect(hwndDlg,&rc);
							SetWindowPos(hwndDlg,0,0,0,rc.right-rc.left+(dat->multiple?dat->multiSplitterX:-dat->multiSplitterX),rc.bottom-rc.top,SWP_NOZORDER|SWP_NOMOVE);
						}
					}
					break;
				case IDC_ADD:
					{	ADDCONTACTSTRUCT acs={0};

						acs.handle=dat->hContact;
						acs.handleType=HANDLE_CONTACT;
						acs.szProto=0;
						CallService(MS_ADDCONTACT_SHOW,(WPARAM)hwndDlg,(LPARAM)&acs);
					}
					if (!DBGetContactSettingByte(dat->hContact,"CList","NotOnList",0)) {
						ShowWindow(GetDlgItem(hwndDlg,IDC_ADD),FALSE);
					}
					break;
				case IDC_MESSAGE:
					if(HIWORD(wParam)==EN_CHANGE && (DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT) || dat->isSend)) {
						int len=GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
						UpdateReadChars(hwndDlg);
						EnableWindow(GetDlgItem(hwndDlg,IDOK),len!=0);
					}
					break;
			}
			break;
		case WM_NOTIFY:
			switch(((NMHDR*)lParam)->idFrom) {
				case IDC_CLIST:
					switch(((NMHDR*)lParam)->code) {
						case CLN_OPTIONSCHANGED:
                            SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_SETGREYOUTFLAGS,0,0);
							SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_SETLEFTMARGIN,2,0);
							break;
					}
					break;
				case IDC_LOG:
					switch(((NMHDR*)lParam)->code) {
						case EN_MSGFILTER:
							switch(((MSGFILTER*)lParam)->msg) {
								case WM_LBUTTONDOWN:
								{	HCURSOR hCur=GetCursor();
									if(hCur==LoadCursor(NULL,IDC_SIZENS) || hCur==LoadCursor(NULL,IDC_SIZEWE) || hCur==LoadCursor(NULL,IDC_SIZENESW) || hCur==LoadCursor(NULL,IDC_SIZENWSE)) {
										SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
										return TRUE;
									}
									break;
								}
								case WM_MOUSEMOVE:
								{	HCURSOR hCur=GetCursor();
									if(hCur==LoadCursor(NULL,IDC_SIZENS) || hCur==LoadCursor(NULL,IDC_SIZEWE) || hCur==LoadCursor(NULL,IDC_SIZENESW) || hCur==LoadCursor(NULL,IDC_SIZENWSE))
										SetCursor(LoadCursor(NULL,IDC_ARROW));
									break;
								}
								case WM_RBUTTONUP:
								{	HMENU hMenu,hSubMenu;
									POINT pt;
									CHARRANGE sel,all={0,-1};

									hMenu=LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_CONTEXT));
									hSubMenu=GetSubMenu(hMenu,5);
									CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hSubMenu,0);
									SendMessage(((NMHDR*)lParam)->hwndFrom,EM_EXGETSEL,0,(LPARAM)&sel);
									if(sel.cpMin==sel.cpMax) EnableMenuItem(hSubMenu,IDM_COPY,MF_BYCOMMAND|MF_GRAYED);
									pt.x=(short)LOWORD(((ENLINK*)lParam)->lParam);
									pt.y=(short)HIWORD(((ENLINK*)lParam)->lParam);
									ClientToScreen(((NMHDR*)lParam)->hwndFrom,&pt);
									switch(TrackPopupMenu(hSubMenu,TPM_RETURNCMD,pt.x,pt.y,0,hwndDlg,NULL)) {
										case IDM_COPY:
											SendMessage(((NMHDR*)lParam)->hwndFrom,WM_COPY,0,0);
											break;
										case IDM_COPYALL:
											SendMessage(((NMHDR*)lParam)->hwndFrom,EM_EXSETSEL,0,(LPARAM)&all);
											SendMessage(((NMHDR*)lParam)->hwndFrom,WM_COPY,0,0);
											SendMessage(((NMHDR*)lParam)->hwndFrom,EM_EXSETSEL,0,(LPARAM)&sel);
											break;
										case IDM_SELECTALL:
											SendMessage(((NMHDR*)lParam)->hwndFrom,EM_EXSETSEL,0,(LPARAM)&all);
											break;
										case IDM_CLEAR:
											SetDlgItemText(hwndDlg,IDC_LOG,"");
											dat->hDbEventFirst=NULL;
											break;
									}
									DestroyMenu(hMenu);
									SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
									return TRUE;
								}
							}
							break;
						case EN_LINK:
							switch(((ENLINK*)lParam)->msg) {
								case WM_SETCURSOR:
									SetCursor(hCurHyperlinkHand);
									SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
									return TRUE;
								case WM_RBUTTONDOWN:
								case WM_LBUTTONUP:
								{	TEXTRANGE tr;
									CHARRANGE sel;

									SendDlgItemMessage(hwndDlg,IDC_LOG,EM_EXGETSEL,0,(LPARAM)&sel);
									if(sel.cpMin!=sel.cpMax) break;
									tr.chrg=((ENLINK*)lParam)->chrg;
									tr.lpstrText=malloc(tr.chrg.cpMax-tr.chrg.cpMin+8);
									SendDlgItemMessage(hwndDlg,IDC_LOG,EM_GETTEXTRANGE,0,(LPARAM)&tr);
									if(strchr(tr.lpstrText,'@')!=NULL && strchr(tr.lpstrText,':')==NULL && strchr(tr.lpstrText,'/')==NULL) {
										MoveMemory(tr.lpstrText+7,tr.lpstrText,tr.chrg.cpMax-tr.chrg.cpMin+1);
										CopyMemory(tr.lpstrText,"mailto:",7);
									}
									if(((ENLINK*)lParam)->msg==WM_RBUTTONDOWN) {
										HMENU hMenu,hSubMenu;
										POINT pt;

										hMenu=LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_CONTEXT));
										hSubMenu=GetSubMenu(hMenu,6);
										CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hSubMenu,0);
										pt.x=(short)LOWORD(((ENLINK*)lParam)->lParam);
										pt.y=(short)HIWORD(((ENLINK*)lParam)->lParam);
										ClientToScreen(((NMHDR*)lParam)->hwndFrom,&pt);
										switch(TrackPopupMenu(hSubMenu,TPM_RETURNCMD,pt.x,pt.y,0,hwndDlg,NULL)) {
											case IDM_OPENNEW:
												CallService(MS_UTILS_OPENURL,1,(LPARAM)tr.lpstrText);
												break;
											case IDM_OPENEXISTING:
												CallService(MS_UTILS_OPENURL,0,(LPARAM)tr.lpstrText);
												break;
											case IDM_COPYLINK:
											{	HGLOBAL hData;
												if(!OpenClipboard(hwndDlg)) break;
												EmptyClipboard();
												hData=GlobalAlloc(GMEM_MOVEABLE,lstrlen(tr.lpstrText)+1);
												lstrcpy((char*)GlobalLock(hData),tr.lpstrText);
												GlobalUnlock(hData);
												SetClipboardData(CF_TEXT,hData);
												CloseClipboard();
												break;
											}
										}
										free(tr.lpstrText);
										DestroyMenu(hMenu);
										SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
										return TRUE;
									}
									else {
										CallService(MS_UTILS_OPENURL,1,(LPARAM)tr.lpstrText);
										SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
									}

									free(tr.lpstrText);
									break;
								}
							}
							break;
					}
					break;
			}
			break;
		case HM_EVENTSENT:
		{	ACKDATA *ack=(ACKDATA*)lParam;
			DBEVENTINFO dbei={0};
			HANDLE hNewEvent;
			int i;

			if(ack->type!=ACKTYPE_MESSAGE) break;

			switch(ack->result) {			
			case ACKRESULT_FAILED:
				KillTimer(hwndDlg,TIMERID_MSGSEND);
				ShowWindow(hwndDlg,SW_SHOWNORMAL);
				EnableWindow(hwndDlg,FALSE);
				CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MSGSENDERROR), hwndDlg, ErrorDlgProc, (LPARAM)strdup((char *)ack->lParam));
				return 0;
			} //switch

			for(i=0;i<dat->sendCount;i++)
				if(ack->hProcess==dat->sendInfo[i].hSendId && ack->hContact==dat->sendInfo[i].hContact) break;
			if(i==dat->sendCount) break;

			dbei.cbSize=sizeof(dbei);
			dbei.eventType=EVENTTYPE_MESSAGE;
			dbei.flags=DBEF_SENT;
			dbei.szModule=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)dat->sendInfo[i].hContact,0);
			dbei.timestamp=time(NULL);
			dbei.cbBlob=lstrlen(dat->sendBuffer)+1;
			dbei.pBlob=(PBYTE)dat->sendBuffer;
			hNewEvent=(HANDLE)CallService(MS_DB_EVENT_ADD,(WPARAM)dat->sendInfo[i].hContact,(LPARAM)&dbei);
			SkinPlaySound("SendMsg");
			if(dat->sendInfo[i].hContact==dat->hContact) {
				if(dat->hDbEventFirst==NULL) {
					dat->hDbEventFirst=hNewEvent;
					SendMessage(hwndDlg,DM_REMAKELOG,0,0);
				}
			}
			dat->sendInfo[i].hSendId=NULL;
			dat->sendInfo[i].hContact=NULL;
			for(i=0;i<dat->sendCount;i++)
				if(dat->sendInfo[i].hContact || dat->sendInfo[i].hSendId) break;
			if(i==dat->sendCount) {
				int len;
				//all messages sent
				dat->sendCount=0;
				free(dat->sendInfo);
				dat->sendInfo=NULL;
				KillTimer(hwndDlg,TIMERID_MSGSEND);
				EnableWindow(GetDlgItem(hwndDlg,IDC_CLIST),TRUE);
				SetDlgItemText(hwndDlg,IDC_MESSAGE,"");
				EnableWindow(GetDlgItem(hwndDlg,IDOK),FALSE);
				SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_SETREADONLY,FALSE,0);
				SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_REPLAYSAVEDKEYSTROKES,0,0);
				if(GetForegroundWindow()==hwndDlg) SetFocus(GetDlgItem(hwndDlg,IDC_MESSAGE));
				len=GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
				UpdateReadChars(hwndDlg);
				EnableWindow(GetDlgItem(hwndDlg,IDOK),len!=0);
				if(dat->hAckEvent) {
					UnhookEvent(dat->hAckEvent);
					dat->hAckEvent=NULL;
				}
				if(DBGetContactSettingByte(NULL,"SRMsg","AutoClose",SRMSGDEFSET_AUTOCLOSE)) DestroyWindow(hwndDlg);
			}
			break;
		}
		case WM_DESTROY:
			if(dat->hAckEvent) UnhookEvent(dat->hAckEvent);
			if(dat->sendInfo) free(dat->sendInfo);
			if(dat->hNewEvent) UnhookEvent(dat->hNewEvent);
			DestroyWindow(dat->hwndToolTips);
			if(dat->hBkgBrush) DeleteObject(dat->hBkgBrush);
			if(dat->sendBuffer!=NULL) free(dat->sendBuffer);
			{int i;for(i=0;i<sizeof(dat->hIcons)/sizeof(dat->hIcons[0]);i++)DestroyIcon(dat->hIcons[i]);}
			WindowList_Remove(dat->isSend?hMessageSendList:hMessageWindowList,hwndDlg);
			DBWriteContactSettingDword(NULL,"SRMsg","splitsplity",dat->splitterY);
			DBWriteContactSettingDword(NULL,"SRMsg","multisplit",dat->multiSplitterX);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_MULTISPLITTER),GWL_WNDPROC,(LONG)OldSplitterProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_SPLITTER),GWL_WNDPROC,(LONG)OldSplitterProc);
			SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_UNSUBCLASSED,0,0);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_MESSAGE),GWL_WNDPROC,(LONG)OldMessageEditProc);
			{	HFONT hFont;
				hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_MESSAGE,WM_GETFONT,0,0);
				if(hFont!=NULL && hFont!=(HFONT)SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0)) DeleteObject(hFont);
			}
			{	WINDOWPLACEMENT wp={0};
				HANDLE hContact;

				if(DBGetContactSettingByte(NULL,"SRMsg","SavePerContact",SRMSGDEFSET_SAVEPERCONTACT))
					hContact=dat->hContact;
				else hContact=NULL;
				wp.length=sizeof(wp);
				GetWindowPlacement(hwndDlg,&wp);
				if(!dat->windowWasCascaded) {
					DBWriteContactSettingDword(hContact,"SRMsg",dat->isSend?"sendx":"splitx",wp.rcNormalPosition.left);
					DBWriteContactSettingDword(hContact,"SRMsg",dat->isSend?"sendy":"splity",wp.rcNormalPosition.top);
				}
				DBWriteContactSettingDword(hContact,"SRMsg",dat->isSend?"sendwidth":"splitwidth",wp.rcNormalPosition.right-wp.rcNormalPosition.left-(dat->multiple?dat->multiSplitterX:0));
				DBWriteContactSettingDword(hContact,"SRMsg",dat->isSend?"sendheight":"splitheight",wp.rcNormalPosition.bottom-wp.rcNormalPosition.top);
			}
			free(dat);
			break;

	}
	return FALSE;
}


