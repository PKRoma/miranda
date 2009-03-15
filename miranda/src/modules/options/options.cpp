/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
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
#include "filter.h"

#define OPENOPTIONSDIALOG_OLD_SIZE 12

#define FILTER_TIMEOUT_TIMER 10012

#define ALL_MODULES_FILTER _T("<all modules>")
#define CORE_MODULES_FILTER _T("<core modules>")

static HANDLE hOptionsInitEvent;
static HWND hwndOptions=NULL;
static HWND hFilterSearchWnd = NULL;

// Thread for search keywords in dialogs
static BYTE bSearchState = 0; // 0 - not executed; 1 - in progress; 2 - completed;
static int FilterPage = 0;
static int FilterLoadProgress = 100;
static int FilterTimerId = 0;

char * GetPluginNameByInstance( HINSTANCE hInstance );

static void FillFilterCombo(int enableKeywordFiltering, HWND hDlg, struct OptionsPageData * opd, int PageCount);

struct OptionsPageInit
{
	int pageCount;
	OPTIONSDIALOGPAGE *odp;
};

struct DlgTemplateExBegin
{
    WORD   dlgVer;
    WORD   signature;
    DWORD  helpID;
    DWORD  exStyle;
    DWORD  style;
    WORD   cDlgItems;
    short  x;
    short  y;
    short  cx;
    short  cy;
};

struct OptionsPageData
{
	DLGTEMPLATE *pTemplate;
	DLGPROC dlgProc;
	HINSTANCE hInst;
	HTREEITEM hTreeItem;
	HWND hwnd;
	int changed;
	int simpleHeight,expertHeight;
	int simpleWidth,expertWidth;
	int simpleBottomControlId,simpleRightControlId;
	int nExpertOnlyControls;
	UINT *expertOnlyControls;
	DWORD flags;
	TCHAR *pszTitle, *pszGroup, *pszTab;
	BOOL insideTab;
	LPARAM dwInitParam;

	int offsetX;
	int offsetY;
};

struct OptionsDlgData
{
	int pageCount;
	int currentPage;
	HTREEITEM hCurrentPage;
	struct OptionsPageData *opd;
	RECT rcDisplay;
	RECT rcTab;
	HFONT hBoldFont;
};

static HTREEITEM FindNamedTreeItemAtRoot(HWND hwndTree, const TCHAR* name)
{
	TVITEM tvi;
	TCHAR str[128];

	tvi.mask = TVIF_TEXT;
	tvi.pszText = str;
	tvi.cchTextMax = SIZEOF( str );
	tvi.hItem = TreeView_GetRoot( hwndTree );
	while( tvi.hItem != NULL ) {
		SendMessage( hwndTree, TVM_GETITEM, 0, (LPARAM)&tvi );
		if( !_tcsicmp( str,name ))
			return tvi.hItem;

		tvi.hItem = TreeView_GetNextSibling( hwndTree, tvi.hItem );
	}
	return NULL;
}

static HTREEITEM FindNamedTreeItemAtChildren(HWND hwndTree, HTREEITEM hItem, const TCHAR* name)
{
	TVITEM tvi;
	TCHAR str[128];

	tvi.mask = TVIF_TEXT;
	tvi.pszText = str;
	tvi.cchTextMax = SIZEOF( str );
	tvi.hItem = TreeView_GetChild( hwndTree, hItem );
	while( tvi.hItem != NULL ) {
		SendMessage( hwndTree, TVM_GETITEM, 0, (LPARAM)&tvi );
		if( !_tcsicmp( str,name ))
			return tvi.hItem;

		tvi.hItem = TreeView_GetNextSibling( hwndTree, tvi.hItem );
	}
	return NULL;
}

static BOOL CALLBACK BoldGroupTitlesEnumChildren(HWND hwnd,LPARAM lParam)
{
	TCHAR szClass[64];

	GetClassName(hwnd,szClass,SIZEOF(szClass));
	if(!lstrcmp(szClass,_T("Button")) && (GetWindowLong(hwnd,GWL_STYLE)&0x0F)==BS_GROUPBOX)
		SendMessage(hwnd,WM_SETFONT,lParam,0);
	return TRUE;
}

struct MoveChildParam
{
	HWND hDlg;
	POINT offset;
};
static BOOL CALLBACK MoveEnumChildren(HWND hwnd,LPARAM lParam)
{
	struct MoveChildParam * param = ( struct MoveChildParam *) lParam;

	RECT rcWnd;
	GetWindowRect( hwnd, &rcWnd);

	HWND hwndParent = GetParent( hwnd );
	if ( hwndParent != param->hDlg )
		return TRUE;	// Do not move subchilds

	POINT pt; pt.x = 0; pt.y = 0;

	ClientToScreen( hwndParent, &pt );
	OffsetRect( &rcWnd, -pt.x, -pt.y );

	SetWindowPos( hwnd, NULL, rcWnd.left + param->offset.x, rcWnd.top + param->offset.y, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE );

	return TRUE;
}

#define OPTSTATE_PREFIX "s_"

static void SaveOptionsTreeState(HWND hdlg)
{
	TVITEMA tvi;
	char buf[130],str[128];
	tvi.mask = TVIF_TEXT | TVIF_STATE;
	tvi.pszText = str;
	tvi.cchTextMax = SIZEOF(str);
	tvi.hItem = TreeView_GetRoot( GetDlgItem( hdlg, IDC_PAGETREE ));
	while ( tvi.hItem != NULL ) {
		if ( SendMessageA( GetDlgItem(hdlg,IDC_PAGETREE), TVM_GETITEMA, 0, (LPARAM)&tvi )) {
			wsprintfA(buf,"%s%s",OPTSTATE_PREFIX,str);
			DBWriteContactSettingByte(NULL,"Options",buf,(BYTE)((tvi.state&TVIS_EXPANDED)?1:0));
		}
		tvi.hItem = TreeView_GetNextSibling( GetDlgItem( hdlg, IDC_PAGETREE ), tvi.hItem );
}	}

#define DM_FOCUSPAGE   (WM_USER+10)
#define DM_REBUILDPAGETREE (WM_USER+11)

static void ThemeDialogBackground(HWND hwnd, BOOL tabbed)
{
	if (enableThemeDialogTexture)
		enableThemeDialogTexture(hwnd,(tabbed?0x00000002:0x00000001)|0x00000004); //0x00000002|0x00000004=ETDT_ENABLETAB
}

static int lstrcmpnull(TCHAR *str1, TCHAR *str2)
{
	if ( str1 == NULL && str2 == NULL )
		return 0;
	if ( str1 != NULL && str2 == NULL )
		return 1;
	if ( str1 == NULL && str2 != NULL )
		return -1;

   return lstrcmp(str1, str2);
}

static TCHAR *GetPluginName(HINSTANCE hInstance, TCHAR *buffer, int size)
{
	TCHAR tszModuleName[MAX_PATH];
	GetModuleFileName(hInstance, tszModuleName, SIZEOF(tszModuleName));
	TCHAR *dllName = _tcsrchr(tszModuleName,'\\');
	if (!dllName)
	{
		dllName = tszModuleName;
	}
	else {
		dllName++;
	}
	
	_tcsncpy(buffer, dllName, size);
	
	return buffer;
}

PageHash GetPluginPageHash(const OptionsPageData *page)
{
	return HashFunc(page->pszGroup) + HashFunc(page->pszTitle) + HashFunc(page->pszTab);
}

static void FindFilterStrings(int enableKeywordFiltering, int current, HWND hWndParent, const OptionsPageData *page)
{
	TCHAR pluginName[MAX_PATH];
	HWND hWnd = 0;
	if (enableKeywordFiltering) {
		if (current)
			hWnd = page->hwnd;
		else
		{
			hWnd = CreateDialogIndirectParamA(page->hInst, page->pTemplate, hWndParent, page->dlgProc, page->dwInitParam); //create the options dialog page so we can parse it
			ShowWindow(hWnd, SW_HIDE); //make sure it's hidden
	}	}
	
	DWORD key = GetPluginPageHash(page); //get the plugin page hash
	
	TCHAR * PluginFullName = NULL;
	char * temp = GetPluginNameByInstance( page->hInst );
	if ( temp ) PluginFullName = a2t( temp );
	GetDialogStrings(enableKeywordFiltering, key, GetPluginName(page->hInst, pluginName, SIZEOF(pluginName)), hWnd, page->pszGroup, page->pszTitle, page->pszTab, PluginFullName );
	if ( PluginFullName ) mir_free( PluginFullName ) ;
	
	if (enableKeywordFiltering && !current)
		DestroyWindow(hWnd); //destroy the page, we're done with it
}

static int MatchesFilter(const OptionsPageData *page, TCHAR *szFilterString)
{
	DWORD key = GetPluginPageHash(page);
	
	return ContainsFilterString(key, szFilterString);
}

