#include "commonheaders.h"
#pragma hdrstop

extern      HINSTANCE g_hInst;
extern		MYGLOBALS myGlobals;

static int EnableDisableMenuCommand(WPARAM wParam,LPARAM lParam)
	{
	Disabled = !(Disabled);

	if (PopupService) {

		CLISTMENUITEM mi = { 0 };

		mi.cbSize = sizeof(mi);
		mi.flags = CMIM_ICON | CMIM_NAME;

		if (!Disabled) {
			mi.pszName = LPGEN("Disable &typing notification");
			mi.hIcon = LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_ENABLED));
			}
		else {
			mi.pszName = LPGEN("Enable &typing notification");
			mi.hIcon = LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_DISABLED));
			}

		CallService(MS_CLIST_MODIFYMENUITEM,(WPARAM)hDisableMenu,(LPARAM)&mi);
		}

	return 0;
	}

static int CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
	switch(message) {
				case WM_COMMAND:
					if (HIWORD(wParam) == STN_CLICKED) {
						HANDLE hContact = PUGetContact(hWnd);
#if defined(_UNICODE)
						CallService(MS_MSG_SENDMESSAGE "W",(WPARAM)hContact,0);
#else
						CallService(MS_MSG_SENDMESSAGE,(WPARAM)hContact,0 );
#endif
						PUDeletePopUp(hWnd);
						return 1;
						}
					break;

				case WM_CONTEXTMENU:
					PUDeletePopUp(hWnd);
					return 1;

				case UM_INITPOPUP:{
					HANDLE hContact = PUGetContact(hWnd);
					WindowList_Add(hPopUpsList,hWnd,hContact);
					}return 1;
					
				case UM_FREEPLUGINDATA:
					WindowList_Remove(hPopUpsList,hWnd);
					return 1;
				default:
					break;
		}
	return DefWindowProc(hWnd, message, wParam, lParam);
	}


int TN_TypingMessage(WPARAM wParam, LPARAM lParam)
	{
	POPUPDATAT ppd = {0};
	TCHAR *szContactName = NULL;
	HWND hPopUpWnd = NULL;
	int notyping;

	// hidden & ignored contacts check
	if (DBGetContactSettingByte((HANDLE)wParam, "CList", "Hidden", 0) ||
		(DBGetContactSettingDword((HANDLE)wParam, "Ignore","Mask1",0) & 1)) // 9 - online notification
		return 0;

	szContactName = (TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, wParam, GCMDF_TCHAR);

	if (PopupService && !Disabled){
		if (OnePopUp){
			hPopUpWnd=WindowList_Find(hPopUpsList,(HANDLE)wParam);
			while(hPopUpWnd){
				PUDeletePopUp(hPopUpWnd);
				hPopUpWnd=WindowList_Find(hPopUpsList,(HANDLE)wParam);
				}
			}

		switch(lParam){
				case PROTOTYPE_CONTACTTYPING_OFF:
					if (StopDisabled) return 0;
					lstrcpyn(ppd.lptzContactName, szContactName, MAX_CONTACTNAME);
					lstrcpyn(ppd.lptzText, szStop, MAX_SECONDLINE);
					ppd.hNotification = hntfStopped;
					notyping = 1;
					break;
				default:
					if (StartDisabled) return 0;
					lstrcpyn(ppd.lptzContactName, szContactName, MAX_CONTACTNAME);
					lstrcpyn(ppd.lptzText, szStart, MAX_SECONDLINE);
					ppd.hNotification = hntfStarted;
					notyping = 0;
					break;
			}

		switch (ColorMode){
				case COLOR_OWN:
					ppd.colorBack = colorPicker[2*notyping].color;
					ppd.colorText = colorPicker[2*notyping+1].color;
					break;
				case COLOR_WINDOWS:
					ppd.colorBack = GetSysColor(COLOR_BTNFACE);
					ppd.colorText = GetSysColor(COLOR_WINDOWTEXT);
					break;
				case COLOR_POPUP:
				default:
					ppd.colorBack = ppd.colorText = 0;
					break;
			}

		if (notyping)
			switch (TimeoutMode2){
				case TIMEOUT_CUSTOM:
					ppd.iSeconds = Timeout2;
					break;
				case TIMEOUT_PERMANENT:
					ppd.iSeconds = -1;
					break;
				case TIMEOUT_POPUP:
				default:
					ppd.iSeconds = 0;
					break;
			}
		else
			switch (TimeoutMode){
				case TIMEOUT_CUSTOM:
					ppd.iSeconds = Timeout;
					break;
				case TIMEOUT_PROTO:
					ppd.iSeconds = (DWORD)lParam;
					break;
				case TIMEOUT_PERMANENT:
					ppd.iSeconds = -1;
					break;
				case TIMEOUT_POPUP:
				default:
					ppd.iSeconds = 0;
									break;
			}

		ppd.lchIcon = notyping?myGlobals.g_buttonBarIcons[13]:myGlobals.g_buttonBarIcons[5];
		ppd.lchContact = (HANDLE)wParam;
		ppd.PluginWindowProc = (WNDPROC)PopupDlgProc;
		ppd.PluginData = NULL;

		ppd.cbSize = sizeof(ppd);

		CallService(MS_POPUP_ADDPOPUPT, (WPARAM)&ppd, APF_NEWDATA);
		}
	return 0;
	}

