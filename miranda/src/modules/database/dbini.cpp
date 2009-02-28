/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project, 
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

#include "../../core/commonheaders.h"
#include "../srfile/file.h"

static BOOL bModuleInitialized = FALSE;
static HANDLE hIniChangeNotification;
extern char mirandabootini[MAX_PATH];

static BOOL CALLBACK InstallIniDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			SetDlgItemTextA(hwndDlg,IDC_ININAME,(char*)lParam);
			{	char szSecurity[11],*pszSecurityInfo;
			  	GetPrivateProfileStringA("AutoExec","Warn","notsafe",szSecurity,sizeof(szSecurity),mirandabootini);
				if(!lstrcmpiA(szSecurity,"all"))
					pszSecurityInfo="Security systems to prevent malicious changes are in place and you will be warned before every change that is made.";
				else if(!lstrcmpiA(szSecurity,"onlyunsafe"))
					pszSecurityInfo="Security systems to prevent malicious changes are in place and you will be warned before changes that are known to be unsafe.";
				else if(!lstrcmpiA(szSecurity,"none"))
					pszSecurityInfo="Security systems to prevent malicious changes have been disabled. You will receive no further warnings.";
				else pszSecurityInfo=NULL;
				if(pszSecurityInfo) SetDlgItemTextA(hwndDlg,IDC_SECURITYINFO,ServiceExists(MS_LANGPACK_TRANSLATESTRING)?Translate(pszSecurityInfo):pszSecurityInfo);
			}
			return TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_VIEWINI:
				{	char szPath[MAX_PATH];
					GetDlgItemTextA(hwndDlg,IDC_ININAME,szPath,sizeof(szPath));
					ShellExecuteA(hwndDlg,"open",szPath,NULL,NULL,SW_SHOW);
					break;
				}
				case IDOK:
				case IDCANCEL:
				case IDC_NOTOALL:
					EndDialog(hwndDlg,LOWORD(wParam));
					break;
			}
			break;
	}
	return FALSE;
}

static int IsInSpaceSeparatedList(const char *szWord,const char *szList)
{
	char *szItem,*szEnd;
	int wordLen = lstrlenA(szWord);

	for(szItem = (char*)szList;;) {
		szEnd = strchr(szItem,' ');
		if (szEnd == NULL)
			return !lstrcmpA( szItem, szWord );
		if ( szEnd - szItem == wordLen ) {
			if ( !strncmp( szItem, szWord, wordLen ))
				return 1;
		}
		szItem = szEnd+1;
}	}

struct warnSettingChangeInfo_t {
	char *szIniPath;
	char *szSection;
	char *szSafeSections;
	char *szUnsafeSections;
	char *szName;
	char *szValue;
	int warnNoMore,cancel;
};

static BOOL CALLBACK WarnIniChangeDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	static struct warnSettingChangeInfo_t *warnInfo;

	switch(message) {
		case WM_INITDIALOG:
		{	char szSettingName[256];
			char *pszSecurityInfo;
			warnInfo=(struct warnSettingChangeInfo_t*)lParam;
			TranslateDialogDefault(hwndDlg);
			SetDlgItemTextA(hwndDlg,IDC_ININAME,warnInfo->szIniPath);
			lstrcpyA(szSettingName,warnInfo->szSection);
			lstrcatA(szSettingName," / ");
			lstrcatA(szSettingName,warnInfo->szName);
			SetDlgItemTextA(hwndDlg,IDC_SETTINGNAME,szSettingName);
			SetDlgItemTextA(hwndDlg,IDC_NEWVALUE,warnInfo->szValue);
			if(IsInSpaceSeparatedList(warnInfo->szSection,warnInfo->szSafeSections))
				pszSecurityInfo="This change is known to be safe.";
			else if(IsInSpaceSeparatedList(warnInfo->szSection,warnInfo->szUnsafeSections))
				pszSecurityInfo="This change is known to be potentially hazardous.";
			else
				pszSecurityInfo="This change is not known to be safe.";
			SetDlgItemTextA(hwndDlg,IDC_SECURITYINFO,ServiceExists(MS_LANGPACK_TRANSLATESTRING)?Translate(pszSecurityInfo):pszSecurityInfo);
			return TRUE;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					warnInfo->cancel=1;
				case IDYES:
				case IDNO:
					warnInfo->warnNoMore=IsDlgButtonChecked(hwndDlg,IDC_WARNNOMORE);
					EndDialog(hwndDlg,LOWORD(wParam));
					break;
			}
			break;
	}
	return FALSE;
}

