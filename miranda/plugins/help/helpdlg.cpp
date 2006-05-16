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
#include <stdio.h>
#include <win2k.h>
#include <newpluginapi.h>
#include <m_utils.h>
#include <m_langpack.h>
#include "help.h"

#include <richedit.h>
#include "resource.h"

extern HINSTANCE hInst;
HWND hwndHelpDlg;

HWND GetControlDialog(HWND hwndCtl)
{
	TCHAR szClassName[32];
	HWND hwndDlg=hwndCtl;
	while(hwndDlg) {
		GetClassName(hwndDlg,szClassName,sizeof(szClassName));
		if(!lstrcmp(szClassName,_T("#32770"))) return hwndDlg;
		hwndDlg=GetParent(hwndDlg);
	}
	return NULL;
}

static int HelpDialogResize(HWND hwndDlg,LPARAM lParam,UTILRESIZECONTROL *urc)
{
	switch(urc->wId) {
		case IDC_DLGID:
		case IDC_CTLTEXT:
		case IDC_MODULE:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_TOP;
		case IDC_TEXT:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

BOOL CALLBACK ShadowDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
		{	BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
			MySetLayeredWindowAttributes=(BOOL (WINAPI *)(HWND,COLORREF,BYTE,DWORD))GetProcAddress(GetModuleHandleA("USER32"),"SetLayeredWindowAttributes");
			if(!MySetLayeredWindowAttributes) {
				DestroyWindow(hwndDlg);
				return TRUE;
			}
			EnableWindow(hwndDlg,FALSE);
			SetWindowLong(hwndDlg,GWL_EXSTYLE,GetWindowLong(hwndDlg,GWL_EXSTYLE)|WS_EX_LAYERED);
			MySetLayeredWindowAttributes(hwndDlg,RGB(0,0,0),96,LWA_ALPHA);
			return FALSE;
		}
		case WM_CTLCOLORDLG:
			return (BOOL)GetStockObject(BLACK_BRUSH);
	}
	return FALSE;
}

BOOL CALLBACK HelpDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	HWND hwndCtl;
#ifndef EDITOR
	static HWND hwndShadowDlg;
	static HCURSOR hHandCursor;
#endif

	hwndCtl=(HWND)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch(message) {
		case WM_INITDIALOG:
			hwndHelpDlg=hwndDlg;
#ifdef EDITOR
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_DLGID),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_DLGID),GWL_STYLE)|SS_ENDELLIPSIS);
			SendDlgItemMessage(hwndDlg,IDC_TEXT,EM_SETEVENTMASK,0,ENM_KEYEVENTS);
			{	RECT rcDlg,rcWork;
				SystemParametersInfo(SPI_GETWORKAREA,0,&rcWork,FALSE);
				GetWindowRect(hwndDlg,&rcDlg);
				SetWindowPos(hwndDlg,0,rcDlg.left,rcWork.bottom-rcDlg.bottom+rcDlg.top,0,0,SWP_NOZORDER|SWP_NOSIZE);
			}
#else
			SendDlgItemMessage(hwndDlg,IDC_TEXT,EM_SETEVENTMASK,0,ENM_REQUESTRESIZE|ENM_LINK);
			SendDlgItemMessage(hwndDlg,IDC_TEXT,EM_SETBKGNDCOLOR,0,GetSysColor(COLOR_INFOBK));
			hwndShadowDlg=CreateDialog(hInst,MAKEINTRESOURCE(IDD_SHADOW),hwndDlg,ShadowDlgProc);
			hHandCursor=LoadCursor(NULL,IDC_HAND);
			if(hHandCursor==NULL) LoadCursor(GetModuleHandle(NULL),MAKEINTRESOURCE(214));
#endif
			return TRUE;
		case WM_SIZE:
		{	UTILRESIZEDIALOG urd={0};
			urd.cbSize=sizeof(urd);
			urd.hInstance=hInst;
			urd.hwndDlg=hwndDlg;
			urd.lParam=0;
			urd.lpTemplate=MAKEINTRESOURCEA(IDD_HELP);
			urd.pfnResizer=HelpDialogResize;
			CallService(MS_UTILS_RESIZEDIALOG,0,(LPARAM)&urd);
			InvalidateRect(hwndDlg,NULL,TRUE);
#ifdef EDITOR
			break;
#endif
		}