static BOOL CALLBACK DlgProcOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
	{
	WORD i;

	switch (msg){
				case WM_INITDIALOG:{
					TranslateDialogDefault(hwndDlg);

					if (!ServiceExists(MS_POPUP_ADDPOPUPT)) SetDlgItemText(hwndDlg,IDC_INFO,TranslateT("Warning: Popup plugin not found!"));
					else
						if (!PopupService) SetDlgItemText(hwndDlg,IDC_INFO,TranslateT("Warning: Current Popup plugin version is not supported! Update it!"));
// 
// 					if(ServiceExists(MS_POPUP_REGISTERNOTIFICATION)){
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_PREVIEW), 0);
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_PERMANENT), 0);
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_POPUP), 0);
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_PROTO), 0);
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_CUSTOM), 0);
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE), 0);
// 
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_PERMANENT2), 0);
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_POPUP2), 0);
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_CUSTOM2), 0);
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE2), 0);
// 
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_USEWINCOLORS), 0);
// 						EnableWindow(GetDlgItem(hwndDlg, IDC_USEPOPUPCOLORS), 0);
// 						
// 						for(i=0;i<sizeof(colorPicker)/sizeof(colorPicker[0]);i++)
// 							EnableWindow(GetDlgItem(hwndDlg,colorPicker[i].res), 0);
// 
// 						}
// 					
// 					else{
						if (ColorMode == COLOR_WINDOWS) {
							CheckDlgButton(hwndDlg, IDC_USEWINCOLORS, BST_CHECKED);
							EnableWindow(GetDlgItem(hwndDlg, IDC_USEPOPUPCOLORS), FALSE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_USEWINCOLORS), TRUE);
							CheckDlgButton(hwndDlg, IDC_USEPOPUPCOLORS, BST_UNCHECKED);
							}
						else if (ColorMode == COLOR_POPUP) {
							CheckDlgButton(hwndDlg, IDC_USEWINCOLORS, BST_UNCHECKED);
							EnableWindow(GetDlgItem(hwndDlg, IDC_USEWINCOLORS), FALSE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_USEPOPUPCOLORS), TRUE);
							CheckDlgButton(hwndDlg, IDC_USEPOPUPCOLORS, BST_CHECKED);
							}

						for(i=0;i<sizeof(colorPicker)/sizeof(colorPicker[0]);i++) {
							SendDlgItemMessage(hwndDlg,colorPicker[i].res,CPM_SETCOLOUR,0,colorPicker[i].color);
							EnableWindow(GetDlgItem(hwndDlg,colorPicker[i].res), (ColorMode == COLOR_OWN));
							}
						//}


					CheckDlgButton(hwndDlg, IDC_TIMEOUT_PERMANENT, (TimeoutMode == TIMEOUT_PERMANENT)?BST_CHECKED:BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_TIMEOUT_POPUP, (TimeoutMode == TIMEOUT_POPUP)?BST_CHECKED:BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_TIMEOUT_PROTO, (TimeoutMode == TIMEOUT_PROTO)?BST_CHECKED:BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_TIMEOUT_CUSTOM, (TimeoutMode == TIMEOUT_CUSTOM)?BST_CHECKED:BST_UNCHECKED);
					SetDlgItemInt(hwndDlg, IDC_TIMEOUT_VALUE, Timeout, 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE), TimeoutMode == TIMEOUT_CUSTOM);

					CheckDlgButton(hwndDlg, IDC_TIMEOUT_PERMANENT2, (TimeoutMode2 == TIMEOUT_PERMANENT)?BST_CHECKED:BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_TIMEOUT_POPUP2, (TimeoutMode2 == TIMEOUT_POPUP)?BST_CHECKED:BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_TIMEOUT_CUSTOM2, (TimeoutMode2 == TIMEOUT_CUSTOM)?BST_CHECKED:BST_UNCHECKED);
					SetDlgItemInt(hwndDlg, IDC_TIMEOUT_VALUE2, Timeout2, 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE2), TimeoutMode2 == TIMEOUT_CUSTOM);

					CheckDlgButton(hwndDlg, IDC_START, (StartDisabled)?BST_UNCHECKED:BST_CHECKED);
					CheckDlgButton(hwndDlg, IDC_STOP, (StopDisabled)?BST_UNCHECKED:BST_CHECKED);

					CheckDlgButton(hwndDlg, IDC_ONEPOPUP, (OnePopUp)?BST_CHECKED:BST_UNCHECKED);
					CheckDlgButton(hwndDlg, IDC_SHOWMENU, (ShowMenu)?BST_CHECKED:BST_UNCHECKED);

					EnableWindow(GetDlgItem(hwndDlg, IDC_ONEPOPUP), PopupService);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWMENU), PopupService);
					EnableWindow(GetDlgItem(hwndDlg, IDC_PREVIEW), PopupService/*&&!ServiceExists(MS_POPUP_REGISTERNOTIFICATION)*/);

					newTimeout = Timeout;
					newTimeoutMode = TimeoutMode;
					newTimeout2 = Timeout2;
					newTimeoutMode2 = TimeoutMode2;
					newColorMode = ColorMode;
					}break;

				case WM_COMMAND: {
					WORD idCtrl = LOWORD(wParam), wNotifyCode = HIWORD(wParam);

					if (wNotifyCode == CPN_COLOURCHANGED) {
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
						return TRUE;
						}

					switch(idCtrl) {
				case IDC_USEWINCOLORS: {
					BOOL bEnableOthers = FALSE;

					if (wNotifyCode != BN_CLICKED) break;

					if (IsDlgButtonChecked(hwndDlg, IDC_USEWINCOLORS)) {
						newColorMode = COLOR_WINDOWS;
						bEnableOthers = FALSE;
						}
					else {
						newColorMode = COLOR_OWN;
						bEnableOthers = TRUE;
						}

					for(i=0;i<sizeof(colorPicker)/sizeof(colorPicker[0]);i++)
						EnableWindow(GetDlgItem(hwndDlg,colorPicker[i].res), bEnableOthers);

					EnableWindow(GetDlgItem(hwndDlg, IDC_USEPOPUPCOLORS), bEnableOthers);

					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
					}
				case IDC_USEPOPUPCOLORS: {
					BOOL bEnableOthers = FALSE;
					if (wNotifyCode != BN_CLICKED) break;

					if (IsDlgButtonChecked(hwndDlg, IDC_USEPOPUPCOLORS)) {
						newColorMode = COLOR_POPUP;
						bEnableOthers = FALSE;
						}
					else {
						newColorMode = COLOR_OWN;
						bEnableOthers = TRUE;
						}

					for(i=0;i<sizeof(colorPicker)/sizeof(colorPicker[0]);i++)
						EnableWindow(GetDlgItem(hwndDlg,colorPicker[i].res), bEnableOthers);

					EnableWindow(GetDlgItem(hwndDlg, IDC_USEWINCOLORS), bEnableOthers);

					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
					}
				case IDC_ONEPOPUP:
				case IDC_CLIST:
				case IDC_DISABLED:
				case IDC_SHOWMENU:
				case IDC_START:
				case IDC_STOP:
				case IDC_WOCL:
					if (wNotifyCode == BN_CLICKED)
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
				case IDC_PREVIEW:
					{
					POPUPDATAT ppd = {0};
					char *szProto = NULL;
					int i, notyping;

					HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

					if (!PopupService) break;

					while (hContact){
						szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
						if (szProto != NULL) break;
						hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
						}

					for (i=0; i<2; i++) {

						switch(i) {
							case PROTOTYPE_CONTACTTYPING_OFF:
								lstrcpy(ppd.lptzContactName, _T("Contact"));
								lstrcpyn(ppd.lptzText, szStop, MAX_SECONDLINE);
								notyping = 1;
								break;
							default:
								lstrcpy(ppd.lptzContactName, _T("Contact"));
								lstrcpyn(ppd.lptzText, szStart, MAX_SECONDLINE);
								notyping = 0;
								break;
							}

						switch (newColorMode) {
								case COLOR_OWN:
									ppd.colorText = SendDlgItemMessage(hwndDlg,colorPicker[2*notyping+1].res,CPM_GETCOLOUR,0,0);
									ppd.colorBack = SendDlgItemMessage(hwndDlg,colorPicker[2*notyping].res,CPM_GETCOLOUR,0,0);
									break;
								case COLOR_WINDOWS:
									ppd.colorBack = GetSysColor(COLOR_BTNFACE);
									ppd.colorText = GetSysColor(COLOR_WINDOWTEXT);
									break;
								case COLOR_POPUP:
								default:
									ppd.colorBack = ppd.colorText = 0;
									break;
							}

						if (notyping)
							switch (newTimeoutMode2) {
								case TIMEOUT_CUSTOM:
									ppd.iSeconds = newTimeout2;
									break;
								case TIMEOUT_PERMANENT:
									ppd.iSeconds = -1;
									break;
								case TIMEOUT_POPUP:
								default:
									ppd.iSeconds = 0;
									break;
							}
						else
							switch (newTimeoutMode) {
									case TIMEOUT_CUSTOM:
										ppd.iSeconds = newTimeout;
										break;
									case TIMEOUT_PROTO:
										ppd.iSeconds = 10;
										break;
									case TIMEOUT_PERMANENT:
										ppd.iSeconds = -1;
										break;
									case TIMEOUT_POPUP:
									default:
										ppd.iSeconds = 0;
										break;
							}


						ppd.lchIcon = notyping?myGlobals.g_buttonBarIcons[13]:myGlobals.g_buttonBarIcons[5];
						ppd.lchContact = (HANDLE)wParam;
						ppd.PluginWindowProc = NULL;
						ppd.PluginData = NULL;

						CallService(MS_POPUP_ADDPOPUPT, (WPARAM)&ppd, 0);

						}
					break;
					}
				case IDC_TIMEOUT_POPUP2:
					if (wNotifyCode != BN_CLICKED) break;
					newTimeoutMode2 = TIMEOUT_POPUP;
					EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE2), 0);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
				case IDC_TIMEOUT_CUSTOM2:
					if (wNotifyCode != BN_CLICKED) break;
					newTimeoutMode2 = TIMEOUT_CUSTOM;
					EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE2), 1);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
				case IDC_TIMEOUT_POPUP:
					if (wNotifyCode != BN_CLICKED) break;
					newTimeoutMode = TIMEOUT_POPUP;
					EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE), 0);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
				case IDC_TIMEOUT_PERMANENT:
					if (wNotifyCode != BN_CLICKED) break;
					newTimeoutMode = TIMEOUT_PERMANENT;
					EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE), 0);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
				case IDC_TIMEOUT_PERMANENT2:
					if (wNotifyCode != BN_CLICKED) break;
					newTimeoutMode2 = TIMEOUT_PERMANENT;
					EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE2), 0);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
				case IDC_TIMEOUT_CUSTOM:
					if (wNotifyCode != BN_CLICKED) break;
					newTimeoutMode = TIMEOUT_CUSTOM;
					EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE), 1);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
				case IDC_TIMEOUT_PROTO:
					if (wNotifyCode != BN_CLICKED) break;
					newTimeoutMode = TIMEOUT_PROTO;
					EnableWindow(GetDlgItem(hwndDlg, IDC_TIMEOUT_VALUE), 0);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
				case IDC_TIMEOUT_VALUE:
				case IDC_TIMEOUT_VALUE2: {
					int newValue = GetDlgItemInt(hwndDlg, idCtrl, NULL, 0);

					if (wNotifyCode == EN_KILLFOCUS) {
						int oldValue;

						if (idCtrl == IDC_TIMEOUT_VALUE)
							oldValue = newTimeout;
						else
							oldValue = newTimeout2;

						if (newValue != oldValue)
							SetDlgItemInt(hwndDlg, idCtrl, oldValue, 0);
						return TRUE;
						}
					if (wNotifyCode != EN_CHANGE || (HWND)lParam != GetFocus())
						return TRUE;

					if (newValue > TIMEOUT_MAXVALUE)
						newValue = TIMEOUT_MAXVALUE;
					else if (newValue < TIMEOUT_MINVALUE)
						newValue = TIMEOUT_MINVALUE;

					if (idCtrl == IDC_TIMEOUT_VALUE)
						newTimeout = newValue;
					else
						newTimeout2 = newValue;

					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					break;
					}

						}
					break;
					}
				case WM_NOTIFY: {
					switch(((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code) {
				case PSN_APPLY:
					for(i=0;i<sizeof(colorPicker)/sizeof(colorPicker[0]);i++)
						{
						colorPicker[i].color=SendDlgItemMessage(hwndDlg,colorPicker[i].res,CPM_GETCOLOUR,0,0);
						DBWriteContactSettingDword(NULL,Module,colorPicker[i].desc,colorPicker[i].color);
						}

					Timeout = newTimeout;
					TimeoutMode = newTimeoutMode;
					Timeout2 = newTimeout2;
					TimeoutMode2 = newTimeoutMode2;
					ColorMode = newColorMode;

					if (Disabled != IsDlgButtonChecked(hwndDlg,IDC_DISABLED))
						EnableDisableMenuCommand(0,0);

					StartDisabled = IsDlgButtonChecked(hwndDlg,IDC_START)?0:2;
					StopDisabled = IsDlgButtonChecked(hwndDlg,IDC_STOP)?0:4;
					OnePopUp = IsDlgButtonChecked(hwndDlg,IDC_ONEPOPUP);
					ShowMenu = IsDlgButtonChecked(hwndDlg,IDC_SHOWMENU);

					DBWriteContactSettingByte(NULL, Module, SET_ONEPOPUP, OnePopUp);
					DBWriteContactSettingByte(NULL, Module, SET_SHOWDISABLEMENU,ShowMenu);
					DBWriteContactSettingByte(NULL, Module, SET_DISABLED,(BYTE) (StartDisabled|StopDisabled));
					DBWriteContactSettingByte(NULL, Module, SET_COLOR_MODE, ColorMode);
					DBWriteContactSettingByte(NULL, Module, SET_TIMEOUT_MODE, TimeoutMode);
					DBWriteContactSettingByte(NULL, Module, SET_TIMEOUT, (BYTE) Timeout);
					DBWriteContactSettingByte(NULL, Module, SET_TIMEOUT_MODE2, TimeoutMode2);
					DBWriteContactSettingByte(NULL, Module, SET_TIMEOUT2,(BYTE)  Timeout2);

					return TRUE;
						}
					break;
						}
					break;
					}
		}
	return FALSE;
	}

