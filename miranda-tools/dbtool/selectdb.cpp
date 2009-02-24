/*
Miranda Database Tool
Copyright (C) 2001-2005  Richard Hughes

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
#include "dbtool.h"

BOOL CALLBACK WelcomeDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK OpenErrorDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK FileAccessDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);

void GetProfileDirectory(TCHAR* szMirandaDir, TCHAR* szPath, int cbPath)
{
	TCHAR szProfileDir[MAX_PATH], szExpandedProfileDir[MAX_PATH], szMirandaBootIni[MAX_PATH];

	lstrcpy(szMirandaBootIni,szMirandaDir);
	lstrcat(szMirandaBootIni,_T("\\mirandaboot.ini"));
	GetPrivateProfileString(_T("Database"),_T("ProfileDir"),_T("."),szProfileDir,SIZEOF(szProfileDir),szMirandaBootIni);
	ExpandEnvironmentStrings(szProfileDir,szExpandedProfileDir,sizeof(szExpandedProfileDir));
	_tchdir(szMirandaDir);
	if(!_tfullpath(szPath,szExpandedProfileDir,cbPath))
		lstrcpyn(szPath,szMirandaDir,cbPath);
	if(szPath[lstrlen(szPath)-1]=='\\') 
		szPath[lstrlen(szPath)-1]='\0';
}

static int AddDatabaseToList(HWND hwndList, TCHAR* filename)
{
	LV_ITEM lvi;
	int iNewItem;
	TCHAR szSize[20], *pName, *pDot, szName[MAX_PATH];
	HANDLE hDbFile;
	DBHeader dbhdr;
	DWORD bytesRead;
	DWORD totalSize,wasted;
	lvi.mask=LVIF_PARAM;
	lvi.iSubItem=0;
	for(lvi.iItem=ListView_GetItemCount(hwndList)-1;lvi.iItem>=0;lvi.iItem--) {
		ListView_GetItem(hwndList,&lvi);
		if( !_tcsicmp(( TCHAR* )lvi.lParam,filename)) return lvi.iItem;
	}
	hDbFile=CreateFile(filename,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
	if ( hDbFile == INVALID_HANDLE_VALUE )
		return -1;

	ReadFile(hDbFile,&dbhdr,sizeof(dbhdr),&bytesRead,NULL);
	totalSize=GetFileSize(hDbFile,NULL);
	if ( bytesRead < sizeof( dbhdr ))
		wasted=0;
	else {
		wasted = dbhdr.slackSpace;
		if ( totalSize > dbhdr.ofsFileEnd )
			wasted += totalSize - dbhdr.ofsFileEnd;
	}
	CloseHandle(hDbFile);

	pName = _tcsrchr(filename,'\\');
	if ( pName == NULL )
		pName = ( LPTSTR )filename;
	else
		pName++;

	_tcscpy(szName,pName);
	pDot = _tcsrchr( szName, '.' );
	if ( pDot != NULL )
		*pDot=0;

	lvi.iItem = 0;
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	lvi.iSubItem = 0;
	lvi.lParam = ( LPARAM )_tcsdup(filename);
	lvi.pszText = szName;
	if ( wasted < 1024*128 )
		lvi.iImage = 0;
	else if ( wasted < 1024*256 + (DWORD)(totalSize > 2*1024*1024 ) ? 256 * 1024 : 0 )
		lvi.iImage=1;
	else
		lvi.iImage=2;

	iNewItem = ListView_InsertItem(hwndList, &lvi );
	_stprintf(szSize,_T("%.2lf MB"),totalSize/1048576.0);
	ListView_SetItemText(hwndList,iNewItem,1,szSize);
	_stprintf(szSize,_T("%.2lf MB"),wasted/1048576.0);
	ListView_SetItemText(hwndList,iNewItem,2,szSize);
	return iNewItem;
}

BOOL CALLBACK SelectDbDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	BOOL bReturn;
	if ( DoMyControlProcessing( hdlg, message, wParam, lParam, &bReturn ))
		return bReturn;

	switch ( message ) {
		case WM_INITDIALOG:
		{	
			TCHAR szMirandaPath[MAX_PATH];
			szMirandaPath[ 0 ] = 0;
			{	HIMAGELIST hIml;
				hIml=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR4|ILC_MASK,3,3);
				ImageList_AddIcon(hIml,LoadIcon(hInst,MAKEINTRESOURCE(IDI_PROFILEGREEN)));
				ImageList_AddIcon(hIml,LoadIcon(hInst,MAKEINTRESOURCE(IDI_PROFILEYELLOW)));
				ImageList_AddIcon(hIml,LoadIcon(hInst,MAKEINTRESOURCE(IDI_PROFILERED)));
				ListView_SetImageList(GetDlgItem(hdlg,IDC_DBLIST),hIml,LVSIL_SMALL);
			}
			ListView_SetExtendedListViewStyleEx(GetDlgItem(hdlg,IDC_DBLIST),LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
			{	LV_COLUMN lvc;
				lvc.mask = LVCF_WIDTH | LVCF_FMT | LVCF_TEXT;
				lvc.cx = 195;
				lvc.fmt = LVCFMT_LEFT;
				lvc.pszText = TranslateT("Database");
				ListView_InsertColumn( GetDlgItem(hdlg,IDC_DBLIST), 0, &lvc );
				lvc.cx = 70;
				lvc.fmt = LVCFMT_RIGHT;
				lvc.pszText = TranslateT("Total size");
				ListView_InsertColumn(GetDlgItem(hdlg,IDC_DBLIST), 1, &lvc );
				lvc.pszText = TranslateT("Wasted");
				ListView_InsertColumn(GetDlgItem(hdlg,IDC_DBLIST), 2, &lvc );
			}
			{	HKEY hKey;
				DWORD cbData = sizeof( szMirandaPath );
				if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\miranda32.exe"),0,KEY_QUERY_VALUE,&hKey)==ERROR_SUCCESS) {
					if ( RegQueryValueEx(hKey,_T("Path"),NULL,NULL,(PBYTE)szMirandaPath,&cbData)!=ERROR_SUCCESS)
						szMirandaPath[0] = 0;
					RegCloseKey( hKey );
				}
			}
			if ( szMirandaPath[0] == 0 ) {
				GetModuleFileName(NULL,szMirandaPath,SIZEOF(szMirandaPath));
				TCHAR* str2 = _tcsrchr(szMirandaPath,'\\');
				if ( str2 != NULL )
					*str2 = 0;
			}
			{	HANDLE hFind;
				WIN32_FIND_DATA fd;
				TCHAR szSearchPath[MAX_PATH],szFilename[MAX_PATH],szProfileDir[MAX_PATH];
				GetProfileDirectory(szMirandaPath,szProfileDir,SIZEOF(szProfileDir));
				lstrcpy( szSearchPath, szProfileDir );
				lstrcat( szSearchPath, _T("\\*.dat"));
				hFind = FindFirstFile( szSearchPath, &fd );
				if ( hFind != INVALID_HANDLE_VALUE ) {
					do {
						wsprintf( szFilename, _T("%s\\%s"), szProfileDir, fd.cFileName);
						AddDatabaseToList( GetDlgItem( hdlg, IDC_DBLIST ), szFilename );
					}
						while( FindNextFile( hFind, &fd ));
					FindClose(hFind);
				}
			}
			{	int i=0;
				if ( opts.filename[0] )
					i = AddDatabaseToList( GetDlgItem( hdlg, IDC_DBLIST ), opts.filename );
				if ( i == -1 )
					i = 0;
				ListView_SetItemState( GetDlgItem(hdlg,IDC_DBLIST), i, LVIS_SELECTED, LVIS_SELECTED );
			}
			if ( opts.hFile != NULL && opts.hFile != INVALID_HANDLE_VALUE )
				CloseHandle( opts.hFile );
			TranslateDialog( hdlg );
			return TRUE;
		}

		case WZN_PAGECHANGING:
			GetDlgItemText( hdlg, IDC_FILE, opts.filename, SIZEOF(opts.filename));
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_FILE:
					if(HIWORD(wParam)==EN_CHANGE)
						EnableWindow(GetDlgItem(GetParent(hdlg),IDOK),GetWindowTextLength(GetDlgItem(hdlg,IDC_FILE)));
					break;
				case IDC_OTHER:
				{	OPENFILENAME ofn={0};
					TCHAR str[MAX_PATH];
					GetDlgItemText( hdlg, IDC_FILE, str, SIZEOF( str ));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hdlg;
					ofn.hInstance = NULL;
					ofn.lpstrFilter = _T("Miranda Databases (*.dat)\0*.DAT\0All Files (*)\0*\0");
					ofn.lpstrDefExt = _T("dat");
					ofn.lpstrFile = str;
					ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					ofn.nMaxFile = SIZEOF(str);
					ofn.nMaxFileTitle = MAX_PATH;
					if ( GetOpenFileName( &ofn )) {
						int i;
						i = AddDatabaseToList( GetDlgItem(hdlg,IDC_DBLIST), str );
						if ( i == -1 )
							i=0;
						ListView_SetItemState( GetDlgItem(hdlg,IDC_DBLIST), i, LVIS_SELECTED, LVIS_SELECTED );
					}
					break;
				}
				case IDC_BACK:
					SendMessage(GetParent(hdlg),WZM_GOTOPAGE,IDD_WELCOME,(LPARAM)WelcomeDlgProc);
					break;
				case IDOK:
					opts.hFile = CreateFile( opts.filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
					if ( opts.hFile == INVALID_HANDLE_VALUE ) {
						opts.error = GetLastError();
						SendMessage( GetParent(hdlg), WZM_GOTOPAGE, IDD_OPENERROR, ( LPARAM )OpenErrorDlgProc );
					}
					else SendMessage( GetParent(hdlg), WZM_GOTOPAGE, IDD_FILEACCESS, (LPARAM)FileAccessDlgProc );
					break;
			}
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case IDC_DBLIST:
					switch(((LPNMLISTVIEW)lParam)->hdr.code) {
						case LVN_ITEMCHANGED:
						{	LV_ITEM lvi;
							lvi.iItem=ListView_GetNextItem(GetDlgItem(hdlg,IDC_DBLIST),-1,LVNI_SELECTED);
							if(lvi.iItem==-1) break;
							lvi.mask=LVIF_PARAM;
							ListView_GetItem(GetDlgItem(hdlg,IDC_DBLIST),&lvi);
							SetDlgItemText(hdlg,IDC_FILE,(TCHAR*)lvi.lParam);
							SendMessage(hdlg,WM_COMMAND,MAKEWPARAM(IDC_FILE,EN_CHANGE),(LPARAM)GetDlgItem(hdlg,IDC_FILE));
							break;
						}
					}
					break;
			}
			break;
		case WM_DESTROY:
			{	LV_ITEM lvi;
				lvi.mask=LVIF_PARAM;
				for(lvi.iItem=ListView_GetItemCount(GetDlgItem(hdlg,IDC_DBLIST))-1;lvi.iItem>=0;lvi.iItem--) {
					ListView_GetItem(GetDlgItem(hdlg,IDC_DBLIST),&lvi);
					free((char*)lvi.lParam);
				}
			}
			break;
	}
	return FALSE;
}