#ifndef EDITOR
		case WM_MOVE:
			if(IsWindow(hwndShadowDlg)) {
				RECT rc;
				HRGN hRgnShadow,hRgnDlg;
				GetWindowRect(hwndDlg,&rc);
				hRgnShadow=CreateRectRgnIndirect(&rc);
				OffsetRgn(hRgnShadow,5,5);
				hRgnDlg=CreateRectRgnIndirect(&rc);
				CombineRgn(hRgnShadow,hRgnShadow,hRgnDlg,RGN_DIFF);
				DeleteObject(hRgnDlg);
				OffsetRgn(hRgnShadow,-rc.left-5,-rc.top-5);
				SetWindowRgn(hwndShadowDlg,hRgnShadow,FALSE);
				SetWindowPos(hwndShadowDlg,HWND_TOPMOST,rc.left+5,rc.top+5,rc.right-rc.left,rc.bottom-rc.top,SWP_SHOWWINDOW);
				SetWindowPos(hwndDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			}
			break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			SetBkColor((HDC)wParam,GetSysColor(COLOR_INFOBK));
			return (BOOL)GetSysColorBrush(COLOR_INFOBK);
		case WM_ACTIVATE:
			if(LOWORD(wParam)==WA_INACTIVE) {
				if(GetParent((HWND)lParam)==hwndDlg) break;
				DestroyWindow(hwndDlg);
			}
			break;
#endif	  //!defined EDITOR
		case M_CHANGEHELPCONTROL:
			SetWindowLong(hwndDlg,GWL_USERDATA,lParam);
			hwndCtl=(HWND)lParam;
			SetDlgItemText(hwndDlg,IDC_CTLTEXT,_T(""));
			#ifdef EDITOR
			{	char text[4096],szModule[512];
				char *szDlgId;
				GetControlTitle(hwndCtl,text,sizeof(text));
				SetDlgItemText(hwndDlg,IDC_CTLTEXT,text);
				wsprintf(text,Translate("Ctl ID: %d"),GetWindowLong(hwndCtl,GWL_ID));
				SetDlgItemText(hwndDlg,IDC_CTLID,text);
				szDlgId=CreateDialogIdString(GetControlDialog(hwndCtl));
				if(szDlgId==NULL)
					SetDlgItemText(hwndDlg,IDC_DLGID,Translate("Dialog ID: <not in dialog>"));
				else {
					wsprintf(text,Translate("Dialog ID: %s"),szDlgId);
					free(szDlgId);
					SetDlgItemText(hwndDlg,IDC_DLGID,text);
				}
				wsprintf(text,Translate("Type: %s"),Translate(szControlTypeNames[GetControlType(hwndCtl)]));
				SetDlgItemText(hwndDlg,IDC_CTLTYPE,text);
				GetModuleFileName((HINSTANCE)GetWindowLong(hwndCtl,GWL_HINSTANCE),szModule,sizeof(szModule));
				szDlgId=strrchr(szModule,'\\');
				if(szDlgId==NULL) szDlgId=szModule;
				else szDlgId++;
				wsprintf(text,Translate("Module: %s"),szDlgId);
				SetDlgItemText(hwndDlg,IDC_MODULE,text);
			}
			#endif	  //defined EDITOR
			SetDlgItemText(hwndDlg,IDC_TEXT,_T(""));
			SendMessage(hwndDlg,M_LOADHELP,0,0);
#ifdef EDITOR
			ShowWindow(hwndDlg,SW_SHOWNORMAL);
#else
			SetFocus(GetDlgItem(hwndDlg,IDC_CARETSUCKER));
			SendDlgItemMessage(hwndDlg,IDC_TEXT,EM_REQUESTRESIZE,0,0);
#endif
			return TRUE;
		case M_HELPDOWNLOADFAILED:
#ifdef EDITOR
			EnableWindow(GetDlgItem(hwndDlg,IDC_TEXT),TRUE);
			{	char text[4096];
				GetControlTitle(hwndCtl,text,sizeof(text));
				SetDlgItemText(hwndDlg,IDC_CTLTEXT,text);
			}
#else
			SetDlgItemText(hwndDlg,IDC_CTLTEXT,TranslateT("No help available"));
#endif
			SetDlgItemText(hwndDlg,IDC_TEXT,_T(""));
			break;
#ifdef EDITOR
		case M_UPLOADCOMPLETE:
#endif
		case M_HELPDOWNLOADED:
		case M_LOADHELP:
		{	char *pszText,*pszTitle;
			char *szDlgId,szModule[512],*pszModuleName;
			int downloading;

#ifdef EDITOR
			EnableWindow(GetDlgItem(hwndDlg,IDC_TEXT),TRUE);
#endif
			szDlgId=CreateDialogIdString(GetControlDialog(hwndCtl));
			if(szDlgId==NULL) break;
			GetModuleFileNameA((HINSTANCE)GetWindowLong(hwndCtl,GWL_HINSTANCE),szModule,sizeof(szModule));
			pszModuleName=strrchr(szModule,'\\');
			if(pszModuleName==NULL) pszModuleName=szModule;
			else pszModuleName++;
			downloading=GetControlHelp(szDlgId,pszModuleName,GetWindowLong(hwndCtl,GWL_ID),&pszTitle,&pszText,NULL,message==M_HELPDOWNLOADED?GCHF_DONTDOWNLOAD:0);
			free(szDlgId);
			if(!downloading) {
				if(pszTitle) SetDlgItemTextA(hwndDlg,IDC_CTLTEXT,pszTitle);
				else SetDlgItemText(hwndDlg,IDC_CTLTEXT,_T(""));
				if(pszText) StreamInHtml(GetDlgItem(hwndDlg,IDC_TEXT),pszText);
				else SetDlgItemText(hwndDlg,IDC_TEXT,_T(""));
			}
			else {
				if(message==M_HELPDOWNLOADED) {
					SetDlgItemText(hwndDlg,IDC_TEXT,_T(""));
#ifndef EDITOR
					SetDlgItemText(hwndDlg,IDC_CTLTEXT,TranslateT("No help available"));
#endif
				}
				else {
#ifdef EDITOR
					EnableWindow(GetDlgItem(hwndDlg,IDC_TEXT),FALSE);
					SetDlgItemText(hwndDlg,IDC_TEXT,TranslateT("Downloading..."));
#else
					SetDlgItemText(hwndDlg,IDC_CTLTEXT,TranslateT("Downloading..."));
					SetDlgItemText(hwndDlg,IDC_TEXT,_T(""));
#endif
				}
			}
			break;
		}
		case WM_NOTIFY:
			switch(((NMHDR*)lParam)->idFrom) {
				case IDC_TEXT:
					switch(((NMHDR*)lParam)->code) {
#ifdef EDITOR
						case EN_MSGFILTER:
							switch(((MSGFILTER*)lParam)->msg) {
								case WM_CHAR:
									switch(((MSGFILTER*)lParam)->wParam) {
										case 'B'-'A'+1:
										case 'I'-'A'+1:
										case 'U'-'A'+1:
										case 'H'-'A'+1:
										case 'L'-'A'+1:
										case 'S'-'A'+1:
										case 'G'-'A'+1:
											SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
											return TRUE;
									}
									break;
								case WM_KEYDOWN:
								{	CHARFORMAT cf={0};
									int changes=0;
									if(!(GetKeyState(VK_CONTROL)&0x8000)) break;
									cf.cbSize=sizeof(cf);
									SendDlgItemMessage(hwndDlg,IDC_TEXT,EM_GETCHARFORMAT,TRUE,(LPARAM)&cf);
									switch(((MSGFILTER*)lParam)->wParam) {
										case 'B':
											cf.dwEffects^=CFE_BOLD;
											cf.dwMask=CFM_BOLD;
											changes=1;
											break;
										case 'I':
											cf.dwEffects^=CFE_ITALIC;
											cf.dwMask=CFM_ITALIC;
											changes=1;
											break;
										case 'U':
											cf.dwEffects^=CFE_UNDERLINE;
											cf.dwMask=CFM_UNDERLINE;
											changes=1;
											break;
										case 'L':
										{	CHOOSECOLOR cc={0};
											COLORREF custCol[16]={0};
											cc.lStructSize=sizeof(cc);
											cc.hwndOwner=hwndDlg;
											cc.lpCustColors=custCol;
											cc.rgbResult=cf.crTextColor;
											cc.Flags=CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT;
											if(!ChooseColor(&cc)) break;
											cf.crTextColor=0;
											cf.dwEffects=0;
											if(cc.rgbResult) cf.crTextColor=cc.rgbResult;
											else cf.dwEffects=CFE_AUTOCOLOR;
											cf.dwMask=CFM_COLOR;
											changes=1;
											break;
										}
										case 'H':
											cf.dwEffects^=CFE_STRIKEOUT;
											cf.dwMask=CFM_STRIKEOUT;
											changes=1;
											break;
										case VK_OEM_PLUS:
											cf.yHeight=GetKeyState(VK_SHIFT)&0x8000?TEXTSIZE_BIG*10:TEXTSIZE_NORMAL*10;
											cf.dwMask=CFM_SIZE;
											changes=1;
											break;
										case VK_OEM_MINUS:
											cf.yHeight=TEXTSIZE_SMALL*10;
											cf.dwMask=CFM_SIZE;
											changes=1;
											break;
										case 'S':
										{	char *szText,szTitle[1024];
											char *szDlgId,szModule[512],*pszModuleName;
											GetDlgItemText(hwndDlg,IDC_CTLTEXT,szTitle,sizeof(szTitle));
											szDlgId=CreateDialogIdString(GetControlDialog(hwndCtl));
											if(szDlgId==NULL) break;
											szText=StreamOutHtml(GetDlgItem(hwndDlg,IDC_TEXT));
											GetModuleFileName((HINSTANCE)GetWindowLong(hwndCtl,GWL_HINSTANCE),szModule,sizeof(szModule));
											pszModuleName=strrchr(szModule,'\\');
											if(pszModuleName==NULL) pszModuleName=szModule;
											else pszModuleName++;
											SetControlHelp(szDlgId,pszModuleName,GetWindowLong(hwndCtl,GWL_ID),szTitle,szText,GetControlType(hwndCtl));
											free(szText);
											free(szDlgId);
											UploadDialogCache();
											SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
											return TRUE;
										}
										case 'G':
											SendMessage(hwndDlg,M_LOADHELP,0,0);
											SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
											return TRUE;
									}
									if(changes) {
										SendDlgItemMessage(hwndDlg,IDC_TEXT,EM_SETCHARFORMAT,SCF_WORD|SCF_SELECTION,(LPARAM)&cf);
										SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
										return TRUE;
									}
									break;
								}
							}
							break;
#else  //defined EDITOR
						case EN_REQUESTRESIZE:
						{	RECT rcDlg,rcEdit,rcCtl,rcNew;
							REQRESIZE *rr=(REQRESIZE*)lParam;
							POINT ptScreenBottomRight;
							GetWindowRect(hwndDlg,&rcDlg);
							GetWindowRect(hwndCtl,&rcCtl);
							GetWindowRect(GetDlgItem(hwndDlg,IDC_TEXT),&rcEdit);
							rcNew.left=rcCtl.left+30;
							rcNew.top=rcCtl.bottom+10;
							rcNew.right=rcNew.left+(rr->rc.right-rr->rc.left)+(rcDlg.right-rcDlg.left)-(rcEdit.right-rcEdit.left)+(GetWindowLong(GetDlgItem(hwndDlg,IDC_TEXT),GWL_STYLE)&WS_VSCROLL?GetSystemMetrics(SM_CXVSCROLL):0);
							if(GetWindowTextLength(GetDlgItem(hwndDlg,IDC_TEXT)))
								rcNew.bottom=rcNew.top+min(GetSystemMetrics(SM_CYSCREEN)/5,(rr->rc.bottom-rr->rc.top)+(rcDlg.bottom-rcDlg.top)-(rcEdit.bottom-rcEdit.top));
							else
								rcNew.bottom=rcNew.top+min(GetSystemMetrics(SM_CYSCREEN)/5,(rcDlg.bottom-rcDlg.top)-(rcEdit.bottom-rcEdit.top));
							if(GetSystemMetrics(SM_CXVIRTUALSCREEN)) {
								ptScreenBottomRight.x=GetSystemMetrics(SM_CXVIRTUALSCREEN)+GetSystemMetrics(SM_XVIRTUALSCREEN);
								ptScreenBottomRight.y=GetSystemMetrics(SM_CYVIRTUALSCREEN)+GetSystemMetrics(SM_YVIRTUALSCREEN);
							}
							else {
								ptScreenBottomRight.x=GetSystemMetrics(SM_CXSCREEN);
								ptScreenBottomRight.y=GetSystemMetrics(SM_CYSCREEN);
							}
							if(rcNew.right>=ptScreenBottomRight.x) OffsetRect(&rcNew,ptScreenBottomRight.x-rcNew.right,0);
							if(rcNew.bottom>=ptScreenBottomRight.y) OffsetRect(&rcNew,0,ptScreenBottomRight.y-rcNew.bottom);
							SetWindowPos(hwndDlg,0,rcNew.left,rcNew.top,rcNew.right-rcNew.left,rcNew.bottom-rcNew.top,SWP_NOZORDER);
							ShowWindow(hwndDlg,SW_SHOWNORMAL);
							break;
						}
						case EN_LINK:
							switch(((ENLINK*)lParam)->msg) {
								case WM_SETCURSOR:
									SetCursor(hHandCursor);
									SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
									return TRUE;
								case WM_LBUTTONUP:
								{	CHARRANGE sel;
									char *pszLink;

									SendDlgItemMessage(hwndDlg,IDC_TEXT,EM_EXGETSEL,0,(LPARAM)&sel);
									if(sel.cpMin!=sel.cpMax) break;
									if(IsHyperlink(((ENLINK*)lParam)->chrg.cpMin,((ENLINK*)lParam)->chrg.cpMax,&pszLink)) {
										CallService(MS_UTILS_OPENURL,1,(LPARAM)pszLink);
									}
									break;
								}
							}
							break;
#endif //defined EDITOR
					}
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					DestroyWindow(hwndDlg);
					break;
			}
			break;
		case WM_CLOSE:
			DestroyWindow(hwndDlg);
			break;
		case WM_DESTROY:
#ifndef EDITOR
			FreeHyperlinkData();
			DestroyCursor(hHandCursor);
#endif
			hwndHelpDlg=NULL;
			break;
	}
	return FALSE;
}