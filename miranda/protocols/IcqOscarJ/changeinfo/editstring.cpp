// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2001-2004 Richard Hughes, Martin �berg
// Copyright � 2004-2009 Joe Kucera, Bio
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  ChangeInfo Plugin stuff
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


static ChangeInfoData *dataStringEdit = NULL;
static WNDPROC OldStringEditProc, OldExpandButtonProc;
static HWND hwndEdit = NULL, hwndExpandButton = NULL, hwndUpDown = NULL;

static const char escapes[]={'a','\a',
'b','\b',
'e',27,
'f','\f',
'r','\r',
't','\t',
'v','\v',
'\\','\\'};

static void EscapesToMultiline(WCHAR *str,PDWORD selStart,PDWORD selEnd)
{ //converts "\\n" and "\\t" to "\r\n" and "\t" because a multi-line edit box can show them properly
	DWORD i;

	for(i=0; *str; str++, i++) 
	{
		if (*str != '\\') continue;
		if (str[1] == 'n') 
		{
			*str++ = '\r'; 
			i++; 
			*str = '\n';
		}
		else if (str[1] == 't') 
		{
			*str = '\t';
			memmove(str+1, str+2, sizeof(WCHAR)*(strlennull(str)-1));

			if (*selStart>i) --*selStart;
			if (*selEnd>i) --*selEnd;
		}
	}
}



static void EscapesToBinary(char *str)
{
	int i;

	for(;*str;str++) 
	{
		if(*str!='\\') continue;
		if(str[1]=='n') {*str++='\r'; i++; *str='\n'; continue;}
		if(str[1]=='0') 
		{
			char *codeend;
			*str=(char)strtol(str+1,&codeend,8);
			if(*str==0) {*str='\\'; continue;}
			memmove(str+1,codeend,strlennull(codeend)+1);
			continue;
		}
		for(i=0;i<sizeof(escapes)/sizeof(escapes[0]);i+=2)
			if(str[1]==escapes[i]) 
			{
				*str=escapes[i+1];
				memmove(str+1,str+2,strlennull(str)-1);
				break;
			}
	}
}



char *BinaryToEscapes(char *str)
{
	int extra=10,len=strlennull(str)+11,i;
	char *out,*pout;

	out=pout=(char*)SAFE_MALLOC(len);
	for(;*str;str++) 
	{
		if((unsigned char)*str>=' ') 
		{
			*pout++=*str; 
			continue;
		}
		if(str[0]=='\r' && str[1]=='\n') 
		{
			*pout++='\\'; 
			*pout++='n';
			str++;
			continue;
		}
		if(extra<3) 
		{
			extra+=8; len+=8;
			pout=out=(char*)SAFE_REALLOC(out,len);
		}
		*pout++='\\';
		for(i=0;i<sizeof(escapes)/sizeof(escapes[0]);i+=2)
			if(*str==escapes[i+1]) 
			{
				*pout++=escapes[i];
				extra--;
				break;
			}
			if(i<sizeof(escapes)/sizeof(escapes[0])) continue;
			*pout++='0'; extra--;
			if(*str>=8) 
			{
				*pout++=(*str>>3)+'0';
				extra--;
			}
			*pout++=(*str&7)+'0'; extra--;
	}
	*pout='\0';
	return out;
}



static LRESULT CALLBACK StringEditSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) 
	{
	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) 
		{
      if (dataStringEdit)
			  dataStringEdit->EndStringEdit(0); 
			return 0;
		}
		if (wParam==VK_RETURN) 
		{
			if (GetWindowLong(hwnd, GWL_STYLE) & ES_MULTILINE && !(GetKeyState(VK_CONTROL) & 0x8000)) break;
      if (dataStringEdit)
			  dataStringEdit->EndStringEdit(1);
			return 0;
		}
		break;

	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS|CallWindowProc(OldStringEditProc, hwnd, msg, wParam, lParam);

	case WM_KILLFOCUS:
		if ((HWND)wParam == hwndExpandButton) break;
    if (dataStringEdit)
		  dataStringEdit->EndStringEdit(1);
		return 0;
	}
	return CallWindowProc(OldStringEditProc, hwnd, msg, wParam, lParam);
}



