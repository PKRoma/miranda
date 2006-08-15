/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "commonheaders.h"
#include "profilemanager.h"
#include <sys/stat.h>

typedef BOOL (__cdecl *ENUMPROFILECALLBACK) (char * fullpath, char * profile, LPARAM lParam);

struct DetailsPageInit {
	int pageCount;
	OPTIONSDIALOGPAGE *odp;
};

struct DetailsPageData {
	DLGTEMPLATE *pTemplate;
	HINSTANCE hInst;
	DLGPROC dlgProc;
	HWND hwnd;
	int changed;
};

struct DlgProfData {
	PROPSHEETHEADER * psh;
	HWND hwndOK;			// handle to OK button
	PROFILEMANAGERDATA * pd;
};

struct DetailsData {
	HINSTANCE hInstIcmp;
	HFONT hBoldFont;
	int pageCount;
	int currentPage;
	struct DetailsPageData *opd;
	RECT rcDisplay;
	struct DlgProfData * prof;
};

extern char mirandabootini[MAX_PATH]; // bad bad bad bad!
static char szDefaultMirandaProfile[MAX_PATH];

static void ThemeDialogBackground(HWND hwnd) {
	if (IsWinVerXPPlus()) {
		static HMODULE hThemeAPI = NULL;
		if (!hThemeAPI) hThemeAPI = GetModuleHandleA("uxtheme");
		if (hThemeAPI) {
			HRESULT (STDAPICALLTYPE *MyEnableThemeDialogTexture)(HWND,DWORD) = (HRESULT (STDAPICALLTYPE*)(HWND,DWORD))GetProcAddress(hThemeAPI,"EnableThemeDialogTexture");
			if (MyEnableThemeDialogTexture)
				MyEnableThemeDialogTexture(hwnd,0x00000002|0x00000004); //0x00000002|0x00000004=ETDT_ENABLETAB
		}
	}
}

static int findProfiles(char * szProfileDir, ENUMPROFILECALLBACK callback, LPARAM lParam)
{
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAA ffd;
	char searchspec[MAX_PATH];
	mir_snprintf(searchspec, SIZEOF(searchspec), "%s\\*.dat", szProfileDir);
	hFind = FindFirstFileA(searchspec, &ffd);
	if ( hFind != INVALID_HANDLE_VALUE ) {
		do { 			
			if ( !(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && isValidProfileName(ffd.cFileName) ) 
			{
				char buf[MAX_PATH];
				mir_snprintf(buf,SIZEOF(buf),"%s\\%s",szProfileDir, ffd.cFileName);
				if ( !callback(buf, ffd.cFileName, lParam) ) break;
			}
		} while ( FindNextFileA(hFind, &ffd) );
		FindClose(hFind);
		return 1;
	}
	return 0;
}

static LRESULT CALLBACK ProfileNameValidate(HWND edit, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg==WM_CHAR ) {		
		if ( strchr(".?/\\#' ",(char)wParam&0xFF) != 0 ) return 0;
		PostMessage(GetParent(edit),WM_USER+2,0,0);
	}
	return CallWindowProc((WNDPROC)GetWindowLong(edit,GWL_USERDATA),edit,msg,wParam,lParam);
}

static int FindDbProviders(char * pluginname, DATABASELINK * dblink, LPARAM lParam)
{
	HWND hwndDlg = (HWND)lParam;
	HWND hwndCombo = GetDlgItem(hwndDlg, IDC_PROFILEDRIVERS);
	char szName[64];

	if ( dblink->getFriendlyName(szName,SIZEOF(szName),1) == 0 ) {
		// add to combo box
		LRESULT index = SendMessageA(hwndCombo, CB_ADDSTRING, 0, (LPARAM)Translate(szName));
		SendMessage(hwndCombo, CB_SETITEMDATA, index, (LPARAM)dblink);
	}	
	return DBPE_CONT;
}

