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
#include "commonheaders.h"
#include <io.h>
#include "file.h"

#define HM_RECVEVENT    (WM_USER+10)

static int CheckVirusScanned(HWND hwnd,struct FileDlgData *dat,int i)
{
	if(dat->send) return 1;
	if(dat->fileVirusScanned[i]) return 1;
	if(DBGetContactSettingByte(NULL,"SRFile","WarnBeforeOpening",1)==0) return 1;
	return IDYES==MessageBox(hwnd,TranslateT("This file has not yet been scanned for viruses. Are you certain you want to open it?"),TranslateT("File Received"),MB_YESNO|MB_DEFBUTTON2);
}

#define M_VIRUSSCANDONE  (WM_USER+100)
struct virusscanthreadstartinfo {
	char *szFile;
	int returnCode;
	HWND hwndReply;
};

static void SetOpenFileButtonStyle(HWND hwndButton,int enabled)
{
	EnableWindow(hwndButton,enabled);
}

static void __cdecl RunVirusScannerThread(struct virusscanthreadstartinfo *info)
{
	PROCESS_INFORMATION pi;
	STARTUPINFOA si={0};
	DBVARIANT dbv;
	char szCmdLine[768];

	if(!DBGetContactSettingString(NULL,"SRFile","ScanCmdLine",&dbv)) {
		if(dbv.pszVal[0]) {
			char *pszReplace;
			si.cb=sizeof(si);
			pszReplace=strstr(dbv.pszVal,"%f");
			if(pszReplace) {
				if(info->szFile[lstrlenA(info->szFile)-1]=='\\') info->szFile[lstrlenA(info->szFile)-1]='\0';
				*pszReplace=0;
				mir_snprintf(szCmdLine,SIZEOF(szCmdLine),"%s\"%s\"%s",dbv.pszVal,info->szFile,pszReplace+2);
			}
			else lstrcpynA(szCmdLine,dbv.pszVal,SIZEOF(szCmdLine));
			if(CreateProcessA(NULL,szCmdLine,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)) {
				if(WaitForSingleObject(pi.hProcess,3600*1000)==WAIT_OBJECT_0)
					PostMessage(info->hwndReply,M_VIRUSSCANDONE,info->returnCode,0);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
		}
		DBFreeVariant(&dbv);
	}
	mir_free(info->szFile);
	mir_free(info);
}

static void SetFilenameControls(HWND hwndDlg,PROTOFILETRANSFERSTATUS *fts)
{
	TCHAR msg[MAX_PATH];
	char *fn = NULL;

	if (fts->currentFile)
	{
		fn = strrchr(fts->currentFile, '\\');
		if (!fn) fn = fts->currentFile;
		else ++fn;
	}

	if (fn && (fts->totalFiles > 1))
	{
		mir_sntprintf(msg, SIZEOF(msg), _T("%s: ") _T(TCHAR_STR_PARAM) _T(" (%d %s %d)"),
			(TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)fts->hContact,GCDNF_TCHAR), fn,
			fts->currentFileNumber+1, TranslateT("of"), fts->totalFiles);
	} else
	if (fn)
	{
		mir_sntprintf(msg, SIZEOF(msg), _T("%s: ") _T(TCHAR_STR_PARAM),
			(TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)fts->hContact,GCDNF_TCHAR),
			fn);
	} else
	{
		lstrcpyn(msg, (TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)fts->hContact,GCDNF_TCHAR), SIZEOF(msg));
	}

	SetDlgItemText(hwndDlg, IDC_CONTACTNAME, msg);
}

static void SetFtStatus(HWND hwndDlg, TCHAR *text, BOOL bProgress)
{
	SetDlgItemText(hwndDlg,IDC_STATUS,TranslateTS(text));
	ShowWindow(GetDlgItem(hwndDlg,IDC_STATUS), bProgress?SW_HIDE:SW_SHOW);
	ShowWindow(GetDlgItem(hwndDlg,IDC_ALLFILESPROGRESS), bProgress?SW_SHOW:SW_HIDE);
}