static LRESULT CALLBACK ExpandButtonSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) 
	{
	case WM_LBUTTONUP:
		if(GetCapture()==hwnd) 
		{
			//do expand
			RECT rcStart,rcEnd;
			DWORD selStart,selEnd;
			WCHAR *text;
			BOOL animEnabled=TRUE;

			GetWindowRect(hwndEdit,&rcStart);
			InflateRect(&rcStart,2,2);

			text = GetWindowTextUcs(hwndEdit);
			SendMessage(hwndEdit,EM_GETSEL,(WPARAM)&selStart,(LPARAM)&selEnd);
			DestroyWindow(hwndEdit);
			EscapesToMultiline(text,&selStart,&selEnd);
			hwndEdit=CreateWindowExA(WS_EX_TOOLWINDOW,"EDIT","",WS_POPUP|WS_BORDER|WS_VISIBLE|ES_WANTRETURN|ES_AUTOVSCROLL|WS_VSCROLL|ES_MULTILINE,rcStart.left,rcStart.top,rcStart.right-rcStart.left,rcStart.bottom-rcStart.top,NULL,NULL,hInst,NULL);
			SetWindowTextUcs(hwndEdit, text);
			OldStringEditProc=(WNDPROC)SetWindowLong(hwndEdit,GWL_WNDPROC,(LONG)StringEditSubclassProc);
			SendMessage(hwndEdit,WM_SETFONT,(WPARAM)dataStringEdit->hListFont,0);
			SendMessage(hwndEdit,EM_SETSEL,selStart,selEnd);
			SetFocus(hwndEdit);

			rcEnd.left=rcStart.left; rcEnd.top=rcStart.top;
			rcEnd.right=rcEnd.left+350;
			rcEnd.bottom=rcEnd.top+150;
			if (LOBYTE(LOWORD(GetVersion()))>4 || HIBYTE(LOWORD(GetVersion()))>0)
				SystemParametersInfo(SPI_GETCOMBOBOXANIMATION,0,&animEnabled,FALSE);
			if(animEnabled) 
			{
				DWORD startTime,timeNow;
				startTime=GetTickCount();
				for(;;) 
				{
					UpdateWindow(hwndEdit);
					timeNow=GetTickCount();
					if(timeNow>startTime+200) break;
					SetWindowPos(hwndEdit,0,rcEnd.left,rcEnd.top,(rcEnd.right-rcStart.right)*(timeNow-startTime)/200+rcStart.right-rcEnd.left,(rcEnd.bottom-rcStart.bottom)*(timeNow-startTime)/200+rcStart.bottom-rcEnd.top,SWP_NOZORDER);
				}
			}
			SetWindowPos(hwndEdit,0,rcEnd.left,rcEnd.top,rcEnd.right-rcEnd.left,rcEnd.bottom-rcEnd.top,SWP_NOZORDER);

			DestroyWindow(hwnd);
			hwndExpandButton=NULL;

			SAFE_FREE((void**)&text);
		}
		break;
	}
	return CallWindowProc(OldExpandButtonProc,hwnd,msg,wParam,lParam);
}


