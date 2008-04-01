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

#define SERVICE(serviceproc)        static int serviceproc(WPARAM wParam,LPARAM lParam)
#define EVENTHOOK(eventhookproc)    static int eventhookproc(WPARAM wParam,LPARAM lParam)
#define CLINTERFACE  static

class CLUI
{
public:
    CLUI();
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

	static HRESULT FillAlphaChannel(HWND hwndClui, HDC hDC, RECT* prcParent, BYTE bAlpha);
	static HRESULT CreateCLC(HWND hwndClui);
	static HRESULT SnappingToEdge(HWND hCluiWnd, WINDOWPOS * lpWindowPos);

private:
    HRESULT LoadDllsRuntime();
    HRESULT RegisterAvatarMenu();  // TODO move to CLC class
    HRESULT CreateCLCWindow(const HWND parent);
    HRESULT CreateUIFrames();
    
protected:
	HMODULE m_hDwmapiDll;
    HMODULE m_hUserDll;
};

#endif // modern_clui_h__