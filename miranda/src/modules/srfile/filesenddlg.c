/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project,
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
#include <sys/types.h>
#include <sys/stat.h>
#include "file.h"

static void SetFileListAndSizeControls(HWND hwndDlg,struct FileDlgData *dat)
{
	int fileCount=0,dirCount=0,totalSize=0,i;
	struct _stat statbuf;
	TCHAR str[64];

	for(i=0;dat->files[i];i++) {
		if(_stat(dat->files[i],&statbuf)==0) {
			if(statbuf.st_mode&_S_IFDIR) dirCount++;
			else fileCount++;
			totalSize+=statbuf.st_size;
		}
	}
	GetSensiblyFormattedSize(totalSize,str,SIZEOF(str),0,1,NULL);
	SetDlgItemText(hwndDlg,IDC_TOTALSIZE,str);
	if(i>1) {
		TCHAR szFormat[32];
		if(fileCount && dirCount) {
			mir_sntprintf(szFormat,SIZEOF(szFormat),_T("%s, %s"),TranslateTS(fileCount==1?_T("%d file"):_T("%d files")),TranslateTS(dirCount==1?_T("%d directory"):_T("%d directories")));
			mir_sntprintf(str,SIZEOF(str),szFormat,fileCount,dirCount);
		}
		else if(fileCount) {
			lstrcpy(szFormat,TranslateT("%d files"));
			mir_sntprintf(str,SIZEOF(str),szFormat,fileCount);
		}
		else {
			lstrcpy(szFormat,TranslateT("%d directories"));
			mir_sntprintf(str,SIZEOF(str),szFormat,dirCount);
		}
		SetDlgItemText(hwndDlg,IDC_FILE,str);
	}
	else SetDlgItemTextA(hwndDlg,IDC_FILE,dat->files[0]);
}

static void FilenameToFileList(HWND hwndDlg, struct FileDlgData *dat, const char *buf)
{

	DWORD dwFileAttributes;

	// Make sure that the file matrix is empty (the user may select files several times)
	FreeFilesMatrix(&dat->files);

	// Get the file attributes of selection
	dwFileAttributes = GetFileAttributesA(buf);
	if (dwFileAttributes != INVALID_FILE_ATTRIBUTES)
	{
		// Check if the selection is a directory or a file
		if (GetFileAttributesA(buf) & FILE_ATTRIBUTE_DIRECTORY)
		{
			char *pBuf;
			int nNumberOfFiles = 0;
			int nTemp;
			int fileOffset;

			// :NOTE: The first string in the buffer is the directory, followed by a
			// NULL separated list of all files

			// fileOffset is the offset to the first file.
			fileOffset = lstrlenA(buf) + 1;

			// Count number of files
			pBuf = (char*)buf + fileOffset;
			while (*pBuf)
			{
				pBuf += lstrlenA(pBuf) + 1;
				nNumberOfFiles++;
			}

			// Allocate memory for a pointer array
			dat->files = (char**)malloc((nNumberOfFiles + 1) * sizeof(char*));

			// Fill the array
			pBuf = (char*)buf + fileOffset;
			nTemp = 0;
			while(*pBuf)
			{
				// Allocate space for path+filename
				dat->files[nTemp] = malloc(fileOffset + lstrlenA(pBuf) + 1);

				// Add path to filename and copy into array
				CopyMemory(dat->files[nTemp], buf, fileOffset - 1);
				dat->files[nTemp][fileOffset-1] = '\\';
				strcpy(dat->files[nTemp] + fileOffset - (buf[fileOffset-2]=='\\'?1:0), pBuf);
				// Move pointers to next file...
				pBuf += lstrlenA(pBuf) + 1;
				nTemp++;
			}
			// Teminate array
			dat->files[nNumberOfFiles] = NULL;
		}
		// ...the selection is a single file
		else
		{
			dat->files = (char**)malloc(2 * sizeof(char*)); // Leaks when aborted
			dat->files[0] = _strdup(buf);
			dat->files[1] = NULL;
		}
	}

	// Update dialog text with new file selection
	SetFileListAndSizeControls(hwndDlg, dat);

}

