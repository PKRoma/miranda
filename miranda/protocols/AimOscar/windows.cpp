#include "windows.h"
int OptionsInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;
	odp.cbSize = sizeof(odp);
    odp.position = 1003000;
    odp.hInstance = conn.hInstance;
    odp.pszTemplate = MAKEINTRESOURCE(IDD_AIM);
    odp.pszGroup = Translate("Network");
    odp.pszTitle = Translate("AimOSCAR");
    odp.pfnDlgProc = options_dialog;
    odp.flags = ODPF_BOLDGROUPS;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);
	return 0;
}
int UserInfoInit(WPARAM wParam,LPARAM lParam)
{
	if(!lParam)//hContact
	{
		OPTIONSDIALOGPAGE odp;
		odp.cbSize = sizeof(odp);
		odp.position = -1900000000;
		odp.hInstance = conn.hInstance;
		odp.pszTemplate = MAKEINTRESOURCE(IDD_INFO);
		odp.pszTitle = Translate("AimOSCAR");
		odp.pfnDlgProc = userinfo_dialog;
		CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM) & odp);
	}
	return 0;
}
static BOOL CALLBACK userinfo_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
	{
        case WM_INITDIALOG:
        {
			DBVARIANT dbv;
			if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PR, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_PROFILE, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			break;
		}
		case WM_COMMAND:
        {
			switch (LOWORD(wParam))
			{
				case IDC_PROFILE:
				{
					if (HIWORD(wParam) == EN_CHANGE)
						EnableWindow(GetDlgItem(hwndDlg, IDC_SETPROFILE), TRUE);
					break;
				}
				case IDC_SETPROFILE:
                {
                    char buf[2048];//MAX
                    GetWindowText(GetDlgItem(hwndDlg, IDC_PROFILE), buf, sizeof(buf));
                    DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PR, buf);
                    if (conn.state==1)
					{
                        aim_set_profile(buf);
                    }
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SETPROFILE), FALSE);
                    break;
                }
			}
				if ((HWND) lParam != GetFocus())
					return 0;
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
		}
        case WM_NOTIFY:
        {
			switch (((LPNMHDR) lParam)->code)
			{
				case PSN_APPLY:
				{

				}
			}
            break;
        }
    }
    return FALSE;
}
static BOOL CALLBACK options_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
	{
        case WM_INITDIALOG:
        {

            DBVARIANT dbv;
            TranslateDialogDefault(hwndDlg);
            if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_SN, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_NK, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_NK, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			else if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_NK, dbv.pszVal);
                DBFreeVariant(&dbv);
			}
            if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, &dbv))
			{
                CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
                SetDlgItemText(hwndDlg, IDC_PW, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_HN, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DG, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_DG, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
			unsigned short timeout=DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, DEFAULT_GRACE_PERIOD);
			SetDlgItemInt(hwndDlg, IDC_GP, timeout,0);
			unsigned short timer=DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA, DEFAULT_KEEPALIVE_TIMER);
			SetDlgItemInt(hwndDlg, IDC_KA, timer,0);
			CheckDlgButton(hwndDlg, IDC_DC, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DC, 0));//Message Delivery Confirmation
            CheckDlgButton(hwndDlg, IDC_FP, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FP, 0));//force proxy
			CheckDlgButton(hwndDlg, IDC_AT, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_AT, 0));//Account Type Icons
			CheckDlgButton(hwndDlg, IDC_HF, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HF, 0));//Fake hiptopness
			break;
		}
		case WM_COMMAND:
        {
            if ((HWND) lParam != GetFocus())
                return 0;
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        }
        case WM_NOTIFY:
        {
            switch (((LPNMHDR) lParam)->code)
			{
                case PSN_APPLY:
                {
                    char str[128];
					//SN
                    GetDlgItemText(hwndDlg, IDC_SN, str, sizeof(str));
					if(strlen(str)>0)
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, str);
					else
						DBDeleteContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN);
					//END SN

					//NK
					if(GetDlgItemText(hwndDlg, IDC_NK, str, sizeof(str)))
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_NK, str);
					else
					{
						GetDlgItemText(hwndDlg, IDC_SN, str, sizeof(str));
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_NK, str);
					}
					//END NK

					//PW
                    GetDlgItemText(hwndDlg, IDC_PW, str, sizeof(str));
					if(strlen(str)>0)
					{
						CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), (LPARAM) str);
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, str);
					}
					else
						DBDeleteContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW);
					//END PW

					//HN
					GetDlgItemText(hwndDlg, IDC_HN, str, sizeof(str));
					if(strlen(str)>0)
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, str);
					else
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HN, AIM_DEFAULT_SERVER);
					//END HN

					//GP
					unsigned long timeout=GetDlgItemInt(hwndDlg, IDC_GP,0,0);
					if(timeout>0xffff||timeout<15)
						 DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP,DEFAULT_GRACE_PERIOD);
					else
						DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP,(WORD)timeout);
					//END GP

					//Delivery Confirmation
					if (IsDlgButtonChecked(hwndDlg, IDC_DC))
                        DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DC, 1);
					else
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DC, 0);
					//End Delivery Confirmation

					//Force Proxy Transfer
					if (IsDlgButtonChecked(hwndDlg, IDC_FP))
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FP, 1);
					else
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FP, 0);
					//End Force Proxy Transfer

					//Disable Account Type Icons
					if (IsDlgButtonChecked(hwndDlg, IDC_AT))
					{
						int acc_disabled=DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_AT, 0);
						if(!acc_disabled)
							remove_AT_icons();
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_AT, 1);
					}
					else
					{
						int acc_disabled=DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_AT, 0);
						if(acc_disabled)
							add_AT_icons();
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_AT, 0);
					}
					//END

					//Disable Extra Status Icons
					if (IsDlgButtonChecked(hwndDlg, IDC_ES))
					{
						int es_disabled=DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_ES, 0);
						if(!es_disabled)
							remove_ES_icons();
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_ES, 1);
					}
					else
					{
						int es_disabled=DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_ES, 0);
						if(es_disabled)
							add_ES_icons();
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_ES, 0);
					}
					//End

					//Fake Hiptop
					if (IsDlgButtonChecked(hwndDlg, IDC_HF))
					{
						int hf=DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HF, 0);
						if(!hf)
							ShowWindow(GetDlgItem(hwndDlg, IDC_MASQ), SW_SHOW);
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HF, 1);
					}
					else
					{
						int hf=DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HF, 0);
						if(hf)
							ShowWindow(GetDlgItem(hwndDlg, IDC_MASQ), SW_SHOW);
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HF, 0);
					}
					//End

					//Default Group
					if(GetDlgItemText(hwndDlg, IDC_DG, str, sizeof(str)))
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DG, str);
					else
						DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DG, AIM_DEFAULT_GROUP);
					//End

					//Keep alive timer
					unsigned long timer=GetDlgItemInt(hwndDlg, IDC_KA,0,0);
					if(timer>0xffff||timer<15)
						 DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA,DEFAULT_KEEPALIVE_TIMER);
					else
						DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA,(WORD)timer);
					//End
                }
            }
            break;
        }
    }
    return FALSE;
}
