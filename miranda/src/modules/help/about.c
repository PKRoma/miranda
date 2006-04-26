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
#include "commonheaders.h"

#if defined( _UNICODE )
	#define STR_VERSION_FORMAT "%s %S"
#else
	#define STR_VERSION_FORMAT "%s %s"
#endif

BOOL CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int iState = 0;
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{	int h;
			LOGFONT lf;
			HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg,IDC_MIRANDA,WM_GETFONT,0,0);
			int iState=0;
			GetObject(hFont,sizeof(lf),&lf);
			h=lf.lfHeight;
			lf.lfHeight=(int)(lf.lfHeight*1.5);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,IDC_MIRANDA,WM_SETFONT,(WPARAM)hFont,0);
			lf.lfHeight=h;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_SETFONT,(WPARAM)hFont,0);
		}
		{	char productVersion[56];
			TCHAR str[64];
			CallService(MS_SYSTEM_GETVERSIONTEXT,SIZEOF(productVersion),(LPARAM)productVersion);
			mir_sntprintf(str,SIZEOF(str),_T(STR_VERSION_FORMAT), TranslateT("Version"), productVersion);
			SetDlgItemText(hwndDlg,IDC_VERSION,str);
			mir_sntprintf(str,SIZEOF(str),TranslateT("Built %s %s"),_T(__DATE__),_T(__TIME__));
			SetDlgItemText(hwndDlg,IDC_BUILDTIME,str);
		}
		ShowWindow(GetDlgItem(hwndDlg, IDC_CREDITSFILE), SW_HIDE);
		{	char* pszMsg = LockResource(LoadResource(GetModuleHandle(NULL),FindResource(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_CREDITS),_T("TEXT"))));
			#if defined( _UNICODE )
				TCHAR* ptszMsg = alloca(2000*sizeof(TCHAR));
            MultiByteToWideChar(1252,0,pszMsg,-1,ptszMsg,2000);
            SetDlgItemText(hwndDlg,IDC_CREDITSFILE, ptszMsg);
			#else
            SetDlgItemText(hwndDlg,IDC_CREDITSFILE, pszMsg);
			#endif
		}
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MIRANDA)));
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			return TRUE;
		case IDC_CONTRIBLINK:
			{
				if (iState) {
					iState = 0;
					SetDlgItemText(hwndDlg, IDC_CONTRIBLINK, TranslateT("Credits >"));
					ShowWindow(GetDlgItem(hwndDlg, IDC_DEVS), SW_SHOW);
					ShowWindow(GetDlgItem(hwndDlg, IDC_BUILDTIME), SW_SHOW);
					ShowWindow(GetDlgItem(hwndDlg, IDC_CREDITSFILE), SW_HIDE);
				}
				else {
					iState = 1;
					SetDlgItemText(hwndDlg, IDC_CONTRIBLINK, TranslateT("< About"));
					ShowWindow(GetDlgItem(hwndDlg, IDC_DEVS), SW_HIDE);
					ShowWindow(GetDlgItem(hwndDlg, IDC_BUILDTIME), SW_HIDE);
					ShowWindow(GetDlgItem(hwndDlg, IDC_CREDITSFILE), SW_SHOW);
				}
				break;
			}
		}
		break;
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_MIRANDA)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_VERSION)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_BUILDTIME)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_CREDITSFILE)
			|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_DEVS)) {
				if((HWND)lParam==GetDlgItem(hwndDlg,IDC_MIRANDA))
					SetTextColor((HDC)wParam,RGB(180,10,10));
				else if((HWND)lParam==GetDlgItem(hwndDlg,IDC_VERSION))
					SetTextColor((HDC)wParam,RGB(70,70,70));
				else
					SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
			break;
	case WM_DESTROY:
		{	HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg,IDC_MIRANDA,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_MIRANDA,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				

			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
		}
		break;
	}
	return FALSE;
}
