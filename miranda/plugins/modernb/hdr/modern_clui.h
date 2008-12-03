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
#ifndef modern_clui_h__
#define modern_clui_h__

#include "windowsX.h"
#define HANDLE_MESSAGE( _message, _fn)    \
	case (_message): return _fn( (hwnd), (_message), (wParam), (lParam) )

class CLUI
{
private:
	CLUI();			// is protected use InitClui to initialize instead
  

public:    

	static HRESULT InitClui();
	~CLUI();

	static HWND& ClcWnd();		//TODO: move to Clc.h
	static HWND& CluiWnd();
	
	CLINTERFACE void cliOnCreateClc();

	EVENTHOOK( OnEvent_ModulesLoaded );
	EVENTHOOK( OnEvent_ContactMenuPreBuild );
	EVENTHOOK( OnEvent_DBSettingChanging );
	
	SERVICE( Service_ShowMainMenu );
	SERVICE( Service_ShowStatusMenu );
	SERVICE( Service_Menu_ShowContactAvatar );
	SERVICE( Service_Menu_HideContactAvatar );

	static CLUI * m_pCLUI;  
	static BOOL m_fMainMenuInited;
	
	static HRESULT FillAlphaChannel(HWND hwndClui, HDC hDC, RECT* prcParent, BYTE bAlpha);
	static HRESULT CreateCLC(HWND hwndClui);
	static HRESULT SnappingToEdge(HWND hCluiWnd, WINDOWPOS * lpWindowPos);
	static BOOL IsMainMenuInited() { return CLUI::m_fMainMenuInited; }
	
	static LRESULT DefCluiWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		return corecli.pfnContactListWndProc( hwnd, msg, wParam, lParam );
	}

	// MessageMap
	static LRESULT CALLBACK cli_ContactListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		BOOL bHandled = FALSE;
		LRESULT lRes= PreProcessWndProc( hwnd, msg, wParam, lParam, bHandled ); 
		if ( bHandled ) return lRes;

		switch ( msg )
		{
			HANDLE_MESSAGE( WM_NCCREATE,				OnNcCreate );
			HANDLE_MESSAGE( WM_CREATE,					OnCreate );
			HANDLE_MESSAGE( UM_CREATECLC,				OnCreateClc );
			HANDLE_MESSAGE( UM_SETALLEXTRAICONS,		OnSetAllExtraIcons );
			HANDLE_MESSAGE( WM_INITMENU,				OnInitMenu );
			HANDLE_MESSAGE( WM_SIZE,					OnSizingMoving );
			HANDLE_MESSAGE( WM_SIZING,					OnSizingMoving );
			HANDLE_MESSAGE( WM_MOVE,					OnSizingMoving );
			HANDLE_MESSAGE( WM_EXITSIZEMOVE,			OnSizingMoving );
			HANDLE_MESSAGE( WM_WINDOWPOSCHANGING,		OnSizingMoving );
			HANDLE_MESSAGE( WM_THEMECHANGED,			OnThemeChanged );
			HANDLE_MESSAGE( WM_DWMCOMPOSITIONCHANGED,	OnDwmCompositionChanged );
			HANDLE_MESSAGE( UM_SYNCCALL,				OnSyncCall );
			HANDLE_MESSAGE( UM_UPDATE,					OnUpdate );
			HANDLE_MESSAGE( WM_NCACTIVATE,				OnNcPaint );
			HANDLE_MESSAGE( WM_PRINT,					OnNcPaint );
			HANDLE_MESSAGE( WM_NCPAINT,					OnNcPaint );
			HANDLE_MESSAGE( WM_ERASEBKGND,				OnEraseBkgnd );
			HANDLE_MESSAGE( WM_PAINT,					OnPaint );
			HANDLE_MESSAGE( WM_LBUTTONDOWN,				OnLButtonDown );
			HANDLE_MESSAGE( WM_PARENTNOTIFY,			OnParentNotify );
			HANDLE_MESSAGE( WM_SETFOCUS,				OnSetFocus );
			HANDLE_MESSAGE( WM_TIMER,					OnTimer );
			HANDLE_MESSAGE( WM_ACTIVATE,				OnActivate );
			HANDLE_MESSAGE( WM_SETCURSOR,				OnSetCursor );
			HANDLE_MESSAGE( WM_MOUSEACTIVATE,			OnMouseActivate );
			HANDLE_MESSAGE( WM_NCLBUTTONDOWN,			OnNcLButtonDown );
			HANDLE_MESSAGE( WM_NCLBUTTONDBLCLK,			OnNcLButtonDblClk );
			HANDLE_MESSAGE( WM_NCHITTEST,				OnNcHitTest );
			HANDLE_MESSAGE( WM_SHOWWINDOW,				OnShowWindow );
			HANDLE_MESSAGE( WM_SYSCOMMAND,				OnSysCommand );
			HANDLE_MESSAGE( WM_KEYDOWN,					OnKeyDown );
			HANDLE_MESSAGE( WM_GETMINMAXINFO,			OnGetMinMaxInfo );
			HANDLE_MESSAGE( WM_MOVING,					OnMoving );
			HANDLE_MESSAGE( WM_NOTIFY,					OnNotify );
			HANDLE_MESSAGE( WM_CONTEXTMENU,				OnContextMenu );
			HANDLE_MESSAGE( WM_MEASUREITEM,				OnMeasureItem );
			HANDLE_MESSAGE( WM_DRAWITEM,				OnDrawItem );
			HANDLE_MESSAGE( WM_DESTROY,					OnDestroy );
		default:
			return DefCluiWndProc( hwnd, msg, wParam, lParam );
		}
		return FALSE;
	}

