#include "windows.h"
int OptionsInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;
	odp.cbSize = sizeof(odp);
    odp.position = 1003000;
    odp.hInstance = conn.hInstance;
    odp.pszTemplate = MAKEINTRESOURCE(IDD_AIM);
    odp.pszGroup = Translate("Network");
    odp.pszTitle = Translate(AIM_PROTOCOL_NAME);
    odp.pfnDlgProc = options_dialog;
    odp.flags = ODPF_BOLDGROUPS;
	odp.nIDBottomSimpleControl=IDC_OPTIONS;
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
		odp.pszTitle = Translate(AIM_PROTOCOL_NAME);
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
                    char buf[MSG_LEN/2];//MAX
                    GetWindowText(GetDlgItem(hwndDlg, IDC_PROFILE), buf, sizeof(buf));
                    DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PR, buf);
                    if (conn.state==1)
					{
						char* msg=strldup(buf,strlen(buf));
						msg=strip_linebreaks(msg);
                        aim_set_profile(msg);//also see set caps for profile setting
						delete[] msg;
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
			unsigned short timeout=DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, DEFAULT_GRACE_PERIOD);
			SetDlgItemInt(hwndDlg, IDC_GP, timeout,0);
			unsigned short timer=DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA, DEFAULT_KEEPALIVE_TIMER);
			SetDlgItemInt(hwndDlg, IDC_KA, timer,0);
			unsigned long it=DBGetContactSettingDword(NULL, AIM_PROTOCOL_NAME, AIM_KEY_IIT, 0);
			unsigned long hours=it/60;
			unsigned long minutes=it%60;
			SetDlgItemInt(hwndDlg, IDC_IIH, hours,0);
			SetDlgItemInt(hwndDlg, IDC_IIM, minutes,0);
			CheckDlgButton(hwndDlg, IDC_DC, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DC, 0));//Message Delivery Confirmation
            CheckDlgButton(hwndDlg, IDC_FP, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FP, 0));//force proxy
			CheckDlgButton(hwndDlg, IDC_AT, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_AT, 0));//Account Type Icons
			CheckDlgButton(hwndDlg, IDC_ES, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_ES, 0));//Extended Status Type Icons
			CheckDlgButton(hwndDlg, IDC_HF, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_HF, 0));//Fake hiptopness
			CheckDlgButton(hwndDlg, IDC_DM, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DM, 0));//Disable Sending Mode Message
			CheckDlgButton(hwndDlg, IDC_FI, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FI, 0));//Format imcoming messages
			CheckDlgButton(hwndDlg, IDC_FO, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FO, 0));//Format outgoing messages
			CheckDlgButton(hwndDlg, IDC_WEBSUPPORT, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_AL, 0));
			CheckDlgButton(hwndDlg, IDC_II, DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_II, 0));//Instant Idle
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

					//Keep alive timer
					unsigned long timer=GetDlgItemInt(hwndDlg, IDC_KA,0,0);
					if(timer>0xffff||timer<15)
						 DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA,DEFAULT_KEEPALIVE_TIMER);
					else
						DBWriteContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA,(WORD)timer);
					//End
					
					//Disable Mode Message Sending
					if (IsDlgButtonChecked(hwndDlg, IDC_DM))
                        DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DM, 1);
					else
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DM, 0);
					//End Disable Mode Message Sending

					//Format Incoming Messages
					if (IsDlgButtonChecked(hwndDlg, IDC_FI))
                        DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FI, 1);
					else
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FI, 0);
					//End Format Incoming Messages
                
					//Format Outgoing Messages
					if (IsDlgButtonChecked(hwndDlg, IDC_FO))
                        DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FO, 1);
					else
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FO, 0);
					//End Format Outgoing Messages

					if (!IsDlgButtonChecked(hwndDlg, IDC_WEBSUPPORT) && DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_AL, 0)) {
                        aim_links_unregister();
                    }
                    if (IsDlgButtonChecked(hwndDlg, IDC_WEBSUPPORT))
					{
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_AL, 1);
                        aim_links_init();
					}
					else
                        aim_links_destroy();
					//Instant Idle
					unsigned long hours=GetDlgItemInt(hwndDlg, IDC_IIH,0,0);
					unsigned short minutes=GetDlgItemInt(hwndDlg, IDC_IIM,0,0);
					if(minutes>59)
						minutes=59;
					if (IsDlgButtonChecked(hwndDlg, IDC_II))
					{
						unsigned long it = DBGetContactSettingDword(NULL, AIM_PROTOCOL_NAME, AIM_KEY_IIT, 0);
						int ii=DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_II, 0);
						if(it!=hours*60+minutes||!ii)
							if (conn.state==1)
								aim_set_idle(hours * 60 * 60 + minutes * 60);
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_II, 1);
					}
					else
					{
						int ii=DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_II, 0);
						if(ii)
							if (conn.state==1)
								aim_set_idle(0);
						DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_II, 0);
					}
					DBWriteContactSettingDword(NULL, AIM_PROTOCOL_NAME, AIM_KEY_IIT, hours*60+minutes);
					//End
				}
            }
            break;
        }
    }
    return FALSE;
}
BOOL CALLBACK first_run_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//The code to implement and run the the first run dialog was contributed by RiPOFF
    switch (msg)
	{

	case WM_INITDIALOG:
        {

            DBVARIANT dbv;
            TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(conn.hInstance, MAKEINTRESOURCE(IDI_AIM)));
            if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
			{
                SetDlgItemText(hwndDlg, IDC_SN, dbv.pszVal);
                DBFreeVariant(&dbv);
            }

            if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, &dbv))
			{
                CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
                SetDlgItemText(hwndDlg, IDC_PW, dbv.pszVal);
                DBFreeVariant(&dbv);
            }

        }
		break;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;

	case WM_COMMAND:
		{

			switch (LOWORD(wParam))
			{
			case IDOK:
				{

					char str[128];
					GetDlgItemText(hwndDlg, IDC_SN, str, sizeof(str));
					DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, str);
					GetDlgItemText(hwndDlg, IDC_PW, str, sizeof(str));
					CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), (LPARAM) str);
					DBWriteContactSettingString(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, str);

				}
				// fall through

			case IDCANCEL:
                {
					// Mark first run as completed
					DBWriteContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FR, 1);
                    EndDialog(hwndDlg, IDCANCEL);
                }
				break;

            }
        }
		break;

    }

    return FALSE;

}
