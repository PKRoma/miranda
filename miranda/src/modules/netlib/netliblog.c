/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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
#include "netlib.h"
#include "../srfile/file.h"

#define MS_NETLIB_LOGWIN "Netlib/Log/Win"

extern struct NetlibUser **netlibUser;
extern int netlibUserCount;
extern HANDLE hConnectionHeaderMutex;

#define TIMEFORMAT_NONE         0
#define TIMEFORMAT_HHMMSS       1
#define TIMEFORMAT_MILLISECONDS 2
#define TIMEFORMAT_MICROSECONDS 3
struct {
	HWND   hwndOpts;
	int    toOutputDebugString;
	int    toFile;
	int    toLog;
	TCHAR* szPath;
	TCHAR* szFile;
	TCHAR* szUserFile;
	int    timeFormat;
	int    showUser;
	int    dumpSent,dumpRecv,dumpProxy;
	int    textDumps,autoDetectText;
	CRITICAL_SECTION cs;
	int    save;
} logOptions = {0};

typedef struct {
	const char* pszHead;
	const char* pszMsg;
} LOGMSG;

static __int64 mirandaStartTime,perfCounterFreq;
static int bIsActive = TRUE;
static HANDLE hLogEvent = NULL;

static const TCHAR* szTimeFormats[] =
{
	_T( "No times" ),
	_T( "Standard hh:mm:ss times" ),
	_T( "Times in milliseconds" ),
	_T( "Times in microseconds" )
};

