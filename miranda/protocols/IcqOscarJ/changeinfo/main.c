// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2001,2002,2003,2004 Richard Hughes, Martin Öberg
// Copyright © 2004,2005 Joe Kucera, Bio
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $Source$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  ChangeInfo Plugin stuff
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


int InitChangeDetails(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp={0};
	char title[128];

	if((HANDLE)lParam != NULL) return 0;

	null_snprintf(title, sizeof(title),"%s %s", Translate(gpszICQProtoName), Translate("Details"));

	odp.cbSize=sizeof(odp);
	odp.hIcon = NULL;
	odp.hInstance = hInst;
  odp.position = -1899999999;
	odp.pszTemplate=MAKEINTRESOURCE(IDD_INFO_CHANGEINFO);
	odp.pszTitle=title;
	odp.pfnDlgProc=ChangeInfoDlgProc;

	CallService(MS_USERINFO_ADDPAGE,wParam,(LPARAM)&odp);

	return 0;
}