// returns 1 if autocreation of the profile is setup, profile has to be at least MAX_PATH!
static int checkAutoCreateProfile(char * profile)
{
	char ac[MAX_PATH];
	char env_profile[MAX_PATH];
	GetPrivateProfileStringA("Database", "AutoCreate", "no", ac, SIZEOF(ac), mirandabootini);
	if ( lstrcmpiA(ac,"yes") != 0 ) return 0;
	GetPrivateProfileStringA("Database", "DefaultProfile", "", ac, SIZEOF(ac), mirandabootini);
	ExpandEnvironmentStringsA(ac, env_profile, SIZEOF(env_profile));	
	if ( profile != NULL ) strcpy(profile, env_profile);
	return lstrlenA(env_profile) > 0;
}

static BOOL CALLBACK DlgProfileNew(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct DlgProfData * dat = (struct DlgProfData *)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (msg) {
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			// lParam = (struct DlgProfData *)
			SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
			dat=(struct DlgProfData *)lParam;
			{
				// fill in the db plugins present
				PLUGIN_DB_ENUM dbe;
				dbe.cbSize=sizeof(dbe);
				dbe.pfnEnumCallback=(int(*)(char*,void*,LPARAM))FindDbProviders;
				dbe.lParam=(LPARAM)hwndDlg;
				if ( CallService(MS_PLUGINS_ENUMDBPLUGINS,0,(LPARAM)&dbe) == (-1) ) {
					// no plugins?!
					EnableWindow(GetDlgItem(hwndDlg,IDC_PROFILEDRIVERS),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_PROFILENAME),FALSE);
					ShowWindow(GetDlgItem(hwndDlg,IDC_NODBDRIVERS),TRUE);
				} //if
				// default item
				SendDlgItemMessage(hwndDlg, IDC_PROFILEDRIVERS, CB_SETCURSEL, 0, 0);
			}
			// subclass the profile name box
			{
				HWND hwndProfile=GetDlgItem(hwndDlg, IDC_PROFILENAME);
				WNDPROC proc = (WNDPROC)GetWindowLong(hwndProfile, GWL_WNDPROC);
				SetWindowLong(hwndProfile,GWL_USERDATA,(LONG)proc);
				SetWindowLong(hwndProfile,GWL_WNDPROC,(LONG)ProfileNameValidate);
			}
			// decide if there is a default profile name given in the INI and if it should be used
			{
				char profile[MAX_PATH];
				if ( checkAutoCreateProfile((char*)&profile) ) SetDlgItemTextA(hwndDlg, IDC_PROFILENAME, profile);
			}
			// focus on the textbox
			PostMessage(hwndDlg,WM_USER+10,0,0);
			return TRUE;
		}
		case WM_USER+10:
		{
			SetFocus(GetDlgItem(hwndDlg,IDC_PROFILENAME));
			break;
		}
		case WM_USER+2: // when input in the edit box changes
		{
			SendMessage(GetParent(hwndDlg),PSM_CHANGED,0,0);
			EnableWindow(dat->hwndOK, GetWindowTextLength(GetDlgItem(hwndDlg,IDC_PROFILENAME)) > 0 );
			break;
		}
		case WM_SHOWWINDOW:
		{
			if ( wParam ) { 
				SetWindowText( dat->hwndOK, TranslateT("&Create"));
				SendMessage(hwndDlg,WM_USER+2,0,0);
			}
			break;
		}
		case WM_NOTIFY:
		{
			NMHDR * hdr = (NMHDR *)lParam;
			if ( hdr && hdr->code == PSN_APPLY && dat && IsWindowVisible(hwndDlg) ) {
				char szName[MAX_PATH];						
				LRESULT curSel = SendDlgItemMessage(hwndDlg,IDC_PROFILEDRIVERS,CB_GETCURSEL,0,0);				
				if ( curSel == CB_ERR ) break; // should never happen				
				GetWindowTextA(GetDlgItem(hwndDlg,IDC_PROFILENAME),szName,SIZEOF(szName));
				if ( lstrlenA(szName) == 0 ) break;
				mir_snprintf(dat->pd->szProfile,MAX_PATH,"%s\\%s.dat",dat->pd->szProfileDir,szName);
				dat->pd->newProfile=1;
				dat->pd->dblink=(DATABASELINK *)SendDlgItemMessage(hwndDlg,IDC_PROFILEDRIVERS,CB_GETITEMDATA,(WPARAM)curSel,0);
				
				if ( makeDatabase(dat->pd->szProfile, dat->pd->dblink, hwndDlg) == 0 ) {
					SetWindowLong(hwndDlg,DWL_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
					return FALSE;
				}
			}
			break;
		}
	}
	return FALSE;
}