static BOOL CALLBACK LogOptionsDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
	case WM_INITDIALOG:
		logOptions.hwndOpts=hwndDlg;
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg,IDC_DUMPRECV,logOptions.dumpRecv?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_DUMPSENT,logOptions.dumpSent?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_DUMPPROXY,logOptions.dumpProxy?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_TEXTDUMPS,logOptions.textDumps?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_AUTODETECTTEXT,logOptions.autoDetectText?BST_CHECKED:BST_UNCHECKED);
		{	int i;
			for( i=0; i < SIZEOF(szTimeFormats); i++ )
				SendDlgItemMessage(hwndDlg,IDC_TIMEFORMAT,CB_ADDSTRING,0,(LPARAM)TranslateTS( szTimeFormats[i] ));
		}
		SendDlgItemMessage(hwndDlg,IDC_TIMEFORMAT,CB_SETCURSEL,logOptions.timeFormat,0);
		CheckDlgButton(hwndDlg,IDC_SHOWNAMES,logOptions.showUser?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_TOOUTPUTDEBUGSTRING,logOptions.toOutputDebugString?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_TOFILE,logOptions.toFile?BST_CHECKED:BST_UNCHECKED);
		SetDlgItemText(hwndDlg,IDC_FILENAME,logOptions.szUserFile);
		SetDlgItemText(hwndDlg,IDC_PATH,logOptions.szFile);
		CheckDlgButton(hwndDlg,IDC_SHOWTHISDLGATSTART,DBGetContactSettingByte(NULL,"Netlib","ShowLogOptsAtStart",0)?BST_CHECKED:BST_UNCHECKED);
		{	DBVARIANT dbv;
			if(!DBGetContactSettingString(NULL,"Netlib","RunAtStart",&dbv)) {
				SetDlgItemTextA(hwndDlg,IDC_RUNATSTART,dbv.pszVal);
				DBFreeVariant(&dbv);
			}
		}
		logOptions.save = 0;
		{
			TVINSERTSTRUCT tvis = {0};
			int i;
			HWND hwndFilter = GetDlgItem(hwndDlg,IDC_FILTER);

			SetWindowLong(hwndFilter, GWL_STYLE, GetWindowLong(hwndFilter, GWL_STYLE) | (TVS_NOHSCROLL | TVS_CHECKBOXES));

			tvis.hParent=NULL;
			tvis.hInsertAfter=TVI_SORT;
			tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_STATE;
			tvis.item.stateMask=TVIS_STATEIMAGEMASK;

			for( i=0; i<netlibUserCount; i++ )
			{
				tvis.item.lParam=i;
				#ifdef  _UNICODE
					tvis.item.pszText=a2u( netlibUser[i]->user.szSettingsModule );
				#else
					tvis.item.pszText=netlibUser[i]->user.szSettingsModule;
				#endif
				tvis.item.state=INDEXTOSTATEIMAGEMASK( (netlibUser[i]->toLog) ? 2 : 1 );
				TreeView_InsertItem(hwndFilter, &tvis);
			}
			tvis.item.lParam=-1;
			tvis.item.pszText=_T("(NULL)");
			tvis.item.state=INDEXTOSTATEIMAGEMASK( (logOptions.toLog) ? 2 : 1 );
			TreeView_InsertItem(hwndFilter, &tvis);
		}
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
/*
		case IDC_DUMPRECV:
		case IDC_DUMPSENT:
		case IDC_DUMPPROXY:
		case IDC_TEXTDUMPS:
		case IDC_AUTODETECTTEXT:
		case IDC_TIMEFORMAT:
		case IDC_SHOWNAMES:
		case IDC_TOOUTPUTDEBUGSTRING:
		case IDC_TOFILE:
		case IDC_SHOWTHISDLGATSTART:
		case IDC_RUNATSTART:
			break;
*/
		case IDC_FILENAME:
			if(HIWORD(wParam)!=EN_CHANGE) break;
			if((HWND)lParam==GetFocus()) {
				CheckDlgButton(hwndDlg,IDC_TOFILE,BST_CHECKED);
			}
			{	TCHAR newpath[MAX_PATH+1];
				TCHAR path[MAX_PATH+1];
				GetWindowText( (HWND)lParam, newpath, MAX_PATH );
				// convert relative path to absolute if needed
				if( _tcsstr(newpath, _T(":")) ||    // X:
					_tcsstr(newpath, _T("\\\\")) || // \\UNC
					_tfullpath( path, newpath, MAX_PATH) == NULL )
					SetDlgItemText( hwndDlg, IDC_PATH, newpath );
				else {
					TCHAR drive[_MAX_DRIVE];
					TCHAR dir[_MAX_DIR];
					_tsplitpath(logOptions.szPath, drive, dir, NULL, NULL);
					if ( path[0] == _T('\\') )
						_sntprintf( path, MAX_PATH, _T("%s%s"), drive, newpath);
					else
						_sntprintf( path, MAX_PATH, _T("%s%s%s"), drive, dir, newpath);
					path[MAX_PATH]=0;
					SetDlgItemText( hwndDlg, IDC_PATH, path );
				}
			}
			break;
		case IDC_FILENAMEBROWSE:
		case IDC_RUNATSTARTBROWSE:
		{	TCHAR str[MAX_PATH+2];
			OPENFILENAME ofn={0};
			TCHAR filter[512],*pfilter;

			GetWindowText(GetWindow((HWND)lParam,GW_HWNDPREV),str,SIZEOF(str));
			ofn.lStructSize=OPENFILENAME_SIZE_VERSION_400;
			ofn.hwndOwner=hwndDlg;
			ofn.Flags=OFN_HIDEREADONLY;
			if (LOWORD(wParam)==IDC_FILENAMEBROWSE) {
				ofn.lpstrTitle=TranslateT("Select where log file will be created");
			} else {
				ofn.Flags|=OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
				ofn.lpstrTitle=TranslateT("Select program to be run");
			}
			_tcscpy(filter,TranslateT("All Files"));
			_tcscat(filter,_T(" (*)"));
			pfilter=filter+lstrlen(filter)+1;
			_tcscpy(pfilter,_T("*"));
			pfilter=pfilter+lstrlen(pfilter)+1;
			*pfilter='\0';
			ofn.lpstrFilter=filter;
			ofn.lpstrFile=str;
			ofn.nMaxFile=SIZEOF(str)-2;
			ofn.nMaxFileTitle=MAX_PATH;
			if (LOWORD(wParam)==IDC_FILENAMEBROWSE) {
				if(!GetSaveFileName(&ofn)) return 1;
			} else {
				if(!GetOpenFileName(&ofn)) return 1;
			}
			if(LOWORD(wParam)==IDC_RUNATSTARTBROWSE && _tcschr(str,' ')!=NULL) {
				MoveMemory(str+1,str,SIZEOF(str)-2);
				str[0]='"';
				lstrcat(str,_T("\""));
			}
			SetWindowText(GetWindow((HWND)lParam,GW_HWNDPREV),str);
			break;
		}
		case IDC_RUNNOW:
			{	TCHAR str[MAX_PATH+1];
				STARTUPINFO si={0};
				PROCESS_INFORMATION pi;
				GetDlgItemText(hwndDlg,IDC_RUNATSTART,str,MAX_PATH);
				si.cb=sizeof(si);
				if(str[0]) CreateProcess(NULL,str,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
			}
			break;
		case IDC_SAVE:
			logOptions.save = 1;
			//
		case IDOK:
		    {
			    TCHAR str[MAX_PATH+1];

   				GetWindowText(GetDlgItem(hwndDlg,IDC_RUNATSTART),str,MAX_PATH);
				DBWriteContactSettingTString(NULL,"Netlib","RunAtStart",str);
	   			DBWriteContactSettingByte(NULL,"Netlib","ShowLogOptsAtStart",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWTHISDLGATSTART));

				EnterCriticalSection(&logOptions.cs);

				mir_free(logOptions.szUserFile);
				GetWindowText(GetDlgItem(hwndDlg,IDC_FILENAME), str, MAX_PATH );
				logOptions.szUserFile = mir_tstrdup(str);

				mir_free(logOptions.szFile);
				GetWindowText(GetDlgItem(hwndDlg,IDC_PATH), str, MAX_PATH );
				logOptions.szFile = mir_tstrdup(str);

				logOptions.dumpRecv=IsDlgButtonChecked(hwndDlg,IDC_DUMPRECV);
				logOptions.dumpSent=IsDlgButtonChecked(hwndDlg,IDC_DUMPSENT);
				logOptions.dumpProxy=IsDlgButtonChecked(hwndDlg,IDC_DUMPPROXY);
				logOptions.textDumps=IsDlgButtonChecked(hwndDlg,IDC_TEXTDUMPS);
				logOptions.autoDetectText=IsDlgButtonChecked(hwndDlg,IDC_AUTODETECTTEXT);
				logOptions.timeFormat=SendDlgItemMessage(hwndDlg,IDC_TIMEFORMAT,CB_GETCURSEL,0,0);
				logOptions.showUser=IsDlgButtonChecked(hwndDlg,IDC_SHOWNAMES);
				logOptions.toOutputDebugString=IsDlgButtonChecked(hwndDlg,IDC_TOOUTPUTDEBUGSTRING);
				logOptions.toFile=IsDlgButtonChecked(hwndDlg,IDC_TOFILE);

				LeaveCriticalSection(&logOptions.cs);
			}
			{
				HWND hwndFilter = GetDlgItem(logOptions.hwndOpts, IDC_FILTER);
				TVITEM tvi={0};
				BOOL checked;

				tvi.mask=TVIF_HANDLE|TVIF_PARAM|TVIF_STATE|TVIF_TEXT;
				tvi.hItem=TreeView_GetRoot(hwndFilter);

				while(tvi.hItem)
				{
					TreeView_GetItem(hwndFilter,&tvi);
					checked = ((tvi.state&TVIS_STATEIMAGEMASK)>>12==2);

					if (tvi.lParam == -1) {
						logOptions.toLog = checked;
					if ( logOptions.save )
						DBWriteContactSettingDword(NULL,"Netlib","NLlog",checked);
					}
					else
					if (tvi.lParam < netlibUserCount) {
						netlibUser[tvi.lParam]->toLog = checked;
						if ( logOptions.save )
							DBWriteContactSettingDword(NULL,netlibUser[tvi.lParam]->user.szSettingsModule,"NLlog",checked);
						#ifdef  _UNICODE
							mir_free( tvi.pszText );
						#endif
					}

					tvi.hItem=TreeView_GetNextSibling(hwndFilter,tvi.hItem);
				}
			}

			if ( logOptions.save ) {
				DBWriteContactSettingByte(NULL,"Netlib","DumpRecv",(BYTE)logOptions.dumpRecv);
				DBWriteContactSettingByte(NULL,"Netlib","DumpSent",(BYTE)logOptions.dumpSent);
				DBWriteContactSettingByte(NULL,"Netlib","DumpProxy",(BYTE)logOptions.dumpProxy);
				DBWriteContactSettingByte(NULL,"Netlib","TextDumps",(BYTE)logOptions.textDumps);
				DBWriteContactSettingByte(NULL,"Netlib","AutoDetectText",(BYTE)logOptions.autoDetectText);
				DBWriteContactSettingByte(NULL,"Netlib","TimeFormat",(BYTE)logOptions.timeFormat);
				DBWriteContactSettingByte(NULL,"Netlib","ShowUser",(BYTE)logOptions.showUser);
				DBWriteContactSettingByte(NULL,"Netlib","ToOutputDebugString",(BYTE)logOptions.toOutputDebugString);
				DBWriteContactSettingByte(NULL,"Netlib","ToFile",(BYTE)logOptions.toFile);
				DBWriteContactSettingTString(NULL,"Netlib","File", logOptions.szFile ? logOptions.szUserFile: _T(""));
				logOptions.save = 0;
			}
			else
				DestroyWindow(hwndDlg);

			break;
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;
	case WM_DESTROY:
		logOptions.hwndOpts=NULL;
		break;
	}
	return FALSE;
}

