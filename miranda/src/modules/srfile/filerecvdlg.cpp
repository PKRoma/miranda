/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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
#include <shlobj.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include "file.h"

#define MAX_MRU_DIRS    5

static BOOL CALLBACK ClipSiblingsChildEnumProc(HWND hwnd, LPARAM)
{
	SetWindowLongPtr(hwnd,GWL_STYLE,GetWindowLongPtr(hwnd,GWL_STYLE)|WS_CLIPSIBLINGS);
	return TRUE;
}

static void GetLowestExistingDirName(const char *szTestDir,char *szExistingDir,int cchExistingDir)
{
	DWORD dwAttributes;
	char *pszLastBackslash;

	lstrcpynA(szExistingDir,szTestDir,cchExistingDir);
	while((dwAttributes=GetFileAttributesA(szExistingDir))!=0xffffffff && !(dwAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
		pszLastBackslash=strrchr(szExistingDir,'\\');
		if(pszLastBackslash==NULL) {*szExistingDir='\0'; break;}
		*pszLastBackslash='\0';
	}
	if(szExistingDir[0]=='\0') GetCurrentDirectoryA(cchExistingDir,szExistingDir);
}

static const char validFilenameChars[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_!&{}-=#@~,. ";
static void RemoveInvalidFilenameChars(char *szString)
{
	size_t i;
	for(i=strspn(szString,validFilenameChars);szString[i];i+=strspn(szString+i+1,validFilenameChars)+1)
		if(szString[i]>=0) szString[i]='%';
}

static INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	char szDir[MAX_PATH];
	switch(uMsg) {
	case BFFM_INITIALIZED:
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
		break;
	case BFFM_SELCHANGED:
		if (SHGetPathFromIDListA((LPITEMIDLIST) lp ,szDir))
			SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
		break;
	}
	return 0;
}

int BrowseForFolder(HWND hwnd,char *szPath)
{
	BROWSEINFOA bi={0};
	LPMALLOC pMalloc;
	LPITEMIDLIST pidlResult;
	int result=0;

	if(SUCCEEDED(OleInitialize(NULL))) {
		if(SUCCEEDED(CoGetMalloc(1,&pMalloc))) {
			bi.hwndOwner=hwnd;
			bi.pszDisplayName=szPath;
			bi.lpszTitle=Translate("Select Folder");
			bi.ulFlags=BIF_NEWDIALOGSTYLE|BIF_EDITBOX|BIF_RETURNONLYFSDIRS;				// Use this combo instead of BIF_USENEWUI
			bi.lpfn=BrowseCallbackProc;
			bi.lParam=(LPARAM)szPath;

			pidlResult=SHBrowseForFolderA(&bi);
			if(pidlResult) {
				SHGetPathFromIDListA(pidlResult,szPath);
				lstrcatA(szPath,"\\");
				result=1;
			}
			pMalloc->lpVtbl->Free(pMalloc,pidlResult);
			pMalloc->lpVtbl->Release(pMalloc);
		}
		OleUninitialize();
	}
	return result;
}

static REPLACEVARSARRAY sttVarsToReplace[] =
{
	{ ( TCHAR* )"///", ( TCHAR* )"//" },
	{ ( TCHAR* )"//", ( TCHAR* )"/" },
	{ ( TCHAR* )"()", ( TCHAR* )"" },
	{ NULL, NULL }
};

static void patchDir( char* str, size_t strSize )
{
	REPLACEVARSDATA dat = { 0 };
	dat.cbSize = sizeof( dat );
	dat.variables = sttVarsToReplace;

	char* result = ( char* )CallService( MS_UTILS_REPLACEVARS, (WPARAM)str, (LPARAM)&dat );
	if ( result ) {
		strncpy( str, result, strSize );
		mir_free( result );
	}

	size_t len = lstrlenA( str );
	if ( len+1 < strSize && str[len-1] != '\\' )
		lstrcpyA( str+len, "\\" );
}

void GetContactReceivedFilesDir(HANDLE hContact,char *szDir,int cchDir, BOOL patchVars)
{
	DBVARIANT dbv;
	char szTemp[MAX_PATH];
	szTemp[0] = 0;

	if ( !DBGetContactSettingString( NULL, "SRFile", "RecvFilesDirAdv", &dbv)) {
		if ( lstrlenA( dbv.pszVal ) > 0 )
			lstrcpynA( szTemp, dbv.pszVal, SIZEOF( szTemp ));
		DBFreeVariant( &dbv );
	}

	if ( !szTemp[0] )
		mir_snprintf( szTemp, SIZEOF(szTemp), "%%miranda_path%%\\%s\\%%userid%%", Translate("Received Files"));

	if ( hContact ) {
		REPLACEVARSDATA dat = { 0 };
		dat.cbSize = sizeof( dat );
		dat.hContact = hContact;
		char* result = ( char* )CallService( MS_UTILS_REPLACEVARS, (WPARAM)szTemp, (LPARAM)&dat );
		if ( result ) {
			strncpy( szTemp, result, SIZEOF(szTemp));
			mir_free( result );
	}	}

	if (patchVars)
		patchDir( szTemp, SIZEOF(szTemp));
	lstrcpynA( szDir, szTemp, cchDir );
}

void GetReceivedFilesDir(char *szDir,int cchDir)
{
	DBVARIANT dbv;
	char szTemp[MAX_PATH];
	szTemp[0] = 0;

	if ( !DBGetContactSettingString( NULL, "SRFile", "RecvFilesDirAdv", &dbv )) {
		if ( lstrlenA( dbv.pszVal ) > 0 )
			lstrcpynA( szTemp, dbv.pszVal, SIZEOF( szTemp ));
		DBFreeVariant(&dbv);
	}

	if ( !szTemp[0] )
		mir_snprintf( szTemp, SIZEOF(szTemp), "%%miranda_path%%\\%s", Translate("Received Files"));

	patchDir( szTemp, SIZEOF(szTemp));
	lstrcpynA( szDir, szTemp, cchDir );
}

INT_PTR CALLBACK DlgProcRecvFile(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct FileDlgData *dat;

	dat=(struct FileDlgData*)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
	switch (msg) {
	case WM_INITDIALOG: {
		TCHAR *contactName;
		char szPath[450];
		CLISTEVENT* cle = (CLISTEVENT*)lParam;

		TranslateDialogDefault(hwndDlg);

		dat=(struct FileDlgData*)mir_calloc(sizeof(struct FileDlgData));
		SetWindowLongPtr(hwndDlg,GWLP_USERDATA,(LONG_PTR)dat);
		dat->hContact = cle->hContact;
		dat->hDbEvent = cle->hDbEvent;
		dat->hPreshutdownEvent = HookEventMessage(ME_SYSTEM_PRESHUTDOWN,hwndDlg,M_PRESHUTDOWN);
		dat->dwTicks = GetTickCount();

		EnumChildWindows(hwndDlg,ClipSiblingsChildEnumProc,0);

		Window_SetIcon_IcoLib(hwndDlg, SKINICON_EVENT_FILE);
		Button_SetIcon_IcoLib(hwndDlg, IDC_ADD, SKINICON_OTHER_ADDCONTACT, "Add Contact Permanently to List");
		Button_SetIcon_IcoLib(hwndDlg, IDC_DETAILS, SKINICON_OTHER_USERDETAILS, "View User's Details");
		Button_SetIcon_IcoLib(hwndDlg, IDC_HISTORY, SKINICON_OTHER_HISTORY, "View User's History");
		Button_SetIcon_IcoLib(hwndDlg, IDC_USERMENU, SKINICON_OTHER_DOWNARROW, "User Menu");

		contactName = cli.pfnGetContactDisplayName( dat->hContact, 0 );
		SetDlgItemText(hwndDlg,IDC_FROM,contactName);
		GetContactReceivedFilesDir(dat->hContact,szPath,SIZEOF(szPath),TRUE);
		SetDlgItemTextA(hwndDlg,IDC_FILEDIR,szPath);
		{
			int i;
			char idstr[32];
			DBVARIANT dbv;

			if (shAutoComplete)
				shAutoComplete(GetWindow(GetDlgItem(hwndDlg,IDC_FILEDIR),GW_CHILD),1);

			for(i=0;i<MAX_MRU_DIRS;i++) {
				mir_snprintf(idstr, SIZEOF(idstr), "MruDir%d",i);
				if(DBGetContactSettingString(NULL,"SRFile",idstr,&dbv)) break;
				SendDlgItemMessageA(hwndDlg,IDC_FILEDIR,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
				DBFreeVariant(&dbv);
			}
		}

		CallService(MS_DB_EVENT_MARKREAD,(WPARAM)dat->hContact,(LPARAM)dat->hDbEvent);
		{
			DBEVENTINFO dbei={0};
			DBTIMETOSTRINGT dbtts;
			TCHAR datetimestr[64];

			dbei.cbSize=sizeof(dbei);
			dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)dat->hDbEvent,0);
			dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob);
			CallService(MS_DB_EVENT_GET,(WPARAM)dat->hDbEvent,(LPARAM)&dbei);
			dat->fs = cle->lParam ? (HANDLE)cle->lParam : (HANDLE)*(PDWORD)dbei.pBlob;
			lstrcpynA(szPath, (char*)dbei.pBlob+4, min(dbei.cbBlob+1,SIZEOF(szPath)));
			SetDlgItemTextA(hwndDlg,IDC_FILENAMES,szPath);
			lstrcpynA(szPath, (char*)dbei.pBlob+4+strlen((char*)dbei.pBlob+4)+1, min((int)(dbei.cbBlob-4-strlen((char*)dbei.pBlob+4)),SIZEOF(szPath)));
			SetDlgItemTextA(hwndDlg,IDC_MSG,szPath);
			mir_free(dbei.pBlob);

			dbtts.szFormat = _T("t d");
			dbtts.szDest = datetimestr;
			dbtts.cbDest = SIZEOF(datetimestr);
			CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, dbei.timestamp, ( LPARAM )&dbtts);
			SetDlgItemText(hwndDlg, IDC_DATE, datetimestr);
		}
		{
			char* szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)dat->hContact, 0);
			if (szProto) {
				CONTACTINFO ci;
				int hasName = 0;
				char buf[128];
				ZeroMemory(&ci,sizeof(ci));

				ci.cbSize = sizeof(ci);
				ci.hContact = dat->hContact;
				ci.szProto = szProto;
				ci.dwFlag = CNF_UNIQUEID;
				if (!CallService(MS_CONTACT_GETCONTACTINFO,0,(LPARAM)&ci)) {
					switch(ci.type) {
					case CNFT_ASCIIZ:
						hasName = 1;
						mir_snprintf(buf, SIZEOF(buf), "%s", ci.pszVal);
						mir_free(ci.pszVal);
						break;
					case CNFT_DWORD:
						hasName = 1;
						mir_snprintf(buf, SIZEOF(buf),"%u",ci.dVal);
						break;
				}	}
				if (hasName)
					SetDlgItemTextA(hwndDlg, IDC_NAME, buf );
				else
					SetDlgItemText(hwndDlg, IDC_NAME, contactName);
		}	}

		if(DBGetContactSettingByte(dat->hContact,"CList","NotOnList",0)) {
			RECT rcBtn1,rcBtn2,rcDateCtrl;
			GetWindowRect(GetDlgItem(hwndDlg,IDC_ADD),&rcBtn1);
			GetWindowRect(GetDlgItem(hwndDlg,IDC_USERMENU),&rcBtn2);
			GetWindowRect(GetDlgItem(hwndDlg,IDC_DATE),&rcDateCtrl);
			SetWindowPos(GetDlgItem(hwndDlg,IDC_DATE),0,0,0,rcDateCtrl.right-rcDateCtrl.left-(rcBtn2.left-rcBtn1.left),rcDateCtrl.bottom-rcDateCtrl.top,SWP_NOZORDER|SWP_NOMOVE);
		}
		else if(DBGetContactSettingByte(NULL,"SRFile","AutoAccept",0)) {
			//don't check auto-min here to fix BUG#647620
			PostMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDOK,BN_CLICKED),(LPARAM)GetDlgItem(hwndDlg,IDOK));
		}
		if(!DBGetContactSettingByte(dat->hContact,"CList","NotOnList",0))
			ShowWindow(GetDlgItem(hwndDlg, IDC_ADD),SW_HIDE);
		return TRUE;
	}

	case WM_MEASUREITEM:
		return CallService(MS_CLIST_MENUMEASUREITEM,wParam,lParam);

	case WM_DRAWITEM:
		{	LPDRAWITEMSTRUCT dis=(LPDRAWITEMSTRUCT)lParam;
			if(dis->hwndItem==GetDlgItem(hwndDlg, IDC_PROTOCOL)) {
				char *szProto;

				szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)dat->hContact,0);
				if (szProto) {
					HICON hIcon;

					hIcon=(HICON)CallProtoService(szProto,PS_LOADICON,PLI_PROTOCOL|PLIF_SMALL,0);
					if (hIcon) {
						DrawIconEx(dis->hDC,dis->rcItem.left,dis->rcItem.top,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
						DestroyIcon(hIcon);
		}	}	}	}
		return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);

	case WM_COMMAND:
		if ( CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam),MPCF_CONTACTMENU), (LPARAM)dat->hContact ))
			break;

		switch ( LOWORD( wParam )) {
		case IDC_FILEDIRBROWSE:
			{
				char szDirName[MAX_PATH],szExistingDirName[MAX_PATH];

				GetDlgItemTextA(hwndDlg,IDC_FILEDIR,szDirName,SIZEOF(szDirName));
				GetLowestExistingDirName(szDirName,szExistingDirName,SIZEOF(szExistingDirName));
				if(BrowseForFolder(hwndDlg,szExistingDirName))
					SetDlgItemTextA(hwndDlg,IDC_FILEDIR,szExistingDirName);
				return TRUE;
			}
		case IDOK:
			{	//most recently used directories
				char szRecvDir[MAX_PATH],szDefaultRecvDir[MAX_PATH];
				GetDlgItemTextA(hwndDlg,IDC_FILEDIR,szRecvDir,SIZEOF(szRecvDir));
				GetContactReceivedFilesDir(NULL,szDefaultRecvDir,SIZEOF(szDefaultRecvDir),TRUE);
				if(_strnicmp(szRecvDir,szDefaultRecvDir,lstrlenA(szDefaultRecvDir))) {
					char idstr[32];
					int i;
					DBVARIANT dbv;
					for(i=MAX_MRU_DIRS-2;i>=0;i--) {
						mir_snprintf(idstr, SIZEOF(idstr), "MruDir%d",i);
						if(DBGetContactSettingString(NULL,"SRFile",idstr,&dbv)) continue;
						mir_snprintf(idstr, SIZEOF(idstr), "MruDir%d",i+1);
						DBWriteContactSettingString(NULL,"SRFile",idstr,dbv.pszVal);
						DBFreeVariant(&dbv);
					}
					DBWriteContactSettingString(NULL,"SRFile",idstr,szRecvDir);
				}
			}
			EnableWindow(GetDlgItem(hwndDlg,IDC_FILENAMES),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MSG),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_FILEDIR),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_FILEDIRBROWSE),FALSE);

			GetDlgItemTextA(hwndDlg,IDC_FILEDIR,dat->szSavePath,SIZEOF(dat->szSavePath));
			GetDlgItemTextA(hwndDlg,IDC_FILE,dat->szFilenames,SIZEOF(dat->szFilenames));
			GetDlgItemTextA(hwndDlg,IDC_MSG,dat->szMsg,SIZEOF(dat->szMsg));
			dat->hwndTransfer=FtMgr_AddTransfer(dat);
			//check for auto-minimize here to fix BUG#647620
			if(DBGetContactSettingByte(NULL,"SRFile","AutoAccept",0) && DBGetContactSettingByte(NULL,"SRFile","AutoMin",0)) {
				ShowWindow(hwndDlg,SW_HIDE);
				ShowWindow(hwndDlg,SW_SHOWMINNOACTIVE);
			}
			DestroyWindow(hwndDlg);
			return TRUE;
		case IDCANCEL:
			if (dat->fs) CallContactService(dat->hContact,PSS_FILEDENY,(WPARAM)dat->fs,(LPARAM)Translate("Cancelled"));
			dat->fs=NULL; /* the protocol will free the handle */
			DestroyWindow(hwndDlg);
			return TRUE;
		case IDC_ADD:
			{	ADDCONTACTSTRUCT acs={0};

				acs.handle=dat->hContact;
				acs.handleType=HANDLE_CONTACT;
				acs.szProto="";
				CallService(MS_ADDCONTACT_SHOW,(WPARAM)hwndDlg,(LPARAM)&acs);
				if(!DBGetContactSettingByte(dat->hContact,"CList","NotOnList",0))
					ShowWindow(GetDlgItem(hwndDlg,IDC_ADD), SW_HIDE);
				return TRUE;
			}
		case IDC_USERMENU:
			{	RECT rc;
				HMENU hMenu=(HMENU)CallService(MS_CLIST_MENUBUILDCONTACT,(WPARAM)dat->hContact,0);
				GetWindowRect((HWND)lParam,&rc);
				TrackPopupMenu(hMenu,0,rc.left,rc.bottom,0,hwndDlg,NULL);
				DestroyMenu(hMenu);
				break;
			}
		case IDC_DETAILS:
			CallService(MS_USERINFO_SHOWDIALOG,(WPARAM)dat->hContact,0);
			return TRUE;
		case IDC_HISTORY:
			CallService(MS_HISTORY_SHOWCONTACTHISTORY,(WPARAM)dat->hContact,0);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		Window_FreeIcon_IcoLib(hwndDlg);
		Button_FreeIcon_IcoLib(hwndDlg,IDC_ADD);
		Button_FreeIcon_IcoLib(hwndDlg,IDC_DETAILS);
		Button_FreeIcon_IcoLib(hwndDlg,IDC_HISTORY);
		Button_FreeIcon_IcoLib(hwndDlg,IDC_USERMENU);
		if(dat->hPreshutdownEvent) UnhookEvent(dat->hPreshutdownEvent);
		return TRUE;
	}
	return FALSE;
}
