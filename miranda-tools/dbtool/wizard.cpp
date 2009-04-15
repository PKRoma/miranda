/*
Miranda Database Tool
Copyright (C) 2001-2005  Richard Hughes

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
#include "dbtool.h"

static HFONT hBoldFont=NULL;
static HENHMETAFILE hEmfHeaderLogo=NULL;

static BOOL CALLBACK MyControlsEnumChildren(HWND hwnd,LPARAM lParam)
{
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	DWORD exstyle=GetWindowLong(hwnd,GWL_EXSTYLE);
	char szClass[64];
	int makeBold=0;

	GetClassNameA(hwnd,szClass,sizeof(szClass));
	if(!strcmp(szClass,"Static")) {
		if(((style&SS_TYPEMASK)==SS_LEFT || (style&SS_TYPEMASK)==SS_CENTER || (style&SS_TYPEMASK)==SS_RIGHT) && exstyle&WS_EX_CLIENTEDGE)
			makeBold=1;
	}
	else if(!strcmp(szClass,"Button")) {
		if(exstyle&WS_EX_CLIENTEDGE)
			makeBold=1;
	}
	if(makeBold) {
		if(hBoldFont==NULL) {
			LOGFONT lf;
			hBoldFont=(HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
			GetObject(hBoldFont,sizeof(lf),&lf);
			lf.lfWeight=FW_BOLD;
			hBoldFont=CreateFontIndirect(&lf);
		}
		SendMessage(hwnd,WM_SETFONT,(WPARAM)hBoldFont,0);
		SetWindowLong(hwnd,GWL_EXSTYLE,exstyle&~WS_EX_CLIENTEDGE);
		SetWindowPos(hwnd,0,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
	}
	return TRUE;
}

int DoMyControlProcessing(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam,INT_PTR *bReturn)
{
	switch(message) {
		case WM_INITDIALOG:
			EnumChildWindows(hdlg,MyControlsEnumChildren,0);
			if(hEmfHeaderLogo==NULL) {
				HRSRC hRsrc=FindResourceA(hInst,MAKEINTRESOURCEA(IDE_HDRLOGO),"EMF");
				HGLOBAL hGlob=LoadResource(hInst,hRsrc);
				hEmfHeaderLogo=SetEnhMetaFileBits(SizeofResource(hInst,hRsrc),(PBYTE)LockResource(hGlob));
			}
			SendDlgItemMessage(hdlg,IDC_HDRLOGO,STM_SETIMAGE,IMAGE_ENHMETAFILE,(LPARAM)hEmfHeaderLogo);
			break;
		case WM_CTLCOLORSTATIC:
			if((GetWindowLong((HWND)lParam,GWL_STYLE)&0xFFFF)==0) {
				char szText[256];
				GetWindowTextA((HWND)lParam,szText,sizeof(szText));
				if(!strcmp(szText,"whiterect")) {
					SetTextColor((HDC)wParam,RGB(255,255,255));
					SetBkColor((HDC)wParam,RGB(255,255,255));
					SetBkMode((HDC)wParam,OPAQUE);
					*bReturn=(INT_PTR)GetStockObject(WHITE_BRUSH);
					return TRUE;
				}
				else {
					SetBkMode((HDC)wParam,TRANSPARENT);
					*bReturn=(INT_PTR)GetStockObject(NULL_BRUSH);
					return TRUE;
				}
			}
			break;
	}
	return FALSE;
}

INT_PTR CALLBACK WizardDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	static HWND hdlgPage;

	switch(message) {
		case WM_INITDIALOG:
			SendMessage(hdlg,WM_SETICON,ICON_SMALL,(LPARAM)LoadIcon(hInst,MAKEINTRESOURCE(IDI_DBTOOL)));
			hdlgPage=NULL;
			SendMessage(hdlg,WZM_GOTOPAGE,IDD_WELCOME,(LPARAM)WelcomeDlgProc);
			TranslateDialog(hdlg);
			return TRUE;
		case WZM_GOTOPAGE:
			if(hdlgPage!=NULL) DestroyWindow(hdlgPage);
			EnableWindow(GetDlgItem(hdlg,IDC_BACK),TRUE);
			EnableWindow(GetDlgItem(hdlg,IDOK),TRUE);
			EnableWindow(GetDlgItem(hdlg,IDCANCEL),TRUE);
			SetDlgItemText(hdlg,IDCANCEL,TranslateT("Cancel"));
			hdlgPage=CreateDialog(hInst,MAKEINTRESOURCE(wParam),hdlg,(DLGPROC)lParam);
			TranslateDialog(hdlgPage);
			SetWindowPos(hdlgPage,0,0,0,0,0,SWP_NOZORDER|SWP_NOSIZE);
			ShowWindow(hdlgPage,SW_SHOW);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BACK:
				case IDOK:
					SendMessage(hdlgPage,WZN_PAGECHANGING,wParam,0);
					SendMessage(hdlgPage,message,wParam,lParam);
					break;
				case IDCANCEL:
					if(SendMessage(hdlgPage,WZN_CANCELCLICKED,0,0)) break;
					EndDialog(hdlg,0);
					break;
			}
			break;
		case WM_DESTROY:
			if (opts.hFile)
				CloseHandle(opts.hFile);
			if (opts.hOutFile)
				CloseHandle(opts.hOutFile);
			DestroyWindow(hdlgPage);
			LangPackShutdown();
			if(hBoldFont!=NULL) DeleteObject(hBoldFont);
			if(hEmfHeaderLogo!=NULL) DeleteEnhMetaFile(hEmfHeaderLogo);
			break;
	}
	return FALSE;
}