static BOOL CALLBACK IniImportDoneDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			SetDlgItemTextA(hwndDlg,IDC_ININAME,(char*)lParam);
			SetDlgItemTextA(hwndDlg,IDC_NEWNAME,(char*)lParam);
			return TRUE;
		case WM_COMMAND:
		{	char szIniPath[MAX_PATH];
			GetDlgItemTextA(hwndDlg,IDC_ININAME,szIniPath,sizeof(szIniPath));
			switch(LOWORD(wParam)) {
				case IDC_DELETE:
					DeleteFileA(szIniPath);
				case IDC_LEAVE:
					EndDialog(hwndDlg,LOWORD(wParam));
					break;
				case IDC_RECYCLE:
					{	SHFILEOPSTRUCTA shfo={0};
						shfo.wFunc=FO_DELETE;
						shfo.pFrom=szIniPath;
						szIniPath[lstrlenA(szIniPath)+1]='\0';
						shfo.fFlags=FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT;
						SHFileOperationA(&shfo);
					}
					EndDialog(hwndDlg,LOWORD(wParam));
					break;
				case IDC_MOVE:
					{	char szNewPath[MAX_PATH];
						GetDlgItemTextA(hwndDlg,IDC_NEWNAME,szNewPath,sizeof(szNewPath));
						MoveFileA(szIniPath,szNewPath);
					}
					EndDialog(hwndDlg,LOWORD(wParam));
					break;
			}
			break;
		}
	}
	return FALSE;
}