static WNDPROC OptionsFilterDefaultProc = NULL;

static LRESULT CALLBACK OptionsFilterSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_PAINT && message != WM_PRINT)
		return CallWindowProc(OptionsFilterDefaultProc, hWnd, message, wParam, lParam );

	if ( GetFocus() == hWnd || GetWindowTextLength( hWnd ) ) 
		return CallWindowProc(OptionsFilterDefaultProc, hWnd, message, wParam, lParam );

	RECT rc;
	GetClientRect( hWnd, &rc);
	HDC hdc;
	PAINTSTRUCT paint;

	if (message == WM_PAINT)
		hdc = BeginPaint( hWnd, &paint);
	else
		hdc = (HDC)wParam;

	TCHAR buf[255];
	if ( bSearchState==1 && FilterLoadProgress < 100 && FilterLoadProgress > 0 )
		mir_sntprintf( buf, SIZEOF(buf), TranslateT("Loading... %d%%"), FilterLoadProgress );
	else
		mir_sntprintf( buf, SIZEOF(buf), TranslateT( "Search" ) );

	BOOL bDrawnByTheme = FALSE;
	
	if ( openThemeData )  {
		HTHEME hTheme = openThemeData( hWnd, _T("EDIT"));
		if ( hTheme ) {
			int style = IsWinVerVistaPlus() ? EP_BACKGROUND : EP_EDITTEXT;
			if ( isThemeBackgroundPartiallyTransparent( hTheme, style, ETS_NORMAL) )
				drawThemeParentBackground( hWnd, hdc, &rc );

			DTBGOPTS dtbgopts = { sizeof(DTBGOPTS), DTBG_OMITBORDER };
			dtbgopts.rcClip = rc;

			drawThemeBackgroundEx( hTheme, hdc, style, ETS_NORMAL, &rc, &dtbgopts );
			HFONT hFont = (HFONT) GetStockObject( DEFAULT_GUI_FONT );
			HFONT oldFont = (HFONT) SelectObject( hdc, hFont );
			drawThemeText( hTheme, hdc,  EP_EDITTEXT, ETS_DISABLED, buf, -1, 0, 0, &rc );
			SelectObject( hdc, oldFont );
			DeleteObject( hFont );
			closeThemeData( hTheme );
			bDrawnByTheme = TRUE;
		}
	}

	if ( !bDrawnByTheme ) {
		HFONT hFont = (HFONT) GetStockObject( DEFAULT_GUI_FONT );
		HFONT oldFont = (HFONT) SelectObject( hdc, hFont );
		SetTextColor( hdc, GetSysColor(COLOR_GRAYTEXT) );
		FillRect( hdc, &rc, GetSysColorBrush( COLOR_WINDOW ) );
		int oldMode = SetBkMode( hdc, TRANSPARENT );
		DrawText( hdc, buf, -1, &rc, 0 );
        SetBkMode( hdc, oldMode );
		SelectObject( hdc, oldFont );
		DeleteObject( hFont );
	}

	if (message == WM_PAINT)
		EndPaint( hWnd, &paint);

	return 0;
}

static BOOL CheckPageShow( HWND hdlg, OptionsDlgData * dat, int i,  TCHAR * szFilterString )
{
	if ( szFilterString && _tcslen(szFilterString) > 0 && !MatchesFilter(&dat->opd[i], szFilterString) ) return FALSE;
	if (( dat->opd[i].flags & ODPF_SIMPLEONLY ) && IsDlgButtonChecked( hdlg, IDC_EXPERT )) return FALSE;
	if (( dat->opd[i].flags & ODPF_EXPERTONLY ) && !IsDlgButtonChecked( hdlg, IDC_EXPERT )) return FALSE;
	return TRUE;
}

static BOOL IsAeroMode()
{
	BOOL result;
	return dwmIsCompositionEnabled && (dwmIsCompositionEnabled(&result) == S_OK) && result;
}

static void AeroPaintControl(HWND hwnd, HDC hdc, WNDPROC OldWndProc, UINT msg = WM_PRINT, LPARAM lpFlags = PRF_CLIENT|PRF_NONCLIENT)
{
	HBITMAP hBmp, hOldBmp;
	RECT rc; GetClientRect(hwnd, &rc);
	BYTE *pBits;

	HDC tempDC = CreateCompatibleDC(hdc);

	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = rc.right;
	bmi.bmiHeader.biHeight = -rc.bottom;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	hBmp = CreateDIBSection(tempDC, &bmi, DIB_RGB_COLORS, (void **)&pBits, NULL, 0);

	hOldBmp = (HBITMAP)SelectObject(tempDC,hBmp);

	//paint
	SetPropA(hwnd, "Miranda.AeroRender.Active", (HANDLE)TRUE);
	CallWindowProc(OldWndProc, hwnd, msg, (WPARAM)tempDC, lpFlags);
	SetPropA(hwnd, "Miranda.AeroRender.Active", (HANDLE)FALSE);

	// Fix alpha channel
	GdiFlush();
	for (int i = 0; i < rc.right*rc.bottom; ++i, pBits += 4)
		if (!pBits[3]) pBits[3] = 255;

	//Copy to output
	BitBlt(hdc,0,0,rc.right,rc.bottom,tempDC,0,0,SRCCOPY);
	SelectObject(tempDC,hOldBmp);
	DeleteObject(hBmp);
	DeleteDC(tempDC);
}

static LRESULT CALLBACK AeroPaintSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC OldWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_USERDATA);
	switch (msg)
	{
		case WM_CTLCOLOREDIT:
			if (!GetPropA((HWND)lParam, "Miranda.AeroRender.Active"))
				RedrawWindow((HWND)lParam, NULL, NULL, RDW_INVALIDATE);
			break;

		case WM_ERASEBKGND:
			return TRUE;

		case WM_PRINT:
		case WM_PRINTCLIENT:
			AeroPaintControl(hwnd, (HDC)wParam, OldWndProc, msg, lParam);
			return TRUE;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			AeroPaintControl(hwnd, hdc, OldWndProc);
			EndPaint(hwnd, &ps);
			return TRUE;
		}
	}
	return CallWindowProc(OldWndProc, hwnd, msg, wParam, lParam);
}

