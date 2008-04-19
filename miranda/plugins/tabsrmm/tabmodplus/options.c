#include "tabmodplus.h"

extern HINSTANCE hinstance;

static BOOL bOptionsInit;

HWND g_opHdlg;

BOOL CALLBACK OptionsProc(HWND hdlg,UINT msg,WPARAM wparam,LPARAM lparam)
	{	
	switch(msg)
		{
		case WM_INITDIALOG:
			{
			g_opHdlg=hdlg;
			bOptionsInit=TRUE;
			TranslateDialogDefault(hdlg); 

			g_bIMGtagButton=DBGetContactSettingByte(NULL,"tabmodplus","IMGtagButton",1);	
			g_bClientInStatusBar=DBGetContactSettingByte(NULL,"tabmodplus","ClientIconInStatusBar",1);
			
			CheckDlgButton(hdlg,IDC_IMGTAG,g_bIMGtagButton);
			CheckDlgButton(hdlg,IDC_CLIENTINSTATBAR,g_bClientInStatusBar);
			CheckDlgButton(hdlg,IDC_TYPINGSOUNDS,DBGetContactSettingByte(NULL,"Tab_SRMsg","soundontyping",1));
			CheckDlgButton(hdlg,IDC_ANIAVATAR,!DBGetContactSettingByte(NULL,"Tab_SRMsg","DisableAniAvatars",0));
			CheckDlgButton(hdlg,IDC_SCROLLFIX,DBGetContactSettingByte(NULL,"Tab_SRMsg","ScrollBarFix",0));
			CheckDlgButton(hdlg,IDC_AUTOCLOSEV2,DBGetContactSettingByte(NULL,"Tab_SRMsg","AutoClose_2",0));
			CheckDlgButton(hdlg,IDC_ICONWARNINGS,DBGetContactSettingByte(NULL,"Tab_SRMsg","IconpackWarning",1));

			bOptionsInit=FALSE;
			}break;

		case WM_NOTIFY:
			switch(((LPNMHDR)lparam)->code)	{
		case PSN_APPLY:
			{
			DBWriteContactSettingByte(NULL,"tabmodplus","IMGtagButton",(BYTE)((IsDlgButtonChecked(hdlg, IDC_IMGTAG))?(g_bIMGtagButton=1):(g_bIMGtagButton=0)));
			DBWriteContactSettingByte(NULL,"tabmodplus","ClientIconInStatusBar",(BYTE)((IsDlgButtonChecked(hdlg, IDC_CLIENTINSTATBAR))?(g_bClientInStatusBar=1):(g_bClientInStatusBar=0)));
			DBWriteContactSettingByte(NULL,"Tab_SRMsg","soundontyping",(BYTE)((IsDlgButtonChecked(hdlg, IDC_TYPINGSOUNDS))?1:0));
			DBWriteContactSettingByte(NULL,"Tab_SRMsg","DisableAniAvatars",(BYTE)((IsDlgButtonChecked(hdlg, IDC_ANIAVATAR))?0:1));
			DBWriteContactSettingByte(NULL,"Tab_SRMsg","ScrollBarFix",(BYTE)((IsDlgButtonChecked(hdlg, IDC_SCROLLFIX))?1:0));
			DBWriteContactSettingByte(NULL,"Tab_SRMsg","AutoClose_2",(BYTE)((IsDlgButtonChecked(hdlg, IDC_AUTOCLOSEV2))?1:0));
			DBWriteContactSettingByte(NULL,"Tab_SRMsg","IconpackWarning",(BYTE)((IsDlgButtonChecked(hdlg, IDC_ICONWARNINGS))?1:0));
			return 1;
			} 
				}break;
		case WM_COMMAND:
			{
			if (HIWORD(wparam)==BN_CLICKED && GetFocus()==(HWND)lparam){
				SendMessage(GetParent(hdlg),PSM_CHANGED,0,0);
				if(!bOptionsInit) ShowWindow(GetDlgItem(hdlg,IDC_RESTART),SW_SHOW);}
			}break;

		case WM_CLOSE:
			EndDialog(hdlg,0);
			return 0;
		}
	return 0;
	}

int OptionsInit(WPARAM wparam,LPARAM lparam)
	{
	OPTIONSDIALOGPAGE odp={0};

	odp.cbSize=sizeof(odp);
	odp.position=950000000;
	odp.hInstance=hinstance;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPTIONS);
	odp.pszTitle=LPGEN("tabmodplus");
	odp.pfnDlgProc=OptionsProc;
	odp.pszGroup=LPGEN("Message Sessions");
	odp.flags=ODPF_BOLDGROUPS;

	CallService(MS_OPT_ADDPAGE,wparam,(LPARAM)&odp);
	return 0;
	}