static void DoAutoExec(void)
{
	HANDLE hFind;
	char szMirandaDir[MAX_PATH],szUse[7],szIniPath[MAX_PATH],szFindPath[MAX_PATH],szExpandedFindPath[MAX_PATH];
	char szLine[2048];
	char *str2;
	WIN32_FIND_DATAA fd;
	FILE *fp;
	char szSection[128];
	int lineLength;
	char szSafeSections[2048],szUnsafeSections[2048],szSecurity[11],szOverrideSecurityFilename[MAX_PATH];
	int warnThisSection=0;

	GetPrivateProfileStringA("AutoExec","Use","prompt",szUse,sizeof(szUse),mirandabootini);
	if(!lstrcmpiA(szUse,"no")) return;
	GetPrivateProfileStringA("AutoExec","Safe","CLC Icons CLUI CList SkinSounds",szSafeSections,sizeof(szSafeSections),mirandabootini);
	GetPrivateProfileStringA("AutoExec","Unsafe","ICQ MSN",szUnsafeSections,sizeof(szUnsafeSections),mirandabootini);
	GetPrivateProfileStringA("AutoExec","Warn","notsafe",szSecurity,sizeof(szSecurity),mirandabootini);
	GetPrivateProfileStringA("AutoExec","OverrideSecurityFilename","",szOverrideSecurityFilename,sizeof(szOverrideSecurityFilename),mirandabootini);
	GetModuleFileNameA(hMirandaInst,szMirandaDir,sizeof(szMirandaDir));
	str2=strrchr(szMirandaDir,'\\');
	if(str2!=NULL) *str2=0;
	_chdir(szMirandaDir);
	GetPrivateProfileStringA("AutoExec","Glob","autoexec_*.ini",szFindPath,sizeof(szFindPath),mirandabootini);
	ExpandEnvironmentStringsA(szFindPath,szExpandedFindPath,sizeof(szExpandedFindPath));
	hFind=FindFirstFileA(szExpandedFindPath,&fd);
	if(hFind==INVALID_HANDLE_VALUE) return;
	str2=strrchr(szExpandedFindPath,'\\');
	if(str2==NULL) str2=szExpandedFindPath;
	else str2++;
	*str2='\0';
	szSection[0]='\0';
	do {
		lstrcpyA(szIniPath,szExpandedFindPath);
		lstrcatA(szIniPath,fd.cFileName);
		if(!lstrcmpiA(szUse,"prompt") && lstrcmpiA(fd.cFileName,szOverrideSecurityFilename)) {
			int result=DialogBoxParam(hMirandaInst,MAKEINTRESOURCE(IDD_INSTALLINI),NULL,InstallIniDlgProc,(LPARAM)szIniPath);
			if(result==IDC_NOTOALL) break;
			if(result==IDCANCEL) continue;
		}
		fp=fopen(szIniPath,"rt");
		while(!feof(fp)) {
			if(fgets(szLine,sizeof(szLine),fp)==NULL) break;
			lineLength=lstrlenA(szLine);
			while(lineLength && (BYTE)(szLine[lineLength-1])<=' ') szLine[--lineLength]='\0';
			if(szLine[0]==';' || szLine[0]<=' ') continue;
			if(szLine[0]=='[') {
				char *szEnd=strchr(szLine+1,']');
				if(szEnd==NULL) continue;
				if(szLine[1]=='!')
					szSection[0]='\0';
				else {
					lstrcpynA(szSection,szLine+1,min(sizeof(szSection),szEnd-szLine));
					if(!lstrcmpiA(szSecurity,"none")) warnThisSection=0;
					else if(!lstrcmpiA(szSecurity,"notsafe"))
						warnThisSection=!IsInSpaceSeparatedList(szSection,szSafeSections);
					else if(!lstrcmpiA(szSecurity,"onlyunsafe"))
						warnThisSection=IsInSpaceSeparatedList(szSection,szUnsafeSections);
					else warnThisSection=1;
					if(!lstrcmpiA(fd.cFileName,szOverrideSecurityFilename)) warnThisSection=0;
				}
			}
			else {
				char *szValue;
				char szName[128];
				struct warnSettingChangeInfo_t warnInfo;

				if(szSection[0]=='\0') continue;
				szValue=strchr(szLine,'=');
				if(szValue==NULL) continue;
				lstrcpynA(szName,szLine,min(sizeof(szName),szValue-szLine+1));
				szValue++;
				warnInfo.szIniPath=szIniPath;
				warnInfo.szName=szName;
				warnInfo.szSafeSections=szSafeSections;
				warnInfo.szSection=szSection;
				warnInfo.szUnsafeSections=szUnsafeSections;
				warnInfo.szValue=szValue;
				warnInfo.warnNoMore=0;
				warnInfo.cancel=0;
				if(!warnThisSection || IDNO!=DialogBoxParam(hMirandaInst,MAKEINTRESOURCE(IDD_WARNINICHANGE),NULL,WarnIniChangeDlgProc,(LPARAM)&warnInfo)) {
					if(warnInfo.cancel) break;
					if(warnInfo.warnNoMore) warnThisSection=0;
					switch(szValue[0]) {
						case 'b':
						case 'B':
							DBWriteContactSettingByte(NULL,szSection,szName,(BYTE)strtol(szValue+1,NULL,0));
							break;
						case 'w':
						case 'W':
							DBWriteContactSettingWord(NULL,szSection,szName,(WORD)strtol(szValue+1,NULL,0));
							break;
						case 'd':
						case 'D':
							DBWriteContactSettingDword(NULL,szSection,szName,(DWORD)strtoul(szValue+1,NULL,0));
							break;
						case 'l':
						case 'L':
							DBDeleteContactSetting(NULL,szSection,szName);
							break;
						case 's':
						case 'S':
							DBWriteContactSettingString(NULL,szSection,szName,szValue+1);
							break;
						case 'u':
						case 'U':
							DBWriteContactSettingStringUtf(NULL,szSection,szName,szValue+1);
							break;
						case 'n':
						case 'N':
							{	PBYTE buf;
								int len;
								char *pszValue,*pszEnd;
								DBCONTACTWRITESETTING cws;

								buf=(PBYTE)mir_alloc(lstrlenA(szValue+1));
								for(len=0,pszValue=szValue+1;;len++) {
									buf[len]=(BYTE)strtol(pszValue,&pszEnd,0x10);
									if(pszValue==pszEnd) break;
									pszValue=pszEnd;
								}
								cws.szModule=szSection;
								cws.szSetting=szName;
								cws.value.type=DBVT_BLOB;
								cws.value.pbVal=buf;
								cws.value.cpbVal=len;
								CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)(HANDLE)NULL,(LPARAM)&cws);
								mir_free(buf);
							}
							break;
						default:
							if(ServiceExists(MS_LANGPACK_TRANSLATESTRING))
								MessageBox(NULL,TranslateT("Invalid setting type. The first character of every value must be b, w, d, l, s or n."),TranslateT("Install Database Settings"),MB_OK);
							else
								MessageBoxA(NULL,"Invalid setting type. The first character of every value must be b, w, d, l, s or n.","Install Database Settings",MB_OK);
							break;
					}
				}
			}
		}
		fclose(fp);
		if(!lstrcmpiA(fd.cFileName,szOverrideSecurityFilename))
			DeleteFileA(szIniPath);
		else {
			char szOnCompletion[8];
			GetPrivateProfileStringA("AutoExec","OnCompletion","recycle",szOnCompletion,sizeof(szOnCompletion),mirandabootini);
			if(!lstrcmpiA(szOnCompletion,"delete"))
				DeleteFileA(szIniPath);
			else if(!lstrcmpiA(szOnCompletion,"recycle")) {
				SHFILEOPSTRUCTA shfo={0};
				shfo.wFunc=FO_DELETE;
				shfo.pFrom=szIniPath;
				szIniPath[lstrlenA(szIniPath)+1]='\0';
				shfo.fFlags=FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT;
				SHFileOperationA(&shfo);
			}
			else if(!lstrcmpiA(szOnCompletion,"rename")) {
				char szRenamePrefix[MAX_PATH];
				char szNewPath[MAX_PATH];
				GetPrivateProfileStringA("AutoExec","RenamePrefix","done_",szRenamePrefix,sizeof(szRenamePrefix),mirandabootini);
				lstrcpyA(szNewPath,szExpandedFindPath);
				lstrcatA(szNewPath,szRenamePrefix);
				lstrcatA(szNewPath,fd.cFileName);
				MoveFileA(szIniPath,szNewPath);
			}
			else if(!lstrcmpiA(szOnCompletion,"ask"))
				DialogBoxParam(hMirandaInst,MAKEINTRESOURCE(IDD_INIIMPORTDONE),NULL,IniImportDoneDlgProc,(LPARAM)szIniPath);
		}
	} while(FindNextFileA(hFind,&fd));
	FindClose(hFind);
}