static BOOL CALLBACK OptionsDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	struct OptionsDlgData* dat = (struct OptionsDlgData* )GetWindowLong( hdlg, GWL_USERDATA );
	static TCHAR szFilterString[1024]={0};	
	switch ( message ) {
	case WM_CTLCOLORSTATIC:
		switch ( GetDlgCtrlID(( HWND )lParam )) 
		{
			case IDC_WHITERECT:
			case IDC_LOGO:
			case IDC_NAME:
			case IDC_DESCRIPTION:
			case IDC_KEYWORD_FILTER:
				SetBkColor(( HDC )wParam, GetSysColor( COLOR_WINDOW ));
				return ( BOOL )GetSysColorBrush( COLOR_WINDOW );
		}
		break;

	case WM_INITDIALOG:
		{	
			PROPSHEETHEADER *psh=(PROPSHEETHEADER*)lParam;
			OPENOPTIONSDIALOG *ood=(OPENOPTIONSDIALOG*)psh->pStartPage;
			OPTIONSDIALOGPAGE *odp;
			int i;
			struct DlgTemplateExBegin *dte;
			TCHAR *lastPage = NULL, *lastGroup = NULL, *lastTab = NULL;
			DBVARIANT dbv;
			TCITEM tie;

			LOGFONT lf;
			HFONT hNormalFont = ( HFONT )SendDlgItemMessage( hdlg, IDC_NAME, WM_GETFONT, 0, 0 );
			GetObject( hNormalFont, sizeof( lf ), &lf );
			lf.lfWeight = FW_BOLD;
			HFONT hBoldFont = CreateFontIndirect(&lf);
			SendDlgItemMessage( hdlg, IDC_NAME, WM_SETFONT, ( WPARAM )hBoldFont, 0 );

			typedef BOOL (STDAPICALLTYPE *pfnGetComboBoxInfo)(HWND, PCOMBOBOXINFO);
			pfnGetComboBoxInfo getComboBoxInfo = (pfnGetComboBoxInfo)GetProcAddress(GetModuleHandleA("user32"), "GetComboBoxInfo"); 
			if (getComboBoxInfo) {
				COMBOBOXINFO cbi;
				cbi.cbSize = sizeof(COMBOBOXINFO);
				getComboBoxInfo(GetDlgItem( hdlg, IDC_KEYWORD_FILTER), &cbi);
				OptionsFilterDefaultProc = (WNDPROC)SetWindowLong( cbi.hwndItem, GWL_WNDPROC, (LONG) OptionsFilterSubclassProc );

				if (IsAeroMode())
				{
					SetWindowLong(cbi.hwndCombo, GWL_USERDATA, GetWindowLong(cbi.hwndCombo, GWLP_WNDPROC));
					SetWindowLong(cbi.hwndCombo, GWLP_WNDPROC, (LONG)AeroPaintSubclassProc);
					SetWindowLong(cbi.hwndItem, GWL_USERDATA, GetWindowLong(cbi.hwndItem, GWLP_WNDPROC));
					SetWindowLong(cbi.hwndItem, GWLP_WNDPROC, (LONG)AeroPaintSubclassProc);
				}
			}

			Utils_RestoreWindowPositionNoSize(hdlg, NULL, "Options", "");
			TranslateDialogDefault(hdlg);
			SendMessage(GetDlgItem(hdlg, IDC_HEADERBAR), WM_SETICON, 0, (LPARAM)LoadIcon(hMirandaInst, MAKEINTRESOURCE(IDI_MIRANDA)));
			Window_SetIcon_IcoLib(hdlg, SKINICON_OTHER_OPTIONS);
			CheckDlgButton(hdlg,IDC_EXPERT,DBGetContactSettingByte(NULL,"Options","Expert",SETTING_SHOWEXPERT_DEFAULT)?BST_CHECKED:BST_UNCHECKED);
			EnableWindow(GetDlgItem(hdlg,IDC_APPLY),FALSE);
			dat=(struct OptionsDlgData*)mir_alloc(sizeof(struct OptionsDlgData));
			SetWindowLong(hdlg,GWL_USERDATA,(LONG)dat);
			SetWindowText(hdlg,psh->pszCaption);
			
			dat->hBoldFont=(HFONT)SendDlgItemMessage(hdlg,IDC_EXPERT,WM_GETFONT,0,0);
			GetObject(dat->hBoldFont,sizeof(lf),&lf);
			lf.lfWeight=FW_BOLD;
			dat->hBoldFont=CreateFontIndirect(&lf);
			
			dat->pageCount = psh->nPages;
			dat->opd = ( struct OptionsPageData* )mir_alloc( sizeof(struct OptionsPageData) * dat->pageCount );
			odp = ( OPTIONSDIALOGPAGE* )psh->ppsp;

			dat->currentPage = -1;
			if ( ood->pszPage == NULL ) {
				if ( !DBGetContactSettingTString( NULL, "Options", "LastPage", &dbv )) {
					lastPage = mir_tstrdup( dbv.ptszVal );
					DBFreeVariant( &dbv );
				}
			}
			else lastPage = LangPackPcharToTchar( ood->pszPage );

			if ( ood->pszGroup == NULL ) {
				if ( !DBGetContactSettingTString( NULL, "Options", "LastGroup", &dbv )) {
					lastGroup = mir_tstrdup( dbv.ptszVal );
					DBFreeVariant( &dbv );
				}
			}
			else lastGroup = LangPackPcharToTchar( ood->pszGroup );

			if ( ood->pszTab == NULL ) {
				if ( !DBGetContactSettingTString( NULL, "Options", "LastTab", &dbv )) {
					lastTab = mir_tstrdup( dbv.ptszVal );
					DBFreeVariant( &dbv );
				}
			}
			else lastTab = LangPackPcharToTchar( ood->pszTab );

			for ( i=0; i < dat->pageCount; i++, odp++ ) {
				struct OptionsPageData* opd = &dat->opd[i];
				HRSRC hrsrc=FindResourceA(odp->hInstance,odp->pszTemplate,MAKEINTRESOURCEA(5));
				HGLOBAL hglb=LoadResource(odp->hInstance,hrsrc);
				DWORD resSize=SizeofResource(odp->hInstance,hrsrc);
				opd->pTemplate = ( DLGTEMPLATE* )mir_alloc(resSize);
				memcpy(opd->pTemplate,LockResource(hglb),resSize);
				dte=(struct DlgTemplateExBegin*)opd->pTemplate;
				if ( dte->signature == 0xFFFF ) {
					//this feels like an access violation, and is according to boundschecker
					//...but it works - for now
					//may well have to remove and sort out the original dialogs
					dte->style&=~(WS_VISIBLE|WS_CHILD|WS_POPUP|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER);
					dte->style|=WS_CHILD;
				}
				else {
					opd->pTemplate->style&=~(WS_VISIBLE|WS_CHILD|WS_POPUP|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER);
					opd->pTemplate->style|=WS_CHILD;
				}
				opd->dlgProc=odp->pfnDlgProc;
				opd->hInst=odp->hInstance;
				opd->hwnd=NULL;
				opd->changed=0;
				opd->simpleHeight=opd->expertHeight=0;
				opd->simpleBottomControlId=odp->nIDBottomSimpleControl;
				opd->simpleWidth=opd->expertWidth=0;
				opd->simpleRightControlId=odp->nIDRightSimpleControl;
				opd->nExpertOnlyControls=odp->nExpertOnlyControls;
				opd->expertOnlyControls=odp->expertOnlyControls;
				opd->flags=odp->flags;
				opd->dwInitParam=odp->dwInitParam;
				if ( odp->pszTitle == NULL )
					opd->pszTitle = NULL;
				else if ( odp->flags & ODPF_UNICODE ) {
					#if defined ( _UNICODE )
						opd->pszTitle = ( TCHAR* )mir_wstrdup( odp->ptszTitle );
					#else
						opd->pszTitle = u2a(( WCHAR* )odp->ptszTitle );
					#endif
				}
				else opd->pszTitle = ( TCHAR* )mir_strdup( odp->pszTitle );

				if ( odp->pszGroup == NULL )
					opd->pszGroup = NULL;
				else if ( odp->flags & ODPF_UNICODE ) {
					#if defined ( _UNICODE )
						opd->pszGroup = ( TCHAR* )mir_wstrdup( odp->ptszGroup );
					#else
						opd->pszGroup = u2a(( WCHAR* )odp->ptszGroup );
					#endif
				}
				else opd->pszGroup = ( TCHAR* )mir_strdup( odp->pszGroup );

				if ( odp->pszTab == NULL )
					opd->pszTab = NULL;
				else if ( odp->flags & ODPF_UNICODE ) {
					#if defined ( _UNICODE )
						opd->pszTab = ( TCHAR* )mir_wstrdup( odp->ptszTab );
					#else
						opd->pszTab = u2a(( WCHAR* )odp->ptszTab );
					#endif
				}
				else opd->pszTab = ( TCHAR* )mir_strdup( odp->pszTab );

				if ( !lstrcmp( lastPage, odp->ptszTitle ) &&
					  !lstrcmpnull( lastGroup, odp->ptszGroup ) &&
					  (( ood->pszTab == NULL && dat->currentPage == -1 ) || !lstrcmpnull( lastTab, odp->ptszTab )))
					dat->currentPage = i;
			}
			mir_free( lastGroup );
			mir_free( lastPage );
			mir_free( lastTab );

			GetWindowRect(GetDlgItem(hdlg,IDC_STNOPAGE),&dat->rcDisplay);
			MapWindowPoints(NULL, hdlg, (LPPOINT)&dat->rcDisplay, 2);

			// Add an item to count in height
			tie.mask = TCIF_TEXT | TCIF_IMAGE;
			tie.iImage = -1;
			tie.pszText = _T("X");
			TabCtrl_InsertItem(GetDlgItem(hdlg,IDC_TAB), 0, &tie);

			GetWindowRect(GetDlgItem(hdlg,IDC_TAB), &dat->rcTab);
			MapWindowPoints(NULL, hdlg, (LPPOINT)&dat->rcTab, 2);
			TabCtrl_AdjustRect(GetDlgItem(hdlg,IDC_TAB), FALSE, &dat->rcTab);

			int enableKeywordFiltering = DBGetContactSettingWord(NULL, "Options", "EnableKeywordFiltering", TRUE); 
			FillFilterCombo(enableKeywordFiltering,  hdlg, dat->opd, dat->pageCount); 
			SendMessage(hdlg,DM_REBUILDPAGETREE,0,0);

			return TRUE;
		}
	case DM_REBUILDPAGETREE:
		{	
			int i;
			TVINSERTSTRUCT tvis;
			TVITEMA tvi;
			BOOL bRemoveFocusFromFilter = FALSE;
			char str[128],buf[130];

			HINSTANCE FilterInst=NULL;
			
			LPARAM oldSel = SendDlgItemMessage(hdlg, IDC_KEYWORD_FILTER, CB_GETEDITSEL, 0, 0);

			GetDlgItemText(hdlg, IDC_KEYWORD_FILTER, szFilterString, SIZEOF(szFilterString));
			
			//if filter string is set to all modules then make the filter string empty (this will return all modules)
			if (_tcscmp(szFilterString, TranslateTS( ALL_MODULES_FILTER ) ) == 0) {
				szFilterString[0] = 0; 
				bRemoveFocusFromFilter = TRUE;
			} 
			//if filter string is set to core modules replace it with the name of the executable (this will return all core modules)
			else if (_tcscmp(szFilterString, TranslateTS( CORE_MODULES_FILTER) ) == 0) {
				//replace string with process name - that will show core settings
				TCHAR szFileName[300];
				GetModuleFileName(NULL, szFileName, SIZEOF(szFileName));
				TCHAR *pos = _tcsrchr(szFileName, _T('\\'));
				if (pos)
					pos++;
				else
					pos = szFileName;
				
				_tcsncpy(szFilterString, pos, SIZEOF(szFilterString));
			}
			else  {
				int sel = SendMessage( GetDlgItem(hdlg, IDC_KEYWORD_FILTER ), (UINT) CB_GETCURSEL, 0,0 );
				if (sel != -1) {
					HINSTANCE hinst = (HINSTANCE)SendMessage( GetDlgItem(hdlg, IDC_KEYWORD_FILTER ), (UINT) CB_GETITEMDATA,  sel ,0 );
					TCHAR szFileName[300];
					GetModuleFileName(hinst, szFileName, SIZEOF(szFileName));
					TCHAR *pos = _tcsrchr(szFileName, _T('\\'));
					if (pos) pos++;
					else pos = szFileName;
					_tcsncpy(szFilterString, pos, SIZEOF(szFilterString));
			}	}
			
			_tcslwr_locale(szFilterString); //all strings are stored as lowercase ... make sure filter string is lowercase too

			ShowWindow(GetDlgItem(hdlg,IDC_PAGETREE),SW_HIDE);	 //deleteall is annoyingly visible
			
			HWND oldWnd = NULL;
			HWND oldTab = NULL; 
			if ( dat->currentPage != (-1)) {	
				oldWnd = dat->opd[dat->currentPage].hwnd;
				if ( dat->opd[dat->currentPage].insideTab )
					oldTab = GetDlgItem( hdlg,IDC_TAB ); 
			}			

			dat->hCurrentPage = NULL;

			TreeView_SelectItem(GetDlgItem(hdlg,IDC_PAGETREE),NULL);

			TreeView_DeleteAllItems(GetDlgItem(hdlg,IDC_PAGETREE));
			
			tvis.hParent = NULL;
			tvis.hInsertAfter = TVI_SORT;
			tvis.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
			tvis.item.state = tvis.item.stateMask = TVIS_EXPANDED;
			for ( i=0; i < dat->pageCount; i++ ) {
				static TCHAR *fullTitle=NULL;
				TCHAR * useTitle;
				if (fullTitle) mir_free(fullTitle);
				fullTitle=NULL;
				if (! CheckPageShow( hdlg, dat, i, szFilterString ) ) continue;
				tvis.hParent = NULL;
				if ( FilterInst!=NULL ) {
					size_t sz=dat->opd[i].pszGroup?_tcslen(dat->opd[i].pszGroup)+1:0;
					if (sz) sz+=3;
					sz+=dat->opd[i].pszTitle?_tcslen(dat->opd[i].pszTitle)+1:0;
					fullTitle = ( TCHAR* )mir_alloc(sz*sizeof(TCHAR));
					mir_sntprintf(fullTitle,sz,(dat->opd[i].pszGroup && dat->opd[i].pszTitle)?_T("%s - %s"):_T("%s%s"),dat->opd[i].pszGroup?dat->opd[i].pszGroup:_T(""),dat->opd[i].pszTitle?dat->opd[i].pszTitle:_T("") );
				}
				useTitle=fullTitle?fullTitle:dat->opd[i].pszTitle;
				if(dat->opd[i].pszGroup != NULL && FilterInst==NULL) {
					tvis.hParent = FindNamedTreeItemAtRoot(GetDlgItem(hdlg,IDC_PAGETREE),dat->opd[i].pszGroup);
					if(tvis.hParent == NULL) {
						tvis.item.lParam = -1;
						tvis.item.pszText = dat->opd[i].pszGroup;
						tvis.hParent = TreeView_InsertItem( GetDlgItem(hdlg,IDC_PAGETREE), &tvis );
					}
				}
				else {
					TVITEM tvi;
					tvi.hItem = FindNamedTreeItemAtRoot(GetDlgItem(hdlg,IDC_PAGETREE),useTitle);
					if( tvi.hItem != NULL ) {
						if ( i == dat->currentPage ) dat->hCurrentPage=tvi.hItem;
						tvi.mask = TVIF_PARAM;
						TreeView_GetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
						if ( tvi.lParam == -1 ) {
							tvi.lParam = i;
							TreeView_SetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
							continue;
				}	}	}

				if ( dat->opd[i].pszTab != NULL ) {
					HTREEITEM hItem;
					if (tvis.hParent == NULL)
						hItem = FindNamedTreeItemAtRoot(GetDlgItem(hdlg,IDC_PAGETREE),useTitle);
					else
						hItem = FindNamedTreeItemAtChildren(GetDlgItem(hdlg,IDC_PAGETREE),tvis.hParent,useTitle);
					if( hItem != NULL ) {
						if ( i == dat->currentPage ) {
							TVITEM tvi;
							tvi.hItem=hItem;
							tvi.mask=TVIF_PARAM;
							tvi.lParam=dat->currentPage;
							TreeView_SetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
							dat->hCurrentPage=hItem;
						}
						continue;
					}
				}

				tvis.item.pszText = useTitle;
				tvis.item.lParam = i;
				dat->opd[i].hTreeItem = TreeView_InsertItem( GetDlgItem(hdlg,IDC_PAGETREE), &tvis);
				if ( i == dat->currentPage )
					dat->hCurrentPage = dat->opd[i].hTreeItem;	

				if (fullTitle) mir_free(fullTitle);
				fullTitle=NULL;
			}
			tvi.mask = TVIF_TEXT | TVIF_STATE;
			tvi.pszText = str;
			tvi.cchTextMax = SIZEOF(str);
			tvi.hItem = TreeView_GetRoot(GetDlgItem(hdlg,IDC_PAGETREE));
			while ( tvi.hItem != NULL ) {
				if ( SendMessageA( GetDlgItem(hdlg,IDC_PAGETREE), TVM_GETITEMA, 0, (LPARAM)&tvi )) {
					wsprintfA(buf,"%s%s",OPTSTATE_PREFIX,str);
					if ( !DBGetContactSettingByte( NULL, "Options", buf, 1 ))
						TreeView_Expand( GetDlgItem(hdlg,IDC_PAGETREE), tvi.hItem, TVE_COLLAPSE );
				}
				tvi.hItem = TreeView_GetNextSibling( GetDlgItem( hdlg, IDC_PAGETREE ), tvi.hItem );
			}
			if(dat->hCurrentPage==NULL) {
				dat->hCurrentPage=TreeView_GetRoot(GetDlgItem(hdlg,IDC_PAGETREE));
				dat->currentPage=-1;
			}
			TreeView_SelectItem(GetDlgItem(hdlg,IDC_PAGETREE),dat->hCurrentPage);
			
			if ( oldWnd ) {
				if ( dat->currentPage == (-1) || oldWnd != dat->opd[dat->currentPage].hwnd ) {
					ShowWindow( oldWnd, SW_HIDE);
					if ( oldTab && ( dat->currentPage ==-1 || !dat->opd[dat->currentPage].insideTab ) )
						ShowWindow( oldTab, SW_HIDE );		
				}	
			}

			if ( szFilterString[0] == 0 ) // Clear the keyword combo box
				SetWindowText( GetDlgItem(hdlg, IDC_KEYWORD_FILTER), _T("") );
			if ( !bRemoveFocusFromFilter )
				SetFocus(GetDlgItem(hdlg, IDC_KEYWORD_FILTER)); //set the focus back to the combo box

			SendDlgItemMessage(hdlg, IDC_KEYWORD_FILTER, CB_SETEDITSEL, 0, oldSel ); //but don't select any of the text
			
			ShowWindow(GetDlgItem(hdlg,IDC_PAGETREE),SW_SHOW);
		}
		break;

	case PSM_CHANGED:
		EnableWindow(GetDlgItem(hdlg,IDC_APPLY),TRUE);
		if(dat->currentPage != (-1)) dat->opd[dat->currentPage].changed=1;
		return TRUE;

	case PSM_ISEXPERT:
		SetWindowLong(hdlg,DWL_MSGRESULT,IsDlgButtonChecked(hdlg,IDC_EXPERT));
		return TRUE;

	case PSM_GETBOLDFONT:
		SetWindowLong(hdlg,DWL_MSGRESULT,(LONG)dat->hBoldFont);
		return TRUE;

	case WM_NOTIFY:
		switch(wParam) {
		case IDC_TAB:
		case IDC_PAGETREE:
			switch(((LPNMHDR)lParam)->code) {
				case TVN_ITEMEXPANDING:
					SetWindowLong(hdlg,DWL_MSGRESULT,FALSE);
					return TRUE;
				case TCN_SELCHANGING:
				case TVN_SELCHANGING:
				{	PSHNOTIFY pshn;
					if(dat->currentPage==-1 || dat->opd[dat->currentPage].hwnd==NULL) break;
					pshn.hdr.code=PSN_KILLACTIVE;
					pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
					pshn.hdr.idFrom=0;
					pshn.lParam=0;
					if(SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn)) {
						SetWindowLong(hdlg,DWL_MSGRESULT,TRUE);
						return TRUE;
					}
					break;
				}
				case TCN_SELCHANGE:
				case TVN_SELCHANGED:
				{	BOOL tabChange = (wParam == IDC_TAB);
					ShowWindow(GetDlgItem(hdlg,IDC_STNOPAGE),SW_HIDE);
					if(dat->currentPage!=-1 && dat->opd[dat->currentPage].hwnd!=NULL) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_HIDE);
					if (!tabChange) {
						TVITEM tvi;
						tvi.hItem=dat->hCurrentPage=TreeView_GetSelection(GetDlgItem(hdlg,IDC_PAGETREE));
						if(tvi.hItem==NULL) break;
						tvi.mask=TVIF_HANDLE|TVIF_PARAM;
						TreeView_GetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
						dat->currentPage=tvi.lParam;
						ShowWindow(GetDlgItem(hdlg,IDC_TAB),SW_HIDE);
					} 
					else {
						TCITEM tie;
						TVITEM tvi;

						tie.mask = TCIF_PARAM;
						TabCtrl_GetItem(GetDlgItem(hdlg,IDC_TAB),TabCtrl_GetCurSel(GetDlgItem(hdlg,IDC_TAB)),&tie);
						dat->currentPage=tie.lParam;

						tvi.hItem=dat->hCurrentPage;
						tvi.mask=TVIF_PARAM;
						tvi.lParam=dat->currentPage;
						TreeView_SetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
					}
					if ( dat->currentPage != -1 ) {
						if ( dat->opd[dat->currentPage].hwnd == NULL ) {
							RECT rcPage;
							RECT rcControl,rc;
							int w,h,pages=0;

							dat->opd[dat->currentPage].hwnd=CreateDialogIndirectParamA(dat->opd[dat->currentPage].hInst,dat->opd[dat->currentPage].pTemplate,hdlg,dat->opd[dat->currentPage].dlgProc,dat->opd[dat->currentPage].dwInitParam);
							if(dat->opd[dat->currentPage].flags&ODPF_BOLDGROUPS)
								EnumChildWindows(dat->opd[dat->currentPage].hwnd,BoldGroupTitlesEnumChildren,(LPARAM)dat->hBoldFont);
							GetClientRect(dat->opd[dat->currentPage].hwnd,&rcPage);
							dat->opd[dat->currentPage].expertWidth=rcPage.right;
							dat->opd[dat->currentPage].expertHeight=rcPage.bottom;
							GetWindowRect(dat->opd[dat->currentPage].hwnd,&rc);

							if(dat->opd[dat->currentPage].simpleBottomControlId) {
								GetWindowRect(GetDlgItem(dat->opd[dat->currentPage].hwnd,dat->opd[dat->currentPage].simpleBottomControlId),&rcControl);
								dat->opd[dat->currentPage].simpleHeight=rcControl.bottom-rc.top;
							}
							else dat->opd[dat->currentPage].simpleHeight=dat->opd[dat->currentPage].expertHeight;

							if(dat->opd[dat->currentPage].simpleRightControlId) {
								GetWindowRect(GetDlgItem(dat->opd[dat->currentPage].hwnd,dat->opd[dat->currentPage].simpleRightControlId),&rcControl);
								dat->opd[dat->currentPage].simpleWidth=rcControl.right-rc.left;
							}
							else dat->opd[dat->currentPage].simpleWidth=dat->opd[dat->currentPage].expertWidth;

							if(IsDlgButtonChecked(hdlg,IDC_EXPERT)) {
								w=dat->opd[dat->currentPage].expertWidth;
								h=dat->opd[dat->currentPage].expertHeight;
							}
							else {
								int i;
								for(i=0;i<dat->opd[dat->currentPage].nExpertOnlyControls;i++)
									ShowWindow(GetDlgItem(dat->opd[dat->currentPage].hwnd,dat->opd[dat->currentPage].expertOnlyControls[i]),SW_HIDE);
								w=dat->opd[dat->currentPage].simpleWidth;
								h=dat->opd[dat->currentPage].simpleHeight;
							}

							dat->opd[dat->currentPage].offsetX = 0;
							dat->opd[dat->currentPage].offsetY = 0;

							if (dat->opd[dat->currentPage].pszTab != NULL) {
								// Count tabs to calc position
								int i;
								for ( i=0; i < dat->pageCount && pages < 2; i++ ) {
									if (!CheckPageShow( hdlg, dat, i, szFilterString ) ) continue;
									//if (( dat->opd[i].flags & ODPF_SIMPLEONLY ) && IsDlgButtonChecked( hdlg, IDC_EXPERT )) continue;
									//if (( dat->opd[i].flags & ODPF_EXPERTONLY ) && !IsDlgButtonChecked( hdlg, IDC_EXPERT )) continue;
									if ( lstrcmp(dat->opd[i].pszTitle, dat->opd[dat->currentPage].pszTitle) || lstrcmpnull(dat->opd[i].pszGroup, dat->opd[dat->currentPage].pszGroup) ) continue;
									pages++;
							}	}
							dat->opd[dat->currentPage].insideTab = (pages > 1);
							if (dat->opd[dat->currentPage].insideTab) {
								SetWindowPos(dat->opd[dat->currentPage].hwnd,HWND_TOP,(dat->rcTab.left+dat->rcTab.right-w)>>1,dat->rcTab.top,w,h,0);
								ThemeDialogBackground(dat->opd[dat->currentPage].hwnd,TRUE);
							} else {
								SetWindowPos(dat->opd[dat->currentPage].hwnd,HWND_TOP,(dat->rcDisplay.left+dat->rcDisplay.right-w)>>1,(dat->rcDisplay.top+dat->rcDisplay.bottom-h)>>1,w,h,0);
								ThemeDialogBackground(dat->opd[dat->currentPage].hwnd,FALSE);
							}
						}
						if ( !tabChange )
						{
							if ( dat->opd[dat->currentPage].pszTab != NULL ) 
							{
								// Count tabs to calc position
								int j;
								int pages=0;
								for ( j=0; j < dat->pageCount && pages < 2; j++ )
								{
									if (!CheckPageShow( hdlg, dat, j, szFilterString ) ) continue;
									//if (( dat->opd[j].flags & ODPF_SIMPLEONLY ) && IsDlgButtonChecked( hdlg, IDC_EXPERT )) continue;
									//if (( dat->opd[j].flags & ODPF_EXPERTONLY ) && !IsDlgButtonChecked( hdlg, IDC_EXPERT )) continue;
									if ( lstrcmp(dat->opd[j].pszTitle, dat->opd[dat->currentPage].pszTitle) || lstrcmpnull(dat->opd[j].pszGroup, dat->opd[dat->currentPage].pszGroup) ) continue;
									pages++;
								}							
								dat->opd[ dat->currentPage].insideTab = (pages > 1);
							}
							if ( dat->opd[dat->currentPage].insideTab  ) {
								// Make tabbed pane
								int i,pages=0,sel=0;
								TCITEM tie;
								HWND hwndTab = GetDlgItem(hdlg,IDC_TAB);

								TabCtrl_DeleteAllItems(hwndTab);
								tie.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
								tie.iImage = -1;
								for ( i=0; i < dat->pageCount; i++ ) {
									if (!CheckPageShow( hdlg, dat, i, szFilterString ) ) continue;
									//if (( dat->opd[i].flags & ODPF_SIMPLEONLY ) && IsDlgButtonChecked( hdlg, IDC_EXPERT )) continue;
									//if (( dat->opd[i].flags & ODPF_EXPERTONLY ) && !IsDlgButtonChecked( hdlg, IDC_EXPERT )) continue;
									if ( lstrcmp(dat->opd[i].pszTitle, dat->opd[dat->currentPage].pszTitle) || lstrcmpnull(dat->opd[i].pszGroup, dat->opd[dat->currentPage].pszGroup) ) continue;

									tie.pszText = dat->opd[i].pszTab;
									tie.lParam = i;
									TabCtrl_InsertItem(hwndTab, pages, &tie);
									if ( !lstrcmp(dat->opd[i].pszTab,dat->opd[dat->currentPage].pszTab) )
										sel = pages;
									pages++;
								}
								TabCtrl_SetCurSel(hwndTab,sel);
								ShowWindow(hwndTab, dat->opd[dat->currentPage].insideTab ? SW_SHOW : SW_HIDE );
							}

							if (dat->opd[dat->currentPage].insideTab) 
								ThemeDialogBackground(dat->opd[dat->currentPage].hwnd,TRUE);
							else 
								ThemeDialogBackground(dat->opd[dat->currentPage].hwnd,FALSE);
						}
						// Resizing
						{
							int pageWidth, pageHeight;

							if(IsDlgButtonChecked(hdlg,IDC_EXPERT)) {
								pageWidth=dat->opd[dat->currentPage].expertWidth;
								pageHeight=dat->opd[dat->currentPage].expertHeight;
							}
							else  {
								pageWidth=dat->opd[dat->currentPage].simpleWidth;
								pageHeight=dat->opd[dat->currentPage].simpleHeight;
							}

							RECT * parentPageRect = &dat->rcDisplay;

							if ( dat->opd[dat->currentPage].insideTab )
								parentPageRect = &dat->rcTab;

							pageHeight = min( pageHeight, parentPageRect->bottom - parentPageRect->top );
							pageWidth  = min( pageWidth,  parentPageRect->right - parentPageRect->left );

							int newOffsetX = ( parentPageRect->right - parentPageRect->left - pageWidth ) >> 1;
							int newOffsetY = dat->opd[dat->currentPage].insideTab ? 0 : ( parentPageRect->bottom - parentPageRect->top - pageHeight ) >> 1;

							struct MoveChildParam mcp;
							mcp.hDlg     = dat->opd[dat->currentPage].hwnd;
							mcp.offset.x = newOffsetX - dat->opd[dat->currentPage].offsetX;
							mcp.offset.y = newOffsetY - dat->opd[dat->currentPage].offsetY;

							
							if ( mcp.offset.x || mcp.offset.y )
							{
								
								EnumChildWindows(dat->opd[dat->currentPage].hwnd,MoveEnumChildren,(LPARAM)(&mcp));

								SetWindowPos( dat->opd[dat->currentPage].hwnd, NULL,
									parentPageRect->left, parentPageRect->top, 
									parentPageRect->right - parentPageRect->left,
									parentPageRect->bottom - parentPageRect->top,
									SWP_NOZORDER | SWP_NOACTIVATE );
								dat->opd[dat->currentPage].offsetX = newOffsetX;
								dat->opd[dat->currentPage].offsetY = newOffsetY;
							}
							
						}						

						ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);
						if(((LPNMTREEVIEW)lParam)->action==TVC_BYMOUSE) PostMessage(hdlg,DM_FOCUSPAGE,0,0);
						else SetFocus(GetDlgItem(hdlg,IDC_PAGETREE));
					}
					else ShowWindow(GetDlgItem(hdlg,IDC_STNOPAGE),SW_SHOW);
					break;
		}	}	}
		break;

	case DM_FOCUSPAGE:
		if (dat->currentPage != -1)
			SetFocus(dat->opd[dat->currentPage].hwnd);
		break;
	
	case WM_TIMER:
		if (wParam == FILTER_TIMEOUT_TIMER) {
			SaveOptionsTreeState(hdlg);
			SendMessage(hdlg,DM_REBUILDPAGETREE,0,0);

			KillTimer(hdlg, FILTER_TIMEOUT_TIMER);
		}
		break;	
	
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_KEYWORD_FILTER:
			//add a timer - when the timer elapses filter the option pages
			if ( (HIWORD(wParam)==CBN_SELCHANGE) || (HIWORD(wParam) == CBN_EDITCHANGE))
				if (!SetTimer(hdlg, FILTER_TIMEOUT_TIMER, 400, NULL))
					MessageBeep(MB_ICONSTOP);

			break;

		case IDC_EXPERT:
			{
				int expert=IsDlgButtonChecked(hdlg,IDC_EXPERT);
				int i,j;
				PSHNOTIFY pshn;
				RECT rcPage;
				int neww,newh;

				DBWriteContactSettingByte(NULL,"Options","Expert",(BYTE)expert);
				pshn.hdr.idFrom=0;
				pshn.lParam=expert;
				pshn.hdr.code=PSN_EXPERTCHANGED;
				for(i=0;i<dat->pageCount;i++) {
					int pages=0;

					if(dat->opd[i].hwnd==NULL) continue;
					if (!CheckPageShow( hdlg, dat, i, szFilterString ) ) continue;
					//if (( dat->opd[i].flags & ODPF_SIMPLEONLY ) && expert) continue;
					//if (( dat->opd[i].flags & ODPF_EXPERTONLY ) && !expert) continue;
					pshn.hdr.hwndFrom=dat->opd[i].hwnd;
					SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);

					for(j=0;j<dat->opd[i].nExpertOnlyControls;j++)
						ShowWindow(GetDlgItem(dat->opd[i].hwnd,dat->opd[i].expertOnlyControls[j]),expert?SW_SHOW:SW_HIDE);

					if (dat->opd[i].pszTab != NULL) {
						// Count tabs to calc position
						int j;
						for ( j=0; j < dat->pageCount && pages < 2; j++ ) {
							if (!CheckPageShow( hdlg, dat, i, szFilterString ) ) continue;
							//if (( dat->opd[j].flags & ODPF_SIMPLEONLY ) && IsDlgButtonChecked( hdlg, IDC_EXPERT )) continue;
							//if (( dat->opd[j].flags & ODPF_EXPERTONLY ) && !IsDlgButtonChecked( hdlg, IDC_EXPERT )) continue;
							if ( lstrcmp(dat->opd[j].pszTitle, dat->opd[i].pszTitle) || lstrcmpnull(dat->opd[j].pszGroup, dat->opd[i].pszGroup) ) continue;
							pages++;
					}	}
					dat->opd[i].insideTab = (pages > 1);

					GetWindowRect(dat->opd[i].hwnd,&rcPage);
					if(dat->opd[i].simpleBottomControlId) newh=expert?dat->opd[i].expertHeight:dat->opd[i].simpleHeight;
					else newh=rcPage.bottom-rcPage.top;
					if(dat->opd[i].simpleRightControlId) neww=expert?dat->opd[i].expertWidth:dat->opd[i].simpleWidth;
					else neww=rcPage.right-rcPage.left;
					if(i==dat->currentPage) {
						POINT ptStart,ptEnd,ptNow;
						DWORD thisTick,startTick;
						RECT rc;

						ptNow.x=ptNow.y=0;
						ClientToScreen(hdlg,&ptNow);
						GetWindowRect(dat->opd[i].hwnd,&rc);
						ptStart.x=rc.left-ptNow.x;
						ptStart.y=rc.top-ptNow.y;
						if (dat->opd[i].insideTab) {
							ptEnd.x=(dat->rcTab.left+dat->rcTab.right-neww)>>1;
							ptEnd.y=dat->rcTab.top;
						} else {
							ptEnd.x=(dat->rcDisplay.left+dat->rcDisplay.right-neww)>>1;
							ptEnd.y=(dat->rcDisplay.top+dat->rcDisplay.bottom-newh)>>1;
						}
						if(abs(ptEnd.x-ptStart.x)>5 || abs(ptEnd.y-ptStart.y)>5) {
							startTick=GetTickCount();
							SetWindowPos(dat->opd[i].hwnd,HWND_TOP,0,0,min(neww,rcPage.right),min(newh,rcPage.bottom),SWP_NOMOVE);
							UpdateWindow(dat->opd[i].hwnd);
							for(;;) {
								thisTick=GetTickCount();
								if(thisTick>startTick+100) break;
								ptNow.x=ptStart.x+(ptEnd.x-ptStart.x)*(int)(thisTick-startTick)/100;
								ptNow.y=ptStart.y+(ptEnd.y-ptStart.y)*(int)(thisTick-startTick)/100;
								SetWindowPos(dat->opd[i].hwnd,0,ptNow.x,ptNow.y,0,0,SWP_NOZORDER|SWP_NOSIZE);
							}
						}
						if (dat->opd[i].insideTab)
							ShowWindow(GetDlgItem(hdlg,IDC_TAB),SW_SHOW);
						else
							ShowWindow(GetDlgItem(hdlg,IDC_TAB),SW_HIDE);
					}

					if (dat->opd[i].insideTab) {
						SetWindowPos(dat->opd[i].hwnd,HWND_TOP,(dat->rcTab.left+dat->rcTab.right-neww)>>1,dat->rcTab.top,neww,newh,0);
						ThemeDialogBackground(dat->opd[i].hwnd,TRUE);
					} else {
						SetWindowPos(dat->opd[i].hwnd,HWND_TOP,(dat->rcDisplay.left+dat->rcDisplay.right-neww)>>1,(dat->rcDisplay.top+dat->rcDisplay.bottom-newh)>>1,neww,newh,0);
						ThemeDialogBackground(dat->opd[i].hwnd,FALSE);
					}
				}
				SaveOptionsTreeState(hdlg);
				SendMessage(hdlg,DM_REBUILDPAGETREE,0,0);
				break;
			}
		case IDCANCEL:
			{	int i;
				PSHNOTIFY pshn;
				pshn.hdr.idFrom=0;
				pshn.lParam=0;
				pshn.hdr.code=PSN_RESET;
				for(i=0;i<dat->pageCount;i++) {
					if(dat->opd[i].hwnd==NULL || !dat->opd[i].changed) continue;
					pshn.hdr.hwndFrom=dat->opd[i].hwnd;
					SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
				}
				DestroyWindow(hdlg);
				break;
			}
		case IDOK:
		case IDC_APPLY:
			{
				int i;
				PSHNOTIFY pshn;
				EnableWindow(GetDlgItem(hdlg,IDC_APPLY),FALSE);
				SetFocus(GetDlgItem(hdlg,IDC_PAGETREE));
				if(dat->currentPage!=(-1)) {
					pshn.hdr.idFrom=0;
					pshn.lParam=0;
					pshn.hdr.code=PSN_KILLACTIVE;
					pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
					if(SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn))
						break;
				}

				pshn.hdr.code=PSN_APPLY;
				for(i=0;i<dat->pageCount;i++) {
					if(dat->opd[i].hwnd==NULL || !dat->opd[i].changed) continue;
					dat->opd[i].changed=0;
					pshn.hdr.hwndFrom=dat->opd[i].hwnd;
					if(SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn)==PSNRET_INVALID_NOCHANGEPAGE) {
						dat->hCurrentPage=dat->opd[i].hTreeItem;
						TreeView_SelectItem(GetDlgItem(hdlg,IDC_PAGETREE),dat->hCurrentPage);
						if(dat->currentPage!=(-1)) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_HIDE);
						dat->currentPage=i;
						if (dat->currentPage != (-1)) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);
						return 0;
				}	}

				if ( LOWORD( wParam ) == IDOK )
					DestroyWindow(hdlg);
				break;
		}	}
		break;

	case WM_DESTROY:
		if ( FilterTimerId ) KillTimer ( hdlg, FilterTimerId );
		DestroyWindow ( hFilterSearchWnd );
		ClearFilterStrings();
		szFilterString[0]=0;

		{
			HFONT hBoldFont = ( HFONT )SendDlgItemMessage( hdlg, IDC_NAME, WM_GETFONT, 0, 0 );
			DeleteObject( hBoldFont );
		}

		SaveOptionsTreeState( hdlg );
		Window_FreeIcon_IcoLib( hdlg );
		if ( dat->currentPage != -1 ) {
			if ( dat->opd[dat->currentPage].pszTab )
				DBWriteContactSettingTString( NULL, "Options", "LastTab", dat->opd[dat->currentPage].pszTab );
			else DBDeleteContactSetting( NULL, "Options", "LastTab" );
			if ( dat->opd[dat->currentPage].pszGroup )
				DBWriteContactSettingTString( NULL, "Options", "LastGroup", dat->opd[dat->currentPage].pszGroup );
			else DBDeleteContactSetting( NULL, "Options", "LastGroup" );
			DBWriteContactSettingTString( NULL, "Options", "LastPage", dat->opd[dat->currentPage].pszTitle );
		}
		else {
			DBDeleteContactSetting(NULL,"Options","LastTab");
			DBDeleteContactSetting(NULL,"Options","LastGroup");
			DBDeleteContactSetting(NULL,"Options","LastPage");
		}
		Utils_SaveWindowPosition(hdlg, NULL, "Options", "");
		{
			int i;
			for ( i=0; i < dat->pageCount; i++ ) {
				if ( dat->opd[i].hwnd != NULL )
					DestroyWindow(dat->opd[i].hwnd);
				mir_free(dat->opd[i].pszGroup);
				mir_free(dat->opd[i].pszTab);
				mir_free(dat->opd[i].pszTitle);
				mir_free(dat->opd[i].pTemplate);
		}	}
		mir_free( dat->opd );
		DeleteObject( dat->hBoldFont );
		mir_free( dat );
		hwndOptions = NULL;
		break;
	}
	return FALSE;
}

