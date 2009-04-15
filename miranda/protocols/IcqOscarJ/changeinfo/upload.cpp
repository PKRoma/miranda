// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright � 2001-2004 Richard Hughes, Martin �berg
// Copyright � 2004-2009 Joe Kucera, Bio
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
//  ChangeInfo Plugin stuff
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


int CIcqProto::StringToListItemId(const char *szSetting,int def)
{
	int i;
	char *szValue;
	FieldNamesItem *list;

	for(i=0;i<settingCount;i++)
		if(!strcmpnull(szSetting,setting[i].szDbSetting))
			break;

	if (i==settingCount) return def;

	list = (FieldNamesItem*)setting[i].pList;

	szValue = getSettingStringUtf(NULL, szSetting, NULL);
	if (!szValue)
		return def;

  for (i=0; list[i].text; i++)
		if (!strcmpnull(list[i].text, szValue))
			break;

	SAFE_FREE((void**)&szValue);
	if (!list[i].text) return def;

	return list[i].code;
}


int ChangeInfoData::UploadSettings(void)
{
	if (!ppro->icqOnline())
	{
		MessageBoxUtf(hwndDlg, LPGEN("You are not currently connected to the ICQ network. You must be online in order to update your information on the server."), LPGEN("Change ICQ Details"), MB_OK);
		return 0;
	}

	hUpload[0] = (HANDLE)ppro->ChangeInfoEx(CIXT_FULL, 0);

	//password
	char* tmp = ppro->GetUserPassword(TRUE);
	if (tmp)
	{
		if (strlennull(Password) > 0 && strcmpnull(Password, tmp))
		{
			hUpload[1] = (HANDLE)ppro->icq_changeUserPasswordServ(tmp);
			char szPwd[16] = {0};

			if (!ppro->getSettingStringStatic(NULL, "Password", szPwd, 16) && strlennull(szPwd))
			{ // password is stored in DB, update
				char ptmp[16];

				strcpy(ptmp, tmp);

				CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(ptmp), (LPARAM)ptmp);

				ppro->setSettingString(NULL, "Password", ptmp);
			}
		}
	}

	return 1;
}