TCHAR* rtrim( TCHAR *string )
{
   TCHAR* p = string + _tcslen( string ) - 1;

   while ( p >= string )
   {  if ( *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' )
         break;

		*p-- = 0;
   }
   return string;
}

static int DetectDbProvider(char * pluginname, DATABASELINK * dblink, LPARAM lParam)
{
	char* fullPath = (char*)lParam;
	int error;

	if ( dblink->grokHeader( fullPath, &error ) == 0 ) {
		dblink->getFriendlyName( fullPath, MAX_PATH, 1 );
		//strncpy( fullPath, pluginname, MAX_PATH );
		return DBPE_HALT;
	}

	return DBPE_CONT;
}

BOOL EnumProfilesForList(char * fullpath, char * profile, LPARAM lParam)
{
	HWND hwndDlg = (HWND) lParam;
	HWND hwndList = GetDlgItem(hwndDlg, IDC_PROFILELIST);
	char sizeBuf[64];
	LVITEMA item;
	int iItem=0;	
	struct stat statbuf;
	int bFileExists = FALSE;
	char * p = strrchr(profile, '.');	
	strcpy(sizeBuf, "0 KB");
	if ( p != NULL ) *p=0;
	ZeroMemory(&item,sizeof(item));
	item.mask = LVIF_TEXT | LVIF_IMAGE;
	item.pszText = profile;
	item.iItem=0;
	item.iImage=0;
	{
		FILE * fp = fopen(fullpath, "r+");
		item.iImage = fp != NULL ? 0 : 1;
		if ( stat(fullpath, &statbuf) == 0) {
			mir_snprintf(sizeBuf,SIZEOF(sizeBuf),"%u KB", statbuf.st_size / 1024);
			bFileExists = TRUE;
		}
		if ( fp ) fclose(fp);
	}
	iItem=SendMessageA( hwndList, LVM_INSERTITEMA, 0, (LPARAM)&item);	
	if ( lstrcmpiA(szDefaultMirandaProfile, profile) == 0 ) 
		ListView_SetItemState(hwndList, iItem, LVIS_SELECTED, LVIS_SELECTED);

	item.iItem = iItem;
	item.iSubItem = 2;
	item.pszText = sizeBuf;
	SendMessageA( hwndList, LVM_SETITEMTEXTA, iItem, (LPARAM)&item );

	if ( bFileExists ) {
		PLUGIN_DB_ENUM dbe;
		char szPath[ MAX_PATH ];

		LVITEM item2;
		item2.mask = LVIF_TEXT;
		item2.iItem = iItem;

		dbe.cbSize=sizeof(dbe);
		dbe.pfnEnumCallback=(int(*)(char*,void*,LPARAM))DetectDbProvider;
		dbe.lParam=(LPARAM)szPath;
		strncpy( szPath, fullpath, sizeof(szPath));
		if (CallService(MS_PLUGINS_ENUMDBPLUGINS,0,(LPARAM)&dbe)==1) {
			HANDLE hFile;

			hFile=CreateFileA(fullpath,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
			if (hFile == INVALID_HANDLE_VALUE) {
				// file locked
				item.pszText = Translate("<In Use>");
			}
			else {
				CloseHandle(hFile);
				item.pszText = szPath;
			}
			item.iSubItem = 1;
			SendMessageA( hwndList, LVM_SETITEMTEXTA, iItem, (LPARAM)&item );
		}

		item2.iSubItem = 3;
		item2.pszText = rtrim( _tctime( &statbuf.st_ctime ));
		SendMessage( hwndList, LVM_SETITEMTEXT, iItem, (LPARAM)&item2 );

		item2.iSubItem = 4;
		item2.pszText = rtrim( _tctime( &statbuf.st_mtime ));
		SendMessage( hwndList, LVM_SETITEMTEXT, iItem, (LPARAM)&item2 );
	}
	return TRUE;
}

static BOOL CALLBACK DlgProfileSelect(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct DlgProfData * dat = (struct DlgProfData *)GetWindowLong(hwndDlg, GWL_USERDATA);
	switch (msg) {
		case WM_INITDIALOG:
		{
			HWND hwndList = GetDlgItem(hwndDlg, IDC_PROFILELIST);
			HIMAGELIST hImgList=0;
			LVCOLUMN col;

			TranslateDialogDefault(hwndDlg);
			
			dat = (struct DlgProfData *) lParam;
			SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)dat);

			// set columns
			col.mask = LVCF_TEXT | LVCF_WIDTH;
			col.pszText = TranslateT("Profile");
			col.cx=122;
			ListView_InsertColumn( hwndList, 0, &col );

			col.pszText = TranslateT("Driver");
			col.cx=100;
			ListView_InsertColumn( hwndList, 1, &col );

			col.pszText = TranslateT("Size");
			col.cx=60;
			ListView_InsertColumn( hwndList, 2, &col );

			col.pszText = TranslateT("Created");
			col.cx=145;
			ListView_InsertColumn( hwndList, 3, &col );

			col.pszText = TranslateT("Accessed");
			col.cx=145;
			ListView_InsertColumn( hwndList, 4, &col );

			// icons
			hImgList=ImageList_Create(16, 16, ILC_MASK | (IsWinVerXPPlus() ? ILC_COLOR32 : ILC_COLOR16), 1, 1);
			ImageList_AddIcon(hImgList, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USERDETAILS)) );
			ImageList_AddIcon(hImgList, LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DELETE)) );	
			// LV will destroy the image list
			ListView_SetImageList(hwndList, hImgList, LVSIL_SMALL);			
			// find all the profiles
			findProfiles(dat->pd->szProfileDir, EnumProfilesForList, (LPARAM)hwndDlg);
			PostMessage(hwndDlg,WM_USER+10,0,0);
			return TRUE;
		}
		case WM_USER+10:
		{
			HWND hwndList=GetDlgItem(hwndDlg,IDC_PROFILELIST);
			SetFocus(hwndList);		
			if ( lstrlenA(szDefaultMirandaProfile) == 0 || ListView_GetSelectedCount(GetDlgItem(hwndDlg,IDC_PROFILELIST)) == 0 ) 
				ListView_SetItemState(hwndList, 0, LVIS_SELECTED, LVIS_SELECTED);
			break;
		}
		case WM_SHOWWINDOW:
		{
			if ( wParam ) {
				SetWindowText(dat->hwndOK,TranslateT("&Run"));
				EnableWindow(dat->hwndOK, ListView_GetSelectedCount(GetDlgItem(hwndDlg,IDC_PROFILELIST))==1);
			}
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR hdr = (LPNMHDR) lParam;
			if ( hdr && hdr->code == PSN_INFOCHANGED) {
				break;
			}
			if ( hdr && hdr->idFrom == IDC_PROFILELIST ) {
				switch ( hdr->code ) {
					case LVN_ITEMCHANGED:
					{
						EnableWindow(dat->hwndOK, ListView_GetSelectedCount(hdr->hwndFrom)==1);
					}
					case NM_DBLCLK:
					{					
						HWND hwndList = GetDlgItem(hwndDlg, IDC_PROFILELIST);
						LVITEMA item;						
						char profile[MAX_PATH];
						
						if ( dat == NULL ) break;
						ZeroMemory(&item,sizeof(item));
						item.mask = LVIF_TEXT;
						item.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED | LVNI_ALL);
						item.pszText = profile;
						item.cchTextMax = SIZEOF(profile);						
						if ( SendMessageA(hwndList, LVM_GETITEMA, 0, (LPARAM)&item) && dat ) {
							mir_snprintf(dat->pd->szProfile, MAX_PATH, "%s\\%s.dat", dat->pd->szProfileDir, profile);
							if ( hdr->code == NM_DBLCLK ) EndDialog(GetParent(hwndDlg), 1);								
						}						
						return TRUE;
					}					
				}
			}
			break;
		}
	} //switch
	return FALSE;
}