static void FreeOptionsData( struct OptionsPageInit* popi )
{
	int i;
	for ( i=0; i < popi->pageCount; i++ ) {
		mir_free(( char* )popi->odp[i].pszTitle );
		mir_free( popi->odp[i].pszGroup );
		mir_free( popi->odp[i].pszTab );
		if (( DWORD )popi->odp[i].pszTemplate & 0xFFFF0000 )
			mir_free((char*)popi->odp[i].pszTemplate);
	}
	mir_free(popi->odp);
}

void OpenAccountOptions( PROTOACCOUNT* pa )
{
	struct OptionsPageInit opi = { 0 };
	if ( pa->ppro == NULL )
		return;

	pa->ppro->OnEvent( EV_PROTO_ONOPTIONS, ( WPARAM )&opi, 0 );
	if ( opi.pageCount > 0 ) {
		TCHAR tszTitle[ 100 ];
		OPENOPTIONSDIALOG ood = { 0 };
		PROPSHEETHEADER psh = { 0 };

		mir_sntprintf( tszTitle, SIZEOF(tszTitle), TranslateT("%s options"), pa->tszAccountName );
		
		ood.cbSize = sizeof(ood);
		ood.pszGroup = LPGEN("Network");
		ood.pszPage = mir_t2a( pa->tszAccountName );

		psh.dwSize = sizeof(psh);
		psh.dwFlags = PSH_PROPSHEETPAGE|PSH_NOAPPLYNOW;
		psh.hwndParent = NULL;
		psh.nPages = opi.pageCount;
		psh.pStartPage = (LPCTSTR)&ood;
		psh.pszCaption = tszTitle;
		psh.ppsp = (PROPSHEETPAGE*)opi.odp;
		hwndOptions = CreateDialogParam(hMirandaInst,MAKEINTRESOURCE(IDD_OPTIONSPAGE),NULL,OptionsDlgProc,(LPARAM)&psh);
		mir_free(( void* )ood.pszPage );
		FreeOptionsData( &opi );
}	}