#define M_FILECHOOSEDONE  (WM_USER+100)
void __cdecl ChooseFilesThread(HWND hwndDlg)
{
	char *buf;
	OPENFILENAMEA ofn={0};
	char filter[128],*pfilter;

	buf=(char*)malloc(32767);
	buf[0]=0;
	ofn.lStructSize=OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner=hwndDlg;
	lstrcpyA(filter,Translate("All Files"));
	lstrcatA(filter," (*)");
	pfilter=filter+strlen(filter)+1;
	lstrcpyA(pfilter,"*");
	pfilter=filter+strlen(filter)+1;
	pfilter[0]='\0';
	ofn.lpstrFilter=filter;
	ofn.lpstrFile=buf;
	ofn.nMaxFile=32767;
	ofn.Flags=OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY;
	if(GetOpenFileNameA(&ofn))
		PostMessage(hwndDlg,M_FILECHOOSEDONE,0,(LPARAM)buf);
	else {
		free(buf);
		PostMessage(hwndDlg,M_FILECHOOSEDONE,0,(LPARAM)(char*)NULL);
	}
}

static BOOL CALLBACK ClipSiblingsChildEnumProc(HWND hwnd,LPARAM lParam)
{
	SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)|WS_CLIPSIBLINGS);
	return TRUE;
}

static WNDPROC OldSendEditProc;
static LRESULT CALLBACK SendEditSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) {
		case WM_CHAR:
			if(wParam=='\n' && GetKeyState(VK_CONTROL)&0x8000) {
				PostMessage(GetParent(hwnd),WM_COMMAND,IDOK,0);
				return 0;
			}
			break;
		case WM_SYSCHAR:
			if((wParam=='s' || wParam=='S') && GetKeyState(VK_MENU)&0x8000) {
				PostMessage(GetParent(hwnd),WM_COMMAND,IDOK,0);
				return 0;
			}
			break;
	}
	return CallWindowProc(OldSendEditProc,hwnd,msg,wParam,lParam);
}