static BOOL CALLBACK DlgProfileManager(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct DetailsData *dat;

	dat=(struct DetailsData*)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (msg)
	{
		case WM_INITDIALOG:
		{	
			struct DlgProfData * prof = (struct DlgProfData *)lParam;			
			PROPSHEETHEADER *psh = prof->psh;
			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USERDETAILS)));
			dat=(struct DetailsData*)mir_alloc(sizeof(struct DetailsData));
			dat->prof = prof;
			prof->hwndOK=GetDlgItem(hwndDlg,IDOK);
			EnableWindow(prof->hwndOK, FALSE);
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
			SetDlgItemTextA(hwndDlg,IDC_NAME,"Miranda IM Profile Manager");
			{	LOGFONT lf;
				HFONT hNormalFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_NAME,WM_GETFONT,0,0);
				GetObject(hNormalFont,sizeof(lf),&lf);
				lf.lfWeight=FW_BOLD;
				dat->hBoldFont=CreateFontIndirect(&lf);
				SendDlgItemMessage(hwndDlg,IDC_NAME,WM_SETFONT,(WPARAM)dat->hBoldFont,0);
			}
			{	OPTIONSDIALOGPAGE *odp;
				int i;
				TCITEM tci;

				dat->currentPage=0;
				dat->pageCount=psh->nPages;
				dat->opd=(struct DetailsPageData*)mir_alloc(sizeof(struct DetailsPageData)*dat->pageCount);
				odp=(OPTIONSDIALOGPAGE*)psh->ppsp;

				tci.mask=TCIF_TEXT;
				for(i=0;i<dat->pageCount;i++) {
					dat->opd[i].pTemplate=(DLGTEMPLATE *)LockResource(LoadResource(odp[i].hInstance,FindResourceA(odp[i].hInstance,odp[i].pszTemplate,MAKEINTRESOURCEA(5))));
					dat->opd[i].dlgProc=odp[i].pfnDlgProc;
					dat->opd[i].hInst=odp[i].hInstance;
					dat->opd[i].hwnd=NULL;
					dat->opd[i].changed=0;
					tci.pszText=(TCHAR*)odp[i].ptszTitle;
					if ( dat->prof->pd->noProfiles || checkAutoCreateProfile(NULL) ) dat->currentPage=1;
					TabCtrl_InsertItem( GetDlgItem(hwndDlg,IDC_TABS), i, &tci );
			}	}

			GetWindowRect(GetDlgItem(hwndDlg,IDC_TABS),&dat->rcDisplay);
			TabCtrl_AdjustRect(GetDlgItem(hwndDlg,IDC_TABS),FALSE,&dat->rcDisplay);
			{	
				POINT pt={0,0};
				ClientToScreen(hwndDlg,&pt);
				OffsetRect(&dat->rcDisplay,-pt.x,-pt.y);
			}

			TabCtrl_SetCurSel(GetDlgItem(hwndDlg,IDC_TABS),dat->currentPage);
			dat->opd[dat->currentPage].hwnd=CreateDialogIndirectParam(dat->opd[dat->currentPage].hInst,dat->opd[dat->currentPage].pTemplate,hwndDlg,dat->opd[dat->currentPage].dlgProc,(LPARAM)dat->prof);
			ThemeDialogBackground(dat->opd[dat->currentPage].hwnd);
			SetWindowPos(dat->opd[dat->currentPage].hwnd,HWND_TOP,dat->rcDisplay.left,dat->rcDisplay.top,0,0,SWP_NOSIZE);
			{	PSHNOTIFY pshn;
				pshn.hdr.code=PSN_INFOCHANGED;
				pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
				pshn.hdr.idFrom=0;
				pshn.lParam=(LPARAM)0;
				SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
			}
			ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);
			return TRUE;
		}
		case WM_CTLCOLORSTATIC:
			switch (GetDlgCtrlID((HWND)lParam)) {
			case IDC_WHITERECT:
			case IDC_LOGO:
			case IDC_NAME:
			case IDC_DESCRIPTION:
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
			break;
		case PSM_CHANGED:
			dat->opd[dat->currentPage].changed=1;
			return TRUE;
		case PSM_FORCECHANGED:
		{	PSHNOTIFY pshn;
			int i;

			pshn.hdr.code=PSN_INFOCHANGED;
			pshn.hdr.idFrom=0;
			pshn.lParam=(LPARAM)0;
			for(i=0;i<dat->pageCount;i++) {
				pshn.hdr.hwndFrom=dat->opd[i].hwnd;
				if(dat->opd[i].hwnd!=NULL)
					SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
			}
			break;
		}
		case WM_NOTIFY:
			switch(wParam) {
				case IDC_TABS:
					switch(((LPNMHDR)lParam)->code) {
						case TCN_SELCHANGING:
						{	PSHNOTIFY pshn;
							if(dat->currentPage==-1 || dat->opd[dat->currentPage].hwnd==NULL) break;
							pshn.hdr.code=PSN_KILLACTIVE;
							pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
							pshn.hdr.idFrom=0;
							pshn.lParam=(LPARAM)0;
							if(SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn)) {
								SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
								return TRUE;
							}
							break;
						}
						case TCN_SELCHANGE:
							if(dat->currentPage!=-1 && dat->opd[dat->currentPage].hwnd!=NULL) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_HIDE);
							dat->currentPage=TabCtrl_GetCurSel(GetDlgItem(hwndDlg,IDC_TABS));
							if(dat->currentPage!=-1) {
								if(dat->opd[dat->currentPage].hwnd==NULL) {
									PSHNOTIFY pshn;
									dat->opd[dat->currentPage].hwnd=CreateDialogIndirectParam(dat->opd[dat->currentPage].hInst,dat->opd[dat->currentPage].pTemplate,hwndDlg,dat->opd[dat->currentPage].dlgProc,(LPARAM)dat->prof);
									ThemeDialogBackground(dat->opd[dat->currentPage].hwnd);
									SetWindowPos(dat->opd[dat->currentPage].hwnd,HWND_TOP,dat->rcDisplay.left,dat->rcDisplay.top,0,0,SWP_NOSIZE);
									pshn.hdr.code=PSN_INFOCHANGED;
									pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
									pshn.hdr.idFrom=0;
									pshn.lParam=(LPARAM)0;
									SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
								}
								ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);
							}
							break;
					}
					break;
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
				{	int i;
					PSHNOTIFY pshn;
					pshn.hdr.idFrom=0;
					pshn.lParam=0;
					pshn.hdr.code=PSN_RESET;					
					for(i=0;i<dat->pageCount;i++) {
						if(dat->opd[i].hwnd==NULL || !dat->opd[i].changed) continue;
						pshn.hdr.hwndFrom=dat->opd[i].hwnd;
						SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
					}
					EndDialog(hwndDlg,0);					
					break;
				}
				case IDOK:
				{	
					int i;
					PSHNOTIFY pshn;
					pshn.hdr.idFrom=0;
					pshn.lParam=(LPARAM)0;					
					if(dat->currentPage!=-1) {
						pshn.hdr.code=PSN_KILLACTIVE;
						pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
						if(SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn))
							break;
					}

					pshn.hdr.code=PSN_APPLY;
					for(i=0;i<dat->pageCount;i++) {
						if(dat->opd[i].hwnd==NULL || !dat->opd[i].changed) continue;
						pshn.hdr.hwndFrom=dat->opd[i].hwnd;
						SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
						if ( GetWindowLong(dat->opd[i].hwnd,DWL_MSGRESULT) == PSNRET_INVALID_NOCHANGEPAGE) {
							TabCtrl_SetCurSel(GetDlgItem(hwndDlg,IDC_TABS),i);
							if(dat->currentPage!=-1) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_HIDE);
							dat->currentPage=i;
							ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);							
							return 0;
						}
					}	
					EndDialog(hwndDlg,1);
					break;
				}
			}
			break;
		case WM_DESTROY:
			SendDlgItemMessage(hwndDlg,IDC_NAME,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDC_WHITERECT,WM_GETFONT,0,0),0);
			DeleteObject(dat->hBoldFont);
			{	int i;
				for(i=0;i<dat->pageCount;i++)
					if(dat->opd[i].hwnd!=NULL) DestroyWindow(dat->opd[i].hwnd);
			}
			mir_free(dat->opd);
			mir_free(dat);
			break;
	}
	return FALSE;
}