static void OpenOptionsNow(const char *pszGroup,const char *pszPage,const char *pszTab, bool bSinglePage=false)
{
	if ( IsWindow( hwndOptions )) {
		ShowWindow( hwndOptions, SW_RESTORE );
		SetForegroundWindow( hwndOptions );
	}
	else {
		struct OptionsPageInit opi = { 0 };
		NotifyEventHooks( hOptionsInitEvent, ( WPARAM )&opi, 0 );
		if ( opi.pageCount > 0 ) {
			OPENOPTIONSDIALOG ood = { 0 };
			PROPSHEETHEADER psh = { 0 };
			psh.dwSize = sizeof(psh);
			psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
			psh.nPages = opi.pageCount;
			ood.pszGroup = pszGroup;
			ood.pszPage = pszPage;
			ood.pszTab = pszTab;
			psh.pStartPage = (LPCTSTR)&ood;	  //more structure misuse
			psh.pszCaption = TranslateT("Miranda IM Options");
			psh.ppsp = (PROPSHEETPAGE*)opi.odp;		  //blatent misuse of the structure, but what the hell
			hwndOptions = CreateDialogParam(hMirandaInst,MAKEINTRESOURCE(bSinglePage?IDD_OPTIONSPAGE:IDD_OPTIONS),NULL,OptionsDlgProc,(LPARAM)&psh);
			FreeOptionsData( &opi );
}	}	}