int TN_OptionsInitialize(WPARAM wParam, LPARAM lParam)
{	

	OPTIONSDIALOGPAGE odp = { 0 };

	if(myGlobals.g_PopupAvail) {
		odp.cbSize = sizeof(odp);
		odp.position = 100000000;
		odp.hInstance = g_hInst;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_TYPINGNOTIFYPOPUP);
		odp.pszTitle = LPGEN("Typing Notify");
		odp.pszGroup = LPGEN("PopUps");
		odp.groupPosition = 910000000;
		odp.flags = ODPF_BOLDGROUPS;
		odp.pfnDlgProc = DlgProcOpts;
		CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	}

	return 0;
}

int TN_ModuleInit()
	{
	WORD i;

	PopupService=(myGlobals.g_PopupWAvail||myGlobals.g_PopupAvail);

	hPopUpsList = (HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST,0,0);

	OnePopUp = DBGetContactSettingByte(NULL,Module,SET_ONEPOPUP,DEF_ONEPOPUP);
	ShowMenu = DBGetContactSettingByte(NULL,Module,SET_SHOWDISABLEMENU,DEF_SHOWDISABLEMENU);

	i = DBGetContactSettingByte(NULL,Module,SET_DISABLED,DEF_DISABLED);
	Disabled = i & 1;
	StartDisabled = i & 2;
	StopDisabled = i & 4;

	ColorMode = DBGetContactSettingByte(NULL,Module,SET_COLOR_MODE,DEF_COLOR_MODE);
	TimeoutMode = DBGetContactSettingByte(NULL,Module,SET_TIMEOUT_MODE,DEF_TIMEOUT_MODE);
	Timeout = DBGetContactSettingByte(NULL,Module,SET_TIMEOUT,DEF_TIMEOUT);
	TimeoutMode2 = DBGetContactSettingByte(NULL,Module,SET_TIMEOUT_MODE2,DEF_TIMEOUT_MODE2);
	Timeout2 = DBGetContactSettingByte(NULL,Module,SET_TIMEOUT2,DEF_TIMEOUT2);

	if (!(DBGetContactSettingDword(NULL,Module,colorPicker[0].desc,1) &&
		!DBGetContactSettingDword(NULL,Module,colorPicker[0].desc,0)))
		for(i=0;i<sizeof(colorPicker)/sizeof(colorPicker[0]);i++)
			colorPicker[i].color = DBGetContactSettingDword(NULL,Module,colorPicker[i].desc,0);

	mir_sntprintf(szStart,sizeof(szStart),TranslateT("...is typing a message."));
	mir_sntprintf(szStop,sizeof(szStop),TranslateT("...has stopped typing."));

	if (PopupService && ShowMenu)
		{
		CLISTMENUITEM mi = { 0 };
		CreateServiceFunction("TypingNotify/EnableDisableMenuCommand",EnableDisableMenuCommand);

		mi.cbSize = sizeof(mi);
		mi.flags=0;

		if (!Disabled) {
			mi.pszName = LPGEN("Disable &typing notification");
			mi.hIcon = LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_ENABLED));
			}
		else {
			mi.pszName = LPGEN("Enable &typing notification");
			mi.hIcon = LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_DISABLED));
			}
		mi.pszService="TypingNotify/EnableDisableMenuCommand";
		mi.pszPopupName = LPGEN("PopUps");
		hDisableMenu = (HANDLE)CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
		}

	SkinAddNewSound("TNStart", Translate("Typing Notify: is typing"),"IsTyping.wav");
	SkinAddNewSound("TNStop", Translate("Typing Notify: stopped typing"),"StoppedTyping.wav");