static int AddProfileManagerPage(struct DetailsPageInit * opi, OPTIONSDIALOGPAGE * odp)
{
	if(opi==NULL||odp==NULL);
	if(odp->cbSize!=sizeof(OPTIONSDIALOGPAGE)) return 1;
	opi->odp=(OPTIONSDIALOGPAGE*)mir_realloc(opi->odp,sizeof(OPTIONSDIALOGPAGE)*(opi->pageCount+1));
	opi->odp[opi->pageCount].cbSize=sizeof(OPTIONSDIALOGPAGE);
	opi->odp[opi->pageCount].hInstance=odp->hInstance;
	opi->odp[opi->pageCount].pfnDlgProc=odp->pfnDlgProc;
	opi->odp[opi->pageCount].position=odp->position;
	opi->odp[opi->pageCount].ptszTitle=LangPackPcharToTchar(odp->pszTitle);
	if((DWORD)odp->pszTemplate&0xFFFF0000) opi->odp[opi->pageCount].pszTemplate=mir_strdup(odp->pszTemplate);
	else opi->odp[opi->pageCount].pszTemplate=odp->pszTemplate;
	opi->odp[opi->pageCount].pszGroup=NULL;
	opi->odp[opi->pageCount].groupPosition=odp->groupPosition;
	opi->odp[opi->pageCount].hGroupIcon=odp->hGroupIcon;
	opi->odp[opi->pageCount].hIcon=odp->hIcon;
	opi->pageCount++;
	return 0;
}