static int OpenOptions(WPARAM, LPARAM lParam)
{
	OPENOPTIONSDIALOG *ood=(OPENOPTIONSDIALOG*)lParam;
	if ( ood == NULL )
		return 1;

	if ( ood->cbSize == OPENOPTIONSDIALOG_OLD_SIZE )
		OpenOptionsNow( ood->pszGroup, ood->pszPage, NULL );
	else if (ood->cbSize == sizeof(OPENOPTIONSDIALOG))
		OpenOptionsNow( ood->pszGroup, ood->pszPage, ood->pszTab );
	else
		return 1;

	return 0;
}

static int OpenOptionsPage(WPARAM, LPARAM lParam)
{
	OPENOPTIONSDIALOG *ood=(OPENOPTIONSDIALOG*)lParam;
	if ( ood == NULL )
		return 1;

	if ( ood->cbSize == OPENOPTIONSDIALOG_OLD_SIZE )
		OpenOptionsNow( ood->pszGroup, ood->pszPage, NULL, true );
	else if (ood->cbSize == sizeof(OPENOPTIONSDIALOG))
		OpenOptionsNow( ood->pszGroup, ood->pszPage, ood->pszTab, true );
	else
		return 1;

	return (int)hwndOptions;
}

static int OpenOptionsDialog(WPARAM, LPARAM)
{
	if (GetAsyncKeyState(VK_CONTROL) || !ServiceExists(MS_MODERNOPT_SHOW))
		OpenOptionsNow(NULL,NULL,NULL);
	else
		CallService(MS_MODERNOPT_SHOW, 0, 0);
	return 0;
}