private:
	static LRESULT PreProcessWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL bHandled = FALSE );
	static LRESULT OnSizingMoving( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnThemeChanged( HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ );
	static LRESULT OnDwmCompositionChanged( HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ );
	static LRESULT OnSyncCall( HWND /*hwnd*/, UINT /*msg*/, WPARAM wParam, LPARAM /*lParam*/ );
	static LRESULT OnUpdate( HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ );
	static LRESULT OnInitMenu( HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ );
	static LRESULT OnNcPaint( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnEraseBkgnd( HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ );
	static LRESULT OnNcCreate( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnPaint( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnCreate( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnSetAllExtraIcons( HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ );
	static LRESULT OnCreateClc( HWND hwnd, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ );
	static LRESULT OnLButtonDown( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnParentNotify( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnSetFocus( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	
	static LRESULT OnTimer( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnStatusBarUpdateTimer( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnAutoAlphaTimer( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnSmoothAlphaTransitionTimer( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnDelayedSizingTimer( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnBringOutTimer( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnBringInTimer( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnUpdateBringTimer( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

	static LRESULT OnActivate( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnSetCursor( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnMouseActivate( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnNcLButtonDown( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnNcLButtonDblClk( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnNcHitTest( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnShowWindow( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnSysCommand( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnKeyDown( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnGetMinMaxInfo( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnMoving( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnNotify( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

	static LRESULT OnNewContactNotify( HWND hwnd, NMCLISTCONTROL * pnmc );
	static LRESULT OnListRebuildNotify( HWND hwnd, NMCLISTCONTROL * pnmc );
	static LRESULT OnListSizeChangeNotify( HWND hwnd, NMCLISTCONTROL * pnmc );
	static LRESULT OnClickNotify( HWND hwnd, NMCLISTCONTROL * pnmc );

	static LRESULT OnContextMenu( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnMeasureItem( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnDrawItem( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT OnDestroy( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

private:
	HRESULT LoadDllsRuntime();
	HRESULT RegisterAvatarMenu();  // TODO move to CLC class
	HRESULT CreateCluiFrames();
	HRESULT CreateCLCWindow(const HWND parent);
	HRESULT CreateUIFrames();

	
protected:
	HMODULE m_hDwmapiDll;
	HMODULE m_hUserDll;

	enum { SNAPTOEDGESENSIVITY = 30 };
};

#endif // modern_clui_h__