static int CheckIniImportNow(WPARAM, LPARAM)
{
	DoAutoExec();
	FindNextChangeNotification(hIniChangeNotification);
	return 0;
}

int InitIni(void)
{
	char szMirandaDir[MAX_PATH];
	char *str2;

	bModuleInitialized = TRUE;

	DoAutoExec();
	GetModuleFileNameA(hMirandaInst,szMirandaDir,sizeof(szMirandaDir));
	str2=strrchr(szMirandaDir,'\\');
	if(str2!=NULL) *str2=0;
	hIniChangeNotification=FindFirstChangeNotificationA(szMirandaDir,0,FILE_NOTIFY_CHANGE_FILE_NAME);
	if(hIniChangeNotification!=INVALID_HANDLE_VALUE) {
		CreateServiceFunction("DB/Ini/CheckImportNow",CheckIniImportNow);
		CallService(MS_SYSTEM_WAITONHANDLE,(WPARAM)hIniChangeNotification,(LPARAM)"DB/Ini/CheckImportNow");
	}
	return 0;
}

void UninitIni(void)
{
	if ( !bModuleInitialized ) return;
	CallService(MS_SYSTEM_REMOVEWAIT,(WPARAM)hIniChangeNotification,0);
	FindCloseChangeNotification(hIniChangeNotification);
}