static int AddOptionsPage(WPARAM wParam,LPARAM lParam)
{	OPTIONSDIALOGPAGE *odp=(OPTIONSDIALOGPAGE*)lParam, *dst;
	struct OptionsPageInit *opi=(struct OptionsPageInit*)wParam;

	if(odp==NULL||opi==NULL) return 1;
	if(odp->cbSize!=sizeof(OPTIONSDIALOGPAGE)
			&& odp->cbSize != OPTIONPAGE_OLD_SIZE
			&& odp->cbSize != OPTIONPAGE_OLD_SIZE2
			&& odp->cbSize != OPTIONPAGE_OLD_SIZE3)
		return 1;

	opi->odp=(OPTIONSDIALOGPAGE*)mir_realloc(opi->odp,sizeof(OPTIONSDIALOGPAGE)*(opi->pageCount+1));
	dst = opi->odp + opi->pageCount;
	memset( dst, 0, sizeof( OPTIONSDIALOGPAGE ));
	memcpy( dst, odp, odp->cbSize );

	if ( odp->ptszTitle != NULL ) {
		#if defined( _UNICODE )
			if ( odp->flags & ODPF_UNICODE )
				dst->ptszTitle = mir_wstrdup( TranslateW( odp->ptszTitle ));
			else {
				dst->ptszTitle = LangPackPcharToTchar( odp->pszTitle );
				dst->flags |= ODPF_UNICODE;
			}
		#else
			dst->pszTitle = mir_strdup( Translate( odp->pszTitle ));
		#endif
	}

	if ( odp->ptszGroup != NULL ) {
		#if defined( _UNICODE )
			if ( odp->flags & ODPF_UNICODE )
				dst->ptszGroup = mir_wstrdup( TranslateW( odp->ptszGroup ));
			else {
				dst->ptszGroup = LangPackPcharToTchar( odp->pszGroup );
				dst->flags |= ODPF_UNICODE;
			}
		#else
			dst->pszGroup = mir_strdup( Translate( odp->pszGroup ));
		#endif
	}

	if ( odp->cbSize > OPTIONPAGE_OLD_SIZE2 && odp->ptszTab != NULL ) {
		#if defined( _UNICODE )
			if ( odp->flags & ODPF_UNICODE )
				dst->ptszTab = mir_wstrdup( TranslateW( odp->ptszTab ));
			else {
				dst->ptszTab = LangPackPcharToTchar( odp->pszTab );
				dst->flags |= ODPF_UNICODE;
			}
		#else
			dst->pszTab = mir_strdup( Translate( odp->pszTab ));
		#endif
	}

	if (( DWORD )odp->pszTemplate & 0xFFFF0000 )
		dst->pszTemplate = mir_strdup( odp->pszTemplate );

	opi->pageCount++;
	return 0;
}