int getProfileManager(PROFILEMANAGERDATA * pd)
{	
	PROPSHEETHEADER psh;
	struct DlgProfData prof;
	struct DetailsPageInit opi;
	int rc=0;
	int i;

	opi.pageCount=0;
	opi.odp=NULL;
	
	{ // remember what the default profile is, if any.
		char defaultProfile[MAX_PATH];
		GetPrivateProfileStringA("Database", "DefaultProfile", "", defaultProfile, SIZEOF(defaultProfile), mirandabootini);
		ExpandEnvironmentStringsA(defaultProfile, szDefaultMirandaProfile, SIZEOF(szDefaultMirandaProfile));
	}

	{
		OPTIONSDIALOGPAGE odp;
		ZeroMemory(&odp,sizeof(odp));
		odp.cbSize=sizeof(odp);
		odp.pszTitle=Translate("My Profiles");
		odp.pfnDlgProc=DlgProfileSelect;
		odp.pszTemplate=MAKEINTRESOURCEA(IDD_PROFILE_SELECTION);
		odp.hInstance=GetModuleHandle(NULL);
		AddProfileManagerPage(&opi, &odp);

		odp.pszTitle=Translate("New Profile");
		odp.pszTemplate=MAKEINTRESOURCEA(IDD_PROFILE_NEW);
		odp.pfnDlgProc=DlgProfileNew;
		AddProfileManagerPage(&opi, &odp);
	}

	ZeroMemory(&psh,sizeof(psh));
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_PROPSHEETPAGE|PSH_NOAPPLYNOW;
	psh.hwndParent = NULL;
	psh.nPages = opi.pageCount;
	psh.pStartPage = 0;
	psh.ppsp = (PROPSHEETPAGE*)opi.odp;	
	prof.pd=pd;
	prof.psh=&psh;
	rc=DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_PROFILEMANAGER),NULL,DlgProfileManager,(LPARAM)&prof);

	if (rc != -1)
	{
		for(i=0;i<opi.pageCount;i++)
		{
			mir_free((char*)opi.odp[i].pszTitle);
			if(opi.odp[i].pszGroup!=NULL) mir_free(opi.odp[i].pszGroup);
			if((DWORD)opi.odp[i].pszTemplate&0xFFFF0000) mir_free((char*)opi.odp[i].pszTemplate);
		}
	}
	if ( opi.odp != NULL )
		mir_free(opi.odp);

	return rc;
}