BOOL CALLBACK DlgProcSendFile(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct FileDlgData *dat;

	dat=(struct FileDlgData*)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (msg)
	{
		case WM_INITDIALOG:
		{	char *contactName;
			struct FileSendData *fsd=(struct FileSendData*)lParam;

			dat=(struct FileDlgData*)malloc(sizeof(struct FileDlgData));
			memset(dat,0,sizeof(struct FileDlgData));
			SetWindowLong(hwndDlg,GWL_USERDATA,(long)dat);
			dat->hContact=fsd->hContact;
			dat->send=1;
			dat->hPreshutdownEvent=HookEventMessage(ME_SYSTEM_PRESHUTDOWN,hwndDlg,M_PRESHUTDOWN);
			dat->fs=NULL;
			dat->dwTicks=GetTickCount();

			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_FILE));
			EnumChildWindows(hwndDlg,ClipSiblingsChildEnumProc,0);
			OldSendEditProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_MSG),GWL_WNDPROC,(LONG)SendEditSubclassProc);

			dat->hUIIcons[0]=LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_USERDETAILS),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
			dat->hUIIcons[1]=LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_HISTORY),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
			dat->hUIIcons[2]=LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_DOWNARROW),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0);
			SendDlgItemMessage(hwndDlg,IDC_DETAILS,BM_SETIMAGE,IMAGE_ICON,(LPARAM)dat->hUIIcons[0]);
			SendDlgItemMessage(hwndDlg,IDC_HISTORY,BM_SETIMAGE,IMAGE_ICON,(LPARAM)dat->hUIIcons[1]);
			SendDlgItemMessage(hwndDlg,IDC_USERMENU,BM_SETIMAGE,IMAGE_ICON,(LPARAM)dat->hUIIcons[2]);
			SendDlgItemMessage(hwndDlg,IDC_DETAILS,BUTTONSETASFLATBTN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_HISTORY,BUTTONSETASFLATBTN,0,0);
			SendDlgItemMessage(hwndDlg,IDC_USERMENU,BUTTONSETASFLATBTN,0,0);
			SendMessage(GetDlgItem(hwndDlg,IDC_USERMENU), BUTTONADDTOOLTIP, (WPARAM)Translate("User Menu"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_DETAILS), BUTTONADDTOOLTIP, (WPARAM)Translate("View User's Details"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_HISTORY), BUTTONADDTOOLTIP, (WPARAM)Translate("View User's History"), 0);

			if(fsd->ppFiles!=NULL && fsd->ppFiles[0]!=NULL) {
				int totalCount,i;
				for(totalCount=0;fsd->ppFiles[totalCount];totalCount++);
				dat->files=(char**)malloc(sizeof(char*)*(totalCount+1)); // Leaks
				for(i=0;i<totalCount;i++)
					dat->files[i]=_strdup(fsd->ppFiles[i]);
				dat->files[totalCount]=NULL;
				SetFileListAndSizeControls(hwndDlg,dat);
			}

			contactName=(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)dat->hContact,0);
			SetDlgItemTextA(hwndDlg,IDC_TO,contactName);
			{
				char *szProto;

				szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)dat->hContact,0);
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
								free(ci.pszVal);
								break;
							case CNFT_DWORD:
								hasName = 1;
								mir_snprintf(buf, SIZEOF(buf),"%u",ci.dVal);
								break;
						}
					}
					SetDlgItemTextA(hwndDlg,IDC_NAME,hasName?buf:contactName);
				}
			}
			if(fsd->ppFiles==NULL) {
				dat->closeIfFileChooseCancelled=1;
				PostMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDC_CHOOSE,BN_CLICKED),(LPARAM)GetDlgItem(hwndDlg,IDC_CHOOSE));
			}
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
					}
				}
			}
			return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);
		}
		case M_FILECHOOSEDONE:
			if((char*)lParam) {
				FilenameToFileList(hwndDlg,dat,(char*)lParam);
				free((char*)lParam);
				dat->closeIfFileChooseCancelled=0;
			}
			else if(dat->closeIfFileChooseCancelled) DestroyWindow(hwndDlg);
			EnableWindow(hwndDlg,TRUE);
			break;
		case WM_COMMAND:
			if(CallService(MS_CLIST_MENUPROCESSCOMMAND,MAKEWPARAM(LOWORD(wParam),MPCF_CONTACTMENU),(LPARAM)dat->hContact))
				break;
			switch (LOWORD(wParam))
			{
				case IDC_CHOOSE:
					EnableWindow(hwndDlg,FALSE);
					//GetOpenFileName() creates its own message queue which prevents any incoming events being processed
					forkthread(ChooseFilesThread,0,hwndDlg);
					break;
				case IDOK:
					if(dat->hwndTransfer) return SendMessage(dat->hwndTransfer,msg,wParam,lParam);
					EnableWindow(GetDlgItem(hwndDlg,IDC_FILENAME),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_MSG),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_CHOOSE),FALSE);
					dat->hwndTransfer=CreateDialog(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_FILETRANSFERINFO),hwndDlg,DlgProcFileTransfer);
					return TRUE;
				case IDCANCEL:
					if(dat->hwndTransfer) return SendMessage(dat->hwndTransfer,msg,wParam,lParam);
					DestroyWindow(hwndDlg);
					return TRUE;
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
		case M_PRESHUTDOWN:
		{
			if (IsWindow(dat->hwndTransfer)) PostMessage(dat->hwndTransfer,WM_CLOSE,0,0);
			break;
		}
		case WM_DESTROY:
			if(dat->hPreshutdownEvent) UnhookEvent(dat->hPreshutdownEvent);
			if(dat->hwndTransfer) DestroyWindow(dat->hwndTransfer);
			FreeFilesMatrix(&dat->files);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_MSG),GWL_WNDPROC,(LONG)OldSendEditProc);
			DestroyIcon(dat->hUIIcons[2]);
			DestroyIcon(dat->hUIIcons[1]);
			DestroyIcon(dat->hUIIcons[0]);
			free(dat);
			return TRUE;
	}
	return FALSE;
}