static int OptModulesLoaded(WPARAM, LPARAM)
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.flags = CMIF_ICONFROMICOLIB;
	mi.icolibItem = GetSkinIconHandle( SKINICON_OTHER_OPTIONS );
	mi.position = 1900000000;
	mi.pszName = LPGEN("&Options...");
	mi.pszService = "Options/OptionsCommand";
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	return 0;
}

int ShutdownOptionsModule(WPARAM, LPARAM)
{
	if (IsWindow(hwndOptions)) DestroyWindow(hwndOptions);
	hwndOptions=NULL;
	
	UnhookFilterEvents();
	
	return 0;
}

int LoadOptionsModule(void)
{
	hwndOptions=NULL;
	hOptionsInitEvent=CreateHookableEvent(ME_OPT_INITIALISE);
	CreateServiceFunction(MS_OPT_ADDPAGE,AddOptionsPage);
	CreateServiceFunction(MS_OPT_OPENOPTIONS,OpenOptions);
	CreateServiceFunction(MS_OPT_OPENOPTIONSPAGE,OpenOptionsPage);
	CreateServiceFunction("Options/OptionsCommand",OpenOptionsDialog);
	HookEvent(ME_SYSTEM_MODULESLOADED,OptModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN,ShutdownOptionsModule);
	
	HookFilterEvents();
	return 0;
}

void CALLBACK FilterSearchTimerFunc( HWND hwnd, UINT, UINT_PTR, DWORD )
{
	struct OptionsDlgData* dat = (struct OptionsDlgData* )GetWindowLong( hwnd, GWL_USERDATA );
	if ( !dat )
		return;
	
	if ( hFilterSearchWnd == NULL)
		hFilterSearchWnd = CreateWindowA( "STATIC", "Test", WS_OVERLAPPED|WS_DISABLED, CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), 0 ); // Fake window to keep option page focused

	if ( FilterPage < dat->pageCount )
		FindFilterStrings( TRUE, dat->currentPage == FilterPage, hFilterSearchWnd, &( dat->opd[FilterPage]) );		

	FilterPage++;
	FilterLoadProgress = FilterPage*100/( (dat->pageCount) ? dat->pageCount : FilterPage );
	if ( FilterPage >= dat->pageCount )
	{
		KillTimer( hwnd, FilterTimerId );
		FilterTimerId = 0;
		bSearchState = 2;
		FilterLoadProgress = 100;
		DestroyWindow( hFilterSearchWnd );
		hFilterSearchWnd = NULL;
	}
	RedrawWindow( GetDlgItem(hwnd, IDC_KEYWORD_FILTER ), NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE );
}

static void ExecuteFindFilterStringsTimer( HWND hdlg )
{
	bSearchState = 1;
	FilterPage = 0;
	if (FilterTimerId) KillTimer( hdlg, FilterTimerId );
	FilterTimerId = SetTimer( hdlg, NULL, 1, FilterSearchTimerFunc );
}

static void FillFilterCombo(int enableKeywordFiltering, HWND hDlg, struct OptionsPageData * opd, int PageCount)
{
	int i;
	int index;
	HINSTANCE* KnownInstances = ( HINSTANCE* )alloca(sizeof(HINSTANCE)*PageCount);
	int countKnownInst = 0;
	SendDlgItemMessage(hDlg, IDC_KEYWORD_FILTER,(UINT) CB_RESETCONTENT, 0,0);
	index=SendDlgItemMessage(hDlg, IDC_KEYWORD_FILTER,(UINT) CB_ADDSTRING,(WPARAM)0, (LPARAM)TranslateTS(ALL_MODULES_FILTER));
	SendDlgItemMessage(hDlg, IDC_KEYWORD_FILTER,(UINT) CB_SETITEMDATA,(WPARAM)index, (LPARAM)NULL);
	index=SendDlgItemMessage(hDlg, IDC_KEYWORD_FILTER,(UINT) CB_ADDSTRING,(WPARAM)0, (LPARAM)TranslateTS(CORE_MODULES_FILTER));
	SendDlgItemMessage(hDlg, IDC_KEYWORD_FILTER,(UINT) CB_SETITEMDATA,(WPARAM)index, (LPARAM)hMirandaInst);
	TCHAR* tszModuleName = ( TCHAR* )alloca(MAX_PATH*sizeof(TCHAR));
	for (i=0; i<PageCount; i++) {		
		TCHAR * dllName = NULL;
		int j;
		HINSTANCE inst=opd[i].hInst;
		
		if ( !enableKeywordFiltering )
			FindFilterStrings( enableKeywordFiltering, FALSE, hDlg, &opd[i]); // only modules name ( fast enougth )
		
		if (inst==hMirandaInst) continue;
		for (j=0; j<countKnownInst; j++)
			if (KnownInstances[j]==inst) break;
		if (j!=countKnownInst) continue;
		KnownInstances[countKnownInst]=inst;
		countKnownInst++;
		GetModuleFileName(inst, tszModuleName, MAX_PATH);
		{
			char * name = GetPluginNameByInstance( inst );
			if ( name )
				dllName = a2t( name ); 
		}

		if (!dllName) dllName = mir_tstrdup( _tcsrchr(tszModuleName,_T('\\')) );
		if (!dllName) dllName = mir_tstrdup( tszModuleName );
		
		if (dllName) {
			index=SendDlgItemMessage(hDlg, IDC_KEYWORD_FILTER,(UINT) CB_ADDSTRING,(WPARAM)0, (LPARAM)dllName);
			SendDlgItemMessage(hDlg, IDC_KEYWORD_FILTER,(UINT) CB_SETITEMDATA,(WPARAM)index, (LPARAM)inst);
			mir_free( dllName );
		}	
	}

	FilterLoadProgress = 100;
	if ( enableKeywordFiltering)
		ExecuteFindFilterStringsTimer( hDlg );
}
