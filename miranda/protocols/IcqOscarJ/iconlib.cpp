// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
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
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Support for IcoLib plug-in
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"
#include "m_icolib.h"

HANDLE CIcqProto::IconLibDefine(const char *desc, const char *section, const char *ident, const TCHAR *def_file, int def_idx)
{
  SKINICONDESC sid = {0};
  char szTemp[MAX_PATH];
  char szName[MAX_PATH + 128];
  HANDLE hIcon;

  sid.cbSize = SKINICONDESC_SIZE;
  sid.pwszSection = make_unicode_string(section);
  sid.pwszDescription = make_unicode_string(ICQTranslateUtfStatic(desc, szTemp, MAX_PATH));
  sid.flags = SIDF_UNICODE | SIDF_PATH_TCHAR;

  null_snprintf(szName, sizeof(szName), "%s_%s", m_szModuleName, ident);
  sid.pszName = szName;
  sid.ptszDefaultFile = (TCHAR*)def_file;
  sid.iDefaultIndex = def_idx;
  sid.cx = sid.cy = 16;

  hIcon = (HANDLE)CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

  SAFE_FREE((void**)&sid.pwszSection);
  SAFE_FREE((void**)&sid.pwszDescription);

  return hIcon;
}

HICON CIcqProto::IconLibGetIcon(const char *ident)
{
  char szTemp[MAX_PATH + 128];

  null_snprintf(szTemp, sizeof(szTemp), "%s_%s", m_szModuleName, ident);

  return (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)szTemp);
}

void CIcqProto::IconLibReleaseIcon(const char *ident)
{
  char szTemp[MAX_PATH + 128];

  null_snprintf(szTemp, sizeof(szTemp), "%s_%s", m_szModuleName, ident);
  CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)szTemp);
}
