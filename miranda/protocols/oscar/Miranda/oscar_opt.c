/*

Copyright 2005 Sam Kothari (egoDust)

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

#include "miranda.h"
#include <resource.h>

static BOOL CALLBACK DlgUserInfoOpt(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg ) {
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			return TRUE;
		}
	}
	return FALSE;
}

static int MirandaOptInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize=sizeof(odp);
	odp.flags=ODPF_BOLDGROUPS;
	odp.position=-10231023;
	odp.pszGroup=Translate("Network");
	odp.pszTitle=Translate("OSCAR");
	odp.pfnDlgProc=DlgUserInfoOpt;
	odp.pszTemplate=MAKEINTRESOURCE(IDD_OSCAR_USERINFO);
	odp.hInstance=g_hInst;
	//CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	return 0;
}

void LoadOptionsServices(void)
{
	HookEvent(ME_OPT_INITIALISE, MirandaOptInit);
}


