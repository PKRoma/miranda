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


void LoadSettingsFromDb(int keepChanged)
{
	int i;
	DBVARIANT dbv;

	for(i=0;i<settingCount;i++) 
  {
		if(setting[i].displayType==LI_DIVIDER) continue;
		if(keepChanged && setting[i].changed) continue;
		if(setting[i].dbType==DBVT_ASCIIZ) 
		{
			SAFE_FREE(&(char*)setting[i].value);
		}
		else if(!keepChanged) 
			setting[i].value = 0;

		setting[i].changed=0;

		if(!ICQGetContactSetting(NULL,setting[i].szDbSetting,&dbv)) 
    {
#ifdef _DEBUG
			if(dbv.type!=setting[i].dbType)
				MessageBox(NULL,"That's not supposed to happen","Huh?",MB_OK);
#endif
			switch(dbv.type) 
      {
				case DBVT_ASCIIZ:
					setting[i].value=(LPARAM)_strdup(dbv.pszVal);
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
					MessageBox(NULL,"That's not supposed to happen either","Huh?",MB_OK);
					break;
#endif
			}
			DBFreeVariant(&dbv);
		}
	}
}


void FreeStoredDbSettings(void)
{
	int i;

	for(i=0;i<settingCount;i++)
		if(setting[i].dbType==DBVT_ASCIIZ)
			SAFE_FREE(&(char*)setting[i].value);
}


int ChangesMade(void)
{
	int i;

	for(i=0;i<settingCount;i++)
		if(setting[i].changed) return 1;
	return 0;
}


void ClearChangeFlags(void)
{
	int i;

	for(i=0;i<settingCount;i++)
		setting[i].changed=0;
}


static BOOL CALLBACK PwConfirmDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static char *Pass;

	switch(msg) 
  {
		case WM_INITDIALOG:
			ICQTranslateDialog(hwndDlg);
			Pass = (char*)lParam;
			SendDlgItemMessage(hwndDlg,IDC_PASSWORD,EM_LIMITTEXT,63,0);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) 
      {
				case IDOK:
					{	
            char szTest[64];

						GetDlgItemText(hwndDlg,IDC_PASSWORD,szTest,sizeof(szTest));
						if(strcmpnull(szTest, Pass)) 
            {
							MessageBox(hwndDlg,Translate("This password does not match the password you originally entered. Check Caps Lock and try again."),Translate("Change ICQ Details"),MB_OK);
							SendDlgItemMessage(hwndDlg,IDC_PASSWORD,EM_SETSEL,0,(LPARAM)-1);
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


int SaveSettingsToDb(HWND hwndDlg)
{
	int i,ret=1;

	for(i=0;i<settingCount;i++) 
  {
		if(!setting[i].changed) continue;
		if(!(setting[i].displayType&LIF_ZEROISVALID) && setting[i].value==0) 
    {
			ICQDeleteContactSetting(NULL,setting[i].szDbSetting);
			continue;
		}
		switch(setting[i].dbType) 
    {
			case DBVT_ASCIIZ:
				if(setting[i].displayType&LIF_PASSWORD) 
        {
					char szScrambled[64];
					if(strlen((char*)setting[i].value)>9 || strlen((char*)setting[i].value)<1) 
          {
						MessageBox(hwndDlg,Translate("The ICQ server does not support passwords longer than 9 characters. Please use a shorter password."),Translate("Change ICQ Details"),MB_OK);
						ret=0;
						break;
					}
					if(IDOK!=DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_PWCONFIRM),hwndDlg,PwConfirmDlgProc,(LPARAM)setting[i].value)) 
          {
						ret=0;
						break;
					}
					lstrcpyn(szScrambled,(char*)setting[i].value,sizeof(szScrambled));
					CallService(MS_DB_CRYPT_ENCODESTRING,sizeof(szScrambled),(LPARAM)szScrambled);
					ICQWriteContactSettingString(NULL,setting[i].szDbSetting,szScrambled);
				}
				else 
        {
					if(*(char*)setting[i].value)
						ICQWriteContactSettingString(NULL,setting[i].szDbSetting,(char*)setting[i].value);
					else
						ICQDeleteContactSetting(NULL,setting[i].szDbSetting);
				}
				break;
			case DBVT_WORD:
				ICQWriteContactSettingWord(NULL,setting[i].szDbSetting,(WORD)setting[i].value);
				break;
			case DBVT_BYTE:
				ICQWriteContactSettingByte(NULL,setting[i].szDbSetting,(BYTE)setting[i].value);
				break;
		}
	}
	return ret;
}
