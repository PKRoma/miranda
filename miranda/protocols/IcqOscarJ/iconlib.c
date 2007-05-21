// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006,2007 Joe Kucera
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


static int bIcoReady = 0;
static int bIcoUtf = 0;


void InitIconLib()
{ // check plugin presence, init variables
  bIcoReady = ServiceExists(MS_SKIN2_GETICON);
  if (bIcoReady)
  {
    SKINICONDESC sid = {0};

    if (ServiceExists(MS_SKIN2_RELEASEICON) || (CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid) >= PLUGIN_MAKE_VERSION(0,0,1,0)))
      bIcoUtf = 1;
  }
}



int IconLibInstalled()
{
  return bIcoReady;
}



void IconLibDefine(const char* desc, const char* section, const char* ident, HICON icon, const char* def_file, int def_idx)
{
  if (bIcoReady)
  {
    SKINICONDESC sid = {0};
    char szTemp[MAX_PATH + 128];

    if (bIcoUtf)
    {
      sid.cbSize = SKINICONDESC_SIZE;
      sid.pwszSection = make_unicode_string(section);
      sid.pwszDescription = make_unicode_string(desc);
      sid.flags = SIDF_UNICODE;
    }
    else
    {
      sid.cbSize = SKINICONDESC_SIZE_V3;
      utf8_decode(section, &sid.pszSection);
      utf8_decode(desc, &sid.pszDescription);
    }
    null_snprintf(szTemp, sizeof(szTemp), "%s_%s", gpszICQProtoName, ident);
    sid.pszName = szTemp;
    sid.pszDefaultFile = (char*)def_file;
    sid.iDefaultIndex = def_idx;
    sid.hDefaultIcon = icon;
    sid.cx = sid.cy = 16;

    CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

    SAFE_FREE(&sid.pwszSection);
    SAFE_FREE(&sid.pwszDescription);
  }
}



HICON IconLibGetIcon(const char* ident)
{
  if (bIcoReady)
  {
		char szTemp[MAX_PATH + 128];
    HICON hNew;

		null_snprintf(szTemp, sizeof(szTemp), "%s_%s", gpszICQProtoName, ident);
		hNew = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)szTemp);
    if (hNew) return hNew;
  }

  return NULL;
}



void IconLibReleaseIcon(const char* ident)
{
  if (bIcoReady)
  {
		char szTemp[MAX_PATH + 128];

		null_snprintf(szTemp, sizeof(szTemp), "%s_%s", gpszICQProtoName, ident);
		CallService(MS_SKIN2_RELEASEICON, 0, (LPARAM)szTemp);
  }
}



HANDLE IconLibHookIconsChanged(MIRANDAHOOK hook)
{
  if (bIcoReady)
  {
    return HookEvent(ME_SKIN2_ICONSCHANGED, hook);
  }
  return NULL;
}
