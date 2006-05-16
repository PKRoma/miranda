// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2001,2002,2003,2004 Richard Hughes, Martin �berg
// Copyright � 2004,2005,2006 Joe Kucera, Bio
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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/changeinfo/main.c,v $
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


int InitChangeDetails(WPARAM wParam, LPARAM lParam)
{
  OPTIONSDIALOGPAGE odp = {0};

  if (lParam) return 0;

  odp.cbSize = sizeof(odp);
  odp.hIcon = NULL;
  odp.hInstance = hInst;
  odp.position = -1899999999;
  odp.pszTemplate = MAKEINTRESOURCE(IDD_INFO_CHANGEINFO);
  odp.pfnDlgProc = ChangeInfoDlgProc;

  AddUserInfoPageUtf(&odp, wParam, "%s Details");

  return 0;
}