void NetlibLogShowOptions(void)
{
	if(logOptions.hwndOpts==NULL)
		logOptions.hwndOpts=CreateDialog(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_NETLIBLOGOPTS),NULL,LogOptionsDlgProc);
	SetForegroundWindow(logOptions.hwndOpts);
}

static int ShowOptions(WPARAM wParam, LPARAM lParam)
{
	NetlibLogShowOptions();
	return 0;
}

static int NetlibLog(WPARAM wParam,LPARAM lParam)
{
	struct NetlibUser *nlu=(struct NetlibUser*)wParam;
	struct NetlibUser nludummy;
	const char *pszMsg=(const char*)lParam;
	char szTime[32], szHead[128];
	LARGE_INTEGER liTimeNow;
	DWORD dwOriginalLastError;

	if ( !bIsActive )
		return 0;

	if ((nlu != NULL && GetNetlibHandleType(nlu)!=NLH_USER) || pszMsg==NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}
	if (nlu==NULL) { /* if the Netlib user handle is NULL, just pretend its not */
		if ( !logOptions.toLog )
			return 1;
		nlu=&nludummy;
		nlu->user.szSettingsModule="(NULL)";
	}
	else if ( !(nlu->toLog) )
		return 1;

	dwOriginalLastError=GetLastError();
	QueryPerformanceCounter(&liTimeNow);
	liTimeNow.QuadPart-=mirandaStartTime;
	switch(logOptions.timeFormat) {
		case TIMEFORMAT_HHMMSS:
			GetTimeFormatA(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT|TIME_NOTIMEMARKER,NULL,NULL,szTime,SIZEOF(szTime)-1);
			break;
		case TIMEFORMAT_MILLISECONDS:
			mir_snprintf(szTime,SIZEOF(szTime)-1,"%I64u.%03I64u",liTimeNow.QuadPart/perfCounterFreq,1000*(liTimeNow.QuadPart%perfCounterFreq)/perfCounterFreq);
			break;
		case TIMEFORMAT_MICROSECONDS:
			mir_snprintf(szTime,SIZEOF(szTime)-1,"%I64u.%06I64u",liTimeNow.QuadPart/perfCounterFreq,1000000*(liTimeNow.QuadPart%perfCounterFreq)/perfCounterFreq);
			break;
		default:
			szTime[0]='\0';
			break;
	}
	if(logOptions.timeFormat || logOptions.showUser)
		mir_snprintf(szHead,SIZEOF(szHead)-1,"[%s%s%s] ",szTime,(logOptions.showUser&&logOptions.timeFormat)?" ":"",logOptions.showUser?nlu->user.szSettingsModule:"");
	else
		szHead[0]=0;

	EnterCriticalSection(&logOptions.cs);
	if(logOptions.toOutputDebugString) {
	    if (szHead[0])
			OutputDebugStringA(szHead);
		OutputDebugStringA(pszMsg);
		OutputDebugStringA("\n");
	}

	if(logOptions.toFile && logOptions.szFile[0]) {
		FILE *fp;
		fp = _tfopen(logOptions.szFile, _T("at"));
		if ( !fp ) {
			TCHAR* pszLastBackslash = _tcsrchr( logOptions.szFile, '\\' );
			if ( pszLastBackslash != NULL ) {
				*pszLastBackslash = '\0';
				CreateDirectoryTreeT( logOptions.szFile );
				*pszLastBackslash = '\\';
			}
			else CreateDirectoryTreeT( logOptions.szFile );
			fp = _tfopen(logOptions.szFile, _T("at"));
		}
		if ( fp ) {
			fprintf(fp,"%s%s\n", szHead, pszMsg);
			fclose(fp);
	}	}

	LeaveCriticalSection(&logOptions.cs);

	{
		LOGMSG logMsg = { szHead, pszMsg };
		CallHookSubscribers( hLogEvent, (WPARAM)nlu, (LPARAM)&logMsg );
	}

	SetLastError(dwOriginalLastError);
	return 1;
}

