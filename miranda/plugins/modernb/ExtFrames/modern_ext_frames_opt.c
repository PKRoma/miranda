/**************************************************************************\

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
all portions of this code base are copyrighted to Artem Shpynov and/or
the people listed in contributors.txt.

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

****************************************************************************

Created: Mar 19, 2007

Author and Copyright:  Artem Shpynov aka FYR:  ashpynov@gmail.com

****************************************************************************

File contains realization of options procedures 
for modern_ext_frames.c module.

This file have to be excluded from compilation and need to be adde to project via
#include preprocessor derective in modern_ext_frames.c

\**************************************************************************/

#include "..\commonheaders.h"  //only for precompiled headers

#ifdef __modern_ext_frames_c__include_c_file   //protection from adding to compilation

static int _ExtFrames_OptionsDlgInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;
	efcheck 0;
	if (MirandaExiting()) return 0;
	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=0;
	odp.hInstance=g_hInst;
	//odp.ptszGroup=TranslateT("Contact List");
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_EXTFRAMES);
	odp.ptszTitle=TranslateT("Contact List");
	odp.pfnDlgProc=_ExtFrames_DlgProcFrameOpts;
	odp.ptszTab=TranslateT("Frames");
	odp.flags=ODPF_BOLDGROUPS|ODPF_EXPERTONLY|ODPF_TCHAR;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}
static BOOL CALLBACK _ExtFrames_DlgProcFrameOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			return TRUE;
		}		
	case 0:
		switch (((LPNMHDR)lParam)->code)
		{
			case PSN_APPLY:
				{

				}
				return TRUE;
		}
	}
	return FALSE;
}

#endif