// 	if (ServiceExists(MS_POPUP_REGISTERNOTIFICATION))
// 		{
// 		POPUPNOTIFICATION pnf = {0};
// 
// 		pnf.cbSize = sizeof(pnf);
// 		pnf.dwFlags = PNF_CONTACT;
// 		lstrcpyA(pnf.lpzGroup, "Events");
// 
// 		lstrcpyA(pnf.lpzName, "Typing...");
// 		pnf.lchIcon = myGlobals.g_buttonBarIcons[5];
// 		pnf.colorBack = colorPicker[0].color;
// 		pnf.colorText = colorPicker[1].color;
// 		pnf.iSeconds = Timeout;
// 		hntfStarted = PURegisterNotification(&pnf);
// 
// 		lstrcpyA(pnf.lpzName, "Typing stopped");
// 		pnf.lchIcon = myGlobals.g_buttonBarIcons[13];
// 		pnf.colorBack = colorPicker[2].color;
// 		pnf.colorText = colorPicker[3].color;
// 		pnf.iSeconds = Timeout2;
// 
// 		hntfStopped = PURegisterNotification(&pnf);
// 		} 
	return 0;
	}

int TN_ModuleDeInit()
	{
	DBWriteContactSettingByte(NULL, Module, SET_DISABLED, (BYTE)(Disabled|StartDisabled|StopDisabled));
	return 0;
	}
