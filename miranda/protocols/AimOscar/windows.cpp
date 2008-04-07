#include "aim.h"
#include "windows.h"

HANDLE hThemeButton = NULL;
COLORREF foreground=0;
COLORREF background=0xffffff;
COLORREF custColours[16]={0};

static int CALLBACK EnumFontsProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX* /*lpntme*/, int /*FontType*/, LPARAM lParam)
{
	if ( !IsWindow((HWND) lParam))
		return FALSE;
	if (SendMessage((HWND) lParam, CB_FINDSTRINGEXACT, 1, (LPARAM) lpelfe->elfLogFont.lfFaceName) == CB_ERR)
		SendMessage((HWND) lParam, CB_ADDSTRING, 0, (LPARAM) lpelfe->elfLogFont.lfFaceName);
	return TRUE;
}

void DrawMyControl(HDC hDC, HWND /*hwndButton*/, HANDLE hTheme, UINT iState, RECT rect)
{
	BOOL bIsPressed = (iState & ODS_SELECTED);
	BOOL bIsFocused  = (iState & ODS_FOCUS);
	if (hTheme)
	{
		DWORD state = (bIsPressed)?PBS_PRESSED:PBS_NORMAL;
		if(state == PBS_NORMAL)
		{
			if(bIsFocused)
				state = PBS_DEFAULTED;
		}
		rect.top-=1;
		rect.left-=1;
		MyDrawThemeBackground(hTheme, hDC, BP_PUSHBUTTON,state, &rect, NULL);
	}
	else
	{
		if (bIsFocused)
		{
			HBRUSH br = CreateSolidBrush(RGB(0,0,0));  
			FrameRect(hDC, &rect, br);
			InflateRect(&rect, -1, -1);
			DeleteObject(br);
		} // if		
		COLORREF crColor = GetSysColor(COLOR_BTNFACE);
		HBRUSH	brBackground = CreateSolidBrush(crColor);
		FillRect(hDC,&rect, brBackground);
		DeleteObject(brBackground);
		// Draw pressed button
		if (bIsPressed)
		{
			HBRUSH brBtnShadow = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
			FrameRect(hDC, &rect, brBtnShadow);
			DeleteObject(brBtnShadow);
		}
		else // ...else draw non pressed button
		{
			UINT uState = DFCS_BUTTONPUSH|(bIsPressed? DFCS_PUSHED : 0);
			DrawFrameControl(hDC, &rect, DFC_BUTTON, uState);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// User info dialog

static BOOL CALLBACK userinfo_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CAimProto* ppro = (CAimProto*)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		{
			SendDlgItemMessage(hwndDlg, IDC_BOLD, BUTTONSETASPUSHBTN, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETEVENTMASK, 0, ENM_CHANGE|ENM_SELCHANGE|ENM_REQUESTRESIZE);
			SendDlgItemMessage(hwndDlg, IDC_BACKGROUNDCOLORPICKER, CPM_SETCOLOUR, 0, 0x00ffffff);
			LOGFONT lf ={0};
			HDC hdc = GetDC(hwndDlg);
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfFaceName[0] = 0;
			lf.lfPitchAndFamily = 0;
			EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC) EnumFontsProc, (LPARAM) GetDlgItem(hwndDlg, IDC_TYPEFACE), 0);
			ReleaseDC(hwndDlg, hdc);
			SendMessage( GetDlgItem(hwndDlg, IDC_FONTSIZE), CB_ADDSTRING, 0, (LPARAM)"8");
			SendMessage( GetDlgItem(hwndDlg, IDC_FONTSIZE), CB_ADDSTRING, 0, (LPARAM)"10");
			SendMessage( GetDlgItem(hwndDlg, IDC_FONTSIZE), CB_ADDSTRING, 0, (LPARAM)"12");
			SendMessage( GetDlgItem(hwndDlg, IDC_FONTSIZE), CB_ADDSTRING, 0, (LPARAM)"14");
			SendMessage( GetDlgItem(hwndDlg, IDC_FONTSIZE), CB_ADDSTRING, 0, (LPARAM)"18");
			SendMessage( GetDlgItem(hwndDlg, IDC_FONTSIZE), CB_ADDSTRING, 0, (LPARAM)"24");
			SendMessage( GetDlgItem(hwndDlg, IDC_FONTSIZE), CB_ADDSTRING, 0, (LPARAM)"36");
			if(SendDlgItemMessage(hwndDlg, IDC_TYPEFACE, CB_SELECTSTRING, 1, (LPARAM)"Arial")!=CB_ERR)
			{
				CHARFORMAT2A cf;
				cf.cbSize = sizeof(cf);
				cf.yHeight=12*20;
				cf.dwMask=CFM_SIZE|CFM_FACE;
				strlcpy(cf.szFaceName,"Arial",7);
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
			}
			else
			{
				CHARFORMAT2A cf;
				cf.cbSize = sizeof(cf);
				cf.yHeight=12*20;
				cf.dwMask=CFM_SIZE;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
			}
			break;
		}
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;

	case WM_SIZING:
		{
			RECT* rect=(RECT*)lParam;
#define MIN_HEIGHT 200
#define MIN_WIDTH 400
			if(WMSZ_RIGHT==wParam||WMSZ_TOPRIGHT==wParam||WMSZ_BOTTOMRIGHT==wParam)
			{
				if(rect->right-rect->left<MIN_WIDTH)
					rect->right=rect->left+MIN_WIDTH;
			}
			if(WMSZ_LEFT==wParam||WMSZ_TOPLEFT==wParam||WMSZ_BOTTOMLEFT==wParam)
			{
				if(rect->right-rect->left<MIN_WIDTH)
					rect->left=rect->right-MIN_WIDTH;
			}
			if(WMSZ_TOP==wParam||WMSZ_TOPRIGHT==wParam||WMSZ_TOPLEFT==wParam)
			{
				if(rect->bottom-rect->top<MIN_HEIGHT)
					rect->top=rect->bottom-MIN_HEIGHT;
			}
			if(WMSZ_BOTTOM==wParam||WMSZ_BOTTOMLEFT==wParam||WMSZ_BOTTOMRIGHT==wParam)
			{
				if(rect->bottom-rect->top<MIN_HEIGHT)
					rect->bottom=rect->top+MIN_HEIGHT;
			}
			break;
		}
	case WM_SIZE:
		{
			int width=LOWORD(lParam);
			int height=HIWORD(lParam);
			SetWindowPos(GetDlgItem(hwndDlg, IDC_PROFILE),HWND_TOP,11,34,width-21,height-83,0);
			SetWindowPos(GetDlgItem(hwndDlg, IDC_SETPROFILE),HWND_TOP,width-134,height-46,0,0,SWP_NOSIZE);
			break;
		}
	case WM_NOTIFY:
		switch (LOWORD(wParam)) {
		case IDC_PROFILE:
			if(((LPNMHDR)lParam)->code==EN_SELCHANGE)
			{
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_FACE | CFM_SIZE;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				if(SendDlgItemMessage(hwndDlg, IDC_TYPEFACE, CB_SELECTSTRING, 1, (LPARAM)cfOld.szFaceName)==-1)
				{
					SendDlgItemMessage(hwndDlg, IDC_TYPEFACE, CB_ADDSTRING, 0, (LPARAM)cfOld.szFaceName);
					SendDlgItemMessage(hwndDlg, IDC_TYPEFACE, CB_SELECTSTRING, 1, (LPARAM)cfOld.szFaceName);
				}
				char size[10];
				_itoa(cfOld.yHeight/20,size,sizeof(size));
				//SetDlgItemText(hwndDlg, IDC_FONTSIZE, size);
				SendDlgItemMessage(hwndDlg, IDC_FONTSIZE, CB_SELECTSTRING, 1, (LPARAM)size);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_SUPERSCRIPT), NULL, FALSE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_NORMALSCRIPT), NULL, FALSE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_SUBSCRIPT), NULL, FALSE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_BOLD), NULL, FALSE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_ITALIC), NULL, FALSE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_UNDERLINE), NULL, FALSE);
			}
			else if(((LPNMHDR)lParam)->code==EN_REQUESTRESIZE)
			{
				//	REQRESIZE* rr= (REQRESIZE*)lParam;
				//SetWindowPos(GetDlgItem(hwndDlg, IDC_PROFILE),HWND_TOP,rr->rc.left,rr->rc.top,rr->rc.right,rr->rc.bottom,0);
			}
			break;

		default:
			if (((LPNMHDR)lParam)->code == PSN_PARAMCHANGED ) {
				ppro = ( CAimProto* )(( PSHNOTIFY* )lParam )->lParam;
				SetWindowLong(hwndDlg, GWL_USERDATA, LPARAM( ppro ));

				DBVARIANT dbv;
				if ( !DBGetContactSettingString(NULL, ppro->m_szModuleName, AIM_KEY_PR, &dbv))
				{
					SetDlgItemTextA(hwndDlg, IDC_PROFILE, dbv.pszVal);
					DBFreeVariant(&dbv);
				}
			}
		}
		break;

	case WM_DRAWITEM:
		{
			if(themeAPIHandle)
			{
				MyCloseThemeData (hThemeButton);
				hThemeButton = MyOpenThemeData (GetDlgItem(hwndDlg, IDC_BOLD), L"Button");
			}
			LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT) lParam;
			if(lpDIS->CtlID == IDC_SUPERSCRIPT)
			{
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_SUPERSCRIPT;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				BOOL isSuper = (cfOld.dwEffects & CFE_SUPERSCRIPT) && (cfOld.dwMask & CFM_SUPERSCRIPT);
				if(isSuper)
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BOLD),hThemeButton,lpDIS->itemState|ODS_SELECTED, lpDIS->rcItem);	
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("sup_scrpt"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("sup_scrpt");
				}
				else
				{	
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BOLD),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("nsup_scrpt"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("nsup_scrpt");
				}
			}
			else if(lpDIS->CtlID == IDC_NORMALSCRIPT)
			{
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_SUBSCRIPT|CFM_SUPERSCRIPT;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				BOOL isSub = (cfOld.dwEffects & CFE_SUBSCRIPT) && (cfOld.dwMask & CFM_SUBSCRIPT);
				BOOL isSuper = (cfOld.dwEffects & CFE_SUPERSCRIPT) && (cfOld.dwMask & CFM_SUPERSCRIPT);
				if(!isSub&&!isSuper)
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BOLD),hThemeButton,lpDIS->itemState|ODS_SELECTED, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("norm_scrpt"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("norm_scrpt");
				}
				else
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BOLD),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("nnorm_scrpt"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("nnorm_scrpt");
				}
			}
			else if(lpDIS->CtlID == IDC_SUBSCRIPT)
			{
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_SUBSCRIPT;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				BOOL isSub = (cfOld.dwEffects & CFE_SUBSCRIPT) && (cfOld.dwMask & CFM_SUBSCRIPT);
				if(isSub)
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BOLD),hThemeButton,lpDIS->itemState|ODS_SELECTED, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("sub_scrpt"), 16, 16, 0, 0, DI_NORMAL);	
					ppro->ReleaseIconEx("sub_scrpt");
				}
				else
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BOLD),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("nsub_scrpt"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("nsub_scrpt");
				}
			}
			else if(lpDIS->CtlID == IDC_BOLD)
			{
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_BOLD;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				BOOL isBold = (cfOld.dwEffects & CFE_BOLD) && (cfOld.dwMask & CFM_BOLD);
				if(!isBold)
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BOLD),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("nbold"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("nbold");
				}
				else
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BOLD),hThemeButton,lpDIS->itemState|ODS_SELECTED, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("bold"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("bold");
				}
			}
			else if(lpDIS->CtlID == IDC_ITALIC)
			{
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_ITALIC;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				BOOL isItalic = (cfOld.dwEffects & CFE_ITALIC) && (cfOld.dwMask & CFM_ITALIC);
				if(!isItalic)
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_ITALIC),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("nitalic"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("nitalic");
				}
				else
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_ITALIC),hThemeButton,lpDIS->itemState|ODS_SELECTED, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("italic"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("italic");
				}
			}
			else if(lpDIS->CtlID == IDC_UNDERLINE)
			{
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_UNDERLINE;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				BOOL isUnderline = (cfOld.dwEffects & CFE_UNDERLINE) && (cfOld.dwMask & CFM_UNDERLINE);
				if(!isUnderline)
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_UNDERLINE),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("nundrln"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("nundrln");
				}
				else
				{
					DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_UNDERLINE),hThemeButton,lpDIS->itemState|ODS_SELECTED, lpDIS->rcItem);
					DrawIconEx(lpDIS->hDC, 4, 5, ppro->LoadIconEx("undrln"), 16, 16, 0, 0, DI_NORMAL);
					ppro->ReleaseIconEx("undrln");
				}
			}
			else if(lpDIS->CtlID == IDC_FOREGROUNDCOLOR)
			{
				DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_FOREGROUNDCOLOR),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
				DrawIconEx(lpDIS->hDC, 4, 2, ppro->LoadIconEx("foreclr"), 16, 16, 0, 0, DI_NORMAL);
				ppro->ReleaseIconEx("foreclr");
				HBRUSH	hbr = CreateSolidBrush(foreground);
				HPEN hp = CreatePen(PS_SOLID, 1, ~foreground&0x00ffffff);
				SelectObject(lpDIS->hDC,hp);
				RECT rect=lpDIS->rcItem;
				rect.top+=18;
				rect.bottom-=4;
				rect.left+=5;
				rect.right-=5;
				Rectangle(lpDIS->hDC,rect.left-1,rect.top-1,rect.right+1,rect.bottom+1);
				FillRect(lpDIS->hDC,&rect, hbr);
				DeleteObject(hbr);
				DeleteObject(hp);
			}
			else if(lpDIS->CtlID == IDC_FOREGROUNDCOLORPICKER)
			{
				DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_FOREGROUNDCOLORPICKER),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
				HBRUSH	hbr = CreateSolidBrush(foreground);
				HPEN hp = CreatePen(PS_SOLID, 1,~foreground&0x00ffffff);
				SelectObject(lpDIS->hDC,hbr);
				SelectObject(lpDIS->hDC,hp);
				POINT tri[3];
				tri[0].x=3;
				tri[0].y=10;
				tri[1].x=9;
				tri[1].y=10;
				tri[2].x=6;
				tri[2].y=15;
				Polygon(lpDIS->hDC,tri,3);
				DeleteObject(hbr);
				DeleteObject(hp);
			}
			else if(lpDIS->CtlID == IDC_BACKGROUNDCOLOR)
			{
				DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BACKGROUNDCOLOR),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
				DrawIconEx(lpDIS->hDC, 4, 2, ppro->LoadIconEx("backclr"), 16, 16, 0, 0, DI_NORMAL);
				ppro->ReleaseIconEx("backclr");
				HBRUSH	hbr = CreateSolidBrush(background);
				HPEN hp = CreatePen(PS_SOLID, 1, ~background&0x00ffffff);
				SelectObject(lpDIS->hDC,hp);
				RECT rect=lpDIS->rcItem;
				rect.top+=18;
				rect.bottom-=4;
				rect.left+=5;
				rect.right-=5;
				Rectangle(lpDIS->hDC,rect.left-1,rect.top-1,rect.right+1,rect.bottom+1);
				FillRect(lpDIS->hDC,&rect, hbr);
				DeleteObject(hbr);
				DeleteObject(hp);
			}
			else if(lpDIS->CtlID == IDC_BACKGROUNDCOLORPICKER)
			{
				DrawMyControl(lpDIS->hDC,GetDlgItem(hwndDlg, IDC_BACKGROUNDCOLORPICKER),hThemeButton,lpDIS->itemState, lpDIS->rcItem);
				HBRUSH	hbr = CreateSolidBrush(background);
				HPEN hp = CreatePen(PS_SOLID, 1,~background&0x00ffffff);
				SelectObject(lpDIS->hDC,hbr);
				SelectObject(lpDIS->hDC,hp);
				POINT tri[3];
				tri[0].x=3;
				tri[0].y=10;
				tri[1].x=9;
				tri[1].y=10;
				tri[2].x=6;
				tri[2].y=15;
				Polygon(lpDIS->hDC,tri,3);
				DeleteObject(hbr);
				DeleteObject(hp);
			}
			break;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROFILE:
			if (HIWORD(wParam) == EN_CHANGE)
				EnableWindow(GetDlgItem(hwndDlg, IDC_SETPROFILE), TRUE);
			break;

		case IDC_SETPROFILE:
			{
				char* buf=rtf_to_html(hwndDlg,IDC_PROFILE);
				DBWriteContactSettingString(NULL, ppro->m_szModuleName, AIM_KEY_PR, buf);
				if (ppro->state==1)
					ppro->aim_set_profile(ppro->hServerConn,ppro->seqno,buf);//also see set caps for profile setting

				EnableWindow(GetDlgItem(hwndDlg, IDC_SETPROFILE), FALSE);
			}
			break;

		case IDC_SUPERSCRIPT:
			if ( HIWORD(wParam) == BN_CLICKED ) {
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwMask=CFM_SUPERSCRIPT;
				cf.dwEffects=CFE_SUPERSCRIPT;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
				InvalidateRect(GetDlgItem(hwndDlg, IDC_NORMALSCRIPT), NULL, FALSE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_SUBSCRIPT), NULL, FALSE);
			}
			break;

		case IDC_NORMALSCRIPT:
			if ( HIWORD(wParam) == BN_CLICKED ) {
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwMask=CFM_SUPERSCRIPT;
				cf.dwEffects &= ~CFE_SUPERSCRIPT;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
				InvalidateRect(GetDlgItem(hwndDlg, IDC_SUPERSCRIPT), NULL, FALSE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_SUBSCRIPT), NULL, FALSE);
			}
			break;

		case IDC_SUBSCRIPT:
			if ( HIWORD(wParam) == BN_CLICKED ) {
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwMask=CFM_SUBSCRIPT;
				cf.dwEffects=CFE_SUBSCRIPT;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
				InvalidateRect(GetDlgItem(hwndDlg, IDC_SUPERSCRIPT), NULL, FALSE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_NORMALSCRIPT), NULL, FALSE);
			}
			break;

		case IDC_BOLD:
			if ( HIWORD(wParam) == BN_CLICKED ) {
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_BOLD;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				BOOL isBold = (cfOld.dwEffects & CFE_BOLD) && (cfOld.dwMask & CFM_BOLD);
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwEffects = isBold ? 0 : CFE_BOLD;
				cf.dwMask = CFM_BOLD;
				CheckDlgButton(hwndDlg, IDC_BOLD, !isBold);
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
			}
			break;

		case IDC_ITALIC:
			if ( HIWORD(wParam) == BN_CLICKED ) {
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_ITALIC;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				BOOL isItalic = (cfOld.dwEffects & CFE_ITALIC) && (cfOld.dwMask & CFM_ITALIC);
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwEffects = isItalic ? 0 : CFE_ITALIC;
				cf.dwMask = CFM_ITALIC;
				CheckDlgButton(hwndDlg, IDC_ITALIC, !isItalic);
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
			}
			break;

		case IDC_UNDERLINE:
			if(HIWORD(wParam)==BN_CLICKED)
			{
				CHARFORMAT2 cfOld;
				cfOld.cbSize = sizeof(CHARFORMAT2);
				cfOld.dwMask = CFM_UNDERLINE;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
				BOOL isUnderline = (cfOld.dwEffects & CFE_UNDERLINE) && (cfOld.dwMask & CFM_UNDERLINE);
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwEffects = isUnderline ? 0 : CFE_UNDERLINE;
				cf.dwMask = CFM_UNDERLINE;
				CheckDlgButton(hwndDlg, IDC_UNDERLINE, !isUnderline);
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
			}
			break;

		case IDC_FOREGROUNDCOLOR:
			if ( HIWORD(wParam) == BN_CLICKED ) {
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwMask=CFM_COLOR;
				cf.dwEffects=0;
				cf.crTextColor=foreground;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetWindowPos(GetDlgItem(hwndDlg, IDC_FOREGROUNDCOLORPICKER),GetDlgItem(hwndDlg, IDC_FOREGROUNDCOLOR),0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
			}
			break;

		case IDC_FOREGROUNDCOLORPICKER:
			if ( HIWORD(wParam) == BN_CLICKED ) {
				CHOOSECOLOR cc={0};
				custColours[0]=foreground;
				custColours[1]=background;
				cc.lStructSize=sizeof(CHOOSECOLOR);
				cc.hwndOwner=hwndDlg;
				cc.hInstance=(HWND)GetModuleHandle(NULL);
				cc.lpCustColors=custColours;
				cc.Flags=CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT;
				if(ChooseColor(&cc)) {
					foreground=cc.rgbResult;
					InvalidateRect(GetDlgItem(hwndDlg, IDC_FOREGROUNDCOLOR), NULL, FALSE);
				}
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
			}
			break;

		case IDC_BACKGROUNDCOLOR:
			if ( HIWORD(wParam) == BN_CLICKED ) {
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwMask=CFM_BACKCOLOR;
				cf.dwEffects=0;
				cf.crBackColor=background;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
			}
			break;

		case IDC_BACKGROUNDCOLORPICKER:
			if(HIWORD(wParam)==BN_CLICKED)
			{
				CHOOSECOLOR cc={0};
				custColours[0]=foreground;
				custColours[1]=background;
				cc.lStructSize=sizeof(CHOOSECOLOR);
				cc.hwndOwner=hwndDlg;
				cc.hInstance=(HWND)GetModuleHandle(NULL);
				cc.lpCustColors=custColours;
				cc.Flags=CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT;
				if(ChooseColor(&cc))
				{
					background=cc.rgbResult;
					InvalidateRect(GetDlgItem(hwndDlg, IDC_BACKGROUNDCOLOR), NULL, FALSE);
				}
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
			}
			break;

		case IDC_TYPEFACE:
			if(HIWORD(wParam)==CBN_SELENDOK)
			{
				CHARFORMAT2A cf;
				cf.cbSize = sizeof(cf);
				cf.dwMask=CFM_FACE;
				cf.dwEffects=0;
				char face[128];
				SendDlgItemMessage(hwndDlg, IDC_TYPEFACE, CB_GETLBTEXT, SendDlgItemMessage(hwndDlg, IDC_TYPEFACE, CB_GETCURSEL, 0, 0),(LPARAM)face);
				strlcpy(cf.szFaceName,face,strlen(face)+1);
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
				break;
			}
			break;

		case IDC_FONTSIZE:
			if(HIWORD(wParam)==CBN_SELENDOK)
			{
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwMask=CFM_SIZE;
				cf.dwEffects=0;
				char chsize[5];
				SendDlgItemMessage(hwndDlg, IDC_FONTSIZE, CB_GETLBTEXT, SendDlgItemMessage(hwndDlg, IDC_FONTSIZE, CB_GETCURSEL, 0, 0),(LPARAM)chsize);
				//strlcpy(cf.szFaceName,size,strlen(size)+1);
				cf.yHeight=atoi(chsize)*20;
				SendDlgItemMessage(hwndDlg, IDC_PROFILE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				SetFocus(GetDlgItem(hwndDlg, IDC_PROFILE));
				break;
			}
			break;
		}
		break;
	}
	return FALSE;
}