void NetlibDumpData(struct NetlibConnection *nlc,PBYTE buf,int len,int sent,int flags)
{
	int isText=1;
	char szTitleLine[128];
	char *szBuf;
	int titleLineLen;
	struct NetlibUser *nlu;

	// This section checks a number of conditions and aborts
	// the dump if the data should not be written to the log

	// Check packet flags
	if ((flags&MSG_PEEK) || (flags&MSG_NODUMP))
		return;

	// Check user's log settings
	if (!( logOptions.toOutputDebugString || 
		((THook*)hLogEvent)->subscriberCount || 
		( logOptions.toFile && logOptions.szFile[0] )))
		return;
	if ((sent && !logOptions.dumpSent) ||
		(!sent && !logOptions.dumpRecv))
		return;
	if ((flags&MSG_DUMPPROXY) && !logOptions.dumpProxy)
		return;

	WaitForSingleObject(hConnectionHeaderMutex, INFINITE);
	nlu = nlc ? nlc->nlu : NULL;
	titleLineLen = mir_snprintf(szTitleLine,SIZEOF(szTitleLine), "(%p:%u) Data %s%s\n", nlc, nlc?nlc->s:0, sent?"sent":"received", flags & MSG_DUMPPROXY?" (proxy)":"");
	ReleaseMutex(hConnectionHeaderMutex);

	// check filter settings
	if ( nlu && !(nlu->toLog) ) return;

	if (!logOptions.textDumps)
		isText = 0;
	else if (!(flags&MSG_DUMPASTEXT)) {
		if (logOptions.autoDetectText) {
			int i;
			for(i = 0; i<len; i++)
				if ((buf[i]<' ' && buf[i]!='\t' && buf[i]!='\r' && buf[i]!='\n') || buf[i]>=0x80)
				{
					isText = 0;
					break;
				}
		}
		else
			isText = 0;
	}

	// Text data
	if (isText) {
		szBuf = (char*)alloca(titleLineLen + len + 1);
		CopyMemory(szBuf, szTitleLine, titleLineLen);
		CopyMemory(szBuf + titleLineLen, (const char*)buf, len);
		szBuf[titleLineLen + len] = '\0';
	}
	// Binary data
	else {
		int line, col, colsInLine;
		char *pszBuf;

		szBuf = (char*)alloca(titleLineLen + ((len+16)>>4) * 76 + 1);
		CopyMemory(szBuf, szTitleLine, titleLineLen);
		pszBuf = szBuf + titleLineLen;
		for (line = 0; ; line += 16) {
			colsInLine = min(16, len - line);

			if (colsInLine == 16) {
				pszBuf += wsprintfA(pszBuf, "%08X: %02X %02X %02X %02X-%02X %02X %02X %02X-%02X %02X %02X %02X-%02X %02X %02X %02X  ",
									line,buf[line],buf[line+1],buf[line+2],buf[line+3],buf[line+4],buf[line+5],buf[line+6],buf[line+7],
									buf[line+8],buf[line+9],buf[line+10],buf[line+11],buf[line+12],buf[line+13],buf[line+14],buf[line+15]
									);
			}
			else {
				pszBuf += wsprintfA(pszBuf, "%08X: ", line);
				// Dump data as hex
				for (col = 0; col < colsInLine; col++)
					pszBuf += wsprintfA(pszBuf, "%02X%c", buf[line + col], ((col&3)==3 && col != 15)?'-':' ');
				// Fill out last line with blanks
				for ( ; col<16; col++)
					{
					lstrcpyA(pszBuf, "   ");
					pszBuf += 3;
				}
				*pszBuf++ = ' ';
			}

			for (col = 0; col < colsInLine; col++)
				*pszBuf++ = buf[line+col]<' '?'.':(char)buf[line+col];

			if (len-line<=16)
				break;

			*pszBuf++ = '\n'; // End each line with a break
		}
		*pszBuf = '\0';
	}

	NetlibLog((WPARAM)nlu,(LPARAM)szBuf);
}

