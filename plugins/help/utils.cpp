/*
Miranda IM Help Plugin
Copyright (C) 2002 Richard Hughes

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

#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <newpluginapi.h>
#include <m_utils.h>
#include "help.h"

#include <commctrl.h>
#include <richedit.h>
#include <m_clc.h>

#ifdef EDITOR
extern const char *szControlTypeNames[]={
	"Unknown type","Dialog box","Button","Check box","Radio button",
	"Static text","Static image","Edit box","Group box","Combo box",
	"List box","Spin edit box","Progress bar","Slider","List view",
	"Tree","Date/time picker","IP address","Status bar","Hyperlink",
	"Contact list","Scroll bar","Animation","Hot key","Tabs","Colour picker"};
#endif

int GetControlType(HWND hwndCtl)
{
	TCHAR szClassName[32];
	DWORD style;
	GetClassName(hwndCtl,szClassName,sizeof(szClassName));
	if(!lstrcmp(szClassName,_T("#32770"))) return CTLTYPE_DIALOG;
	else if(!lstrcmp(szClassName,_T("Static"))) {
		style=GetWindowLong(hwndCtl,GWL_STYLE);
		switch(style&SS_TYPEMASK) {
			case SS_BITMAP:
			case SS_BLACKFRAME:
			case SS_BLACKRECT:
			case SS_ENHMETAFILE:
			case SS_ETCHEDFRAME:
			case SS_ETCHEDHORZ:
			case SS_ETCHEDVERT:
			case SS_WHITEFRAME:
			case SS_WHITERECT:
			case SS_GRAYFRAME:
			case SS_GRAYRECT:
			case SS_ICON:
			case SS_OWNERDRAW:
				return CTLTYPE_IMAGE;
		}
		return CTLTYPE_TEXT;
	}
	else if(!lstrcmp(szClassName,_T("Button"))) {
		style=GetWindowLong(hwndCtl,GWL_STYLE);
		switch(style&0x1f) {
			case BS_CHECKBOX:
			case BS_AUTOCHECKBOX:
			case BS_3STATE:
			case BS_AUTO3STATE:
				return CTLTYPE_CHECKBOX;
			case BS_RADIOBUTTON:
			case BS_AUTORADIOBUTTON:
				return CTLTYPE_RADIO;
			case BS_GROUPBOX:
				return CTLTYPE_GROUP;
		}
		return CTLTYPE_BUTTON;
	}
	else if(!lstrcmp(szClassName,_T("Edit"))) {
		GetClassName(GetParent(hwndCtl),szClassName,sizeof(szClassName));
		if(!lstrcmp(szClassName,_T("ComboBox"))) return CTLTYPE_COMBO;
		if(GetClassName(GetWindow(hwndCtl,GW_HWNDNEXT),szClassName,sizeof(szClassName)) && !lstrcmp(szClassName,UPDOWN_CLASS))
			return CTLTYPE_SPINEDIT;
		return CTLTYPE_EDIT;
	}
	else if(!lstrcmp(szClassName,_T("RichEdit")) || !lstrcmp(szClassName,RICHEDIT_CLASS))
		return CTLTYPE_EDIT;
	else if(!lstrcmp(szClassName,_T("ListBox"))) return CTLTYPE_LIST;
	else if(!lstrcmp(szClassName,_T("ComboBox")) || !lstrcmp(szClassName,WC_COMBOBOXEX)) return CTLTYPE_COMBO;
	else if(!lstrcmp(szClassName,_T("ScrollBar"))) return CTLTYPE_SCROLLBAR;
	else if(!lstrcmp(szClassName,UPDOWN_CLASS)) return CTLTYPE_SPINEDIT;
	else if(!lstrcmp(szClassName,PROGRESS_CLASS)) return CTLTYPE_PROGRESS;
	else if(!lstrcmp(szClassName,TRACKBAR_CLASS)) return CTLTYPE_TRACKBAR;
	else if(!lstrcmp(szClassName,WC_LISTVIEW) || !lstrcmp(szClassName,WC_HEADER)) return CTLTYPE_LISTVIEW;
	else if(!lstrcmp(szClassName,WC_TREEVIEW)) return CTLTYPE_TREEVIEW;
	else if(!lstrcmp(szClassName,DATETIMEPICK_CLASS)) return CTLTYPE_DATETIME;
	else if(!lstrcmp(szClassName,WC_IPADDRESS)) return CTLTYPE_IP;
	else if(!lstrcmp(szClassName,STATUSCLASSNAME)) return CTLTYPE_STATUSBAR;
	else if(!lstrcmp(szClassName,CLISTCONTROL_CLASS)) return CTLTYPE_CLC;
	else if(!lstrcmp(szClassName,WNDCLASS_HYPERLINK)) return CTLTYPE_HYPERLINK;
	else if(!lstrcmp(szClassName,ANIMATE_CLASS)) return CTLTYPE_ANIMATION;
	else if(!lstrcmp(szClassName,HOTKEY_CLASS)) return CTLTYPE_HOTKEY;
	else if(!lstrcmp(szClassName,WC_TABCONTROL)) return CTLTYPE_TABS;
	else if(!lstrcmp(szClassName,WNDCLASS_COLOURPICKER)) return CTLTYPE_COLOUR;
	return CTLTYPE_UNKNOWN;
}

#ifdef EDITOR
int GetControlTitle(HWND hwndCtl,char *pszTitle,int cbTitle)
{
	int type;
	type=GetControlType(hwndCtl);
	switch(type) {
		case CTLTYPE_DIALOG:
		case CTLTYPE_BUTTON:
		case CTLTYPE_CHECKBOX:
		case CTLTYPE_RADIO:
		case CTLTYPE_TEXT:
		case CTLTYPE_GROUP:
		case CTLTYPE_HYPERLINK:
			GetWindowText(hwndCtl,pszTitle,cbTitle);
			return 0;
	}
	hwndCtl=GetWindow(hwndCtl,GW_HWNDPREV);
	if(hwndCtl) {
		type=GetControlType(hwndCtl);
		switch(type) {
			case CTLTYPE_TEXT:
			case CTLTYPE_GROUP:
				GetWindowText(hwndCtl,pszTitle,cbTitle);
				return 0;
		}
	}
	if(cbTitle) pszTitle[0]='\0';
	return 1;
}
#endif  //defined EDITOR

static char base64chars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//free() the return value
static char *Base64Encode(PBYTE pBuf,int cbBuf)
{
	int iIn,iOut;
	char *pszOut;

	pszOut=(char*)malloc((cbBuf*4+11)/12*4+1);
	for(iIn=0,iOut=0;iIn<cbBuf;iIn+=3) {
		pszOut[iOut++]=base64chars[pBuf[iIn]>>2];
		if(cbBuf-iIn==1) {
			pszOut[iOut++]=base64chars[(pBuf[iIn]&3)<<4];
			pszOut[iOut++]='=';
			pszOut[iOut++]='=';
			break;
		}
		pszOut[iOut++]=base64chars[((pBuf[iIn]&3)<<4)|(pBuf[iIn+1]>>4)];
		if(cbBuf-iIn==2) {
			pszOut[iOut++]=base64chars[(pBuf[iIn+1]&0xF)<<2];
			pszOut[iOut++]='=';
			break;
		}
		pszOut[iOut++]=base64chars[((pBuf[iIn+1]&0xF)<<2)|(pBuf[iIn+2]>>6)];
		pszOut[iOut++]=base64chars[pBuf[iIn+2]&0x3F];
	}
	pszOut[iOut]='\0';
	return pszOut;
}

struct CreateDialogIdBinaryData {
	int alloced,count;
	PBYTE buf;
	HWND hwndParent;
};

BOOL CALLBACK CreateDlgIdBinEnumProc(HWND hwnd,LPARAM lParam)
{
	struct CreateDialogIdBinaryData *cdib=(struct CreateDialogIdBinaryData*)lParam;
	int type,id;

	if(GetParent(hwnd)!=cdib->hwndParent) return TRUE;
	type=GetControlType(hwnd);
	id=GetWindowLong(hwnd,GWL_ID);
	if(type==CTLTYPE_UNKNOWN || type==CTLTYPE_DIALOG || type==CTLTYPE_TEXT || type==CTLTYPE_GROUP) return TRUE;
	if(cdib->count+3>cdib->alloced) {
		cdib->alloced+=32;
		cdib->buf=(PBYTE)realloc(cdib->buf,cdib->alloced);
	}
	cdib->buf[cdib->count]=(BYTE)type;
	*(PWORD)(cdib->buf+cdib->count+1)=(WORD)id;
	cdib->count+=3;
	return TRUE;
}

//free() the return value
char *CreateDialogIdString(HWND hwndDlg)
{
	struct CreateDialogIdBinaryData cdib={0};
	char *szRet;

	if(hwndDlg==NULL) return NULL;
	cdib.hwndParent=hwndDlg;
	EnumChildWindows(hwndDlg,CreateDlgIdBinEnumProc,(LPARAM)&cdib);
	szRet=Base64Encode(cdib.buf,cdib.count);
	free(cdib.buf);
	return szRet;
}

void AppendCharToCharBuffer(struct ResizableCharBuffer *rcb,char c)
{
	if(rcb->cbAlloced<=rcb->iEnd+1) {
		rcb->cbAlloced+=1024;
		rcb->sz=(char*)realloc(rcb->sz,rcb->cbAlloced);
	}
	rcb->sz[rcb->iEnd++]=c;
	rcb->sz[rcb->iEnd]='\0';
}

void AppendToCharBuffer(struct ResizableCharBuffer *rcb,const char *fmt,...)
{
	va_list va;
	int charsDone;

	if(rcb->cbAlloced==0) {
		rcb->cbAlloced=1024;
		rcb->sz=(char*)malloc(rcb->cbAlloced);
	}
	va_start(va,fmt);
	for(;;) {
		charsDone=_vsnprintf(rcb->sz+rcb->iEnd,rcb->cbAlloced-rcb->iEnd,fmt,va);
		if(charsDone>=0) break;
		rcb->cbAlloced+=1024;
		rcb->sz=(char*)realloc(rcb->sz,rcb->cbAlloced);
	}
	va_end(va);
	rcb->iEnd+=charsDone;
}