int CAimProto::OnUserInfoInit(WPARAM wParam,LPARAM lParam)
{
	if ( !lParam )//hContact
	{
		OPTIONSDIALOGPAGE odp = { 0 };
		odp.cbSize = sizeof(odp);
		odp.position = -1900000000;
		odp.hInstance = hInstance;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_INFO);
		odp.pszTitle = m_szModuleName;
		odp.dwInitParam = LPARAM(this);
		odp.pfnDlgProc = userinfo_dialog;
		CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM) & odp);
	}
	return 0;
}

int CAimProto::EditProfile(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_AIM), NULL, userinfo_dialog, LPARAM( this ));
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Options dialog

static BOOL CALLBACK options_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CAimProto* ppro = (CAimProto*)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		ppro = (CAimProto*)lParam;
		{
			DBVARIANT dbv;
			if ( !ppro->getString(AIM_KEY_SN, &dbv)) {
				SetDlgItemTextA(hwndDlg, IDC_SN, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if ( !ppro->getString(AIM_KEY_NK, &dbv)) {
				SetDlgItemTextA(hwndDlg, IDC_NK, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			else if ( !ppro->getString(AIM_KEY_SN, &dbv)) {
				SetDlgItemTextA(hwndDlg, IDC_NK, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if ( !ppro->getString(AIM_KEY_PW, &dbv)) {
				CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
				SetDlgItemTextA(hwndDlg, IDC_PW, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if ( !ppro->getString(AIM_KEY_HN, &dbv)) {
				SetDlgItemTextA(hwndDlg, IDC_HN, dbv.pszVal);
				DBFreeVariant(&dbv);
			}

			WORD timeout = (WORD)ppro->getWord( AIM_KEY_GP, DEFAULT_GRACE_PERIOD);
			SetDlgItemInt(hwndDlg, IDC_GP, timeout,0);

			CheckDlgButton(hwndDlg, IDC_DC, ppro->getByte( AIM_KEY_DC, 0));//Message Delivery Confirmation
			CheckDlgButton(hwndDlg, IDC_FP, ppro->getByte( AIM_KEY_FP, 0));//force proxy
			CheckDlgButton(hwndDlg, IDC_AT, ppro->getByte( AIM_KEY_AT, 0));//Account Type Icons
			CheckDlgButton(hwndDlg, IDC_ES, ppro->getByte( AIM_KEY_ES, 0));//Extended Status Type Icons
			CheckDlgButton(hwndDlg, IDC_HF, ppro->getByte( AIM_KEY_HF, 0));//Fake hiptopness
			CheckDlgButton(hwndDlg, IDC_DM, ppro->getByte( AIM_KEY_DM, 0));//Disable Sending Mode Message
			CheckDlgButton(hwndDlg, IDC_FI, ppro->getByte( AIM_KEY_FI, 0));//Format imcoming messages
			CheckDlgButton(hwndDlg, IDC_FO, ppro->getByte( AIM_KEY_FO, 0));//Format outgoing messages
			CheckDlgButton(hwndDlg, IDC_WEBSUPPORT, ppro->getByte( AIM_KEY_AL, 0));
			CheckDlgButton(hwndDlg, IDC_II, ppro->getByte( AIM_KEY_II, 0));//Instant Idle
			CheckDlgButton(hwndDlg, IDC_CM, ppro->getByte( AIM_KEY_CM, 0));//Check Mail
			CheckDlgButton(hwndDlg, IDC_KA, ppro->getByte( AIM_KEY_KA, 0));//Keep Alive
			CheckDlgButton(hwndDlg, IDC_DA, ppro->getByte( AIM_KEY_DA, 0));//Disable Avatars
		}
		break;

	case WM_COMMAND:
		{
			if ((LOWORD(wParam) == IDC_SN || LOWORD(wParam) == IDC_NK || LOWORD(wParam) == IDC_PW || LOWORD(wParam) == IDC_HN
				|| LOWORD(wParam) == IDC_GP) && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
				return 0;
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
		case PSN_APPLY:
			{
				char str[128];
				//SN
				GetDlgItemTextA(hwndDlg, IDC_SN, str, sizeof(str));
				if(lstrlenA(str)>0)
					ppro->setString(AIM_KEY_SN, str);
				else
					DBDeleteContactSetting(NULL, ppro->m_szModuleName, AIM_KEY_SN);
				//END SN

				//NK
				if(GetDlgItemTextA(hwndDlg, IDC_NK, str, sizeof(str)))
					ppro->setString(AIM_KEY_NK, str);
				else
				{
					GetDlgItemTextA(hwndDlg, IDC_SN, str, sizeof(str));
					ppro->setString(AIM_KEY_NK, str);
				}
				//END NK

				//PW
				GetDlgItemTextA(hwndDlg, IDC_PW, str, sizeof(str));
				if(lstrlenA(str)>0)
				{
					CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), (LPARAM) str);
					ppro->setString(AIM_KEY_PW, str);
				}
				else
					DBDeleteContactSetting(NULL, ppro->m_szModuleName, AIM_KEY_PW);
				//END PW

				//HN
				GetDlgItemTextA(hwndDlg, IDC_HN, str, sizeof(str));
				if(lstrlenA(str)>0)
					ppro->setString(AIM_KEY_HN, str);
				else
					ppro->setString(AIM_KEY_HN, AIM_DEFAULT_SERVER);
				//END HN

				//GP
				unsigned long timeout=GetDlgItemInt(hwndDlg, IDC_GP,0,0);
				if(timeout>0xffff||timeout<15)
					ppro->setWord( AIM_KEY_GP,DEFAULT_GRACE_PERIOD);
				else
					ppro->setWord( AIM_KEY_GP,(WORD)timeout);
				//END GP

				//Delivery Confirmation
				if (IsDlgButtonChecked(hwndDlg, IDC_DC))
					ppro->setByte( AIM_KEY_DC, 1);
				else
					ppro->setByte( AIM_KEY_DC, 0);
				//End Delivery Confirmation

				//Disable Avatar
				if (IsDlgButtonChecked(hwndDlg, IDC_DA))
					ppro->setByte( AIM_KEY_DA, 1);
				else
					ppro->setByte( AIM_KEY_DA, 0);
				//Disable Avatar

				//Force Proxy Transfer
				if (IsDlgButtonChecked(hwndDlg, IDC_FP))
					ppro->setByte( AIM_KEY_FP, 1);
				else
					ppro->setByte( AIM_KEY_FP, 0);
				//End Force Proxy Transfer

				//Disable Account Type Icons
				if (IsDlgButtonChecked(hwndDlg, IDC_AT))
				{
					int acc_disabled = ppro->getByte( AIM_KEY_AT, 0);
					if(!acc_disabled)
						ppro->remove_AT_icons();
					ppro->setByte( AIM_KEY_AT, 1);
				}
				else
				{
					int acc_disabled = ppro->getByte( AIM_KEY_AT, 0);
					if(acc_disabled)
						ppro->add_AT_icons();
					ppro->setByte( AIM_KEY_AT, 0);
				}
				//END

				//Disable Extra Status Icons
				if (IsDlgButtonChecked(hwndDlg, IDC_ES))
				{
					int es_disabled = ppro->getByte( AIM_KEY_ES, 0);
					if(!es_disabled)
						ppro->remove_ES_icons();
					ppro->setByte( AIM_KEY_ES, 1);
				}
				else
				{
					int es_disabled = ppro->getByte( AIM_KEY_ES, 0);
					if(es_disabled)
						ppro->add_ES_icons();
					ppro->setByte( AIM_KEY_ES, 0);
				}
				//End

				//Fake Hiptop
				if (IsDlgButtonChecked(hwndDlg, IDC_HF))
				{
					int hf = ppro->getByte( AIM_KEY_HF, 0);
					if(!hf)
						ShowWindow(GetDlgItem(hwndDlg, IDC_MASQ), SW_SHOW);
					ppro->setByte( AIM_KEY_HF, 1);
				}
				else
				{
					int hf = ppro->getByte( AIM_KEY_HF, 0);
					if(hf)
						ShowWindow(GetDlgItem(hwndDlg, IDC_MASQ), SW_SHOW);
					ppro->setByte( AIM_KEY_HF, 0);
				}
				//End

				//Disable Mode Message Sending
				if (IsDlgButtonChecked(hwndDlg, IDC_DM))
					ppro->setByte( AIM_KEY_DM, 1);
				else
					ppro->setByte( AIM_KEY_DM, 0);
				//End Disable Mode Message Sending

				//Format Incoming Messages
				if (IsDlgButtonChecked(hwndDlg, IDC_FI))
					ppro->setByte( AIM_KEY_FI, 1);
				else
					ppro->setByte( AIM_KEY_FI, 0);
				//End Format Incoming Messages

				//Format Outgoing Messages
				if (IsDlgButtonChecked(hwndDlg, IDC_FO))
					ppro->setByte( AIM_KEY_FO, 1);
				else
					ppro->setByte( AIM_KEY_FO, 0);
				//End Format Outgoing Messages

				if ( !IsDlgButtonChecked(hwndDlg, IDC_WEBSUPPORT) && ppro->getByte( AIM_KEY_AL, 0))
				{
					ppro->setByte( AIM_KEY_AL, 0);
					ppro->aim_links_unregister();
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_WEBSUPPORT))
				{
					ppro->setByte( AIM_KEY_AL, 1);
					ppro->aim_links_init();
				}
				else ppro->aim_links_destroy();

				//Instant Idle on Login
				if (IsDlgButtonChecked(hwndDlg, IDC_II))
					ppro->setByte( AIM_KEY_II, 1);
				else
					ppro->setByte( AIM_KEY_II, 0);
				//End
				//Check Mail on Login
				if (IsDlgButtonChecked(hwndDlg, IDC_CM))
					ppro->setByte( AIM_KEY_CM, 1);
				else
					ppro->setByte( AIM_KEY_CM, 0);
				//End

				//Keep alive timer
				if (IsDlgButtonChecked(hwndDlg, IDC_KA))
				{
					ppro->setByte( AIM_KEY_KA, 1);
					mir_forkthread(( pThreadFunc )aim_keepalive_thread, ppro );
				}
				else
				{
					ppro->setByte( AIM_KEY_KA, 0);
					SetEvent(ppro->hKeepAliveEvent);
				}
				//End 
			}
		}
		break;
	}
	return FALSE;
}

int CAimProto::OnOptionsInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize = sizeof(odp);
	odp.position = 1003000;
	odp.hInstance = hInstance;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_AIM);
	odp.pszGroup = LPGEN("Network");
	odp.pszTitle = m_szModuleName;
	odp.pfnDlgProc = options_dialog;
	odp.dwInitParam = LPARAM(this);
	odp.flags = ODPF_BOLDGROUPS;
	odp.nIDBottomSimpleControl=IDC_OPTIONS;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Brief account info dialog

/*

BOOL CALLBACK first_run_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// The code to implement and run the the first run dialog was contributed by RiPOFF
	switch (msg) {
	case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIconEx("aim"));
			if ( !getString(AIM_KEY_SN, &dbv))
			{
				SetDlgItemText(hwndDlg, IDC_SN, dbv.pszVal);
				DBFreeVariant(&dbv);
			}

			if ( !getString(AIM_KEY_PW, &dbv))
			{
				CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
				SetDlgItemText(hwndDlg, IDC_PW, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
		}
		break;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;

	case WM_DESTROY:
		ReleaseIconEx("aim");
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				char str[128];
				GetDlgItemText(hwndDlg, IDC_SN, str, sizeof(str));
				setString(AIM_KEY_SN, str);
				GetDlgItemText(hwndDlg, IDC_PW, str, sizeof(str));
				CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), (LPARAM) str);
				setString(AIM_KEY_PW, str);

			}
			// fall through

		case IDCANCEL:
			// Mark first run as completed
			setByte( AIM_KEY_FR, 1);
			EndDialog(hwndDlg, IDCANCEL);
			break;
		}
		break;
	}

	return FALSE;
}

*/

/////////////////////////////////////////////////////////////////////////////////////////
// Instant idle dialog

BOOL CALLBACK instant_idle_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CAimProto* ppro = (CAimProto*)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		ppro = (CAimProto*)lParam;
		{
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)ppro->LoadIconEx("idle"));
			unsigned long it = ppro->getDword( AIM_KEY_IIT, 0);
			unsigned long hours=it/60;
			unsigned long minutes=it%60;
			SetDlgItemInt(hwndDlg, IDC_IIH, hours,0);
			SetDlgItemInt(hwndDlg, IDC_IIM, minutes,0);
		}
		break;
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;

	case WM_DESTROY:
		ppro->ReleaseIconEx("idle");
		break;

	case WM_COMMAND:
		{
			unsigned long hours=GetDlgItemInt(hwndDlg, IDC_IIH,0,0);
			unsigned short minutes=(unsigned short)GetDlgItemInt(hwndDlg, IDC_IIM,0,0);
			if ( minutes > 59 )
				minutes = 59;
			ppro->setDword( AIM_KEY_IIT, hours*60+minutes);
			switch (LOWORD(wParam)) {
			case IDOK:
				//Instant Idle
				if (ppro->state==1) {
					ppro->aim_set_idle(ppro->hServerConn,ppro->seqno,hours * 60 * 60 + minutes * 60);
					ppro->instantidle=1;
				}
				EndDialog(hwndDlg, IDOK);
				break;

			case IDCANCEL:
				ppro->aim_set_idle(ppro->hServerConn,ppro->seqno,0);
				ppro->instantidle=0;
				EndDialog(hwndDlg, IDCANCEL);
				break;
			}
		}
		break;
	}
	return FALSE;
}