void ChangeInfoData::BeginStringEdit(int iItem, RECT *rc, int i, WORD wVKey)
{
	char *szValue;
	char str[80];
	int alloced=0;

	EndStringEdit(0);
	InflateRect(rc,-2,-2);
	rc->left-=2;
	if (setting[i].displayType & LIF_PASSWORD && !settingData[i].changed)
		szValue = "                ";
	else if ((setting[i].displayType & LIM_TYPE) == LI_NUMBER) 
	{
		szValue = str;
		null_snprintf(str, sizeof(str), "%d", settingData[i].value );
	}
	else if (settingData[i].value) 
	{
		szValue = BinaryToEscapes((char*)settingData[i].value);
		alloced = 1;
	}
	else szValue = "";

  iEditItem = iItem;

	if ((setting[i].displayType & LIM_TYPE)==LI_LONGSTRING) 
	{
		rc->right-=rc->bottom-rc->top;
		hwndExpandButton=CreateWindowA("BUTTON","",WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_ICON,rc->right,rc->top,rc->bottom-rc->top,rc->bottom-rc->top,hwndList,NULL,hInst,NULL);
		SendMessage(hwndExpandButton,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadImage(hInst,MAKEINTRESOURCE(IDI_EXPANDSTRINGEDIT),IMAGE_ICON,0,0,LR_SHARED));
		OldExpandButtonProc=(WNDPROC)SetWindowLong(hwndExpandButton,GWL_WNDPROC,(LONG)ExpandButtonSubclassProc);
	}

  dataStringEdit = this;
	hwndEdit = CreateWindow(_T("EDIT"),_T(""),WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL|((setting[i].displayType&LIM_TYPE)==LI_NUMBER?ES_NUMBER:0)|(setting[i].displayType&LIF_PASSWORD?ES_PASSWORD:0),rc->left,rc->top,rc->right-rc->left,rc->bottom-rc->top,hwndList,NULL,hInst,NULL);
	SetWindowTextUtf(hwndEdit, szValue);
	if (alloced) SAFE_FREE((void**)&szValue);
	OldStringEditProc=(WNDPROC)SetWindowLong(hwndEdit,GWL_WNDPROC,(LONG)StringEditSubclassProc);
	SendMessage(hwndEdit,WM_SETFONT,(WPARAM)hListFont,0);
	if ((setting[i].displayType & LIM_TYPE) == LI_NUMBER) 
	{
		int *range= (int*)setting[i].pList;
		RECT rcUpDown;
		hwndUpDown=CreateWindow(UPDOWN_CLASS,_T(""),WS_VISIBLE|WS_CHILD|UDS_AUTOBUDDY|UDS_ALIGNRIGHT|UDS_HOTTRACK|UDS_NOTHOUSANDS|UDS_SETBUDDYINT,0,0,0,0,hwndList,NULL,hInst,NULL);
		SendMessage(hwndUpDown, UDM_SETRANGE32, range[0], range[1]);
		SendMessage(hwndUpDown, UDM_SETPOS32, 0, settingData[i].value);
		if(!(setting[i].displayType & LIF_ZEROISVALID) && settingData[i].value==0)
			SetWindowTextA(hwndEdit, "");
		GetClientRect(hwndUpDown, &rcUpDown);
		rc->right -= rcUpDown.right;
		SetWindowPos(hwndEdit,0,0,0,rc->right-rc->left,rc->bottom-rc->top,SWP_NOZORDER|SWP_NOMOVE);
	}
	SendMessage(hwndEdit,EM_SETSEL,0,(LPARAM)-1);
	SetFocus(hwndEdit);
	PostMessage(hwndEdit,WM_KEYDOWN,wVKey,0);
}


void ChangeInfoData::EndStringEdit(int save)
{
	if (hwndEdit == NULL || iEditItem == -1 || this != dataStringEdit) return;
	if (save) 
	{
		char *text = (char*)SAFE_MALLOC(GetWindowTextLength(hwndEdit)+1);

		GetWindowTextA(hwndEdit,(char*)text,GetWindowTextLength(hwndEdit)+1);
		EscapesToBinary(text);
		if((setting[iEditItem].displayType&LIM_TYPE)==LI_NUMBER)
		{
			LPARAM newValue;
			int *range=(int*)setting[iEditItem].pList;
			newValue = atoi((char*)text);
			if (newValue) 
			{
				if (newValue<range[0]) newValue=range[0];
				if (newValue>range[1]) newValue=range[1];
			}
			settingData[iEditItem].changed = settingData[iEditItem].value != newValue;
			settingData[iEditItem].value = newValue;
			SAFE_FREE((void**)&text);
		}
		else
		{
			if (!(setting[iEditItem].displayType & LIF_PASSWORD))
			{
				SAFE_FREE((void**)&text);
				text = GetWindowTextUtf(hwndEdit);
				EscapesToBinary((char*)text);
			}
			if ((setting[iEditItem].displayType & LIF_PASSWORD && strcmpnull(text,"                ")) ||
				(!(setting[iEditItem].displayType & LIF_PASSWORD) && strcmpnull(text, (char*)settingData[iEditItem].value) && (strlennull(text) + strlennull((char*)settingData[iEditItem].value))))
			{
				SAFE_FREE((void**)&settingData[iEditItem].value);
				if (text[0])
					settingData[iEditItem].value = (LPARAM)text;
				else
				{
					settingData[iEditItem].value = 0; 
					SAFE_FREE((void**)&text);
				}
				settingData[iEditItem].changed = 1;
			}
		}
		if (settingData[iEditItem].changed) EnableDlgItem(hwndDlg, IDC_SAVE, TRUE);
	}
	ListView_RedrawItems(hwndList, iEditItem, iEditItem);
	iEditItem = -1;
  dataStringEdit = NULL;
	DestroyWindow(hwndEdit);
	hwndEdit = NULL;
	if (hwndExpandButton) DestroyWindow(hwndExpandButton);
	hwndExpandButton = NULL;
	if (hwndUpDown) DestroyWindow(hwndUpDown);
	hwndUpDown = NULL;
}



int IsStringEditWindow(HWND hwnd)
{
	if (hwnd == hwndEdit) return 1;
	if (hwnd == hwndExpandButton) return 1;
	if (hwnd == hwndUpDown) return 1;
	return 0;
}