void NetlibLogInit(void)
{
	DBVARIANT dbv;
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	perfCounterFreq=li.QuadPart;
	QueryPerformanceCounter(&li);
	mirandaStartTime=li.QuadPart;

	CreateServiceFunction( MS_NETLIB_LOGWIN, ShowOptions );
	CreateServiceFunction( MS_NETLIB_LOG, NetlibLog );
	hLogEvent = CreateHookableEvent( ME_NETLIB_FASTDUMP );

	InitializeCriticalSection(&logOptions.cs);
	logOptions.dumpRecv=DBGetContactSettingByte(NULL,"Netlib","DumpRecv",1);
	logOptions.dumpSent=DBGetContactSettingByte(NULL,"Netlib","DumpSent",1);
	logOptions.dumpProxy=DBGetContactSettingByte(NULL,"Netlib","DumpProxy",0);
	logOptions.textDumps=DBGetContactSettingByte(NULL,"Netlib","TextDumps",1);
	logOptions.autoDetectText=DBGetContactSettingByte(NULL,"Netlib","AutoDetectText",1);
	logOptions.timeFormat=DBGetContactSettingByte(NULL,"Netlib","TimeFormat",TIMEFORMAT_HHMMSS);
	logOptions.showUser=DBGetContactSettingByte(NULL,"Netlib","ShowUser",1);
	logOptions.toOutputDebugString=DBGetContactSettingByte(NULL,"Netlib","ToOutputDebugString",0);
	logOptions.toFile=DBGetContactSettingByte(NULL,"Netlib","ToFile",0);
	logOptions.toLog=DBGetContactSettingDword(NULL,"Netlib","NLlog",1);

	if(!DBGetContactSettingTString(NULL, "Netlib", "File", &dbv)) {
		TCHAR path[MAX_PATH+1];

	  	if(_tfullpath(path, dbv.ptszVal, MAX_PATH) == NULL)
			logOptions.szFile = mir_tstrdup(dbv.ptszVal);
		else
	    	logOptions.szFile = mir_tstrdup(path);

		logOptions.szUserFile = mir_tstrdup(dbv.ptszVal);

	  	if(_tfullpath(path, _T( "." ), MAX_PATH) == NULL)
			logOptions.szPath = mir_tstrdup(_T( "" ));
		else {
			_tcscat(path,_T("\\"));
			logOptions.szPath = mir_tstrdup(path);
		}

		DBFreeVariant(&dbv);
	}

	if(logOptions.toFile && logOptions.szFile[0]) {
		FILE *fp;
		fp = _tfopen(logOptions.szFile, _T("wt"));
		if(fp) fclose(fp);
	}

	if(DBGetContactSettingByte(NULL,"Netlib","ShowLogOptsAtStart",0))
		NetlibLogShowOptions();

	if(!DBGetContactSettingTString(NULL,"Netlib","RunAtStart",&dbv)) {
		STARTUPINFO si={0};
		PROCESS_INFORMATION pi;
		si.cb=sizeof(si);
		if(dbv.ptszVal[0]) CreateProcess(NULL,dbv.ptszVal,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
		DBFreeVariant(&dbv);
	}

}

void NetlibLogShutdown(void)
{
	bIsActive = FALSE;
	DestroyHookableEvent( hLogEvent );
	if ( IsWindow( logOptions.hwndOpts ))
		DestroyWindow( logOptions.hwndOpts );
	DeleteCriticalSection( &logOptions.cs );
	mir_free( logOptions.szFile );
	mir_free( logOptions.szPath );
	mir_free( logOptions.szUserFile );
}
