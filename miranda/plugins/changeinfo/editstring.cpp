/*
Change ICQ Details plugin for Miranda IM

Copyright © 2001-2005 Richard Hughes, Martin Öberg

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



#include <windows.h>
#include <commctrl.h>
#include <newpluginapi.h>
#include <m_langpack.h>
#include "changeinfo.h"
#include "resource.h"

#ifndef AW_SLIDE
#define SPI_GETCOMBOBOXANIMATION 0x1004
#define AW_SLIDE			0x40000
#define AW_ACTIVATE			0x20000
#define AW_VER_POSITIVE		0x4
#define UDM_SETPOS32            (WM_USER+113)
#define UDM_GETPOS32            (WM_USER+114)
#endif

static WNDPROC OldStringEditProc,OldExpandButtonProc;
static HWND hwndEdit=NULL,hwndExpandButton=NULL,hwndUpDown=NULL;
extern HWND hwndList;
extern HFONT hListFont;
extern HINSTANCE hInst;
extern int iEditItem;

static const char escapes[]={'a','\a',
							 'b','\b',
							 'e',27,
							 'f','\f',
							 'r','\r',
							 't','\t',
							 'v','\v',
							 '\\','\\'};

static void EscapesToMultiline(char *str,PDWORD selStart,PDWORD selEnd)
{  //converts "\\n" and "\\t" to "\r\n" and "\t" because a multi-line edit box can show them properly
	DWORD i;

	for(i=0;*str;str++,i++) {
		if(*str!='\\') continue;
		if(str[1]=='n') {*str++='\r'; i++; *str='\n';}
		else if(str[1]=='t') {
			*str='\t';
			memmove(str+1,str+2,strlen(str)-1);
			if(*selStart>i) --*selStart;
			if(*selEnd>i) --*selEnd;
		}
	}
}

static void EscapesToBinary(char *str)
{
	int i;

	for(;*str;str++) {
		if(*str!='\\') continue;
		if(str[1]=='n') {*str++='\r'; i++; *str='\n'; continue;}
		if(str[1]=='0') {
			char *codeend;
			*str=(char)strtol(str+1,&codeend,8);
			if(*str==0) {*str='\\'; continue;}
			memmove(str+1,codeend,strlen(codeend)+1);
			continue;
		}
		for(i=0;i<sizeof(escapes)/sizeof(escapes[0]);i+=2)
			if(str[1]==escapes[i]) {
				*str=escapes[i+1];
				memmove(str+1,str+2,strlen(str)-1);
				break;
			}
	}
}

char *BinaryToEscapes(char *str)
{
	int extra=10,len=strlen(str)+11,i;
	char *out,*pout;

	out=pout=(char*)malloc(len);
	for(;*str;str++) {
		if((unsigned char)*str>=' ') {*pout++=*str; continue;}
		if(str[0]=='\r' && str[1]=='\n') {
			*pout++='\\'; *pout++='n';
			str++;
			continue;
		}
		if(extra<3) {
			extra+=8; len+=8;
			pout=out=(char*)realloc(out,len);
		}
		*pout++='\\';
		for(i=0;i<sizeof(escapes)/sizeof(escapes[0]);i+=2)
			if(*str==escapes[i+1]) {
				*pout++=escapes[i];
				extra--;
				break;
			}
		if(i<sizeof(escapes)/sizeof(escapes[0])) continue;
		*pout++='0'; extra--;
		if(*str>=8) {*pout++=(*str>>3)+'0'; extra--;}
		*pout++=(*str&7)+'0'; extra--;
	}
	*pout='\0';
	return out;
}

static LRESULT CALLBACK StringEditSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) {
		case WM_KEYDOWN:
			if(wParam==VK_ESCAPE) {EndStringEdit(0); return 0;}
			if(wParam==VK_RETURN) {
				if(GetWindowLong(hwnd,GWL_STYLE)&ES_MULTILINE && !(GetKeyState(VK_CONTROL)&0x8000)) break;
				EndStringEdit(1);
				return 0;
			}
			break;
		case WM_GETDLGCODE:
			return DLGC_WANTALLKEYS|CallWindowProc(OldStringEditProc,hwnd,msg,wParam,lParam);
		case WM_KILLFOCUS:
			if((HWND)wParam==hwndExpandButton) break;
			EndStringEdit(1);
			return 0;
	}
	return CallWindowProc(OldStringEditProc,hwnd,msg,wParam,lParam);
}

static LRESULT CALLBACK ExpandButtonSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) {
		case WM_LBUTTONUP:
			if(GetCapture()==hwnd) {
				//do expand
				RECT rcStart,rcEnd;
				DWORD selStart,selEnd;
				char *text;
				BOOL animEnabled=TRUE;

				GetWindowRect(hwndEdit,&rcStart);
				InflateRect(&rcStart,2,2);

				text=(char*)malloc(GetWindowTextLength(hwndEdit)+1);
				GetWindowText(hwndEdit,text,GetWindowTextLength(hwndEdit)+1);
				SendMessage(hwndEdit,EM_GETSEL,(WPARAM)&selStart,(LPARAM)&selEnd);
				DestroyWindow(hwndEdit);
				EscapesToMultiline(text,&selStart,&selEnd);
				hwndEdit=CreateWindowEx(WS_EX_TOOLWINDOW,"EDIT",text,WS_POPUP|WS_BORDER|WS_VISIBLE|ES_WANTRETURN|ES_AUTOVSCROLL|WS_VSCROLL|ES_MULTILINE,rcStart.left,rcStart.top,rcStart.right-rcStart.left,rcStart.bottom-rcStart.top,NULL,NULL,hInst,NULL);
				free(text);
				OldStringEditProc=(WNDPROC)SetWindowLong(hwndEdit,GWL_WNDPROC,(LONG)StringEditSubclassProc);
				SendMessage(hwndEdit,WM_SETFONT,(WPARAM)hListFont,0);
				SendMessage(hwndEdit,EM_SETSEL,selStart,selEnd);
				SetFocus(hwndEdit);

				rcEnd.left=rcStart.left; rcEnd.top=rcStart.top;
				rcEnd.right=rcEnd.left+350;
				rcEnd.bottom=rcEnd.top+150;
				if(LOBYTE(LOWORD(GetVersion()))>4 || HIBYTE(LOWORD(GetVersion()))>0)
					SystemParametersInfo(SPI_GETCOMBOBOXANIMATION,0,&animEnabled,FALSE);
				if(animEnabled) {
					DWORD startTime,timeNow;
					startTime=GetTickCount();
					for(;;) {
						UpdateWindow(hwndEdit);
						timeNow=GetTickCount();
						if(timeNow>startTime+200) break;
						SetWindowPos(hwndEdit,0,rcEnd.left,rcEnd.top,(rcEnd.right-rcStart.right)*(timeNow-startTime)/200+rcStart.right-rcEnd.left,(rcEnd.bottom-rcStart.bottom)*(timeNow-startTime)/200+rcStart.bottom-rcEnd.top,SWP_NOZORDER);
					}
				}
				SetWindowPos(hwndEdit,0,rcEnd.left,rcEnd.top,rcEnd.right-rcEnd.left,rcEnd.bottom-rcEnd.top,SWP_NOZORDER);

				DestroyWindow(hwnd);
				hwndExpandButton=NULL;
			}
			break;
	}
	return CallWindowProc(OldExpandButtonProc,hwnd,msg,wParam,lParam);
}

void BeginStringEdit(int iItem,RECT *rc,int i,WORD wVKey)
{
	char *szValue;
	char str[80];
	int alloced=0;

	EndStringEdit(0);
	InflateRect(rc,-2,-2);
	rc->left-=2;
	if(setting[i].displayType&LIF_PASSWORD && !setting[i].changed)
		szValue="                ";
	else if((setting[i].displayType&LIM_TYPE)==LI_NUMBER) {
		szValue=str;
		wsprintf(str,"%d",setting[i].value);
	}
	else if(setting[i].value) {
		szValue=BinaryToEscapes((char*)setting[i].value);
		alloced=1;
	}
	else szValue="";
	iEditItem=iItem;

	if((setting[i].displayType&LIM_TYPE)==LI_LONGSTRING) {
		rc->right-=rc->bottom-rc->top;
		hwndExpandButton=CreateWindow("BUTTON","",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_ICON,rc->right,rc->top,rc->bottom-rc->top,rc->bottom-rc->top,hwndList,NULL,hInst,NULL);
		SendMessage(hwndExpandButton,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadImage(hInst,MAKEINTRESOURCE(IDI_EXPANDSTRINGEDIT),IMAGE_ICON,0,0,LR_SHARED));
		OldExpandButtonProc=(WNDPROC)SetWindowLong(hwndExpandButton,GWL_WNDPROC,(LONG)ExpandButtonSubclassProc);
	}

	hwndEdit=CreateWindow("EDIT",szValue,WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL|((setting[i].displayType&LIM_TYPE)==LI_NUMBER?ES_NUMBER:0)|(setting[i].displayType&LIF_PASSWORD?ES_PASSWORD:0),rc->left,rc->top,rc->right-rc->left,rc->bottom-rc->top,hwndList,NULL,hInst,NULL);
	if(alloced) free(szValue);
	OldStringEditProc=(WNDPROC)SetWindowLong(hwndEdit,GWL_WNDPROC,(LONG)StringEditSubclassProc);
	SendMessage(hwndEdit,WM_SETFONT,(WPARAM)hListFont,0);
	if((setting[i].displayType&LIM_TYPE)==LI_NUMBER) {
		int *range=(int*)setting[i].pList;
		RECT rcUpDown;
		hwndUpDown=CreateWindow(UPDOWN_CLASS,"",WS_VISIBLE|WS_CHILD|UDS_AUTOBUDDY|UDS_ALIGNRIGHT|UDS_HOTTRACK|UDS_NOTHOUSANDS|UDS_SETBUDDYINT,0,0,0,0,hwndList,NULL,hInst,NULL);
		SendMessage(hwndUpDown,UDM_SETRANGE32,range[0],range[1]);
		SendMessage(hwndUpDown,UDM_SETPOS32,0,setting[i].value);
		if(!(setting[i].displayType&LIF_ZEROISVALID) && setting[i].value==0)
			SetWindowText(hwndEdit,"");
		GetClientRect(hwndUpDown,&rcUpDown);
		rc->right-=rcUpDown.right;
		SetWindowPos(hwndEdit,0,0,0,rc->right-rc->left,rc->bottom-rc->top,SWP_NOZORDER|SWP_NOMOVE);
	}
	SendMessage(hwndEdit,EM_SETSEL,0,(LPARAM)-1);
	SetFocus(hwndEdit);
	PostMessage(hwndEdit,WM_KEYDOWN,wVKey,0);
}

void EndStringEdit(int save)
{
	if(hwndEdit==NULL || iEditItem==-1) return;
	if(save) {
		char *text;
		text=(char*)malloc(GetWindowTextLength(hwndEdit)+1);
		GetWindowText(hwndEdit,text,GetWindowTextLength(hwndEdit)+1);
		EscapesToBinary(text);
		if((setting[iEditItem].displayType&LIM_TYPE)==LI_NUMBER) {
			LPARAM newValue;
			int *range=(int*)setting[iEditItem].pList;
			newValue=atoi(text);
			if(newValue) {
				if(newValue<range[0]) newValue=range[0];
				if(newValue>range[1]) newValue=range[1];
			}
			setting[iEditItem].changed=setting[iEditItem].value!=newValue;
			setting[iEditItem].value=newValue;
		}
		else {
			if((setting[iEditItem].displayType&LIF_PASSWORD && lstrcmp(text,"                ")) ||
			   (!(setting[iEditItem].displayType&LIF_PASSWORD) && (setting[iEditItem].value || text[0]) && !(setting[iEditItem].value && !strcmp(text,(char*)setting[iEditItem].value)))) {
				if(setting[iEditItem].value) free((char*)setting[iEditItem].value);
				if(text[0]) setting[iEditItem].value=(LPARAM)text;
				else {setting[iEditItem].value=0; free(text);}
				setting[iEditItem].changed=1;
			}
		}
		if(setting[iEditItem].changed) EnableWindow(GetDlgItem(GetParent(hwndList),IDC_SAVE),TRUE);
	}
	ListView_RedrawItems(hwndList,iEditItem,iEditItem);
	iEditItem=-1;
	DestroyWindow(hwndEdit);
	hwndEdit=NULL;
	if(hwndExpandButton!=NULL) DestroyWindow(hwndExpandButton);
	hwndExpandButton=NULL;
	if(hwndUpDown!=NULL) DestroyWindow(hwndUpDown);
	hwndUpDown=NULL;
}

int IsStringEditWindow(HWND hwnd)
{
	if(hwnd==hwndEdit) return 1;
	if(hwnd==hwndExpandButton) return 1;
	if(hwnd==hwndUpDown) return 1;
	return 0;
}