static void HideProgressControls(HWND hwndDlg)
{
	RECT rc;
	GetWindowRect(GetDlgItem(hwndDlg, IDC_ALLPRECENTS), &rc);
	MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc, 2);
	SetWindowPos(hwndDlg, NULL, 0, 0, 100, rc.bottom+3, SWP_NOMOVE|SWP_NOZORDER);
	ShowWindow(GetDlgItem(hwndDlg, IDC_ALLTRANSFERRED), SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_ALLSPEED), SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_ALLPRECENTS), SW_HIDE);
	PostMessage(GetParent(hwndDlg), WM_FT_RESIZE, 0, (LPARAM)hwndDlg);
}

static int FileTransferDlgResizer(HWND hwndDlg,LPARAM lParam,UTILRESIZECONTROL *urc)
{
	switch(urc->wId) {
		case IDC_CONTACTNAME:
		case IDC_STATUS:
		case IDC_ALLFILESPROGRESS:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_TOP;
		case IDC_FRAME:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_BOTTOM;
		case IDC_ALLPRECENTAGE:
		case IDCANCEL:
		case IDC_OPENFILE:
			return RD_ANCHORX_RIGHT|RD_ANCHORY_TOP;

		case IDC_ALLTRANSFERRED:
			urc->rcItem.right = urc->rcItem.left + (urc->rcItem.right - urc->rcItem.left) / 3;
			return RD_ANCHORX_CUSTOM|RD_ANCHORY_CUSTOM;

		case IDC_ALLSPEED:
			urc->rcItem.right = urc->rcItem.right - urc->dlgOriginalSize.cx + urc->dlgNewSize.cx;
			urc->rcItem.left = urc->rcItem.left + (urc->rcItem.right - urc->rcItem.left) / 3;
			return RD_ANCHORX_CUSTOM|RD_ANCHORY_CUSTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

BOOL CALLBACK DlgProcFileTransfer(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct FileDlgData *dat=NULL;

	dat=(struct FileDlgData*)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (msg)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			dat = (struct FileDlgData*)lParam;
			SetWindowLong(hwndDlg, GWL_USERDATA, (LPARAM)dat);
			dat->hNotifyEvent=HookEventMessage(ME_PROTO_ACK,hwndDlg,HM_RECVEVENT);
			dat->transferStatus.currentFileNumber = -1;
			if(dat->send) {
				dat->fs=(HANDLE)CallContactService(dat->hContact,PSS_FILE,(WPARAM)dat->szMsg,(LPARAM)dat->files);
				SetFtStatus(hwndDlg, _T("Request sent, waiting for acceptance..."), FALSE);
				SetOpenFileButtonStyle(GetDlgItem(hwndDlg,IDC_OPENFILE),1);
				dat->waitingForAcceptance=1;
				// hide "open" button since it may cause potential access violations...
				ShowWindow(GetDlgItem(hwndDlg, IDC_OPENFILE), SW_HIDE);
			}
			else {	//recv
				CreateDirectoryTree(dat->szSavePath);
				dat->fs=(HANDLE)CallContactService(dat->hContact,PSS_FILEALLOW,(WPARAM)dat->fs,(LPARAM)dat->szSavePath);
				dat->transferStatus.workingDir=mir_strdup(dat->szSavePath);
				if(DBGetContactSettingByte(dat->hContact,"CList","NotOnList",0)) dat->resumeBehaviour=FILERESUME_ASK;
				else dat->resumeBehaviour=DBGetContactSettingByte(NULL,"SRFile","IfExists",FILERESUME_ASK);
				SetFtStatus(hwndDlg, _T("Waiting for connection..."), FALSE);
			}
			{
				/* check we actually got an fs handle back from the protocol */
				if (!dat->fs) {
					SetFtStatus(hwndDlg, _T("Unable to initiate transfer."), FALSE);
					dat->waitingForAcceptance=0;
				}
			}
			{	LOGFONT lf;
				HFONT hFont;
				hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CONTACTNAME,WM_GETFONT,0,0);
				GetObject(hFont,sizeof(lf),&lf);
				lf.lfWeight=FW_BOLD;
				hFont=CreateFontIndirect(&lf);
				SendDlgItemMessage(hwndDlg,IDC_CONTACTNAME,WM_SETFONT,(WPARAM)hFont,0);
			}

			SendDlgItemMessage( hwndDlg, IDC_CONTACT, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_USERDETAILS ));
			SendDlgItemMessage( hwndDlg, IDC_CONTACT, BUTTONADDTOOLTIP, (WPARAM)"Contact menu", 0);
			SendDlgItemMessage( hwndDlg, IDC_CONTACT, BUTTONSETASFLATBTN, 0, 0);

			SendDlgItemMessage( hwndDlg, IDC_OPENFILE, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_DOWNARROW ));
			SendDlgItemMessage( hwndDlg, IDC_OPENFILE, BUTTONADDTOOLTIP, (WPARAM)"Open...", 0);
			SendDlgItemMessage( hwndDlg, IDC_OPENFILE, BUTTONSETASFLATBTN, 0, 0);
			SendDlgItemMessage( hwndDlg, IDC_OPENFILE, BUTTONSETASPUSHBTN, 0, 0);

			SendDlgItemMessage( hwndDlg, IDCANCEL, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadSkinIcon( SKINICON_OTHER_DELETE ));
			SendDlgItemMessage( hwndDlg, IDCANCEL, BUTTONADDTOOLTIP, (WPARAM)"Cancel", 0);
			SendDlgItemMessage( hwndDlg, IDCANCEL, BUTTONSETASFLATBTN, 0, 0);

			SetDlgItemText(hwndDlg, IDC_CONTACTNAME, (TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)dat->hContact,GCDNF_TCHAR));

			if(!dat->waitingForAcceptance) SetTimer(hwndDlg,1,1000,NULL);
			return TRUE;
		case WM_TIMER:
			MoveMemory(dat->bytesRecvedHistory+1,dat->bytesRecvedHistory,sizeof(dat->bytesRecvedHistory)-sizeof(dat->bytesRecvedHistory[0]));
			dat->bytesRecvedHistory[0]=dat->transferStatus.totalProgress;
			if ( dat->bytesRecvedHistorySize < SIZEOF(dat->bytesRecvedHistory))
				dat->bytesRecvedHistorySize++;

			{	TCHAR szSpeed[32], szTime[32], szDisplay[96];
				SYSTEMTIME st;
				ULARGE_INTEGER li;
				FILETIME ft;

				GetSensiblyFormattedSize((dat->bytesRecvedHistory[0]-dat->bytesRecvedHistory[dat->bytesRecvedHistorySize-1])/dat->bytesRecvedHistorySize,szSpeed,SIZEOF(szSpeed),0,1,NULL);
				if(dat->bytesRecvedHistory[0]==dat->bytesRecvedHistory[dat->bytesRecvedHistorySize-1])
					lstrcpy(szTime,_T("??:??:??"));
				else {
					li.QuadPart=BIGI(10000000)*(dat->transferStatus.currentFileSize-dat->transferStatus.currentFileProgress)*dat->bytesRecvedHistorySize/(dat->bytesRecvedHistory[0]-dat->bytesRecvedHistory[dat->bytesRecvedHistorySize-1]);
					ft.dwHighDateTime=li.HighPart; ft.dwLowDateTime=li.LowPart;
					FileTimeToSystemTime(&ft,&st);
					GetTimeFormat(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT|TIME_NOTIMEMARKER,&st,NULL,szTime,SIZEOF(szTime));
				}
				if(dat->bytesRecvedHistory[0]!=dat->bytesRecvedHistory[dat->bytesRecvedHistorySize-1]) {
					li.QuadPart=BIGI(10000000)*(dat->transferStatus.totalBytes-dat->transferStatus.totalProgress)*dat->bytesRecvedHistorySize/(dat->bytesRecvedHistory[0]-dat->bytesRecvedHistory[dat->bytesRecvedHistorySize-1]);
					ft.dwHighDateTime=li.HighPart; ft.dwLowDateTime=li.LowPart;
					FileTimeToSystemTime(&ft,&st);
					GetTimeFormat(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT|TIME_NOTIMEMARKER,&st,NULL,szTime,SIZEOF(szTime));
				}

				mir_sntprintf(szDisplay,SIZEOF(szDisplay),_T("%s/%s  (%s %s)"),szSpeed,TranslateT("sec"),szTime,TranslateT("remaining"));
				SetDlgItemText(hwndDlg,IDC_ALLSPEED,szDisplay);
			}
			break;

		case WM_MEASUREITEM:
			return CallService(MS_CLIST_MENUMEASUREITEM,wParam,lParam);

		case WM_DRAWITEM:
			return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);

		case WM_COMMAND:
			if ( CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam),MPCF_CONTACTMENU), (LPARAM)dat->hContact ))
				break;

			switch(LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					PostMessage(GetParent(hwndDlg), WM_FT_REMOVE, 0, (LPARAM)hwndDlg);
					DestroyWindow(hwndDlg);
					break;

				case IDC_CONTACT:
					{	RECT rc;
						HMENU hMenu=(HMENU)CallService(MS_CLIST_MENUBUILDCONTACT,(WPARAM)dat->hContact,0);
						GetWindowRect((HWND)lParam,&rc);
						TrackPopupMenu(hMenu,0,rc.left,rc.bottom,0,hwndDlg,NULL);
						DestroyMenu(hMenu);
						break;
					}

				case IDC_OPENFILE:
				{
					char **files;
					HMENU hMenu;
					RECT rc;
					int ret;

					if (dat->send)
						if (dat->files == NULL)
							files = dat->transferStatus.files;
						else
							files = dat->files;
					else
						files=dat->files;

					hMenu = CreatePopupMenu();
					AppendMenu(hMenu, MF_STRING, 1, TranslateT("Open folder"));
					AppendMenu(hMenu, MF_SEPARATOR, 0, 0);

					if (files && *files)
					{
						int i, limit;
						char *pszFilename,*pszNewFileName;

						if (dat->send)
							limit = dat->transferStatus.totalFiles;
						else
							limit = dat->transferStatus.currentFileNumber;

						// Loop over all transfered files and add them to the menu
						for (i = 0; i < limit; i++) {
							pszFilename = strrchr(files[i], '\\');
							if (pszFilename == NULL)
								pszFilename = files[i];
							else
								pszFilename++;
							{
								if (pszFilename) {
									int pszlen;
									char *p;

									pszNewFileName = (char*)mir_alloc(strlen(pszFilename)*2);
									p = pszNewFileName;
									for (pszlen=0; pszlen<(int)strlen(pszFilename); pszlen++) {
										*p++ = pszFilename[pszlen];
										if (pszFilename[pszlen]=='&')
											*p++ = '&';
									}
									*p = '\0';
									AppendMenuA(hMenu, MF_STRING, i+10, pszNewFileName);
									mir_free(pszNewFileName);
								}
							}
						}
					}

					GetWindowRect((HWND)lParam, &rc);
					CheckDlgButton(hwndDlg, IDC_OPENFILE, BST_CHECKED);
					ret = TrackPopupMenu(hMenu, TPM_RETURNCMD|TPM_RIGHTALIGN, rc.right, rc.bottom, 0, hwndDlg, NULL);
					CheckDlgButton(hwndDlg, IDC_OPENFILE, BST_UNCHECKED);
					DestroyMenu(hMenu);

					if (ret == 1)
					{
						ShellExecuteA(NULL,NULL,dat->transferStatus.workingDir,NULL,NULL,SW_SHOW);
					} else
					if (ret && CheckVirusScanned(hwndDlg, dat, ret))
					{
						ShellExecuteA(NULL, NULL, files[ret-10], NULL, NULL, SW_SHOW);
					}

					break;
				}
			}
			break;
		case M_FILEEXISTSDLGREPLY:
		{	PROTOFILERESUME *pfr=(PROTOFILERESUME*)lParam;
			char *szOriginalFilename=(char*)wParam;
			char *szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)dat->hContact,0);

			EnableWindow(hwndDlg,TRUE);
			switch(pfr->action) {
				case FILERESUME_CANCEL:
					if (dat->fs) CallContactService(dat->hContact,PSS_FILECANCEL,(WPARAM)dat->fs,0);
					dat->fs=NULL;
					mir_free(szOriginalFilename);
					if(pfr->szFilename) mir_free((char*)pfr->szFilename);
					mir_free(pfr);
					return 0;
				case FILERESUME_RESUMEALL:
				case FILERESUME_OVERWRITEALL:
					dat->resumeBehaviour=pfr->action;
					pfr->action&=~FILERESUMEF_ALL;
					break;
				case FILERESUME_RENAMEALL:
					pfr->action=FILERESUME_RENAME;
					{	char *pszExtension,*pszFilename;
						int i;
						if((pszFilename=strrchr(szOriginalFilename,'\\'))==NULL) pszFilename=szOriginalFilename;
						if((pszExtension=strrchr(pszFilename+1,'.'))==NULL) pszExtension=pszFilename+lstrlenA(pszFilename);
						if(pfr->szFilename) mir_free((char*)pfr->szFilename);
						pfr->szFilename=(char*)mir_alloc((pszExtension-szOriginalFilename)+21+lstrlenA(pszExtension));
						for(i=1;;i++) {
							sprintf((char*)pfr->szFilename,"%.*s (%u)%s",pszExtension-szOriginalFilename,szOriginalFilename,i,pszExtension);
							if(_access(pfr->szFilename,0)!=0) break;
						}
					}
					break;
			}
			mir_free(szOriginalFilename);
			CallProtoService(szProto,PS_FILERESUME,(WPARAM)dat->fs,(LPARAM)pfr);
			if(pfr->szFilename) mir_free((char*)pfr->szFilename);
			mir_free(pfr);
			break;
		}
		case HM_RECVEVENT:
		{	ACKDATA *ack=(ACKDATA*)lParam;
			if (ack->hProcess!=dat->fs) break; /* icq abuses this sometimes */
			if(ack->hContact!=dat->hContact) break;
			if(ack->type!=ACKTYPE_FILE) break;

			if(dat->waitingForAcceptance) {
				SetTimer(hwndDlg,1,1000,NULL);
				dat->waitingForAcceptance=0;
			}
			switch(ack->result) {
				case ACKRESULT_SENTREQUEST: SetFtStatus(hwndDlg, _T("Decision sent"), FALSE); break;
				case ACKRESULT_CONNECTING: SetFtStatus(hwndDlg, _T("Connecting..."), FALSE); break;
				case ACKRESULT_CONNECTPROXY: SetFtStatus(hwndDlg, _T("Connecting to proxy..."), FALSE); break;
				case ACKRESULT_CONNECTED: SetFtStatus(hwndDlg, _T("Connected"), FALSE); break;
				case ACKRESULT_LISTENING: SetFtStatus(hwndDlg, _T("Waiting for connection..."), FALSE); break;
				case ACKRESULT_INITIALISING: SetFtStatus(hwndDlg, _T("Initialising..."), FALSE); break;
				case ACKRESULT_NEXTFILE:
					SetFtStatus(hwndDlg, _T("Moving to next file..."), FALSE);
					SetDlgItemTextA(hwndDlg,IDC_FILENAME,"");
					if(dat->transferStatus.currentFileNumber==1 && dat->transferStatus.totalFiles>1 && !dat->send)
						SetOpenFileButtonStyle(GetDlgItem(hwndDlg,IDC_OPENFILE),1);
					if(dat->transferStatus.currentFileNumber!=-1 && dat->files && !dat->send && DBGetContactSettingByte(NULL,"SRFile","UseScanner",VIRUSSCAN_DISABLE)==VIRUSSCAN_DURINGDL) {
						if(GetFileAttributesA(dat->files[dat->transferStatus.currentFileNumber])&FILE_ATTRIBUTE_DIRECTORY)
							PostMessage(hwndDlg,M_VIRUSSCANDONE,dat->transferStatus.currentFileNumber,0);
						else {
							struct virusscanthreadstartinfo *vstsi;
							vstsi=(struct virusscanthreadstartinfo*)mir_alloc(sizeof(struct virusscanthreadstartinfo));
							vstsi->hwndReply=hwndDlg;
							vstsi->szFile=mir_strdup(dat->files[dat->transferStatus.currentFileNumber]);
							vstsi->returnCode=dat->transferStatus.currentFileNumber;
							forkthread((void (*)(void*))RunVirusScannerThread,0,vstsi);
						}
					}
					break;
				case ACKRESULT_FILERESUME:
				{
					PROTOFILETRANSFERSTATUS *fts=(PROTOFILETRANSFERSTATUS*)ack->lParam;

					UpdateProtoFileTransferStatus(&dat->transferStatus,fts);
					SetFilenameControls(hwndDlg,fts);
					if(_access(fts->currentFile,0)!=0) break;
					SetFtStatus(hwndDlg, _T("File already exists"), FALSE);
					if(dat->resumeBehaviour==FILERESUME_ASK) {
						struct TDlgProcFileExistsParam param = { hwndDlg, fts };
						ShowWindow(hwndDlg,SW_SHOWNORMAL);
						CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_FILEEXISTS),hwndDlg,DlgProcFileExists,(LPARAM)&param);
						EnableWindow(hwndDlg,FALSE);
					}
					else {
						PROTOFILERESUME *pfr;
						pfr=(PROTOFILERESUME*)mir_alloc(sizeof(PROTOFILERESUME));
						pfr->action=dat->resumeBehaviour;
						pfr->szFilename=NULL;
						PostMessage(hwndDlg,M_FILEEXISTSDLGREPLY,(WPARAM)mir_strdup(fts->currentFile),(LPARAM)pfr);
					}
					SetWindowLong(hwndDlg,DWL_MSGRESULT,1);
					return TRUE;
				}
				case ACKRESULT_DATA:
				{
					PROTOFILETRANSFERSTATUS *fts=(PROTOFILETRANSFERSTATUS*)ack->lParam;
					TCHAR str[256],szSizeDone[32],szSizeTotal[32];//,*contactName;
					int units;

					if ( dat->fileVirusScanned==NULL )
						dat->fileVirusScanned=(int*)mir_calloc(sizeof(int) * fts->totalFiles);

					// This needs to be here - otherwise we get holes in the files array
					if ( !dat->send ) {
						if ( dat->files == NULL )
							dat->files = ( char** )mir_calloc(( fts->totalFiles+1 )*sizeof( char* ));
						if ( fts->currentFileNumber < fts->totalFiles && dat->files[ fts->currentFileNumber ] == NULL )
							dat->files[ fts->currentFileNumber ] = mir_strdup( fts->currentFile );
					}

					/* HACK: for 0.3.3, limit updates to around 1.1 ack per second */
					if (fts->totalProgress!=fts->totalBytes && GetTickCount() - dat->dwTicks < 650) break; // the last update was less than a second ago!
					dat->dwTicks=GetTickCount();

					// Update local transfer status with data from protocol
					UpdateProtoFileTransferStatus(&dat->transferStatus,fts);

					SetFtStatus(hwndDlg, fts->sending?_T("Sending..."):_T("Receiving..."), TRUE);
					SetFilenameControls(hwndDlg,fts);
					//SendDlgItemMessage(hwndDlg,IDC_CURRENTFILEPROGRESS, PBM_SETPOS, fts->currentFileSize?(WPARAM)(BIGI(100)*fts->currentFileProgress/fts->currentFileSize):0, 0);
					SendDlgItemMessage(hwndDlg,IDC_ALLFILESPROGRESS, PBM_SETPOS, fts->totalBytes?(WPARAM)(BIGI(100)*fts->totalProgress/fts->totalBytes):0, 0);

					GetSensiblyFormattedSize(fts->totalBytes,szSizeTotal,SIZEOF(szSizeTotal),0,1,&units);
					GetSensiblyFormattedSize(fts->totalProgress,szSizeDone,SIZEOF(szSizeDone),units,0,NULL);
					mir_sntprintf(str,SIZEOF(str),_T("%s/%s"),szSizeDone,szSizeTotal);
					SetDlgItemText(hwndDlg,IDC_ALLTRANSFERRED,str);

					mir_sntprintf(str,SIZEOF(str),_T("%d%%"),fts->totalBytes?(int)(BIGI(100)*fts->totalProgress/fts->totalBytes):0);
					SetDlgItemText(hwndDlg,IDC_ALLPRECENTAGE,str);
					break;
				}
				case ACKRESULT_SUCCESS:
				{
					HideProgressControls(hwndDlg);
					dat->fs=NULL; /* protocol will free structure */
					SetFtStatus(hwndDlg, _T("Transfer completed"), FALSE);
					if (ack->result==ACKRESULT_SUCCESS) SkinPlaySound("FileDone");
					if(!dat->send) {	//receiving
						int useScanner=DBGetContactSettingByte(NULL,"SRFile","UseScanner",VIRUSSCAN_DISABLE);
						if(useScanner!=VIRUSSCAN_DISABLE) {
							struct virusscanthreadstartinfo *vstsi;
							vstsi=(struct virusscanthreadstartinfo*)mir_alloc(sizeof(struct virusscanthreadstartinfo));
							vstsi->hwndReply=hwndDlg;
							if(useScanner==VIRUSSCAN_DURINGDL) {
								vstsi->returnCode=dat->transferStatus.currentFileNumber;
								if(GetFileAttributesA(dat->files[dat->transferStatus.currentFileNumber])&FILE_ATTRIBUTE_DIRECTORY) {
									PostMessage(hwndDlg,M_VIRUSSCANDONE,vstsi->returnCode,0);
									mir_free(vstsi);
									vstsi=NULL;
								}
								else vstsi->szFile=mir_strdup(dat->files[dat->transferStatus.currentFileNumber]);
							}
							else {
								vstsi->szFile=mir_strdup(dat->transferStatus.workingDir);
								vstsi->returnCode=-1;
							}
							SetFtStatus(hwndDlg, _T("Scanning for viruses..."), FALSE);
							if(vstsi) forkthread((void (*)(void*))RunVirusScannerThread,0,vstsi);
						}
						dat->transferStatus.currentFileNumber=dat->transferStatus.totalFiles;
					}
					else {	 //sending
						DBEVENTINFO dbei={0};
						dbei.cbSize=sizeof(dbei);
						dbei.szModule=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)dat->hContact,0);
						dbei.eventType=EVENTTYPE_FILE;
						dbei.flags=DBEF_SENT;
						dbei.timestamp=time(NULL);
						dbei.cbBlob=sizeof(DWORD)+lstrlenA(dat->szFilenames)+lstrlenA(dat->szMsg)+2;
						dbei.pBlob=(PBYTE)mir_alloc(dbei.cbBlob);
						*(PDWORD)dbei.pBlob=0;
						lstrcpyA(dbei.pBlob+sizeof(DWORD),dat->szFilenames);
						lstrcpyA(dbei.pBlob+sizeof(DWORD)+lstrlenA(dat->szFilenames)+1,dat->szMsg);
						CallService(MS_DB_EVENT_ADD,(WPARAM)dat->hContact,(LPARAM)&dbei);
						if (dbei.pBlob)
							mir_free(dbei.pBlob);
						dat->files=NULL;   //protocol library frees this
					}
				}
					//fall through
				case ACKRESULT_FAILED:
					HideProgressControls(hwndDlg);
					dat->fs=NULL; /* protocol will free structure */
					KillTimer(hwndDlg,1);
					if(!dat->send) SetOpenFileButtonStyle(GetDlgItem(hwndDlg,IDC_OPENFILE),1);
					SetDlgItemText(hwndDlg,IDCANCEL,TranslateT("Close"));
					if (dat->hNotifyEvent) UnhookEvent(dat->hNotifyEvent);
					dat->hNotifyEvent=NULL;
					if(ack->result==ACKRESULT_FAILED) {
						SetFtStatus(hwndDlg, _T("File transfer failed"), FALSE);
						SkinPlaySound("FileFailed");
					} else
						if(DBGetContactSettingByte(NULL,"SRFile","AutoClose",0))
							PostMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDCANCEL,BN_CLICKED),(LPARAM)GetDlgItem(hwndDlg,IDCANCEL));
					break;

				case ACKRESULT_DENIED:
					HideProgressControls(hwndDlg);
					dat->fs=NULL; /* protocol will free structure */
					SkinPlaySound("FileDenied");
					KillTimer(hwndDlg, 1);
					if (!dat->send) SetOpenFileButtonStyle(GetDlgItem(hwndDlg, IDC_OPENFILE), 1);
					SetDlgItemText(hwndDlg, IDCANCEL, TranslateT("Close"));
					SetFtStatus(hwndDlg, _T("File transfer denied"), FALSE);
					UnhookEvent(dat->hNotifyEvent);
					dat->hNotifyEvent = NULL;
					break;

			}
			break;
		}
		case M_VIRUSSCANDONE:
		{	int done=1,i;
			if((int)wParam==-1) {
				for(i=0;i<dat->transferStatus.totalFiles;i++) dat->fileVirusScanned[i]=1;
			}
			else {
				dat->fileVirusScanned[wParam]=1;
				for(i=0;i<dat->transferStatus.totalFiles;i++) if(!dat->fileVirusScanned[i]) {done=0; break;}
			}
			if(done) SetFtStatus(hwndDlg, _T("Transfer and virus scan complete"), FALSE);
			break;
		}
		case WM_SIZE:
		{
			UTILRESIZEDIALOG urd={0};
			urd.cbSize=sizeof(urd);
			urd.hwndDlg=hwndDlg;
			urd.hInstance=GetModuleHandle(NULL);
			urd.lpTemplate=MAKEINTRESOURCEA(IDD_FILETRANSFERINFO);
			urd.pfnResizer=FileTransferDlgResizer;
			CallService(MS_UTILS_RESIZEDIALOG,0,(LPARAM)&urd);

			RedrawWindow(GetDlgItem(hwndDlg, IDC_ALLTRANSFERRED), NULL, NULL, RDW_INVALIDATE|RDW_NOERASE);
			RedrawWindow(GetDlgItem(hwndDlg, IDC_ALLSPEED), NULL, NULL, RDW_INVALIDATE|RDW_NOERASE);
			RedrawWindow(GetDlgItem(hwndDlg, IDC_CONTACTNAME), NULL, NULL, RDW_INVALIDATE|RDW_NOERASE);
			RedrawWindow(GetDlgItem(hwndDlg, IDC_STATUS), NULL, NULL, RDW_INVALIDATE|RDW_NOERASE);
			break;
		}
		case WM_DESTROY:
			KillTimer(hwndDlg,1);
			if(dat->fs) CallContactService(dat->hContact,PSS_FILECANCEL,(WPARAM)dat->fs,0);
			dat->fs=NULL;
			if(dat->hNotifyEvent) {UnhookEvent(dat->hNotifyEvent); dat->hNotifyEvent=NULL;}
			FreeProtoFileTransferStatus(&dat->transferStatus);
			if(!dat->send) FreeFilesMatrix(&dat->files);
			if(dat->fileVirusScanned) mir_free(dat->fileVirusScanned);
			{	HFONT hFont;
				hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CONTACTNAME,WM_GETFONT,0,0);
				DeleteObject(hFont);
			}
			if (dat->send)
				FreeFilesMatrix(&dat->files);
			mir_free(dat);
			break;
	}
	return FALSE;
}
