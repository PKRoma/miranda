/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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

#include "IcoLib.h"

HANDLE hEventIconsChanged;

int InitSkin2Icons(void);
void UninitSkin2Icons(void);
BOOL CALLBACK DlgProcIcoLibOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

static UINT iconsExpertOnlyControls[]={IDC_IMPORT};
static int SkinOptionsInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};

	odp.cbSize = sizeof(odp);
	odp.hInstance = GetModuleHandle(NULL);
	odp.flags = ODPF_BOLDGROUPS | ODPF_TCHAR;
	odp.position = -180000000;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_ICOLIB);
	odp.ptszTitle = TranslateT("Icons");
	odp.ptszGroup = TranslateT("Customize");
	odp.pfnDlgProc = DlgProcIcoLibOpts;
	odp.expertOnlyControls = iconsExpertOnlyControls;
	odp.nExpertOnlyControls = SIZEOF(iconsExpertOnlyControls);
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	return 0;
}

static int SkinSystemModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	HookEvent(ME_OPT_INITIALISE, SkinOptionsInit);
	return 0;
}

//
//   Miranda's module load/unload code
//

int LoadIcoLibModule( void )
{
	HookEvent(ME_SYSTEM_MODULESLOADED, SkinSystemModulesLoaded);

	if (InitSkin2Icons()) return 1;

	return 0;
}
