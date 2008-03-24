// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2001-2004 Richard Hughes, Martin Öberg
// Copyright © 2004-2008 Joe Kucera, Bio
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

void CIcqProto::LoadSettingsFromDb(int keepChanged)
{
	for ( int i=0; i < settingCount; i++ ) {
		if (setting[i].displayType == LI_DIVIDER) continue;
		if (keepChanged && setting[i].changed) continue;
		if (setting[i].dbType == DBVT_ASCIIZ) 
			SAFE_FREE((void**)(char**)&setting[i].value);
		else if (!keepChanged)
			setting[i].value = 0;

		setting[i].changed=0;

		if (setting[i].displayType & LIF_PASSWORD) continue;

		DBVARIANT dbv;
		if (!getSetting(NULL,setting[i].szDbSetting,&dbv)) {
			switch(dbv.type) {
			case DBVT_ASCIIZ:
				setting[i].value=(LPARAM)getStringUtf(NULL,setting[i].szDbSetting, NULL);
				break;
			case DBVT_UTF8:
				setting[i].value=(LPARAM)null_strdup(dbv.pszVal);
				break;
			case DBVT_WORD:
				if(setting[i].displayType&LIF_SIGNED) 
					setting[i].value=dbv.sVal;
				else 
					setting[i].value=dbv.wVal;
				break;
			case DBVT_BYTE:
				if(setting[i].displayType&LIF_SIGNED) 
					setting[i].value=dbv.cVal;
				else 
					setting[i].value=dbv.bVal;
				break;
#ifdef _DEBUG
			default:
				MessageBoxA(NULL,"That's not supposed to happen either","Huh?",MB_OK);
				break;
#endif
			}
			ICQFreeVariant(&dbv);
		}
	}
}

void CIcqProto::FreeStoredDbSettings(void)
{
	for ( int i=0; i < settingCount; i++ )
		if (setting[i].dbType == DBVT_ASCIIZ)
			SAFE_FREE((void**)(char**)&setting[i].value);
}

int CIcqProto::ChangesMade(void)
{
	for (int i=0; i < settingCount; i++ )
		if (setting[i].changed)
			return 1;
	return 0;
}

void CIcqProto::ClearChangeFlags(void)
{
	for (int i=0; i < settingCount; i++)
		setting[i].changed = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct PwConfirmDlgParam
{
	CIcqProto* ppro;
	char* Pass;
};

static BOOL CALLBACK PwConfirmDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PwConfirmDlgParam* dat = (PwConfirmDlgParam*)GetWindowLong(hwndDlg, GWL_USERDATA);

	switch(msg) {
	case WM_INITDIALOG:
		ICQTranslateDialog(hwndDlg);
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		SendDlgItemMessage(hwndDlg,IDC_PASSWORD,EM_LIMITTEXT,15,0);
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK:
			{  
				char szTest[16];

				GetDlgItemTextA(hwndDlg,IDC_OLDPASS,szTest,sizeof(szTest));

				if (strcmpnull(szTest, dat->ppro->GetUserPassword(TRUE))) 
				{
					MessageBoxUtf(hwndDlg, LPGEN("The password does not match your current password. Check Caps Lock and try again."), LPGEN("Change ICQ Details"), MB_OK);
					SendDlgItemMessage(hwndDlg,IDC_OLDPASS,EM_SETSEL,0,(LPARAM)-1);
					SetFocus(GetDlgItem(hwndDlg,IDC_OLDPASS));
					break;
				}

				GetDlgItemTextA(hwndDlg,IDC_PASSWORD,szTest,sizeof(szTest));
				if(strcmpnull(szTest, dat->Pass)) 
				{
					MessageBoxUtf(hwndDlg, LPGEN("The password does not match the password you originally entered. Check Caps Lock and try again."), LPGEN("Change ICQ Details"), MB_OK);
					SendDlgItemMessage(hwndDlg,IDC_PASSWORD,EM_SETSEL,0,(LPARAM)-1);
					SetFocus(GetDlgItem(hwndDlg,IDC_PASSWORD));
					break;
				}
			}
		case IDCANCEL:
			EndDialog(hwndDlg,wParam);
			break;
		}
		break;
	}
	return FALSE;
}

int CIcqProto::SaveSettingsToDb(HWND hwndDlg)
{
	int i,ret=1;

	for( i=0; i < settingCount; i++ ) {
		if(!setting[i].changed) continue;
		if(!(setting[i].displayType & LIF_ZEROISVALID) && setting[i].value==0) {
			DeleteSetting(NULL,setting[i].szDbSetting);
			continue;
		}
		switch(setting[i].dbType) {
		case DBVT_ASCIIZ:
			if(setting[i].displayType&LIF_PASSWORD) {
				int nSettingLen = strlennull((char*)setting[i].value);

				if (nSettingLen > 8 || nSettingLen < 1) {
					MessageBoxUtf(hwndDlg, LPGEN("The ICQ server does not support passwords longer than 8 characters. Please use a shorter password."), LPGEN("Change ICQ Details"), MB_OK);
					ret=0;
					break;
				}
				PwConfirmDlgParam param = { this, (char*)setting[i].value };
				if (IDOK != DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_PWCONFIRM),hwndDlg,PwConfirmDlgProc,(LPARAM)&param)) {
					ret = 0;
					break;
				}
				strcpy(m_szPassword, (char*)setting[i].value);
			}
			else {
				if(*(char*)setting[i].value)
					setStringUtf(NULL,setting[i].szDbSetting,(char*)setting[i].value);
				else
					DeleteSetting(NULL,setting[i].szDbSetting);
			}
			break;
		case DBVT_WORD:
			setWord(NULL,setting[i].szDbSetting,(WORD)setting[i].value);
			break;
		case DBVT_BYTE:
			setByte(NULL,setting[i].szDbSetting,(BYTE)setting[i].value);
			break;
		}
	}
	return ret;
}
